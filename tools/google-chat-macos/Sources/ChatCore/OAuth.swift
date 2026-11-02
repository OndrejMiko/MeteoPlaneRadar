import Foundation
import CryptoKit
import Security
import Network
import AppKit

public enum OAuthTools {
    public static func random() throws -> String {
        var bytes = [UInt8](repeating: 0, count: 48)
        guard SecRandomCopyBytes(kSecRandomDefault, bytes.count, &bytes) == errSecSuccess else {
            throw BridgeError.message("Nepodarilo sa vytvoriť bezpečné prihlásenie.")
        }
        return base64(Data(bytes))
    }
    public static func base64(_ data: Data) -> String {
        data.base64EncodedString().replacingOccurrences(of: "+", with: "-")
            .replacingOccurrences(of: "/", with: "_").replacingOccurrences(of: "=", with: "")
    }
    public static func challenge(_ verifier: String) -> String { base64(Data(SHA256.hash(data: Data(verifier.utf8)))) }
    public static func callback(_ target: String, state: String) throws -> String {
        guard let u = URLComponents(string: target), u.path == "/callback", u.host == nil else {
            throw BridgeError.message("Neplatný návrat z prihlásenia.")
        }
        let fields = u.queryItems ?? []
        let states = fields.filter { $0.name == "state" }
        guard states.count == 1, let received = states.first?.value,
              constantEqual(received, state) else { throw BridgeError.message("Neplatný stav prihlásenia.") }
        if fields.contains(where: { $0.name == "error" }) { throw BridgeError.message("Prihlásenie do Googlu bolo zrušené.") }
        let codes = fields.filter { $0.name == "code" }
        guard codes.count == 1, let code = codes.first?.value, !code.isEmpty else {
            throw BridgeError.message("Google nevrátil prihlasovací kód.")
        }
        return code
    }
    private static func constantEqual(_ a: String, _ b: String) -> Bool {
        let x = Array(SHA256.hash(data: Data(a.utf8))), y = Array(SHA256.hash(data: Data(b.utf8)))
        return zip(x, y).reduce(UInt8(0)) { $0 | ($1.0 ^ $1.1) } == 0
    }
}

@MainActor public final class GoogleOAuth {
    private var listener: NWListener?
    private var continuation: CheckedContinuation<String, Error>?
    private var timeout: Task<Void, Never>?
    private var state = "", verifier = "", redirect = ""
    private let http: HTTPClient
    private let openBrowser: (URL) -> Bool
    public init(http: HTTPClient = NativeHTTP(), openBrowser: @escaping (URL) -> Bool = { NSWorkspace.shared.open($0) }) {
        self.http = http; self.openBrowser = openBrowser
    }
    public func signIn(client: OAuthClient) async throws -> Token {
        state = try OAuthTools.random(); verifier = try OAuthTools.random()
        let code = try await withTaskCancellationHandler(operation: {
            try await withCheckedThrowingContinuation { (c: CheckedContinuation<String, Error>) in
                continuation = c
                do {
                    let p = NWParameters.tcp
                    p.requiredLocalEndpoint = .hostPort(host: "127.0.0.1", port: .any)
                    let server = try NWListener(using: p); listener = server
                    server.newConnectionHandler = { [weak self] connection in
                        Task { @MainActor in self?.receive(connection, data: Data()) }
                    }
                    server.stateUpdateHandler = { [weak self] status in
                        Task { @MainActor in
                            guard let self, self.continuation != nil else { return }
                            switch status {
                            case .ready:
                                guard let port = self.listener?.port else { return }
                                self.redirect = "http://127.0.0.1:\(port.rawValue)/callback"
                                var url = URLComponents(string: "https://accounts.google.com/o/oauth2/v2/auth")!
                                url.queryItems = ["client_id": client.client_id, "redirect_uri": self.redirect,
                                    "response_type": "code", "scope": "openid https://www.googleapis.com/auth/chat.spaces.readonly https://www.googleapis.com/auth/chat.messages.readonly",
                                    "state": self.state, "code_challenge": OAuthTools.challenge(self.verifier),
                                    "code_challenge_method": "S256", "access_type": "offline", "prompt": "consent"]
                                    .map { URLQueryItem(name: $0.key, value: $0.value) }
                                if !self.openBrowser(url.url!) { self.finish(.failure(BridgeError.message("Nepodarilo sa otvoriť prehliadač."))) }
                            case .failed: self.finish(.failure(BridgeError.message("Nepodarilo sa spustiť lokálny návrat z prihlásenia.")))
                            default: break
                            }
                        }
                    }
                    server.start(queue: .main)
                    timeout = Task { [weak self] in
                        do { try await Task.sleep(nanoseconds: 300_000_000_000) } catch { return }
                        self?.finish(.failure(BridgeError.message("Prihlásenie vypršalo. Skúste to znova.")))
                    }
                } catch { finish(.failure(BridgeError.message("Lokálny prihlasovací port nie je dostupný."))) }
            }
        }, onCancel: { Task { @MainActor in self.finish(.failure(CancellationError())) } })
        try Task.checkCancellation()
        var fields = ["client_id": client.client_id, "code": code, "code_verifier": verifier,
                      "redirect_uri": redirect, "grant_type": "authorization_code"]
        if let secret = client.client_secret { fields["client_secret"] = secret }
        var token = try JSONDecoder().decode(Token.self, from: await http.send(formRequest("https://oauth2.googleapis.com/token", fields: fields)))
        guard token.refresh_token != nil else {
            throw BridgeError.message("Google nevydal trvalé prihlásenie. Odoberte aplikácii prístup v Google účte a skúste to znova.")
        }
        token.updateExpiry(); return token
    }
    private func receive(_ connection: NWConnection, data: Data) {
        if data.isEmpty {
            connection.start(queue: .main)
            // Do not let a stalled local connection live beyond this login attempt.
            DispatchQueue.main.asyncAfter(deadline: .now() + 10) { connection.cancel() }
        }
        connection.receive(minimumIncompleteLength: 1, maximumLength: 8192) { [weak self] chunk, _, complete, error in
            Task { @MainActor in
                guard let self else { connection.cancel(); return }
                var buffer = data; buffer.append(chunk ?? Data())
                guard buffer.count <= 8192, error == nil else { connection.cancel(); return }
                guard let request = String(data: buffer, encoding: .utf8), request.contains("\r\n\r\n") else {
                    if complete { connection.cancel() } else { self.receive(connection, data: buffer) }
                    return
                }
                let parts = (request.components(separatedBy: "\r\n").first ?? "").split(separator: " ")
                guard parts.count == 3, parts[0] == "GET" else { connection.cancel(); return }
                let target = String(parts[1])
                do {
                    let code = try OAuthTools.callback(target, state: self.state)
                    self.reply(connection, ok: true)
                    self.finish(.success(code))
                } catch {
                    self.reply(connection, ok: false)
                    // A valid cancellation returns to the app, unrelated requests
                    // and mismatching states cannot terminate the login attempt.
                    if let u = URLComponents(string: target), u.host == nil, u.path == "/callback",
                       u.queryItems?.filter({ $0.name == "state" }).count == 1,
                       u.queryItems?.contains(URLQueryItem(name: "state", value: self.state)) == true,
                       u.queryItems?.contains(where: { $0.name == "error" }) == true {
                        self.finish(.failure(BridgeError.message("Prihlásenie bolo zrušené v prehliadači.")))
                    }
                }
            }
        }
    }
    private func reply(_ connection: NWConnection, ok: Bool) {
        let body = ok ? "Prihlásenie bolo prijaté. Vráťte sa do aplikácie Meteo Chat. Túto kartu môžete zavrieť." : "Neplatný návrat. Vráťte sa do aplikácie Meteo Chat a prihlásenie zopakujte."
        let response = "HTTP/1.1 \(ok ? "200 OK" : "400 Bad Request")\r\nContent-Type: text/plain; charset=utf-8\r\nCache-Control: no-store\r\nContent-Length: \(body.utf8.count)\r\nConnection: close\r\n\r\n\(body)"
        connection.send(content: Data(response.utf8), completion: .contentProcessed { _ in connection.cancel() })
    }
    private func finish(_ result: Result<String, Error>) {
        let c = continuation; continuation = nil
        timeout?.cancel(); timeout = nil; listener?.cancel(); listener = nil
        c?.resume(with: result)
    }
}
