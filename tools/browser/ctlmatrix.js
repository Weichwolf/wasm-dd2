// Systematic browser controls matrix: live race (?race=9), measure car response to each input.
const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('playwright');
const buildDir=process.argv[2]||'../../web/dd2';
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const carState=(page)=>page.evaluate(()=>({x:HEAP32[0x792a24>>2],z:HEAP32[0x792a34>>2],vx:HEAP32[0x792a04>>2],vz:HEAP32[0x792a18>>2],pad:HEAPU16[0x754448>>1],cf:HEAP32[0x462ff0>>2]}));
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage();
  await page.goto(`http://localhost:${server.address().port}/index.html?race=9`,{waitUntil:'load'});
  for(let i=0;i<80;i++){const r=await page.evaluate(()=>typeof HEAP32!=='undefined'&&HEAP32[0x462ff0>>2]>200).catch(()=>false);if(r)break;await page.waitForTimeout(1000);}
  await page.click('#canvas');
  const test=async(name,code,ms)=>{
    const b=await carState(page);
    await page.evaluate(c=>window.dispatchEvent(new KeyboardEvent('keydown',{code:c})),code);
    await page.waitForTimeout(ms);
    const d=await carState(page);
    await page.evaluate(c=>window.dispatchEvent(new KeyboardEvent('keyup',{code:c})),code);
    await page.waitForTimeout(2500);
    console.log(`KBD ${name.padEnd(8)} pad=0x${d.pad.toString(16).padStart(4,'0')} dpos=${Math.abs(d.x-b.x)+Math.abs(d.z-b.z)} vx=${d.vx} vz=${d.vz}`);
  };
  await test('accel-A','KeyA',3000);
  await test('brake-Z','KeyZ',3000);
  await test('steerL','ArrowLeft',2500);
  await test('steerR','ArrowRight',2500);
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR',e);server.close();process.exit(1);});
