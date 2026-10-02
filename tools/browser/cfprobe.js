const http = require('http'); const fs = require('fs'); const path = require('path');
const { chromium } = require('./playwright');
const buildDir = process.argv[2];
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
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  // wait until the race actually starts (cf advancing past the countdown)
  for (let i=0;i<120;i++){
    const cf = await page.evaluate(()=>(typeof HEAP32!=='undefined')?HEAP32[0x462ff0>>2]:-1).catch(()=>-1);
    if (cf>50) break;
    await page.waitForTimeout(1000);
  }
  console.log('race started');
  await page.click('#canvas'); await page.keyboard.down('KeyA');
  for (let i=0;i<5;i++){
    await page.waitForTimeout(3000);
    console.log(JSON.stringify(await page.evaluate(()=>({cf:HEAP32[0x462ff0>>2], pad448:HEAPU16[0x754448>>1],
      x:HEAP32[0x792a24>>2], z:HEAP32[0x792a34>>2]}))));
  }
  console.log(JSON.stringify(await page.evaluate(()=>({audio: !!Module._dd2ac,
    state: Module._dd2ac ? Module._dd2ac.state : null,
    cursor: Module._dd2t, now: Module._dd2ac ? Module._dd2ac.currentTime : null}))));
  await page.screenshot({path:'/tmp/browser_driving.png'});
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR',e); process.exit(1);});
