const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('playwright');
const buildDir=process.argv[2]||'../../web/dd2';
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const lbl=(page,a)=>page.evaluate(x=>{const p=HEAP32[x>>2];if(!(p>0x400000&&p<0x980000))return'?';let s='';for(let i=0;i<44;i++){const c=HEAPU8[p+i];if(!c)break;s+=String.fromCharCode(c);}return s;},a);
const key=async(page,c)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(150);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(700);};
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage(); page.on('pageerror',e=>console.log('PAGEERR '+e.message.slice(0,140)));
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){const r=await page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);if(r)break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
  await key(page,'ArrowRight'); await key(page,'ArrowRight');
  console.log('screen:',await lbl(page,0x46975c));
  console.log('SAVED: d93de10(demo-save)='+await page.evaluate(()=>HEAP32[0x93de10>>2])+' ncars='+await page.evaluate(()=>HEAP32[0x46765c>>2]));
  await key(page,'F2'); await key(page,'F2');   // cycle to S.C.A. Motorplex (track 2)
  console.log('track:',await lbl(page,0x469784),'race_track=',await page.evaluate(()=>HEAP32[0x4673fc>>2]));
  await key(page,'ArrowDown'); await key(page,'ArrowDown'); await key(page,'ArrowDown');
  console.log('at:',await lbl(page,0x46975c));
  // poll num_cars densely across the Enter launch
  let nc=[];
  await page.evaluate(()=>window.dispatchEvent(new KeyboardEvent('keydown',{code:'Enter'})));
  for(let i=0;i<50;i++){ nc.push(await page.evaluate(()=>HEAP32[0x46765c>>2])); await page.waitForTimeout(200); }
  await page.evaluate(()=>window.dispatchEvent(new KeyboardEvent('keyup',{code:'Enter'})));
  console.log('NCARS-trace:',nc.join(','));
  for(let t=0;t<2;t++){ await page.waitForTimeout(4000);
    console.log('RACE t'+t+': '+JSON.stringify(await page.evaluate(()=>({level:HEAP32[0x936ff4>>2],demo:HEAP32[0x46385c>>2],cf:HEAP32[0x462ff0>>2],car0:HEAP32[0x792a24>>2]+','+HEAP32[0x792a34>>2],ncars:HEAP32[0x46765c>>2],rcar:HEAP32[0x467400>>2],c2:HEAP32[(0x792a24+2*0x1b2)>>2]}))));
  }
  await page.screenshot({path:'/tmp/btrack_race.png'});
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR',e);server.close();process.exit(1);});
