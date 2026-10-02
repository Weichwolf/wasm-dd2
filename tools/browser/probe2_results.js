// QA: after RETIRE, watch the results-replay for a menu (Save Replay/Next Race/View League...).
// Poll fe005 + a few candidate screen bytes; screenshot each phase; try Enter/Esc to surface a menu.
const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('./playwright');
const buildDir='../../web/dd2'; const OUT='/tmp/qa_explore2'; fs.mkdirSync(OUT,{recursive:true});
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const key=async(page,c,post=650)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(150);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(post);};
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
const info=(page)=>page.evaluate(()=>({cf:HEAP32[0x462ff0>>2],fe005:HEAPU8[0x460005],rf:HEAP32[0x467448>>2]|0,menu:(function(){const q=HEAPU32[0x46975c>>2];if(!(q>0x400000&&q<0x980000))return'';let s='';for(let i=0;i<40;i++){const c=HEAPU8[q+i];if(!c)break;s+=String.fromCharCode(c);}return s;})()})).catch(()=>'DEAD');
let errs=[];
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,160)));
  page.on('crash',()=>errs.push('PAGE CRASHED'));
  await page.goto(`http://localhost:${server.address().port}/index.html?race=9`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.click('#canvas'); await page.waitForTimeout(13000);
  // pause -> retire -> yes
  await key(page,'Escape');await page.waitForTimeout(600);
  await key(page,'ArrowDown');await key(page,'ArrowDown');await key(page,'ArrowDown'); // Retire
  await key(page,'Enter'); await key(page,'ArrowDown'); await key(page,'Enter');
  console.log('retired. watching replay...');
  let last='';
  for(let t=0;t<28;t++){
    await page.waitForTimeout(2000);
    const nfo=await info(page);
    const a=await alive(page);
    console.log(`  t=${t*2}s ${JSON.stringify(nfo)} alive=${a} errs=${errs.length}`);
    if(t%3===0||JSON.stringify(nfo).slice(0,30)!==last){await page.screenshot({path:`${OUT}/results_t${t*2}.png`});last=JSON.stringify(nfo).slice(0,30);}
    if(!a){console.log('  DEAD');break;}
  }
  // try to surface/select a menu option
  console.log('--- try Enter then arrows ---');
  await key(page,'Enter');await page.waitForTimeout(1500);
  console.log('  after Enter '+JSON.stringify(await info(page)));
  await page.screenshot({path:`${OUT}/results_afterEnter.png`});
  for(const k of ['ArrowDown','ArrowDown','ArrowUp','Enter']){await key(page,k);console.log(`  ${k} `+JSON.stringify(await info(page)));}
  await page.screenshot({path:`${OUT}/results_final.png`});
  console.log('FINAL alive='+await alive(page)+' errs='+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);try{server.close();}catch(_){}process.exit(1);});
