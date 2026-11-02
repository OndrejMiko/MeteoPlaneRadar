# Meteo Chat: presný postup pre Google prihlásenie jedným tlačidlom

Pripravené 2. novembra 2026. Cieľ na ráno: raz zaregistrovať aplikáciu,
pridať jej OAuth klienta pri zostavení a potom sa prihlasovať cez prehliadač.
Používateľ hotovej aplikácie nebude importovať JSON.

## Čo si pripraviť

- Mac s macOS 13+ a zdrojovým projektom v
  `/Users/akolyt/www/develop/meteoradar`.
- Apple Command Line Tools so Swift 5.9+; overíš `swift --version`.
  Ak chýbajú, spusti `xcode-select --install` a dokonči inštaláciu.
- Prístup k svojmu Google Cloud projektu a účtu, ktorým budeš čítať Chat.
- Displej s firmware **0.7.13 alebo novším**, Mac a displej v rovnakej LAN.
- Druhého človeka alebo druhý účet, ktorý pošle skúšobnú správu do vybraného
  chatu. Vlastné správy mostík zámerne preskakuje.

Google pre tento spôsob čítania Chatu uvádza Business/Enterprise Workspace
účet. Osobný Gmail tu nie je overený. Firemné pravidlá môžu vyžadovať správcu.
[Predpoklady Google Chat API](https://developers.google.com/workspace/chat/list-messages).

Kód na displeji páruje Mac s ESP32. Google OAuth je samostatné povolenie
na čítanie Chatu; na ESP32 sa Google heslo ani token neukladá.

## 1. Vytvor alebo vyber projekt

1. Otvor [Google Cloud Console](https://console.cloud.google.com/).
2. Prihlás sa účtom, pod ktorým chceš spravovať aplikáciu.
3. V hornom výbere projektov zvoľ **New project / Nový projekt**.
4. Názov projektu: **Meteo Chat**. Organizáciu zvoľ podľa svojho Workspace.
5. Vytvor projekt a skontroluj, že je v hornom prepínači vybraný.

Ak firemná politika nedovolí vytvorenie projektu alebo zapnutie API, túto
časť musí pripraviť správca. Prihlásenie v našej aplikácii to neobíde.

## 2. Zapni Google Chat API

1. Otvor **APIs & Services → Library**.
2. Vyhľadaj **Google Chat API** a zvoľ **Enable**.

Pre naše používateľské read-only volania stačí zapnutie API a OAuth klient.
Nepotrebuješ incoming webhook, service account, API key ani verejný server
pre prichádzajúce správy. Interaktívny Chat bot sa tu nevytvára.
[Google: read-only konfigurácia](https://developers.google.com/workspace/chat/configure-chat-api).

## 3. Nastav Google Auth Platform

Otvor **Google Auth Platform → Branding**. Ak sa zobrazí úvodný sprievodca,
klikni **Get started**.

Vyplň názov **Meteo Chat**, svoj kontaktný/support e-mail a kontakt autora.
Pri vytváraní potvrď podmienky až po ich prečítaní.

V **Audience** vyber jednu možnosť:

- **External + Testing** na úvodný test. V **Test users → Add users** pridaj
  presný e-mail účtu, ktorým sa zajtra prihlásiš.
- **Internal**, ak je voľba dostupná a aplikáciu majú používať výhradne účty
  tej istej Workspace organizácie. Dostupnosť určuje organizácia.

Zatiaľ nepotrebuješ publikovať aplikáciu pre verejnosť.
[Google: Branding a Audience](https://developers.google.com/workspace/guides/configure-oauth-consent).

## 4. Pridaj presne tieto oprávnenia

V **Google Auth Platform → Data Access → Add or Remove Scopes** vyber/zadaj:

```text
openid
https://www.googleapis.com/auth/chat.spaces.readonly
https://www.googleapis.com/auth/chat.messages.readonly
```

Ulož zmeny. `openid` sa môže zobrazovať v základných identifikačných
oprávneniach. Zvyšné dva scopes pokrývajú zoznam chatov a čítanie správ.
Kód žiada práve tieto tri prístupy. Netreba právo písať alebo mazať správy.

`chat.spaces.readonly` Google označuje ako **sensitive**,
`chat.messages.readonly` ako **restricted**. To je podstatné pre neskoršiu
verejnú distribúciu, nie dôvod pridávať širšie oprávnenia.
[Google: zoznam a kategórie scopes](https://developers.google.com/workspace/chat/authenticate-authorize).

## 5. Vytvor OAuth klienta aplikácie

1. **Google Auth Platform → Clients → Create client**.
2. **Application type: Desktop app**.
3. Názov: **Meteo Chat macOS**.
4. Klikni **Create** a hneď **Download JSON**.
5. V Downloads premenuj stiahnutý súbor na **meteochat-desktop.json**.

JSON stiahni pri vytvorení: Google obmedzuje neskoršie zobrazenie/download
plného client secret. Ulož si pôvodný súbor súkromne.
[Správa OAuth klientov](https://support.google.com/cloud/answer/15549257?hl=en).

Použi Desktop app, nie Web application, iOS, TV alebo service account.
Desktop flow nepotrebuje ručne pridávať webové JavaScript origins ani pevnú
redirect URL. Aplikácia otvorí dočasný `127.0.0.1:<náhodný-port>/callback`;
port nemusíš nastavovať v routeri alebo otvárať do internetu.
[Desktop klient](https://developers.google.com/workspace/guides/create-credentials),
[loopback prihlasovanie](https://developers.google.com/identity/protocols/oauth2/native-app).

## 6. Pridaj klienta do projektu – raz na strane autora

V Termináli spusti tieto príkazy. Predpokladajú premenovanie súboru v kroku 5.

```sh
cd /Users/akolyt/www/develop/meteoradar

tools/google-chat-macos/configure-oauth.sh "$HOME/Downloads/meteochat-desktop.json"
```

Skript overí, že ide o Desktop konfiguráciu so skutočným client ID a secret.
Z pôvodného JSON vyberie iba tieto dve registračné hodnoty a uloží ich do:

```text
tools/google-chat-macos/config/google-oauth.json
```

Tento lokálny súbor Git ignoruje; necommituj ho ani jeho obsah neposielaj do
chatu. Ukážka `config/google-oauth.example.json` je iba šablóna a validátor ju
odmietne. Do tohto súboru nikdy nepridávaj access/refresh tokeny, párovací
kľúč displeja ani heslo Google účtu.

Úspech spoznáš podľa správy **Google Desktop OAuth konfigurácia je pripravená**.
Ak chýba client secret alebo si stiahol webového klienta, vráť sa do kroku 5.

## 7. Zostav aplikáciu so zabudovaným prihlásením

Z rovnakého priečinka:

```sh
METEO_CHAT_REQUIRE_OAUTH=1 tools/google-chat-macos/build.sh
```

Táto voľba zastaví build, ak OAuth konfigurácia chýba. Zabráni tak zostave,
v ktorej by používateľ znovu našiel neaktívne prihlasovacie tlačidlo.

Výsledok:

```text
tools/google-chat-macos/dist/Meteo Chat.app
tools/google-chat-macos/dist/Meteo-Chat-<aktuálna-verzia>-macOS.zip
```

Verzia pochádza z `MeteoPlaneRadar/Version.h`. Aplikácia obsahuje registračné
údaje ako `Contents/Resources/GoogleOAuth.json`. Build overí a normalizuje ich,
až potom podpíše balík. Používateľ už tento JSON nikam nevyberá.

Alternatívne môžeš použiť pôvodný stiahnutý JSON priamo bez lokálnej kópie:

```sh
METEO_CHAT_OAUTH_CONFIG="$HOME/Downloads/meteochat-desktop.json" \
METEO_CHAT_REQUIRE_OAUTH=1 \
tools/google-chat-macos/build.sh
```

Vývojová zostava bez predvoleného klienta sa robí explicitne takto:

```sh
METEO_CHAT_OAUTH_CONFIG=none tools/google-chat-macos/build.sh
```

Natívna aplikácia je verejný OAuth klient: registračné údaje v distribuovanom
balíku možno prezrieť. Ochrana prihlásenia používa PKCE a náhodný state;
Google používateľské tokeny zostávajú v Kľúčenke. Client secret v desktopovom
balíku nepovažuj za Google heslo alebo dôkaz dôveryhodnosti používateľa.
[Google: desktopové aplikácie sú public clients](https://developers.google.com/identity/protocols/oauth2/native-app).

## 8. Nainštaluj a prvýkrát sa prihlás

1. Ukonči starý **Meteo Chat** cez jeho menu v hornej lište.
2. Novú **Meteo Chat.app** z priečinka `dist` presuň do **Applications**.
3. Spusti novú aplikáciu a otvor **Nastavenia…**.
4. Tlačidlo **Prihlásiť cez Google** má byť aktívne aj bez spárovania displeja.
5. Klikni naň, v prehliadači vyber svoj testovací/Workspace účet a povoľ všetky
   tri žiadané oprávnenia.
6. Po úspešnom návrate môžeš kartu prehliadača zavrieť. V aplikácii sa zobrazí
   prihlásený stav a zoznam chatov; prípadne klikni **Obnoviť**.

Ak si v minulosti importoval iného OAuth klienta, funkčné prihlásenie sa
pri aktualizácii zachová. Nového klienta z tejto zostavy zapneš cez
**Pokročilé nastavenie Googlu → Použiť predvolené prihlásenie Meteo Chat**.
Tým sa odhlási starý Google účet a vymaže výber chatov; párovanie ESP32 ostane.

Build je ad-hoc podpísaný, zatiaľ bez Apple notarizácie. Prípadné prvé
povolenie aplikácie rieš štandardným spôsobom cez macOS Súkromie a bezpečnosť;
nevypínaj systémové ochrany. Google overenie a Apple notarizácia sú samostatné.

## 9. Spáruj displej a over skutočnú správu

1. Vo webovom UI displeja otvor **Notifikácie → Vygenerovať párovací kód**.
   Ak je nastavené heslo správcu, vyplň ho predtým na karte Systém.
2. Do Mac aplikácie zadaj adresu displeja a šesť číslic. Klikni **Spárovať kódom**.
3. **Overiť displej** má zobraziť skúšobnú notifikáciu. Tento krok funguje
   nezávisle od Google účtu a overuje LAN, párovanie aj notifikačnú vrstvu.
4. Vyber aspoň jeden chat a klikni **Zapnúť mostík**.
5. Až teraz nech ti druhý účet pošle novú správu do vybraného chatu.
6. Počkaj aspoň jeden interval kontroly (15 sekúnd). Pri blokujúcom sťahovaní
   radaru alebo plnom rade displeja môže doručenie trvať dlhšie.
7. Over text na displeji, stav v menu a čas posledného kontaktu.
8. Vyskúšaj pauzu, opätovné zapnutie a ukončenie/opätovné spustenie aplikácie.

Nový výber chatu začína od aktuálneho času. Správa poslaná pred výberom sa
neprepošle. Po pauze sa pokračuje z posledného potvrdeného checkpointu.

## Ak sa niečo nepodarí

| Prejav | Čo skontrolovať |
| --- | --- |
| Google tlačidlo je neaktívne | Spustená nová aplikácia z `dist`/Applications; strict build prešiel; `GoogleOAuth.json` je v balíku. Klient nesmie byť ukážkový. |
| `invalid_client`, HTTP 400/401 pri prihlásení | Správny projekt, Desktop app, úplný stiahnutý client secret; zopakuj configure a strict build. Pri vlastnom starom klientovi prepnúť predvolené prihlásenie. |
| `redirect_uri_mismatch` | Použitý Desktop app, nie Web/iOS/TV klient. V aplikácii sa nesnaž meniť callback na verejnú doménu. |
| `access_denied`, testovací účet nie je povolený | Audience → Test users obsahuje presný účet, ktorý si vybral v prehliadači. |
| Aplikáciu blokuje organizácia | Workspace správca musí povoliť OAuth klienta a požadované oprávnenia podľa firemnej politiky. |
| Prihlásenie prebehne, Chat API vráti 403 | Zapnuté Chat API v tom istom projekte, udelené oba Chat scopes, prístup účtu k Chatu a pravidlá správcu. |
| Chaty sa nezobrazia | Členstvo účtu v chatoch a vyššie uvedené povolenia; klikni Obnoviť. Niektoré chaty API vracia iba pod identifikátorom. |
| Kód nefunguje / endpoint 404 | Firmware aspoň 0.7.13, správna LAN adresa, nový nepoužitý kód do dvoch minút. |
| Test displeja ide, správa nie | Vybraný správny chat, mostík zapnutý, nová správa od iného účtu, Mac bdelý a online. |
| Po čase treba prihlásenie znova | V Testing režime externého projektu má refresh token pri našich scopes obvykle 7-dňovú platnosť; tiež môže byť odvolaný účtom alebo správcom. |

[Google: platnosť tokenov v Testing](https://developers.google.com/identity/protocols/oauth2),
[Google: správa publika](https://support.google.com/cloud/answer/15549945?hl=en).

Pri diagnostike posielaj len chybu, HTTP kód a verziu aplikácie. Neposielaj
obsah OAuth JSON, tokeny, párovací kľúč ani text súkromných správ.

## Pred verejnou distribúciou

Na ranné vlastné otestovanie stačí vyššie uvedený Testing alebo vhodný
Internal projekt. Pred verejným vydaním priprav samostatný produkčný projekt,
reálne kontakty, web a zásady ochrany súkromia, prípadné overenie domény,
odôvodnenie scopes a ukážku prihlasovania/spracovania dát pre Google review.

Kvôli restricted scope môže byť potrebné dodatočné overenie a podľa
spracovania dát aj bezpečnostné posúdenie; konkrétne požiadavky alebo výnimky
určuje Google. Samotné prepnutie na Production sa nerovná dokončenému review.
Apple Developer ID podpis a notarizáciu rieš osobitne pred distribúciou Mac app.
[Google: restricted scope verification](https://developers.google.com/identity/protocols/oauth2/production-readiness/restricted-scope-verification).

## Čo je už pripravené v kóde

- Automatické predvolené prihlásenie z balíka aplikácie, bez importu používateľom.
- Validácia registračného JSON a odmietnutie ukážkových/zamenených klientov.
- Strict build, normalizovaný export bez používateľských tokenov a odstránenie
  starej konfigurácie pri opakovanom zostavení bez nej.
- PKCE, state, dočasný loopback, zrušenie, obnova tokenov a macOS Kľúčenka.
- Zachovanie už importovaného funkčného účtu a pokročilá voľba prepnúť na default.
- Samostatné párovanie displeja kódom a doručovanie s checkpointmi.

Offline overenie pred ranným reálnym testom:

```sh
swift run --build-system native --package-path tools/google-chat-macos ChatCoreChecks
tests/macos_oauth_build.sh
```

Reálna registrácia projektu, client ID/secret, súhlas používateľa a prípadné
schválenie správcom ešte nie sú vykonané. V repozitári nie sú skutočné
registračné údaje; dnešná všeobecná zostava preto Google prihlásenie zatiaľ
nezapína. Po krokoch 6–7 ho bude mať tvoja nová zostava pripravené.
