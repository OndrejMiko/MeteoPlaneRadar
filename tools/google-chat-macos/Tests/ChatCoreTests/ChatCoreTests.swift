// Standalone checks work with Command Line Tools; XCTest requires full Xcode.
func expect(_ condition: @autoclosure () -> Bool, file: StaticString = #file, line: UInt = #line) {
    if !condition() { fatalError("Check failed", file: file, line: line) }
}
func expectFalse(_ value: @autoclosure () -> Bool) { expect(!value()) }
func expectEqual<T: Equatable>(_ a: @autoclosure () throws -> T, _ b: T, file: StaticString = #file, line: UInt = #line) rethrows { let value = try a(); expect(value == b, file: file, line: line) }
func expectNotEqual<T: Equatable>(_ a: T, _ b: T) { expect(a != b) }
func expectNil<T>(_ a: T?) { expect(a == nil) }
func expectAtLeast<T: Comparable>(_ a: T, _ b: T) { expect(a >= b) }
func expectAtMost<T: Comparable>(_ a: T, _ b: T) { expect(a <= b) }
func expectThrows<T>(_ value: @autoclosure () throws -> T) { do { _ = try value() } catch { return }; fatalError("Expected an error") }
func fail(_ message: String) { fatalError(message) }
func unwrap<T>(_ a: T?) throws -> T { guard let a else { throw BridgeError.message("Missing test value") }; return a }
import Foundation
import Network
import ChatCore

final class FakeHTTP: HTTPClient {
    var requests: [URLRequest] = []
    var reply: (URLRequest) throws -> Data = { _ in Data("{}".utf8) }
    func send(_ request: URLRequest) async throws -> Data { requests.append(request); return try reply(request) }
}
@MainActor final class ChatCoreChecks {
    func message(_ id: String, sender: String = "other") -> [String: Any] {
        ["name": "spaces/one/messages/" + id, "createTime": "2026-11-02T12:00:00Z", "sender": ["name": "users/" + sender], "text": "Ahoj"]
    }
    func fixture(_ http: FakeHTTP, preferences: Preferences? = nil,
                 save: @escaping (Preferences) throws -> Void = { _ in }) throws -> ChatService {
        var secrets = Secrets()
        secrets.pair = Pairing(device: "http://127.0.0.1:12345/api/notifications", key: String(repeating: "a", count: 64))
        secrets.client = try OAuthClient.decodeFile(Data(#"{"installed":{"client_id":"test","client_secret":"secret"}}"#.utf8))
        secrets.token = try JSONDecoder().decode(Token.self, from: Data(#"{"access_token":"access","refresh_token":"refresh"}"#.utf8))
        secrets.token?.expiresAt = .distantFuture; secrets.selfID = "own"
        var p = preferences ?? Preferences()
        if preferences == nil { p.select(Space(name: "spaces/one"), enabled: true); p.checkpoints["spaces/one"] = Checkpoint(cursor: "2026-11-02T00:00:00Z") }
        return ChatService(secrets: secrets, preferences: p, http: http, saveSecrets: { _ in }, savePreferences: save)
    }
    func testPairAndDesktopClientValidation() throws {
        try Pairing(device: "http://meteoplaneradar.local/api/notifications", key: String(repeating: "b", count: 64)).validate()
        for url in ["file:///api/notifications", "http://user:pass@host/api/notifications", "https://host/wrong", "https://host/api/notifications?q=x", "https://host/api/notifications#key"] {
            expectThrows(try Pairing(device: url, key: String(repeating: "a", count: 64)).validate())
        }
        expectThrows(try Pairing(device: "http://host/api/notifications", key: "short").validate())
        expectThrows(try OAuthClient.decodeFile(Data(#"{"web":{"client_id":"web"}}"#.utf8)))
        expectThrows(try OAuthClient.decodeFile(Data(#"{"installed":{"client_id":""}}"#.utf8)))
    }
    func testPKCEStateCallbackAndForms() throws {
        expectEqual(OAuthTools.challenge("dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk"), "E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM")
        try expectEqual(try OAuthTools.callback("/callback?code=a%2Bb&state=valid", state: "valid"), "a+b")
        for target in ["/callback?code=secret&state=wrong", "/other?code=x&state=valid", "/callback?state=valid", "/callback?state=valid&state=valid&code=x", "/callback?code=x&code=y&state=valid", "/callback?state=valid&error=access_denied", "http://external/callback?state=valid&code=x"] {
            expectThrows(try OAuthTools.callback(target, state: "valid"))
        }
        let a = try OAuthTools.random(), b = try OAuthTools.random()
        expectNotEqual(a, b); expectAtLeast(a.count, 43)
        let form = formRequest("https://example.test", fields: ["code": "a+b&c=é"])
        expectEqual(String(data: form.httpBody!, encoding: .utf8), "code=a%2Bb%26c%3D%C3%A9")
    }
    func testPayloadLimitsAttachmentsAndStableIDs() throws {
        let obj: [String: Any] = ["name": "spaces/a/messages/b", "createTime": "2026-11-02T00:00:00Z", "sender": ["displayName": String(repeating: "Ž", count: 200)], "text": String(repeating: "\"\\Ž🙂\n", count: 500)]
        let msg = try JSONDecoder().decode(ChatMessage.self, from: JSONSerialization.data(withJSONObject: obj))
        let p = try unwrap(Payload.make(msg, space: Space(name: "spaces/a", displayName: String(repeating: "é", count: 300))))
        expectAtMost(p.text.utf8.count, 1024); expectAtMost(p.sender.utf8.count, 127)
        expectAtMost(p.room.utf8.count, 127); expectAtMost(try JSONEncoder().encode(p).count, 2048)
        expectEqual(p.id, try Payload.make(msg, space: Space(name: "spaces/a"))?.id)
        expect(p.id.hasPrefix("chat-")); expectEqual(p.id.count, 69)
        let attachment = try JSONDecoder().decode(ChatMessage.self, from: Data(#"{"name":"spaces/a/messages/file","createTime":"2026-11-02T00:00:00Z","attachment":[{"contentName":"foto.png"}]}"#.utf8))
        try expectEqual(try Payload.make(attachment, space: Space(name: "spaces/a"))?.text, "[Attachment] foto.png")
    }
    func testRetryRestartEqualTimesPaginationAndOwnMessages() async throws {
        let http = FakeHTTP(); var saved = Preferences(); let service = try fixture(http, save: { saved = $0 })
        let page: [String: Any] = ["messages": [message("1"), message("2")], "nextPageToken": "page2"]
        var attempts = 0
        http.reply = { request in
            if request.url!.host == "chat.googleapis.com" { return try JSONSerialization.data(withJSONObject: page) }
            attempts += 1
            if attempts == 2 { throw BridgeError.http(503) }
            return Data("{}".utf8)
        }
        do { _ = try await service.poll(Space(name: "spaces/one")); fail("must retry") } catch {}
        expectEqual(saved.checkpoints["spaces/one"]?.cursor, "2026-11-02T00:00:00Z")
        expectEqual(saved.checkpoints["spaces/one"]?.accepted, ["spaces/one/messages/1"])
        let restarted = try fixture(http, preferences: saved, save: { saved = $0 })
        let count = try await restarted.poll(Space(name: "spaces/one")); expectEqual(count, 1)
        expectEqual(saved.checkpoints["spaces/one"]?.pageToken, "page2")
        expectEqual(saved.checkpoints["spaces/one"]?.accepted, [])
        http.reply = { request in
            if request.url!.host == "chat.googleapis.com" {
                let params = URLComponents(url: request.url!, resolvingAgainstBaseURL: false)!.queryItems!
                expectEqual(params.first { $0.name == "pageToken" }?.value, "page2")
                expectEqual(params.first { $0.name == "filter" }?.value, "createTime > \"2026-11-02T00:00:00Z\"")
                return try JSONSerialization.data(withJSONObject: ["messages": [self.message("3"), self.message("4", sender: "own")]])
            }
            return Data("{}".utf8)
        }
        let lastCount = try await restarted.poll(Space(name: "spaces/one")); expectEqual(lastCount, 1)
        expectEqual(saved.checkpoints["spaces/one"]?.cursor, "2026-11-02T12:00:00Z")
        expectNil(saved.checkpoints["spaces/one"]?.pageToken)
        let posts = http.requests.filter { $0.httpMethod == "POST" }
        expectEqual(posts.count, 4) // first attempt, failed second, retried second, third
        let firstPayload = try JSONDecoder().decode([String: String].self, from: posts[1].httpBody!)
        let retriedPayload = try JSONDecoder().decode([String: String].self, from: posts[2].httpBody!)
        expectEqual(firstPayload, retriedPayload) // JSON property order is not significant.
        expectEqual(posts[0].value(forHTTPHeaderField: "Authorization"), "Bearer " + String(repeating: "a", count: 64))
    }
    func testRefreshKeepsOfflineTokenAndPersistsBeforeGoogle() async throws {
        let http = FakeHTTP(), service = try fixture(FakeHTTP())
        var secrets = service.secrets; secrets.token?.expiresAt = .distantPast
        var stored: Secrets?
        let active = ChatService(secrets: secrets, preferences: service.preferences, http: http,
                                 saveSecrets: { stored = $0 }, savePreferences: { _ in })
        http.reply = { request in
            if request.url!.host == "oauth2.googleapis.com" { return Data(#"{"access_token":"renewed","expires_in":3600}"#.utf8) }
            expectEqual(stored?.token?.refresh_token, "refresh")
            expectEqual(request.value(forHTTPHeaderField: "Authorization"), "Bearer renewed")
            return Data(#"{"spaces":[]}"#.utf8)
        }
        let spaces = try await active.listSpaces(); expect(spaces.isEmpty)
        expectEqual(http.requests.count, 2); expectEqual(stored?.token?.access_token, "renewed")
    }
    func testLiveLoopbackOAuthAndCancellation() async throws {
        let http = FakeHTTP()
        http.reply = { req in
            expectEqual(req.url?.host, "oauth2.googleapis.com")
            expect(String(data: req.httpBody!, encoding: .utf8)!.contains("code_verifier="))
            return Data(#"{"access_token":"new","refresh_token":"offline","expires_in":3600}"#.utf8)
        }
        var callback: Task<Void, Error>?
        let flow = GoogleOAuth(http: http, openBrowser: { url in
            let items = URLComponents(url: url, resolvingAgainstBaseURL: false)!.queryItems!
            let redirect = items.first { $0.name == "redirect_uri" }!.value!
            let state = items.first { $0.name == "state" }!.value!
            expectEqual(URL(string: redirect)?.host, "127.0.0.1")
            callback = Task {
                // A wrong state must return 400 without aborting the real login.
                let (_, wrong) = try await URLSession.shared.data(from: URL(string: redirect + "?state=wrong&code=bad")!)
                expectEqual((wrong as? HTTPURLResponse)?.statusCode, 400)
                var good = URLComponents(string: redirect)!
                good.queryItems = [URLQueryItem(name: "code", value: "valid-code"), URLQueryItem(name: "state", value: state)]
                let (_, response) = try await URLSession.shared.data(from: good.url!)
                expectEqual((response as? HTTPURLResponse)?.statusCode, 200)
            }
            return true
        })
        let client = try OAuthClient.decodeFile(Data(#"{"installed":{"client_id":"test"}}"#.utf8))
        let token = try await flow.signIn(client: client)
        try await callback?.value
        expectEqual(token.refresh_token, "offline"); expectEqual(http.requests.count, 1)
        let blocked = GoogleOAuth(http: http, openBrowser: { _ in true })
        let task = Task { try await blocked.signIn(client: client) }
        try await Task.sleep(nanoseconds: 100_000_000); task.cancel()
        do { _ = try await task.value; fail("cancelled OAuth must stop") } catch is CancellationError {}
        expectEqual(http.requests.count, 1)
    }
    func testNativeRedirectAndCancelledDelivery() async throws {
        let server = try LocalHTTPServer()
        let url = try await server.start()
        var request = URLRequest(url: url); request.setValue("Bearer fake", forHTTPHeaderField: "Authorization")
        do { _ = try await NativeHTTP().send(request); fail("Redirect must be rejected") }
        catch BridgeError.http(let code) { expectEqual(code, 302) }
        expectEqual(server.requests, 1); server.listener.cancel()
        let http = FakeHTTP(), service = try fixture(http)
        let task = Task { try await service.device() }; task.cancel()
        do { _ = try await task.value; fail("Cancelled delivery must stop") } catch is CancellationError {}
        expect(http.requests.isEmpty)
    }
    func testCodePairingRequestAndErrors() async throws {
        let http = FakeHTTP()
        http.reply = { request in
            expectEqual(request.url?.absoluteString, "http://meteoplaneradar.local/api/notifications/pair/claim")
            expectEqual(request.httpMethod, "POST")
            let fields = try JSONDecoder().decode([String: String].self, from: request.httpBody!)
            expectEqual(fields, ["code": "001234"])
            expectNil(request.value(forHTTPHeaderField: "Authorization"))
            return try JSONEncoder().encode(["key": String(repeating: "b", count: 64)])
        }
        let pair = try await Pairing.claim(address: "meteoplaneradar.local", code: "001 234", http: http)
        expectEqual(pair.device, "http://meteoplaneradar.local/api/notifications")
        expectEqual(pair.key, String(repeating: "b", count: 64))
        for address in ["https://user:pass@host", "file:///test", "http://host/private", "http://host/?secret=key"] {
            do { _ = try await Pairing.claim(address: address, code: "001234", http: http); fail("Invalid address") } catch {}
        }
        do { _ = try await Pairing.claim(address: "host", code: "12345", http: http); fail("Invalid code") } catch {}
        expectEqual(http.requests.count, 1)
        for status in [403, 404, 410, 429] {
            http.reply = { _ in throw BridgeError.http(status) }
            do { _ = try await Pairing.claim(address: "host", code: "123456", http: http); fail("Must reject failed pairing") }
            catch BridgeError.message(let message) { expect(!message.isEmpty) }
        }
        http.reply = { _ in Data(#"{"key":"short"}"#.utf8) }
        do { _ = try await Pairing.claim(address: "host", code: "123456", http: http); fail("Invalid returned key") } catch {}
    }
    func testBundledOAuthDefaultsAndExport() throws {
        let input = Data(#"{"installed":{"client_id":"123456789012-desktop.apps.googleusercontent.com","client_secret":"test-client-secret","project_id":"example","access_token":"SHOULD_NOT_SHIP","auth_uri":"https://untrusted.invalid"},"tokens":"SHOULD_NOT_SHIP"}"#.utf8)
        let client = try OAuthConfiguration.decode(input)
        let exported = try OAuthConfiguration.export(client)
        let root = try JSONSerialization.jsonObject(with: exported) as! [String: Any]
        expectEqual(Set(root.keys), Set(["installed"]))
        expectEqual(Set((root["installed"] as! [String: Any]).keys), Set(["client_id", "client_secret"]))
        expectFalse(String(data: exported, encoding: .utf8)!.contains("SHOULD_NOT_SHIP"))
        for raw in [#"{"web":{"client_id":"web"}}"#, #"{"installed":{"client_id":"REPLACE_WITH_DESKTOP_CLIENT_ID","client_secret":"REPLACE_WITH_DESKTOP_CLIENT_SECRET"}}"#, #"{"installed":{"client_id":"123-test.apps.googleusercontent.com"}}"#] {
            expectThrows(try OAuthConfiguration.decode(Data(raw.utf8)))
        }
        let fresh = OAuthConfiguration.applying(client, to: Secrets())
        expectEqual(fresh.client?.client_id, client.client_id); expectNil(fresh.token)
        var old = Secrets(); old.client = client; old.selfID = "own"
        old.pair = Pairing(device: "http://host/api/notifications", key: String(repeating: "a", count: 64))
        old.token = try JSONDecoder().decode(Token.self, from: Data(#"{"access_token":"old","refresh_token":"refresh"}"#.utf8))
        let same = OAuthConfiguration.applying(client, to: old)
        expectEqual(same.token?.refresh_token, "refresh"); expectEqual(same.pair, old.pair)
        var rotated = client; rotated.client_secret = "rotated-secret"
        let updated = OAuthConfiguration.applying(rotated, to: old)
        expectEqual(updated.client?.client_secret, "rotated-secret")
        expectEqual(updated.token?.refresh_token, "refresh"); expectEqual(updated.selfID, "own")
        let different = try OAuthConfiguration.decode(Data(#"{"installed":{"client_id":"456-other.apps.googleusercontent.com","client_secret":"other-secret"}}"#.utf8))
        let preserved = OAuthConfiguration.applying(different, to: old)
        expectEqual(preserved.client, old.client); expectEqual(preserved.selfID, old.selfID)
        expectEqual(preserved.token?.access_token, "old")
        let withoutBundle = OAuthConfiguration.applying(nil, to: old)
        expectEqual(withoutBundle.client, old.client)
        var orphan = old; orphan.client = nil
        let recovered = OAuthConfiguration.applying(client, to: orphan)
        expectNil(recovered.token); expectNil(recovered.selfID); expectEqual(recovered.pair, old.pair)
    }
    func testRetiredIntegrationCleanup() throws {
        let folder = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
        let fm = FileManager.default
        defer { try? fm.removeItem(at: folder) }
        let appFolder = folder.appendingPathComponent("MeteoChat")
        let settings = folder.appendingPathComponent("custom-code-config/settings.json")
        let fallback = folder.appendingPathComponent("unused/settings.json")
        try fm.createDirectory(at: appFolder, withIntermediateDirectories: true)
        try fm.createDirectory(at: settings.deletingLastPathComponent(), withIntermediateDirectories: true)
        func write(_ value: [String: Any], _ file: URL) throws {
            try JSONSerialization.data(withJSONObject: value, options: [.sortedKeys]).write(to: file)
        }
        let record = appFolder.appendingPathComponent("claude-install.json")
        let helper = appFolder.appendingPathComponent("ClaudeUsageRelay")
        let snapshot = appFolder.appendingPathComponent("claude-usage.json")
        let state = appFolder.appendingPathComponent("state.json")
        let googleState = Data(#"{"enabled":true,"accountID":"own","spaces":[{"name":"spaces/a"}],"checkpoints":{"spaces/a":{"cursor":"2026-11-01T00:00:00Z","accepted":[]}},"claudeEnabled":true}"#.utf8)
        try googleState.write(to: state)
        let original: [String: Any] = ["type": "command", "command": "cat", "padding": 2]
        let installed: [String: Any] = ["type": "command", "command": "'/synthetic/ClaudeUsageRelay' --snapshot '/synthetic/usage.json' --forward 'cat'", "padding": 2]
        func prepare(_ previous: Any = original) throws {
            try write(["settings": settings.path, "original": previous, "installed": installed], record)
            try Data("synthetic helper".utf8).write(to: helper)
            try Data("{}".utf8).write(to: snapshot)
        }
        try prepare()
        try write(["statusLine": installed, "theme": "light", "permissions": ["allow": ["Read"]]], settings)
        try LegacyBridgeCleanup.remove(folder: appFolder, fallbackSettings: fallback)
        var restored = try JSONSerialization.jsonObject(with: Data(contentsOf: settings)) as! [String: Any]
        expect(NSDictionary(dictionary: restored["statusLine"] as! [String: Any]).isEqual(to: original))
        expectEqual(restored["theme"] as? String, "light")
        expectNotEqual(restored["permissions"] as? [String: [String]], nil)
        for file in [record, helper, snapshot] { expectFalse(fm.fileExists(atPath: file.path)) }
        try expectEqual(try Data(contentsOf: state), googleState)
        try LegacyBridgeCleanup.remove(folder: appFolder, fallbackSettings: fallback) // idempotent
        // Fresh installations originally had no status line.
        try prepare(NSNull())
        try write(["statusLine": installed, "theme": "dark"], settings)
        try LegacyBridgeCleanup.remove(folder: appFolder, fallbackSettings: fallback)
        restored = try JSONSerialization.jsonObject(with: Data(contentsOf: settings)) as! [String: Any]
        expectNil(restored["statusLine"])
        // A later unrelated user command remains exactly as saved.
        try prepare()
        try write(["statusLine": ["type": "command", "command": "my-own-command"]], settings)
        let changed = try Data(contentsOf: settings)
        try LegacyBridgeCleanup.remove(folder: appFolder, fallbackSettings: fallback)
        try expectEqual(try Data(contentsOf: settings), changed)
        // A user-modified wrapper still needs the helper; preserve all recovery files.
        try prepare()
        try write(["statusLine": ["type": "command", "command": "/synthetic/ClaudeUsageRelay --custom"]], settings)
        let wrapper = try Data(contentsOf: settings)
        expectThrows(try LegacyBridgeCleanup.remove(folder: appFolder, fallbackSettings: fallback))
        try expectEqual(try Data(contentsOf: settings), wrapper)
        for file in [record, helper, snapshot] { expect(fm.fileExists(atPath: file.path)) }
        try Data("not json".utf8).write(to: record)
        expectThrows(try LegacyBridgeCleanup.remove(folder: appFolder, fallbackSettings: fallback))
        expect(fm.fileExists(atPath: helper.path))
        // Old preference fields are ignored; all Google selections/cursors survive re-encoding.
        let prefs = try JSONDecoder().decode(Preferences.self, from: googleState)
        let clean = try JSONEncoder().encode(prefs)
        let object = try JSONSerialization.jsonObject(with: clean) as! [String: Any]
        expectNil(object["claudeEnabled"]); expect(prefs.enabled)
        expectEqual(prefs.spaces.map(\.id), ["spaces/a"])
        expectEqual(prefs.checkpoints["spaces/a"]?.cursor, "2026-11-01T00:00:00Z")
    }
    func testSelectionAndDisabledDisplay() async throws {
        var p = Preferences(); let space = Space(name: "spaces/a")
        p.select(space, enabled: true); let checkpoint = p.checkpoints[space.id]
        p.select(space, enabled: true); expectEqual(p.spaces.count, 1); expectEqual(p.checkpoints[space.id], checkpoint)
        p.select(space, enabled: false); expect(p.spaces.isEmpty); expectNil(p.checkpoints[space.id])
        let http = FakeHTTP(); http.reply = { _ in Data(#"{"enabled":false}"#.utf8) }
        let service = try fixture(http); let enabled = try await service.device(); expectFalse(enabled)
        expectEqual(http.requests.count, 1); expect(http.requests[0].url!.path.hasSuffix("/ping"))
    }
}

@main struct CheckRunner {
    @MainActor static func main() async throws {
        let tests = ChatCoreChecks()
        try tests.testRetiredIntegrationCleanup()
        try tests.testPairAndDesktopClientValidation()
        try tests.testPKCEStateCallbackAndForms()
        try tests.testPayloadLimitsAttachmentsAndStableIDs()
        try await tests.testRetryRestartEqualTimesPaginationAndOwnMessages()
        try await tests.testRefreshKeepsOfflineTokenAndPersistsBeforeGoogle()
        try await tests.testSelectionAndDisabledDisplay()
        try tests.testBundledOAuthDefaultsAndExport()
        try await tests.testCodePairingRequestAndErrors()
        try await tests.testLiveLoopbackOAuthAndCancellation()
        try await tests.testNativeRedirectAndCancelledDelivery()
        print("11 native bridge checks passed (including retired integration cleanup): pairing, OAuth/PKCE, bounded payloads, retry/restart/pagination, token refresh and selection")
    }
}

@MainActor final class LocalHTTPServer {
    let listener: NWListener
    var requests = 0
    init() throws {
        let p = NWParameters.tcp; p.requiredLocalEndpoint = .hostPort(host: "127.0.0.1", port: .any)
        listener = try NWListener(using: p)
    }
    func start() async throws -> URL {
        try await withCheckedThrowingContinuation { (continuation: CheckedContinuation<URL, Error>) in
            listener.stateUpdateHandler = { state in
                Task { @MainActor in
                    switch state {
                    case .ready: continuation.resume(returning: URL(string: "http://127.0.0.1:\(self.listener.port!.rawValue)/start")!)
                    case .failed(let error): continuation.resume(throwing: error)
                    default: break
                    }
                }
            }
            listener.newConnectionHandler = { connection in
                connection.start(queue: .main)
                connection.receive(minimumIncompleteLength: 1, maximumLength: 8192) { _, _, _, _ in
                    Task { @MainActor in
                        self.requests += 1
                        let response = "HTTP/1.1 302 Found\r\nLocation: /destination\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"
                        connection.send(content: Data(response.utf8), completion: .contentProcessed { _ in connection.cancel() })
                    }
                }
            }
            listener.start(queue: .main)
        }
    }
}
