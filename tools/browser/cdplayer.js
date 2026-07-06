// Reach CD Player (main-screen 4) and exercise category nav (Prev/Play/Stop/Next) + drill-in.
const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('playwright');
const buildDir=process.argv[2]||'../../web/dd2'; const OUT='/tmp/cdplayer'; fs.mkdirSync(OUT,{recursive:true});
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
let errs=[];
const key=async(page,c)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(150);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(650);};
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
// main-menu selected label @0x46975c ; CD-player category label @0x469d64
const rd=(page,va)=>page.evaluate(a=>{const q=HEAPU32[a>>2];if(!(q>0x400000&&q<0x980000))return'<'+q+'>';let s='';for(let i=0;i<40;i++){const c=HEAPU8[q+i];if(!c)break;s+=String.fromCharCode(c);}return s;},va).catch(()=>'ERR');
const track=(page)=>page.evaluate(()=>HEAP32[0x469efc>>2]).catch(()=>'?');
const step=async(page,c,l)=>{errs=[];await key(page,c);const a=await alive(page);console.log(`  ${l} (${c}): alive=${a} errs=${errs.length} menu="${await rd(page,0x46975c)}" cdcat="${await rd(page,0x469d64)}" track=${await track(page)}`);errs.forEach(e=>console.log(`     >> ${e}`));await page.screenshot({path:`${OUT}/${l}.png`});return a;};
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' \\n ').slice(0,260)));
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
  console.log(`[${buildDir}] booted alive=${await alive(page)} menu="${await rd(page,0x46975c)}"`);
  // probe main menu: navigate around to find CD Player, screenshot each
  const seq=process.argv[3] ? process.argv[3].split(',') : ['ArrowDown','ArrowRight','ArrowLeft','ArrowUp'];
  for(let i=0;i<seq.length;i++) await step(page,seq[i],`nav${i}_${seq[i]}`);
  await step(page,'Enter','open');
  // if we're in CD player, exercise categories
  await step(page,'ArrowRight','catR1');
  await step(page,'Enter','act1');
  await step(page,'ArrowRight','catR2');
  await step(page,'Enter','act2');
  await step(page,'ArrowLeft','catL');
  await step(page,'Escape','exit');
  console.log(`FINAL alive=${await alive(page)}`);
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
