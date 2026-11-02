import Foundation
import Security
import ChatCore

enum Vault {
    private static let query: [String: Any] = [kSecClass as String: kSecClassGenericPassword,
        kSecAttrService as String: "sk.meteoplaneradar.chat", kSecAttrAccount as String: "bridge"]
    static func read() throws -> Secrets {
        var q = query; q[kSecReturnData as String] = true; q[kSecMatchLimit as String] = kSecMatchLimitOne
        var result: CFTypeRef?
        let status = SecItemCopyMatching(q as CFDictionary, &result)
        if status == errSecItemNotFound { return Secrets() }
        guard status == errSecSuccess, let data = result as? Data else {
            throw BridgeError.message("Kľúčenka nie je dostupná. Odomknite ju a aplikáciu spustite znova.")
        }
        return try JSONDecoder().decode(Secrets.self, from: data)
    }
    static func write(_ secrets: Secrets) throws {
        let data = try JSONEncoder().encode(secrets)
        var status = SecItemUpdate(query as CFDictionary, [kSecValueData as String: data] as CFDictionary)
        if status == errSecItemNotFound {
            var q = query; q[kSecValueData as String] = data
            q[kSecAttrAccessible as String] = kSecAttrAccessibleAfterFirstUnlockThisDeviceOnly
            q[kSecAttrLabel as String] = "Meteo Chat – Google a displej"
            status = SecItemAdd(q as CFDictionary, nil)
        }
        guard status == errSecSuccess else { throw BridgeError.message("Prihlásenie sa nepodarilo uložiť do Kľúčenky.") }
    }
}
enum LocalState {
    static var folder: URL {
        FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
            .appendingPathComponent("MeteoChat", isDirectory: true)
    }
    static func read() throws -> Preferences {
        let file = folder.appendingPathComponent("state.json")
        guard FileManager.default.fileExists(atPath: file.path) else { return Preferences() }
        return try JSONDecoder().decode(Preferences.self, from: Data(contentsOf: file))
    }
    static func write(_ value: Preferences) throws {
        try FileManager.default.createDirectory(at: folder, withIntermediateDirectories: true,
                                               attributes: [.posixPermissions: 0o700])
        try FileManager.default.setAttributes([.posixPermissions: 0o700], ofItemAtPath: folder.path)
        let file = folder.appendingPathComponent("state.json")
        try JSONEncoder().encode(value).write(to: file, options: .atomic)
        try FileManager.default.setAttributes([.posixPermissions: 0o600], ofItemAtPath: file.path)
    }
}
