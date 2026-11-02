import AppKit
import SwiftUI
import UniformTypeIdentifiers
import ServiceManagement
import ChatCore

@MainActor final class AppModel: ObservableObject {
    static let shared = AppModel()
    @Published var status = "Mostík je vypnutý"
    @Published var error: String?
    @Published var running = false
    @Published var busy = false
    @Published var spaces: [Space] = []
    @Published var selected = Set<String>()
    @Published var deviceHost = "Displej nie je spárovaný"
    @Published var pairAddress = "meteoplaneradar.local"
    @Published var pairCode = ""
    @Published var hasClient = false
    @Published var signedIn = false
    @Published var lastContact: Date?
    @Published var delivered = 0
    @Published var launchAtLogin = SMAppService.mainApp.status == .enabled
    private(set) var storageReady = true
    private(set) var bundledClient: OAuthClient?
    var usesBundledClient: Bool { bundledClient != nil && service.secrets.client?.client_id == bundledClient?.client_id }
    private var service: ChatService
    private var worker: Task<Void, Never>?
    private var operation: Task<Void, Never>?
    private var window: NSWindow?
    var configured: Bool { service.secrets.pair != nil && signedIn && !selected.isEmpty }
    var hasPair: Bool { service.secrets.pair != nil }

    private init() {
        var secrets = Secrets(), preferences = Preferences(), failure: String?
        do {
            secrets = try Vault.read(); preferences = try LocalState.read()
            if let url = Bundle.main.url(forResource: "GoogleOAuth", withExtension: "json") {
                bundledClient = try OAuthConfiguration.decode(Data(contentsOf: url))
                secrets = OAuthConfiguration.applying(bundledClient, to: secrets)
            }
            if preferences.accountID != secrets.selfID {
                preferences = Preferences(); preferences.accountID = secrets.selfID
                try LocalState.write(preferences)
            }
        }
        catch { failure = "Nastavenia sa nepodarilo načítať. Odomknite Kľúčenku a spustite aplikáciu znova. Dáta sa neprepísali." }
        service = ChatService(secrets: secrets, preferences: preferences,
                              saveSecrets: Vault.write, savePreferences: LocalState.write)
        if let failure { storageReady = false; error = failure }
        if let saved = secrets.pair, var address = URLComponents(string: saved.device) {
            address.path = ""; pairAddress = address.string ?? "meteoplaneradar.local"
        }
        sync()
    }
    func launch() {
        if storageReady && service.preferences.enabled && configured { start() }
        if !configured || !storageReady { openSettings() }
    }
    private func sync() {
        deviceHost = service.secrets.pair?.host ?? "Displej nie je spárovaný"
        hasClient = service.secrets.client != nil; signedIn = service.secrets.token != nil
        selected = Set(service.preferences.spaces.map(\.id))
        if spaces.isEmpty { spaces = service.preferences.spaces }
    }
    private func describe(_ e: Error) -> String {
        if let e = e as? BridgeError { return e.localizedDescription }
        if e is CancellationError || (e as NSError).code == NSURLErrorCancelled { return "Operácia bola zrušená." }
        // Never surface URLs, response bodies, OAuth codes or credentials.
        return "Spojenie alebo odpoveď služby zlyhali. Skontrolujte sieť a skúste to znova."
    }
    private func perform(_ job: @escaping () async throws -> Void) {
        guard !busy && storageReady else { return }
        busy = true; error = nil
        operation = Task {
            defer { busy = false; operation = nil; sync() }
            do { try await job() } catch { self.error = describe(error) }
        }
    }
    func claimPair() {
        guard !running else { return }
        let address = pairAddress, code = pairCode
        perform {
            let pair = try await Pairing.claim(address: address, code: code)
            var next = self.service.secrets; next.pair = pair
            try Vault.write(next); self.service.secrets = next
            self.pairCode = ""; self.sync()
            self.status = "Displej bol spárovaný kódom. Môžete overiť spojenie."
        }
    }
    func importPair() {
        guard !running else { return }
        chooseFile { data in
            let pair = try JSONDecoder().decode(Pairing.self, from: data); try pair.validate()
            var next = self.service.secrets; next.pair = pair
            try Vault.write(next); self.service.secrets = next; self.sync()
            self.status = "Displej je spárovaný. Môžete overiť spojenie."
        }
    }
    func importClient() {
        guard !running else { return }
        chooseFile { data in
            let client = try OAuthClient.decodeFile(data)
            try self.installClient(client)
            self.sync(); self.status = "OAuth klient je pripravený. Prihláste sa cez prehliadač."
        }
    }
    private func installClient(_ client: OAuthClient) throws {
        defer { sync() }
        if service.secrets.client != client {
            var next = service.secrets; next.client = client; next.token = nil; next.selfID = nil
            let p = Preferences()
            try LocalState.write(p); service.preferences = p; spaces = []
            try Vault.write(next); service.secrets = next
        }
        sync()
    }
    func useDefaultClient() {
        guard !running && !busy, let client = bundledClient else { return }
        do { try installClient(client); status = "Predvolené Google prihlásenie je pripravené."; error = nil }
        catch { self.error = describe(error) }
    }
    private func chooseFile(_ action: (Data) throws -> Void) {
        guard !busy && storageReady else { return }
        let picker = NSOpenPanel(); picker.allowedContentTypes = [.json]
        picker.allowsMultipleSelection = false; picker.canChooseDirectories = false
        if picker.runModal() == .OK, let url = picker.url {
            do {
                let size = try url.resourceValues(forKeys: [.fileSizeKey]).fileSize ?? 0
                guard size <= 1024 * 1024 else { throw BridgeError.message("Vybraný súbor je príliš veľký.") }
                try action(Data(contentsOf: url)); error = nil
            } catch { self.error = describe(error) }
        }
    }
    func signIn() {
        guard !running, let client = service.secrets.client else { return }
        perform {
            self.status = "Dokončite prihlásenie v prehliadači…"
            let flow = GoogleOAuth()
            let token = try await flow.signIn(client: client)
            // Resolve account identity before replacing a working account.
            var request = URLRequest(url: URL(string: "https://openidconnect.googleapis.com/v1/userinfo")!)
            request.setValue("Bearer " + token.access_token, forHTTPHeaderField: "Authorization")
            struct Identity: Decodable { var sub: String }
            let identity = try JSONDecoder().decode(Identity.self, from: await NativeHTTP().send(request))
            var next = self.service.secrets; next.token = token; next.selfID = identity.sub
            if next.selfID != self.service.secrets.selfID {
                var p = Preferences(); p.accountID = next.selfID
                try LocalState.write(p); self.service.preferences = p; self.spaces = []
            }
            try Vault.write(next); self.service.secrets = next; self.sync()
            self.spaces = try await self.service.listSpaces()
            self.status = self.spaces.isEmpty ? "Nenašli sa žiadne chaty. Skontrolujte členstvo a povolenia Chat API." : "Google je prihlásený. Vyberte chaty na doručovanie."
        }
    }
    func cancelOperation() { operation?.cancel() }
    func reloadSpaces() {
        guard !running else { return }
        perform { self.spaces = try await self.service.listSpaces(); self.status = "Zoznam chatov bol obnovený." }
    }
    func select(_ space: Space, enabled: Bool) {
        guard !running && !busy else { return }
        do {
            var p = service.preferences; p.select(space, enabled: enabled)
            try LocalState.write(p); service.preferences = p; sync()
        } catch { self.error = describe(error) }
    }
    func signOut() {
        guard !running && !busy else { return }
        do {
            var next = service.secrets; next.token = nil; next.selfID = nil
            let p = Preferences()
            try LocalState.write(p); service.preferences = p
            try Vault.write(next); service.secrets = next
            spaces = []; sync(); status = "Google je odhlásený na tomto Macu."
        } catch { self.error = describe(error) }
    }
    func testDisplay() { perform { try await self.service.testDisplay(); self.status = "Skúšobná notifikácia bola doručená." } }
    func toggle() {
        guard !busy else { return }
        if !running { start(); return }
        busy = true; worker?.cancel()
        let previous = worker
        Task {
            await previous?.value; worker = nil; running = false
            do {
                var p = service.preferences; p.enabled = false
                try LocalState.write(p); service.preferences = p
                status = "Mostík je vypnutý"
            } catch { self.error = describe(error) }
            busy = false
        }
    }
    private func start() {
        guard configured && !busy && !running && storageReady else { return }
        do {
            var p = service.preferences; p.enabled = true
            try LocalState.write(p); service.preferences = p
        } catch { self.error = describe(error); return }
        running = true; error = nil; status = "Pripájam displej a Google Chat…"
        worker = Task {
            var failures = 0
            while !Task.isCancelled {
                do {
                    if try await service.device() {
                        for space in service.preferences.spaces {
                            delivered += try await service.poll(space)
                        }
                        status = "Pripojené · kontrola každých 15 sekúnd"
                    } else { status = "Notifikácie sú vypnuté na displeji. Čakám…" }
                    lastContact = Date(); error = nil; failures = 0
                } catch {
                    if Task.isCancelled { break }
                    failures = min(failures + 1, 5); self.error = describe(error)
                    status = "Čakám na opätovné spojenie…"
                }
                do { try await Task.sleep(nanoseconds: UInt64(min(300, 15 * (1 << failures))) * 1_000_000_000) }
                catch { break }
            }
        }
    }
    func setLaunchAtLogin(_ value: Bool) {
        do {
            if value { try SMAppService.mainApp.register() } else { try SMAppService.mainApp.unregister() }
            launchAtLogin = SMAppService.mainApp.status == .enabled
            if SMAppService.mainApp.status == .requiresApproval {
                error = "Povoľte Meteo Chat v Systémové nastavenia → Všeobecné → Prihlasovacie položky."
                SMAppService.openSystemSettingsLoginItems()
            }
        } catch { self.error = "Automatické spúšťanie sa nepodarilo nastaviť. Aplikáciu najprv presuňte do Applications." }
    }
    func openSettings() {
        if window == nil {
            let w = NSWindow(contentRect: NSRect(x: 0, y: 0, width: 620, height: 760),
                             styleMask: [.titled, .closable, .miniaturizable, .resizable], backing: .buffered, defer: false)
            w.title = "Meteo Chat"; w.isReleasedWhenClosed = false
            w.contentViewController = NSHostingController(rootView: SetupView(model: self))
            w.minSize = NSSize(width: 560, height: 560); w.center(); window = w
        }
        window?.makeKeyAndOrderFront(nil); NSApp.activate(ignoringOtherApps: true)
    }
}
