import Foundation
import ChatCore

// Build-time tool, never reads Keychain or contacts Google. Errors contain no
// client ID/secret or raw JSON; the source file stays outside Git.
do {
    let args = Array(CommandLine.arguments.dropFirst())
    guard args.count == 2 || args.count == 3, ["validate", "export"].contains(args[0]),
          (args[0] == "validate" && args.count == 2) || (args[0] == "export" && args.count == 3) else {
        throw BridgeError.message("Použitie: OAuthConfigTool validate INPUT alebo OAuthConfigTool export INPUT OUTPUT")
    }
    let input = URL(fileURLWithPath: args[1])
    guard (try input.resourceValues(forKeys: [.fileSizeKey]).fileSize ?? 0) <= 65536 else {
        throw BridgeError.message("OAuth konfigurácia je príliš veľká.")
    }
    let client = try OAuthConfiguration.decode(Data(contentsOf: input))
    if args[0] == "export" {
        let output = URL(fileURLWithPath: args[2])
        try FileManager.default.createDirectory(at: output.deletingLastPathComponent(), withIntermediateDirectories: true,
                                               attributes: [.posixPermissions: 0o700])
        try OAuthConfiguration.export(client).write(to: output, options: .atomic)
        try FileManager.default.setAttributes([.posixPermissions: 0o600], ofItemAtPath: output.path)
    }
    print("Google Desktop OAuth konfigurácia je pripravená.")
} catch {
    let text = (error as? BridgeError)?.localizedDescription ?? "OAuth JSON sa nedá načítať. Použite stiahnutý súbor klienta typu Desktop app."
    FileHandle.standardError.write(Data((text + "\n").utf8)); exit(1)
}
