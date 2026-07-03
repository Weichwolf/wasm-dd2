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
  let f2seen=0; page.on('console',m=>{ if(m.text().includes('F2evt')) f2seen++; });
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){const r=await page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);if(r)break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000);
  await page.click('#canvas');
  // instrument: log keydowns
  await page.evaluate(()=>{ window.addEventListener('keydown',e=>console.log('F2evt',e.code),true); });
  await page.keyboard.down('ArrowRight');await page.waitForTimeout(120);await page.keyboard.up('ArrowRight');await page.waitForTimeout(700);
  await page.keyboard.down('ArrowRight');await page.waitForTimeout(120);await page.keyboard.up('ArrowRight');await page.waitForTimeout(700);
  // direct engine F2 (bypass DOM) — does Toggle_Track fire?
  const before=await page.evaluate(()=>HEAP32[0x4673fc>>2]);
  await page.keyboard.down('F2'); await page.waitForTimeout(150); await page.keyboard.up('F2');
  await page.waitForTimeout(600);
  const domres=await page.evaluate(()=>HEAP32[0x4673fc>>2]);
  const direct=await page.evaluate(()=>{
    Module.ccall('dd2_browser_key_event','number',['string','number'],['F2',1]);
    const r=HEAP32[0x4673fc>>2];
    Module.ccall('dd2_browser_key_event','number',['string','number'],['F2',0]);
    return r;
  });
  console.log('RESULT before='+before+' DOM_F2='+domres+' directVK_only(no-poll)='+direct+' f2keydowns='+f2seen);
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR',e);server.close();process.exit(1);});
