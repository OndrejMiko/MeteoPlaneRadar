import Foundation

extension Pairing {
    public static func claim(address: String, code: String, http: HTTPClient = NativeHTTP()) async throws -> Pairing {
        let raw = address.trimmingCharacters(in: .whitespacesAndNewlines)
        let url = raw.contains("://") ? raw : "http://" + raw
        guard var origin = URLComponents(string: url), ["http", "https"].contains(origin.scheme ?? ""),
              let host = origin.host, !host.isEmpty, origin.user == nil, origin.password == nil,
              ["", "/"].contains(origin.path), origin.query == nil, origin.fragment == nil else {
            throw BridgeError.message("Zadajte adresu displeja, napríklad meteoplaneradar.local alebo http://192.168.1.42.")
        }
        let digits = code.components(separatedBy: .whitespacesAndNewlines).joined()
        guard digits.range(of: "^[0-9]{6}$", options: .regularExpression) != nil else {
            throw BridgeError.message("Zadajte šesťmiestny kód z webového UI displeja.")
        }
        origin.path = "/api/notifications/pair/claim"
        guard let endpoint = origin.url else { throw BridgeError.message("Neplatná adresa displeja.") }
        var request = URLRequest(url: endpoint); request.httpMethod = "POST"
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        request.httpBody = try JSONEncoder().encode(["code": digits])
        let data: Data
        do { data = try await http.send(request) }
        catch BridgeError.http(let status) {
            switch status {
            case 404: throw BridgeError.message("Displej ešte nepodporuje párovanie kódom. Aktualizujte jeho firmvér.")
            case 403, 410, 429: throw BridgeError.message("Kód je nesprávny, vypršal alebo už bol použitý. Vygenerujte nový kód vo webovom UI displeja.")
            default: throw BridgeError.http(status)
            }
        }
        struct Response: Decodable { var key: String }
        let response = try JSONDecoder().decode(Response.self, from: data)
        origin.path = "/api/notifications"
        let pair = Pairing(device: origin.url!.absoluteString, key: response.key)
        try pair.validate(); return pair
    }
}
