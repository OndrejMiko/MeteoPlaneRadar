import Foundation
import CryptoKit

public enum BridgeError: Error, LocalizedError {
    case message(String), http(Int)
    public var errorDescription: String? {
        switch self {
        case .message(let text): return text
        case .http(let code):
            switch code {
            case 401: return "Prihlásenie alebo párovanie vypršalo (HTTP 401). Prihláste sa znova alebo importujte nové párovanie."
            case 403: return "Prístup je zamietnutý (HTTP 403). Skontrolujte Chat API a povolenia účtu alebo správcu."
            case 409, 503: return "Displej má plný rad alebo je zaneprázdnený. Doručenie sa zopakuje."
            default: return "Služba vrátila HTTP \(code). Pokus sa zopakuje."
            }
        }
    }
}
public struct Pairing: Codable, Equatable {
    public var device: String
    public var key: String
    public init(device: String, key: String) { self.device = device; self.key = key }
    public func validate() throws {
        guard let u = URLComponents(string: device), ["http", "https"].contains(u.scheme ?? ""),
              let host = u.host, !host.isEmpty, u.user == nil, u.password == nil,
              u.path == "/api/notifications", u.query == nil, u.fragment == nil,
              key.range(of: "^[a-fA-F0-9]{64}$", options: .regularExpression) != nil else {
            throw BridgeError.message("Neplatný párovací súbor. Stiahnite ho z karty Notifikácie vo webovom UI displeja.")
        }
    }
    public var host: String { URLComponents(string: device)?.host ?? "Displej" }
}
public struct OAuthClient: Codable, Equatable {
    public var client_id: String
    public var client_secret: String?
    public static func decodeFile(_ data: Data) throws -> Self {
        struct File: Decodable { var installed: OAuthClient? }
        guard let c = try JSONDecoder().decode(File.self, from: data).installed,
              !c.client_id.isEmpty else {
            throw BridgeError.message("Vyberte Google OAuth JSON typu Desktop app, nie webového klienta.")
        }
        return c
    }
}
public struct Token: Codable {
    public var access_token: String
    public var refresh_token: String?
    public var expires_in: Double?
    public var expiresAt: Date?
    public mutating func updateExpiry() { expiresAt = Date().addingTimeInterval(expires_in ?? 3600) }
}
public struct Secrets: Codable {
    public var pair: Pairing?
    public var client: OAuthClient?
    public var token: Token?
    public var selfID: String?
    public init() {}
}
public struct Space: Codable, Identifiable, Equatable {
    public var name: String
    public var displayName: String?
    public var id: String { name }
    public var title: String { displayName.flatMap { $0.isEmpty ? nil : $0 } ?? name }
    public init(name: String, displayName: String? = nil) { self.name = name; self.displayName = displayName }
}
public struct Checkpoint: Codable, Equatable {
    public var cursor: String
    public var latest: String?
    public var pageToken: String?
    public var accepted: [String] = []
    public init(cursor: String) { self.cursor = cursor }
    public static func now() -> Self {
        let f = ISO8601DateFormatter(); f.formatOptions = [.withInternetDateTime, .withFractionalSeconds]
        return Self(cursor: f.string(from: Date()))
    }
}
public struct Preferences: Codable {
    public var enabled = false
    public var claudeEnabled: Bool?
    public var accountID: String?
    public var spaces: [Space] = []
    public var checkpoints: [String: Checkpoint] = [:]
    public init() {}
    public mutating func select(_ space: Space, enabled: Bool) {
        if enabled {
            if !spaces.contains(where: { $0.id == space.id }) { spaces.append(space) }
            if checkpoints[space.id] == nil { checkpoints[space.id] = .now() }
        } else { spaces.removeAll { $0.id == space.id }; checkpoints[space.id] = nil }
    }
}
public struct ChatMessage: Decodable {
    public struct Sender: Decodable { public var name: String?; public var displayName: String? }
    public struct Attachment: Decodable { public var contentName: String? }
    public var name: String
    public var createTime: String
    public var sender: Sender?
    public var text: String?
    public var attachment: [Attachment]?
    public var deleteTime: String?
}
public struct MessagePage: Decodable { public var messages: [ChatMessage]?; public var nextPageToken: String? }
public struct Payload: Encodable {
    public var service = "google-chat"
    public var id: String
    public var sender: String
    public var room: String
    public var text: String
    public static func bounded(_ value: String, bytes: Int) -> String {
        guard value.utf8.count > bytes else { return value }
        var data = Array(value.utf8.prefix(max(0, bytes - 3)))
        while !data.isEmpty && String(bytes: data, encoding: .utf8) == nil { data.removeLast() }
        return (String(bytes: data, encoding: .utf8) ?? "") + "..."
    }
    public static func make(_ m: ChatMessage, space: Space) throws -> Payload? {
        var text = (m.text ?? "").trimmingCharacters(in: .whitespacesAndNewlines)
        if text.isEmpty, let attachments = m.attachment, !attachments.isEmpty {
            text = "[Attachment] " + attachments.map { $0.contentName ?? "file" }.joined(separator: ", ")
        }
        guard !text.isEmpty else { return nil }
        let hash = SHA256.hash(data: Data(m.name.utf8)).map { String(format: "%02x", $0) }.joined()
        var payload = Payload(id: "chat-" + hash,
                              sender: bounded(m.sender?.displayName ?? m.sender?.name ?? "Google Chat", bytes: 127),
                              room: bounded(space.title, bytes: 127), text: bounded(text, bytes: 1024))
        while try JSONEncoder().encode(payload).count > 2048 {
            payload.text = bounded(payload.text, bytes: max(3, payload.text.utf8.count - 128))
        }
        return payload
    }
}
