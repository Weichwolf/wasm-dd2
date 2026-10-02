// Browser GAMEPAD controls matrix: synthetic standard-mapping pad, ?race=9&pad, measure car response.
const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('./playwright');
const buildDir=process.argv[2]||'../../web/dd2';
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const st=(page)=>page.evaluate(()=>({x:HEAP32[0x792a30>>2],z:HEAP32[0x792a38>>2],vz:HEAP32[0x792a18>>2],pad:HEAPU16[0x754448>>1],analog:HEAPU8[0x754450],cf:HEAP32[0x462ff0>>2]}));
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage();
  await page.addInitScript(()=>{ window.__pad={id:'Xbox',index:0,connected:true,mapping:'standard',axes:[0,0,0,0],buttons:Array.from({length:16},()=>({pressed:false,value:0}))};
    navigator.getGamepads=()=>[window.__pad]; });
  await page.goto(`http://localhost:${server.address().port}/index.html?race=9&pad`,{waitUntil:'load'});
  // Countdown can reset cf and advances separately from adaptive frame skips.
  await page.waitForFunction(()=>typeof HEAP32!=='undefined' && HEAP32[0x936ff4>>2]===9 && HEAP32[0x784298>>2]<0,null,{timeout:30000});
  const test=async(name,setup)=>{
    const b=await st(page);
    await page.evaluate(setup);
    await page.waitForTimeout(3000);
    const d=await st(page);
    await page.evaluate(()=>{__pad.axes=[0,0,0,0]; __pad.buttons.forEach(b=>{b.pressed=false;b.value=0;});});
    await page.waitForTimeout(2500);
    console.log(`PAD ${name.padEnd(10)} pad=0x${d.pad.toString(16).padStart(4,'0')} analog=${d.analog} dpos=${Math.abs(d.x-b.x)+Math.abs(d.z-b.z)} vz=${d.vz}`);
  };
  await test('accel-btn0',()=>{__pad.buttons[0]={pressed:true,value:1};});
  await test('brake-btn1',()=>{__pad.buttons[1]={pressed:true,value:1};});
  await test('steerL-axis',()=>{__pad.axes[0]=-1;});
  await test('steerR-axis',()=>{__pad.axes[0]=1;});
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR',e);server.close();process.exit(1);});
