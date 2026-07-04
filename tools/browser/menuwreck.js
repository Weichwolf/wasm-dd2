const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('playwright');
const buildDir=process.argv[2]||'../../web/dd2'; const OUT='/tmp/mwreck'; fs.mkdirSync(OUT,{recursive:true});
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const key=async(page,c)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(150);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(650);};
const down=(page,c)=>page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);
const up=(page,c)=>page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
const st=(page)=>page.evaluate(()=>[HEAP32[0x462ff0>>2],HEAP32[0x46765c>>2]]).catch(()=>'DEAD'); // cf, num_cars
let errs=[];
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.split('\n')[0]));
  page.on('crash',()=>errs.push('PAGE CRASHED'));
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(9000); await page.click('#canvas');
  // navigate to Go! (bottom-right): Down then Right x3 (proven-ish); then Enter to launch
  await key(page,'ArrowRight'); await key(page,'ArrowRight'); await key(page,'ArrowDown'); await key(page,'ArrowDown'); await key(page,'ArrowDown');
  await page.screenshot({path:`${OUT}/prego.png`});
  console.log('at Go!? errs='+errs.length);
  await key(page,'Enter');          // Go! -> launch race
  await page.waitForTimeout(14000);
  console.log('race launch: alive='+await alive(page)+' [cf,numcars]='+await st(page)+' errs='+errs.length);
  await page.screenshot({path:`${OUT}/racestart.png`});
  await down(page,'KeyA'); const dirs=['ArrowLeft','ArrowRight'];
  for(let t=0;t<30;t++){const d=dirs[t%2];await down(page,d);await page.waitForTimeout(1200);await up(page,d);
    if(t%5===0){const a=await alive(page);console.log(`  t=${t*2}s [cf,nc]=${await st(page)} alive=${a} errs=${errs.length}`);
      if(!a||errs.length){console.log('  DIED/ERR: '+JSON.stringify([...new Set(errs)]));break;}}}
  await up(page,'KeyA');
  console.log('FINAL: alive='+await alive(page)+' errs='+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
