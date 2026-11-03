import {readFileSync} from 'node:fs';
import vm from 'node:vm';
import assert from 'node:assert/strict';
import {webcrypto} from 'node:crypto';

const page = readFileSync(new URL('../MeteoPlaneRadar/WebPage.h', import.meta.url), 'utf8');
const version = readFileSync(new URL('../MeteoPlaneRadar/Version.h', import.meta.url), 'utf8').match(/#define FW_VERSION "([^"]+)"/)[1];
const script = page.match(/<script>([\s\S]*?)<\/script>/)[1];
const elements = new Map([...page.matchAll(/id="([^"]+)"/g)].map(([,id]) => [id, {
  value: '0', checked: false, classList: {add(){}, remove(){}, toggle(){}},
  addEventListener(){}, textContent: '', innerHTML: '', style: {}
}]));
const translations = [...page.matchAll(/data-i18n="([^"]+)"/g)]
  .map(([,key]) => ({dataset:{i18n:key}, textContent:''}));
const downloads=[];
const document = {
  createElement() { return {click(){downloads.push({href:this.href,name:this.download});}}; },
  documentElement: {},
  getElementById(id) { assert(elements.has(id), `Missing DOM element: ${id}`); return elements.get(id); },
  querySelectorAll(selector) { return selector === '[data-i18n]' ? translations : []; }
};
const context = vm.createContext({document, crypto:webcrypto, URL, Blob, location:{href:'http://192.168.1.42/'}, window:{scrollTo(){}}, setInterval(){}, clearInterval(){}, setTimeout(){},
  fetch: async () => ({json: async () => ({screen:4,enabled:[true,true,true,true,true],range:''})})});
// load is exercised below with a realistic config; all other initial handlers
// execute so missing DOM references left by screen removal fail immediately.
vm.runInContext(script.replace(/load\(\);\s*$/, ''), context);
const run = code => vm.runInContext(code, context);
for (const lang of ['cs','en','sk']) {
  run(`setLang('${lang}')`);
  assert.equal(document.documentElement.lang, lang);
  for (const key of run('Object.keys(D.cs)')) assert(run(`D.${lang}[${JSON.stringify(key)}]`), `Missing ${lang} translation: ${key}`);
  for (const el of translations) assert(el.textContent, `Untranslated ${el.dataset.i18n}`);
}
assert.equal(elements.get('uiLang').value, '2');
run('drawScrBtns(4,[true,true,true,true,true])');
assert.equal((elements.get('scrBtns').innerHTML.match(/<button/g) || []).length, 6);
assert(elements.get('scrBtns').innerHTML.includes('Nastavenia'));
const body = run('body()');
assert.equal(body.lang, 2);
assert.deepEqual(Object.keys(body.screens), ['clock','planes','meteo','forecast','calendar']);
for (const removed of ['priceFee','priceVat','mixCountry']) assert(!(removed in body));
assert.equal(run('SCR.length'), 6);
assert.equal(run('SCR[4][1]'), 4);
for (const removed of ['sPrice','sMix','tEnergy']) assert(!elements.has(removed));

context.fetch = async url => ({json:async()=>url === '/api/config' ? {
  version,lang:2,lat:48.15,lon:17.11,screens:{clock:true,planes:true,meteo:true,forecast:true},
  clockColor:65535,secColor:1535,hasPassword:false,calendarView:1,calendarRefresh:15,calendars:[{name:'Rodina',hasUrl:true,color:1535,enabled:true}]
} : {screen:4,enabled:[true,true,true,true,true],range:''}});
await run('load()');
assert.equal(document.documentElement.lang, 'sk');
assert.equal(elements.get('ver').textContent, 'v'+version);
assert.equal(elements.get('sForecast').checked, true);
assert.equal(elements.get('calendarView').value, 1);
assert.equal(elements.get('cal0Url').value, '');
assert.equal(elements.get('cal0Url').placeholder, 'Odkaz uložený');
assert.equal(run('body().calendars.length'),3);
// Saving screen changes queues a reboot: secrets clear after success and the
// page must not race the restart with another GET /api/config.
const requests=[];
context.fetch=async (url,options)=>{requests.push({url,options});return {ok:true,json:async()=>url==='/api/config'?{ok:true,restart:true}:{screen:4,enabled:[true,true,true,true,true,true],range:''}};};
elements.get('cal0Url').value='https://example.test/secret.ics';
await run('save()');
const saved=JSON.parse(requests.find(r=>r.url==='/api/config').options.body);
assert.equal(saved.calendars[0].url,'https://example.test/secret.ics');
assert.equal(elements.get('cal0Url').value,'');
assert(!requests.some(r=>r.url==='/api/config'&&!r.options));
// Pairing posts a fresh random key without changing unrelated settings.
requests.length=0;
elements.get('ntSeconds').value='15';
await run('pairNotifications()');
const paired=JSON.parse(requests[0].options.body);
assert.equal(requests[0].url,'/api/config');
assert.deepEqual(Object.keys(paired).sort(),['notifications','password']);
assert.match(paired.notifications.key,/^[0-9a-f]{64}$/);
assert.equal(paired.notifications.enabled,true);
assert.equal(downloads.length,1);
assert.equal(downloads[0].name,'meteoplaneradar-chat.json');
const pairing=await(await fetch(downloads[0].href)).json();
assert.equal(pairing.device,'http://192.168.1.42/api/notifications');
assert.equal(pairing.key,paired.notifications.key);
assert.equal(run('CFG.notifications.paired'),true);
assert.equal(run('body().notifications.key'),undefined);
assert.equal(elements.get('ntPairState').textContent,'Most spárovaný');
requests.length=0;
await run('testNotification()');
assert(requests.some(r=>r.url==='/api/notifications/test'));
requests.length=0;elements.get('ntSeconds').value='3';
await run('testNotification()');assert.equal(requests.length,0);
console.log('Web languages, config, pairing download, secret handling and notification controls passed');

// A saved URL does not imply an enabled screen. The dedicated action must
// enable Calendar without overwriting unrelated form fields or reading secrets.
run('CFG.screens={clock:true,calendar:false}; CFG.calendars=[{name:"Rodina",enabled:true,hasUrl:true,color:1535}]; calendarLabels()');
assert.equal(elements.get('calScreenState').textContent,run('D[L].calScreenOff'));
for(let i=0;i<3;i++){
 elements.get('cal'+i+'Url').value='';elements.get('cal'+i+'Clear').checked=false;
 elements.get('cal'+i+'On').checked=i===0;
}
requests.length=0;
await run('saveCalendars()');
const activation=JSON.parse(requests[0].options.body);
assert.equal(requests[0].url,'/api/config');
assert.deepEqual(activation.screens,{calendar:true});
assert(!('lat' in activation)&&!('notifications' in activation));
assert.equal(activation.calendars[0].url,'');
assert.equal(run('CFG.calendars[0].hasUrl'),true);
assert.equal(elements.get('sCalendar').checked,true);
assert.equal(elements.get('calScreenState').textContent,run('D[L].calScreenOn'));
assert.equal(requests.length,1); // restart pending: no refresh/status race
// With an active screen, persist first, then refresh the saved subscription.
requests.length=0;
context.fetch=async(url,options)=>{requests.push({url,options});return{ok:true,json:async()=>({restart:false,calendarSources:[]})};};
await run('saveCalendars()');
assert.deepEqual(requests.map(r=>r.url),['/api/config','/api/calendar/refresh','/api/status']);
// Failed validation must preserve the entered private URL and off state.
run('CFG.screens.calendar=false');
elements.get('cal0Url').value='webcal://example.test/private';
context.fetch=async()=>({ok:false,status:400,json:async()=>({error:'calendarUrl'})});
await run('saveCalendars()');
assert.equal(elements.get('cal0Url').value,'webcal://example.test/private');
assert.equal(run('CFG.screens.calendar'),false);
// No source means no misleading successful activation.
requests.length=0;elements.get('cal0Url').value='';elements.get('cal0Clear').checked=true;
context.fetch=async(...args)=>{requests.push(args);throw Error('should not request');};
await run('saveCalendars()');assert.equal(requests.length,0);
console.log('Calendar activation, saved URL reuse, restart handling and failed saves passed');

context.fetch=async()=>({json:async()=>({calendarSources:[{state:'Nepodporovane casove pasmo',unsupportedTimezone:'Unknown/Zone',updated:0}]})});
await run('status()');
assert(elements.get('cal0Status').textContent.includes('TZID: Unknown/Zone'));

// Code pairing uses only the admin-gated start/cancel APIs; no file/key export.
requests.length=0;
context.fetch=async(url,options)=>{requests.push({url,options});return{ok:true,json:async()=>({code:'001234',expiresIn:120})};};
await run('startPairCode()');
assert.equal(elements.get('ntCode').textContent,'001 234');
assert.equal(elements.get('ntCodeAddress').textContent,'http://192.168.1.42');
assert.equal(requests[0].url,'/api/notifications/pair/start');
assert.deepEqual(Object.keys(JSON.parse(requests[0].options.body)),['password']);
await run('cancelPairCode()');assert.equal(elements.get('ntCode').textContent,'');
assert.equal(requests[1].url,'/api/notifications/pair/cancel');
context.fetch=async()=>({ok:true,json:async()=>({code:'123456',expiresIn:1})});
await run('startPairCode()');run('pairDeadline=Date.now()-1;tickPairCode()');
assert.equal(elements.get('ntCode').textContent,'');assert.equal(elements.get('ntCodeExpiry').textContent,run('D[L].ntCodeExpired'));
console.log('One-time code display, cancellation and local expiry passed');

context.fetch=async()=>({ok:true,json:async()=>({code:'012345',expiresIn:120})});
await run('startPairCode()');
context.fetch=async()=>({json:async()=>({notificationPairingComplete:true})});
await run('status()');assert.equal(elements.get('ntCode').textContent,'');
assert.equal(elements.get('ntCodeExpiry').textContent,run('D[L].ntCodeDone'));
assert.equal(run('CFG.notifications.paired'),true);assert.equal(elements.get('ntEnabled').checked,true);
console.log('Successful code pairing clears code and updates paired/enabled state without reloading fields');

assert(!page.includes('python3 tools/google-chat-bridge'));
assert(!page.includes('ntStep1'));
assert(page.includes('class="notification-actions"><button type="button" class="sec" id="ntTestBtn"'));
