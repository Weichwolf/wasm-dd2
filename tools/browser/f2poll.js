const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('playwright');
const buildDir=process.argv[2]||'../../web/dd2';
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const synth=(page,c,t)=>page.evaluate(([cc,tt])=>window.dispatchEvent(new KeyboardEvent(tt,{code:cc})),[c,t]);
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage();
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){const r=await page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);if(r)break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
  await synth(page,'ArrowRight','keydown');await page.waitForTimeout(150);await synth(page,'ArrowRight','keyup');await page.waitForTimeout(700);
  await synth(page,'ArrowRight','keydown');await page.waitForTimeout(150);await synth(page,'ArrowRight','keyup');await page.waitForTimeout(700);
  // F2 down, then sample pad44a rapidly
  await synth(page,'F2','keydown');
  let seen=[];
  for(let i=0;i<12;i++){ const v=await page.evaluate(()=>[HEAPU16[0x75444a>>1],HEAPU8[0x46304e]]); seen.push(v.join('/')); await page.waitForTimeout(20);}
  await synth(page,'F2','keyup');
  console.log('pad44a/flag samples:', seen.join(' '));
  console.log('race_track:', await page.evaluate(()=>HEAP32[0x4673fc>>2]));
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR',e);server.close();process.exit(1);});
