// Browser track-select E2E: boot FE, navigate to Select Track, cycle the track (F2),
// go to Go!, accept, verify a race on the chosen level starts. Reads level from wasm heap.
const http = require('http'); const fs = require('fs'); const path = require('path');
const { chromium } = require('playwright');
const buildDir = process.argv[2] || '../../web/dd2';
const MIME = {'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server = http.createServer((req,res)=>{
  let p = decodeURIComponent(req.url.split('?')[0]); if (p==='/') p='/index.html';
  fs.readFile(path.join(buildDir,p),(err,buf)=>{ if(err){res.writeHead(404);res.end();return;}
    res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'}); res.end(buf); });
});
const label = (page,addr)=>page.evaluate(a=>{
  const p=HEAP32[a>>2]; if(!(p>0x400000&&p<0x980000))return '?';
  let s=''; for(let i=0;i<44;i++){const c=HEAPU8[p+i]; if(!c)break; s+=String.fromCharCode(c);} return s;
}, addr);
(async ()=>{
  await new Promise(r=>server.listen(0,r));
  const browser = await chromium.launch({ args:['--no-sandbox'] });
  const page = await browser.newPage({viewport:{width:700,height:520}});
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<90;i++){ const r=await page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false); if(r)break; await page.waitForTimeout(1000);}
  await page.waitForTimeout(9000);          // FE settle (beat the attract idle-timeout)
  await page.click('#canvas');
  await page.keyboard.down('ArrowRight'); await page.waitForTimeout(250); await page.keyboard.up('ArrowRight'); await page.waitForTimeout(1200);
  await page.keyboard.down('ArrowRight'); await page.waitForTimeout(250); await page.keyboard.up('ArrowRight'); await page.waitForTimeout(1200);
  console.log('at:', await label(page,0x46975c), '/', await label(page,0x469784));
  await page.keyboard.down('F2'); await page.waitForTimeout(250); await page.keyboard.up('F2'); await page.waitForTimeout(1200);   // cycle track
  console.log('after F2:', await label(page,0x469784), 'race_track=', await page.evaluate(()=>HEAP32[0x4673fc>>2]));
  await page.keyboard.down('ArrowDown'); await page.waitForTimeout(250); await page.keyboard.up('ArrowDown'); await page.waitForTimeout(1000);
  await page.keyboard.down('ArrowDown'); await page.waitForTimeout(250); await page.keyboard.up('ArrowDown'); await page.waitForTimeout(1000);
  await page.keyboard.down('ArrowDown'); await page.waitForTimeout(250); await page.keyboard.up('ArrowDown'); await page.waitForTimeout(1000);
  console.log('at:', await label(page,0x46975c));
  await page.keyboard.down('Enter'); await page.waitForTimeout(250); await page.keyboard.up('Enter'); await page.waitForTimeout(22000);  // Go!
  const st = await page.evaluate(()=>({level:HEAP32[0x936ff4>>2], demo:HEAP32[0x46385c>>2], cf:HEAP32[0x462ff0>>2]}));
  console.log('RACE:', JSON.stringify(st));
  await page.screenshot({path:'/tmp/btrack_race.png'});
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR',e); server.close(); process.exit(1);});
