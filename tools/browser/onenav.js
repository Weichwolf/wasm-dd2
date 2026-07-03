const http = require('http'); const fs = require('fs'); const path = require('path');
const { chromium } = require('playwright');
const buildDir = process.argv[2] || '../../web/dd2';
const MIME = {'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server = http.createServer((req,res)=>{ let p=decodeURIComponent(req.url.split('?')[0]); if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{ if(e){res.writeHead(404);res.end();return;} res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'}); res.end(b);});});
const lbl=(page,a)=>page.evaluate(x=>{const p=HEAP32[x>>2]; if(!(p>0x400000&&p<0x980000))return'?'; let s=''; for(let i=0;i<44;i++){const c=HEAPU8[p+i]; if(!c)break; s+=String.fromCharCode(c);} return s;},a);
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage();
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){const r=await page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false); if(r)break; await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000);
  await page.click('#canvas');
  console.log('start:', await lbl(page,0x46975c));
  for(let k=0;k<4;k++){
    await page.keyboard.down('ArrowRight'); await page.waitForTimeout(120); await page.keyboard.up('ArrowRight');
    await page.waitForTimeout(900);
    console.log(`after RIGHT#${k+1}:`, await lbl(page,0x46975c));
  }
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR',e); server.close(); process.exit(1);});
