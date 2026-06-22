// Verify multiple levels render: load, reach the race, then press 'N' to cycle tracks,
// screenshotting each.  node multishot.js <buildDir> <outDir> <count> [firstWaitMs] [stepMs]
const http=require('http'), fs=require('fs'), path=require('path');
const { chromium }=require('playwright');
const buildDir=process.argv[2]||'web/build', outDir=process.argv[3]||'out/levels';
const count=parseInt(process.argv[4]||'6',10), firstWait=parseInt(process.argv[5]||'8000',10), step=parseInt(process.argv[6]||'4000',10);
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream','.png':'image/png'};
const server=http.createServer((req,res)=>{ let p=decodeURIComponent(req.url.split('?')[0]); if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{ if(e){res.writeHead(404);res.end();return;} res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'}); res.end(b); }); });
(async()=>{
  fs.mkdirSync(outDir,{recursive:true});
  await new Promise(r=>server.listen(0,r)); const port=server.address().port;
  const browser=await chromium.launch({args:['--use-gl=angle','--use-angle=swiftshader','--enable-unsafe-swiftshader','--no-sandbox','--ignore-gpu-blocklist']});
  const page=await browser.newPage({viewport:{width:1000,height:820}});
  page.on('console',m=>console.log('[page]',m.text())); page.on('pageerror',e=>console.log('[pageerror]',e.message));
  await page.goto(`http://localhost:${port}/index.html`,{waitUntil:'load',timeout:60000});
  await page.waitForTimeout(firstWait);
  await page.mouse.click(500,400);                 // focus canvas
  for(let i=0;i<count;i++){
    await page.screenshot({path:path.join(outDir,`lvl_${i}.png`)});
    console.log('shot',i);
    await page.keyboard.press('n');                // next track
    await page.waitForTimeout(step);
  }
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR',e);server.close();process.exit(1);});
