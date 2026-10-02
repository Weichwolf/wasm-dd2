// Browser FE E2E: boot the real front end, navigate the menu with real key events,
// accept, verify a race starts. Screenshots at each stage.
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
  const page = await browser.newPage({ viewport:{width:700,height:520} });
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  await page.waitForTimeout(30000);                 // boot -> main menu
  await page.screenshot({path:'/tmp/bmenu_1.png'});
  await page.click('#canvas');
  await page.keyboard.press('ArrowRight');          // move selection
  await page.waitForTimeout(4000);
  await page.screenshot({path:'/tmp/bmenu_2.png'});
  await page.keyboard.press('KeyA');                // accept
  await page.waitForTimeout(20000);                 // transition + race load
  await page.screenshot({path:'/tmp/bmenu_3.png'});
  await browser.close(); server.close();
  console.log('done');
})().catch(e=>{console.error('ERR',e); server.close(); process.exit(1);});
