# Meteo Chat pre macOS

Jednoduchá natívna aplikácia pre **macOS 13 a novší**, Apple Silicon aj Intel.
Žije v hornej lište, bez ikony v Docku. **Nepotrebuje Python, Homebrew ani
ďalšie balíčky.** Posiela nové správy z vybraných chatov na MeteoPlaneRadar.

## Prvé spustenie

1. Rozbaľte `Meteo-Chat-<verzia>-macOS.zip` a presuňte **Meteo Chat.app** do
   **Applications / Aplikácie**. Spustite ju; zobrazí sa ikona papierového
   lietadla v hornej lište a okno nastavení.
2. Vo webovom UI displeja otvorte **Notifikácie → Spárovať most / stiahnuť
   konfiguráciu**. V aplikácii zvoľte **Importovať párovanie…** a vyberte
   `meteoplaneradar-chat.json`. **Overiť displej** doručí skúšobnú správu.
3. Pri prvom nastavení vytvorte v Google Cloud OAuth klienta typu **Desktop
   app** podľa postupu nižšie. Jeho stiahnutý JSON vyberte cez
   **Importovať OAuth klienta…**.
4. Kliknite **Prihlásiť cez Google**. Otvorí sa predvolený prehliadač;
   vyberte účet a povoľte čítanie Chatu. Po návrate môžete kartu zatvoriť.
   Heslo do Meteo Chatu nezadávate.
5. Vyberte chaty a kliknite **Zapnúť mostík**. Prvý výber začína od aktuálneho
   času, stará história sa neposiela. Načítanie je každých 15 sekúnd.

Aplikácia je pri lokálnom zostavení podpísaná **ad-hoc**, nie Apple Developer
ID ani notarizovaná. Ak ju macOS pri prvom otvorení zablokuje, použite jeho
štandardnú možnosť **Otvoriť aj tak** v Systémových nastaveniach → Súkromie
a bezpečnosť. Nevypínajte Gatekeeper. Na verejnú distribúciu treba podpis
Developer ID a notarizáciu účtom autora distribúcie.

## Google Cloud – iba pri prvom nastavení

Prihlásenie cez prehliadač potrebuje identitu aplikácie u Googlu. Repozitár
neobsahuje cudzí OAuth projekt ani univerzálne prihlasovacie údaje.

1. V [Google Cloud Console](https://console.cloud.google.com/) vyberte alebo
   vytvorte projekt a povoľte **Google Chat API**.
2. V Google Auth Platform nastavte údaje aplikácie a publikum. Pri externom
   testovacom projekte pridajte vlastný účet medzi **Test users**.
3. Povoľte prístupy `openid`, `chat.spaces.readonly` a
   `chat.messages.readonly`. Správca organizácie ich môže blokovať.
4. Vytvorte OAuth client ID typu **Desktop app**, stiahnite JSON a importujte
   ho do aplikácie. Webový OAuth klient ani súbor servisného účtu nefunguje.

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
  identifikátor účtu, názvy vybraných chatov a checkpointy doručovania. Súbor má
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
```

Výstup je v ignorovanom `tools/google-chat-macos/dist/`: univerzálna `.app`
a ZIP pre Apple Silicon aj Intel. Voliteľný prvý argument skriptu je výstupný
adresár. Verziu preberá z `MeteoPlaneRadar/Version.h`; neudržiava druhé číslo.
Natívny SwiftPM build system je zvolený aj pre prostredia iba s Command Line
Tools, bez XCBuild a XCTest z plného Xcode.

Testy používajú syntetické HTTP odpovede a skutočný lokálny HTTP server; žiadny
Google účet ani displej nekontaktujú. Overujú PKCE, OAuth state a návrat cez
loopback vrátane zrušenia, odmietnutie presmerovania, obmedzenie UTF-8/JSON,
obnovenie tokenu, zastavenie doručovania, obnovu po chybe/plnom rade,
rovnaké časové pečiatky, stránkovanie a vynechanie vlastných správ.

Skutočné Google OAuth prihlásenie, firemné schválenie a doručenie na fyzický
ESP32 sa musia dokončiť s vaším účtom a displejom; offline testy ich nenahrádzajú.
