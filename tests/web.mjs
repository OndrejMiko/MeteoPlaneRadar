import {readFileSync} from 'node:fs';
import vm from 'node:vm';
import assert from 'node:assert/strict';

const page = readFileSync(new URL('../MeteoPlaneRadar/WebPage.h', import.meta.url), 'utf8');
const version = readFileSync(new URL('../MeteoPlaneRadar/Version.h', import.meta.url), 'utf8').match(/#define FW_VERSION "([^"]+)"/)[1];
const script = page.match(/<script>([\s\S]*?)<\/script>/)[1];
const elements = new Map([...page.matchAll(/id="([^"]+)"/g)].map(([,id]) => [id, {
  value: '0', checked: false, classList: {add(){}, remove(){}, toggle(){}},
  addEventListener(){}, textContent: '', innerHTML: '', style: {}
}]));
const translations = [...page.matchAll(/data-i18n="([^"]+)"/g)]
  .map(([,key]) => ({dataset:{i18n:key}, textContent:''}));
const document = {
  documentElement: {},
  getElementById(id) { assert(elements.has(id), `Missing DOM element: ${id}`); return elements.get(id); },
  querySelectorAll(selector) { return selector === '[data-i18n]' ? translations : []; }
};
const context = vm.createContext({document, window:{scrollTo(){}}, setInterval(){}, setTimeout(){},
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
console.log('Web language switching, config load/save and screen controls passed');
