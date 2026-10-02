// Per-key probe of the Info screen: capture FULL error text + which key triggers a bad call_indirect.
const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('./playwright');
const buildDir=process.argv[2]||'../../web/dd2';
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
let errs=[];
const key=async(page,c)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(150);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(650);};
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
const keyP=async(page,c)=>{errs=[];await key(page,c);const a=await alive(page);console.log(`  key ${c}: alive=${a} errs=${errs.length}`);errs.forEach(e=>console.log(`     >> ${e}`));};
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' \\n ').slice(0,300)));
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
  console.log(`booted alive=${await alive(page)}`);
  console.log('-- navigate to Info (Down,Right) & open (Enter) --');
  await keyP(page,'ArrowDown'); await keyP(page,'ArrowRight'); await keyP(page,'Enter');
  console.log('-- inside Info: each key probed --');
  await keyP(page,'ArrowRight'); await keyP(page,'ArrowRight');
  await keyP(page,'ArrowLeft'); await keyP(page,'ArrowDown'); await keyP(page,'ArrowUp');
  await keyP(page,'Escape');
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
