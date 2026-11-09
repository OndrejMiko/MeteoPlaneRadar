using System.Reflection;
using System.Security.Principal;
using MeteoChat.Core;

namespace MeteoChat.Windows;

internal static class Program
{
    [STAThread]
    private static int Main(string[] args)
    {
        ApplicationConfiguration.Initialize();
        if (args.Contains("--self-test")) return SelfTest();
        var identity = WindowsIdentity.GetCurrent().User?.Value ?? Environment.UserName;
        using var single = new Mutex(true, @"Local\MeteoChat-" + identity, out var created);
        if (!created) { MessageBox.Show("Meteo Chat už beží. Otvorte ho cez ikonu pri hodinách.", "Meteo Chat"); return 0; }
        using var http = new NativeHttp();
        try
        {
            var folder = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "MeteoChat");
            var store = new StateStore(folder, new UserProtector());
            // Never overwrite an unreadable vault with empty credentials.
            var service = new ChatService(store.Read(), http, store.Write);
            using var bundled = Assembly.GetExecutingAssembly().GetManifestResourceStream("MeteoChat.GoogleOAuth.json");
            if (bundled != null)
            {
                using var bytes = new MemoryStream(); bundled.CopyTo(bytes);
                service.ApplyBundledClient(OAuthClient.Decode(bytes.ToArray()));
            }
            using var app = new TrayApplication(service, http, args.Contains("--background"));
            Application.Run(app);
            return 0;
        }
        catch
        {
            MessageBox.Show("Nastavenia sa nepodarilo bezpečne načítať. Spustite aplikáciu pod pôvodným Windows účtom a skúste to znova. Uložené dáta sa neprepísali.",
                "Meteo Chat", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 1;
        }
    }
    private static int SelfTest()
    {
        // Run on Windows after building: checks DPAPI, persistence and UI creation
        // without opening Google, touching saved user state or changing startup.
        var dir = Path.Combine(Path.GetTempPath(), "meteo-chat-check-" + Guid.NewGuid().ToString("N"));
        try
        {
            var protector = new UserProtector();
            var original = System.Text.Encoding.UTF8.GetBytes("synthetic-secret");
            var encrypted = protector.Protect(original);
            if (encrypted.SequenceEqual(original) || !protector.Unprotect(encrypted).SequenceEqual(original)) return 2;
            var store = new StateStore(dir, protector);
            store.Write(new BridgeState { Pair = new Pairing("http://127.0.0.1/api/notifications", new string('a',64)) });
            if (store.Read().Pair == null) return 3;
            using var http = new NativeHttp();
            using var app = new TrayApplication(new ChatService(new BridgeState(), http, _ => {}), http, true, smokeTest: true);
            using var form = app.CreateSettingsForTest();
            form.Show(); Application.DoEvents(); form.Hide();
            return 0;
        }
        catch { return 4; }
        finally { if (Directory.Exists(dir)) Directory.Delete(dir, true); }
    }
}
