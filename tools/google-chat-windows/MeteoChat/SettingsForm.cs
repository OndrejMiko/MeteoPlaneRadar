using MeteoChat.Core;

namespace MeteoChat.Windows;

internal sealed class SettingsForm : Form
{
    private readonly TrayApplication app;
    private readonly Label status = new() { AutoSize = true, MaximumSize = new Size(630,0) };
    private readonly Label error = new() { AutoSize = true, ForeColor = Color.Firebrick, MaximumSize = new Size(630,0) };
    private readonly Label device = new() { AutoSize = true };
    private readonly Label google = new() { AutoSize = true };
    private readonly TextBox address = new() { Text = "meteoplaneradar.local", Width = 350, AccessibleName = "Adresa displeja" };
    private readonly TextBox code = new() { Width = 110, MaxLength = 12, AccessibleName = "Šesťmiestny párovací kód" };
    private readonly CheckedListBox spaces = new() { CheckOnClick = true, Dock = DockStyle.Fill, IntegralHeight = false, AccessibleName = "Chaty na doručovanie" };
    private readonly Button start = Button("Zapnúť mostík");
    private readonly Button pause = Button("Pozastaviť");
    private readonly Button cancel = Button("Zrušiť operáciu");
    private readonly Button signIn = Button("Prihlásiť cez Google");
    private readonly Button refresh = Button("Obnoviť chaty");
    private readonly Button signOut = Button("Odhlásiť Google");
    private readonly List<Control> editable = [];
    private bool loading, allowExit;
    private string? knownPair;
    public SettingsForm(TrayApplication app)
    {
        this.app = app;
        Text = "Meteo Chat · Windows v" + TrayApplication.Version;
        ClientSize = new Size(720,790); MinimumSize = new Size(740,650);
        AutoScaleMode = AutoScaleMode.Dpi; Font = new Font("Segoe UI",10); StartPosition = FormStartPosition.CenterScreen;
        Icon = TrayIcon.Create();
        var root = new TableLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(20), ColumnCount = 1, RowCount = 5 };
        root.RowStyles.Add(new RowStyle(SizeType.AutoSize)); root.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        root.RowStyles.Add(new RowStyle(SizeType.Percent,100)); root.RowStyles.Add(new RowStyle(SizeType.AutoSize)); root.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        root.Controls.Add(TextLabel("Meteo Chat",18,true),0,0);
        var summary = Stack(); summary.Controls.Add(status); summary.Controls.Add(error); root.Controls.Add(summary,0,1);
        var tabs = new TabControl { Dock = DockStyle.Fill };
        var connection = new TabPage("Displej"); var googleTab = new TabPage("Google Chat"); var advanced = new TabPage("Pokročilé");
        tabs.TabPages.AddRange([connection,googleTab,advanced]); root.Controls.Add(tabs,0,2);
        var pairing = Stack(); connection.Controls.Add(pairing); pairing.Dock = DockStyle.Fill; pairing.AutoScroll = true;
        pairing.Controls.Add(TextLabel("1. Spárujte displej",13,true));
        pairing.Controls.Add(Hint("Vo webovom UI displeja otvorte Notifikácie → Zobraziť párovací kód. Zadajte jeho adresu a šesťmiestny kód (platí 2 minúty)."));
        pairing.Controls.Add(TextLabel("Adresa displeja")); pairing.Controls.Add(address);
        pairing.Controls.Add(TextLabel("Párovací kód")); pairing.Controls.Add(code);
        var pair = Button("Spárovať kódom"); var test = Button("Overiť displej / poslať test");
        pair.Click += async (_,_) => { await app.Pair(address.Text,code.Text); if (app.Error == null) code.Clear(); };
        test.Click += async (_,_) => await app.Perform(async ct => { await app.Service.TestDisplay(ct); });
        pairing.Controls.Add(Row(pair,test)); pairing.Controls.Add(device);
        pairing.Controls.Add(Hint("Pri problémoch s názvom .local použite IP adresu z obrazovky Nastavenia. Displej aj počítač musia byť v rovnakej domácej sieti."));
        var launch = new CheckBox { Text = "Spúšťať po prihlásení do Windows", AutoSize = true, Checked = Startup.Enabled };
        launch.CheckedChanged += (_,_) => {
            if (loading) return;
            try { Startup.Set(launch.Checked); }
            catch { MessageBox.Show("Automatické spúšťanie sa nepodarilo nastaviť.","Meteo Chat"); loading=true; launch.Checked=Startup.Enabled; loading=false; }
        };
        pairing.Controls.Add(launch); editable.AddRange([address,code,pair,test,launch]);
        var googleRoot = new TableLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(12), ColumnCount = 1, RowCount = 4 };
        googleRoot.RowStyles.Add(new RowStyle(SizeType.AutoSize)); googleRoot.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        googleRoot.RowStyles.Add(new RowStyle(SizeType.Percent,100)); googleRoot.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        googleRoot.Controls.Add(google,0,0); googleRoot.Controls.Add(Row(signIn,refresh,signOut),0,1);
        googleRoot.Controls.Add(spaces,0,2);
        googleRoot.Controls.Add(Hint("2. Prihláste Google v prehliadači. 3. Zaškrtnite chaty a zapnite mostík. Nový výber začína od aktuálneho času; stará história sa neposiela. Počítač musí zostať zapnutý. Pred úpravou výberu mostík pozastavte."),0,3);
        googleTab.Controls.Add(googleRoot);
        spaces.DisplayMember = nameof(Space.Title);
        spaces.ItemCheck += (_,e) => {
            if (loading || app.Busy || app.Running) return;
            var space = (Space)spaces.Items[e.Index];
            // Persist before updating the checkbox; a failed save cannot appear selected.
            app.SaveSelection(space,e.NewValue == CheckState.Checked);
            e.NewValue = app.Service.State.Spaces.Any(s => s.Name == space.Name) ? CheckState.Checked : CheckState.Unchecked;
        };
        signIn.Click += async (_,_) => await app.SignIn();
        refresh.Click += async (_,_) => await app.ReloadSpaces();
        signOut.Click += async (_,_) => await app.SignOut();
        var extra = Stack(); extra.Dock = DockStyle.Fill; advanced.Controls.Add(extra);
        extra.Controls.Add(Hint("Hotová zostava môže mať Google prihlásenie pripravené. Pri vlastnom zostavení importujte raz registráciu typu Desktop app; heslo ku Googlu sa do aplikácie nezadáva."));
        var client = Button("Importovať Google OAuth registráciu…"); var importPair = Button("Importovať staršie párovanie…");
        client.Click += async (_,_) => await app.ImportClient(); importPair.Click += async (_,_) => await app.ImportPair();
        extra.Controls.Add(client); extra.Controls.Add(importPair);
        extra.Controls.Add(Hint("Tokeny, párovanie, chaty a stav doručovania sa ukladajú cez Windows DPAPI pre aktuálneho používateľa do %LOCALAPPDATA%\\MeteoChat. Pri odhlásení sa Google tokeny odstránia; párovanie displeja zostane."));
        editable.AddRange([client,importPair]);
        start.Click += (_,_) => app.Start(); pause.Click += async (_,_) => await app.Pause(); cancel.Click += (_,_) => app.CancelOperation();
        var actions = Row(start,pause,cancel); root.Controls.Add(actions,0,3);
        root.Controls.Add(Hint("Zatvorenie okna ponechá aplikáciu pri hodinách. Úplné ukončenie: ikona → Ukončiť. Pozastavenie neodhlási účet."),0,4);
        Controls.Add(root);
        FormClosing += (_,e) => { if (!allowExit && e.CloseReason == CloseReason.UserClosing) { e.Cancel=true; Hide(); } };
        RefreshState();
    }
    internal void AllowExit() => allowExit = true;
    internal void RefreshState()
    {
        if (IsDisposed) return;
        loading = true;
        try
        {
            var state = app.Service.State; var locked = app.Busy || app.Running;
            status.Text = app.Status + (app.LastContact == null ? "" : $"\nPosledný kontakt: {app.LastContact:HH:mm:ss} · doručených: {app.Delivered}");
            error.Text = app.Error ?? ""; error.Visible = app.Error != null;
            device.Text = state.Pair == null ? "Displej zatiaľ nie je spárovaný." : "Spárovaný displej: " + state.Pair.Host;
            if (knownPair != state.Pair?.Device)
            {
                knownPair = state.Pair?.Device;
                if (knownPair != null) address.Text = new Uri(knownPair).GetLeftPart(UriPartial.Authority);
            }
            google.Text = state.Token != null ? "Google je prihlásený. Vyberte chaty na doručovanie." : state.Client != null
                ? "Prihlásenie cez prehliadač je pripravené." : "Google registrácia nie je pripravená — pozrite Pokročilé.";
            signIn.Text = state.Token == null ? "Prihlásiť cez Google" : "Prihlásiť znova";
            foreach (var control in editable) control.Enabled = !locked;
            signIn.Enabled = !locked; refresh.Enabled = signOut.Enabled = !locked && state.Token != null;
            spaces.Enabled = !locked && state.Token != null;
            var entries = app.AvailableSpaces;
            // Avoid rebuilding inside ItemCheck; selection itself is applied by its event.
            if (spaces.Items.Count != entries.Count || !spaces.Items.Cast<Space>().SequenceEqual(entries))
            { spaces.Items.Clear(); foreach (var space in entries) spaces.Items.Add(space,state.Spaces.Any(s=>s.Name==space.Name)); }
            start.Enabled = !app.Busy && !app.Running && app.Service.Configured;
            pause.Enabled = !app.Busy && app.Running; cancel.Enabled = app.Busy;
        }
        finally { loading = false; }
    }
    private static Button Button(string text) => new() { Text=text, AutoSize=true, MinimumSize=new Size(100,38), Margin=new Padding(0,5,10,5), Padding=new Padding(6) };
    private static Label TextLabel(string text, float size=10, bool bold=false) => new() {
        Text=text, AutoSize=true, Font=new Font("Segoe UI",size,bold?FontStyle.Bold:FontStyle.Regular), Margin=new Padding(0,8,0,6)
    };
    private static Label Hint(string text) => new() { Text=text,AutoSize=true,MaximumSize=new Size(600,0),ForeColor=Color.DimGray,Margin=new Padding(0,10,0,16) };
    private static FlowLayoutPanel Stack() => new() { AutoSize=true,FlowDirection=FlowDirection.TopDown,WrapContents=false,Padding=new Padding(12),Dock=DockStyle.Top };
    private static FlowLayoutPanel Row(params Control[] controls)
    {
        var row = new FlowLayoutPanel { AutoSize=true,Dock=DockStyle.Top,WrapContents=true,Margin=new Padding(0) };
        row.Controls.AddRange(controls); return row;
    }
    protected override void Dispose(bool disposing)
    {
        if (disposing) Icon?.Dispose();
        base.Dispose(disposing);
    }
}
