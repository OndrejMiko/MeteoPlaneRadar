# Meteo Chat pre Windows

Samostatný most **Google Chat → MeteoPlaneRadar** s ikonou pri hodinách.
Zatvorenie okna ho ponechá bežať; menu ikony ponúka nastavenia, zapnutie,
pozastavenie, skúšobnú notifikáciu a úplné ukončenie. Používa rovnaký
firmvérový protokol a párovací kód ako [macOS aplikácia](../google-chat-macos/README.md).

## Prvé spustenie

1. Rozbaľte ZIP do trvalého priečinka a spustite **MeteoChat.exe**. Hotový
   self-contained balík nepotrebuje Python, samostatnú inštaláciu .NET ani práva správcu.
   Zostavy sú pre **Windows 11 x64** a **Windows 11 ARM64**; vyberte svoju architektúru.
2. Vo webovom UI displeja otvorte **Notifikácie → Zobraziť párovací kód**.
   Ak máte heslo správcu, najprv ho vyplňte na karte Systém. V aplikácii zadajte
   adresu displeja (napr. `meteoplaneradar.local` alebo jeho IP), šesťmiestny kód
   a kliknite **Spárovať kódom**. Kód platí dve minúty a funguje raz.
3. **Overiť displej / poslať test** pošle správu bez potreby Google účtu.
4. Na karte **Google Chat** kliknite **Prihlásiť cez Google**. Otvorí sa váš
   bežný prehliadač; vyberte účet a povoľte čítanie Chatu. Heslo nezadávate
   do aplikácie. Po návrate môžete kartu prehliadača zavrieť.
5. Zaškrtnite chaty a kliknite **Zapnúť mostík**. Odber nového chatu začína
   od aktuálneho času, stará história sa neodosiela. Mostík kontroluje nové
   správy každých 15 sekúnd; pri výpadku skúša znova s rastúcou prestávkou.

Prihlásenie tlačidlom potrebuje zostavu s pripravenou Google OAuth registráciou
**Desktop app**. Autor ju môže vložiť pri zostavení — používateľ potom žiadny JSON
nerieši. Verejný zdrojový kód cudziu registráciu neobsahuje. Zostava bez registrácie
ponúkne na karte **Pokročilé** jej jednorazový import s jasným vysvetlením;
tlačidlo prihlásenia nie je bez vysvetlenia neaktívne.

## Bežné používanie

- **Zapnúť / Pozastaviť** riadi doručovanie; pozastavenie zachová účet,
  párovanie, chaty a rozpracované doručenie.
- Pred zmenou účtu, displeja alebo výberu chatov mostík pozastavte.
- **Zrušiť operáciu** zruší prebiehajúce prihlásenie alebo sieťovú operáciu.
- **Spúšťať po prihlásení do Windows** je voliteľné. Zapíše iba vlastnú
  položku `MeteoChat` do používateľského registra `HKCU\...\Run`.
  Aplikáciu pred zapnutím tejto voľby premiestnite do trvalého priečinka.
  Po presune ju vypnite a znovu zapnite; voľba sa sama neaktivuje.
- Krížik okna ho skryje. **Ikona → Ukončiť** ukončí proces. Ak bol mostík zapnutý,
  pri ďalšom spustení pokračuje. Pozastavenie si aplikácia pamätá tiež.
- **Odhlásiť Google** odstráni lokálne Google tokeny a výber chatov,
  párovanie displeja ponechá. Prístup aplikácii môžete samostatne odobrať v Google účte.

Počítač musí bežať a mať prístup na internet aj do siete displeja. Pri vypnutom
počítači alebo režime spánku sa nič nekontroluje. Vlastné a zmazané správy
sa neodosielajú. Plná fronta displeja nepresunie kurzor za nedoručenú správu;
prijaté správy sa priebežne ukladajú, vrátane rozpracovanej stránky Google API.

Tokeny, párovací kľúč, výber chatov a doručovací stav sú v
`%LOCALAPPDATA%\MeteoChat\state.dat`, chránené **Windows DPAPI pre aktuálneho
používateľa**. Súbor neprenášajte ako konfiguráciu na iný účet alebo počítač;
tam displej spárujte a Google prihláste znova. Poškodené alebo nečitateľné
úložisko sa automaticky neprepíše prázdnymi údajmi. Aplikácia neloguje texty
správ, tokeny, kľúče ani návratové kódy OAuth.

Starší párovací súbor z displeja možno importovať v **Pokročilé**. mDNS názov
`.local` nemusí byť v každej sieti dostupný; použite IP z obrazovky Nastavenia.

## Zostavenie vo Windows

Potrebujete **.NET 10 SDK**. Spustite PowerShell v tomto priečinku:

```powershell
# Zdrojová zostava bez vloženej registrácie (jednorazový import v Pokročilé).
.\build.ps1

# Zostava s pripraveným prihlásením; registrácia zostane mimo Gitu.
.\build.ps1 -OAuthConfig 'C:\private\google-desktop-app.json' -RequireOAuth

# Natívna ARM64 zostava.
.\build.ps1 -Runtime win-arm64 -OAuthConfig 'C:\private\google-desktop-app.json' -RequireOAuth
```

Skript spustí kontroly jadra, zostaví self-contained aplikáciu, vo Windows
spustí test DPAPI/úložiska/vytvorenia okna a pri zhodnej architektúre aj test
výsledného EXE. Výsledok je `dist/Meteo-Chat-<verzia>-win-x64.zip` alebo `win-arm64.zip`
a súbor s SHA-256. Pri krížovom zostavení ARM64 na x64 je potrebné výsledný
ARM64 program ešte spustiť na ARM64 Windows. Verzia sa číta z
`MeteoPlaneRadar/Version.h`, nekopíruje sa ručne do projektov.

`config/google-oauth.json` sa použije ako voliteľná lokálna konfigurácia;
`config/google-oauth.example.json` je iba ukážka. Pred vložením do EXE sa
registrácia validuje a exportujú sa len `client_id` a `client_secret`, bez
ľubovoľných URL, Google tokenov alebo párovania. Konfigurácia sa vkladá do
prostriedkov programu a po zostavení nezostáva vedľa EXE ako súbor.
Nevkladajte tokeny používateľov do distribúcie. Lokálna zostava nie je podpísaná
certifikátom Authenticode; Windows pri neznámom vydavateľovi môže zobraziť upozornenie.

### Google registrácia

Používa rovnaký typ **Desktop app** ako Mac. Existujúci Desktop OAuth klient
môže obsluhovať aj Windows; netreba nastavovať webový redirect alebo vlastný
server. Prihlasovanie používa systémový prehliadač, PKCE/S256, náhodný `state`
a krátkodobý návrat na `127.0.0.1` s náhodným portom. Port sa neotvára do siete.
Prístupy sú iba `openid`, `chat.spaces.readonly` a `chat.messages.readonly`.

Nastavenie Google Chat API, publika a testovacích účtov je v
[spoločnom návode](../google-chat-macos/OAUTH_SETUP.sk.md). Z neho použite kroky
Google Cloud/OAuth; build aplikácie vykonajte vyššie uvedeným Windows skriptom.
Prístup k Chat API závisí od účtu a pravidiel organizácie rovnako ako na Macu.

## Overenie pre vývojárov

```powershell
dotnet run --project Checks/Checks.csproj -c Release
dotnet build MeteoChat/MeteoChat.csproj -c Release
# Na Windows; bez skutočného displeja/účtu, bez zmien prihlasovacích položiek.
dotnet run --project MeteoChat/MeteoChat.csproj -c Release -- --self-test
```

Testy jadra bežia aj na macOS/Linux: párovanie a expirácia, PKCE a skutočný
lokálny callback, zrušenie, obnovenie tokenu, zmena účtu, stránkovanie s rovnakými
časmi, reštart po plnej fronte, limity UTF-8/JSON, odmietanie presmerovaní a ochrana
uložených údajov pri chybe zápisu. Natívny DPAPI a UI test vyžaduje Windows.
Krížové zostavenie EXE samo osebe neoveruje jeho vzhľad a správanie na Windows.

Oficiálne podklady: [Google Desktop OAuth](https://developers.google.com/identity/protocols/oauth2/native-app),
[Microsoft NotifyIcon](https://learn.microsoft.com/dotnet/api/system.windows.forms.notifyicon),
[Windows DPAPI](https://learn.microsoft.com/windows/win32/api/dpapi/nf-dpapi-cryptprotectdata).
