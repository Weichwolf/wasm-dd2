const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('./playwright');
const buildDir=process.argv[2]||'../../web/dd2';
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const lbl=(page,a)=>page.evaluate(x=>{const q=HEAP32[x>>2];if(!(q>0x400000&&q<0x980000))return'?';let s='';for(let i=0;i<44;i++){const c=HEAPU8[q+i];if(!c)break;s+=String.fromCharCode(c);}return s;},a);
const key=async(page,c)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(150);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(900);};
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>console.log('PAGEERR '+e.message.split('\n')[0]));
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){const r=await page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);if(r)break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(9000); await page.click('#canvas');
  await key(page,'ArrowRight'); console.log('at: '+await lbl(page,0x46975c).catch(()=>'dead'));
  await key(page,'Enter');
  console.log('entered car-select, cycling cars + waiting...');
  for(let i=0;i<8;i++){
    await key(page,'F2');
    const alive=await page.evaluate(()=>typeof HEAP32!=='undefined'?HEAP32[0x467400>>2]:'DEAD').catch(()=>'DEAD');
    console.log('  after F2 #'+i+': race_car='+alive);
    if(alive==='DEAD'){console.log('CRASHED after F2 #'+i);break;}
  }
  await page.screenshot({path:'/tmp/carsel_browser.png'});
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
