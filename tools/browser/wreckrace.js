// Thorough Wrecking-Practice race: menu Go! -> long aggressive race (full throttle + steering
// into walls/cars) -> watch for page crash / pageerror through many laps.
const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('./playwright');
const buildDir=process.argv[2]||'../../web/dd2'; const OUT='/tmp/wreck'; fs.mkdirSync(OUT,{recursive:true});
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const key=async(page,c)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(150);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(650);};
const down=(page,c)=>page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);
const up=(page,c)=>page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
const cf=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined'?HEAP32[0x462ff0>>2]:'DEAD').catch(()=>'DEAD');
let errs=[];
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.split('\n')[0]));
  page.on('crash',()=>errs.push('PAGE CRASHED'));
  await page.goto(`http://localhost:${server.address().port}/index.html?race=9`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.click('#canvas'); await page.waitForTimeout(10000);
  console.log('race started (?race=9 = Wrecking), cf='+await cf(page)+' errs='+errs.length);
  // aggressive drive: full throttle + alternating hard steer for ~70s, watch for crash
  await down(page,'KeyA');
  const dirs=['ArrowLeft','ArrowRight'];
  for(let t=0;t<35;t++){
    const d=dirs[t%2];
    await down(page,d); await page.waitForTimeout(1200); await up(page,d);
    if(t%5===0){const a=await alive(page);console.log(`  t=${t*2}s cf=${await cf(page)} alive=${a} errs=${errs.length}`);
      if(!a||errs.length){console.log('  DIED/ERR: '+JSON.stringify([...new Set(errs)]));break;}}
  }
  await up(page,'KeyA');
  await page.screenshot({path:`${OUT}/end.png`});
  console.log('FINAL: alive='+await alive(page)+' cf='+await cf(page)+' errs='+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
