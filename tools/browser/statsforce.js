// Validate View_Statistics render: force stats_recorded=1 in HEAP so the Info screen lands on
// rec-0 (View Statistics), then drive category nav + drill-in. Screenshots + error capture.
const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('playwright');
const buildDir=process.argv[2]||'../../web/dd2'; const OUT='/tmp/statsforce'; fs.mkdirSync(OUT,{recursive:true});
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
let errs=[];
const key=async(page,c)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(150);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(650);};
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
const label=(page,va)=>page.evaluate(a=>{const q=HEAPU32[a>>2];if(!(q>0x400000&&q<0x980000))return'<'+q+'>';let s='';for(let i=0;i<40;i++){const c=HEAPU8[q+i];if(!c)break;s+=String.fromCharCode(c);}return s;},va).catch(()=>'ERR');
const setStats=(page)=>page.evaluate(()=>{HEAP32[0x46741c>>2]=1;return HEAP32[0x46741c>>2];}).catch(e=>'ERR:'+e);
const step=async(page,c,l)=>{errs=[];await key(page,c);const a=await alive(page);const lb=await label(page,0x469b44);console.log(`  ${l} (${c}): alive=${a} errs=${errs.length} label="${lb}"`);errs.forEach(e=>console.log(`     >> ${e}`));await page.screenshot({path:`${OUT}/${l}.png`});return a;};
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' \\n ').slice(0,260)));
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
  console.log(`[${buildDir}] booted alive=${await alive(page)}`);
  console.log('  force stats_recorded=1 ->', await setStats(page));
  await step(page,'ArrowDown','01_down');
  await step(page,'ArrowRight','02_right');
  await page.evaluate(()=>{HEAP32[0x46741c>>2]=1;});  // re-assert right before opening
  await step(page,'Enter','03_open_info');   // should land on View Statistics (rec-0)
  await step(page,'Enter','04_launch_stats'); // drill into the highlighted category viewer
  await step(page,'ArrowRight','05_cat_next');
  await step(page,'ArrowRight','06_cat_next2');
  await step(page,'ArrowLeft','07_cat_prev');
  await step(page,'Escape','08_exit');
  console.log(`FINAL alive=${await alive(page)}`);
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
