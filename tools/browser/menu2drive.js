// Full browser loop: FE menu -> Go! -> race -> DRIVE (hold A) -> car accelerates.
const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('./playwright');
const buildDir=process.argv[2]||'../../web/dd2';
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const key=async(page,c)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(150);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(700);};
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage();
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){const r=await page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);if(r)break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
  const L=async()=>{const p=await page.evaluate(()=>HEAP32[0x46975c>>2]); return await page.evaluate(x=>{const q=HEAP32[x>>2];if(!(q>0x400000&&q<0x980000))return'?';let s='';for(let i=0;i<44;i++){const c=HEAPU8[q+i];if(!c)break;s+=String.fromCharCode(c);}return s;},0x46975c);};
  await key(page,'ArrowRight'); await key(page,'ArrowRight'); console.log('NAV1 '+await L());
  await key(page,'ArrowDown'); await key(page,'ArrowDown'); await key(page,'ArrowDown'); console.log('NAV2 '+await L());
  await key(page,'Enter'); console.log('after-Enter '+await L());
  await page.waitForTimeout(2000);
  for(let t=0;t<6;t++){
    await page.waitForTimeout(4000);
    const st=await page.evaluate(()=>({cf:HEAP32[0x462ff0>>2],lvl:HEAP32[0x936ff4>>2],
      c0:HEAP32[0x792a24>>2]+','+HEAP32[0x792a34>>2], c1:HEAP32[(0x792a24+0x1b2)>>2]}));
    console.log('POLL cf='+st.cf+' lvl='+st.lvl+' car0='+st.c0+' car1x='+st.c1);
  }
  // DRIVE: hold A (accelerate) for several seconds
  await page.evaluate(()=>window.dispatchEvent(new KeyboardEvent('keydown',{code:'KeyA'})));
  await page.waitForTimeout(8000);
  await page.evaluate(()=>window.dispatchEvent(new KeyboardEvent('keyup',{code:'KeyA'})));

  
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR',e);server.close();process.exit(1);});
