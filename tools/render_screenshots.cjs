// Render snapshots emitted by the host fixtures; no device/account data is used.
// See tests/README.md. Requires Playwright and its Chrome channel.
const {readFileSync,writeFileSync,mkdirSync}=require('node:fs');
const path=require('node:path');
const {chromium}=require('playwright');
const source=process.argv[2];
if(!source)throw new Error('Usage: node tools/render_screenshots.cjs /absolute/path/to/snapshots');
const root=path.resolve(__dirname,'..'),out=path.join(root,'docs/screens');
const version=readFileSync(path.join(root,'MeteoPlaneRadar/Version.h'),'utf8').match(/FW_VERSION "([^"]+)"/)[1];
const read=name=>readFileSync(path.join(source,name+'.svg'),'utf8');
function layer(svg){
 const match=svg.match(/<g clip-path='url\(#round\)'>([\s\S]*)<\/g><\/svg>/);
 if(!match)throw new Error('Unknown fixture SVG format');
 return match[1].replace(/^<rect width='480' height='480' fill='#[^']+'\/>/,'');
}
const wrap=body=>`<svg xmlns="http://www.w3.org/2000/svg" width="480" height="480"><defs><clipPath id="round"><circle cx="240" cy="240" r="239"/></clipPath></defs><g clip-path="url(#round)"><rect width="480" height="480" fill="black"/>${body}</g></svg>`;
const panels=[
 ['clock','Hodiny / Clock','clock-font-1'],
 ['aircraft','Letadla / Aircraft','aircraft-font-1'],
 ['forecast','Předpověď / Forecast','forecast-font-1'],
 ['weather','Meteoradar / Weather controls','weather-chrome'],
 ['calendar-day','Kalendář: den / Day','calendar-smooth-0'],
 ['calendar-month','Kalendář: měsíc / Month','calendar-smooth-1'],
 ['notification-banner','Banner nad hodinami / Banner',null],
 ['notification-detail','Detail notifikace / Message',null],
 ['settings','Nastavení / Settings','settings-font-1']
];
(async()=>{
 mkdirSync(out,{recursive:true});
 const browser=await chromium.launch({channel:'chrome',headless:true});
 try{
  const page=await browser.newPage({viewport:{width:480,height:480},deviceScaleFactor:1});
  for(const [name,,fixture] of panels){
   let svg=fixture?read(fixture):(name==='notification-banner'?wrap(layer(read('clock-font-1'))+layer(read('banner-smooth'))):read('detail-smooth'));
   if(name==='weather'){
    // Explicitly synthetic map under the real weather chrome, not radar data.
    const grid=Array.from({length:12},(_,i)=>`<path d="M0 ${i*44}L480 ${i*44-50}" stroke="#273444"/><path d="M${i*44} 0L${i*44-70} 480" stroke="#273444"/>`).join('');
    svg=svg.replace(/<rect width='480' height='480' fill='#[^']+'\/>/,`<rect width="480" height="480" fill="#101923"/>${grid}`);
   }
   await page.setContent('<style>body{margin:0;background:#141820}svg{display:block;border-radius:50%;overflow:hidden}</style>'+svg);
   await page.screenshot({path:path.join(out,name+'.png')});
  }
  const cards=panels.map(([name,title])=>`<article><img src="data:image/png;base64,${readFileSync(path.join(out,name+'.png')).toString('base64')}"/><h2>${title}</h2></article>`).join('');
  await page.setViewportSize({width:1200,height:1600});
  await page.setContent(`<style>*{box-sizing:border-box}body{margin:0;background:#141820;color:#eef2f6;font-family:Arial,sans-serif;padding:32px}header{margin-bottom:24px}h1{font-size:34px;margin:0 0 10px}p{font-size:17px;color:#b1bdca;margin:0;line-height:1.5}.grid{display:grid;grid-template-columns:repeat(3,1fr);gap:26px 24px}article{margin:0}img{width:100%;display:block}h2{text-align:center;font-size:17px;margin:14px 0 4px;font-weight:500}footer{font-size:15px;color:#b1bdca;margin-top:28px;line-height:1.6}</style><header><h1>MeteoPlaneRadar · OndrejMiko fork · v${version}</h1><p>Aktuální rozhraní / Current interface · 480 × 480 · ukázková data / synthetic data</p></header><main class="grid">${cards}</main><footer>Vlevo / Left → next screen · Vpravo / Right → previous screen · Nahoru/dolů / Up/down → map zoom<br>Calendar and notifications use real drawing code. Weather shows controls on a synthetic grid, not live radar.<br>Electricity screens are optional and available only in Czech. Original project: chiptron.cz / petus.</footer>`);
  await page.evaluate(()=>Promise.all([...document.images].map(i=>i.decode())));
  const height=await page.evaluate(()=>document.documentElement.scrollHeight);
  await page.setViewportSize({width:1200,height});
  await page.screenshot({path:path.join(root,'obrazovky.png'),fullPage:true});
  writeFileSync(path.join(out,'README.md'),`# Screen previews\n\nFirmware ${version}. Generated from host-fixture SVG drawing calls with synthetic data.\nNo live calendars, chat messages, tokens or subscription URLs are used.\nSmooth text uses the firmware bitmap fonts; built-in pixel text is approximated by the host font.\nWeather uses the real chrome on a synthetic grid, not a radar download.\nThe banner composites the real notification layer over the clock; expanded details replace the screen.\n\nRegeneration: see [tests/README.md](../../tests/README.md).\n`);
  console.log('Updated obrazovky.png and '+out);
 }finally{await browser.close();}
})().catch(error=>{console.error(error);process.exitCode=1;});
