const http = require('http'); const fs = require('fs'); const path = require('path');
const { chromium } = require('playwright');
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
  page.on('console', m=>console.log('[page]', m.text().slice(0,120)));
  page.on('pageerror', e=>console.log('[pageerror]', e.message.slice(0,200)));
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for (let i=0;i<6;i++){
    await page.waitForTimeout(8000);
    const st = await page.evaluate(()=>{
      if (typeof HEAP32==='undefined') return {ready:false};
      return {ready:true, nrec:HEAP32[0x754404>>2], menustate:HEAP32[0x463018>>2],
              fb0:HEAPU8[0x700450+320*100+160], restart_cd:HEAP32[0x467420>>2]};
    }).catch(e=>({err:''+e}));
    console.log(JSON.stringify(st));
  }
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR',e); server.close(); process.exit(1);});
