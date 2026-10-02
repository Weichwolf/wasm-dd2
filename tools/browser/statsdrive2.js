// Navigate Info to the "View Statistics" record (read label ptr @0x469b44), launch it, navigate cats.
const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('./playwright');
const buildDir=process.argv[2]||'../../web/dd2'; const OUT='/tmp/statsdrive2'; fs.mkdirSync(OUT,{recursive:true});
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
let errs=[];
const key=async(page,c)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(150);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(650);};
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
// read the C string that label-ptr @va points to
const label=(page,va)=>page.evaluate(a=>{const q=HEAPU32[a>>2];if(!(q>0x400000&&q<0x980000))return'<'+q+'>';let s='';for(let i=0;i<40;i++){const c=HEAPU8[q+i];if(!c)break;s+=String.fromCharCode(c);}return s;},va).catch(e=>'ERR');
const step=async(page,c,label_)=>{errs=[];await key(page,c);const a=await alive(page);const lb=await label(page,0x469b44);console.log(`  ${label_} (${c}): alive=${a} errs=${errs.length} label="${lb}"`);errs.forEach(e=>console.log(`     >> ${e}`));await page.screenshot({path:`${OUT}/${label_}.png`});return a;};
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' \\n ').slice(0,260)));
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
  console.log(`[${buildDir}] booted alive=${await alive(page)}`);
  await step(page,'ArrowDown','01_down');
  await step(page,'ArrowRight','02_right');
  await step(page,'Enter','03_open_info');
  // navigate to the "View Statistics" record within Info
  await step(page,'ArrowLeft','04_navleft');
  await step(page,'ArrowLeft','05_navleft2');
  await step(page,'Enter','06_launch');   // launch whatever is selected
  await step(page,'ArrowRight','07_cat');
  await step(page,'ArrowRight','08_cat');
  await step(page,'ArrowLeft','09_cat');
  await step(page,'Enter','10_drill');
  await step(page,'Escape','11_exit');
  console.log(`FINAL alive=${await alive(page)}`);
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
