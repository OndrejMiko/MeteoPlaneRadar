namespace MeteoChat.Core;

// The UI serializes operations; the worker must be stopped before settings change.
public sealed class ChatService(BridgeState state, IBridgeHttp http, Action<BridgeState> save)
{
    public BridgeState State { get; private set; } = state;
    public bool Configured => State.Pair != null && State.Client != null && State.Token != null && State.Spaces.Count > 0;
    public void Commit(BridgeState value) { save(value); State = value; }
    public void Change(Action<BridgeState> change)
    {
        var next = Json.Copy(State); change(next); Commit(next);
    }
    public void InstallClient(OAuthClient client)
    {
        if (State.Client == client) return;
        Change(next => {
            next.Client = client; next.Token = null; next.SelfId = null; next.Enabled = false;
            next.Spaces.Clear(); next.Checkpoints.Clear();
        });
    }
    public void ApplyBundledClient(OAuthClient client)
    {
        // An imported registration always wins; updating the same ID keeps login.
        if (State.Client == null) InstallClient(client);
        else if (State.Client.ClientId == client.ClientId && State.Client != client)
            Change(next => next.Client = client);
    }
    public async Task SignIn(Token token, CancellationToken ct)
    {
        using var request = new HttpRequestMessage(HttpMethod.Get, "https://openidconnect.googleapis.com/v1/userinfo");
        Requests.Bearer(request, token.AccessToken);
        var identity = Json.Read<Identity>(await http.Send(request, ct));
        if (string.IsNullOrWhiteSpace(identity.Sub)) throw new BridgeException("Google nevrátil identitu účtu.");
        Change(next => {
            if (next.SelfId != identity.Sub) { next.Spaces.Clear(); next.Checkpoints.Clear(); }
            next.SelfId = identity.Sub; next.Token = token;
        });
    }
    public void SignOut() => Change(next => {
        next.Token = null; next.SelfId = null; next.Enabled = false; next.Spaces.Clear(); next.Checkpoints.Clear();
    });
    private sealed record Identity(string Sub);
    private async Task<byte[]> Google(Uri url, CancellationToken ct)
    {
        var client = State.Client ?? throw new BridgeException("Najprv pripravte Google prihlásenie.");
        var token = State.Token ?? throw new BridgeException("Najprv sa prihláste cez Google.");
        if (token.ExpiresAt < DateTimeOffset.UtcNow.AddSeconds(60))
        {
            if (string.IsNullOrEmpty(token.RefreshToken)) throw new BridgeException("Prihláste sa znova cez Google.");
            var fields = new Dictionary<string,string> {
                ["client_id"] = client.ClientId, ["refresh_token"] = token.RefreshToken, ["grant_type"] = "refresh_token"
            };
            if (client.ClientSecret != null) fields["client_secret"] = client.ClientSecret;
            using var refresh = Requests.Form("https://oauth2.googleapis.com/token", fields);
            var renewed = Json.Read<Token>(await http.Send(refresh, ct));
            renewed.RefreshToken ??= token.RefreshToken; renewed.SetExpiry();
            Change(next => next.Token = renewed); token = renewed;
        }
        using var request = new HttpRequestMessage(HttpMethod.Get, url);
        Requests.Bearer(request, token.AccessToken);
        return await http.Send(request, ct);
    }
    private sealed record SpacePage(List<Space>? Spaces, string? NextPageToken);
    public async Task<List<Space>> ListSpaces(CancellationToken ct)
    {
        var result = new List<Space>(); var seen = new HashSet<string>(); string? pageToken = null;
        do
        {
            var query = new Dictionary<string,string> { ["pageSize"] = "100" };
            if (pageToken != null) query["pageToken"] = pageToken;
            var page = Json.Read<SpacePage>(await Google(Requests.Query("https://chat.googleapis.com/v1/spaces", query), ct));
            foreach (var space in page.Spaces ?? []) { space.Validate(); result.Add(space); }
            pageToken = string.IsNullOrEmpty(page.NextPageToken) ? null : page.NextPageToken;
            if (pageToken != null && !seen.Add(pageToken)) throw new BridgeException("Neplatné stránkovanie chatov.");
            if (result.Count > 10000) throw new BridgeException("Zoznam chatov je príliš veľký.");
        } while (pageToken != null);
        return result.DistinctBy(s => s.Name).ToList();
    }
    public async Task<bool> Device(Payload? payload, CancellationToken ct)
    {
        ct.ThrowIfCancellationRequested();
        var pair = State.Pair ?? throw new BridgeException("Najprv spárujte displej kódom."); pair.Validate();
        using var request = payload == null ? new HttpRequestMessage(HttpMethod.Get, pair.Device + "/ping")
            : Requests.Json(new Uri(pair.Device), payload);
        Requests.Bearer(request, pair.Key);
        var data = await http.Send(request, ct);
        return payload != null || Json.Read<Ping>(data).Enabled;
    }
    private sealed record Ping(bool Enabled);
    public Task<bool> TestDisplay(CancellationToken ct) => Device(new Payload("win-test-" + Guid.NewGuid(),
        "Meteo Chat pre Windows", "Test spojenia", "Mostík vo Windows je pripojený k displeju."), ct);
    public async Task<int> Poll(Space space, CancellationToken ct)
    {
        space.Validate();
        if (!State.Checkpoints.TryGetValue(space.Name, out var saved))
            throw new BridgeException("Vyberte chat znova, aby sa nastavil začiatok odberu.");
        var checkpoint = Json.Copy(saved);
        var fields = new Dictionary<string,string> {
            ["pageSize"] = "100", ["orderBy"] = "createTime ASC", ["filter"] = $"createTime > \"{checkpoint.Cursor}\""
        };
        if (checkpoint.PageToken != null) fields["pageToken"] = checkpoint.PageToken;
        var page = Json.Read<MessagePage>(await Google(Requests.Query("https://chat.googleapis.com/v1/" + space.Name + "/messages", fields), ct));
        var delivered = 0;
        foreach (var message in page.Messages ?? [])
        {
            ct.ThrowIfCancellationRequested();
            if (checkpoint.Accepted.Contains(message.Name)) continue;
            if (string.IsNullOrEmpty(message.Name) || !DateTimeOffset.TryParse(message.CreateTime, out _))
                throw new BridgeException("Google vrátil neplatnú správu.");
            var own = State.SelfId != null && message.Sender?.Name == "users/" + State.SelfId;
            if (!own && message.DeleteTime == null && Payload.Make(message, space) is { } payload)
            { await Device(payload, ct); ++delivered; }
            checkpoint.Accepted.Add(message.Name); checkpoint.Latest = message.CreateTime;
            Persist(checkpoint, space.Name); // Acknowledge only after successful delivery.
        }
        if (!string.IsNullOrEmpty(page.NextPageToken))
        {
            if (checkpoint.PageToken == page.NextPageToken) throw new BridgeException("Neplatné stránkovanie správ.");
            checkpoint.PageToken = page.NextPageToken;
        }
        else { checkpoint.Cursor = checkpoint.Latest ?? checkpoint.Cursor; checkpoint.Latest = null; checkpoint.PageToken = null; }
        checkpoint.Accepted.Clear(); Persist(checkpoint, space.Name);
        return delivered;
    }
    private void Persist(Checkpoint checkpoint, string name) => Change(next => next.Checkpoints[name] = Json.Copy(checkpoint));
}
