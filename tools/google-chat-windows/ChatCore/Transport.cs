using System.Net.Http.Headers;

namespace MeteoChat.Core;

public interface IBridgeHttp
{
    Task<byte[]> Send(HttpRequestMessage request, CancellationToken cancellationToken = default);
}
public sealed class NativeHttp : IBridgeHttp, IDisposable
{
    public const int MaxResponse = 2 * 1024 * 1024;
    private readonly HttpClient client = new(new HttpClientHandler { AllowAutoRedirect = false, UseCookies = false })
    { Timeout = Timeout.InfiniteTimeSpan };
    public async Task<byte[]> Send(HttpRequestMessage request, CancellationToken cancellationToken = default)
    {
        using var deadline = CancellationTokenSource.CreateLinkedTokenSource(cancellationToken);
        deadline.CancelAfter(TimeSpan.FromSeconds(30));
        using var response = await client.SendAsync(request, HttpCompletionOption.ResponseHeadersRead, deadline.Token);
        if (!response.IsSuccessStatusCode) throw new BridgeHttpException((int)response.StatusCode);
        if (response.Content.Headers.ContentLength > MaxResponse)
            throw new BridgeException("Odpoveď služby je príliš veľká.");
        await using var stream = await response.Content.ReadAsStreamAsync(deadline.Token);
        using var output = new MemoryStream(); var buffer = new byte[8192];
        int size;
        while ((size = await stream.ReadAsync(buffer, deadline.Token)) != 0)
        {
            if (output.Length + size > MaxResponse) throw new BridgeException("Odpoveď služby je príliš veľká.");
            output.Write(buffer, 0, size);
        }
        return output.ToArray();
    }
    public void Dispose() => client.Dispose();
}
public static class Requests
{
    public static HttpRequestMessage Json<T>(Uri uri, T payload) => new(HttpMethod.Post, uri)
    { Content = new ByteArrayContent(Core.Json.Write(payload)) { Headers = { ContentType = new MediaTypeHeaderValue("application/json") } } };
    public static HttpRequestMessage Form(string uri, Dictionary<string,string> values) => new(HttpMethod.Post, uri)
    { Content = new FormUrlEncodedContent(values) };
    public static void Bearer(HttpRequestMessage request, string token) =>
        request.Headers.Authorization = new AuthenticationHeaderValue("Bearer", token);
    public static Uri Query(string uri, IEnumerable<KeyValuePair<string,string>> fields) =>
        new(uri + "?" + string.Join("&", fields.Select(p => Uri.EscapeDataString(p.Key) + "=" + Uri.EscapeDataString(p.Value))));
}
