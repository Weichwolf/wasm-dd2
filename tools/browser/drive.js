// Browser input E2E: load the interactive build, hold KeyA (accelerate) via real
// keyboard events, screenshot before/after — proves the key path drives the game in-browser.
//   node drive.js <buildDir> <outPrefix> [bootMs] [driveMs]
const http = require('http'); const fs = require('fs'); const path = require('path');
const { chromium } = require('playwright');
const buildDir = process.argv[2] || '../../web/dd2';
const pre = process.argv[3] || '/tmp/drive';
const boot = parseInt(process.argv[4] || '40000', 10);
const drive = parseInt(process.argv[5] || '15000', 10);
const MIME = {'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm',
              '.data':'application/octet-stream'};
const server = http.createServer((req,res)=>{
  let p = decodeURIComponent(req.url.split('?')[0]); if (p==='/') p='/index.html';
  fs.readFile(path.join(buildDir,p),(err,buf)=>{
    if(err){res.writeHead(404);res.end();return;}
    res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});
    res.end(buf); });
});
(async ()=>{
  await new Promise(r=>server.listen(0,r));
  const browser = await chromium.launch({ args:['--use-gl=angle','--use-angle=swiftshader',
    '--enable-unsafe-swiftshader','--no-sandbox'] });
  const page = await browser.newPage({ viewport:{width:700,height:520} });
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load',timeout:60000});
  await page.waitForTimeout(boot);
  await page.screenshot({ path: pre+'_before.png' });
  await page.click('#canvas');
  await page.keyboard.down('KeyA');            // accelerate
  await page.waitForTimeout(drive);
  await page.screenshot({ path: pre+'_during.png' });
  await page.keyboard.up('KeyA');
  await page.keyboard.down('ArrowLeft');          // steer
  await page.waitForTimeout(4000);
  await page.screenshot({ path: pre+'_after.png' });
  await browser.close(); server.close();
  console.log('done ->', pre+'_{before,during,after}.png');
})().catch(e=>{console.error('ERR',e); server.close(); process.exit(1);});
