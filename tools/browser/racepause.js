const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('./playwright');
const buildDir=process.argv[2]||'../../web/dd2';
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const key=async(page,c,hold=150)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(hold);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(700);};
const cf=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined'?HEAP32[0x462ff0>>2]:'DEAD').catch(()=>'DEAD');
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>console.log('PAGEERR '+e.message.split('\n').slice(0,4).join(' | ')));
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){const r=await page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);if(r)break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(9000); await page.click('#canvas');
  // main menu (Wrecking Racing/Practice default) -> DOWN to Go! -> Enter
  await key(page,'ArrowRight'); await key(page,'ArrowRight'); // to Select Track (screen2)
  await key(page,'ArrowDown');await key(page,'ArrowDown');await key(page,'ArrowDown'); // to Go!
  console.log('launching race, cf before='+await cf(page));
  await key(page,'Enter');
  await page.waitForTimeout(20000);
  console.log('race running, cf='+await cf(page));
  // drive a bit
  await page.evaluate(()=>window.dispatchEvent(new KeyboardEvent('keydown',{code:'KeyA'})));
  await page.waitForTimeout(4000);
  await page.evaluate(()=>window.dispatchEvent(new KeyboardEvent('keyup',{code:'KeyA'})));
  console.log('after driving, cf='+await cf(page));
  // PAUSE (Esc)
  await key(page,'Escape');
  await page.waitForTimeout(2000);
  const c1=await cf(page); await page.waitForTimeout(3000); const c2=await cf(page);
  console.log('after Esc(pause): cf1='+c1+' cf2='+c2+(c1===c2&&c1!=='DEAD'?' (FROZEN/paused)':''));
  await page.screenshot({path:'/tmp/racepause.png'});
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
