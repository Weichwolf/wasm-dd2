const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('playwright');
const buildDir=process.argv[2]||'../../web/dd2'; const OUT='/tmp/stats'; fs.mkdirSync(OUT,{recursive:true});
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const key=async(page,c)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(150);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(650);};
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
let errs=[],crashed=false;
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.split('\n')[0])); page.on('crash',()=>crashed=true);
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(9000); await page.click('#canvas');
  // Info screen (document, bottom row 2nd): Down + Right
  await key(page,'ArrowDown'); await key(page,'ArrowRight');
  await key(page,'Enter'); await page.waitForTimeout(700);   // open Info dialog
  console.log('Info dialog: alive='+await alive(page)+' errs='+errs.length);
  // rec-0 = View Statistics; confirm to enter it
  await key(page,'Enter'); await page.waitForTimeout(900);
  await page.screenshot({path:`${OUT}/1_stats_open.png`});
  console.log('View Statistics open: alive='+await alive(page)+' crashed='+crashed+' errs='+errs.length+' '+JSON.stringify([...new Set(errs)]));
  // navigate categories (next/prev) + drill-in
  await key(page,'ArrowRight'); await page.screenshot({path:`${OUT}/2_next.png`});
  await key(page,'ArrowRight');
  await key(page,'ArrowLeft');
  console.log('after category nav: alive='+await alive(page)+' crashed='+crashed+' errs='+errs.length);
  // drill in (accept = 0x4000 = KeyA? no, 0x4000 edge. Enter=accept maps to 0x4000). Use KeyA for drill? 
  // The drill bit 0x4000: in-menu accept. Try Enter (may exit) and A.
  await page.evaluate(()=>window.dispatchEvent(new KeyboardEvent('keydown',{code:'KeyA'})));
  await page.waitForTimeout(200);
  await page.evaluate(()=>window.dispatchEvent(new KeyboardEvent('keyup',{code:'KeyA'})));
  await page.waitForTimeout(800);
  await page.screenshot({path:`${OUT}/3_drill.png`});
  console.log('after drill(A): alive='+await alive(page)+' crashed='+crashed+' errs='+errs.length);
  // exit (0x1000 = back). Esc maps to back/pause.
  await key(page,'Escape'); await page.waitForTimeout(800);
  console.log('after exit: alive='+await alive(page)+' crashed='+crashed+' errs='+errs.length);
  // RECOVERY: can we still navigate + launch a race?
  await key(page,'Escape');
  await key(page,'ArrowUp'); await key(page,'ArrowRight'); await key(page,'ArrowRight'); await key(page,'ArrowDown'); await key(page,'ArrowDown'); await key(page,'ArrowDown');
  await key(page,'Enter'); await page.waitForTimeout(12000);
  const cf=await page.evaluate(()=>HEAP32[0x462ff0>>2]).catch(()=>'?');
  console.log('RECOVERY race: alive='+await alive(page)+' cf='+cf+' crashed='+crashed);
  await page.screenshot({path:`${OUT}/4_recovery.png`});
  console.log('FINAL errs: '+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
