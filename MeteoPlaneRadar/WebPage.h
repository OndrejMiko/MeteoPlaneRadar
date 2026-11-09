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
:root{--bg:#11161d;--card:#1a222c;--line:#303c49;--fg:#edf2f7;--mut:#a0adbc;--acc:#49c6e8;--ok:#68d49a;--warn:#ffc36b;--err:#ff8590;--header:88px}
*{box-sizing:border-box}
body{margin:0;padding-top:var(--header);background:var(--bg);color:var(--fg);font:15px/1.5 system-ui,-apple-system,Segoe UI,Roboto,sans-serif;-webkit-text-size-adjust:100%}
header{position:fixed;inset:0 0 auto;background:var(--bg);border-bottom:1px solid var(--line);z-index:10}
.header-inner{max-width:1080px;height:var(--header);margin:auto;padding:16px 24px;display:grid;grid-template-columns:1fr auto auto;align-items:center;gap:24px}
.brand{display:flex;align-items:baseline;gap:10px;flex-wrap:wrap}
h1{font-size:20px;letter-spacing:-.4px;margin:0}.ver{color:var(--mut);font-size:12px}
#uiLang{font-size:13px;min-width:128px}.bar{display:flex;justify-content:flex-end;align-items:center;min-width:176px}
#saveButton{min-height:44px}.save-mode{color:var(--mut);font-size:13px;text-align:right;max-width:190px}
nav.tabs{display:flex;gap:24px;overflow-x:auto;padding:0 max(24px,calc((100% - 1032px)/2));border-bottom:1px solid var(--line);position:sticky;top:var(--header);background:var(--bg);z-index:9;scrollbar-width:thin}
nav.tabs button{flex:none;background:transparent;color:var(--mut);border:0;border-bottom:3px solid transparent;border-radius:0;padding:17px 0 14px;white-space:nowrap;font-weight:500;font-size:14px;cursor:pointer}
nav.tabs button:hover{color:var(--fg)}nav.tabs button.on{color:var(--acc);border-bottom-color:var(--acc)}
.wrap{max-width:1080px;margin:0 auto;padding:32px 24px 64px}
.card{background:var(--card);border:1px solid var(--line);border-radius:12px;padding:28px;margin-bottom:24px}
.card h2{font-size:19px;letter-spacing:-.2px;margin:0 0 24px;color:var(--fg)}
h3{font-size:15px;margin:0 0 16px}.row{display:flex;align-items:center;gap:16px;margin:16px 0;flex-wrap:wrap}
.row label{flex:1 1 260px;min-width:0}.row>input:not([type=checkbox]):not([type=color]):not([type=range]),.row>select{width:340px;max-width:100%;flex:0 1 340px}
input[type=text],input[type=password],input[type=number],select{background:#111820;color:var(--fg);border:1px solid #435365;border-radius:7px;padding:10px 12px;min-width:0;min-height:44px;font:inherit}
input[type=color]{background:#111820;border:1px solid #435365;border-radius:7px;height:44px;width:64px;padding:4px}
input[type=range]{flex:0 1 296px;accent-color:var(--acc)}.row>input[type=range]+span{width:28px;text-align:right}.row>input[type=color]{margin-right:276px}input[type=checkbox]{width:20px;height:20px;flex:none;accent-color:var(--acc);margin:0}
button{background:var(--acc);color:#08202a;border:1px solid transparent;border-radius:7px;padding:10px 16px;font-weight:600;cursor:pointer;font-size:14px;min-height:44px}
button.sec{background:#293541;border-color:#435365;color:var(--fg)}button.danger{background:transparent;border-color:var(--err);color:var(--err)}button:disabled{cursor:default;opacity:.5}
.notification-actions{margin-top:24px;padding-top:20px;border-top:1px solid var(--line)}.notification-actions button{min-height:48px;padding:12px 18px}
.pair-code{display:block;font:600 44px/1.4 ui-monospace,monospace;letter-spacing:6px;color:var(--acc);margin:16px 0}summary{cursor:pointer;color:var(--mut);padding:8px 0}details[open]>summary{margin-bottom:16px}
:focus-visible{outline:2px solid var(--acc);outline-offset:4px}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:12px}
.chk{display:flex;align-items:center;gap:12px;min-height:44px}.grid .chk{background:#111820;padding:12px;border:1px solid var(--line);border-radius:7px}
.hint{color:var(--mut);font-size:13px;margin:8px 0 0;max-width:76ch}.card>.hint:first-of-type{margin-bottom:20px}
.source{min-width:0;margin:28px 0;padding:20px;border:1px solid var(--line);border-radius:8px}.source legend{padding:0 8px;font-weight:600}.source>.chk{margin-top:4px}
.help{margin-top:28px;padding-top:24px;border-top:1px solid var(--line)}.help .hint{margin:12px 0}.actions{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:12px;padding-top:24px;margin-top:28px;border-top:1px solid var(--line)}.actions .danger{grid-column:1/-1;justify-self:start;margin-top:12px}
#msg{position:fixed;z-index:11;right:max(24px,calc((100% - 1032px)/2));top:calc(var(--header) + 64px);max-width:min(440px,calc(100vw - 32px));padding:12px 16px;border:1px solid var(--line);border-radius:8px;background:#202c38;box-shadow:0 8px 30px #0006;font-size:14px;overflow-wrap:anywhere}
#msg:empty{display:none}.ok{color:var(--ok)}.err{color:var(--err)}.warn{color:var(--warn)}
table{width:100%;border-collapse:collapse;font-size:14px}td{padding:12px 0;border-bottom:1px solid var(--line);overflow-wrap:anywhere}td:first-child{color:var(--mut);width:48%;padding-right:20px}tr:last-child td{border:0}
a{color:var(--acc)}pre{background:#111820;border:1px solid var(--line);border-radius:7px;padding:16px;font-size:12px}li+li{margin-top:12px}
.hide{display:none!important}#scrBtns{gap:8px;margin-bottom:24px}#scrBtns button{flex:1 1 120px}#rangeNow{font-variant-numeric:tabular-nums}.step-group{display:flex;align-items:center;gap:12px}.remote-steps{justify-content:space-between;border-top:1px solid var(--line);padding-top:20px}
@media(max-width:680px){
 :root{--header:124px}.header-inner{padding:12px 16px;grid-template-columns:1fr auto;gap:8px 12px}.brand{grid-column:1/-1;gap:8px}h1{font-size:18px}#uiLang{width:132px}.bar{min-width:0}#saveButton{font-size:13px;padding:10px 12px}.save-mode{font-size:12px;max-width:175px}
 nav.tabs{gap:24px;padding:0 16px}.wrap{padding:24px 16px 40px}.card{padding:20px;margin-bottom:20px}.card h2{font-size:18px;margin-bottom:20px}
 .row{gap:8px;margin:20px 0}.row label{flex:1 1 100%}.row>input:not([type=checkbox]):not([type=color]):not([type=range]),.row>select{width:100%;flex-basis:100%}.row input[type=range]{flex:1 1 70%}.row>input[type=color]{margin-right:0}.source{padding:16px;margin:24px 0}.grid{grid-template-columns:1fr 1fr;gap:8px}.grid .chk{font-size:13px;padding:10px;gap:8px}
 .notification-actions button{width:100%}
 #msg{right:16px}.remote-steps{gap:10px}td:first-child{width:46%}
}
</style></head><body>
<header><div class="header-inner">
  <div class="brand"><h1>MeteoPlaneRadar</h1><span class="ver" id="ver"></span></div>
  <select id="uiLang" aria-label="Language" onchange="setLang(this.value)"><option value="0">Čeština</option><option value="1">English</option><option value="2">Slovenčina</option></select>
  <div class="bar" id="saveBar">
    <span class="save-mode hide" id="saveMode" data-i18n="autoMode"></span>
    <button type="button" id="saveButton" class="hide" onclick="commitSettings()" data-i18n="save">Uložit nastavení</button>
  </div>
</div></header>
<div id="msg" role="status" aria-live="polite" aria-atomic="true"></div>

<nav class="tabs" id="tabs" role="tablist" aria-label="MeteoPlaneRadar">
  <button type="button" role="tab" id="tab-tCtl" aria-controls="tCtl" aria-selected="true" tabindex="0" data-tab="tCtl"    class="on" data-i18n="tabCtl">Ovládání</button>
  <button type="button" role="tab" id="tab-tLoc" aria-controls="tLoc" aria-selected="false" tabindex="-1" data-tab="tLoc"    data-i18n="tabLoc">Poloha</button>
  <button type="button" role="tab" id="tab-tScr" aria-controls="tScr" aria-selected="false" tabindex="-1" data-tab="tScr"    data-i18n="tabScr">Obrazovky</button>
  <button type="button" role="tab" id="tab-tLook" aria-controls="tLook" aria-selected="false" tabindex="-1" data-tab="tLook"   data-i18n="tabLook">Vzhled</button>
  <button type="button" role="tab" id="tab-tPlanes" aria-controls="tPlanes" aria-selected="false" tabindex="-1" data-tab="tPlanes" data-i18n="tabPlanes">Letadla</button>
  <button type="button" role="tab" id="tab-tCal" aria-controls="tCal" aria-selected="false" tabindex="-1" data-tab="tCal" data-i18n="scrCalendar">Kalendář</button>
  <button type="button" role="tab" id="tab-tNotify" aria-controls="tNotify" aria-selected="false" tabindex="-1" data-tab="tNotify" data-i18n="ntTitle">Notifikácie</button>
  <button type="button" role="tab" id="tab-tHa" aria-controls="tHa" aria-selected="false" tabindex="-1" data-tab="tHa" data-i18n="tabHa">Home Assistant</button>
  <button type="button" role="tab" id="tab-tSys" aria-controls="tSys" aria-selected="false" tabindex="-1" data-tab="tSys"    data-i18n="tabSys">Systém</button>
</nav>

<div class="wrap">

<!-- ================= OVLÁDÁNÍ ================= -->
<section id="tCtl" role="tabpanel" aria-labelledby="tab-tCtl" class="tab">
  <div class="card hide" id="cardWifi">
    <h2 data-i18n="wifi">WiFi</h2>
    <p class="hint" id="wifiNow"></p>
    <div class="row"><label data-i18n="network">Síť</label>
      <select id="ssid"></select>
      <button class="sec" onclick="scan()" data-i18n="scan">Vyhledat</button></div>
    <div class="row"><label data-i18n="password">Heslo</label><input type="password" id="wpass"></div>
    <p class="hint" id="wifiHintTxt"></p>
    <button onclick="saveWifi()" data-i18n="connect">Připojit</button>

    <div id="wifiSlots">
      <h2 style="margin-top:18px" data-i18n="wifiSaved">Uložené sítě</h2>
      <p class="hint" data-i18n="wifiSavedHint">Zařízení si pamatuje až tři sítě.</p>
      <datalist id="ssidList"></datalist>
      <div class="row"><label>1.</label><input type="text" id="ws0" list="ssidList" maxlength="32" style="flex:2 1 160px"><input type="password" id="wp0" maxlength="64" style="flex:2 1 140px"></div>
      <div class="row"><label>2.</label><input type="text" id="ws1" list="ssidList" maxlength="32" style="flex:2 1 160px"><input type="password" id="wp1" maxlength="64" style="flex:2 1 140px"></div>
      <div class="row"><label>3.</label><input type="text" id="ws2" list="ssidList" maxlength="32" style="flex:2 1 160px"><input type="password" id="wp2" maxlength="64" style="flex:2 1 140px"></div>
      <button class="sec" onclick="saveWifiList()" data-i18n="wifiSaveList">Uložit sítě</button>
    </div>
  </div>

  <div class="card" id="cardRemote">
    <h2 data-i18n="remote">Dálkové ovládání</h2>
    <div class="row" id="scrBtns"></div>
    <div class="row remote-steps">
      <div class="step-group"><button class="sec" onclick="stepScreen(-1)" aria-label="Previous screen">&#8592;</button>
      <button class="sec" aria-label="Next screen" onclick="stepScreen(1)">&#8594;</button>
      </div><div class="step-group"><span data-i18n="rangeLbl">Rozsah</span>
      <button class="sec" id="rMinus" onclick="stepRange(-1)">&minus;</button>
      <b id="rangeNow" style="min-width:80px;text-align:center">&ndash;</b>
      <button class="sec" id="rPlus" onclick="stepRange(1)">+</button></div>
    </div>
    <p class="hint" data-i18n="gestureHint"></p>
    <p class="hint" data-i18n="remoteHint">Rozsah se mění jen na obrazovkách Letadla a Meteoradar. Zásah pozastaví automatické střídání.</p>
  </div>

  <div class="card">
    <h2 data-i18n="statusHdr">Stav zařízení</h2>
    <table id="statusTab"></table>
  </div>
</section>

<!-- ================= POLOHA ================= -->
<section id="tLoc" role="tabpanel" aria-labelledby="tab-tLoc" class="tab hide">
  <div class="card">
    <h2 data-i18n="location">Poloha</h2>
    <div class="row"><label data-i18n="findCity">Najít město</label>
      <input type="text" id="q" placeholder="Praha">
      <button class="sec" onclick="geo()" data-i18n="search">Hledat</button></div>
    <div class="row hide" id="geoRow"><label data-i18n="found">Nalezeno</label>
      <select id="geoSel" onchange="pickCity()"></select></div>
    <div class="row"><label data-i18n="lat">Zeměpisná šířka</label><input type="number" step="0.0001" id="lat"></div>
    <div class="row"><label data-i18n="lon">Zeměpisná délka</label><input type="number" step="0.0001" id="lon"></div>
    <p class="hint" data-i18n="locHint">Změna polohy vyžaduje restart, o který se zařízení postará samo.</p>
  </div>
</section>

<!-- ================= OBRAZOVKY ================= -->
<section id="tScr" role="tabpanel" aria-labelledby="tab-tScr" class="tab hide">
  <div class="card">
    <h2 data-i18n="screens">Obrazovky</h2>
    <div class="grid">
      <label class="chk"><input type="checkbox" id="sClock"><span data-i18n="scrClock">Hodiny</span></label>
      <label class="chk"><input type="checkbox" id="sPlanes"><span data-i18n="scrPlanes">Letadla</span></label>
      <label class="chk"><input type="checkbox" id="sMeteo"><span data-i18n="scrMeteo">Meteoradar</span></label>
      <label class="chk"><input type="checkbox" id="sForecast"><span data-i18n="scrForecast">Předpověď</span></label>
      <label class="chk hide" id="energyPriceOption"><input type="checkbox" id="sPrice"><span data-i18n="scrPrice">Cena elektřiny</span></label>
      <label class="chk hide" id="energyMixOption"><input type="checkbox" id="sMix"><span data-i18n="scrMix">Výroba elektřiny</span></label>
      <label class="chk"><input type="checkbox" id="sCalendar"><span data-i18n="scrCalendar">Kalendář</span></label>
    </div>
    <p class="hint" data-i18n="scrHint">Vypnuté obrazovky se přeskakují. Nastavení je dostupné vždy.</p>
    <p class="hint" data-i18n="restartHint">Změna obrazovek, zdroje radaru nebo polohy potřebuje restart, takže se ukládá až tlačítkem vpravo nahoře.</p>
    <div class="row"><label data-i18n="autoRotate">Automatické střídání (sekundy, 0 = vypnuto)</label>
      <input type="number" id="autoRotate" min="0" max="3600" step="5"></div>
    <p class="hint" data-i18n="rotHint">Střídání pozastaví přejetí prstem, dlouhý stisk nebo přepnutí z prohlížeče — na trojnásobek intervalu, pak pokračuje samo. Obyčejné klepnutí ho nezastaví, otevřený detail letadla ho drží. Na obrazovce Nastavení se nestřídá.</p>
  </div>

  <div class="card hide" id="energySettings">
    <h2 data-i18n="energyTitle">Elektřina</h2>
    <p class="hint" data-i18n="energyHint">Tyto obrazovky jsou dostupné pouze v češtině. Ceny platí pro český trh; původní geografické omezení zdrojů zůstává.</p>
    <div class="row"><label data-i18n="priceFee">Příplatek dodavatele (Kč/MWh)</label><input type="number" id="priceFee" min="0" max="20000" step="10"></div>
    <div class="row"><label data-i18n="priceVat">DPH (%)</label><input type="number" id="priceVat" min="0" max="100"></div>
    <p class="hint" data-i18n="priceHint">Nula znamená čistou burzovní cenu bez příplatku a DPH. Nejde o celkovou cenu vyúčtování.</p>
    <div class="row"><label data-i18n="mixCountry">Země výroby</label><input type="text" id="mixCountry" maxlength="2" pattern="[A-Za-z]{2}" placeholder="cz"></div>
  </div>

  <div class="card">
    <h2 data-i18n="stormHdr">Meteoradar jen při bouřce</h2>
    <div class="row"><label class="chk"><input type="checkbox" id="stormOnly"><span data-i18n="stormOnly">Meteoradar ve střídání jen když se blíží srážky/bouřka</span></label></div>
    <div class="row"><label data-i18n="stormDbz">Práh odrazivosti (dBZ)</label><input type="number" id="stormDbz" min="4" max="60" step="1"></div>
    <div class="row"><label data-i18n="stormKm">Vzdálenost od zařízení (km)</label><input type="number" id="stormKm" min="5" max="250" step="5"></div>
    <p class="hint" data-i18n="stormHint"></p>
  </div>

  <div class="card">
    <h2 data-i18n="paHdr">Letadla jen když je letadlo poblíž</h2>
    <div class="row"><label class="chk"><input type="checkbox" id="planeAlert"><span data-i18n="planeAlert">Ukázat letadla jen když je letadlo blízko</span></label></div>
    <div class="row"><label data-i18n="paKm">Vzdálenost (km)</label><input type="number" id="planeAlertKm" min="1" max="50" step="1"></div>
    <div class="row"><label data-i18n="paSec">Jak dlouho ukázat (sekundy)</label><input type="number" id="planeAlertSec" min="5" max="300" step="5"></div>
    <p class="hint" data-i18n="paHint"></p>
  </div>

  <div class="card">
    <h2 data-i18n="radar">Meteoradar</h2>
    <div class="row"><label data-i18n="radarSrc">Zdroj dat</label>
      <select id="radarSrc">
        <option value="0" data-i18n="srcChmu">ČHMÚ (ostřejší, jen ČR)</option>
        <option value="2" data-i18n="srcChmuM">ČHMÚ masked (jen srážky dopadající na zem)</option>
        <option value="1" data-i18n="srcRv">RainViewer (Evropa i svět)</option>
      </select></div>
    <p class="hint" data-i18n="srcDiff"></p>
    <p class="hint" data-i18n="radarHint">Mimo ČR nemá ČHMÚ data a obrazovka zůstane prázdná — použijte RainViewer.</p>
    <div class="row"><label class="chk"><input type="checkbox" id="meteoLegend"><span data-i18n="mtLegend">Zobrazit legendu (dBZ / mm/h)</span></label></div>
    <p class="hint" data-i18n="mtLegendHint">Legenda zabírá levý okraj mapy. Když stupnici znáte, dá se skrýt a je vidět víc území.</p>
  </div>
</section>

<!-- ================= VZHLED ================= -->
<section id="tLook" role="tabpanel" aria-labelledby="tab-tLook" class="tab hide">
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
    <div class="row"><label data-i18n="clockFont">Písmo (všechny obrazovky)</label>
      <select id="clockFont">
        <option value="1">Inter</option>
        <option value="2">Poppins</option>
        <option value="3" data-i18n="fontThin">Inter tenké</option>
        <option value="0" data-i18n="fontPixel">pixelové (původní)</option>
      </select></div>
    <div class="row"><label data-i18n="clockColor">Barva hodin</label><input type="color" id="clockColor"></div>
    <div class="row"><label data-i18n="secColor">Barva vteřin</label><input type="color" id="secColor"></div>
  </div>
</section>

<!-- ================= LETADLA ================= -->
<section id="tPlanes" role="tabpanel" aria-labelledby="tab-tPlanes" class="tab hide">
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

  <div class="card">
    <h2 data-i18n="altHdr">Barvy podle výšky</h2>
    <div class="row"><label data-i18n="altBelow">Pod 1. hladinou</label><input type="color" id="ac0"></div>
    <div class="row"><label data-i18n="altB1">1. hladina (m)</label><input type="number" id="ab0" min="100" max="20000" step="100"><input type="color" id="ac1"></div>
    <div class="row"><label data-i18n="altB2">2. hladina (m)</label><input type="number" id="ab1" min="100" max="20000" step="100"><input type="color" id="ac2"></div>
    <div class="row"><label data-i18n="altB3">3. hladina (m)</label><input type="number" id="ab2" min="100" max="20000" step="100"><input type="color" id="ac3"></div>
    <div class="row"><button class="sec" onclick="altDefaults()" data-i18n="altDef">Výchozí barvy a hladiny</button></div>
    <p class="hint" data-i18n="altHint"></p>
  </div>

  <div class="card">
    <h2 data-i18n="winHdr">Výhled z okna</h2>
    <div class="row"><label class="chk"><input type="checkbox" id="windowOn"><span data-i18n="winOn">Používat výhled z okna</span></label></div>
    <div class="row"><label data-i18n="winAz">Směr okna</label>
      <select id="windowAz">
        <option value="0" data-i18n="dir0">sever (S)</option>
        <option value="45" data-i18n="dir45">severovýchod (SV)</option>
        <option value="90" data-i18n="dir90">východ (V)</option>
        <option value="135" data-i18n="dir135">jihovýchod (JV)</option>
        <option value="180" data-i18n="dir180">jih (J)</option>
        <option value="225" data-i18n="dir225">jihozápad (JZ)</option>
        <option value="270" data-i18n="dir270">západ (Z)</option>
        <option value="315" data-i18n="dir315">severozápad (SZ)</option>
      </select></div>
    <div class="row"><label data-i18n="winW">Šířka výhledu (°)</label><input type="number" id="windowWidth" min="10" max="300" step="5"></div>
    <div class="row"><label data-i18n="winKm">Dohled (km)</label><input type="number" id="windowKm" min="1" max="100" step="1"></div>
    <p class="hint" data-i18n="winHint"></p>
  </div>

  <div class="card">
    <h2 data-i18n="iconHdr">Ikony letadel</h2>
    <p class="hint" data-i18n="iconIntro"></p>
    <div id="iconLegend" style="display:grid;grid-template-columns:repeat(auto-fill,minmax(230px,1fr));gap:6px 14px;margin-top:10px"></div>
    <p class="hint" data-i18n="iconCredit"></p>
  </div>
</section>

<!-- ================= KALENDÁŘ ================= -->
<section id="tCal" role="tabpanel" aria-labelledby="tab-tCal" class="tab hide">
 <div class="card">
  <h2 data-i18n="scrCalendar">Kalendář</h2>
  <p class="hint" data-i18n="calIntro"></p>
  <p class="hint" id="calScreenState"></p>
  <div class="row"><label for="calendarView" data-i18n="calView"></label><select id="calendarView"><option value="0" data-i18n="calDay"></option><option value="1" data-i18n="calMonth"></option></select></div>
  <div class="row"><label for="calendarRefresh" data-i18n="calRefresh"></label><input id="calendarRefresh" type="number" min="5" max="1440" value="15"></div>
  <fieldset class="source"><legend><span data-i18n="calSource"></span> 1</legend>
  <label class="chk"><input type="checkbox" id="cal0On"><span data-i18n="calEnabled"></span></label>
  <div class="row"><label for="cal0Name" data-i18n="calName"></label><input type="text" id="cal0Name" maxlength="31"></div>
  <div class="row"><label for="cal0Color" data-i18n="calColor"></label><input id="cal0Color" type="color" value="#00bfff"></div>
  <div class="row"><label for="cal0Url" data-i18n="calUrl"></label><input id="cal0Url" type="password" autocomplete="off" spellcheck="false" maxlength="767"></div>
  <label class="chk"><input type="checkbox" id="cal0Clear"><span data-i18n="calClear"></span></label>
  <p class="hint" id="cal0Status"></p></fieldset>
  <fieldset class="source"><legend><span data-i18n="calSource"></span> 2</legend>
  <label class="chk"><input type="checkbox" id="cal1On"><span data-i18n="calEnabled"></span></label>
  <div class="row"><label for="cal1Name" data-i18n="calName"></label><input type="text" id="cal1Name" maxlength="31"></div>
  <div class="row"><label for="cal1Color" data-i18n="calColor"></label><input id="cal1Color" type="color" value="#ff9900"></div>
  <div class="row"><label for="cal1Url" data-i18n="calUrl"></label><input id="cal1Url" type="password" autocomplete="off" spellcheck="false" maxlength="767"></div>
  <label class="chk"><input type="checkbox" id="cal1Clear"><span data-i18n="calClear"></span></label>
  <p class="hint" id="cal1Status"></p></fieldset>
  <fieldset class="source"><legend><span data-i18n="calSource"></span> 3</legend>
  <label class="chk"><input type="checkbox" id="cal2On"><span data-i18n="calEnabled"></span></label>
  <div class="row"><label for="cal2Name" data-i18n="calName"></label><input type="text" id="cal2Name" maxlength="31"></div>
  <div class="row"><label for="cal2Color" data-i18n="calColor"></label><input id="cal2Color" type="color" value="#00ff00"></div>
  <div class="row"><label for="cal2Url" data-i18n="calUrl"></label><input id="cal2Url" type="password" autocomplete="off" spellcheck="false" maxlength="767"></div>
  <label class="chk"><input type="checkbox" id="cal2Clear"><span data-i18n="calClear"></span></label>
  <p class="hint" id="cal2Status"></p></fieldset>
  <div class="help"><p class="hint" data-i18n="calUrlHint"></p>
  <p class="hint" data-i18n="calGoogleHint"></p>
  <p class="hint" data-i18n="calAppleHint"></p>
  <p class="hint" data-i18n="calDotsHint"></p>
  <p class="hint" data-i18n="calLimits"></p>
  <p class="hint" data-i18n="calSaveHint"></p>
  <button class="sec" type="button" onclick="refreshCalendars()" data-i18n="calReload"></button></div>
 </div>
</section>

<section id="tNotify" role="tabpanel" aria-labelledby="tab-tNotify" class="tab hide">
 <div class="card">
  <h2 data-i18n="ntCodeTitle"></h2>
  <p class="hint" data-i18n="ntCodeHint"></p>
  <div class="row">
   <button type="button" id="ntCodeBtn" onclick="startPairCode()" data-i18n="ntCodeStart"></button>
   <button type="button" id="ntCodeCancel" class="sec hide" onclick="cancelPairCode()" data-i18n="ntCodeCancel"></button>
  </div>
  <output id="ntCode" class="pair-code hide" aria-live="polite"></output>
  <p class="hint" id="ntCodeAddress"></p>
  <p class="hint" id="ntCodeExpiry" role="status"></p>
  <details class="help"><summary data-i18n="ntAdvanced"></summary>
   <div class="notification-actions"><button type="button" id="ntPairBtn" onclick="pairNotifications()" data-i18n="ntPair"></button></div>
   <p class="hint" data-i18n="ntPairHint"></p>
  </details>
 </div>

 <div class="card">
  <h2 data-i18n="ntTitle"></h2>
  <p data-i18n="ntIntro"></p>
  <label class="chk"><input type="checkbox" id="ntEnabled"><span data-i18n="ntEnabled"></span></label>
  <div class="row"><label for="ntSeconds" data-i18n="ntSeconds"></label><input type="number" id="ntSeconds" min="0" max="120" value="15"></div>
  <p class="hint" data-i18n="ntTimeoutHint"></p>
  <p class="hint" id="ntPairState"></p>
  <p class="hint" id="ntStatus"></p>
  <div class="notification-actions"><button type="button" class="sec" id="ntTestBtn" onclick="testNotification()" data-i18n="ntTest"></button></div>
 </div>

</section>

<!-- ================= HOME ASSISTANT ================= -->
<section id="tHa" role="tabpanel" aria-labelledby="tab-tHa" class="tab hide">
  <div class="card">
    <h2>Home Assistant</h2>
    <p class="hint warn" data-i18n="haUntested"></p>
    <div class="row"><label class="chk"><input type="checkbox" id="haEnabled"><span data-i18n="haOn">Zpřístupnit data pro Home Assistant</span></label></div>
    <p class="hint" data-i18n="haHow"></p>
    <p><b id="haUrl"></b> &nbsp;<a id="haLink" target="_blank" rel="noopener" data-i18n="haOpen">otevřít</a></p>
    <p class="hint" data-i18n="haYamlHint"></p>
    <pre id="haYaml" style="background:#0e1218;border:1px solid var(--line);border-radius:6px;padding:10px;overflow-x:auto;font-size:12px;white-space:pre"></pre>
    <button class="sec" onclick="copyHa()" data-i18n="haCopy">Zkopírovat</button>
  </div>
</section>

<!-- ================= SYSTÉM ================= -->
<section id="tSys" role="tabpanel" aria-labelledby="tab-tSys" class="tab hide">
  <div class="card">
    <h2 data-i18n="system">Systém</h2>
    <div class="row"><label data-i18n="adminPass">Současné heslo</label>
      <input type="password" id="adminPass" placeholder="•••"></div>
    <div class="row"><label data-i18n="newPass">Nové heslo</label>
      <input type="password" id="newPass" placeholder="•••"></div>
    <p class="hint" id="pwState"></p>
    <p class="hint" data-i18n="passHint">Současné heslo je potřeba pro aktualizaci, import, reset i pro změnu hesla. Prázdné nové heslo nic nemění; jedna mezera ochranu zruší.</p>
    <p class="hint" data-i18n="passNote">Poznámka: výchozí stav je bez hesla — aktualizace, import i reset jsou pak přístupné komukoli v síti. Heslo se ukládá v čitelné podobě, protože aktualizační stránka používá HTTP Basic, kde otisk použít nelze; chrání tedy před domácností, ne před útočníkem. Zapomenuté heslo jde zrušit jedině podržením tlačítka BOOT při startu (~3 s), což smaže i WiFi a všechna ostatní nastavení.</p>
    <div class="actions">
      <button class="sec" onclick="location='/update'" data-i18n="fwUpdate">Aktualizace firmwaru</button>
      <button class="sec" onclick="exportCfg()" data-i18n="export">Export nastavení</button>
      <button class="sec" onclick="document.getElementById('imp').click()" data-i18n="import">Import</button>
      <input type="file" id="imp" class="hide" accept="application/json" onchange="importCfg(this)">
      <button class="sec" onclick="doReboot()" data-i18n="reboot">Restart</button>
      <button class="danger" onclick="doReset()" data-i18n="factory">Tovární reset</button>
    </div>

    <!-- Attribution for the active data sources. -->
  </div>
  <div class="card">
    <h2 data-i18n="srcTitle">Zdroje dat</h2>
    <p class="hint" data-i18n="srcIntro">Data pocházejí z těchto služeb. Několik z nich vyžaduje uvedení zdroje, proto jsou uvedeny zde. Podrobnosti a podmínky jsou v souboru LICENSE.txt.</p>
    <p class="hint">
      <a href="https://adsb.fi" target="_blank" rel="noopener">adsb.fi</a> ·
      <a href="https://adsb.lol" target="_blank" rel="noopener">adsb.lol</a> ·
      <a href="https://opendata.chmi.cz" target="_blank" rel="noopener">ČHMÚ</a> ·
      <a href="https://www.rainviewer.com" target="_blank" rel="noopener">RainViewer</a> ·
      <a href="https://open-meteo.com" target="_blank" rel="noopener">Open-Meteo</a> ·
      <a href="https://www.geonames.org" target="_blank" rel="noopener">GeoNames</a> (CC BY 4.0) ·
      Natural Earth ·
      <a href="http://ip-api.com" target="_blank" rel="noopener">ip-api.com</a> ·
      <span data-i18n="srcIcons">ikony letadel podle</span> <a href="https://adsb-radar.com" target="_blank" rel="noopener">ADS-B Radar</a>
    </p>
  </div>
</section>

</div>



<script>
const D={
 cs:{"scrCalendar":"Kalendář","calIntro":"Události z Google Kalendáře, iCloudu nebo jiného ICS odběru. Na displeji přepnete den a měsíc; klepnutí na datum otevře jeho události.","calView":"Výchozí / poslední pohled","calDay":"Den","calMonth":"Měsíc","calRefresh":"Obnovování (minuty, 5–1440)","calSource":"Kalendář","calEnabled":"Používat tento kalendář","calName":"Název","calColor":"Barva","calUrl":"Nový ICS odkaz (HTTPS / webcal)","calClear":"Smazat uložený odkaz","calUrlHint":"Odkazy jsou přístup k událostem: nevracejí se do stránky ani do exportu. Prázdné pole ponechá uložený odkaz. Po importu zálohy do nového zařízení je zadejte znovu.","calGoogleHint":"Google: Nastavení kalendáře → Integrovat kalendář → Tajná adresa ve formátu iCal. Není potřeba zveřejňovat kalendář.","calAppleHint":"iCloud: zapněte Veřejný kalendář a vložte jeho odkaz. Kdokoli s tímto odkazem může kalendář číst. Jde o odběr pro zobrazení; přihlášení Apple/Google ani zápis událostí zde nejsou.","calDotsHint":"Jeden aktivní kalendář: zvýrazněné dny, bez barevných teček a názvu zdroje u události. Více kalendářů: barevné tečky a názvy zdrojů.","calLimits":"Ukládá se předchozí měsíc, aktuální a dva následující; nejvýše 256 výskytů a 512 KiB na zdroj. Podporovány jsou UTC, místní čas a běžná středoevropská pásma. Nepodporované opakování nebo pásmo se zobrazí jako chyba; poslední platná data zůstanou.","calSaveHint":"Zdroje a pohled uložte tlačítkem vpravo nahoře. Změna sady obrazovek restartuje zařízení. Je-li nastaveno heslo správce, vyplňte jej na kartě Systém.","calReload":"Obnovit uložené kalendáře","calConfigured":"Odkaz uložen","calMissing":"Odkaz není nastaven","calMemory":"Paměť kalendáře (PSRAM)",tabCtl:"Ovládání",tabLoc:"Poloha",tabScr:"Obrazovky",tabLook:"Vzhled",tabPlanes:"Letadla",tabSys:"Systém",
  timezone:"Časové pásmo",
  wifi:"WiFi",network:"Síť",password:"Heslo",scan:"Vyhledat",connect:"Připojit",
  wifiHint:"Po uložení se zařízení připojí a přístupový bod zmizí.",
  wifiHintSta:"Změna sítě přeruší spojení s touto stránkou. Když se připojení nepovede, zařízení vytvoří vlastní síť MeteoPlaneRadar a zadáte ji znovu.",
  wifiNow:"Připojeno k síti",
  autoSaved:"Uloženo",
  liveHint:"Změny na této záložce se ukládají hned, tlačítko Uložit tu nepotřebujete.",
  restartHint:"Změna obrazovek, zdroje radaru nebo polohy potřebuje restart, takže se ukládá až tlačítkem vpravo nahoře.",
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
 en:{"scrCalendar":"Calendar","calIntro":"Events from Google Calendar, iCloud or another ICS subscription. Switch day/month on the display; tap a date to open its agenda.","calView":"Default / last view","calDay":"Day","calMonth":"Month","calRefresh":"Refresh interval (minutes, 5–1440)","calSource":"Calendar","calEnabled":"Use this calendar","calName":"Name","calColor":"Colour","calUrl":"New ICS URL (HTTPS / webcal)","calClear":"Delete the saved URL","calUrlHint":"URLs grant access to events: they are never returned to the page or exported. An empty field keeps the saved URL. Re-enter URLs after importing a backup on a new device.","calGoogleHint":"Google: Calendar settings → Integrate calendar → Secret address in iCal format. The calendar does not need to be public.","calAppleHint":"iCloud: enable Public Calendar and paste its link. Anyone with that link can read the calendar. This is a display subscription; Apple/Google sign-in and editing events are not provided.","calDotsHint":"One enabled calendar: highlighted dates, no coloured dots or source name in the agenda. Multiple calendars: source dots and names.","calLimits":"Caches the previous, current and next two months; up to 256 occurrences and 512 KiB per source. Supports UTC, local time and common Central European zones. Unsupported recurrence or zones report an error; the last valid data remains.","calSaveHint":"Save sources and view with the button at the top right. Changing enabled screens restarts the device. If an admin password is set, enter it on the System tab.","calReload":"Refresh saved calendars","calConfigured":"URL saved","calMissing":"No URL configured","calMemory":"Calendar memory (PSRAM)",timezone:"Time zone",tabCtl:"Control",tabLoc:"Location",tabScr:"Screens",tabLook:"Appearance",tabPlanes:"Aircraft",tabSys:"System",
  wifi:"WiFi",network:"Network",password:"Password",scan:"Scan",connect:"Connect",
  wifiHint:"After saving the device connects and the access point disappears.",
  wifiHintSta:"Changing the network drops this page. If the connection fails, the device brings up its own MeteoPlaneRadar network and you enter it again.",
  wifiNow:"Connected to",
  autoSaved:"Saved",
  liveHint:"Changes on this tab are saved immediately - no need for the Save button.",
  restartHint:"Changing the screens, the radar source or the location needs a restart, so those are saved with the button at the top right.",
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
 sk:{"scrCalendar":"Kalendár","calIntro":"Udalosti z Google Kalendára, iCloudu alebo iného ICS odberu. Na displeji prepnete deň a mesiac; ťuknutie na dátum otvorí jeho udalosti.","calView":"Predvolený / posledný pohľad","calDay":"Deň","calMonth":"Mesiac","calRefresh":"Obnovovanie (minúty, 5–1440)","calSource":"Kalendár","calEnabled":"Používať tento kalendár","calName":"Názov","calColor":"Farba","calUrl":"Nový ICS odkaz (HTTPS / webcal)","calClear":"Vymazať uložený odkaz","calUrlHint":"Odkazy umožňujú prístup k udalostiam: nevracajú sa do stránky ani do exportu. Prázdne pole ponechá uložený odkaz. Po importe zálohy do nového zariadenia ich zadajte znova.","calGoogleHint":"Google: Nastavenia kalendára → Integrovať kalendár → Tajná adresa vo formáte iCal. Kalendár nemusí byť verejný.","calAppleHint":"iCloud: zapnite Verejný kalendár a vložte jeho odkaz. Ktokoľvek s odkazom môže kalendár čítať. Ide o odber na zobrazenie; prihlásenie Apple/Google ani úpravy udalostí tu nie sú.","calDotsHint":"Jeden aktívny kalendár: zvýraznené dni, bez farebných bodiek a názvu zdroja pri udalosti. Viac kalendárov: farebné bodky a názvy zdrojov.","calLimits":"Ukladá sa predchádzajúci mesiac, aktuálny a dva nasledujúce; najviac 256 výskytov a 512 KiB na zdroj. Podporované sú UTC, miestny čas a bežné stredoeurópske pásma. Nepodporované opakovanie alebo pásmo sa zobrazí ako chyba; posledné platné dáta zostanú.","calSaveHint":"Zdroje a pohľad uložte tlačidlom vpravo hore. Zmena sady obrazoviek reštartuje zariadenie. Ak je nastavené heslo správcu, vyplňte ho na karte Systém.","calReload":"Obnoviť uložené kalendáre","calConfigured":"Odkaz uložený","calMissing":"Odkaz nie je nastavený","calMemory":"Pamäť kalendára (PSRAM)",
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
  "restartHint": "Zmena obrazoviek, zdroja radaru alebo polohy vyžaduje reštart, preto sa ukladá tlačidlom vpravo hore.",
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
Object.assign(D.en,{"ntTitle": "Notifications", "ntIntro": "Compact banner above every screen. Tap for details; the large X dismisses the message only on this display.", "ntEnabled": "Enable notifications", "ntSeconds": "Banner duration (seconds)", "ntTimeoutHint": "0 = until closed, otherwise 5–120 s. After timeout, messages stay behind the count badge. Details stay open until dismissed.", "ntTest": "Save and show a test", "ntPair": "Pair bridge / download configuration", "ntPairHint": "Enables notifications and rotates the pairing key. Previous bridges stop working. Keep the downloaded file private; it is sent over your home LAN. If set, enter the current admin password on System first.", "ntPaired": "Bridge paired", "ntUnpaired": "Bridge not paired", "ntBridge": "Bridge contact (s ago; −1 = none)", "ntQueue": "Pending", "ntMemory": "Notification PSRAM"});
Object.assign(D.sk,{"ntTitle": "Notifikácie", "ntIntro": "Kompaktný banner nad každou obrazovkou. Ťuknutie otvorí detail; veľký krížik zavrie správu iba na displeji.", "ntEnabled": "Zapnúť notifikácie", "ntSeconds": "Trvanie bannera (sekundy)", "ntTimeoutHint": "0 = do zatvorenia, inak 5–120 s. Po uplynutí času zostanú správy pod odznakom s počtom. Detail zostáva otvorený do zatvorenia.", "ntTest": "Uložiť a zobraziť skúšku", "ntPair": "Spárovať most / stiahnuť konfiguráciu", "ntPairHint": "Zapne notifikácie a vymení párovací kľúč. Predchádzajúci most prestane fungovať. Stiahnutý súbor uchovajte súkromne; prenáša sa cez domácu LAN. Ak máte heslo správcu, najprv ho zadajte na karte Systém.", "ntPaired": "Most spárovaný", "ntUnpaired": "Most ešte nie je spárovaný", "ntBridge": "Kontakt mosta (pred s; −1 = žiadny)", "ntQueue": "Čakajúce správy", "ntMemory": "PSRAM notifikácií"});
Object.assign(D.cs,{"ntTitle": "Notifikace", "ntIntro": "Kompaktní banner nad každou obrazovkou. Klepnutí otevře detail; velký křížek zavře zprávu pouze na displeji.", "ntEnabled": "Zapnout notifikace", "ntSeconds": "Trvání banneru (sekundy)", "ntTimeoutHint": "0 = do zavření, jinak 5–120 s. Po uplynutí času zůstanou zprávy pod odznakem s počtem. Detail zůstává otevřený do zavření.", "ntTest": "Uložit a zobrazit test", "ntPair": "Spárovat most / stáhnout konfiguraci", "ntPairHint": "Zapne notifikace a vymění párovací klíč. Předchozí most přestane fungovat. Stažený soubor uchovejte soukromě; přenáší se přes domácí LAN. Máte-li heslo správce, nejprve jej vyplňte na kartě Systém.", "ntPaired": "Most spárovaný", "ntUnpaired": "Most ještě není spárovaný", "ntBridge": "Kontakt mostu (před s; −1 = žádný)", "ntQueue": "Čekající zprávy", "ntMemory": "PSRAM notifikací"});
Object.assign(D.cs,{"calSaveEnable": "Uložit a zapnout kalendář", "calScreenOff": "Obrazovka Kalendář je vypnutá. Samotné uložení odkazu ji nezapne. Tlačítko „Uložit a zapnout kalendář“ uloží odběry a zpřístupní obrazovku.", "calScreenOn": "Obrazovka Kalendář je zapnutá. Stav stahování najdete pod jednotlivými odběry.", "calNeedSource": "Zadejte odkaz a zapněte alespoň jeden odběr, případně použijte již uložený odkaz.", "calEnabledSaved": "Kalendář uložen a zapnut. Při změně obrazovek se zařízení restartuje; po chvíli obnovte stránku.", "calSaveHint": "Odběry i obrazovku zapnete tlačítkem Uložit a zapnout kalendář nahoře. Obnovit uložené kalendáře používá pouze již uložené odkazy. Máte-li heslo správce, vyplňte jej na kartě Systém."});
Object.assign(D.en,{"calSaveEnable": "Save and enable calendar", "calScreenOff": "The Calendar screen is disabled. Saving a URL alone does not enable it. Save and enable calendar saves subscriptions and enables the screen.", "calScreenOn": "The Calendar screen is enabled. Download status appears below each subscription.", "calNeedSource": "Enter a URL and enable at least one subscription, or use a saved URL.", "calEnabledSaved": "Calendar saved and enabled. A change of enabled screens restarts the device; reload this page shortly.", "calSaveHint": "Use Save and enable calendar at the top right to save subscriptions and enable the screen. Refresh saved calendars only uses URLs already saved. If an admin password is set, enter it on System."});
Object.assign(D.sk,{"calSaveEnable": "Uložiť a zapnúť kalendár", "calScreenOff": "Obrazovka Kalendár je vypnutá. Samotné uloženie odkazu ju nezapne. Tlačidlo „Uložiť a zapnúť kalendár“ uloží odbery a sprístupní obrazovku.", "calScreenOn": "Obrazovka Kalendár je zapnutá. Stav sťahovania nájdete pod jednotlivými odbermi.", "calNeedSource": "Zadajte odkaz a zapnite aspoň jeden odber, prípadne použite už uložený odkaz.", "calEnabledSaved": "Kalendár uložený a zapnutý. Pri zmene obrazoviek sa zariadenie reštartuje; po chvíli obnovte stránku.", "calSaveHint": "Odbery aj obrazovku zapnete tlačidlom Uložiť a zapnúť kalendár vpravo hore. Obnoviť uložené kalendáre používa iba už uložené odkazy. Ak máte heslo správcu, vyplňte ho na karte Systém."});
Object.assign(D.cs,{autoMode:"Změny se ukládají průběžně"});
Object.assign(D.en,{autoMode:"Changes save automatically"});
Object.assign(D.sk,{autoMode:"Zmeny sa ukladajú priebežne"});
Object.assign(D.cs,{ntCodeTitle:"Spárovat Mac kódem",ntCodeHint:"Vygenerujte kód a zadejte jej v aplikaci Meteo Chat spolu s adresou tohoto displeje. Kód platí 2 minuty a lze jej použít jednou. Po spárování se notifikace zapnou a původní párování se nahradí.",ntCodeStart:"Vygenerovat párovací kód",ntCodeCancel:"Zrušit kód",ntCodeRemaining:"Platnost kódu (s): ",ntCodeExpired:"Kód vypršel. Vygenerujte nový.",ntCodeCancelled:"Párovací kód byl zrušen.",ntCodeDone:"Mac byl úspěšně spárován.",ntCodeClosed:"Párování je ukončené. Vygenerujte nový kód.",ntAdvanced:"Pokročilé párování souborem"});
Object.assign(D.en,{ntCodeTitle:"Pair a Mac with a code",ntCodeHint:"Generate a code and enter it in Meteo Chat with this display's address. The code lasts 2 minutes and works once. Pairing enables notifications and replaces the previous pairing.",ntCodeStart:"Generate pairing code",ntCodeCancel:"Cancel code",ntCodeRemaining:"Code valid for (s): ",ntCodeExpired:"Code expired. Generate a new one.",ntCodeCancelled:"Pairing code cancelled.",ntCodeDone:"Mac paired successfully.",ntCodeClosed:"Pairing window closed. Generate a new code.",ntAdvanced:"Advanced file pairing"});
Object.assign(D.sk,{ntCodeTitle:"Spárovať Mac kódom",ntCodeHint:"Vygenerujte kód a zadajte ho v aplikácii Meteo Chat spolu s adresou tohto displeja. Kód platí 2 minúty a dá sa použiť raz. Po spárovaní sa notifikácie zapnú a pôvodné párovanie sa nahradí.",ntCodeStart:"Vygenerovať párovací kód",ntCodeCancel:"Zrušiť kód",ntCodeRemaining:"Platnosť kódu (s): ",ntCodeExpired:"Kód vypršal. Vygenerujte nový.",ntCodeCancelled:"Párovací kód bol zrušený.",ntCodeDone:"Mac bol úspešne spárovaný.",ntCodeClosed:"Párovanie je ukončené. Vygenerujte nový kód.",ntAdvanced:"Pokročilé párovanie súborom"});
Object.assign(D.cs,{"tabHa": "Home Assistant", "wifiSaved": "Uložené sítě", "wifiSavedHint": "Zařízení si pamatuje až tři sítě. Lze je zapsat ručně i když zrovna nejsou v dosahu — třeba síť v kanceláři ještě doma. Po výpadku spojení zkusí zařízení po minutě ostatní uložené sítě, nejsilnější jako první. Prázdné pole hesla nechá uložené heslo beze změny. Uložení sítí zařízení neodpojí.", "wifiSaveList": "Uložit sítě", "passStored": "heslo uloženo", "passNone": "heslo", "ssidPh": "název sítě", "stormHdr": "Meteoradar jen při bouřce", "stormOnly": "Meteoradar ve střídání jen když se blíží srážky/bouřka", "stormDbz": "Práh odrazivosti (dBZ)", "stormKm": "Vzdálenost od zařízení (km)", "stormHint": "Každých 5 minut se zkontroluje nejnovější snímek radaru ČHMÚ. Když je v zadané vzdálenosti odraz aspoň zadané síly, meteoradar se přidá do střídání; po dvou klidných snímcích zase vypadne. Funguje i když je obrazovka Meteoradar výše vypnutá. Orientačně: 20 dBZ slabý déšť, 40 dBZ silný déšť, nad 50 dBZ bouřka s možným krupobitím. Data jsou jen z ČHMÚ, takže v ČR a kousek za hranicemi. Potřebuje zapnuté automatické střídání.", "paHdr": "Letadla jen když je letadlo poblíž", "planeAlert": "Ukázat letadla jen když je letadlo blízko (nebo ve výhledu z okna)", "paKm": "Vzdálenost (km)", "paSec": "Jak dlouho ukázat (sekundy)", "paHint": "Obrazovka Letadla se pak sama neobjevuje ve střídání. Jakmile se letadlo přiblíží na zadanou vzdálenost, zařízení na ni přepne, otevře detail toho letadla a po zadané době se vrátí tam, kde bylo. Každé letadlo se ukáže jen jednou (za 15 minut). Funguje i bez automatického střídání. Výhled z okna se nastavuje na záložce Letadla a má přednost — letadlo, které z okna vidíte, se ukáže, i když je dál než jiné za domem. Dotyk během ukázky ji ukončí a zůstane se na letadlech.", "srcChmuM": "ČHMÚ masked (jen srážky dopadající na zem)", "srcDiff": "ČHMÚ ukazuje všechno, co radary vidí, plnou barvou — včetně srážek, které se vypaří dřív, než dopadnou, a kovadlin vzdálených bouřek. ČHMÚ masked je tentýž snímek, ale plnou barvou jen tam, kde podle měření ve výšce 2 km srážky opravdu padají na zem; zbytek je bledší. Pro otázku „bude u nás pršet?“ je masked přesnější, pro sledování bouřek je lepší plný.", "clockFont": "Písmo (všechny obrazovky)", "fontThin": "Inter tenké", "fontPixel": "pixelové (původní)", "altHdr": "Barvy podle výšky", "altBelow": "Pod 1. hladinou", "altB1": "1. hladina (m)", "altB2": "2. hladina (m)", "altB3": "3. hladina (m)", "altDef": "Výchozí barvy a hladiny", "altHint": "Barva u hladiny platí od té výšky nahoru až k další hladině. Výchozí je červená pod 2 km, oranžová 2–6 km, žlutá 6–10 km a modrá nad 10 km. Hladiny musí jít vzestupně. Legenda na displeji se přizpůsobí.", "altBad": "Hladiny musí jít vzestupně", "winHdr": "Výhled z okna", "winOn": "Používat výhled z okna", "winAz": "Směr okna", "dir0": "sever (S)", "dir45": "severovýchod (SV)", "dir90": "východ (V)", "dir135": "jihovýchod (JV)", "dir180": "jih (J)", "dir225": "jihozápad (JZ)", "dir270": "západ (Z)", "dir315": "severozápad (SZ)", "winW": "Šířka výhledu (°)", "winKm": "Dohled (km)", "winHint": "Výseč oblohy, kterou z okna vidíte: kam okno míří, jak široký úhel oblohy z něj vidíte (výseč je rozložená rovnoměrně kolem směru okna — 90° u okna na SV pokryje sever až východ) a jak daleko letadlo ještě rozeznáte. Na radaru letadel se zakreslí tenkými čarami. Letadlo ve výhledu má při upozornění přednost před bližším letadlem, které z okna vidět není.", "haUntested": "Tato funkce zatím nebyla vyzkoušena se skutečným Home Assistantem. Formát odpovídá dokumentaci integrace RESTful, ale pokud něco nesedí, dejte vědět.", "haOn": "Zpřístupnit data pro Home Assistant", "haHow": "Home Assistant si data sám stahuje (zhruba jednou za minutu) z adresy níže. Nic se nikam neposílá a nepotřebujete MQTT. Data jsou jen ke čtení: teplota, vítr, srážky, bouřka v okolí, nejbližší letadlo. Dokud je to zapnuté, zařízení hlídá i bouřku (jeden snímek ČHMÚ za 5 minut) a letadla poblíž (malý dotaz každých 30 s).", "haOpen": "otevřít", "haYamlHint": "Do souboru configuration.yaml v Home Assistantu vložte tohle a Home Assistant restartujte:", "haCopy": "Zkopírovat", "copied": "Zkopírováno", "stormSt": "Bouřka", "planeWatchSt": "Letadla poblíž", "srcIcons": "ikony letadel podle", "iconHdr": "Ikony letadel", "iconIntro": "Tvar ikony na radaru podle typu letadla, velikost odpovídá hmotnosti. Typ se pozná z kódu typu (A20N, B738, EC35…), u neznámého z kategorie, kterou vysílá transpondér. Barva ikony udává výšku (viz níže).", "iconCredit": "Rozdělení typů letadel do ikon vychází ze schématu ADS-B Radar (adsb-radar.com/help/icons.html). Tvary kreslí firmware sám, soubory ikon se nepřebírají.", "icNarrow": "úzkotrupé — A320, B737, B757", "icWide": "širokotrupé dvoumotorové — A330, A350, B767, B777, B787", "icQuad": "čtyřmotorové — A340, A380, B747", "icRegional": "regionální — CRJ, Embraer E-Jet, Fokker 100", "icBizjet": "business jet — Citation, Learjet, Falcon, Gulfstream", "icTwinprop": "turbovrtulové — ATR, Dash 8, King Air, L 410", "icLight": "malá letadla — Cessna, Piper, Zlin, ultralighty", "icMiltrans": "vojenská transportní — C-130, C-160, A400M", "icFreighter": "těžká nákladní — An-124, C-17, C-5, Il-76", "icFighter": "stíhačky a cvičné proudové", "icGlider": "kluzáky a motorové kluzáky", "icTilt": "konvertoplány — V-22, AW609", "icHeli": "vrtulníky", "icBalloon": "balony a vzducholodě", "icDrone": "drony", "icGeneric": "nezjištěný typ", "icNoTrack": "neznámý směr letu"});
Object.assign(D.en,{"tabHa": "Home Assistant", "wifiSaved": "Saved networks", "wifiSavedHint": "The device remembers up to three networks. They can be typed in even when out of range - set up the office network while still at home. When the link drops, after a minute the device tries the other saved networks, strongest first. An empty password field keeps the stored password. Saving does not disconnect the device.", "wifiSaveList": "Save networks", "passStored": "password stored", "passNone": "password", "ssidPh": "network name", "stormHdr": "Weather radar only for storms", "stormOnly": "Weather radar in the cycling only when rain/a storm is coming", "stormDbz": "Reflectivity threshold (dBZ)", "stormKm": "Distance from the device (km)", "stormHint": "Every 5 minutes the newest CHMU radar frame is checked. When an echo at least this strong lies within this distance, the weather radar joins the cycling; after two clear frames it drops out again. Works even with the Weather radar screen switched off above. Roughly: 20 dBZ light rain, 40 dBZ heavy rain, above 50 dBZ a storm with possible hail. The data is CHMU's, so Czechia and a little beyond. Needs automatic cycling on.", "paHdr": "Aircraft only when one is near", "planeAlert": "Show the aircraft only when one is near (or in the window view)", "paKm": "Distance (km)", "paSec": "How long to show it (seconds)", "paHint": "The Aircraft screen then never comes up in the cycling by itself. As soon as an aircraft comes this close, the device switches to it, opens that aircraft's detail and after the set time goes back to where it was. Each aircraft is shown once (per 15 minutes). Works without automatic cycling too. The window view is set on the Aircraft tab and takes priority - an aircraft you can see from the window is shown even if it is further away than another one behind the house. Touching the screen during the alert ends it and stays on the aircraft.", "srcChmuM": "CHMU masked (only rain reaching the ground)", "srcDiff": "CHMU shows everything the radars see at full colour - including rain that evaporates before it reaches the ground and the anvils of distant storms. CHMU masked is the same picture, but at full colour only where measurements at 2 km say the rain actually reaches the ground; the rest is paler. For 'will it rain here?' masked is more accurate, for watching storms the full one is better.", "clockFont": "Typeface (all screens)", "fontThin": "Inter thin", "fontPixel": "pixel (original)", "altHdr": "Colours by altitude", "altBelow": "Below level 1", "altB1": "Level 1 (m)", "altB2": "Level 2 (m)", "altB3": "Level 3 (m)", "altDef": "Default colours and levels", "altHint": "A level's colour applies from that altitude up to the next level. The default is red below 2 km, orange 2-6 km, yellow 6-10 km and blue above 10 km. Levels must ascend. The legend on the display follows.", "altBad": "The levels must ascend", "winHdr": "Window view", "winOn": "Use the window view", "winAz": "Window faces", "dir0": "north (N)", "dir45": "north-east (NE)", "dir90": "east (E)", "dir135": "south-east (SE)", "dir180": "south (S)", "dir225": "south-west (SW)", "dir270": "west (W)", "dir315": "north-west (NW)", "winW": "Width of the view (°)", "winKm": "Visibility (km)", "winHint": "The slice of sky you see from the window: which way the window faces, how wide an angle of sky you see from it (spread evenly around that direction - 90° on a north-east window covers north to east) and how far you can still make out an aircraft. It is drawn as thin lines on the aircraft radar. An aircraft in the view takes priority for the alert over a nearer one you cannot see.", "haUntested": "This feature has not been tested with a real Home Assistant yet. The format follows the RESTful integration's documentation, but if something does not fit, please report it.", "haOn": "Make the data available to Home Assistant", "haHow": "Home Assistant fetches the data itself (about once a minute) from the address below. Nothing is sent anywhere and no MQTT is needed. The data is read-only: temperature, wind, rain, storm nearby, nearest aircraft. While this is on, the device also watches for storms (one CHMU frame every 5 minutes) and nearby aircraft (a small request every 30 s).", "haOpen": "open", "haYamlHint": "Paste this into Home Assistant's configuration.yaml and restart Home Assistant:", "haCopy": "Copy", "copied": "Copied", "stormSt": "Storm", "planeWatchSt": "Aircraft nearby", "srcIcons": "aircraft icons after", "iconHdr": "Aircraft icons", "iconIntro": "The icon's shape follows the aircraft type and its size the weight. The type comes from the type code (A20N, B738, EC35...), or from the category the transponder sends when the type is unknown. The colour shows the altitude (see below).", "iconCredit": "The assignment of aircraft types to icons follows the scheme of ADS-B Radar (adsb-radar.com/help/icons.html). The shapes are drawn by the firmware itself; no icon files are used.", "icNarrow": "narrow-body - A320, B737, B757", "icWide": "wide-body twin - A330, A350, B767, B777, B787", "icQuad": "four-engine - A340, A380, B747", "icRegional": "regional jets - CRJ, Embraer E-Jet, Fokker 100", "icBizjet": "business jets - Citation, Learjet, Falcon, Gulfstream", "icTwinprop": "turboprops - ATR, Dash 8, King Air, L 410", "icLight": "light aircraft - Cessna, Piper, Zlin, ultralights", "icMiltrans": "military transport - C-130, C-160, A400M", "icFreighter": "heavy freighters - An-124, C-17, C-5, Il-76", "icFighter": "fighters and jet trainers", "icGlider": "gliders and motor gliders", "icTilt": "tiltrotors - V-22, AW609", "icHeli": "helicopters", "icBalloon": "balloons and airships", "icDrone": "drones", "icGeneric": "type not identified", "icNoTrack": "direction of flight unknown"});
Object.assign(D.sk,{"tabHa": "Home Assistant", "wifiSaved": "Uložené siete", "wifiSavedHint": "Zariadenie si pamätá až tri siete. Zadajte ich ručne aj mimo dosahu. Po výpadku skúsi po minúte ostatné uložené siete, najsilnejšiu ako prvú. Prázdne heslo ponechá uložené heslo. Uloženie sietí zariadenie neodpojí.", "wifiSaveList": "Uložiť siete", "passStored": "heslo uložené", "passNone": "heslo", "ssidPh": "názov siete", "stormHdr": "Meteoradar iba pri búrke", "stormOnly": "Meteoradar v striedaní iba pri blížiacich sa zrážkach alebo búrke", "stormDbz": "Prah odrazivosti (dBZ)", "stormKm": "Vzdialenosť od zariadenia (km)", "stormHint": "Každých 5 minút sa preverí najnovšia snímka ČHMÚ. Odraz nad zvoleným prahom v zadanom okolí pridá meteoradar do striedania; po dvoch pokojných snímkach sa vynechá. Funguje aj pri vypnutej obrazovke meteoradaru. Približne: 20 dBZ slabý dážď, 40 dBZ silný dážď, nad 50 dBZ možná búrka s krúpami. Platí iba v pokrytí ČHMÚ. Vyžaduje automatické striedanie.", "paHdr": "Lietadlá iba pri blízkom lietadle", "planeAlert": "Zobraziť lietadlá iba pri blízkom lietadle alebo vo výhľade z okna", "paKm": "Vzdialenosť (km)", "paSec": "Trvanie zobrazenia (sekundy)", "paHint": "Obrazovka Lietadlá sa sama nezaradí do striedania. Pri blízkom lietadle sa otvorí jeho detail a po zadanom čase sa vráti predchádzajúca obrazovka. Každé lietadlo sa ukáže raz za 15 minút. Výhľad z okna má prednosť pred bližším lietadlom za domom. Dotyk ukážku ukončí. Funguje aj bez automatického striedania.", "srcChmuM": "ČHMÚ masked (iba zrážky dopadajúce na zem)", "srcDiff": "ČHMÚ ukazuje všetky radarové odrazy plnou farbou. ČHMÚ masked zobrazuje plnou farbou iba miesta, kde podľa merania vo výške 2 km zrážky dopadajú na zem; ostatné odrazy sú bledšie. Na sledovanie búrok je vhodný plný ČHMÚ, na otázku, či bude pršať, masked.", "clockFont": "Písmo (všetky obrazovky)", "fontThin": "Inter tenké", "fontPixel": "pixelové (pôvodné)", "altHdr": "Farby podľa výšky", "altBelow": "Pod 1. hladinou", "altB1": "1. hladina (m)", "altB2": "2. hladina (m)", "altB3": "3. hladina (m)", "altDef": "Predvolené farby a hladiny", "altHint": "Farba platí od danej výšky po ďalšiu hladinu. Predvolené: červená pod 2 km, oranžová 2–6 km, žltá 6–10 km a modrá nad 10 km. Hladiny musia byť vzostupné; legenda sa prispôsobí.", "altBad": "Hladiny musia byť vzostupné", "winHdr": "Výhľad z okna", "winOn": "Používať výhľad z okna", "winAz": "Smer okna", "dir0": "sever (S)", "dir45": "severovýchod (SV)", "dir90": "východ (V)", "dir135": "juhovýchod (JV)", "dir180": "juh (J)", "dir225": "juhozápad (JZ)", "dir270": "západ (Z)", "dir315": "severozápad (SZ)", "winW": "Šírka výhľadu (°)", "winKm": "Dohľad (km)", "winHint": "Výsek oblohy viditeľný z okna: smer, šírka uhla a vzdialenosť, na ktorú lietadlo ešte rozoznáte. Výsek je rovnomerne okolo smeru a zakreslí sa do radaru. Lietadlo vo výhľade má pri upozornení prednosť pred bližším lietadlom mimo výhľadu.", "haUntested": "Funkcia ešte nebola overená so skutočným Home Assistantom. Formát vychádza z integrácie RESTful.", "haOn": "Sprístupniť dáta pre Home Assistant", "haHow": "Home Assistant si dáta sťahuje zo zadanej adresy približne raz za minútu. Dostupné sú teplota, vietor, zrážky, búrky a najbližšie lietadlo. MQTT netreba. Zapnutá funkcia sleduje aj búrky každých 5 minút a lietadlá každých 30 sekúnd.", "haOpen": "otvoriť", "haYamlHint": "Vložte nasledujúci text do configuration.yaml a reštartujte Home Assistant:", "haCopy": "Skopírovať", "copied": "Skopírované", "stormSt": "Búrka", "planeWatchSt": "Lietadlá v okolí", "srcIcons": "ikony lietadiel podľa", "iconHdr": "Ikony lietadiel", "iconIntro": "Tvar ikony zodpovedá typu lietadla a veľkosť jeho hmotnosti. Typ sa určí z kódu alebo kategórie transpondéra. Farba označuje výšku.", "iconCredit": "Rozdelenie typov vychádza zo schémy ADS-B Radar (adsb-radar.com/help/icons.html). Tvary kreslí firmvér; súbory ikon sa nepreberajú.", "icNarrow": "úzkotrupé — A320, B737, B757", "icWide": "širokotrupé dvojmotorové — A330, A350, B767, B777, B787", "icQuad": "štvormotorové — A340, A380, B747", "icRegional": "regionálne — CRJ, Embraer E-Jet, Fokker 100", "icBizjet": "business jet — Citation, Learjet, Falcon, Gulfstream", "icTwinprop": "turbovrtuľové — ATR, Dash 8, King Air, L 410", "icLight": "malé lietadlá — Cessna, Piper, Zlin, ultralighty", "icMiltrans": "vojenské transportné — C-130, C-160, A400M", "icFreighter": "ťažké nákladné — An-124, C-17, C-5, Il-76", "icFighter": "stíhačky a cvičné prúdové", "icGlider": "klzáky a motorové klzáky", "icTilt": "konvertoplány — V-22, AW609", "icHeli": "vrtuľníky", "icBalloon": "balóny a vzducholode", "icDrone": "drony", "icGeneric": "nezistený typ", "icNoTrack": "neznámy smer letu"});
let L="cs", CFG={}, TAB="tCtl";
Object.assign(D.cs,{"scrPrice": "Cena elektřiny", "scrMix": "Výroba elektřiny", "energyTitle": "Elektřina", "energyHint": "Tyto obrazovky jsou dostupné pouze v češtině. Ceny platí pro český trh; původní geografické omezení zdrojů zůstává.", "priceFee": "Příplatek dodavatele (Kč/MWh)", "priceVat": "DPH (%)", "priceHint": "Nula znamená čistou burzovní cenu bez příplatku a DPH. Nejde o celkovou cenu vyúčtování.", "mixCountry": "Kód země výroby (cz, sk, de…)"});
Object.assign(D.en,{"scrPrice": "Electricity price", "scrMix": "Electricity generation", "energyTitle": "Electricity", "energyHint": "These screens are available only in Czech. Prices cover the Czech market; the original geographic limits still apply.", "priceFee": "Supplier surcharge (CZK/MWh)", "priceVat": "VAT (%)", "priceHint": "Zero shows the exchange price without surcharge or VAT. This is not the total billed price.", "mixCountry": "Generation country code (cz, sk, de…)"});
Object.assign(D.sk,{"scrPrice": "Cena elektriny", "scrMix": "Výroba elektriny", "energyTitle": "Elektrina", "energyHint": "Tieto obrazovky sú dostupné iba v češtine. Ceny platia pre český trh; pôvodné geografické obmedzenia zostávajú.", "priceFee": "Príplatok dodávateľa (Kč/MWh)", "priceVat": "DPH (%)", "priceHint": "Nula zobrazí burzovú cenu bez príplatku a DPH. Nejde o celkovú cenu vyúčtovania.", "mixCountry": "Kód krajiny výroby (cz, sk, de…)"});
Object.assign(D.cs,{"gestureHint": "Na displeji: vlevo další obrazovka, vpravo předchozí. Nahoru/dolů na mapě přiblíží/oddálí. Dlouhý stisk levé/pravé poloviny přepíná zpět/vpřed. Body obrazovek mizí po 5 s, body zoomu vpravo po 10 s. Kalendářem listují jeho šipky.", "remoteHint": "Rozsah mění zoom map, v kalendáři listuje dny/měsíci a u ceny elektřiny (jen CZ) přepíná dnes/zítra. Ruční ovládání pozastaví automatické střídání."});
Object.assign(D.en,{"gestureHint": "On the display: left for next screen, right for previous. Up/down zooms maps in/out. Long-press the left/right half for previous/next. Screen dots hide after 5 s, right-side zoom dots after 10 s. Use the calendar arrows to browse dates.", "remoteHint": "Range changes map zoom, browses calendar days/months, or switches electricity prices (Czech only) between today/tomorrow. Manual control pauses automatic cycling."});
Object.assign(D.sk,{"gestureHint": "Na displeji: doľava ďalšia obrazovka, doprava predchádzajúca. Hore/dole na mape priblíži/oddiali. Dlhý stisk ľavej/pravej polovice prepína späť/vpred. Bodky obrazoviek miznú po 5 s, bodky zoomu vpravo po 10 s. V kalendári listujú jeho šípky.", "remoteHint": "Rozsah mení zoom máp, v kalendári listuje dni/mesiace a pri cene elektriny (iba CZ) prepína dnes/zajtra. Ručné ovládanie pozastaví automatické striedanie."});
const $=id=>document.getElementById(id);

function setLang(v){L=(v==2||v=="sk")?"sk":(v==1||v=="en")?"en":"cs";$("uiLang").value=String({cs:0,en:1,sk:2}[L]);
 document.documentElement.lang=L;
 document.querySelectorAll("[data-i18n]").forEach(e=>{const t=D[L][e.dataset.i18n];if(t)e.textContent=t;});
 energyLanguage();pwState();calendarLabels();notificationLabels();updateActions();if(pairDeadline)tickPairCode();status();if(CFG.wifi)wifiSlots();iconLegend();}

// Shared fixed action area; immediate settings keep their existing autosave.
function updateActions(){
 const manual=["tLoc","tScr","tCal","tNotify","tSys"].includes(TAB);
 $("saveButton").classList.toggle("hide",!manual);
 $("saveMode").classList.toggle("hide",manual||TAB=="tCtl");
 $("saveButton").textContent=TAB=="tCal"?D[L].calSaveEnable:D[L].save;
}
let committing=false;
async function commitSettings(){
 if(committing)return;
 committing=true;$("saveButton").disabled=true;
 try{if(TAB=="tCal")await saveCalendars();else await save();}
 finally{committing=false;$("saveButton").disabled=false;}
}
function showTab(id){
 TAB=id;
 document.querySelectorAll("section.tab").forEach(s=>s.classList.toggle("hide",s.id!=id));
 document.querySelectorAll("#tabs button").forEach(b=>{
  const active=b.dataset.tab==id;
  b.classList.toggle("on",active);b.setAttribute("aria-selected",String(active));b.tabIndex=active?0:-1;
 });
 updateActions();window.scrollTo(0,0);
}
const tabButtons=Array.from(document.querySelectorAll("#tabs button"));
tabButtons.forEach((b,i)=>{
 b.onclick=()=>showTab(b.dataset.tab);
 b.onkeydown=e=>{
  let next;if(e.key=="ArrowRight")next=(i+1)%tabButtons.length;
  else if(e.key=="ArrowLeft")next=(i+tabButtons.length-1)%tabButtons.length;
  else if(e.key=="Home")next=0;else if(e.key=="End")next=tabButtons.length-1;else return;
  e.preventDefault();const target=tabButtons[next];showTab(target.dataset.tab);target.focus();target.scrollIntoView({block:"nearest",inline:"nearest"});
 };
});
// Associate row labels with their controls without changing API field IDs.
document.querySelectorAll(".row").forEach(row=>{
 const label=row.querySelector("label"),input=row.querySelector("input,select");
 if(label&&input&&!label.contains(input))label.htmlFor=input.id;
});

function msg(t,c){$("msg").textContent=t;$("msg").className=c||"";setTimeout(()=>{$("msg").textContent=""},4000);}
function rgb565ToHex(v){const r=(v>>11&31)*255/31|0,g=(v>>5&63)*255/63|0,b=(v&31)*255/31|0;
 return "#"+[r,g,b].map(x=>x.toString(16).padStart(2,"0")).join("");}
function hexToRgb565(h){const r=parseInt(h.substr(1,2),16),g=parseInt(h.substr(3,2),16),b=parseInt(h.substr(5,2),16);
 return ((r>>3)<<11)|((g>>2)<<5)|(b>>3);}

const SCR=[["scrClock",0],["scrPlanes",1],["scrMeteo",2],["scrForecast",3],
           ["scrCalendar",4],["scrPrice",5],["scrMix",6],["settings",7]];
function energyLanguage(){
 ["energyPriceOption","energyMixOption","energySettings"].forEach(id=>$(id).classList.toggle("hide",L!=="cs"));
}
function drawScrBtns(cur,enabled){
 $("scrBtns").innerHTML=SCR.filter(([,i])=>L==="cs"||(i!==5&&i!==6)).map(([k,i])=>{
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
 ["clockFont","change","clockFont",e=>+e.value],
 ["stormOnly","change","stormOnly",e=>e.checked],
 ["stormDbz","change","stormDbz",e=>+e.value],
 ["stormKm","change","stormKm",e=>+e.value],
 ["planeAlert","change","planeAlert",e=>e.checked],
 ["planeAlertKm","change","planeAlertKm",e=>+e.value],
 ["planeAlertSec","change","planeAlertSec",e=>+e.value],
 ["windowOn","change","windowOn",e=>e.checked],
 ["windowAz","change","windowAz",e=>+e.value],
 ["windowWidth","change","windowWidth",e=>+e.value],
 ["windowKm","change","windowKm",e=>+e.value],
 ["haEnabled","change","haEnabled",e=>e.checked],
];
// The altitude levels and the colours each go as one set - applied one value
// at a time, the levels would pass through an out-of-order state and be refused.
["ab0","ab1","ab2"].forEach(id=>$(id).addEventListener("change",()=>altSave("bands")));
["ac0","ac1","ac2","ac3"].forEach(id=>$(id).addEventListener("change",()=>altSave("colors")));
function altBands(){return[+$("ab0").value,+$("ab1").value,+$("ab2").value];}
function altColors(){return[0,1,2,3].map(i=>hexToRgb565($("ac"+i).value));}
function altOk(b){return b[0]>0&&b[0]<b[1]&&b[1]<b[2];}
function altSave(what){
 if(what=="bands"){const b=altBands();if(!altOk(b)){msg(D[L].altBad,"warn");return;}autoSave("altBands",b);}
 else autoSave("altColors",altColors());
}
function altDefaults(){
 [2000,6000,10000].forEach((v,i)=>$("ab"+i).value=v);
 [0xF800,0xFC00,0xFFE0,0x001F].forEach((v,i)=>$("ac"+i).value=rgb565ToHex(v));
 autoSave("altBands",altBands());autoSave("altColors",altColors());
}
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
 $("sPrice").checked=!!CFG.screens.price;$("sMix").checked=!!CFG.screens.mix;
 $("priceFee").value=CFG.priceFee??0;$("priceVat").value=CFG.priceVat??0;
 $("mixCountry").value=CFG.mixCountry||"cz";
 $("ntEnabled").checked=!!(CFG.notifications||{}).enabled;
 $("ntSeconds").value=(CFG.notifications||{}).seconds??15;
 notificationLabels();
 $("ntTestBtn").disabled=$("ntPairBtn").disabled=$("ntCodeBtn").disabled=!!CFG.apMode;
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
 $("clockFont").value=(CFG.clockFont!=null)?CFG.clockFont:1;
 (CFG.altBands||[2000,6000,10000]).forEach((v,i)=>$("ab"+i).value=v);
 (CFG.altColors||[0xF800,0xFC00,0xFFE0,0x001F]).forEach((v,i)=>$("ac"+i).value=rgb565ToHex(v));
 $("stormOnly").checked=!!CFG.stormOnly;$("stormDbz").value=CFG.stormDbz||20;$("stormKm").value=CFG.stormKm||50;
 $("planeAlert").checked=!!CFG.planeAlert;$("planeAlertKm").value=CFG.planeAlertKm||5;$("planeAlertSec").value=CFG.planeAlertSec||15;
 $("windowOn").checked=!!CFG.windowOn;// Eight directions; an older value between them snaps to the nearest one.
 $("windowAz").value=String((Math.round(((CFG.windowAz!=null)?CFG.windowAz:180)/45)%8)*45);
 $("windowWidth").value=CFG.windowWidth||90;$("windowKm").value=CFG.windowKm||15;
 $("haEnabled").checked=!!CFG.haEnabled;haInfo();
 wifiSlots();
 if(CFG.apMode)$("wifiSlots").classList.add("hide");
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
 if(pairDeadline&&s.notificationPairingComplete===true){
  clearPairCode(D[L].ntCodeDone);CFG.notifications={...(CFG.notifications||{}),paired:true,enabled:true};
  $("ntEnabled").checked=true;notificationLabels();
 }else if(pairDeadline&&s.notificationPairingActive===false){clearPairCode(D[L].ntCodeClosed);}
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
  [D[L].stormSt,s.storm],[D[L].planeWatchSt,s.planeWatch],
  ["Firmware","v"+s.version],[D[L].board,s.board]];
 $("ntStatus").textContent=D[L].ntQueue+": "+(s.notificationCount||0)+" · "+D[L].ntBridge+": "+(s.notificationBridgeAge??-1)+" · "+D[L].ntMemory+": "+(s.notificationMemory||0)+" B";
 (s.calendarSources||[]).forEach((c,i)=>{if(i<3)$("cal"+i+"Status").textContent=c.state+(c.unsupportedTimezone?" · TZID: "+c.unsupportedTimezone:"")+(c.updated?" · "+new Date(c.updated*1000).toLocaleString(L):"");});
 if(L==="cs"){rows.push([D[L].scrPrice,s.price||"–"],[D[L].scrMix,s.mix||"–"]);}
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
 calendars:calendarSources(),
 clockFont:+$("clockFont").value,altBands:altBands(),altColors:altColors(),
 stormOnly:$("stormOnly").checked,stormDbz:+$("stormDbz").value,stormKm:+$("stormKm").value,
 planeAlert:$("planeAlert").checked,planeAlertKm:+$("planeAlertKm").value,planeAlertSec:+$("planeAlertSec").value,
 windowOn:$("windowOn").checked,windowAz:+$("windowAz").value,windowWidth:+$("windowWidth").value,windowKm:+$("windowKm").value,
 haEnabled:$("haEnabled").checked,
 priceFee:+$("priceFee").value,priceVat:+$("priceVat").value,mixCountry:$("mixCountry").value,
 screens:{clock:$("sClock").checked,planes:$("sPlanes").checked,meteo:$("sMeteo").checked,
  forecast:$("sForecast").checked,calendar:$("sCalendar").checked,price:$("sPrice").checked,mix:$("sMix").checked}};}

function calendarSources(){return [0,1,2].map(i=>({name:$("cal"+i+"Name").value,enabled:$("cal"+i+"On").checked,color:hexToRgb565($("cal"+i+"Color").value),url:$("cal"+i+"Url").value.trim(),clearUrl:$("cal"+i+"Clear").checked}));}
function calendarLabels(){
 const enabled=!!(CFG.screens||{}).calendar;
 $("calScreenState").textContent=enabled?D[L].calScreenOn:D[L].calScreenOff;
 $("calScreenState").className="hint "+(enabled?"ok":"warn");
 for(let i=0;i<3;i++){
  const c=(CFG.calendars||[])[i]||{};
  $("cal"+i+"Url").placeholder=c.hasUrl?D[L].calConfigured:D[L].calMissing;
 }
}
function rememberCalendars(calendars){
 CFG.calendars=calendars.map((c,i)=>({name:c.name,color:c.color,enabled:c.enabled,
   hasUrl:!!c.url||(!c.clearUrl&&!!((CFG.calendars||[])[i]||{}).hasUrl)}));
 for(let i=0;i<3;i++){$("cal"+i+"Url").value="";$("cal"+i+"Clear").checked=false;}
}
async function saveCalendars(){
 try{
  const calendars=calendarSources();
  if(!calendars.some((c,i)=>c.enabled&&(c.url||(!c.clearUrl&&!!((CFG.calendars||[])[i]||{}).hasUrl)))){
   msg(D[L].calNeedSource,"warn");return;
  }
  const payload={calendars,calendarView:+$("calendarView").value,calendarRefresh:+$("calendarRefresh").value,
   screens:{calendar:true},password:$("adminPass").value};
  const r=await fetch("/api/config",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify(payload)});
  const result=await r.json();
  if(!r.ok){msg(D[L].failed+" ("+(result.error||r.status)+")","err");return;}
  rememberCalendars(calendars);
  CFG.screens={...(CFG.screens||{}),calendar:true};$("sCalendar").checked=true;
  calendarLabels();msg(result.restart?D[L].calEnabledSaved:D[L].saved,"ok");
  if(!result.restart){await refreshCalendars();await status();}
 }catch(e){msg(D[L].failed,"err");}
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
 if(pairDeadline)clearPairCode(D[L].ntCodeCancelled);
 $("ntEnabled").checked=notifications.enabled;
 return notifications;
}
async function testNotification(){
 try{$("ntEnabled").checked=true;await saveNotificationSettings();const r=await fetch("/api/notifications/test",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({password:$("adminPass").value})});
 if(!r.ok)throw Error(r.status);msg(D[L].autoSaved,"ok");await status();}catch(e){msg(D[L].failed+" ("+e.message+")","err");}
}
let pairDeadline=0,pairTimer=null;
function clearPairCode(text){
 pairDeadline=0;clearInterval(pairTimer);pairTimer=null;
 $("ntCode").textContent="";$("ntCode").classList.add("hide");$("ntCodeCancel").classList.add("hide");
 $("ntCodeExpiry").textContent=text||"";
}
function tickPairCode(){
 const seconds=Math.max(0,Math.ceil((pairDeadline-Date.now())/1000));
 if(!seconds){clearPairCode(D[L].ntCodeExpired);return;}
 $("ntCodeExpiry").textContent=D[L].ntCodeRemaining+seconds;
}
async function startPairCode(){
 $("ntCodeBtn").disabled=true;
 try{
  const r=await fetch("/api/notifications/pair/start",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({password:$("adminPass").value})});
  if(!r.ok)throw Error(r.status);
  const result=await r.json();
  if(!/^[0-9]{6}$/.test(result.code)||!Number.isInteger(result.expiresIn)||result.expiresIn<1||result.expiresIn>120)throw Error("response");
  clearPairCode();$("ntCode").textContent=result.code.slice(0,3)+" "+result.code.slice(3);
  $("ntCode").classList.remove("hide");$("ntCodeCancel").classList.remove("hide");
  $("ntCodeAddress").textContent=new URL(location.href).origin;
  pairDeadline=Date.now()+result.expiresIn*1000;tickPairCode();pairTimer=setInterval(tickPairCode,1000);
 }catch(e){msg(D[L].failed+" ("+e.message+")","err");}
 finally{$("ntCodeBtn").disabled=!!CFG.apMode;}
}
async function cancelPairCode(){
 try{
  const r=await fetch("/api/notifications/pair/cancel",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({password:$("adminPass").value})});
  if(!r.ok)throw Error(r.status);clearPairCode(D[L].ntCodeCancelled);
 }catch(e){msg(D[L].failed+" ("+e.message+")","err");}
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
  if(pairDeadline)clearPairCode(D[L].ntCodeCancelled);
  rememberCalendars(payload.calendars);
  CFG.screens=payload.screens;
  $("newPass").value="";
  setLang($("uiLang").value);
  msg(D[L].saved,"ok");
  // A restart is already queued after screen/location changes. Do not request
  // another config while the device is shutting down its web server.
  if(!result.restart)await load();
 }catch(e){msg(D[L].failed,"err");}
}
function escapeHtml(value){return String(value).replace(/[&<>"']/g,c=>({"&":"&amp;","<":"&lt;",">":"&gt;",'"':"&quot;","'":"&#39;"}[c]));}
async function scan(){
 try{const networks=await(await fetch("/api/scan")).json();
 $("ssidList").innerHTML=networks.map(n=>"<option value='"+escapeHtml(n.ssid)+"'>"+escapeHtml(n.ssid)+" ("+Number(n.rssi)+")</option>").join("");
 }catch(e){$("ssidList").innerHTML="";}
}
// The three remembered networks. Passwords never come back from the device,
// so an empty field means "keep what is stored" and the placeholder says
// whether there is anything stored.
function wifiSlots(){
 (CFG.wifi||[]).forEach((w,i)=>{if(!$("ws"+i))return;$("ws"+i).value=w.ssid||"";$("wp"+i).value="";
  $("ws"+i).placeholder=D[L].ssidPh;$("wp"+i).placeholder=w.hasPass?D[L].passStored:D[L].passNone;});
}
async function saveWifiList(){
 const nets=[0,1,2].map(i=>({ssid:$("ws"+i).value.trim(),pass:$("wp"+i).value}));
 try{const r=await fetch("/api/wifilist",{method:"POST",headers:{"Content-Type":"application/json"},body:JSON.stringify({nets})});
 msg(r.ok?D[L].saved:D[L].failed,r.ok?"ok":"err");
 if(r.ok){const c=await(await fetch("/api/config")).json();CFG.wifi=c.wifi;wifiSlots();}}catch(e){msg(D[L].failed,"err")}
}
// Home Assistant: the address to poll and a ready configuration.yaml block.
function haInfo(){
 const host=(CFG.ip&&CFG.ip!="0.0.0.0")?CFG.ip:"meteoplaneradar.local";
 const url="http://"+host+"/api/ha";
 $("haUrl").textContent=url;$("haLink").href=url;
 const q='"';
 const S=(n,t,u,dc,extra)=>"      - name: "+q+n+q+"\n        unique_id: "+n.toLowerCase().replace(/[^a-z0-9]+/g,"_")+
  "\n        value_template: "+q+"{{ "+t+" }}"+q+"\n"+(u?"        unit_of_measurement: "+q+u+q+"\n":"")+
  (dc?"        device_class: "+dc+"\n":"")+(extra||"");
 // Weather is null until the first forecast lands - unavailable, not zero.
 const av=k=>"        availability: "+q+"{{ value_json."+k+" is not none }}"+q+"\n";
 $("haYaml").textContent=
  "rest:\n  - resource: "+url+"\n    scan_interval: 60\n    sensor:\n"+
  S("MPR Teplota","value_json.temperature","°C","temperature","        state_class: measurement\n"+av("temperature"))+
  S("MPR Vitr","value_json.wind_kmh","km/h","wind_speed","        state_class: measurement\n"+av("wind_kmh"))+
  S("MPR Srazky","value_json.precip_mm","mm","precipitation",av("precip_mm"))+
  S("MPR Bourka max dBZ","value_json.storm.max_dbz | default(0)","dBZ")+
  S("MPR Bourka vzdalenost","value_json.storm.nearest_km","km","distance",
   "        availability: "+q+"{{ value_json.storm.nearest_km is defined and value_json.storm.nearest_km is not none }}"+q+"\n")+
  S("MPR Letadla v okoli","value_json.aircraft.count")+
  S("MPR Nejblizsi letadlo","value_json.aircraft.callsign | default('-', true)",null,null,
   "        json_attributes_path: "+q+"$.aircraft"+q+"\n        json_attributes:\n"+
   ["distance_km","altitude_m","speed_kmh","type","kind","bearing","in_window"].map(a=>"          - "+a+"\n").join(""))+
  "    binary_sensor:\n"+
  "      - name: "+q+"MPR Bourka"+q+"\n        unique_id: mpr_bourka\n        value_template: "+q+"{{ 'on' if value_json.storm.active else 'off' }}"+q+"\n        device_class: safety\n"+
  "      - name: "+q+"MPR Letadlo nablizku"+q+"\n        unique_id: mpr_letadlo_nablizku\n        value_template: "+q+"{{ 'on' if value_json.aircraft.alert else 'off' }}"+q+"\n";
}
// --- Aircraft icon legend ----------------------------------------------------
// The same polygon tables PlaneIcon.cpp draws on the display, so the legend
// cannot drift from what the radar actually shows. A part is
// [count, mirror, x0,y0, x1,y1, ...] with y pointing forward (nose up).
const SH={"GENERIC":[10,0,0,12,3,1,13,-8,3,-5,3,-7,0,-12,-3,-7,-3,-5,-13,-8,-3,1,0],"NARROW":[7,0,0,14,2,11,2,-10,1,-14,-1,-14,-2,-10,-2,11,4,1,2,4,13,-3,13,-5,2,-1,4,1,4,1,6,0,6,-3,4,-3,4,1,1,-9,6,-12,6,-14,1,-12,0],"WIDE":[7,0,0,14,3,10,3,-10,1,-14,-1,-14,-3,-10,-3,10,4,1,3,4,15,-4,15,-6,3,-1,4,1,5,2,8,1,8,-3,5,-3,4,1,1,-9,7,-12,7,-14,1,-12,0],"QUAD":[7,0,0,14,3,10,3,-10,1,-14,-1,-14,-3,-10,-3,10,4,1,3,4,15,-4,15,-6,3,-1,4,1,5,2,7,1,7,-2,5,-2,4,1,9,-1,11,-2,11,-5,9,-5,4,1,1,-9,7,-12,7,-14,1,-12,0],"REGIONAL":[7,0,0,13,2,10,2,-9,1,-13,-1,-13,-2,-9,-2,10,4,1,2,3,11,-2,11,-4,2,-1,4,1,2,-5,4,-5,4,-9,2,-9,4,1,0,-11,6,-12,6,-13,0,-13,0],"BIZJET":[7,0,0,12,2,8,2,-8,1,-12,-1,-12,-2,-8,-2,8,4,1,2,2,10,-4,10,-5,2,-1,4,1,2,-5,4,-5,4,-8,2,-8,4,1,0,-10,5,-12,5,-13,0,-12,0],"TWINPROP":[6,0,0,13,2,10,2,-11,0,-13,-2,-11,-2,10,4,1,2,3,14,2,14,-1,2,-1,4,1,4,6,6,6,6,-1,4,-1,4,1,1,-9,6,-10,6,-12,1,-12,0],"LIGHT":[6,0,0,10,2,8,2,-9,0,-11,-2,-9,-2,8,4,1,2,5,14,5,14,2,2,2,4,1,1,-8,5,-8,5,-10,1,-10,0],"MILTRANS":[6,0,0,13,3,10,3,-10,0,-13,-3,-10,-3,10,4,1,3,4,16,3,16,0,3,0,4,1,5,7,7,7,7,0,5,0,4,1,10,6,12,6,12,0,10,0,4,1,1,-9,7,-10,7,-12,1,-12,0],"FREIGHTER":[7,0,0,14,3,10,3,-10,1,-14,-1,-14,-3,-10,-3,10,4,1,3,5,16,-1,16,-3,3,1,4,1,6,3,8,2,8,-1,6,-1,4,1,11,1,13,0,13,-3,11,-3,4,1,0,-11,7,-13,7,-14,0,-14,0],"FIGHTER":[6,0,0,13,2,8,2,-11,1,-13,-1,-13,-2,-11,3,1,1,6,10,-6,1,-6,3,1,1,-8,5,-12,1,-12,0],"GLIDER":[6,0,0,10,1,8,1,-10,0,-11,-1,-10,-1,8,4,1,1,3,17,2,17,1,1,1,4,1,0,-9,4,-9,4,-10,0,-10,0],"TILT":[6,0,0,12,2,9,2,-10,0,-12,-2,-10,-2,9,4,1,2,2,10,2,10,-1,2,-1,4,1,1,-8,5,-9,5,-11,1,-11,0]};
const ICONS=[ // [shape, scale, key] - order and sizes as on the display
 ["NARROW",1,"icNarrow"],["WIDE",1.25,"icWide"],["QUAD",1.4,"icQuad"],["REGIONAL",.95,"icRegional"],
 ["BIZJET",.85,"icBizjet"],["TWINPROP",.9,"icTwinprop"],["LIGHT",.8,"icLight"],["MILTRANS",1.15,"icMiltrans"],
 ["FREIGHTER",1.4,"icFreighter"],["FIGHTER",.9,"icFighter"],["GLIDER",.85,"icGlider"],["TILT",1,"icTilt"],
 ["HELI",1,"icHeli"],["BALLOON",1,"icBalloon"],["DRONE",1,"icDrone"],["GENERIC",1,"icGeneric"],["UNKNOWN",1,"icNoTrack"]];
function iconSvg(name,k){
 const c="#e8edf4";let b="";
 if(name=="HELI")b="<circle r='11' fill='none' stroke='"+c+"' stroke-width='1.3'/><circle r='4' fill='"+c+"'/><path d='M0 0V13M-3 13H3' stroke='"+c+"' stroke-width='2'/>";
 else if(name=="BALLOON")b="<circle cy='-3' r='7' fill='"+c+"'/><path d='M-3 3L-2 7M3 3L2 7' stroke='"+c+"'/><rect x='-2' y='7' width='5' height='3' fill='"+c+"'/>";
 else if(name=="DRONE")b="<path d='M-7-7L7 7M-7 7L7-7' stroke='"+c+"' stroke-width='1.3'/>"+[[-7,-7],[7,-7],[-7,7],[7,7]].map(p=>"<circle cx='"+p[0]+"' cy='"+p[1]+"' r='3' fill='none' stroke='"+c+"'/>").join("")+"<rect x='-2' y='-2' width='5' height='5' fill='"+c+"'/>";
 else if(name=="UNKNOWN")b="<circle r='7' fill='none' stroke='"+c+"' stroke-width='1.3'/><circle r='2' fill='"+c+"'/>";
 else{const a=SH[name];let i=0;while(a[i]){const n=a[i],m=a[i+1],v=a.slice(i+2,i+2+2*n);
  for(const sg of (m?[1,-1]:[1]))b+="<polygon fill='"+c+"' points='"+v.map((x,j)=>j%2?(-x*k).toFixed(1):(sg*x*k).toFixed(1)).reduce((o,x,j)=>o+(j%2?","+x+" ":x),"")+"'/>";
  i+=2+2*n;}}
 return "<svg viewBox='-21 -21 42 42' width='42' height='42' style='flex:0 0 42px'>"+b+"</svg>";
}
function iconLegend(){
 $("iconLegend").innerHTML=ICONS.map(([n,k,key])=>"<div class='chk' style='gap:10px'>"+iconSvg(n,k)+
  "<span style='font-size:14px'>"+D[L][key]+"</span></div>").join("");
}
function copyHa(){
 const t=$("haYaml").textContent;
 if(navigator.clipboard&&window.isSecureContext){navigator.clipboard.writeText(t).then(()=>msg(D[L].copied,"ok"));return;}
 // Plain http on the LAN is not a secure context, where the clipboard API is
 // missing - fall back to selecting the text the old way.
 const r=document.createRange();r.selectNodeContents($("haYaml"));
 const sel=window.getSelection();sel.removeAllRanges();sel.addRange(r);
 try{document.execCommand("copy");msg(D[L].copied,"ok");}catch(e){}
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
