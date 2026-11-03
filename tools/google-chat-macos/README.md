# Meteo Chat pre macOS

Jednoduchá natívna aplikácia pre **macOS 13 a novší**, Apple Silicon aj Intel.
Žije v hornej lište, bez ikony v Docku. **Nepotrebuje Python, Homebrew ani
ďalšie balíčky.** Posiela nové správy z vybraných chatov na MeteoPlaneRadar.

## Prvé spustenie

1. Rozbaľte `Meteo-Chat-<verzia>-macOS.zip` a presuňte **Meteo Chat.app** do
   **Applications / Aplikácie**. Spustite ju; zobrazí sa ikona papierového
   lietadla v hornej lište a okno nastavení.
2. Aktualizujte displej na firmware **0.7.13 alebo novší**. Vo webovom UI
   otvorte **Notifikácie → Vygenerovať párovací kód**. Ak má displej heslo
   správcu, najprv ho zadajte na karte Systém. V aplikácii vyplňte adresu
   displeja (predvolené `meteoplaneradar.local`, prípadne IP z webového UI),
   zadajte šesťmiestny kód a kliknite **Spárovať kódom**. Žiadny párovací
   súbor netreba. **Overiť displej** doručí skúšobnú správu.
3. V zostave s pripraveným Google prihlásením kliknite **Prihlásiť cez Google**.
   Otvorí sa predvolený prehliadač; vyberte účet a povoľte čítanie Chatu.
4. Po návrate môžete kartu zatvoriť. Heslo do Meteo Chatu nezadávate.
   Ak zostavujete aplikáciu sami, pripravte raz registračné údaje podľa
   [kompletného postupu pre autora](OAUTH_SETUP.sk.md). Používateľ hotovej
   zostavy už neimportuje žiadny OAuth súbor.
5. Vyberte chaty a kliknite **Zapnúť mostík**. Prvý výber začína od aktuálneho
   času, stará história sa neposiela. Načítanie je každých 15 sekúnd.

Párovací kód platí 2 minúty, funguje raz a po piatich chybných pokusoch sa
zablokuje. Vygenerovanie kódu ponechá existujúce spojenie funkčné; nový
kľúč sa vytvorí až pri úspešnom spárovaní a nahradí pôvodný. Kód možno
zrušiť tlačidlom na webe. Starší firmware podporuje pôvodný import súboru
pod **Pokročilé: import párovacieho súboru**.

Aplikácia je pri lokálnom zostavení podpísaná **ad-hoc**, nie Apple Developer
ID ani notarizovaná. Ak ju macOS pri prvom otvorení zablokuje, použite jeho
štandardnú možnosť **Otvoriť aj tak** v Systémových nastaveniach → Súkromie
a bezpečnosť. Nevypínajte Gatekeeper. Na verejnú distribúciu treba podpis
Developer ID a notarizáciu účtom autora distribúcie.

## Claude · zostávajúce limity na hodinách

Vyžaduje firmware a aplikáciu **0.7.17 alebo novšie**. Google Chat môže zostať
odhlásený, Claude funguje samostatne.

1. Aktualizujte firmware a Mac aplikáciu; existujúce párovanie zostáva platné.
2. V Mac aplikácii pozastavte mostík a zapnite **Posielať zostávajúce limity
   Claude na hodiny**. Aplikácia pridá lokálny statusLine pomocník do
   `~/.claude/settings.json` (alebo `CLAUDE_CONFIG_DIR`, ak je nastavený pri
   spustení aplikácie). Existujúci príkaz ostane funkčný. Pomocník sa uloží
   do Application Support, takže presun `.app` nepoškodí jeho cestu.
3. Otvorte **terminálový Claude Code**, aktualizovaný na verziu s oficiálnymi
   `rate_limits`, prihlásený cez svoje predplatné, a odošlite správu.
   Projektové nastavenie `statusLine` môže mať prednosť pred používateľským;
   v takom projekte ho odstráňte alebo zjednoťte. Integrácia nečíta údaje
   z VS Code rozšírenia ani z webového Claude Code priamo.
4. Vo webovom UI displeja otvorte **Vzhľad → Hodiny → Zobraziť zostávajúce
   limity Claude na hodinách**. Nastavenie sa uloží okamžite.
5. V Mac aplikácii kliknite **Zapnúť mostík**. Do 15 sekúnd po novom údaji
   sa zobrazí zostávajúce percento a čas resetu pre 5h a týždeň.

Sú to percentá limitov predplatného, nie súčet tokenov konverzácií. Zdieľané
limity zahŕňajú aj web prihlásený tým istým účtom, ale samotné používanie webu
neobnovuje lokálny zdroj. Nové údaje musia prísť z Claude Code. Po 15 minútach
hodiny označia posledný stav `~` a sivou farbou. Po známom čase resetu alebo
pri chýbajúcom okne zobrazia `--`; automaticky nepredpokladajú 100 %.
Reštart displeja vymaže snímok a ďalší prenos z mostíka ho obnoví.

Na vypnutie pozastavte mostík a vypnite prepínač Claude; obnoví sa pôvodný
statusLine. Ak ste ho medzitým sami zmenili, aplikácia ho neprepíše a upozorní
na zálohu `~/Library/Application Support/MeteoChat/claude-install.json`.
Na skrytie údajov na displeji vypnite aj prepínač v jeho webovom UI.

Pomocník nečíta Claude OAuth tokeny ani konverzácie a nevolá Anthropic API.
Ukladá iba normalizované limity, čas pozorovania a otlačok zdroja do
`~/Library/Application Support/MeteoChat/claude-usage.json` s právami 600.
Opakované spustenie s rovnakým vstupom neposúva čas pozorovania.
Dokumentácia zdroja: [Claude Code statusLine](https://code.claude.com/docs/en/statusline).

## Google Cloud – registrácia na strane autora

Presný postup od Google Cloud cez build po test skutočnej správy je v
[OAUTH_SETUP.sk.md](OAUTH_SETUP.sk.md). Na zajtrajšiu integráciu začnite tam.

Kód vyššie páruje Mac s displejom, nenahrádza Google OAuth klienta.
Prihlásenie cez prehliadač potrebuje identitu aplikácie u Googlu. Repozitár
neobsahuje cudzí OAuth projekt ani univerzálne prihlasovacie údaje.

1. V [Google Cloud Console](https://console.cloud.google.com/) vyberte alebo
   vytvorte projekt a povoľte **Google Chat API**.
2. V Google Auth Platform nastavte údaje aplikácie a publikum. Pri externom
   testovacom projekte pridajte vlastný účet medzi **Test users**.
3. Povoľte prístupy `openid`, `chat.spaces.readonly` a
   `chat.messages.readonly`. Správca organizácie ich môže blokovať.
4. Vytvorte OAuth client ID typu **Desktop app**, stiahnite JSON a použite
   `configure-oauth.sh` a strict build podľa úplného návodu. Aplikácia potom
   použije zabudované prihlásenie. Manuálny import je iba v Pokročilom nastavení
   Googlu; webový OAuth klient ani súbor servisného účtu nefunguje.

Google dokumentuje používateľské čítanie správ pre Workspace účty; dostupnosť
osobného Gmail účtu tu nie je overená. Externý OAuth projekt v testovacom
režime môže mať krátko platné refresh tokeny (typicky 7 dní). Pri expirácii
použite **Prihlásiť znova**, pri trvalej prevádzke nastavte vhodné publikum
projektu podľa pravidiel Googlu a organizácie.

Oficiálne návody: [čítanie správ](https://developers.google.com/workspace/chat/list-messages),
[desktopové OAuth](https://developers.google.com/identity/protocols/oauth2/native-app).

## Ovládanie v hornej lište

- **Zapnúť / Pozastaviť mostík**: zapína alebo vypína doručovanie, účet
  zostáva prihlásený. Nastavenie sa pamätá aj po opätovnom spustení.
- **Nastavenia…**: zmena displeja, účtu a výberu chatov. Pred zmenou mostík
  pozastavte. Zatvorenie okna nezastaví proces.
- **Skúšobná notifikácia**: odošle test na displej bez potreby Google účtu.
- **Ukončiť Meteo Chat**: aplikáciu úplne vypne. Žiadny samostatný proces
  ani Python po nej nezostáva.
- **Spúšťať po prihlásení do macOS** v nastaveniach je voliteľné. macOS môže
  vyžiadať schválenie v Prihlasovacích položkách. Presuňte aplikáciu do
  Applications pred zapnutím tejto voľby.

Prázdne lietadlo znamená pauzu, vyplnené zapnutý mostík a bublina s výkričníkom
chybu vyžadujúcu pozornosť. Menu obsahuje stav a posledný kontakt.

Mac musí zostať zapnutý a dostupný na internete aj v LAN displeja. Spánok
Macu doručovanie preruší; po prebudení sa spojenie obnoví. Po pauze alebo
výpadku mostík pokračuje od poslednej potvrdenej správy, teda môže doručiť
aj správy z času pauzy. Odobratie a opätovné vybratie chatu začína odteraz.
Vlastné a zmazané správy sa nepreposielajú. Textové prílohy majú oznámenie
s názvom; samotné súbory, história pred výberom a úpravy správ sa nesťahujú.
Google nemusí vracať čitateľný názov každého priameho chatu alebo odosielateľa;
vtedy sa zobrazí jeho identifikátor z API.

## Uložené údaje a bezpečnosť

- OAuth klient, Google tokeny a párovací kľúč sú v **macOS Kľúčenke**, služba
  `sk.meteoplaneradar.chat`. Nezapisujú sa do logov ani konfigurácie v repozitári.
- `~/Library/Application Support/MeteoChat/state.json` uchováva len zapnutie,
  voľbu Claude, identifikátor účtu, názvy vybraných chatov a checkpointy doručovania. Súbor má
  práva `0600`, adresár `0700`; **obsah správ sa doň neukladá**.
- Odhlásenie odstráni Google tokeny z lokálnej položky Kľúčenky a vymaže výber
  chatov. Párovanie displeja a OAuth klient zostávajú pre ďalšie prihlásenie.
  Prístup aplikácie možno odobrať aj v nastaveniach Google účtu.
- Prihlásenie používa náhodný loopback port výhradne na `127.0.0.1`, PKCE a
  kontrolu OAuth state. Po dokončení, zrušení alebo 5 minútach sa port zatvorí.
- Google API používa pevné HTTPS adresy. Automatické HTTP presmerovanie je
  zakázané, aby sa bearer kľúče neposlali inému hostiteľovi. Lokálny displej
  používa adresu z párovacieho súboru, spravidla HTTP v dôveryhodnej domácej LAN.
- Správy majú rovnaké limity ako Python mostík: 1024 UTF-8 bajtov textu a do
  2048 B JSON. Plný rad / nedostupný displej checkpoint neposunie. Potvrdené
  správy sa pri obnovení nestrácajú ani neposielajú opakovane z checkpointu;
  pri páde medzi potvrdením a uložením pomáha stabilné ID a deduplikácia ESP32.
- Nespúšťajte naraz Python mostík aj aplikáciu s rovnakým párovaním a chatmi.
  Na displeji netreba kvôli tejto aplikácii meniť notifikačný protokol ani kľúč.

## Zostavenie a offline testy

Stačia Apple Command Line Tools so Swift 5.9+ (`xcode-select --install`).
Nie sú potrebné knižnice z internetu.

```sh
swift run --build-system native --package-path tools/google-chat-macos ChatCoreChecks
tools/google-chat-macos/build.sh
python3 tests/claude_relay.py "tools/google-chat-macos/dist/Meteo Chat.app/Contents/MacOS/ClaudeUsageRelay"
```

Výstup je v ignorovanom `tools/google-chat-macos/dist/`: univerzálna `.app`
a ZIP pre Apple Silicon aj Intel. Voliteľný prvý argument skriptu je výstupný
adresár. Verziu preberá z `MeteoPlaneRadar/Version.h`; neudržiava druhé číslo.
Na zostavu so zabudovaným Google prihlásením použite
`METEO_CHAT_REQUIRE_OAUTH=1 tools/google-chat-macos/build.sh` po jednorazovom
`configure-oauth.sh`. Predvolený lokálny `config/google-oauth.json` je ignorovaný
Gitom; vlastný súbor zvolí `METEO_CHAT_OAUTH_CONFIG`. Hodnota `none` vytvorí
vývojovú zostavu bez klienta a odstráni predchádzajúcu vloženú konfiguráciu.

Natívny SwiftPM build system je zvolený aj pre prostredia iba s Command Line
Tools, bez XCBuild a XCTest z plného Xcode.

Testy používajú syntetické HTTP odpovede a skutočný lokálny HTTP server; žiadny
Google účet ani displej nekontaktujú. Overujú PKCE, OAuth state a návrat cez
loopback vrátane zrušenia, odmietnutie presmerovania, obmedzenie UTF-8/JSON,
obnovenie tokenu, zastavenie doručovania, obnovu po chybe/plnom rade,
rovnaké časové pečiatky, stránkovanie a vynechanie vlastných správ. Claude testy
overujú zachovanie/obnovu statusLine, sanitizáciu údajov, percentá, zastaranie
a skutočný zabalený pomocník; používajú iba dočasné nastavenia a syntetický vstup.

Skutočné Google OAuth prihlásenie, firemné schválenie a doručenie na fyzický
ESP32 sa musia dokončiť s vaším účtom a displejom; offline testy ich nenahrádzajú.
