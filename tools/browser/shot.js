// Self-contained: serve web/build, load it in headless Chromium (SwiftShader WebGL),
// screenshot the canvas, exit. No external server / pkill needed.
//   node shot.js <buildDir> <out.png> [waitMs]
const http = require('http'); const fs = require('fs'); const path = require('path');
const { chromium } = require('playwright');

const buildDir = process.argv[2] || 'web/build';
const out = process.argv[3] || 'shot.png';
const wait = parseInt(process.argv[4] || '10000', 10);
const MIME = {'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm',
              '.data':'application/octet-stream','.png':'image/png'};

const server = http.createServer((req,res)=>{
  let p = decodeURIComponent(req.url.split('?')[0]); if (p==='/') p='/index.html';
  const fp = path.join(buildDir, p);
  fs.readFile(fp,(err,buf)=>{
    if(err){res.writeHead(404);res.end('404');return;}
    res.writeHead(200,{'Content-Type':MIME[path.extname(fp)]||'application/octet-stream'});
    res.end(buf);
  });
});

(async ()=>{
  await new Promise(r=>server.listen(0,r));
  const port = server.address().port;
  const browser = await chromium.launch({ args:['--use-gl=angle','--use-angle=swiftshader',
    '--enable-unsafe-swiftshader','--no-sandbox','--ignore-gpu-blocklist'] });
  const page = await browser.newPage({ viewport:{width:1000,height:820} });
  page.on('console', m=>console.log('[page]', m.text()));
  page.on('pageerror', e=>console.log('[pageerror]', e.message));
  await page.goto(`http://localhost:${port}/index.html`,{waitUntil:'load',timeout:60000});
  await page.waitForTimeout(wait);
  await page.screenshot({ path: out });
  await browser.close(); server.close();
  console.log('screenshot saved ->', out);
})().catch(e=>{console.error('ERR',e); server.close(); process.exit(1);});
