// Browser gamepad E2E: monkeypatch navigator.getGamepads with a synthetic pad and verify the
// shell's poll loop -> dd2_pad_update -> engine joystick path drives the car. Validates the
// full chain except the physical HID layer.
const http = require('http'); const fs = require('fs'); const path = require('path');
const { chromium } = require('./playwright');
const buildDir = process.argv[2] || '../../web/dd2';
const MIME = {'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server = http.createServer((req,res)=>{
  let p = decodeURIComponent(req.url.split('?')[0]); if (p==='/') p='/index.html';
  fs.readFile(path.join(buildDir,p),(err,buf)=>{ if(err){res.writeHead(404);res.end();return;}
    res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'}); res.end(buf); });
});
(async ()=>{
  await new Promise(r=>server.listen(0,r));
  const browser = await chromium.launch({ args:['--no-sandbox'] });
  const page = await browser.newPage();
  // synthetic pad BEFORE the page loads (the engine detects at boot)
  await page.addInitScript(()=>{
    window.__pad = { id:'Synthetic Xbox', index:0, connected:true, mapping:'standard',
      axes:[0,0,0,0], buttons:Array.from({length:16},()=>({pressed:false,value:0})) };
    navigator.getGamepads = ()=>[window.__pad];
  });
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for (let i=0;i<120;i++){
    const cf = await page.evaluate(()=>(typeof HEAP32!=='undefined')?HEAP32[0x462ff0>>2]:-1).catch(()=>-1);
    if (cf>10) break;
    await page.waitForTimeout(1000);
  }
  console.log('race started; engaging pad: button0 (accel) + full left');
  await page.evaluate(()=>{ __pad.buttons[0]={pressed:true,value:1}; __pad.axes[0]=-1; });
  const samples=[];
  for (let i=0;i<4;i++){
    await page.waitForTimeout(3000);
    samples.push(await page.evaluate(()=>({cf:HEAP32[0x462ff0>>2], pad448:HEAPU16[0x754448>>1],
      analog:HEAPU8[0x754450], mode:HEAPU8[0x46303e], x:HEAP32[0x792a24>>2], z:HEAP32[0x792a34>>2]})));
  }
  samples.forEach(s=>console.log(JSON.stringify(s)));
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR',e); server.close(); process.exit(1);});
