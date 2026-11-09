using System.Diagnostics;
using System.Reflection;
using MeteoChat.Core;

namespace MeteoChat.Windows;

internal sealed class TrayApplication : ApplicationContext
{
    internal readonly ChatService Service;
    private readonly IBridgeHttp http;
    private readonly NotifyIcon tray;
    private readonly ToolStripMenuItem toggle = new("Zapnúť mostík");
    private readonly ToolStripMenuItem test = new("Skúšobná notifikácia");
    private readonly ToolStripMenuItem statusItem = new("Mostík je pozastavený") { Enabled = false };
    private readonly Icon icon = TrayIcon.Create();
    private SettingsForm? form;
    private CancellationTokenSource? workerCancellation, operationCancellation;
    private Task? worker;
    private bool exiting, disposed;
    internal bool Busy { get; private set; }
    internal bool Running => workerCancellation != null;
    internal string Status { get; private set; } = "Mostík je pozastavený";
    internal string? Error { get; private set; }
    internal List<Space> AvailableSpaces { get; private set; } = [];
    internal int Delivered { get; private set; }
    internal DateTimeOffset? LastContact { get; private set; }
    internal static string Version => typeof(TrayApplication).Assembly.GetCustomAttribute<AssemblyInformationalVersionAttribute>()?.InformationalVersion.Split('+')[0] ?? "";
    internal TrayApplication(ChatService service, IBridgeHttp http, bool background, bool smokeTest = false)
    {
        Service = service; this.http = http; AvailableSpaces = [.. service.State.Spaces];
        var menu = new ContextMenuStrip();
        menu.Items.Add(new ToolStripMenuItem("Meteo Chat · v" + Version) { Enabled = false });
        menu.Items.Add(statusItem); menu.Items.Add(new ToolStripSeparator());
        menu.Items.Add(toggle); menu.Items.Add("Nastavenia…", null, (_,_) => ShowSettings());
        menu.Items.Add(test); menu.Items.Add(new ToolStripSeparator());
        menu.Items.Add("Ukončiť", null, async (_,_) => await Quit());
        toggle.Click += async (_,_) => { if (Running) await Pause(); else Start(); };
        test.Click += async (_,_) => await Perform(async ct => { await service.TestDisplay(ct); Status = "Skúšobná notifikácia bola odoslaná."; });
        tray = new NotifyIcon { Icon = icon, Text = "Meteo Chat", ContextMenuStrip = menu, Visible = !smokeTest };
        tray.DoubleClick += (_,_) => ShowSettings();
        Sync();
        if (!smokeTest)
        {
            EventHandler? launch = null;
            launch = (_,_) => {
                Application.Idle -= launch;
                if (disposed || exiting) return;
                if (!background || !service.Configured) ShowSettings();
                if (service.State.Enabled && service.Configured) Start();
            };
            Application.Idle += launch;
        }
    }
    internal Form CreateSettingsForTest() => new SettingsForm(this);
    internal void ShowSettings()
    {
        if (exiting) return;
        if (form == null || form.IsDisposed) form = new SettingsForm(this);
        form.RefreshState(); form.Show(); form.WindowState = FormWindowState.Normal; form.Activate();
    }
    internal void Sync()
    {
        if (disposed) return;
        toggle.Text = Running ? "Pozastaviť mostík" : "Zapnúť mostík";
        toggle.Enabled = !Busy && (Running || Service.Configured);
        test.Enabled = !Busy && !Running && Service.State.Pair != null;
        statusItem.Text = Running ? "Mostík je zapnutý" : "Mostík je pozastavený";
        tray.Text = Running ? "Meteo Chat — zapnutý" : "Meteo Chat — pozastavený";
        form?.RefreshState();
    }
    internal async Task Perform(Func<CancellationToken,Task> action)
    {
        if (Busy || Running || exiting) return;
        Busy = true; Error = null; operationCancellation = new(); Sync();
        try { await action(operationCancellation.Token); }
        catch (Exception e) { Error = Errors.Describe(e); }
        finally { operationCancellation.Dispose(); operationCancellation = null; Busy = false; Sync(); }
    }
    internal void CancelOperation() => operationCancellation?.Cancel();
    internal void SaveSelection(Space space, bool selected)
    {
        if (Busy || Running) return;
        try { Service.Change(next => next.Select(space, selected)); Error = null; }
        catch (Exception e) { Error = Errors.Describe(e); }
        Sync();
    }
    internal async Task Pair(string address, string code) => await Perform(async ct => {
        var pair = await Pairing.Claim(http, address, code, ct);
        Service.Change(next => next.Pair = pair); Status = "Displej je spárovaný. Overte spojenie skúšobnou notifikáciou.";
    });
    internal async Task ImportClient() => await Perform(_ => {
        var data = PickJson("Google OAuth — Desktop app");
        if (data != null)
        {
            Service.InstallClient(OAuthClient.Decode(data)); AvailableSpaces = [.. Service.State.Spaces];
            Status = "Google prihlásenie je pripravené. Kliknite Prihlásiť cez Google.";
        }
        return Task.CompletedTask;
    });
    internal async Task ImportPair() => await Perform(_ => {
        var data = PickJson("Párovanie displeja");
        if (data != null)
        {
            var pair = Core.Json.Read<Pairing>(data); pair.Validate(); Service.Change(next => next.Pair = pair);
            Status = "Displej je spárovaný.";
        }
        return Task.CompletedTask;
    });
    private static byte[]? PickJson(string title)
    {
        using var picker = new OpenFileDialog { Title = title, Filter = "JSON súbor (*.json)|*.json", CheckFileExists = true };
        if (picker.ShowDialog() != DialogResult.OK) return null;
        if (new FileInfo(picker.FileName).Length > 1024 * 1024) throw new BridgeException("Vybraný súbor je príliš veľký.");
        return File.ReadAllBytes(picker.FileName);
    }
    internal async Task SignIn()
    {
        if (Service.State.Client == null)
        {
            MessageBox.Show("Táto zostava nemá pripravenú Google registráciu. V časti Pokročilé importujte raz OAuth klienta typu Desktop app alebo použite zostavu s pripraveným prihlásením.", "Meteo Chat");
            return;
        }
        await Perform(async ct => {
            Status = "Dokončite prihlásenie v otvorenom prehliadači…"; Sync();
            var login = new GoogleOAuth(http, uri => {
                Process.Start(new ProcessStartInfo(uri.AbsoluteUri) { UseShellExecute = true }); return true;
            });
            var token = await login.SignIn(Service.State.Client!, ct);
            await Service.SignIn(token, ct);
            AvailableSpaces = [.. Service.State.Spaces]; // discard the old account before discovery
            AvailableSpaces = await Service.ListSpaces(ct);
            Status = AvailableSpaces.Count == 0 ? "Nenašli sa chaty. Skontrolujte členstvo a povolenia Google Chat API." : "Google je prihlásený. Vyberte chaty na doručovanie.";
        });
    }
    internal async Task ReloadSpaces() => await Perform(async ct => {
        AvailableSpaces = await Service.ListSpaces(ct); Status = "Zoznam chatov bol obnovený.";
    });
    internal async Task SignOut() => await Perform(_ => {
        Service.SignOut(); AvailableSpaces.Clear(); Status = "Google účet je odhlásený. Párovanie displeja zostalo uložené.";
        return Task.CompletedTask;
    });
    internal void Start()
    {
        if (Busy || Running || exiting || !Service.Configured) return;
        try { Service.Change(next => next.Enabled = true); }
        catch (Exception e) { Error = Errors.Describe(e); Sync(); return; }
        workerCancellation = new(); Status = "Pripájam displej a Google Chat…"; Error = null;
        // Yield lets worker be assigned before an immediately failing operation.
        worker = Run(workerCancellation.Token); Sync();
    }
    private async Task Run(CancellationToken ct)
    {
        await Task.Yield(); var failures = 0;
        while (!ct.IsCancellationRequested)
        {
            try
            {
                if (await Service.Device(null, ct))
                {
                    foreach (var space in Service.State.Spaces.ToArray()) Delivered += await Service.Poll(space, ct);
                    Status = "Pripojené · kontrola každých 15 sekúnd";
                }
                else Status = "Notifikácie sú vypnuté na displeji. Čakám…";
                LastContact = DateTimeOffset.Now; Error = null; failures = 0;
            }
            catch (Exception e)
            {
                if (ct.IsCancellationRequested) break;
                failures = Math.Min(failures + 1, 5); Error = Errors.Describe(e); Status = "Čakám na opätovné spojenie…";
            }
            Sync();
            try { await Task.Delay(TimeSpan.FromSeconds(Math.Min(300, 15 * (1 << failures))), ct); }
            catch (OperationCanceledException) { break; }
        }
    }
    internal async Task Pause()
    {
        if (!Running || Busy) return;
        Busy = true; Sync();
        try {
            var cancellation = workerCancellation!; cancellation.Cancel();
            if (worker != null) await worker;
            worker = null; cancellation.Dispose(); workerCancellation = null;
            Service.Change(next => next.Enabled = false); Error = null;
            Status = "Mostík je pozastavený";
        }
        catch (Exception e) { Error = Errors.Describe(e); }
        finally { Status = "Mostík je pozastavený"; Busy = false; Sync(); }
    }
    private async Task Quit()
    {
        if (exiting) return;
        exiting = true; operationCancellation?.Cancel();
        // Preserve enabled preference on app exit, like the macOS bridge.
        workerCancellation?.Cancel();
        if (worker != null) await worker;
        tray.Visible = false; form?.AllowExit(); form?.Close(); ExitThread();
    }
    protected override void Dispose(bool disposing)
    {
        if (disposing && !disposed)
        {
            disposed = true; workerCancellation?.Cancel(); operationCancellation?.Cancel();
            form?.AllowExit(); form?.Dispose(); tray.Dispose(); icon.Dispose(); workerCancellation?.Dispose();
        }
        base.Dispose(disposing);
    }
}
