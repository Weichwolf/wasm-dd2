// Isolation test: does merely shifting wasm table indices break an EXISTING working FE dispatch?
// Opens Info + Config screens and navigates WITHIN them (call_indirect to rec setup handlers),
// WITHOUT drilling into any unreconstructed raw-VA action. Reports full RuntimeError text.
const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('./playwright');
const buildDir=process.argv[2]||'../../web/dd2';
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const key=async(page,c)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(150);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(650);};
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
let errs=[];
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.split('\n').slice(0,2).join(' | ')));
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
  console.log(`[${buildDir}] booted alive=${await alive(page)}`);
  // Info screen: Down,Right -> Enter to open
  for(const k of ['ArrowDown','ArrowRight']) await key(page,k);
  await key(page,'Enter'); await page.waitForTimeout(800);
  console.log(`  Info opened: alive=${await alive(page)} errs=${errs.length} ${JSON.stringify([...new Set(errs)])}`);
  // navigate categories within Info (setup call_indirects), do NOT drill in
  errs=[];
  for(const k of ['ArrowRight','ArrowRight','ArrowLeft','ArrowLeft']) await key(page,k);
  console.log(`  Info nav-categories: alive=${await alive(page)} errs=${errs.length} ${JSON.stringify([...new Set(errs)])}`);
  await key(page,'Escape'); await page.waitForTimeout(600); errs=[];
  // Config screen
  await key(page,'ArrowRight'); await key(page,'Enter'); await page.waitForTimeout(800);
  console.log(`  Config opened: alive=${await alive(page)} errs=${errs.length} ${JSON.stringify([...new Set(errs)])}`);
  errs=[];
  for(const k of ['ArrowRight','ArrowLeft']) await key(page,k);
  console.log(`  Config nav: alive=${await alive(page)} errs=${errs.length} ${JSON.stringify([...new Set(errs)])}`);
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
