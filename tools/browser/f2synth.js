const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('playwright');
const buildDir=process.argv[2]||'../../web/dd2';
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const lbl=(page,a)=>page.evaluate(x=>{const p=HEAP32[x>>2];if(!(p>0x400000&&p<0x980000))return'?';let s='';for(let i=0;i<44;i++){const c=HEAPU8[p+i];if(!c)break;s+=String.fromCharCode(c);}return s;},a);
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage();
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){const r=await page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);if(r)break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
  const synth=(code,type)=>page.evaluate(([c,t])=>window.dispatchEvent(new KeyboardEvent(t,{code:c})),[code,type]);
  await synth('ArrowRight','keydown');await page.waitForTimeout(150);await synth('ArrowRight','keyup');await page.waitForTimeout(700);
  await synth('ArrowRight','keydown');await page.waitForTimeout(150);await synth('ArrowRight','keyup');await page.waitForTimeout(700);
  console.log('screen:',await lbl(page,0x46975c));
  for(let k=0;k<3;k++){
    await synth('F2','keydown');await page.waitForTimeout(150);await synth('F2','keyup');await page.waitForTimeout(700);
    console.log(`F2#${k+1}: track=`,await lbl(page,0x469784),'race_track=',await page.evaluate(()=>HEAP32[0x4673fc>>2]));
  }
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR',e);server.close();process.exit(1);});
