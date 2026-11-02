import Foundation

public enum OAuthConfiguration {
    // Only registration data is bundled. Ignore project metadata, arbitrary URLs
    // and any extra fields; user tokens/pairing keys never belong in this file.
    public static func decode(_ data: Data) throws -> OAuthClient {
        let client = try OAuthClient.decodeFile(data)
        guard client.client_id.range(of: "^[0-9]+-[A-Za-z0-9_-]+\\.apps\\.googleusercontent\\.com$", options: .regularExpression) != nil,
              let secret = client.client_secret, !secret.trimmingCharacters(in: .whitespacesAndNewlines).isEmpty,
              !secret.contains("REPLACE_") else {
            throw BridgeError.message("Doplňte skutočného Google OAuth klienta typu Desktop app vrátane client_id a client_secret. Ukážková konfigurácia sa nedá použiť.")
        }
        return client
    }
    public static func export(_ client: OAuthClient) throws -> Data {
        struct File: Encodable { var installed: OAuthClient }
        let encoder = JSONEncoder(); encoder.outputFormatting = [.prettyPrinted, .sortedKeys]
        return try encoder.encode(File(installed: client))
    }
    // Preserve a previously imported client and working account. The bundle is
    // the default for new installations; same-ID secret updates keep tokens.
    public static func applying(_ client: OAuthClient?, to saved: Secrets) -> Secrets {
        guard let client else { return saved }
        var result = saved
        if saved.client == nil {
            result.client = client; result.token = nil; result.selfID = nil
        } else if saved.client?.client_id == client.client_id {
            result.client = client
        }
        return result
    }
}
