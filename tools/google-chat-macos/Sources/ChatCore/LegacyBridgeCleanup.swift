import Foundation

/// One-way removal of the retired 0.7.17–0.7.18 status-line integration.
/// Retain the backup/helper if a later user edit still depends on the helper.
public enum LegacyBridgeCleanup {
    public static func remove(folder: URL, fallbackSettings: URL) throws {
        let fm = FileManager.default
        let record = folder.appendingPathComponent("claude-install.json")
        let helper = folder.appendingPathComponent("ClaudeUsageRelay")
        let snapshot = folder.appendingPathComponent("claude-usage.json")
        guard [record, helper, snapshot].contains(where: { fm.fileExists(atPath: $0.path) }) else { return }
        func object(_ url: URL) throws -> [String: Any] {
            guard fm.fileExists(atPath: url.path) else { return [:] }
            guard (try url.resourceValues(forKeys: [.fileSizeKey]).fileSize ?? 0) <= 1024 * 1024,
                  let value = try JSONSerialization.jsonObject(with: Data(contentsOf: url)) as? [String: Any] else {
                throw BridgeError.message("Starú integráciu sa nepodarilo odstrániť. Záloha claude-install.json zostala v Application Support/MeteoChat.")
            }
            return value
        }
        let saved = try object(record)
        let settings: URL
        if let path = saved["settings"] as? String {
            guard path.hasPrefix("/"), URL(fileURLWithPath: path).lastPathComponent == "settings.json" else {
                throw BridgeError.message("Záloha starej integrácie má neplatnú cestu. Nastavenia sa nezmenili.")
            }
            settings = URL(fileURLWithPath: path)
        } else { settings = fallbackSettings }
        var config = try object(settings)
        let current = config["statusLine"] ?? NSNull()
        let matchesInstalled = saved["installed"].map {
            NSDictionary(dictionary: ["v": current]).isEqual(to: ["v": $0])
        } ?? false
        if matchesInstalled {
            guard let original = saved["original"] else {
                throw BridgeError.message("V zálohe starej integrácie chýba pôvodné nastavenie. Záloha a pomocník zostali zachované.")
            }
            if original is NSNull { config.removeValue(forKey: "statusLine") }
            else { config["statusLine"] = original }
            let data = try JSONSerialization.data(withJSONObject: config, options: [.prettyPrinted, .sortedKeys])
            try data.write(to: settings, options: [.atomic])
            try fm.setAttributes([.posixPermissions: 0o600], ofItemAtPath: settings.path)
        }
        let finalLine = config["statusLine"] ?? NSNull()
        let encoded = try JSONSerialization.data(withJSONObject: ["line": finalLine])
        if String(decoding: encoded, as: UTF8.self).contains("ClaudeUsageRelay") {
            throw BridgeError.message("Statusový riadok Claude Code bol ručne upravený a ešte používa starý pomocník. Obnovte ho podľa claude-install.json v Application Support/MeteoChat. Google Chat môže ďalej fungovať.")
        }
        // The record is removed last so a failed deletion can be retried on launch.
        for file in [snapshot, helper, record] where fm.fileExists(atPath: file.path) {
            try fm.removeItem(at: file)
        }
    }
}
