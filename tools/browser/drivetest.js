// Interactive drive test: load the LIVE (demo_mode=0) browser build, focus the canvas, HOLD a key
// (keydown, no keyup) to drive the car, and screenshot at intervals so we can see the car respond.
//   node tools/browser/drivetest.js <buildDir> <outDir> <holdKey> [shots] [stepMs] [preMs]
// e.g. node drivetest.js /tmp/web_live /tmp/drive ArrowUp 4 900 6000
const http=require('http'),fs=require('fs'),path=require('path');const{chromium}=require('playwright');
const buildDir=process.argv[2]||'/tmp/web_live', outDir=process.argv[3]||'/tmp/drive';
const holdKey=process.argv[4]||'ArrowUp', shots=parseInt(process.argv[5]||'4',10);
const step=parseInt(process.argv[6]||'900',10), pre=parseInt(process.argv[7]||'6000',10);
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
fs.mkdirSync(outDir,{recursive:true});
const srv=http.createServer((q,r)=>{let p=q.url.split('?')[0];if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){r.writeHead(404);r.end();return;}
    r.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});r.end(b);});});
(async()=>{await new Promise(r=>srv.listen(0,r));const port=srv.address().port;
  const br=await chromium.launch({args:['--use-gl=angle','--use-angle=swiftshader','--enable-unsafe-swiftshader','--no-sandbox','--ignore-gpu-blocklist']});
  const pg=await br.newPage({viewport:{width:1000,height:820}});
  pg.on('pageerror',e=>console.log('[pageerror]',e.message));
  await pg.goto(`http://localhost:${port}/index.html`,{waitUntil:'load',timeout:60000});
  await pg.waitForTimeout(pre); await pg.mouse.click(500,400);            // focus canvas
  await pg.screenshot({path:path.join(outDir,'00_before.png')});
  await pg.keyboard.down(holdKey);                                        // HOLD the key
  for(let i=0;i<shots;i++){ await pg.waitForTimeout(step);
    await pg.screenshot({path:path.join(outDir,`${String(i+1).padStart(2,'0')}_${holdKey}.png`)}); }
  await pg.keyboard.up(holdKey);
  console.log('saved',shots+1,'shots ->',outDir);
  await br.close();srv.close();})().catch(e=>{console.error('ERR',e);srv.close();process.exit(1);});
