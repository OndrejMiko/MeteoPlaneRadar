// swift-tools-version: 5.9
import PackageDescription
let package = Package(
    name: "MeteoChat",
    platforms: [.macOS(.v13)],
    products: [.executable(name: "MeteoChat", targets: ["MeteoChat"])],
    targets: [
        .target(name: "ChatCore"),
        .executableTarget(name: "ClaudeUsageRelay", dependencies: ["ChatCore"]),
        .executableTarget(name: "OAuthConfigTool", dependencies: ["ChatCore"]),
        .executableTarget(name: "MeteoChat", dependencies: ["ChatCore"]),
        .executableTarget(name: "ChatCoreChecks", dependencies: ["ChatCore"], path: "Tests/ChatCoreTests")
    ]
)
