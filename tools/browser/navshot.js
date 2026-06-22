// Navigate the front-end with key presses, then screenshot.
//   node tools/browser/navshot.js <out.png> <keys-csv> [preWaitMs] [postWaitMs]
// keys-csv e.g. "ArrowDown,Enter" (pressed in order with small gaps).
const http=require('http'),fs=require('fs'),path=require('path');const{chromium}=require('playwright');
const out=process.argv[2]||'out/nav.png', keys=(process.argv[3]||'').split(',').filter(Boolean);
const pre=parseInt(process.argv[4]||'2500',10), post=parseInt(process.argv[5]||'900',10);
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const srv=http.createServer((q,r)=>{let p=q.url.split('?')[0];if(p==='/')p='/index.html';
  fs.readFile('web/build'+p,(e,b)=>{if(e){r.writeHead(404);r.end();return;}r.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});r.end(b);});});
(async()=>{await new Promise(r=>srv.listen(0,r));const port=srv.address().port;
  const br=await chromium.launch({args:['--use-gl=angle','--use-angle=swiftshader','--enable-unsafe-swiftshader','--no-sandbox','--ignore-gpu-blocklist']});
  const pg=await br.newPage({viewport:{width:1000,height:820}});pg.on('console',m=>console.log('[page]',m.text()));
  await pg.goto(`http://localhost:${port}/index.html`,{waitUntil:'load',timeout:60000});
  await pg.waitForTimeout(pre); await pg.mouse.click(500,400);
  for(const k of keys){ await pg.keyboard.press(k); await pg.waitForTimeout(200); }
  await pg.waitForTimeout(post); await pg.screenshot({path:out}); console.log('saved',out);
  await br.close();srv.close();})().catch(e=>{console.error('ERR',e);srv.close();process.exit(1);});
