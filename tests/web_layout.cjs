// Optional real-browser QA: NODE_PATH=/path/to/node_modules node tests/web_layout.cjs
const {chromium}=require('playwright');
const {readFileSync,mkdirSync}=require('node:fs');
const assert=require('node:assert/strict');
const path=require('node:path');
const root=path.resolve(__dirname,'..');
const html=readFileSync(path.join(root,'MeteoPlaneRadar/WebPage.h'),'utf8').match(/R"rawliteral\(([\s\S]*?)\)rawliteral"/)[1];
const version=readFileSync(path.join(root,'MeteoPlaneRadar/Version.h'),'utf8').match(/FW_VERSION "([^"]+)"/)[1];
const config={version,lang:2,lat:48.15,lon:17.11,screens:{clock:true,planes:true,meteo:true,forecast:true,calendar:true},clockColor:65535,secColor:1535,hasPassword:false,calendarView:0,calendarRefresh:15,calendars:[{name:'Rodina',hasUrl:true,color:1535,enabled:true}],notifications:{enabled:true,paired:true,seconds:15},autoRotate:30,radarSrc:1,meteoLegend:true,briDay:80,briNight:20,nightAuto:true,nightOffset:0,secStyle:1,metric:true,topBearing:0,altMin:0,altMax:60000};
const status={version,screen:1,enabled:[true,true,true,true,true,true],range:'100 km',ip:'192.168.1.42',ssid:'Domaca WiFi',rssi:-52,uptime:'2 h 34 min',heap:157096,psram:6200000,resetReason:'Power on',adsb:'OK',radar:'OK',forecast:'OK',timezone:'Europe/Bratislava',board:'Waveshare ESP32-S3 2.1',calendarSources:[{state:'OK',updated:0}],notificationCount:0,notificationBridgeAge:12,notificationMemory:7176};
(async()=>{
 const browser=await chromium.launch({channel:'chrome',headless:true});
 try{
 const page=await browser.newPage();const errors=[];const posts=[];let pairingComplete=false;
 page.on('pageerror',e=>errors.push(e.message));
 await page.route('http://meteoradar.test/**',async route=>{
  const req=route.request(),url=new URL(req.url());
  if(req.method()==='POST'){posts.push({url:url.pathname,body:req.postDataJSON()});await route.fulfill({json:url.pathname==='/api/notifications/pair/start'?{code:'001234',expiresIn:120}:{ok:true,restart:true}});return;}
  if(url.pathname==='/')await route.fulfill({contentType:'text/html',body:html});
  else await route.fulfill({json:url.pathname==='/api/config'?config:url.pathname==='/api/scan'?[]:{...status,notificationPairingComplete:pairingComplete}});
 });
 const dir=process.env.WEB_SCREENSHOTS;if(dir)mkdirSync(dir,{recursive:true});
 for(const width of [320,390,768,1280]){
  await page.setViewportSize({width,height:900});await page.goto('http://meteoradar.test/');await page.waitForFunction(()=>document.querySelector('#ver').textContent.startsWith('v'));
  for(const lang of ['cs','en','sk']){
   await page.evaluate(l=>setLang(l),lang);
   for(const id of ['tCtl','tLoc','tScr','tLook','tPlanes','tCal','tNotify','tSys']){
    await page.locator('[data-tab="'+id+'"]').click();
    const fit=await page.evaluate(()=>({width:innerWidth,scroll:document.documentElement.scrollWidth,selected:document.querySelectorAll('[role=tab][aria-selected=true]').length,panels:document.querySelectorAll('section.tab:not(.hide)').length}));
    assert(fit.scroll<=fit.width,`page overflow ${width} ${lang} ${id}: ${fit.scroll}`);assert.equal(fit.selected,1);assert.equal(fit.panels,1);
    const manual=['tLoc','tScr','tCal','tNotify','tSys'].includes(id);
    assert.equal(await page.locator('#saveButton').isVisible(),manual);
    if(manual){
     const before=await page.locator('#saveButton').boundingBox();await page.evaluate(()=>window.scrollTo(0,document.body.scrollHeight));const after=await page.locator('#saveButton').boundingBox();assert.equal(before.y,after.y);assert(after.x>=0&&after.x+after.width<=width);assert(after.y>=0&&after.y+after.height<=124);
    }
    if(dir&&lang==='sk'&&(width===390||width===1280)){
     await page.evaluate(()=>window.scrollTo(0,0));await page.screenshot({path:path.join(dir,`${width}-${id}.png`),fullPage:true});
    }
   }
  }
 }
 for(const width of [320,390,1280]){
  await page.setViewportSize({width,height:900});await page.goto('http://meteoradar.test/');await page.waitForFunction(()=>document.querySelector('#ver').textContent.startsWith('v'));
  await page.locator('[data-tab=tNotify]').click();await page.locator('#ntCodeBtn').click();
  await page.waitForFunction(()=>document.querySelector('#ntCode').textContent==='001 234');
  assert(await page.locator('#ntCode').isVisible());
  const box=await page.locator('#ntCode').boundingBox();assert(box.x>=0&&box.x+box.width<=width);
  assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
  if(dir)await page.screenshot({path:path.join(dir,`${width}-code-pairing.png`),fullPage:true});
  await page.locator('#ntCodeCancel').click();await page.waitForFunction(()=>document.querySelector('#ntCode').classList.contains('hide'));assert(!(await page.locator('#ntCode').isVisible()));
 }
 await page.locator('#ntCodeBtn').click();await page.waitForFunction(()=>document.querySelector('#ntCode').textContent==='001 234');
 pairingComplete=true;await page.evaluate(()=>status());assert(!(await page.locator('#ntCode').isVisible()));
 assert(await page.locator('#ntEnabled').isChecked());
 await page.locator('[data-tab=tLoc]').click();await page.locator('#lat').fill('49.1234');await page.locator('[data-tab=tCal]').click();await page.locator('[data-tab=tLoc]').click();assert.equal(await page.locator('#lat').inputValue(),'49.1234');
 await page.locator('[data-tab=tLoc]').focus();await page.keyboard.press('ArrowRight');assert.equal(await page.locator('[data-tab=tScr]').getAttribute('aria-selected'),'true');await page.keyboard.press('End');assert.equal(await page.locator('[data-tab=tSys]').getAttribute('aria-selected'),'true');await page.keyboard.press('Home');assert.equal(await page.locator('[data-tab=tCtl]').getAttribute('aria-selected'),'true');
 await page.locator('[data-tab=tCal]').click();await page.locator('#saveButton').click();await page.waitForFunction(()=>!document.querySelector('#saveButton').disabled);
 const calendar=posts.find(p=>p.url==='/api/config');assert.deepEqual(calendar.body.screens,{calendar:true});assert(!('lat' in calendar.body));assert.equal(calendar.body.calendars[0].url,'');
 await page.evaluate(()=>msg('Testovacia správa','ok'));assert(await page.locator('#msg').isVisible());
 assert.deepEqual(errors,[]);console.log('Web browser: 4 widths × 3 languages × 8 tabs; fixed saves, keyboard navigation, unsaved fields and calendar save and code pairing passed');
 }finally{await browser.close();}
})().catch(e=>{console.error(e);process.exit(1)});
