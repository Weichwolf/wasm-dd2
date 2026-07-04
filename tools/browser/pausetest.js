const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('playwright');
const buildDir=process.argv[2]||'../../web/dd2';
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const key=async(page,c,hold=140)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(hold);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(650);};
const cf=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined'?HEAP32[0x462ff0>>2]:'DEAD').catch(()=>'DEAD');
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>console.log('PAGEERR '+e.message.split('\n').slice(0,3).join(' | ')));
  // boot directly into a race
  await page.goto(`http://localhost:${server.address().port}/index.html?race=7`,{waitUntil:'load'});
  for(let i=0;i<60;i++){const r=await page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);if(r)break;await page.waitForTimeout(1000);}
  await page.click('#canvas'); await page.waitForTimeout(12000);
  let c0=await cf(page); console.log('racing cf='+c0);
  // enter pause (Esc mapped to Start)
  await key(page,'Escape'); await page.waitForTimeout(1500);
  let cP1=await cf(page); await page.waitForTimeout(1500); let cP2=await cf(page);
  console.log('paused: cf1='+cP1+' cf2='+cP2+((cP1===cP2)?' (frozen=paused OK)':' (still advancing?)'));
  await page.screenshot({path:'/tmp/pause_menu.png'});
  // navigate DOWN in pause menu (should move highlight, sfx), then select Continue (top) via UP to 0 then Enter
  await key(page,'ArrowDown'); await key(page,'ArrowUp');
  // select item 0 = Continue (Enter/fire). fire bit needs the select edge -> Enter/Space
  await key(page,'Enter'); await page.waitForTimeout(2000);
  let cR1=await cf(page); await page.waitForTimeout(2500); let cR2=await cf(page);
  console.log('after select Continue: cf1='+cR1+' cf2='+cR2+((cR2>cR1)?' (RACE RESUMED ✓)':' (STILL FROZEN = hang)'));
  await page.screenshot({path:'/tmp/pause_resumed.png'});
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
