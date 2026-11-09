using System.Net;
using System.Net.Sockets;
using System.Text;
using MeteoChat.Core;

static class Checks
{
    static int assertions;
    static void Check(bool value, string label)
    { ++assertions; if (!value) throw new Exception("Check failed: " + label); }
    static async Task Fails<T>(Func<Task> work, string label) where T:Exception
    {
        try { await work(); } catch (T) { ++assertions; return; }
        throw new Exception("Expected " + typeof(T).Name + ": " + label);
    }
    static void Fails<T>(Action work, string label) where T:Exception => Fails<T>(()=>{work();return Task.CompletedTask;},label).GetAwaiter().GetResult();
    static readonly OAuthClient Client = new("123-example.apps.googleusercontent.com", "synthetic-registration");
    static readonly Pairing Pair = new("http://127.0.0.1:8080/api/notifications",new string('a',64));
    static readonly Space Space = new("spaces/AAA", "Pracovný chat");
    static BridgeState Ready() => new() {
        Client=Client,Pair=Pair,SelfId="self",Token=new Token{AccessToken="synthetic-access",RefreshToken="synthetic-refresh",ExpiresAt=DateTimeOffset.UtcNow.AddHours(1)},
        Spaces=[Space],Checkpoints=new(){[Space.Name]=new Checkpoint{Cursor="2026-11-09T08:00:00Z"}}
    };
    static byte[] Data<T>(T data)=>Json.Write(data);
    static byte[] Page(params ChatMessage[] messages)=>Data(new MessagePage([..messages],null));
    static ChatMessage Message(string id, string text="Správa", string time="2026-11-09T09:00:00Z") =>
        new("spaces/AAA/messages/"+id,time,new ChatSender("users/other","Matúš"),text);
    public static async Task<int> Main(string[] args)
    {
        try
        {
            if (args is ["--export-oauth",var input,var output]) {
                if (new FileInfo(input).Length>1024*1024) throw new BridgeException("Registračný súbor je príliš veľký.");
                File.WriteAllBytes(output,OAuthClient.Export(OAuthClient.Decode(File.ReadAllBytes(input))));
                Console.WriteLine("Google Desktop app registration validated and sanitized."); return 0;
            }
            await Validation(); await Delivery(); await RefreshAndAccounts(); await Login(); await Transport(); Storage();
            Console.WriteLine($"Windows bridge: {assertions} checks passed (pairing, OAuth/PKCE/loopback, pagination/retry, refresh, UTF-8, HTTP limits/redirects and storage).\nWindows-only DPAPI/UI checks: run MeteoChat.exe --self-test on Windows.");
            return 0;
        }
        catch(Exception error) { Console.Error.WriteLine(args.Length==0?error.ToString():Errors.Describe(error)); return 1; }
    }
    static async Task Validation()
    {
        Pair.Validate();
        foreach(var url in new[]{"file:///tmp/test","http://u:p@host/api/notifications","http://host/other","http://host/api/notifications?q=1","http://host/api/notifications#secret"})
            Fails<BridgeException>(()=>new Pairing(url,Pair.Key).Validate(),"pair endpoint");
        Fails<BridgeException>(()=>new Pairing(Pair.Device,"bad").Validate(),"key");
        foreach(var address in new[]{"http://u:p@host","http://host/api/notifications","ftp://host","http://host/?x=1"})
            Fails<BridgeException>(()=>Pairing.Origin(address),"origin");
        var fake=new FakeHttp((r,_)=>{
            Check(r.Uri.AbsolutePath=="/api/notifications/pair/claim","claim path");
            Check(r.Body.Contains("001234",StringComparison.Ordinal)&&r.Authorization==null,"leading zeros and no credential on claim");
            return Data(new{key=Pair.Key});
        });
        var pair=await Pairing.Claim(fake,"127.0.0.1:8080","001 234",default);
        Check(pair==Pair,"claimed pair");
        await Fails<BridgeException>(()=>Pairing.Claim(fake,"127.0.0.1","１２３４５６",default),"ASCII digits");
        await Fails<BridgeException>(()=>Pairing.Claim(new FakeHttp((_,_)=>throw new BridgeHttpException(410)),"127.0.0.1","123456",default),"expired code");
        Fails<BridgeException>(()=>OAuthClient.Decode(Data(new{installed=new{client_secret="synthetic"}})),"missing client ID");
        var extra=Data(new{installed=new{client_id=Client.ClientId,client_secret=Client.ClientSecret,token="must-not-export",redirect_uris=new[]{"https://evil.test"}}});
        Check(OAuthClient.Decode(extra)==Client,"desktop registration");
        Check(!Encoding.UTF8.GetString(OAuthClient.Export(OAuthClient.Decode(extra))).Contains("must-not-export",StringComparison.Ordinal),"sanitized registration");
        Fails<BridgeException>(()=>OAuthClient.Decode(Data(new{web=new{client_id=Client.ClientId,client_secret=Client.ClientSecret}})),"web client refused");
        foreach(var text in new[]{new string('\\',2000),string.Concat(Enumerable.Repeat("ľščťž😀",500)),"hello"+new string('\n',1500)})
        {
            var payload=Payload.Make(Message("id",text),Space)!;
            Check(Data(payload).Length<=2048,"escaped payload <=2048");
            Check(Encoding.UTF8.GetByteCount(payload.Text)<=1024,"UTF8 text bound");
            Check(!payload.Text.Contains('\ufffd'),"valid UTF8 truncation");
        }
        Check(Payload.Make(Message("same"),Space)!.Id==Payload.Make(Message("same"),Space)!.Id,"stable dedupe id");
        Check(Payload.Make(Message("blank","  "),Space)==null,"empty skipped");
        Check(Payload.Make(Message("file","") with{Attachment=[new Attachment("map.png")]},Space)!.Text.Contains("map.png",StringComparison.Ordinal),"attachment");
        Check(!Errors.Describe(new IOException("secret-url?token=private")).Contains("private",StringComparison.Ordinal),"redacted errors");
    }
    static async Task Delivery()
    {
        var sent=new List<string>();var fail=true;BridgeState? persisted=null;
        var messages=new[]{Message("1"),Message("2"),Message("3")};
        var http=new FakeHttp((r,_)=>{
            if(r.Uri.Host=="chat.googleapis.com") return Page(messages);
            var payload=Json.Read<Payload>(Encoding.UTF8.GetBytes(r.Body));
            if(payload.Id==Payload.Make(messages[1],Space)!.Id&&fail)throw new BridgeHttpException(503);
            sent.Add(payload.Id);return Data(new{ok=true});
        });
        var service=new ChatService(Ready(),http,next=>persisted=Json.Copy(next));
        await Fails<BridgeHttpException>(()=>service.Poll(Space,default),"device backpressure");
        Check(persisted!.Checkpoints[Space.Name].Accepted.Count==1,"first ACK persisted");
        Check(persisted.Checkpoints[Space.Name].Cursor.EndsWith("08:00:00Z",StringComparison.Ordinal),"cursor stable midpage");
        fail=false;service=new ChatService(persisted,http,next=>persisted=Json.Copy(next));
        Check(await service.Poll(Space,default)==2,"resume only unacknowledged messages");
        Check(sent.Count==3&&sent.Distinct().Count()==3,"no duplicates on retry/restart");
        Check(service.State.Checkpoints[Space.Name].Accepted.Count==0,"clear ACK page after commit");
        var queries=new List<string>();var pageNumber=0;
        http=new FakeHttp((r,_)=>{
            if(r.Uri.Host!="chat.googleapis.com")return Data(new{ok=true});
            queries.Add(Uri.UnescapeDataString(r.Uri.Query));++pageNumber;
            return pageNumber==1?Data(new MessagePage([Message("a")],"next-page")):Page(Message("b"));
        });
        service=new ChatService(Ready(),http,_=>{});
        await service.Poll(Space,default);await service.Poll(Space,default);
        Check(queries[1].Contains("pageToken=next-page",StringComparison.Ordinal),"page token used");
        Check(queries.All(q=>q.Contains("08:00:00Z",StringComparison.Ordinal)),"equal timestamp lower bound stable across pages");
        var delivered=0;
        http=new FakeHttp((r,_)=>{
            if(r.Uri.Host=="chat.googleapis.com")return Page(Message("own") with{Sender=new ChatSender("users/self","Me")},Message("deleted") with{DeleteTime="2026-11-09T10:00:00Z"},Message("blank",""));
            ++delivered;return Data(new{ok=true});
        });
        service=new ChatService(Ready(),http,_=>{});
        Check(await service.Poll(Space,default)==0&&delivered==0,"own/deleted/empty messages skipped");
        service=new ChatService(Ready(),new FakeHttp((_,_)=>Page(Message("a"))),_=>throw new IOException("disk full"));
        await Fails<IOException>(()=>service.Poll(Space,default),"state save failure");
        Check(service.State.Checkpoints[Space.Name].Accepted.Count==0,"in-memory checkpoint not advanced on failed save");
        using var stop=new CancellationTokenSource();stop.Cancel();
        await Fails<OperationCanceledException>(()=>service.Device(null,stop.Token),"cancel before delivery");
        Fails<BridgeException>(()=>new Space("spaces/AAA/../../userinfo").Validate(),"no path injection");
    }
    static async Task RefreshAndAccounts()
    {
        var state=Ready();state.Token!.ExpiresAt=DateTimeOffset.MinValue;var refreshCalls=0;
        var http=new FakeHttp((r,_)=>{
            if(r.Uri.AbsoluteUri=="https://oauth2.googleapis.com/token"){
                ++refreshCalls;Check(r.Body.Contains("synthetic-refresh",StringComparison.Ordinal),"refresh token sent only to token URL");
                return Data(new{access_token="renewed",expires_in=3600});
            }
            Check(r.Authorization=="Bearer renewed","renewed bearer");return Data(new{spaces=new[]{Space}});
        });
        var service=new ChatService(state,http,_=>{});
        Check((await service.ListSpaces(default)).Count==1&&refreshCalls==1,"refresh and spaces");
        Check(service.State.Token!.RefreshToken=="synthetic-refresh","refresh token retained when not rotated");
        service=new ChatService(Ready(),new FakeHttp((_,_)=>Data(new{sub="another-account"})),_=>{});
        await service.SignIn(new Token{AccessToken="new"},default);
        Check(service.State.Spaces.Count==0&&service.State.Checkpoints.Count==0&&service.State.Pair==Pair,"account switch resets cursors, keeps pair");
        service.SignOut();Check(service.State.Pair==Pair&&service.State.Token==null&&!service.State.Enabled,"sign out");
        service=new ChatService(Ready(),http,_=>{});service.ApplyBundledClient(Client with{ClientSecret="updated"});
        Check(service.State.Token!=null&&service.State.Client!.ClientSecret=="updated","same-ID bundle preserves token");
        service.ApplyBundledClient(Client with{ClientId="456-other.apps.googleusercontent.com"});
        Check(service.State.Client!.ClientId==Client.ClientId,"bundle cannot replace imported identity");
        var selected=new BridgeState();selected.Select(Space,true);var cursor=selected.Checkpoints[Space.Name].Cursor;
        selected.Select(Space,true);Check(selected.Spaces.Count==1&&selected.Checkpoints[Space.Name].Cursor==cursor,"select starts now once");
        selected.Select(Space,false);Check(selected.Checkpoints.Count==0,"unselect drops cursor");
        var paging=new FakeHttp((_,_)=>Data(new{spaces=new[]{Space},nextPageToken="loop"}));
        await Fails<BridgeException>(()=>new ChatService(Ready(),paging,_=>{}).ListSpaces(default),"pagination loop rejected");
    }
    static async Task Login()
    {
        Check(OAuthTools.Challenge("dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk")=="E9Melhoa2OwvFrEMTJguCHaoeK1t8URWbuGJSstw-cM","RFC7636 PKCE vector");
        Check(OAuthTools.Callback("/callback?state=valid&code=a%2Bb","valid")=="a+b","code decoding");
        foreach(var target in new[]{"/other?state=valid&code=a","/callback?state=bad&code=a","/callback?state=valid&state=valid&code=a","/callback?state=valid&code=a&code=b","https://evil.test/callback?state=valid&code=a","//evil.test/callback?state=valid&code=a"})
            Check(OAuthTools.Callback(target,"valid")==null,"callback rejected");
        Fails<BridgeException>(()=>OAuthTools.Callback("/callback?state=valid&error=access_denied","valid"),"cancelled browser");
        Uri? authorize=null;Task? callback=null;
        var tokenHttp=new FakeHttp((r,_)=>{
            Check(r.Uri.AbsoluteUri=="https://oauth2.googleapis.com/token","exchange endpoint");
            var form=Query(r.Body);Check(form["code"]=="accepted+code","loopback code");
            Check(OAuthTools.Challenge(form["code_verifier"])==Query(authorize!.Query[1..])["code_challenge"],"verifier matches challenge");
            return Data(new{access_token="access",refresh_token="refresh",expires_in=3600});
        });
        var flow=new GoogleOAuth(tokenHttp,url=>{
            authorize=url;var query=Query(url.Query[1..]);
            Check(url.Host=="accounts.google.com"&&query["code_challenge_method"]=="S256","browser OAuth");
            Check(query["scope"]==OAuthTools.Scope,"read-only scopes");
            var redirect=new Uri(query["redirect_uri"]);Check(redirect.Host=="127.0.0.1","loopback only");
            callback=Task.Run(async()=>{
                using var client=new HttpClient();
                using var bad=await client.GetAsync(redirect+"?state=wrong&code=ignore");Check(bad.StatusCode==HttpStatusCode.BadRequest,"wrong state does not finish");
                using var good=await client.GetAsync(redirect+"?state="+query["state"]+"&code=accepted%2Bcode");good.EnsureSuccessStatusCode();
            });return true;
        });
        var token=await flow.SignIn(Client,default);await callback!;
        Check(token.RefreshToken=="refresh"&&token.ExpiresAt>DateTimeOffset.UtcNow,"real loopback exchange");
        var closedPort=new Uri(Query(authorize!.Query[1..])["redirect_uri"]).Port;
        using(var socket=new TcpClient())await Fails<SocketException>(()=>socket.ConnectAsync(IPAddress.Loopback,closedPort),"listener closed");
        using var cancelled=new CancellationTokenSource();cancelled.Cancel();
        await Fails<OperationCanceledException>(()=>new GoogleOAuth(tokenHttp,_=>true).SignIn(Client,cancelled.Token),"login cancellation");
    }
    static Dictionary<string,string> Query(string input)=>input.Split('&').Select(p=>p.Split('=',2)).ToDictionary(p=>Uri.UnescapeDataString(p[0].Replace('+',' ')),p=>Uri.UnescapeDataString(p[1].Replace('+',' ')));
    static async Task Transport()
    {
        var listener=new TcpListener(IPAddress.Loopback,0);listener.Start();
        var origin="http://127.0.0.1:"+((IPEndPoint)listener.LocalEndpoint).Port;
        var server=Task.Run(async()=>{
            for(var i=0;i<2;++i){
                using var conn=await listener.AcceptTcpClientAsync();await using var stream=conn.GetStream();
                var buffer=new byte[4096];var received=await stream.ReadAsync(buffer);Check(received>0,"test HTTP request received");
                var response=i==0?$"HTTP/1.1 302 Found\r\nLocation: {origin}/must-not-follow\r\nContent-Length: 0\r\nConnection: close\r\n\r\n":"HTTP/1.1 200 OK\r\nContent-Length: 2097153\r\nConnection: close\r\n\r\n";
                await stream.WriteAsync(Encoding.ASCII.GetBytes(response));
            }
        });
        try{
            using var native=new NativeHttp();
            using var redirect=new HttpRequestMessage(HttpMethod.Get,origin+"/redirect");Requests.Bearer(redirect,"synthetic-credential");
            await Fails<BridgeHttpException>(()=>native.Send(redirect),"no bearer redirects");
            using var huge=new HttpRequestMessage(HttpMethod.Get,origin+"/huge");
            await Fails<BridgeException>(()=>native.Send(huge),"response size cap");await server;
        }finally{listener.Stop();}
    }
    static void Storage()
    {
        var dir=Path.Combine(Path.GetTempPath(),"meteo-chat-check-"+Guid.NewGuid().ToString("N"));
        try{
            var protector=new TestProtector();var store=new StateStore(dir,protector);store.Write(Ready());
            Check(store.Read().Pair==Pair,"encrypted storage roundtrip");
            var previous=File.ReadAllBytes(Path.Combine(dir,"state.dat"));
            var oversized=Ready();oversized.Spaces=[new Space(Space.Name,new string('x',NativeHttp.MaxResponse))];
            Fails<BridgeException>(()=>store.Write(oversized),"state remains readable under size cap");
            Check(File.ReadAllBytes(Path.Combine(dir,"state.dat")).SequenceEqual(previous),"oversized state preserves old file");
            protector.Fail=true;
            Fails<IOException>(()=>store.Write(new BridgeState()),"failed protection preserves old state");
            Check(File.ReadAllBytes(Path.Combine(dir,"state.dat")).SequenceEqual(previous),"old file intact");
            protector.Fail=false;File.WriteAllText(Path.Combine(dir,"state.dat"),"invalid");
            Fails<System.Security.Cryptography.CryptographicException>(()=>store.Read(),"corruption not overwritten");
            Check(File.ReadAllText(Path.Combine(dir,"state.dat"))=="invalid","corrupt state retained");
        }finally{if(Directory.Exists(dir))Directory.Delete(dir,true);}
    }
    sealed class TestProtector:IProtector
    {
        public bool Fail;
        public byte[] Protect(byte[] bytes){if(Fail)throw new IOException("simulated protection error");return Encoding.ASCII.GetBytes(Convert.ToBase64String(bytes));}
        public byte[] Unprotect(byte[] bytes){try{return Convert.FromBase64String(Encoding.ASCII.GetString(bytes));}catch(FormatException){throw new System.Security.Cryptography.CryptographicException();}}
    }
    sealed record Captured(Uri Uri,string Body,string? Authorization);
    sealed class FakeHttp(Func<Captured,CancellationToken,byte[]> response):IBridgeHttp
    {
        public async Task<byte[]> Send(HttpRequestMessage request,CancellationToken cancellationToken=default)
        {
            cancellationToken.ThrowIfCancellationRequested();
            return response(new Captured(request.RequestUri!,request.Content==null?"":await request.Content.ReadAsStringAsync(cancellationToken),request.Headers.Authorization?.ToString()),cancellationToken);
        }
    }
}
