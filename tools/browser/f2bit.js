const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('playwright');
const buildDir=process.argv[2]||'../../web/dd2';
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage();
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){const r=await page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);if(r)break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
  // raw pad flag DAT_0046304e (F2 target) + polled mask, via direct ccall (no DOM timing)
  const r=await page.evaluate(()=>{
    const flag_before=HEAPU8[0x46304e];
    Module.ccall('dd2_browser_key_event','number',['string','number'],['F2',1]);
    const flag_after=HEAPU8[0x46304e];
    const vk=Module.ccall('dd2_browser_key_event','number',['string','number'],['F1',1]);
    Module.ccall('dd2_browser_key_event','number',['string','number'],['F2',0]);
    Module.ccall('dd2_browser_key_event','number',['string','number'],['F1',0]);
    return {flag_before,flag_after,f1vk:vk,flag46304a:HEAPU8[0x46304a],flag463047:HEAPU8[0x463047]};
  });
  console.log('F2BIT:', JSON.stringify(r));
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR',e);server.close();process.exit(1);});
