using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;
using System.Text.RegularExpressions;

namespace MeteoChat.Core;

public sealed class BridgeException(string message) : Exception(message);
public sealed class BridgeHttpException(int status) : Exception($"HTTP {status}")
{
    public int Status { get; } = status;
}
public static class Json
{
    public static readonly JsonSerializerOptions Options = new(JsonSerializerDefaults.Web);
    public static T Read<T>(byte[] data) => JsonSerializer.Deserialize<T>(data, Options)
        ?? throw new BridgeException("Služba vrátila neplatné dáta.");
    public static byte[] Write<T>(T value) => JsonSerializer.SerializeToUtf8Bytes(value, Options);
    public static T Copy<T>(T value) => Read<T>(Write(value));
}
public sealed record Pairing(string Device, string Key)
{
    public void Validate()
    {
        if (!Uri.TryCreate(Device, UriKind.Absolute, out var uri) ||
            (uri.Scheme != "http" && uri.Scheme != "https") || string.IsNullOrEmpty(uri.Host) ||
            uri.UserInfo != "" || uri.AbsolutePath != "/api/notifications" || uri.Query != "" ||
            uri.Fragment != "" || !Regex.IsMatch(Key, "\\A[0-9a-fA-F]{64}\\z"))
            throw new BridgeException("Neplatné párovanie. Vytvorte nový kód vo webovom UI displeja.");
    }
    [JsonIgnore] public string Host => new Uri(Device).Host;
    public static Uri Origin(string address)
    {
        var raw = address.Trim();
        if (!raw.Contains("://", StringComparison.Ordinal)) raw = "http://" + raw;
        if (!Uri.TryCreate(raw, UriKind.Absolute, out var uri) ||
            (uri.Scheme != "http" && uri.Scheme != "https") || string.IsNullOrEmpty(uri.Host) ||
            uri.UserInfo != "" || uri.AbsolutePath != "/" || uri.Query != "" || uri.Fragment != "")
            throw new BridgeException("Zadajte adresu displeja, napríklad meteoplaneradar.local alebo 192.168.1.42.");
        return uri;
    }
    public static async Task<Pairing> Claim(IBridgeHttp http, string address, string code, CancellationToken ct)
    {
        var origin = Origin(address);
        var digits = string.Concat(code.Where(c => !char.IsWhiteSpace(c)));
        if (!Regex.IsMatch(digits, "\\A[0-9]{6}\\z"))
            throw new BridgeException("Zadajte šesťmiestny kód z webového UI displeja.");
        using var request = Requests.Json(new Uri(origin, "/api/notifications/pair/claim"), new { code = digits });
        byte[] data;
        try { data = await http.Send(request, ct); }
        catch (BridgeHttpException e) when (e.Status is 403 or 410 or 429)
        { throw new BridgeException("Kód je nesprávny, vypršal alebo už bol použitý. Vygenerujte nový kód."); }
        catch (BridgeHttpException e) when (e.Status == 404)
        { throw new BridgeException("Aktualizujte firmware displeja: táto verzia ešte nepodporuje párovanie kódom."); }
        var reply = Json.Read<PairReply>(data);
        var pair = new Pairing(new Uri(origin, "/api/notifications").AbsoluteUri, reply.Key);
        pair.Validate(); return pair;
    }
    private sealed record PairReply(string Key);
}
public sealed record OAuthClient(
    [property: JsonPropertyName("client_id")] string ClientId,
    [property: JsonPropertyName("client_secret")] string? ClientSecret)
{
    public static OAuthClient Decode(byte[] bytes)
    {
        var client = Json.Read<ClientFile>(bytes).Installed;
        if (client == null || string.IsNullOrEmpty(client.ClientId) || client.ClientId.Length > 256 || !Regex.IsMatch(client.ClientId ?? "", "\\A[0-9]+-[A-Za-z0-9_-]+\\.apps\\.googleusercontent\\.com\\z") ||
            string.IsNullOrWhiteSpace(client.ClientSecret) || client.ClientSecret.Length > 4096 || client.ClientSecret.Contains("REPLACE_", StringComparison.Ordinal))
            throw new BridgeException("Vyberte skutočnú Google OAuth registráciu typu Desktop app.");
        return client;
    }
    public static byte[] Export(OAuthClient client) => Json.Write(new ClientFile(client));
    private sealed record ClientFile(OAuthClient? Installed);
}
public sealed record Token
{
    [JsonPropertyName("access_token")] public string AccessToken { get; set; } = "";
    [JsonPropertyName("refresh_token")] public string? RefreshToken { get; set; }
    [JsonPropertyName("expires_in")] public double ExpiresIn { get; set; } = 3600;
    public DateTimeOffset ExpiresAt { get; set; }
    public void SetExpiry()
    {
        if (string.IsNullOrWhiteSpace(AccessToken) || ExpiresIn <= 0 || ExpiresIn > 86400)
            throw new BridgeException("Google vrátil neplatné prihlásenie.");
        ExpiresAt = DateTimeOffset.UtcNow.AddSeconds(ExpiresIn);
    }
}
public sealed record Space(string Name, string? DisplayName = null)
{
    [JsonIgnore] public string Title => string.IsNullOrEmpty(DisplayName) ? Name : DisplayName;
    public void Validate()
    {
        if (!Regex.IsMatch(Name ?? "", "\\Aspaces/[A-Za-z0-9_-]+\\z"))
            throw new BridgeException("Google vrátil neplatný názov chatu.");
    }
}
public sealed record Checkpoint
{
    public string Cursor { get; set; } = DateTimeOffset.UtcNow.ToString("O");
    public string? Latest { get; set; }
    public string? PageToken { get; set; }
    public List<string> Accepted { get; set; } = [];
}
public sealed record BridgeState
{
    public Pairing? Pair { get; set; }
    public OAuthClient? Client { get; set; }
    public Token? Token { get; set; }
    public string? SelfId { get; set; }
    public bool Enabled { get; set; }
    public List<Space> Spaces { get; set; } = [];
    public Dictionary<string, Checkpoint> Checkpoints { get; set; } = [];
    public void Select(Space space, bool on)
    {
        space.Validate();
        if (on)
        {
            if (Spaces.All(s => s.Name != space.Name)) Spaces.Add(space);
            Checkpoints.TryAdd(space.Name, new Checkpoint());
        }
        else { Spaces.RemoveAll(s => s.Name == space.Name); Checkpoints.Remove(space.Name); }
    }
}
public sealed record ChatSender(string? Name, string? DisplayName);
public sealed record Attachment(string? ContentName);
public sealed record ChatMessage(string Name, string CreateTime, ChatSender? Sender = null,
    string? Text = null, List<Attachment>? Attachment = null, string? DeleteTime = null);
public sealed record MessagePage(List<ChatMessage>? Messages, string? NextPageToken);
public sealed record Payload(string Id, string Sender, string Room, string Text, string Service = "google-chat")
{
    public static string Bounded(string text, int maxBytes)
    {
        if (Encoding.UTF8.GetByteCount(text) <= maxBytes) return text;
        var result = new StringBuilder(); var used = 0;
        foreach (var rune in text.EnumerateRunes())
        {
            if (used + rune.Utf8SequenceLength > maxBytes - 3) break;
            result.Append(rune); used += rune.Utf8SequenceLength;
        }
        return result.ToString() + "...";
    }
    public static Payload? Make(ChatMessage message, Space space)
    {
        var text = (message.Text ?? "").Trim();
        if (text == "" && message.Attachment is { Count: > 0 })
            text = "[Attachment] " + string.Join(", ", message.Attachment.Select(a => a.ContentName ?? "file"));
        if (text == "") return null;
        var payload = new Payload("chat-" + Convert.ToHexStringLower(SHA256.HashData(Encoding.UTF8.GetBytes(message.Name))),
            Bounded(message.Sender?.DisplayName ?? message.Sender?.Name ?? "Google Chat", 127),
            Bounded(space.Title, 127), Bounded(text, 1024));
        while (Json.Write(payload).Length > 2048)
            payload = payload with { Text = Bounded(payload.Text, Math.Max(3, Encoding.UTF8.GetByteCount(payload.Text) - 128)) };
        return payload;
    }
}
public static class Errors
{
    // Deliberately never display exception messages containing HTTP URLs/bodies.
    public static string Describe(Exception error) => error switch
    {
        BridgeException e => e.Message,
        OperationCanceledException => "Operácia bola zrušená.",
        BridgeHttpException { Status: 401 } => "Prihlásenie alebo párovanie vypršalo. Prihláste Google alebo spárujte displej znova.",
        BridgeHttpException { Status: 403 } => "Prístup je zamietnutý. Skontrolujte Google Chat API a povolenia účtu alebo správcu.",
        BridgeHttpException { Status: 409 or 503 } => "Displej je zaneprázdnený alebo má plnú frontu. Doručenie sa zopakuje.",
        BridgeHttpException e => $"Služba vrátila HTTP {e.Status}. Pokus sa zopakuje.",
        _ => "Spojenie alebo odpoveď služby zlyhali. Skontrolujte sieť a skúste to znova."
    };
}
