using System.Net;
using System.Net.Sockets;
using System.Security.Cryptography;
using System.Text;

namespace MeteoChat.Core;

public static class OAuthTools
{
    public const string Scope = "openid https://www.googleapis.com/auth/chat.spaces.readonly https://www.googleapis.com/auth/chat.messages.readonly";
    public static string Base64(byte[] value) => Convert.ToBase64String(value).TrimEnd('=').Replace('+', '-').Replace('/', '_');
    public static string Random() => Base64(RandomNumberGenerator.GetBytes(48));
    public static string Challenge(string verifier) => Base64(SHA256.HashData(Encoding.ASCII.GetBytes(verifier)));
    public static bool Equal(string a, string b) => CryptographicOperations.FixedTimeEquals(
        SHA256.HashData(Encoding.UTF8.GetBytes(a)), SHA256.HashData(Encoding.UTF8.GetBytes(b)));
    public static string? Callback(string target, string state)
    {
        if (!target.StartsWith("/callback?", StringComparison.Ordinal) || target.Contains('#')) return null;
        var values = new Dictionary<string,List<string>>();
        foreach (var field in target[10..].Split('&'))
        {
            var pair = field.Split('=', 2);
            var key = Uri.UnescapeDataString(pair[0].Replace('+', ' '));
            var value = pair.Length == 2 ? Uri.UnescapeDataString(pair[1].Replace('+', ' ')) : "";
            if (!values.TryGetValue(key, out var list)) values[key] = list = [];
            list.Add(value);
        }
        if (!values.TryGetValue("state", out var states) || states.Count != 1 || !Equal(states[0], state)) return null;
        if (values.ContainsKey("error")) throw new BridgeException("Prihlásenie bolo zrušené v prehliadači.");
        return values.TryGetValue("code", out var codes) && codes.Count == 1 && !string.IsNullOrEmpty(codes[0]) ? codes[0] : null;
    }
}
public sealed class GoogleOAuth(IBridgeHttp http, Func<Uri,bool> openBrowser)
{
    public async Task<Token> SignIn(OAuthClient client, CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        var state = OAuthTools.Random(); var verifier = OAuthTools.Random();
        using var timeout = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
        timeout.CancelAfter(TimeSpan.FromMinutes(5));
        var server = new TcpListener(IPAddress.Loopback, 0);
        string code, redirect;
        server.Start(8);
        try
        {
            var port = ((IPEndPoint)server.LocalEndpoint).Port;
            redirect = $"http://127.0.0.1:{port}/callback";
            var fields = new Dictionary<string,string> {
                ["client_id"] = client.ClientId, ["redirect_uri"] = redirect, ["response_type"] = "code",
                ["scope"] = OAuthTools.Scope, ["state"] = state, ["code_challenge"] = OAuthTools.Challenge(verifier),
                ["code_challenge_method"] = "S256", ["access_type"] = "offline", ["prompt"] = "consent"
            };
            if (!openBrowser(Requests.Query("https://accounts.google.com/o/oauth2/v2/auth", fields)))
                throw new BridgeException("Nepodarilo sa otvoriť prehliadač. Skúste prihlásenie znova.");
            while (true)
            {
                using var connection = await server.AcceptTcpClientAsync(timeout.Token);
                using var readDeadline = CancellationTokenSource.CreateLinkedTokenSource(timeout.Token);
                readDeadline.CancelAfter(TimeSpan.FromSeconds(10));
                try
                {
                    await using var stream = connection.GetStream();
                    var bytes = new byte[8192]; var used = 0;
                    while (used < bytes.Length)
                    {
                        var read = await stream.ReadAsync(bytes.AsMemory(used), readDeadline.Token);
                        if (read == 0) break;
                        used += read;
                        if (Encoding.ASCII.GetString(bytes, 0, used).Contains("\r\n\r\n", StringComparison.Ordinal)) break;
                    }
                    var request = Encoding.UTF8.GetString(bytes, 0, used);
                    if (used == bytes.Length || !request.Contains("\r\n\r\n", StringComparison.Ordinal)) continue;
                    var parts = request.Split("\r\n", 2)[0].Split(' ');
                    if (parts.Length != 3 || parts[0] != "GET") continue;
                    string? result;
                    try { result = OAuthTools.Callback(parts[1], state); }
                    catch (BridgeException) { await Reply(stream, false, readDeadline.Token); throw; }
                    await Reply(stream, result != null, readDeadline.Token);
                    if (result != null) { code = result; break; }
                }
                catch (Exception e) when ((e is IOException or OperationCanceledException) && !timeout.IsCancellationRequested)
                { /* Ignore a stalled/unrelated local connection, never its contents. */ }
            }
        }
        finally { server.Stop(); }
        var exchange = new Dictionary<string,string> {
            ["client_id"] = client.ClientId, ["code"] = code, ["code_verifier"] = verifier,
            ["redirect_uri"] = redirect, ["grant_type"] = "authorization_code"
        };
        if (client.ClientSecret != null) exchange["client_secret"] = client.ClientSecret;
        using var requestToken = Requests.Form("https://oauth2.googleapis.com/token", exchange);
        var token = Json.Read<Token>(await http.Send(requestToken, timeout.Token));
        if (string.IsNullOrEmpty(token.RefreshToken))
            throw new BridgeException("Google nevydal trvalé prihlásenie. Odoberte aplikácii prístup v Google účte a prihláste sa znova.");
        token.SetExpiry(); return token;
    }
    private static async Task Reply(NetworkStream stream, bool ok, CancellationToken ct)
    {
        var body = Encoding.UTF8.GetBytes(ok ? "Prihlásenie bolo prijaté. Vráťte sa do aplikácie Meteo Chat. Túto kartu môžete zavrieť."
            : "Neplatný návrat. Vráťte sa do aplikácie Meteo Chat.");
        var header = Encoding.ASCII.GetBytes($"HTTP/1.1 {(ok ? "200 OK" : "400 Bad Request")}\r\nContent-Type: text/plain; charset=utf-8\r\nCache-Control: no-store\r\nContent-Length: {body.Length}\r\nConnection: close\r\n\r\n");
        await stream.WriteAsync(header, ct); await stream.WriteAsync(body, ct);
    }
}
