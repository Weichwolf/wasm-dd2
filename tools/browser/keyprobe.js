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
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){ const r=await page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false); if(r)break; await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000);
  // direct engine call (bypass DOM) — does the input path work at all?
  const direct = await page.evaluate(()=>{
    HEAPU8[0x463043]=0;                    // _pad_lup
    Module.ccall('dd2_browser_key_event','number',['string','number'],['ArrowUp',1]);
    const after=HEAPU8[0x463043];
    Module.ccall('dd2_browser_key_event','number',['string','number'],['ArrowUp',0]);
    return {padlup_after_down:after, mapactive:Array.from({length:14},(_,i)=>HEAPU8[0x46302c+i])};
  });
  console.log('DIRECT:', JSON.stringify(direct));
  // now DOM path
  await page.click('#canvas');
  await page.keyboard.down('ArrowRight');
  await page.waitForTimeout(100);
  const dom = await page.evaluate(()=>({pad44a:HEAPU16[0x75444a>>1], pad448:HEAPU16[0x754448>>1], rawright:HEAPU8[0x46303f]}));
  await page.keyboard.up('ArrowRight');
  console.log('DOM:', JSON.stringify(dom));
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR',e); server.close(); process.exit(1);});
