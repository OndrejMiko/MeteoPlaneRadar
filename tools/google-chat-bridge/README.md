# Google Chat → MeteoPlaneRadar (Python)

**Pre Mac je dostupná aj [natívna aplikácia Meteo Chat](../google-chat-macos/README.md)**
s ikonou v hornej lište, pauzou a prihlásením cez prehliadač, bez Pythonu.
Používa rovnaký párovací súbor a notifikačný protokol. Nespúšťajte oba
mostíky pre tie isté chaty naraz.

Most číta nové správy z vybraných priestorov Google Chat a doručuje ich na
ESP32 cez domácu LAN. Funguje s Pythonom 3.9+ bez ďalších balíčkov. Počítač,
Mac alebo Raspberry Pi musí počas doručovania zostať zapnutý a most spustený.
Displej neotvárajte do internetu. Na zariadení zostáva len samostatný párovací
kľúč; prihlásenie a tokeny Googlu zostávajú na počítači.

Google dokumentuje čítanie Chat API pre Google Workspace. Dostupnosť pre váš
účet a povolenie OAuth závisia od Googlu a správcu organizácie; osobný Gmail
účet tu nie je overený. **Incoming webhook Google Chat nie je prijímač správ**:
slúži na opačný smer, posielanie do priestoru Chatu.

## 1. Displej a webové UI

1. Nahrajte firmware 0.7.3 a pripojte zariadenie k domácej Wi-Fi.
2. Otvorte jeho IP adresu alebo `http://meteoplaneradar.local/` (aktuálny názov
   overíte v nastaveniach zariadenia). Vyberte **Notifikácie**.
3. Nastavte trvanie bannera: 0 = do zatvorenia, inak 5–120 sekúnd.
4. Ak máte heslo správcu, vyplňte aktuálne heslo na karte **Systém**.
5. Kliknite **Spárovať most / stiahnuť konfiguráciu**. Zapne notifikácie,
   vytvorí nový kľúč a stiahne `meteoplaneradar-chat.json`.
   Každé nové párovanie zneplatní predchádzajúci kľúč. Nezdieľajte súbor.
6. **Uložiť a zobraziť skúšku** ukáže banner aj bez pripojeného Googlu.
   Zmeny ostatných nastavení uložte tlačidlom vpravo hore.

Po importe zálohy do nového zariadenia zopakujte párovanie.

Google účet sa neprihlasuje priamo na ESP32. Na párovanie je potrebná domáca
Wi-Fi, nie konfiguračný prístupový bod. Ak sa IP zariadenia mení, nastavte
rezerváciu DHCP v routeri alebo v lokálnom párovacom súbore použite jeho
funkčný mDNS názov.

## 2. Prihlásenie Google

Na **rovnakom počítači, na ktorom spúšťate setup**, otvorte
[Google Cloud Console](https://console.cloud.google.com/):

1. Vyberte/vytvorte projekt a povoľte **Google Chat API**.
   V jeho konfigurácii vyplňte povinné údaje aplikácie podľa konzoly.
2. V **Google Auth Platform** nastavte Branding, Audience a Data Access.
   Pri testovacej externej aplikácii pridajte svoj účet do Test users.
   Používajú sa iba `openid`, `chat.spaces.readonly`, `chat.messages.readonly`.
   Práva čítať správy sú citlivé; firemný správca ich môže blokovať.
3. Vytvorte OAuth client ID typu **Desktop app** a stiahnite jeho JSON.
   Uložte ho ako `client_secret.json` do priečinka repozitára.
4. Do toho istého priečinka presuňte párovací súbor z webového UI.
5. V termináli v priečinku repozitára spustite:

```sh
python3 tools/google-chat-bridge/bridge.py setup --client client_secret.json --pair meteoplaneradar-chat.json
```

Otvorí sa prehliadač, prihlásite sa a povolíte čítanie Chatu. Návrat používa
lokálnu adresu `127.0.0.1` s náhodným portom, PKCE a kontrolu OAuth state;
nejde o zrušený tok s kopírovaním kódu. Preto neprihlasujte prehliadač na inom
počítači než ten, kde beží setup. V termináli vyberte čísla priestorov,
napríklad `1,3`. Most overí spojenie s displejom.

Pre Raspberry Pi bez monitora: setup vykonajte na počítači s prehliadačom a
potom súkromne preneste celý priečinok `.chat-bridge` na Pi. Zachovajte práva
súborov iba pre vlastníka. Nesynchronizujte tento priečinok do verejného cloudu.
Kredenciály aj párovacie súbory sú v `.gitignore`.

Testovací režim externej OAuth aplikácie môže obmedzovať životnosť refresh
tokenu (typicky 7 dní). Pri `invalid_grant` spustite setup znova; pre trvalú
prevádzku nastavte vhodný interný/produkčný režim podľa pravidiel Googlu.
Spravované účty môžu vyžadovať schválenie správcom.

## 3. Prevádzka

```sh
python3 tools/google-chat-bridge/bridge.py run
```

Predvolené obnovenie je 15 sekúnd (`run --interval 30` ho zmení). Nechajte
príkaz bežať, prípadne ho pridajte do správcu služieb na počítači/Pi. Most
počúva iba dočasne na loopbacku počas prihlasovania; za prevádzky žiadny
server ani verejný port neotvára. Do Googlu pristupuje cez overované HTTPS.
Lokálne doručenie používa HTTP a oddelený bearer kľúč, rovnako ako webové UI
zariadenia; je určené pre dôveryhodnú domácu LAN.

Voliteľný priečinok dát musí byť pred podpríkazom:

```sh
python3 tools/google-chat-bridge/bridge.py --state /sukromna/cesta/chat run
```

V **Notifikácie** uvidíte počet čakajúcich správ, spotrebu PSRAM a vek
posledného kontaktu mosta. `−1` znamená, že most ešte nekontaktoval zariadenie.
Samotné „spárovaný“ znamená uložený kľúč, nie overené prihlásenie ku Googlu.

- Ťuknutie na banner otvorí celý text, pri dlhšej správe listujte `<` / `>`.
- Krížik je 32 × 32 px; aktívna oblasť 64 × 64 px nemá viditeľné pozadie.
- Zatvorenie odstráni správu iba z displeja; pri ďalšej správe otvorí nový banner.
- Po uplynutí trvania banner zmizne, správy zostanú pod odznakom s počtom dole.
  Ťuknutie na odznak ich opäť otvorí. Detail sa automaticky nezatvára.
- Otvorený banner/detail zachytí dotyky a pozastaví automatické prepínanie stránok.
- Prijímanie funguje nad hodinami, radarom, počasím, kalendárom aj nastaveniami.
  Počas blokujúceho sťahovania radaru/ICS môže byť doručenie oneskorené.
- Vypnutie notifikácií zmaže frontu a uvoľní jej PSRAM. Správy sa na ESP32
  nepersistujú: reštart vyprázdni lokálnu frontu.

## Čo sa doručuje

Nové správy od ostatných používateľov vo vybraných priestoroch po dokončení
setupu, aj keď ste ich už inde prečítali. Vlastné správy sa preskočia. Nejde o
zrkadlenie push notifikácií aplikácie Google Chat ani o synchronizáciu stavu
prečítané/neprečítané. História, úpravy a zmazania sa nesynchronizujú.

Doručuje sa text, prípadne názov prílohy pri správe bez textu; žiadne obrázky,
interaktívne karty, stiahnuté prílohy ani odpovedanie. Text je obmedzený na
1024 bajtov UTF-8 a na displeji sa slovenská/česká diakritika prepisuje podľa
existujúceho písma, emoji na `?`. Mená odosielateľov závisia od údajov API;
ak Google vráti len `users/123…`, môžete do `.chat-bridge/config.json` pridať:

```json
"aliases": { "users/123456789": "Matus" }
```

Fronta má 6 miest. Pri zaplnení vráti ESP32 HTTP 503 a most neskôr zopakuje
nepotvrdenú správu. Google stránkovanie, kontrolné body a potvrdené ID sú
uložené lokálne, takže prechod medzi stránkami ani rovnaké časy správ nestratia
správy pri bežnom prerušení. ESP32 drží hash posledných 32 prijatých ID proti
bezprostredným duplicitám. Nie je to garantovaná trvalá schránka; reštart ESP32
alebo odstránenie lokálneho stavu môže viesť k strate/duplicitnému upozorneniu.
Nespúšťajte dva mosty s rovnakým priečinkom dát súčasne.

## Riešenie problémov

- HTTP 403 z displeja: nové párovanie zneplatnilo starý kľúč; zopakujte setup
  s aktuálnym súborom. HTTP 403 z Googlu: API, scope alebo firemné povolenia.
- HTTP 409: notifikácie sú vypnuté. Zapnite ich vo webovom UI.
- HTTP 503: plná fronta alebo nedostatok PSRAM; zavrite čakajúce správy,
  skontrolujte pamäť v UI. Most používa obmedzený odstup medzi opakovaniami.
- HTTP 400 pri obnovení Googlu: často expirovaný/odvolaný refresh token;
  spustite setup znova. Hlásenia nezverejňujú tokeny ani obsah správ.
- Zmena siete/IP: overte adresu v párovacom súbore a zopakujte setup.
- Odpojenie Googlu: zastavte most, odoberte aplikácii prístup v Google účte,
  vymažte súkromný `.chat-bridge` a vypnite notifikácie na displeji.

## Pamäť a overenie

Zostavenie 0.7.3 pre profil `default`: **1 675 285 B flash**, **170 576 B statickej
RAM**, **157 104 B** zostáva na dynamické alokácie a zásobníky. Oproti 0.7.2:
**+17 032 B flash, +112 B statickej RAM**. Fronta a história ID sa alokujú až
pri prvej správe: **7 176 B PSRAM**, bez ďalšieho framebufferu. JSON požiadavka
je ohraničená na 2048 B; jej parsovanie a HTTP server používajú aj dynamickú
RAM. Tieto údaje nie sú meraním minimálneho voľného heapu za behu.

Doska má 8 MiB PSRAM. Kalendár má hornú hranicu približne 679 kB, notifikácie
pridávajú približne 7 kB. Podľa zostavenia a limitov sa zmestia; reálny
najhorší súbeh Wi-Fi/TLS/radaru a dlhodobú prevádzku treba potvrdiť na module.
Aktuálny voľný heap/PSRAM sú na karte Ovládanie. Host testy overujú parser,
frontu, deduplikáciu, dotykové oblasti, nedostatok pamäte, párovanie, HTTP
prenos, refresh token a pokračovanie doručenia cez stránky/reštart mosta.
Fyzický panel a prihlásenie do reálneho Google účtu zatiaľ neboli testované.

Oficiálne podklady: [čítanie správ](https://developers.google.com/workspace/chat/api/reference/rest/v1/spaces.messages/list),
[zoznam priestorov](https://developers.google.com/workspace/chat/api/reference/rest/v1/spaces/list),
[desktop OAuth](https://developers.google.com/identity/protocols/oauth2/native-app),
[obmedzenia incoming webhookov](https://developers.google.com/workspace/chat/quickstart/webhooks),
[životnosť OAuth tokenov](https://developers.google.com/identity/protocols/oauth2#expiration).
