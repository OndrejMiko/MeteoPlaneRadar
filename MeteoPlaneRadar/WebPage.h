// =============================================================================
//  MeteoPlaneRadar
//  The configuration page, as one PROGMEM string.
//
//  One file, no CDN, no build step: the device has to be configurable on a
//  network with no internet at all, so markup, style and script all ship inside
//  the firmware.
//
//  The page is split into TABS. Twenty-five controls in one column is a long
//  scroll on a phone, and most of them are set once and never touched again -
//  while the remote control and the status are opened repeatedly, so they get
//  the landing tab. Tabs rather than separate pages because every field stays
//  in the DOM: one Save button still posts the whole form, with no partial
//  saves to reconcile.
//
//  The page also translates itself. Shipping all three languages as data and
//  switching them in the browser costs a few kilobytes of flash and saves a
//  second Czech string table in C that would have to be kept in step by hand.
//
//  Project: MeteoPlaneRadar - live aircraft radar on a round touchscreen
//  Author:  Petr / chiptron.cz   (vyvoj / development: chiptron.cz)
//  Web:     https://chiptron.cz
// =============================================================================
#pragma once
#include <Arduino.h>

static const char PAGE_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="cs"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="color-scheme" content="dark">
<title>MeteoPlaneRadar</title>
<style>
:root{--bg:#12161c;--card:#1b212b;--line:#2b3441;--fg:#e8edf4;--mut:#93a1b3;--acc:#37c0e8;--ok:#3fbf7f;--warn:#e8a33d;--err:#e05561}
*{box-sizing:border-box}
body{margin:0;background:var(--bg);color:var(--fg);font:15px/1.5 system-ui,-apple-system,Segoe UI,Roboto,sans-serif;-webkit-text-size-adjust:100%}
header{padding:12px 16px;display:flex;align-items:baseline;gap:10px;flex-wrap:wrap}
h1{font-size:18px;margin:0}
.ver{color:var(--mut);font-size:13px}
nav.tabs{display:flex;gap:6px;overflow-x:auto;padding:0 12px 10px;border-bottom:1px solid var(--line);
 position:sticky;top:0;background:var(--bg);z-index:5;scrollbar-width:none}
nav.tabs::-webkit-scrollbar{display:none}
nav.tabs button{background:transparent;color:var(--mut);border:1px solid var(--line);border-radius:999px;
 padding:8px 15px;white-space:nowrap;font-weight:500;font-size:14px;cursor:pointer}
nav.tabs button.on{background:var(--acc);color:#08202a;border-color:var(--acc);font-weight:600}
.wrap{max-width:820px;margin:0 auto;padding:16px 16px 96px}
.card{background:var(--card);border:1px solid var(--line);border-radius:10px;padding:14px;margin-bottom:14px}
.card h2{font-size:15px;margin:0 0 10px;color:var(--acc)}
.row{display:flex;align-items:center;gap:10px;margin:8px 0;flex-wrap:wrap}
.row label{flex:1 1 200px;min-width:150px}
input[type=text],input[type=password],input[type=number],select{background:#0e1218;color:var(--fg);
 border:1px solid var(--line);border-radius:6px;padding:8px 9px;min-width:110px;font-size:15px}
input[type=color]{background:#0e1218;border:1px solid var(--line);border-radius:6px;height:38px;width:60px;padding:2px}
input[type=range]{flex:1 1 160px}
input[type=checkbox]{width:20px;height:20px;accent-color:var(--acc)}
button{background:var(--acc);color:#08202a;border:0;border-radius:7px;padding:10px 14px;font-weight:600;
 cursor:pointer;font-size:15px}
button.sec{background:#39434f;color:var(--fg)}
button.danger{background:var(--err);color:#fff}
button:disabled{cursor:default}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:8px}
.chk{display:flex;align-items:center;gap:8px}
.hint{color:var(--mut);font-size:13px;margin:8px 0 0}
.bar{position:fixed;left:0;right:0;bottom:0;background:var(--bg);border-top:1px solid var(--line);
 padding:12px 16px calc(12px + env(safe-area-inset-bottom));display:flex;gap:12px;align-items:center;z-index:6}
#msg{font-size:14px}
.ok{color:var(--ok)}.err{color:var(--err)}.warn{color:var(--warn)}
table{width:100%;border-collapse:collapse;font-size:14px}
td{padding:5px 0;border-bottom:1px solid var(--line)}
td:first-child{color:var(--mut);width:50%}
.hide{display:none}
#scrBtns button{padding:8px 12px;font-size:14px}
@media (max-width:520px){
  .row label{flex:1 1 100%}
  .row input[type=text],.row input[type=password],.row input[type=number],.row select{flex:1 1 100%}
}
</style></head><body>
<header>
  <h1>MeteoPlaneRadar</h1><span class="ver" id="ver"></span>
  <span style="flex:1"></span>
  <select id="uiLang" onchange="setLang(this.value)"><option value="0">Čeština</option><option value="1">English</option><option value="2">Slovenčina</option></select>
</header>

<nav class="tabs" id="tabs">
  <button data-tab="tCtl"    class="on" data-i18n="tabCtl">Ovládání</button>
  <button data-tab="tLoc"    data-i18n="tabLoc">Poloha</button>
  <button data-tab="tScr"    data-i18n="tabScr">Obrazovky</button>
  <button data-tab="tLook"   data-i18n="tabLook">Vzhled</button>
  <button data-tab="tPlanes" data-i18n="tabPlanes">Letadla</button>
  <button data-tab="tCal" data-i18n="scrCalendar">Kalendář</button>
  <button data-tab="tNotify" data-i18n="ntTitle">Notifikácie</button>
  <button data-tab="tSys"    data-i18n="tabSys">Systém</button>
</nav>

<div class="wrap">

<!-- ================= OVLÁDÁNÍ ================= -->
<section id="tCtl" class="tab">
  <div class="card hide" id="cardWifi">
    <h2 data-i18n="wifi">WiFi</h2>
    <p class="hint" id="wifiNow"></p>
    <div class="row"><label data-i18n="network">Síť</label>
      <select id="ssid" style="flex:2 1 220px"></select>
      <button class="sec" onclick="scan()" data-i18n="scan">Vyhledat</button></div>
    <div class="row"><label data-i18n="password">Heslo</label><input type="password" id="wpass" style="flex:2 1 220px"></div>
    <p class="hint" id="wifiHintTxt"></p>
    <button onclick="saveWifi()" data-i18n="connect">Připojit</button>
  </div>

  <div class="card" id="cardRemote">
    <h2 data-i18n="remote">Dálkové ovládání</h2>
    <div class="row" id="scrBtns"></div>
    <div class="row">
      <button class="sec" onclick="stepScreen(-1)">&#8592;</button>
      <button class="sec" onclick="stepScreen(1)">&#8594;</button>
      <span style="flex:1"></span>
      <span data-i18n="rangeLbl">Rozsah</span>
      <button class="sec" id="rMinus" onclick="stepRange(-1)">&minus;</button>
      <b id="rangeNow" style="min-width:80px;text-align:center">&ndash;</b>
      <button class="sec" id="rPlus" onclick="stepRange(1)">+</button>
    </div>
    <p class="hint" data-i18n="remoteHint">Rozsah se mění jen na obrazovkách Letadla a Meteoradar. Zásah pozastaví automatické střídání.</p>
  </div>

  <div class="card">
    <h2 data-i18n="statusHdr">Stav zařízení</h2>
    <table id="statusTab"></table>
  </div>
</section>

<!-- ================= POLOHA ================= -->
<section id="tLoc" class="tab hide">
  <div class="card">
    <h2 data-i18n="location">Poloha</h2>
    <div class="row"><label data-i18n="findCity">Najít město</label>
      <input type="text" id="q" style="flex:2 1 200px" placeholder="Praha">
      <button class="sec" onclick="geo()" data-i18n="search">Hledat</button></div>
    <div class="row hide" id="geoRow"><label data-i18n="found">Nalezeno</label>
      <select id="geoSel" style="flex:2 1 240px" onchange="pickCity()"></select></div>
    <div class="row"><label data-i18n="lat">Zeměpisná šířka</label><input type="number" step="0.0001" id="lat"></div>
    <div class="row"><label data-i18n="lon">Zeměpisná délka</label><input type="number" step="0.0001" id="lon"></div>
    <p class="hint" data-i18n="locHint">Změna polohy vyžaduje restart, o který se zařízení postará samo.</p>
  </div>
</section>

<!-- ================= OBRAZOVKY ================= -->
<section id="tScr" class="tab hide">
  <div class="card">
    <h2 data-i18n="screens">Obrazovky</h2>
    <div class="grid">
      <label class="chk"><input type="checkbox" id="sClock"><span data-i18n="scrClock">Hodiny</span></label>
      <label class="chk"><input type="checkbox" id="sPlanes"><span data-i18n="scrPlanes">Letadla</span></label>
      <label class="chk"><input type="checkbox" id="sMeteo"><span data-i18n="scrMeteo">Meteoradar</span></label>
      <label class="chk"><input type="checkbox" id="sForecast"><span data-i18n="scrForecast">Předpověď</span></label>
      <label class="chk"><input type="checkbox" id="sCalendar"><span data-i18n="scrCalendar">Kalendář</span></label>
    </div>
    <p class="hint" data-i18n="scrHint">Vypnuté obrazovky se přeskakují. Nastavení je dostupné vždy.</p>
    <p class="hint" data-i18n="restartHint">Změna obrazovek, zdroje radaru nebo polohy potřebuje restart, takže se ukládá až tlačítkem dole.</p>
    <div class="row"><label data-i18n="autoRotate">Automatické střídání (sekundy, 0 = vypnuto)</label>
      <input type="number" id="autoRotate" min="0" max="3600" step="5"></div>
    <p class="hint" data-i18n="rotHint">Střídání pozastaví přejetí prstem, dlouhý stisk nebo přepnutí z prohlížeče — na trojnásobek intervalu, pak pokračuje samo. Obyčejné klepnutí ho nezastaví, otevřený detail letadla ho drží. Na obrazovce Nastavení se nestřídá.</p>
  </div>

  <div class="card">
    <h2 data-i18n="radar">Meteoradar</h2>
    <div class="row"><label data-i18n="radarSrc">Zdroj dat</label>
      <select id="radarSrc">
        <option value="0" data-i18n="srcChmu">ČHMÚ (ostřejší, jen ČR)</option>
        <option value="1" data-i18n="srcRv">RainViewer (Evropa i svět)</option>
      </select></div>
    <p class="hint" data-i18n="radarHint">Mimo ČR nemá ČHMÚ data a obrazovka zůstane prázdná — použijte RainViewer.</p>
    <div class="row"><label class="chk"><input type="checkbox" id="meteoLegend"><span data-i18n="mtLegend">Zobrazit legendu (dBZ / mm/h)</span></label></div>
    <p class="hint" data-i18n="mtLegendHint">Legenda zabírá levý okraj mapy. Když stupnici znáte, dá se skrýt a je vidět víc území.</p>
  </div>
</section>

<!-- ================= VZHLED ================= -->
<section id="tLook" class="tab hide">
  <div class="card">
    <h2 data-i18n="brightness">Jas</h2>
    <div class="row"><label data-i18n="briDay">Denní jas</label><input type="range" id="briDay" min="10" max="100"><span id="briDayV"></span></div>
    <div class="row"><label data-i18n="briNight">Noční jas</label><input type="range" id="briNight" min="5" max="100"><span id="briNightV"></span></div>
    <div class="row"><label class="chk"><input type="checkbox" id="nightAuto"><span data-i18n="nightAuto">Přepínat automaticky podle slunce</span></label></div>
    <div class="row"><label data-i18n="nightOffset">Posun proti východu/západu (min)</label><input type="number" id="nightOffset" min="-120" max="120"></div>
    <p class="hint" data-i18n="liveHint">Změny na této záložce se ukládají hned, tlačítko Uložit tu nepotřebujete.</p>
  </div>

  <div class="card">
    <h2 data-i18n="clockHdr">Hodiny</h2>
    <div class="row"><label data-i18n="secStyle">Vteřinový prstenec</label>
      <select id="secStyle">
        <option value="0" data-i18n="secOff">vypnuto</option>
        <option value="1" data-i18n="secDots">tečky</option>
        <option value="2" data-i18n="secLine">čára</option>
        <option value="3" data-i18n="secComet">kometa</option>
      </select></div>
    <div class="row"><label data-i18n="clockColor">Barva hodin</label><input type="color" id="clockColor"></div>
    <div class="row"><label data-i18n="secColor">Barva vteřin</label><input type="color" id="secColor"></div>
  </div>
</section>

<!-- ================= LETADLA ================= -->
<section id="tPlanes" class="tab hide">
  <div class="card">
    <h2 data-i18n="planes">Letadla</h2>
    <div class="row"><label data-i18n="altMin">Nejnižší výška (ft)</label><input type="number" id="altMin" min="0" max="60000"></div>
    <div class="row"><label data-i18n="altMax">Nejvyšší výška (ft)</label><input type="number" id="altMax" min="0" max="60000"></div>
    <div class="row"><label class="chk"><input type="checkbox" id="onlyCallsign"><span data-i18n="onlyCs">Jen letadla s callsignem</span></label></div>
    <div class="row"><label class="chk"><input type="checkbox" id="squawkAlert"><span data-i18n="sqAlert">Upozornit na nouzové squawky (7500/7600/7700)</span></label></div>
    <div class="row"><label data-i18n="watch">Sledovaný callsign nebo ICAO</label><input type="text" id="watch" maxlength="9" placeholder="CSA1234"></div>
    <p class="hint" data-i18n="planesHint">Filtry se týkají jen kreslení. Nouzový squawk ani sledované letadlo neschovají.</p>
    <p class="hint" data-i18n="liveHint">Změny na této záložce se ukládají hned, tlačítko Uložit tu nepotřebujete.</p>
  </div>

  <div class="card">
    <h2 data-i18n="planesView">Zobrazení radaru</h2>
    <div class="row"><label data-i18n="topBearing">Nahoře na radaru</label>
      <select id="topBearing"><option value="0">S</option><option value="45">SV</option><option value="90">V</option><option value="135">JV</option><option value="180">J</option><option value="225">JZ</option><option value="270">Z</option><option value="315">SZ</option></select></div>
    <div class="row"><label class="chk"><input type="checkbox" id="metric"><span data-i18n="metric">Metrické jednotky (m, km/h)</span></label></div>
    <p class="hint" data-i18n="planesViewHint">Nastavte směr, kterým se díváte z okna. Meteoradar se záměrně neotáčí. Jednotky platí pro detail letadla.</p>
  </div>
</section>

<!-- ================= KALENDÁŘ ================= -->
<section id="tCal" class="tab hide">
 <div class="card">
  <h2 data-i18n="scrCalendar">Kalendář</h2>
  <p class="hint" data-i18n="calIntro"></p>
  <div class="row"><label for="calendarView" data-i18n="calView"></label><select id="calendarView"><option value="0" data-i18n="calDay"></option><option value="1" data-i18n="calMonth"></option></select></div>
  <div class="row"><label for="calendarRefresh" data-i18n="calRefresh"></label><input id="calendarRefresh" type="number" min="5" max="1440" value="15"></div>
  <h3><span data-i18n="calSource"></span> 1</h3>
  <label class="chk"><input type="checkbox" id="cal0On"><span data-i18n="calEnabled"></span></label>
  <div class="row"><label for="cal0Name" data-i18n="calName"></label><input type="text" id="cal0Name" maxlength="31"></div>
  <div class="row"><label for="cal0Color" data-i18n="calColor"></label><input id="cal0Color" type="color" value="#00bfff"></div>
  <div class="row"><label for="cal0Url" data-i18n="calUrl"></label><input id="cal0Url" type="password" autocomplete="off" spellcheck="false" maxlength="767"></div>
  <label class="chk"><input type="checkbox" id="cal0Clear"><span data-i18n="calClear"></span></label>
  <p class="hint" id="cal0Status"></p>
  <h3><span data-i18n="calSource"></span> 2</h3>
  <label class="chk"><input type="checkbox" id="cal1On"><span data-i18n="calEnabled"></span></label>
  <div class="row"><label for="cal1Name" data-i18n="calName"></label><input type="text" id="cal1Name" maxlength="31"></div>
  <div class="row"><label for="cal1Color" data-i18n="calColor"></label><input id="cal1Color" type="color" value="#ff9900"></div>
  <div class="row"><label for="cal1Url" data-i18n="calUrl"></label><input id="cal1Url" type="password" autocomplete="off" spellcheck="false" maxlength="767"></div>
  <label class="chk"><input type="checkbox" id="cal1Clear"><span data-i18n="calClear"></span></label>
  <p class="hint" id="cal1Status"></p>
  <h3><span data-i18n="calSource"></span> 3</h3>
  <label class="chk"><input type="checkbox" id="cal2On"><span data-i18n="calEnabled"></span></label>
  <div class="row"><label for="cal2Name" data-i18n="calName"></label><input type="text" id="cal2Name" maxlength="31"></div>
  <div class="row"><label for="cal2Color" data-i18n="calColor"></label><input id="cal2Color" type="color" value="#00ff00"></div>
  <div class="row"><label for="cal2Url" data-i18n="calUrl"></label><input id="cal2Url" type="password" autocomplete="off" spellcheck="false" maxlength="767"></div>
  <label class="chk"><input type="checkbox" id="cal2Clear"><span data-i18n="calClear"></span></label>
  <p class="hint" id="cal2Status"></p>
  <p class="hint" data-i18n="calUrlHint"></p>
  <p class="hint" data-i18n="calGoogleHint"></p>
  <p class="hint" data-i18n="calAppleHint"></p>
  <p class="hint" data-i18n="calDotsHint"></p>
  <p class="hint" data-i18n="calLimits"></p>
  <p class="hint" data-i18n="calSaveHint"></p>
  <button class="sec" type="button" onclick="refreshCalendars()" data-i18n="calReload"></button>
 </div>
</section>

<section id="tNotify" class="tab hide">
 <div class="card">
  <h2 data-i18n="ntTitle"></h2>
  <p data-i18n="ntIntro"></p>
  <label class="chk"><input type="checkbox" id="ntEnabled"><span data-i18n="ntEnabled"></span></label>
  <div class="row"><label for="ntSeconds" data-i18n="ntSeconds"></label><input type="number" id="ntSeconds" min="0" max="120" value="15"></div>
  <p class="hint" data-i18n="ntTimeoutHint"></p>
  <p class="hint" id="ntPairState"></p>
  <p class="hint" id="ntStatus"></p>
  <button type="button" class="sec" id="ntTestBtn" onclick="testNotification()" data-i18n="ntTest"></button>
 </div>
 <div class="card">
  <h2>Google Chat</h2>
  <ol>
   <li data-i18n="ntStep1"></li>
   <li data-i18n="ntStep2"></li>
   <li data-i18n="ntStep3"></li>
   <li data-i18n="ntStep4"></li>
  </ol>
  <pre style="white-space:pre-wrap;overflow-wrap:anywhere">python3 tools/google-chat-bridge/bridge.py setup --client client_secret.json --pair meteoplaneradar-chat.json
python3 tools/google-chat-bridge/bridge.py run</pre>
  <button type="button" id="ntPairBtn" onclick="pairNotifications()" data-i18n="ntPair"></button>
  <p class="hint" data-i18n="ntPairHint"></p>
  <p class="hint" data-i18n="ntLimit"></p>
 </div>
</section>

<!-- ================= SYSTÉM ================= -->
<section id="tSys" class="tab hide">
  <div class="card">
    <h2 data-i18n="system">Systém</h2>
    <div class="row"><label data-i18n="adminPass">Současné heslo</label>
      <input type="password" id="adminPass" placeholder="•••"></div>
    <div class="row"><label data-i18n="newPass">Nové heslo</label>
      <input type="password" id="newPass" placeholder="•••"></div>
    <p class="hint" id="pwState"></p>
    <p class="hint" data-i18n="passHint">Současné heslo je potřeba pro aktualizaci, import, reset i pro změnu hesla. Prázdné nové heslo nic nemění; jedna mezera ochranu zruší.</p>
    <p class="hint" data-i18n="passNote">Poznámka: výchozí stav je bez hesla — aktualizace, import i reset jsou pak přístupné komukoli v síti. Heslo se ukládá v čitelné podobě, protože aktualizační stránka používá HTTP Basic, kde otisk použít nelze; chrání tedy před domácností, ne před útočníkem. Zapomenuté heslo jde zrušit jedině podržením tlačítka BOOT při startu (~3 s), což smaže i WiFi a všechna ostatní nastavení.</p>
    <div class="row" style="margin-top:12px">
      <button class="sec" onclick="location='/update'" data-i18n="fwUpdate">Aktualizace firmwaru</button>
      <button class="sec" onclick="exportCfg()" data-i18n="export">Export nastavení</button>
      <button class="sec" onclick="document.getElementById('imp').click()" data-i18n="import">Import</button>
      <input type="file" id="imp" class="hide" accept="application/json" onchange="importCfg(this)">
      <button class="sec" onclick="doReboot()" data-i18n="reboot">Restart</button>
      <button class="danger" onclick="doReset()" data-i18n="factory">Tovární reset</button>
    </div>

    <!-- Attribution for the active data sources. -->
    <h2 style="margin-top:22px" data-i18n="srcTitle">Zdroje dat</h2>
    <p class="hint" data-i18n="srcIntro">Data pocházejí z těchto služeb. Několik z nich vyžaduje uvedení zdroje, proto jsou uvedeny zde. Podrobnosti a podmínky jsou v souboru LICENSE.txt.</p>
    <p class="hint">
      <a href="https://adsb.fi" target="_blank" rel="noopener">adsb.fi</a> ·
      <a href="https://adsb.lol" target="_blank" rel="noopener">adsb.lol</a> ·
      <a href="https://opendata.chmi.cz" target="_blank" rel="noopener">ČHMÚ</a> ·
      <a href="https://www.rainviewer.com" target="_blank" rel="noopener">RainViewer</a> ·
      <a href="https://open-meteo.com" target="_blank" rel="noopener">Open-Meteo</a> ·
      <a href="https://www.geonames.org" target="_blank" rel="noopener">GeoNames</a> (CC BY 4.0) ·
      Natural Earth ·
      <a href="http://ip-api.com" target="_blank" rel="noopener">ip-api.com</a>
    </p>
  </div>
</section>

</div>

<div class="bar hide" id="saveBar"><button onclick="save()" data-i18n="save">Uložit nastavení</button><span id="msg"></span></div>

<script>
const D={
 cs:{"scrCalendar":"Kalendář","calIntro":"Události z Google Kalendáře, iCloudu nebo jiného ICS odběru. Na displeji přepnete den a měsíc; klepnutí na datum otevře jeho události.","calView":"Výchozí / poslední pohled","calDay":"Den","calMonth":"Měsíc","calRefresh":"Obnovování (minuty, 5–1440)","calSource":"Kalendář","calEnabled":"Používat tento kalendář","calName":"Název","calColor":"Barva","calUrl":"Nový ICS odkaz (HTTPS / webcal)","calClear":"Smazat uložený odkaz","calUrlHint":"Odkazy jsou přístup k událostem: nevracejí se do stránky ani do exportu. Prázdné pole ponechá uložený odkaz. Po importu zálohy do nového zařízení je zadejte znovu.","calGoogleHint":"Google: Nastavení kalendáře → Integrovat kalendář → Tajná adresa ve formátu iCal. Není potřeba zveřejňovat kalendář.","calAppleHint":"iCloud: zapněte Veřejný kalendář a vložte jeho odkaz. Kdokoli s tímto odkazem může kalendář číst. Jde o odběr pro zobrazení; přihlášení Apple/Google ani zápis událostí zde nejsou.","calDotsHint":"Jeden aktivní kalendář: zvýrazněné dny, bez barevných teček a názvu zdroje u události. Více kalendářů: barevné tečky a názvy zdrojů.","calLimits":"Ukládá se předchozí měsíc, aktuální a dva následující; nejvýše 256 výskytů a 512 KiB na zdroj. Podporovány jsou UTC, místní čas a běžná středoevropská pásma. Nepodporované opakování nebo pásmo se zobrazí jako chyba; poslední platná data zůstanou.","calSaveHint":"Zdroje a pohled uložte tlačítkem dole. Změna sady obrazovek restartuje zařízení. Je-li nastaveno heslo správce, vyplňte jej na kartě Systém.","calReload":"Obnovit uložené kalendáře","calConfigured":"Odkaz uložen","calMissing":"Odkaz není nastaven","calMemory":"Paměť kalendáře (PSRAM)",tabCtl:"Ovládání",tabLoc:"Poloha",tabScr:"Obrazovky",tabLook:"Vzhled",tabPlanes:"Letadla",tabSys:"Systém",
  timezone:"Časové pásmo",
  wifi:"WiFi",network:"Síť",password:"Heslo",scan:"Vyhledat",connect:"Připojit",
  wifiHint:"Po uložení se zařízení připojí a přístupový bod zmizí.",
  wifiHintSta:"Změna sítě přeruší spojení s touto stránkou. Když se připojení nepovede, zařízení vytvoří vlastní síť MeteoPlaneRadar a zadáte ji znovu.",
  wifiNow:"Připojeno k síti",
  autoSaved:"Uloženo",
  liveHint:"Změny na této záložce se ukládají hned, tlačítko Uložit tu nepotřebujete.",
  restartHint:"Změna obrazovek, zdroje radaru nebo polohy potřebuje restart, takže se ukládá až tlačítkem dole.",
  remote:"Dálkové ovládání",rangeLbl:"Rozsah",statusHdr:"Stav zařízení",
  remoteHint:"Rozsah se mění jen na obrazovkách Letadla a Meteoradar. Zásah pozastaví automatické střídání.",
  location:"Poloha",findCity:"Najít město",search:"Hledat",found:"Nalezeno",lat:"Zeměpisná šířka",lon:"Zeměpisná délka",
  locHint:"Změna polohy vyžaduje restart, o který se zařízení postará samo.",
  screens:"Obrazovky",scrClock:"Hodiny",scrPlanes:"Letadla",scrMeteo:"Meteoradar",scrForecast:"Předpověď",
  board:"Deska",
  scrHint:"Vypnuté obrazovky se přeskakují. Nastavení je dostupné vždy.",autoRotate:"Automatické střídání (sekundy, 0 = vypnuto)",
  rotHint:"Střídání pozastaví přejetí prstem, dlouhý stisk nebo přepnutí z prohlížeče — na trojnásobek intervalu, pak pokračuje samo. Obyčejné klepnutí ho nezastaví, otevřený detail letadla ho drží. Na obrazovce Nastavení se nestřídá.",
  radar:"Meteoradar",radarSrc:"Zdroj dat",srcChmu:"ČHMÚ (ostřejší, jen ČR)",srcRv:"RainViewer (Evropa i svět)",
  mtLegend:"Zobrazit legendu (dBZ / mm/h)",
  mtLegendHint:"Legenda zabírá levý okraj mapy. Když stupnici znáte, dá se skrýt a je vidět víc území.",
  radarHint:"Mimo ČR nemá ČHMÚ data a obrazovka zůstane prázdná — použijte RainViewer.",
  brightness:"Jas",clockHdr:"Hodiny",
  briDay:"Denní jas",briNight:"Noční jas",nightAuto:"Přepínat automaticky podle slunce",
  nightOffset:"Posun proti východu/západu (min)",secStyle:"Vteřinový prstenec",secOff:"vypnuto",secDots:"tečky",secLine:"čára",secComet:"kometa",
  clockColor:"Barva hodin",secColor:"Barva vteřin",metric:"Metrické jednotky (m, km/h)",topBearing:"Nahoře na radaru letadel",
  planes:"Letadla",altMin:"Nejnižší výška (ft)",altMax:"Nejvyšší výška (ft)",onlyCs:"Jen letadla s callsignem",
  sqAlert:"Upozornit na nouzové squawky (7500/7600/7700)",watch:"Sledovaný callsign nebo ICAO",
  planesHint:"Filtry se týkají jen kreslení. Nouzový squawk ani sledované letadlo neschovají.",
  planesView:"Zobrazení radaru",
  planesViewHint:"Nastavte směr, kterým se díváte z okna. Meteoradar se záměrně neotáčí. Jednotky platí pro detail letadla.",
  pwNone:"Zatím není nastavené žádné heslo — aktualizace, import a reset jsou otevřené.",
  pwSet:"Heslo je nastavené.",
  passNote:"Poznámka: výchozí stav je bez hesla — aktualizace, import i reset jsou pak přístupné komukoli v síti. Heslo se ukládá v čitelné podobě, protože aktualizační stránka používá HTTP Basic, kde otisk použít nelze; chrání tedy před domácností, ne před útočníkem. Zapomenuté heslo jde zrušit jedině podržením tlačítka BOOT při startu (~3 s), což smaže i WiFi a všechna ostatní nastavení.",
  srcTitle:"Zdroje dat",
  srcIntro:"Data pocházejí z těchto služeb. Několik z nich vyžaduje uvedení zdroje, proto jsou uvedeny zde. Podrobnosti a podmínky jsou v souboru LICENSE.txt.",
  system:"Systém",adminPass:"Současné heslo",newPass:"Nové heslo",
  passHint:"Současné heslo je potřeba pro aktualizaci, import, reset i pro změnu hesla. Prázdné nové heslo nic nemění; jedna mezera ochranu zruší.",
  fwUpdate:"Aktualizace firmwaru",export:"Export nastavení",import:"Import",reboot:"Restart",factory:"Tovární reset",save:"Uložit nastavení",
  settings:"Nastavení",
  saved:"Uloženo",failed:"Nepovedlo se",searching:"Hledám…",nothing:"Nic nenalezeno",
  disabled:"Obrazovka je vypnutá",confirmReset:"Opravdu smazat všechna nastavení včetně WiFi?"},
 en:{"scrCalendar":"Calendar","calIntro":"Events from Google Calendar, iCloud or another ICS subscription. Switch day/month on the display; tap a date to open its agenda.","calView":"Default / last view","calDay":"Day","calMonth":"Month","calRefresh":"Refresh interval (minutes, 5–1440)","calSource":"Calendar","calEnabled":"Use this calendar","calName":"Name","calColor":"Colour","calUrl":"New ICS URL (HTTPS / webcal)","calClear":"Delete the saved URL","calUrlHint":"URLs grant access to events: they are never returned to the page or exported. An empty field keeps the saved URL. Re-enter URLs after importing a backup on a new device.","calGoogleHint":"Google: Calendar settings → Integrate calendar → Secret address in iCal format. The calendar does not need to be public.","calAppleHint":"iCloud: enable Public Calendar and paste its link. Anyone with that link can read the calendar. This is a display subscription; Apple/Google sign-in and editing events are not provided.","calDotsHint":"One enabled calendar: highlighted dates, no coloured dots or source name in the agenda. Multiple calendars: source dots and names.","calLimits":"Caches the previous, current and next two months; up to 256 occurrences and 512 KiB per source. Supports UTC, local time and common Central European zones. Unsupported recurrence or zones report an error; the last valid data remains.","calSaveHint":"Save sources and view with the button below. Changing enabled screens restarts the device. If an admin password is set, enter it on the System tab.","calReload":"Refresh saved calendars","calConfigured":"URL saved","calMissing":"No URL configured","calMemory":"Calendar memory (PSRAM)",timezone:"Time zone",tabCtl:"Control",tabLoc:"Location",tabScr:"Screens",tabLook:"Appearance",tabPlanes:"Aircraft",tabSys:"System",
  wifi:"WiFi",network:"Network",password:"Password",scan:"Scan",connect:"Connect",
  wifiHint:"After saving the device connects and the access point disappears.",
  wifiHintSta:"Changing the network drops this page. If the connection fails, the device brings up its own MeteoPlaneRadar network and you enter it again.",
  wifiNow:"Connected to",
  autoSaved:"Saved",
  liveHint:"Changes on this tab are saved immediately - no need for the Save button.",
  restartHint:"Changing the screens, the radar source or the location needs a restart, so those are saved with the button below.",
  remote:"Remote control",rangeLbl:"Range",statusHdr:"Device status",
  remoteHint:"The range only applies to the Aircraft and Weather screens. Using this pauses the automatic cycling.",
  location:"Location",findCity:"Find a town",search:"Search",found:"Found",lat:"Latitude",lon:"Longitude",
  locHint:"Changing the location needs a restart, which the device does by itself.",
  screens:"Screens",scrClock:"Clock",scrPlanes:"Aircraft",scrMeteo:"Weather radar",scrForecast:"Forecast",
  board:"Board",
  scrHint:"Disabled screens are skipped. Settings is always reachable.",autoRotate:"Auto cycling (seconds, 0 = off)",
  rotHint:"Cycling is paused by a swipe, a long press or a switch from the browser - for three times the interval, then it resumes on its own. A plain tap does not stop it; an open aircraft detail holds it. It does not run on the Settings screen.",
  radar:"Weather radar",radarSrc:"Data source",srcChmu:"CHMU (sharper, Czechia only)",srcRv:"RainViewer (Europe and beyond)",
  mtLegend:"Show the legend (dBZ / mm/h)",
  mtLegendHint:"The legend takes up the left edge of the map. If you know the scale, hide it and see more ground.",
  radarHint:"Outside Czechia CHMU has no data and the screen stays blank — use RainViewer.",
  brightness:"Brightness",clockHdr:"Clock",
  briDay:"Day brightness",briNight:"Night brightness",nightAuto:"Switch automatically with the sun",
  nightOffset:"Offset from sunrise/sunset (min)",secStyle:"Seconds ring",secOff:"off",secDots:"dots",secLine:"line",secComet:"comet",
  clockColor:"Clock colour",secColor:"Seconds colour",metric:"Metric units (m, km/h)",topBearing:"Top of the aircraft radar",
  planes:"Aircraft",altMin:"Lowest altitude (ft)",altMax:"Highest altitude (ft)",onlyCs:"Only aircraft with a callsign",
  sqAlert:"Alert on emergency squawks (7500/7600/7700)",watch:"Watched callsign or ICAO",
  planesHint:"Filters affect drawing only. They never hide an emergency squawk or a watched aircraft.",
  planesView:"Radar view",
  planesViewHint:"Set the direction you are looking out of the window. The weather radar deliberately does not turn. Units apply to the aircraft detail.",
  pwNone:"No password is set yet - update, import and reset are open.",
  pwSet:"A password is set.",
  passNote:"Note: the default is no password, which leaves update, import and reset open to anyone on the network. The password is stored in the clear because the update page uses HTTP Basic, where a digest cannot be used - so it protects against the household, not against an attacker. A forgotten password can only be cleared by holding BOOT at startup (~3 s), which also erases WiFi and every other setting.",
  srcTitle:"Data sources",
  srcIntro:"The data comes from the services below. Several of them require their source to be credited, which is why they are listed here. The full terms are in LICENSE.txt.",
  system:"System",adminPass:"Current password",newPass:"New password",
  passHint:"The current password is needed for updates, import, reset and to change the password itself. An empty new password changes nothing; a single space removes the protection.",
  fwUpdate:"Firmware update",export:"Export settings",import:"Import",reboot:"Reboot",factory:"Factory reset",save:"Save settings",
  settings:"Settings",
  saved:"Saved",failed:"Failed",searching:"Searching…",nothing:"Nothing found",
  disabled:"That screen is switched off",confirmReset:"Really erase all settings including WiFi?"},
 sk:{"scrCalendar":"Kalendár","calIntro":"Udalosti z Google Kalendára, iCloudu alebo iného ICS odberu. Na displeji prepnete deň a mesiac; ťuknutie na dátum otvorí jeho udalosti.","calView":"Predvolený / posledný pohľad","calDay":"Deň","calMonth":"Mesiac","calRefresh":"Obnovovanie (minúty, 5–1440)","calSource":"Kalendár","calEnabled":"Používať tento kalendár","calName":"Názov","calColor":"Farba","calUrl":"Nový ICS odkaz (HTTPS / webcal)","calClear":"Vymazať uložený odkaz","calUrlHint":"Odkazy umožňujú prístup k udalostiam: nevracajú sa do stránky ani do exportu. Prázdne pole ponechá uložený odkaz. Po importe zálohy do nového zariadenia ich zadajte znova.","calGoogleHint":"Google: Nastavenia kalendára → Integrovať kalendár → Tajná adresa vo formáte iCal. Kalendár nemusí byť verejný.","calAppleHint":"iCloud: zapnite Verejný kalendár a vložte jeho odkaz. Ktokoľvek s odkazom môže kalendár čítať. Ide o odber na zobrazenie; prihlásenie Apple/Google ani úpravy udalostí tu nie sú.","calDotsHint":"Jeden aktívny kalendár: zvýraznené dni, bez farebných bodiek a názvu zdroja pri udalosti. Viac kalendárov: farebné bodky a názvy zdrojov.","calLimits":"Ukladá sa predchádzajúci mesiac, aktuálny a dva nasledujúce; najviac 256 výskytov a 512 KiB na zdroj. Podporované sú UTC, miestny čas a bežné stredoeurópske pásma. Nepodporované opakovanie alebo pásmo sa zobrazí ako chyba; posledné platné dáta zostanú.","calSaveHint":"Zdroje a pohľad uložte tlačidlom dole. Zmena sady obrazoviek reštartuje zariadenie. Ak je nastavené heslo správcu, vyplňte ho na karte Systém.","calReload":"Obnoviť uložené kalendáre","calConfigured":"Odkaz uložený","calMissing":"Odkaz nie je nastavený","calMemory":"Pamäť kalendára (PSRAM)",
  "tabCtl": "Ovládanie",
  "tabLoc": "Poloha",
  "tabScr": "Obrazovky",
  "tabLook": "Vzhľad",
  "tabPlanes": "Lietadlá",
  "tabSys": "Systém",
  "timezone": "Časové pásmo",
  "wifi": "WiFi",
  "network": "Sieť",
  "password": "Heslo",
  "scan": "Vyhľadať",
  "connect": "Pripojiť",
  "wifiHint": "Po uložení sa zariadenie pripojí a prístupový bod zmizne.",
  "wifiHintSta": "Zmena siete preruší spojenie s touto stránkou. Ak sa pripojenie nepodarí, zariadenie vytvorí vlastnú sieť MeteoPlaneRadar a zadáte ju znova.",
  "wifiNow": "Pripojené k sieti",
  "autoSaved": "Uložené",
  "liveHint": "Zmeny na tejto karte sa ukladajú okamžite, tlačidlo Uložiť nepotrebujete.",
  "restartHint": "Zmena obrazoviek, zdroja radaru alebo polohy vyžaduje reštart, preto sa ukladá tlačidlom dole.",
  "remote": "Diaľkové ovládanie",
  "rangeLbl": "Rozsah",
  "statusHdr": "Stav zariadenia",
  "remoteHint": "Rozsah sa mení len na obrazovkách Lietadlá a Meteoradar. Zásah pozastaví automatické striedanie.",
  "location": "Poloha",
  "findCity": "Nájsť mesto",
  "search": "Hľadať",
  "found": "Nájdené",
  "lat": "Zemepisná šírka",
  "lon": "Zemepisná dĺžka",
  "locHint": "Zmena polohy vyžaduje reštart, o ktorý sa zariadenie postará samo.",
  "screens": "Obrazovky",
  "scrClock": "Hodiny",
  "scrPlanes": "Lietadlá",
  "scrMeteo": "Meteoradar",
  "scrForecast": "Predpoveď",
  "board": "Doska",
  "scrHint": "Vypnuté obrazovky sa preskakujú. Nastavenia sú dostupné vždy.",
  "autoRotate": "Automatické striedanie (sekundy, 0 = vypnuté)",
  "rotHint": "Striedanie pozastaví potiahnutie prstom, dlhé stlačenie alebo prepnutie z prehliadača na trojnásobok intervalu, potom pokračuje samo. Bežné ťuknutie ho nezastaví, otvorený detail lietadla ho pozastaví. Na obrazovke Nastavenia sa nestrieda.",
  "radar": "Meteoradar",
  "radarSrc": "Zdroj dát",
  "srcChmu": "ČHMÚ (ostrejší, len ČR)",
  "srcRv": "RainViewer (Európa aj svet)",
  "mtLegend": "Zobraziť legendu (dBZ / mm/h)",
  "mtLegendHint": "Legenda zaberá ľavý okraj mapy. Ak stupnicu poznáte, môžete ju skryť a uvidíte viac územia.",
  "radarHint": "Mimo ČR nemá ČHMÚ dáta a obrazovka zostane prázdna — použite RainViewer.",
  "brightness": "Jas",
  "clockHdr": "Hodiny",
  "briDay": "Denný jas",
  "briNight": "Nočný jas",
  "nightAuto": "Prepínať automaticky podľa slnka",
  "nightOffset": "Posun oproti východu/západu (min)",
  "secStyle": "Sekundový prstenec",
  "secOff": "vypnuté",
  "secDots": "bodky",
  "secLine": "čiara",
  "secComet": "kométa",
  "clockColor": "Farba hodín",
  "secColor": "Farba sekúnd",
  "metric": "Metrické jednotky (m, km/h)",
  "topBearing": "Hore na radare lietadiel",
  "planes": "Lietadlá",
  "altMin": "Najnižšia výška (ft)",
  "altMax": "Najvyššia výška (ft)",
  "onlyCs": "Len lietadlá s volacím znakom",
  "sqAlert": "Upozorniť na núdzové squawky (7500/7600/7700)",
  "watch": "Sledovaný volací znak alebo ICAO",
  "planesHint": "Filtre ovplyvňujú len vykreslenie. Núdzový squawk ani sledované lietadlo neskryjú.",
  "planesView": "Zobrazenie radaru",
  "planesViewHint": "Nastavte smer, ktorým sa pozeráte z okna. Meteoradar sa zámerne neotáča. Jednotky platia pre detail lietadla.",
  "pwNone": "Zatiaľ nie je nastavené žiadne heslo — aktualizácia, import a reset sú prístupné.",
  "pwSet": "Heslo je nastavené.",
  "passNote": "Poznámka: predvolený stav je bez hesla — aktualizácia, import aj reset sú potom prístupné komukoľvek v sieti. Heslo sa ukladá v čitateľnej podobe, pretože aktualizačná stránka používa HTTP Basic, kde odtlačok použiť nemožno; chráni teda pred domácnosťou, nie pred útočníkom. Zabudnuté heslo možno zrušiť iba podržaním tlačidla BOOT pri štarte (~3 s), čo vymaže aj WiFi a všetky ostatné nastavenia.",
  "srcTitle": "Zdroje dát",
  "srcIntro": "Dáta pochádzajú z týchto služieb. Niekoľko z nich vyžaduje uvedenie zdroja, preto sú uvedené tu. Podrobnosti a podmienky sú v súbore LICENSE.txt.",
  "system": "Systém",
  "adminPass": "Súčasné heslo",
  "newPass": "Nové heslo",
  "passHint": "Súčasné heslo je potrebné na aktualizáciu, import, reset aj zmenu hesla. Prázdne nové heslo nič nemení; jedna medzera ochranu zruší.",
  "fwUpdate": "Aktualizácia firmvéru",
  "export": "Export nastavení",
  "reboot": "Reštart",
  "factory": "Obnovenie výrobných nastavení",
  "save": "Uložiť nastavenia",
  "settings": "Nastavenia",
  "saved": "Uložené",
  "failed": "Nepodarilo sa",
  "searching": "Hľadám…",
  "nothing": "Nič sa nenašlo",
  "disabled": "Obrazovka je vypnutá",
  "confirmReset": "Naozaj vymazať všetky nastavenia vrátane WiFi?",
  "import": "Import"
}};
Object.assign(D.en,{"ntTitle": "Notifications", "ntIntro": "Compact banner above every screen. Tap for details; the large X dismisses the message only on this display.", "ntEnabled": "Enable notifications", "ntSeconds": "Banner duration (seconds)", "ntTimeoutHint": "0 = until closed, otherwise 5–120 s. After timeout, messages stay behind the count badge. Details stay open until dismissed.", "ntTest": "Save and show a test", "ntPair": "Pair bridge / download configuration", "ntPairHint": "Enables notifications and rotates the pairing key. Previous bridges stop working. Keep the downloaded file private; it is sent over your home LAN. If set, enter the current admin password on System first.", "ntStep1": "Run the included Python 3 bridge on a computer or Raspberry Pi in the same LAN. It must stay running. No Python packages are needed.", "ntStep2": "In Google Cloud enable Google Chat API, configure the OAuth consent screen (add your account as a test user), create an OAuth client of type Desktop app and download its JSON as client_secret.json.", "ntStep3": "Download pairing configuration below. In the repository folder run the setup command. A browser opens for Google sign-in; select Chat spaces in the terminal.", "ntStep4": "Run the second command and leave it running. Then use the test button here. Save changes with the button below.", "ntLimit": "Google documents this API for Workspace accounts; availability depends on your account and administrator. Incoming Chat webhooks cannot receive messages. The bridge reads new text and attachment notices, not message history or edits. Up to 6 pending messages, 1024 UTF-8 bytes each; closing does not mark Chat messages read. Delivery may wait during radar downloads. Pairing keys and Google tokens are excluded from backups.", "ntPaired": "Bridge paired", "ntUnpaired": "Bridge not paired", "ntBridge": "Bridge contact (s ago; −1 = none)", "ntQueue": "Pending", "ntMemory": "Notification PSRAM"});
Object.assign(D.sk,{"ntTitle": "Notifikácie", "ntIntro": "Kompaktný banner nad každou obrazovkou. Ťuknutie otvorí detail; veľký krížik zavrie správu iba na displeji.", "ntEnabled": "Zapnúť notifikácie", "ntSeconds": "Trvanie bannera (sekundy)", "ntTimeoutHint": "0 = do zatvorenia, inak 5–120 s. Po uplynutí času zostanú správy pod odznakom s počtom. Detail zostáva otvorený do zatvorenia.", "ntTest": "Uložiť a zobraziť skúšku", "ntPair": "Spárovať most / stiahnuť konfiguráciu", "ntPairHint": "Zapne notifikácie a vymení párovací kľúč. Predchádzajúci most prestane fungovať. Stiahnutý súbor uchovajte súkromne; prenáša sa cez domácu LAN. Ak máte heslo správcu, najprv ho zadajte na karte Systém.", "ntStep1": "Priložený most pre Python 3 spustite na počítači alebo Raspberry Pi v rovnakej LAN. Musí zostať spustený. Netreba inštalovať balíčky Pythonu.", "ntStep2": "V Google Cloud zapnite Google Chat API, nastavte súhlas OAuth (pridajte svoj účet medzi testovacích používateľov), vytvorte OAuth klienta typu Desktop app a stiahnite JSON ako client_secret.json.", "ntStep3": "Nižšie stiahnite párovaciu konfiguráciu. V priečinku repozitára spustite príkaz setup. Otvorí sa prehliadač na prihlásenie ku Googlu; v termináli vyberiete priestory Chatu.", "ntStep4": "Spustite druhý príkaz a nechajte ho bežať. Potom tu použite skúšobné tlačidlo. Zmeny uložte tlačidlom dole.", "ntLimit": "Google dokumentuje toto API pre účty Workspace; dostupnosť závisí od účtu a správcu. Prichádzajúci webhook Chatu nedokáže prijímať správy. Most číta nové texty a oznámenia o prílohách, nie históriu ani úpravy. Najviac 6 čakajúcich správ, každá 1024 bajtov UTF-8; zatvorenie neoznačí správu v Chate ako prečítanú. Počas sťahovania radaru môže doručenie chvíľu čakať. Párovacie kľúče a tokeny Googlu nie sú v zálohách.", "ntPaired": "Most spárovaný", "ntUnpaired": "Most ešte nie je spárovaný", "ntBridge": "Kontakt mosta (pred s; −1 = žiadny)", "ntQueue": "Čakajúce správy", "ntMemory": "PSRAM notifikácií"});
Object.assign(D.cs,{"ntTitle": "Notifikace", "ntIntro": "Kompaktní banner nad každou obrazovkou. Klepnutí otevře detail; velký křížek zavře zprávu pouze na displeji.", "ntEnabled": "Zapnout notifikace", "ntSeconds": "Trvání banneru (sekundy)", "ntTimeoutHint": "0 = do zavření, jinak 5–120 s. Po uplynutí času zůstanou zprávy pod odznakem s počtem. Detail zůstává otevřený do zavření.", "ntTest": "Uložit a zobrazit test", "ntPair": "Spárovat most / stáhnout konfiguraci", "ntPairHint": "Zapne notifikace a vymění párovací klíč. Předchozí most přestane fungovat. Stažený soubor uchovejte soukromě; přenáší se přes domácí LAN. Máte-li heslo správce, nejprve jej vyplňte na kartě Systém.", "ntStep1": "Přiložený most pro Python 3 spusťte na počítači nebo Raspberry Pi ve stejné LAN. Musí zůstat spuštěný. Není třeba instalovat balíčky Pythonu.", "ntStep2": "V Google Cloud zapněte Google Chat API, nastavte souhlas OAuth (přidejte svůj účet mezi testovací uživatele), vytvořte OAuth klienta typu Desktop app a stáhněte JSON jako client_secret.json.", "ntStep3": "Níže stáhněte párovací konfiguraci. Ve složce repozitáře spusťte příkaz setup. Otevře se prohlížeč pro přihlášení ke Googlu; v terminálu vyberete prostory Chatu.", "ntStep4": "Spusťte druhý příkaz a nechte jej běžet. Potom zde použijte testovací tlačítko. Změny uložte tlačítkem dole.", "ntLimit": "Google dokumentuje toto API pro účty Workspace; dostupnost závisí na účtu a správci. Příchozí webhook Chatu neumí přijímat zprávy. Most čte nové texty a oznámení o přílohách, ne historii ani úpravy. Nejvýše 6 čekajících zpráv, každá 1024 bajtů UTF-8; zavření neoznačí zprávu v Chatu jako přečtenou. Během stahování radaru může doručení chvíli čekat. Párovací klíče a tokeny Googlu nejsou v zálohách.", "ntPaired": "Most spárovaný", "ntUnpaired": "Most ještě není spárovaný", "ntBridge": "Kontakt mostu (před s; −1 = žádný)", "ntQueue": "Čekající zprávy", "ntMemory": "PSRAM notifikací"});
let L="cs", CFG={}, TAB="tCtl";
const $=id=>document.getElementById(id);

function setLang(v){L=(v==2||v=="sk")?"sk":(v==1||v=="en")?"en":"cs";$("uiLang").value=String({cs:0,en:1,sk:2}[L]);
 document.documentElement.lang=L;
 document.querySelectorAll("[data-i18n]").forEach(e=>{const t=D[L][e.dataset.i18n];if(t)e.textContent=t;});
 pwState();calendarLabels();notificationLabels();status();}

// Tabs. The Save bar is hidden on Control - nothing there is saved, it is sent
// straight away - so the button cannot look like it applies to the buttons.
function showTab(id){
 TAB=id;
 document.querySelectorAll("section.tab").forEach(s=>s.classList.toggle("hide",s.id!=id));
 document.querySelectorAll("#tabs button").forEach(b=>b.classList.toggle("on",b.dataset.tab==id));
 $("saveBar").classList.toggle("hide",id=="tCtl");
 window.scrollTo(0,0);
}
document.querySelectorAll("#tabs button").forEach(b=>b.onclick=()=>showTab(b.dataset.tab));

function msg(t,c){$("msg").textContent=t;$("msg").className=c||"";setTimeout(()=>{$("msg").textContent=""},4000);}
function rgb565ToHex(v){const r=(v>>11&31)*255/31|0,g=(v>>5&63)*255/63|0,b=(v&31)*255/31|0;
 return "#"+[r,g,b].map(x=>x.toString(16).padStart(2,"0")).join("");}
function hexToRgb565(h){const r=parseInt(h.substr(1,2),16),g=parseInt(h.substr(3,2),16),b=parseInt(h.substr(5,2),16);
 return ((r>>3)<<11)|((g>>2)<<5)|(b>>3);}

const SCR=[["scrClock",0],["scrPlanes",1],["scrMeteo",2],["scrForecast",3],
           ["scrCalendar",4],["settings",5]];
function drawScrBtns(cur,enabled){
 $("scrBtns").innerHTML=SCR.map(([k,i])=>{
  const on=enabled?enabled[i]:true;
  const cls=(i==cur)?"":"sec";
  const dis=on?"":" disabled style='opacity:.4'";
  return "<button class='"+cls+"'"+dis+" onclick='goScreen("+i+")'>"+D[L][k]+"</button>";
 }).join(" ");
}
async function post(u,b){try{const r=await fetch(u,{method:"POST",headers:{"Content-Type":"application/json"},
 body:JSON.stringify(b)});
 if(r.status==409)msg(D[L].disabled,"warn");else if(!r.ok)msg(D[L].failed,"err");
 await status();}catch(e){msg(D[L].failed,"err")}}
function goScreen(i){post("/api/screen",{index:i});}
function stepScreen(d){post("/api/screen",{step:d});}
function stepRange(d){post("/api/range",{step:d});}

// --- Okamzite ukladani ------------------------------------------------------
// The API applies only the keys it actually receives, so a single field can be
// sent on its own without disturbing anything else.
//
// Deliberately NOT every field: the location, the set of screens and the radar
// source make the device restart, and auto-saving those while someone is still
// typing coordinates would reboot it mid-keystroke. Those keep the Save button.
let saveTimer = {};
function autoSave(key, val){
 clearTimeout(saveTimer[key]);
 saveTimer[key] = setTimeout(async () => {
  const o = {}; o[key] = val;
  try{
   const r = await fetch("/api/config",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(o)});
   msg(r.ok ? D[L].autoSaved : D[L].failed, r.ok ? "ok" : "err");
  }catch(e){ msg(D[L].failed,"err"); }
 }, 350);
}

// [id, event, JSON key, how to read the value]
const AUTO = [
 ["uiLang","change","lang",e=>+e.value],
 ["metric","change","metric",e=>e.checked],
 ["topBearing","change","topBearing",e=>+e.value],
 ["briDay","input","briDay",e=>+e.value],
 ["briNight","input","briNight",e=>+e.value],
 ["nightAuto","change","nightAuto",e=>e.checked],
 ["nightOffset","change","nightOffset",e=>+e.value],
 ["secStyle","change","secStyle",e=>+e.value],
 ["clockColor","change","clockColor",e=>hexToRgb565(e.value)],
 ["secColor","change","secColor",e=>hexToRgb565(e.value)],
 ["altMin","change","altMin",e=>+e.value],
 ["altMax","change","altMax",e=>+e.value],
 ["onlyCallsign","change","onlyCallsign",e=>e.checked],
 ["squawkAlert","change","squawkAlert",e=>e.checked],
 ["meteoLegend","change","meteoLegend",e=>e.checked],
 ["watch","change","watch",e=>e.value],
 ["autoRotate","change","autoRotate",e=>+e.value],
  // on the next redraw and do not need the Save button or a restart.
];
function wireAutoSave(){
 AUTO.forEach(([id,ev,key,get])=>{
  const el=$(id); if(!el) return;
  el.addEventListener(ev,()=>autoSave(key,get(el)));
 });
}

async function load(){
 const r=await fetch("/api/config");CFG=await r.json();
 setLang(CFG.lang);
 $("ver").textContent="v"+CFG.version;
 // The WiFi card is shown in BOTH modes. Hiding it once connected meant that
 // swapping a router forced you back to the touchscreen, or to a factory reset.
 $("cardWifi").classList.remove("hide");
 if(CFG.apMode){$("cardRemote").classList.add("hide");scan();}
 $("wifiHintTxt").textContent=CFG.apMode?D[L].wifiHint:D[L].wifiHintSta;
 $("lat").value=CFG.lat.toFixed(4);$("lon").value=CFG.lon.toFixed(4);
 $("sClock").checked=CFG.screens.clock;$("sPlanes").checked=CFG.screens.planes;
 $("sMeteo").checked=CFG.screens.meteo;$("sForecast").checked=CFG.screens.forecast;
 $("sCalendar").checked=!!CFG.screens.calendar;
 $("ntEnabled").checked=!!(CFG.notifications||{}).enabled;
 $("ntSeconds").value=(CFG.notifications||{}).seconds??15;
 notificationLabels();
 $("ntTestBtn").disabled=$("ntPairBtn").disabled=!!CFG.apMode;
 $("calendarView").value=CFG.calendarView||0;$("calendarRefresh").value=CFG.calendarRefresh||15;
 for(let i=0;i<3;i++){
  const c=(CFG.calendars||[])[i]||{};
  $("cal"+i+"On").checked=c.enabled!==false;
  $("cal"+i+"Name").value=c.name||("Calendar "+(i+1));
  $("cal"+i+"Color").value=rgb565ToHex(c.color??[1535,64800,2016][i]);
  $("cal"+i+"Url").value="";$("cal"+i+"Clear").checked=false;
 }
 calendarLabels();
 $("autoRotate").value=CFG.autoRotate;$("radarSrc").value=CFG.radarSrc;
 $("meteoLegend").checked=CFG.meteoLegend;
 $("briDay").value=CFG.briDay;$("briNight").value=CFG.briNight;
 $("nightAuto").checked=CFG.nightAuto;$("nightOffset").value=CFG.nightOffset;
 $("secStyle").value=CFG.secStyle;$("metric").checked=CFG.metric;$("topBearing").value=CFG.topBearing;
 $("clockColor").value=rgb565ToHex(CFG.clockColor);$("secColor").value=rgb565ToHex(CFG.secColor);
 $("altMin").value=CFG.altMin;$("altMax").value=CFG.altMax;
 pwState();
 $("onlyCallsign").checked=CFG.onlyCallsign;$("squawkAlert").checked=CFG.squawkAlert;$("watch").value=CFG.watch||"";
 bri();wireAutoSave();status();
}
// Says whether a password is actually set - more use than a static sentence,
// and it is the first thing to check when /update stops asking for one.
function pwState(){
 if(!$("pwState"))return;
 const on=!!CFG.hasPassword;
 $("pwState").textContent=on?D[L].pwSet:D[L].pwNone;
 $("pwState").className="hint "+(on?"ok":"warn");
}
function bri(){$("briDayV").textContent=$("briDay").value+"%";$("briNightV").textContent=$("briNight").value+"%";}
$("briDay").addEventListener("input",bri);$("briNight").addEventListener("input",bri);

async function status(){
 try{const s=await(await fetch("/api/status")).json();
 drawScrBtns(s.screen,s.enabled);
 if(!CFG.apMode&&$("wifiNow")) $("wifiNow").textContent=D[L].wifiNow+" "+s.ssid+".";
 const hasR=(s.range&&s.range.length>0);
 $("rangeNow").textContent=hasR?s.range:"–";
 $("rMinus").disabled=!hasR;$("rPlus").disabled=!hasR;
 $("rMinus").style.opacity=$("rPlus").style.opacity=hasR?"1":".4";
 const rows=[["IP",s.ip],["WiFi",s.ssid+" ("+s.rssi+" dBm)"],["Uptime",s.uptime],
  ["Heap",s.heap+" B"],["PSRAM",s.psram+" B"],["Restart",s.resetReason],
  ["ADS-B",s.adsb],["Radar",s.radar],[D[L].scrForecast,s.forecast],
  [D[L].timezone,s.timezone],[D[L].calMemory,(s.calendarMemory||0)+" B"],
  ["Firmware","v"+s.version],[D[L].board,s.board]];
 $("ntStatus").textContent=D[L].ntQueue+": "+(s.notificationCount||0)+" · "+D[L].ntBridge+": "+(s.notificationBridgeAge??-1)+" · "+D[L].ntMemory+": "+(s.notificationMemory||0)+" B";
 (s.calendarSources||[]).forEach((c,i)=>{if(i<3)$("cal"+i+"Status").textContent=c.state+(c.updated?" · "+new Date(c.updated*1000).toLocaleString(L):"");});
 $("statusTab").innerHTML=rows.map(r=>"<tr><td>"+r[0]+"</td><td>"+r[1]+"</td></tr>").join("");}catch(e){}
}
setInterval(status,10000);

function body(){return{lat:parseFloat($("lat").value),lon:parseFloat($("lon").value),
 lang:parseInt($("uiLang").value),metric:$("metric").checked,
 briDay:+$("briDay").value,briNight:+$("briNight").value,nightAuto:$("nightAuto").checked,
 nightOffset:+$("nightOffset").value,radarSrc:+$("radarSrc").value,autoRotate:+$("autoRotate").value,
 meteoLegend:$("meteoLegend").checked,
 topBearing:+$("topBearing").value,secStyle:+$("secStyle").value,
 clockColor:hexToRgb565($("clockColor").value),secColor:hexToRgb565($("secColor").value),
 altMin:+$("altMin").value,altMax:+$("altMax").value,onlyCallsign:$("onlyCallsign").checked,
 squawkAlert:$("squawkAlert").checked,watch:$("watch").value,
 password:$("adminPass").value,newPassword:$("newPass").value,
 notifications:{enabled:$("ntEnabled").checked,seconds:+$("ntSeconds").value},
 calendarView:+$("calendarView").value,calendarRefresh:+$("calendarRefresh").value,
 calendars:[0,1,2].map(i=>({name:$("cal"+i+"Name").value,enabled:$("cal"+i+"On").checked,color:hexToRgb565($("cal"+i+"Color").value),url:$("cal"+i+"Url").value.trim(),clearUrl:$("cal"+i+"Clear").checked})),
 screens:{clock:$("sClock").checked,planes:$("sPlanes").checked,meteo:$("sMeteo").checked,
  forecast:$("sForecast").checked,calendar:$("sCalendar").checked}};}

function calendarLabels(){
 for(let i=0;i<3;i++){
  const c=(CFG.calendars||[])[i]||{};
  $("cal"+i+"Url").placeholder=c.hasUrl?D[L].calConfigured:D[L].calMissing;
 }
}
async function refreshCalendars(){
 const r=await fetch("/api/calendar/refresh",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({password:$("adminPass").value})});
 msg(r.ok?D[L].autoSaved:D[L].failed,r.ok?"ok":"err");
}
function notificationLabels(){
 $("ntPairState").textContent=(CFG.notifications||{}).paired?D[L].ntPaired:D[L].ntUnpaired;
}
async function saveNotificationSettings(key){
 const seconds=+$("ntSeconds").value;
 if(!Number.isInteger(seconds)||(seconds!==0&&(seconds<5||seconds>120)))throw Error("5–120 / 0");
 const notifications={enabled:$("ntEnabled").checked,seconds};
 if(key){notifications.key=key;notifications.enabled=true;}
 const r=await fetch("/api/config",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({notifications,password:$("adminPass").value})});
 if(!r.ok)throw Error(r.status);
 $("ntEnabled").checked=notifications.enabled;
 return notifications;
}
async function testNotification(){
 try{$("ntEnabled").checked=true;await saveNotificationSettings();const r=await fetch("/api/notifications/test",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({password:$("adminPass").value})});
 if(!r.ok)throw Error(r.status);msg(D[L].autoSaved,"ok");await status();}catch(e){msg(D[L].failed+" ("+e.message+")","err");}
}
async function pairNotifications(){
 try{
 const bytes=new Uint8Array(32);crypto.getRandomValues(bytes);
 const key=Array.from(bytes,b=>b.toString(16).padStart(2,"0")).join("");
 await saveNotificationSettings(key);
 const url=new URL("/api/notifications",location.href).href;
 const blob=new Blob([JSON.stringify({device:url,key},null,2)],{type:"application/json"});
 const a=document.createElement("a");a.href=URL.createObjectURL(blob);a.download="meteoplaneradar-chat.json";a.click();setTimeout(()=>URL.revokeObjectURL(a.href),1000);
 CFG.notifications={enabled:true,seconds:+$("ntSeconds").value,paired:true};notificationLabels();msg(D[L].autoSaved,"ok");
 }catch(e){msg(D[L].failed+" ("+e.message+")","err");}
}
async function save(){
 try{
  const payload=body();
  const r=await fetch("/api/config",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(payload)});
  const result=await r.json();
  if(!r.ok){msg(D[L].failed+" ("+r.status+")","err");return;}
  CFG.calendars=payload.calendars.map((c,i)=>({name:c.name,color:c.color,enabled:c.enabled,
   hasUrl:!!c.url||(!c.clearUrl&&!!((CFG.calendars||[])[i]||{}).hasUrl)}));
  for(let i=0;i<3;i++){$("cal"+i+"Url").value="";$("cal"+i+"Clear").checked=false;}
  $("newPass").value="";
  setLang($("uiLang").value);
  msg(D[L].saved,"ok");
  // A restart is already queued after screen/location changes. Do not request
  // another config while the device is shutting down its web server.
  if(!result.restart)await load();
 }catch(e){msg(D[L].failed,"err");}
}
async function scan(){
 const s=$("ssid");s.innerHTML="<option>…</option>";
 try{const l=await(await fetch("/api/scan")).json();
 s.innerHTML=l.map(n=>"<option value='"+n.ssid+"'>"+n.ssid+" ("+n.rssi+")</option>").join("");}catch(e){s.innerHTML=""}
}
async function saveWifi(){
 const r=await fetch("/api/wifi",{method:"POST",headers:{"Content-Type":"application/json"},
  body:JSON.stringify({ssid:$("ssid").value,pass:$("wpass").value})});
 msg(r.ok?D[L].saved:D[L].failed,r.ok?"ok":"err");
}
async function geo(){
 const q=$("q").value.trim();if(!q)return;msg(D[L].searching);
 $("geoRow").classList.add("hide");$("geoSel").innerHTML="";
 try{const l=await(await fetch("/api/geocode?q="+encodeURIComponent(q))).json();
 if(!l.length){$("geoRow").classList.add("hide");$("geoSel").innerHTML="";msg(D[L].nothing,"warn");return;}
 $("geoRow").classList.remove("hide");
 $("geoSel").innerHTML=l.map(c=>"<option value='"+c.lat+","+c.lon+"'>"+c.name+(c.country?", "+c.country:"")+"</option>").join("");
 pickCity();msg("");}catch(e){$("geoRow").classList.add("hide");msg(D[L].failed,"err")}
}
function pickCity(){const v=$("geoSel").value.split(",");$("lat").value=(+v[0]).toFixed(4);$("lon").value=(+v[1]).toFixed(4);}
function exportCfg(){location="/api/export";}
async function importCfg(el){
 const f=el.files[0];if(!f)return;const t=await f.text();
 const r=await fetch("/api/import",{method:"POST",headers:{"Content-Type":"application/json"},
  body:JSON.stringify({password:$("adminPass").value,config:JSON.parse(t)})});
 msg(r.ok?D[L].saved:D[L].failed,r.ok?"ok":"err");if(r.ok)load();
}
async function doReboot(){await fetch("/api/reboot",{method:"POST"});msg("…");}
async function doReset(){
 if(!confirm(D[L].confirmReset))return;
 const r=await fetch("/api/reset",{method:"POST",headers:{"Content-Type":"application/json"},
  body:JSON.stringify({password:$("adminPass").value})});
 msg(r.ok?D[L].saved:D[L].failed,r.ok?"ok":"err");
}
load();
</script></body></html>)rawliteral";
