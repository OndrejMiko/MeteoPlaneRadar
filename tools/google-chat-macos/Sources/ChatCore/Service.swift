import Foundation

public protocol HTTPClient {
    func send(_ request: URLRequest) async throws -> Data
}
private final class NoRedirect: NSObject, URLSessionTaskDelegate {
    func urlSession(_ session: URLSession, task: URLSessionTask,
                    willPerformHTTPRedirection response: HTTPURLResponse,
                    newRequest request: URLRequest, completionHandler: @escaping (URLRequest?) -> Void) {
        completionHandler(nil) // Never redirect Google or display bearer credentials.
    }
}
public final class NativeHTTP: HTTPClient {
    private let session: URLSession
    public init() {
        let c = URLSessionConfiguration.ephemeral
        c.timeoutIntervalForRequest = 20; c.timeoutIntervalForResource = 30
        c.httpCookieStorage = nil; c.urlCache = nil
        session = URLSession(configuration: c, delegate: NoRedirect(), delegateQueue: nil)
    }
    public func send(_ request: URLRequest) async throws -> Data {
        let (bytes, response) = try await session.bytes(for: request)
        guard let r = response as? HTTPURLResponse else { throw BridgeError.message("Neplatná odpoveď služby.") }
        guard (200..<300).contains(r.statusCode) else { throw BridgeError.http(r.statusCode) }
        var data = Data()
        for try await byte in bytes {
            if data.count >= 2 * 1024 * 1024 { throw BridgeError.message("Odpoveď služby je príliš veľká.") }
            data.append(byte)
        }
        return data
    }
}
public func formRequest(_ endpoint: String, fields: [String: String]) -> URLRequest {
    var r = URLRequest(url: URL(string: endpoint)!)
    r.httpMethod = "POST"
    r.setValue("application/x-www-form-urlencoded", forHTTPHeaderField: "Content-Type")
    // URLQueryItem leaves + unescaped; OAuth forms must encode literal plus signs.
    let allowed = CharacterSet.alphanumerics.union(CharacterSet(charactersIn: "-._~"))
    r.httpBody = fields.sorted { $0.key < $1.key }.map {
        $0.key.addingPercentEncoding(withAllowedCharacters: allowed)! + "=" +
        $0.value.addingPercentEncoding(withAllowedCharacters: allowed)!
    }.joined(separator: "&").data(using: .utf8)
    return r
}

@MainActor public final class ChatService {
    public var secrets: Secrets
    public var preferences: Preferences
    public var saveSecrets: (Secrets) throws -> Void
    public var savePreferences: (Preferences) throws -> Void
    private let http: HTTPClient
    public init(secrets: Secrets, preferences: Preferences, http: HTTPClient = NativeHTTP(),
                saveSecrets: @escaping (Secrets) throws -> Void,
                savePreferences: @escaping (Preferences) throws -> Void) {
        self.secrets = secrets; self.preferences = preferences; self.http = http
        self.saveSecrets = saveSecrets; self.savePreferences = savePreferences
    }
    public func google(_ url: URL) async throws -> Data {
        guard let client = secrets.client, var token = secrets.token else {
            throw BridgeError.message("Najprv sa prihláste do Googlu.")
        }
        if (token.expiresAt ?? .distantPast) < Date().addingTimeInterval(60) {
            guard let refresh = token.refresh_token else { throw BridgeError.message("Prihláste sa znova do Googlu.") }
            var fields = ["client_id": client.client_id, "refresh_token": refresh, "grant_type": "refresh_token"]
            if let secret = client.client_secret { fields["client_secret"] = secret }
            var renewed = try JSONDecoder().decode(Token.self, from: await http.send(formRequest("https://oauth2.googleapis.com/token", fields: fields)))
            if renewed.refresh_token == nil { renewed.refresh_token = refresh }
            renewed.updateExpiry(); token = renewed
            var updated = secrets; updated.token = token
            try saveSecrets(updated); secrets = updated
        }
        var req = URLRequest(url: url)
        req.setValue("Bearer " + token.access_token, forHTTPHeaderField: "Authorization")
        return try await http.send(req)
    }
    public func listSpaces() async throws -> [Space] {
        struct Page: Decodable { var spaces: [Space]?; var nextPageToken: String? }
        var spaces: [Space] = [], token: String? = nil, seen = Set<String>()
        repeat {
            var url = URLComponents(string: "https://chat.googleapis.com/v1/spaces")!
            url.queryItems = [URLQueryItem(name: "pageSize", value: "100")]
            if let t = token { url.queryItems?.append(URLQueryItem(name: "pageToken", value: t)) }
            let page = try JSONDecoder().decode(Page.self, from: await google(url.url!))
            spaces += page.spaces ?? []; token = page.nextPageToken
            if let t = token, !seen.insert(t).inserted { throw BridgeError.message("Neplatné stránkovanie chatov.") }
        } while token != nil
        return spaces
    }
    public func device(_ payload: Payload? = nil) async throws -> Bool {
        try Task.checkCancellation()
        guard let pair = secrets.pair else { throw BridgeError.message("Importujte párovanie displeja.") }
        try pair.validate()
        var r = URLRequest(url: URL(string: pair.device + (payload == nil ? "/ping" : ""))!)
        r.setValue("Bearer " + pair.key, forHTTPHeaderField: "Authorization")
        if let p = payload {
            r.httpMethod = "POST"; r.httpBody = try JSONEncoder().encode(p)
            r.setValue("application/json", forHTTPHeaderField: "Content-Type")
        }
        let data = try await http.send(r)
        if payload != nil { return true }
        struct Ping: Decodable { var enabled: Bool }
        return try JSONDecoder().decode(Ping.self, from: data).enabled
    }
    public func testDisplay() async throws {
        let payload = Payload(id: "mac-test-" + UUID().uuidString, sender: "Meteo Chat pre Mac",
                              room: "Test spojenia", text: "Mostík na Macu je pripojený k displeju.")
        _ = try await device(payload)
    }
    public func sendClaudeUsage(_ snapshot: ClaudeUsageSnapshot) async throws {
        try Task.checkCancellation()
        try snapshot.validate()
        guard let pair = secrets.pair else { throw BridgeError.message("Najprv spárujte displej.") }
        try pair.validate()
        var url = URLComponents(string: pair.device)!
        url.path = "/api/claude/usage"
        var request = URLRequest(url: url.url!)
        request.httpMethod = "POST"
        request.setValue("Bearer " + pair.key, forHTTPHeaderField: "Authorization")
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        request.httpBody = try snapshot.wireData()
        _ = try await http.send(request)
    }
    public func poll(_ space: Space) async throws -> Int {
        guard var checkpoint = preferences.checkpoints[space.id] else {
            throw BridgeError.message("Vyberte chat znova, aby sa nastavil začiatok odberu.")
        }
        // One whole Google page per tick. Keep the lower bound stable through all
        // pages, including equal-timestamp messages and mid-page delivery failures.
        var url = URLComponents(string: "https://chat.googleapis.com/v1/\(space.name)/messages")!
        url.queryItems = [URLQueryItem(name: "pageSize", value: "100"),
                          URLQueryItem(name: "orderBy", value: "createTime ASC"),
                          URLQueryItem(name: "filter", value: "createTime > \"\(checkpoint.cursor)\"")]
        if let token = checkpoint.pageToken { url.queryItems?.append(URLQueryItem(name: "pageToken", value: token)) }
        let page = try JSONDecoder().decode(MessagePage.self, from: await google(url.url!))
        var delivered = 0
        for message in page.messages ?? [] {
            try Task.checkCancellation()
            if checkpoint.accepted.contains(message.name) { continue }
            let own = secrets.selfID.map { message.sender?.name == "users/" + $0 } ?? false
            if !own && message.deleteTime == nil, let payload = try Payload.make(message, space: space) {
                _ = try await device(payload); delivered += 1
            }
            checkpoint.accepted.append(message.name); checkpoint.latest = message.createTime
            try persist(checkpoint, for: space.id)
        }
        if let next = page.nextPageToken, !next.isEmpty {
            if next == checkpoint.pageToken { throw BridgeError.message("Neplatné stránkovanie správ.") }
            checkpoint.pageToken = next
        } else {
            checkpoint.cursor = checkpoint.latest ?? checkpoint.cursor
            checkpoint.latest = nil; checkpoint.pageToken = nil
        }
        checkpoint.accepted = []; try persist(checkpoint, for: space.id)
        return delivered
    }
    private func persist(_ checkpoint: Checkpoint, for id: String) throws {
        var updated = preferences; updated.checkpoints[id] = checkpoint
        try savePreferences(updated); preferences = updated
    }
}
