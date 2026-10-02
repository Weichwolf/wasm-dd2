// Reach Sound Volume (Config rec-1) and exercise volume up/down. Screenshots + error capture.
const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('./playwright');
const buildDir=process.argv[2]||'../../web/dd2'; const OUT='/tmp/soundvol'; fs.mkdirSync(OUT,{recursive:true});
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
let errs=[];
const key=async(page,c)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(150);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(650);};
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
const cfg=(page)=>page.evaluate(()=>{const q=HEAPU32[0x469158>>2];if(!(q>0x400000&&q<0x980000))return'<'+q+'>';let s='';for(let i=0;i<40;i++){const c=HEAPU8[q+i];if(!c)break;s+=String.fromCharCode(c);}return s;}).catch(()=>'ERR');
const vol=(page)=>page.evaluate(()=>HEAP32[0x93fd20>>2]).catch(()=>'?');
const step=async(page,c,l)=>{errs=[];await key(page,c);const a=await alive(page);console.log(`  ${l} (${c}): alive=${a} errs=${errs.length} cfglabel="${await cfg(page)}" workvol=${await vol(page)}`);errs.forEach(e=>console.log(`     >> ${e}`));await page.screenshot({path:`${OUT}/${l}.png`});return a;};
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' \\n ').slice(0,260)));
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
  console.log(`[${buildDir}] booted alive=${await alive(page)}`);
  // Config = wrench (bottom-right icon): Down, Right, Right
  await step(page,'ArrowDown','01_down');
  await step(page,'ArrowRight','02_right');
  await step(page,'ArrowRight','03_right2');
  await step(page,'Enter','04_open_config');
  // navigate to Sound Volume record (rec-1) and accept
  await step(page,'ArrowRight','05_navrec');
  await step(page,'Enter','06_launch_soundvol');
  // exercise volume
  await step(page,'ArrowRight','07_vol_up');
  await step(page,'ArrowRight','08_vol_up2');
  await step(page,'ArrowLeft','09_vol_down');
  await step(page,'Escape','10_back');
  console.log(`FINAL alive=${await alive(page)}`);
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
