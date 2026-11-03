import Foundation
import CryptoKit
import CoreFoundation

public struct ClaudeUsageWindow: Codable, Equatable {
    public var used_percentage: Double
    public var resets_at: Int64
}
public struct ClaudeUsageSnapshot: Codable, Equatable {
    public var observedAt: Int64
    public var five_hour: ClaudeUsageWindow?
    public var seven_day: ClaudeUsageWindow?
    // Fingerprint of Code's source data: a timer re-running the status line must
    // not make an unchanged cached payload appear freshly fetched from Claude.
    public var sourceMarker: String?
    public static func capture(_ data: Data, previous: Self? = nil, now: Date = Date()) throws -> Self? {
        guard data.count <= 256 * 1024,
              let root = try JSONSerialization.jsonObject(with: data) as? [String: Any] else {
            throw BridgeError.message("Neplatný vstup Claude Code.")
        }
        guard let limits = root["rate_limits"] as? [String: Any] else { return nil }
        let stamp = Int64(now.timeIntervalSince1970)
        func window(_ key: String) throws -> ClaudeUsageWindow? {
            guard let item = limits[key], !(item is NSNull) else { return nil }
            guard let v = item as? [String: Any], let used = v["used_percentage"] as? NSNumber,
                  CFGetTypeID(used) != CFBooleanGetTypeID(), let reset = v["resets_at"] as? NSNumber,
                  CFGetTypeID(reset) != CFBooleanGetTypeID(), used.doubleValue.isFinite,
                  (0...100).contains(used.doubleValue), reset.doubleValue.isFinite,
                  reset.doubleValue.rounded() == reset.doubleValue,
                  reset.doubleValue > Double(stamp), reset.doubleValue <= Double(stamp + 8 * 86400) else {
                throw BridgeError.message("Neplatné limity Claude Code.")
            }
            return ClaudeUsageWindow(used_percentage: used.doubleValue, resets_at: reset.int64Value)
        }
        // An already expired window in Code's cached JSON becomes unknown.
        var filtered = limits
        for key in ["five_hour", "seven_day"] {
            if let v = limits[key] as? [String: Any], let reset = v["resets_at"] as? NSNumber,
               reset.doubleValue <= Double(stamp) { filtered.removeValue(forKey: key) }
        }
        // Parse using the original input, except for explicitly expired windows.
        let five = filtered["five_hour"] == nil ? nil : try window("five_hour")
        let week = filtered["seven_day"] == nil ? nil : try window("seven_day")
        let markerData = try JSONSerialization.data(withJSONObject: [
            "limits": filtered, "session": root["session_id"] ?? "",
            "prompt": root["prompt_id"] ?? "",
            "apiDuration": (root["cost"] as? [String: Any])?["total_api_duration_ms"] ?? 0
        ], options: [.sortedKeys])
        let marker = SHA256.hash(data: markerData).map { String(format: "%02x", $0) }.joined()
        if previous?.sourceMarker == marker { return previous }
        return Self(observedAt: stamp, five_hour: five, seven_day: week, sourceMarker: marker)
    }
    public func validate(now: Date = Date()) throws {
        let stamp = Int64(now.timeIntervalSince1970)
        guard observedAt >= stamp - 7 * 86400, observedAt <= stamp + 60 else {
            throw BridgeError.message("Údaje Claude sú príliš staré. Použite Claude Code na obnovenie.")
        }
        for w in [five_hour, seven_day].compactMap({ $0 }) {
            guard w.used_percentage.isFinite, (0...100).contains(w.used_percentage),
                  w.resets_at > observedAt, w.resets_at <= observedAt + 8 * 86400 else {
                throw BridgeError.message("Neplatné limity Claude Code.")
            }
        }
    }
    public func wireData() throws -> Data {
        var copy = self; copy.sourceMarker = nil
        return try JSONEncoder().encode(copy)
    }
    public func remaining(_ window: ClaudeUsageWindow?, now: Date = Date()) -> String {
        guard let w = window, w.used_percentage.isFinite, (0...100).contains(w.used_percentage),
              w.resets_at > Int64(now.timeIntervalSince1970) else { return "—" }
        return "\(Int(floor(100 - w.used_percentage))) %"
    }
}

public enum ClaudeUsageFile {
    public static func read(_ url: URL) throws -> ClaudeUsageSnapshot? {
        guard FileManager.default.fileExists(atPath: url.path) else { return nil }
        guard (try url.resourceValues(forKeys: [.fileSizeKey]).fileSize ?? 0) <= 4096 else {
            throw BridgeError.message("Súbor limitov Claude je príliš veľký.")
        }
        return try JSONDecoder().decode(ClaudeUsageSnapshot.self, from: Data(contentsOf: url))
    }
    public static func write(_ data: Data, to url: URL) throws {
        let fm = FileManager.default
        try fm.createDirectory(at: url.deletingLastPathComponent(), withIntermediateDirectories: true,
                               attributes: [.posixPermissions: 0o700])
        try data.write(to: url, options: [.atomic])
        try fm.setAttributes([.posixPermissions: 0o600], ofItemAtPath: url.path)
    }
}

/// Changes only statusLine and restores it only if it still matches our command.
/// No Claude credentials, transcripts or message content are read.
public struct ClaudeUsageInstallation {
    public let settings: URL, record: URL, snapshot: URL, relay: URL
    public init(settings: URL, folder: URL, relay: URL) {
        self.settings = settings; self.relay = relay
        record = folder.appendingPathComponent("claude-install.json")
        snapshot = folder.appendingPathComponent("claude-usage.json")
    }
    private func object(_ url: URL) throws -> [String: Any] {
        guard FileManager.default.fileExists(atPath: url.path) else { return [:] }
        guard (try url.resourceValues(forKeys: [.fileSizeKey]).fileSize ?? 0) <= 1024 * 1024,
              let value = try JSONSerialization.jsonObject(with: Data(contentsOf: url)) as? [String: Any] else {
            throw BridgeError.message("Nastavenia Claude Code sa nedajú bezpečne upraviť.")
        }
        return value
    }
    public var installed: Bool { FileManager.default.fileExists(atPath: record.path) }
    private func matches(_ a: Any, _ b: Any) -> Bool {
        NSDictionary(dictionary: ["v": a]).isEqual(to: ["v": b])
    }
    private func checkPath(_ saved: [String: Any]) throws {
        if let path = saved["settings"] as? String, path != settings.path {
            throw BridgeError.message("Claude bol pripojený z iného priečinka nastavení. Použite pôvodný CLAUDE_CONFIG_DIR alebo obnovte zálohu claude-install.json.")
        }
    }
    public static func quote(_ s: String) -> String { "'" + s.replacingOccurrences(of: "'", with: "'\\''") + "'" }
    public func enable() throws {
        guard FileManager.default.isExecutableFile(atPath: relay.path) else {
            throw BridgeError.message("Chýba pomocník Claude. Nainštalujte aktuálnu aplikáciu do Applications.")
        }
        var config = try object(settings)
        let saved = try object(record)
        try checkPath(saved)
        let current = config["statusLine"] ?? NSNull()
        if !saved.isEmpty && !matches(current, saved["installed"] ?? NSNull()) &&
            !matches(current, saved["original"] ?? NSNull()) {
            throw BridgeError.message("Nastavenie statusLine sa medzitým zmenilo. Obnovte ho podľa claude-install.json; vlastná zmena sa neprepísala.")
        }
        let original = saved["original"] ?? current
        let old = original as? [String: Any]
        if !(original is NSNull) && (old?["type"] as? String != "command" || old?["command"] as? String == nil) {
            throw BridgeError.message("Existujúci statusLine má nepodporovaný formát. Nastavenie sa nezmenilo.")
        }
        var line = old ?? [:]
        line["type"] = "command"
        var command = Self.quote(relay.path) + " --snapshot " + Self.quote(snapshot.path)
        if let forward = old?["command"] as? String { command += " --forward " + Self.quote(forward) }
        line["command"] = command
        let backup: [String: Any] = ["original": original, "installed": line, "settings": settings.path]
        try ClaudeUsageFile.write(JSONSerialization.data(withJSONObject: backup, options: [.prettyPrinted, .sortedKeys]), to: record)
        config["statusLine"] = line
        try ClaudeUsageFile.write(JSONSerialization.data(withJSONObject: config, options: [.prettyPrinted, .sortedKeys]), to: settings)
    }
    public func disable() throws {
        let saved = try object(record)
        guard !saved.isEmpty else { return }
        try checkPath(saved)
        var config = try object(settings)
        let current = config["statusLine"] ?? NSNull()
        guard matches(current, saved["installed"] ?? NSNull()) || matches(current, saved["original"] ?? NSNull()) else {
            throw BridgeError.message("Vlastná zmena statusLine zostala zachovaná. Obnovte nastavenie podľa claude-install.json.")
        }
        if saved["original"] is NSNull { config.removeValue(forKey: "statusLine") }
        else { config["statusLine"] = saved["original"] }
        try ClaudeUsageFile.write(JSONSerialization.data(withJSONObject: config, options: [.prettyPrinted, .sortedKeys]), to: settings)
        try FileManager.default.removeItem(at: record)
        try? FileManager.default.removeItem(at: snapshot)
    }
}
