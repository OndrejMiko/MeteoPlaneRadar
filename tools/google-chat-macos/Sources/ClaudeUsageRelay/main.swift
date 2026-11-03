import Foundation
import ChatCore

// A short-lived local statusLine command. It never calls Anthropic or reads its
// tokens. Only the normalized rate limits are saved, never the input session JSON.
var args = Array(CommandLine.arguments.dropFirst())
func argument(_ name: String) -> String? {
    guard let i = args.firstIndex(of: name), i + 1 < args.count else { return nil }
    return args[i + 1]
}
guard let path = argument("--snapshot") else { exit(1) }
var input = Data()
while let chunk = try? FileHandle.standardInput.read(upToCount: 8192), !chunk.isEmpty {
    input.append(chunk)
    if input.count > 256 * 1024 { exit(1) }
}
let url = URL(fileURLWithPath: path)
do {
    let old = try? ClaudeUsageFile.read(url)
    if let next = try ClaudeUsageSnapshot.capture(input, previous: old) {
        try ClaudeUsageFile.write(JSONEncoder().encode(next), to: url)
    }
} catch { /* Do not break the user's terminal status line on a capture failure. */ }
if let command = argument("--forward") {
    let process = Process(), pipe = Pipe()
    process.executableURL = URL(fileURLWithPath: "/bin/sh")
    process.arguments = ["-c", command]
    process.standardInput = pipe
    process.standardOutput = FileHandle.standardOutput
    process.standardError = FileHandle.standardError
    do {
        try process.run()
        try pipe.fileHandleForWriting.write(contentsOf: input)
        try pipe.fileHandleForWriting.close()
        process.waitUntilExit()
        exit(process.terminationStatus)
    } catch { exit(1) }
} else {
    if let snapshot = try? ClaudeUsageFile.read(url) {
        print("Claude zostáva · 5h \(snapshot.remaining(snapshot.five_hour)) · 7d \(snapshot.remaining(snapshot.seven_day))")
    } else { print("Claude · čakám na limity") }
}
