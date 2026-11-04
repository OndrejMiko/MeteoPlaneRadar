import SwiftUI
import AppKit
import ChatCore

@MainActor final class AppDelegate: NSObject, NSApplicationDelegate {
    func applicationDidFinishLaunching(_ notification: Notification) {
        NSApp.setActivationPolicy(.accessory); AppModel.shared.launch()
    }
}
@main struct MeteoChatApp: App {
    @NSApplicationDelegateAdaptor(AppDelegate.self) var delegate
    @StateObject private var model = AppModel.shared
    var body: some Scene {
        MenuBarExtra {
            Text("Meteo Chat").font(.headline)
            Text(model.status)
            if model.error != nil || model.migrationWarning != nil { Text("Vyžaduje pozornosť – otvorte nastavenia") }
            Divider()
            Button(model.running ? "Pozastaviť mostík" : "Zapnúť mostík") { model.toggle() }
                .disabled(model.busy || (!model.running && !model.configured))
            Button("Nastavenia…") { model.openSettings() }.keyboardShortcut(",")
            Button("Skúšobná notifikácia") { model.testDisplay() }.disabled(!model.hasPair || model.busy)
            if let date = model.lastContact { Text("Posledný kontakt: \(date.formatted(date: .omitted, time: .shortened))") }
            Text("Doručené počas spustenia: \(model.delivered)")
            Divider()
            Button("Ukončiť Meteo Chat") { NSApp.terminate(nil) }.keyboardShortcut("q")
        } label: {
            Image(systemName: model.error != nil || model.migrationWarning != nil ? "exclamationmark.bubble" : (model.running ? "paperplane.fill" : "paperplane"))
                .help("Meteo Chat · " + model.status)
        }
    }
}

struct SetupView: View {
    @ObservedObject var model: AppModel
    private var locked: Bool { model.running || model.busy || !model.storageReady }
    var body: some View {
        ScrollView {
            VStack(alignment: .leading, spacing: 22) {
                HStack(alignment: .top, spacing: 14) {
                    Image(systemName: "paperplane.circle.fill").font(.system(size: 44)).foregroundStyle(.cyan)
                    VStack(alignment: .leading, spacing: 4) {
                        Text("Google Chat → displej").font(.title2.bold())
                        Text("Mostík beží v hornej lište. Zatvorenie tohto okna ho neukončí.")
                            .foregroundStyle(.secondary)
                    }
                }
                HStack {
                    VStack(alignment: .leading, spacing: 5) {
                        Label(model.running ? "Mostík je zapnutý" : "Mostík je vypnutý", systemImage: model.running ? "checkmark.circle.fill" : "pause.circle")
                            .foregroundStyle(model.running ? .green : .secondary)
                        Text(model.status).font(.caption).foregroundStyle(.secondary)
                    }
                    Spacer()
                    Button(model.running ? "Pozastaviť" : "Zapnúť mostík") { model.toggle() }
                        .buttonStyle(.borderedProminent).disabled(model.busy || (!model.running && !model.configured))
                }.padding(16).background(.quaternary.opacity(0.4), in: RoundedRectangle(cornerRadius: 12))
                if let error = model.error {
                    Label(error, systemImage: "exclamationmark.triangle").foregroundStyle(.orange)
                        .textSelection(.enabled).fixedSize(horizontal: false, vertical: true)
                }
                if let warning = model.migrationWarning {
                    Label(warning, systemImage: "exclamationmark.triangle").foregroundStyle(.orange)
                        .textSelection(.enabled).fixedSize(horizontal: false, vertical: true)
                }
                if model.busy {
                    HStack { ProgressView().controlSize(.small); Text("Prebieha operácia…"); Spacer()
                        Button("Zrušiť") { model.cancelOperation() }
                    }
                }
                GroupBox {
                    VStack(alignment: .leading, spacing: 12) {
                        Text(model.deviceHost).font(.headline)
                        Text("Vo webovom UI displeja otvorte Notifikácie → Vygenerovať párovací kód. Sem zadajte adresu displeja a šesťmiestny kód. Súbor nepotrebujete.")
                            .font(.callout).foregroundStyle(.secondary)
                        LabeledContent("Adresa displeja") {
                            TextField("meteoplaneradar.local", text: $model.pairAddress)
                                .textFieldStyle(.roundedBorder).disabled(locked)
                        }
                        LabeledContent("Párovací kód") {
                            TextField("123 456", text: $model.pairCode)
                                .font(.system(.body, design: .monospaced)).textFieldStyle(.roundedBorder)
                                .frame(width: 140).disabled(locked).onSubmit { model.claimPair() }
                        }
                        HStack {
                            Button("Spárovať kódom") { model.claimPair() }.buttonStyle(.borderedProminent)
                                .disabled(locked || model.pairCode.isEmpty)
                            Button("Overiť displej") { model.testDisplay() }.disabled(!model.hasPair || model.busy)
                        }
                        DisclosureGroup("Pokročilé: import párovacieho súboru") {
                            Button("Importovať párovanie…") { model.importPair() }.disabled(locked)
                                .padding(.top, 8)
                        }.font(.caption)
                    }.frame(maxWidth: .infinity, alignment: .leading).padding(10)
                } label: { Label("1 · Displej", systemImage: "display") }
                GroupBox {
                    VStack(alignment: .leading, spacing: 12) {
                        Text(model.signedIn ? "Google je prihlásený" : "Prihláste sa cez prehliadač").font(.headline)
                        Text(model.hasClient
                             ? "Kliknite na prihlásenie, v prehliadači vyberte účet a povoľte čítanie Chatu. Heslo nezadávate do tejto aplikácie."
                             : "Google prihlásenie v tejto zostave ešte nie je nakonfigurované. Použite zostavu s pripraveným prihlásením alebo pokročilé nastavenie.")
                            .font(.callout).foregroundStyle(.secondary)
                        Button(model.signedIn ? "Prihlásiť znova" : "Prihlásiť cez Google") { model.signIn() }
                            .buttonStyle(.borderedProminent).disabled(locked || !model.hasClient)
                        if model.signedIn { Button("Odhlásiť Google na tomto Macu") { model.signOut() }.disabled(locked) }
                        DisclosureGroup("Pokročilé nastavenie Googlu") {
                            VStack(alignment: .leading, spacing: 10) {
                                Text(model.usesBundledClient ? "Používa sa predvolené prihlásenie Meteo Chat." : (model.hasClient ? "Používa sa vlastný OAuth klient." : "OAuth klient zatiaľ nie je pripravený."))
                                Button("Importovať vlastného OAuth klienta…") { model.importClient() }.disabled(locked)
                                if model.bundledClient != nil && !model.usesBundledClient {
                                    Button("Použiť predvolené prihlásenie Meteo Chat") { model.useDefaultClient() }.disabled(locked)
                                    Text("Prepnutie odhlási aktuálny Google účet a vymaže výber chatov. Párovanie displeja zostane zachované.")
                                }
                                Link("Návod Google OAuth pre autora aplikácie", destination: URL(string: "https://developers.google.com/workspace/chat/authenticate-authorize-chat-user")!)
                            }.font(.caption).padding(.top, 8)
                        }
                    }.frame(maxWidth: .infinity, alignment: .leading).padding(10)
                } label: { Label("2 · Google účet", systemImage: "person.crop.circle") }
                GroupBox {
                    VStack(alignment: .leading, spacing: 12) {
                        HStack {
                            Text("Vyberte chaty na doručovanie").font(.headline); Spacer()
                            Button("Obnoviť") { model.reloadSpaces() }.disabled(locked || !model.signedIn)
                        }
                        if model.spaces.isEmpty {
                            Text("Po prihlásení sa tu zobrazia chaty dostupné cez Google Chat API.")
                                .foregroundStyle(.secondary).font(.callout)
                        }
                        ForEach(model.spaces) { space in
                            Toggle(isOn: Binding(get: { model.selected.contains(space.id) },
                                                 set: { model.select(space, enabled: $0) })) {
                                VStack(alignment: .leading, spacing: 2) {
                                    Text(space.title)
                                    if space.displayName != nil { Text(space.name).font(.caption).foregroundStyle(.secondary) }
                                }
                            }.toggleStyle(.checkbox).disabled(locked)
                        }
                        Text("Prvý výber začína od aktuálneho času. Vlastné správy sa neposielajú. Pri zmene nastavení mostík najprv pozastavte.")
                            .font(.caption).foregroundStyle(.secondary)
                    }.frame(maxWidth: .infinity, alignment: .leading).padding(10)
                } label: { Label("3 · Chaty", systemImage: "bubble.left.and.bubble.right") }
                Toggle("Spúšťať po prihlásení do macOS", isOn: Binding(get: { model.launchAtLogin }, set: { model.setLaunchAtLogin($0) }))
                Text("Mac musí zostať zapnutý a mať prístup k internetu aj displeju v domácej sieti. Google môže vyžadovať Workspace účet a súhlas správcu. Kľúče a tokeny sú v macOS Kľúčenke; obsah správ sa neukladá do súborov ani logov.")
                    .font(.caption).foregroundStyle(.secondary)
                HStack { Text("Meteo Chat \(Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as? String ?? "dev")"); Spacer()
                    Text("macOS 13+") }.font(.caption2).foregroundStyle(.secondary)
            }.padding(26)
        }.frame(minWidth: 540, minHeight: 520)
    }
}
