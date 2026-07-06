// Map the main-menu ring: press each arrow from boot and log the selected-button label @0x46975c.
const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('playwright');
const buildDir=process.argv[2]||'../../web/dd2';
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
let errs=[];
const key=async(page,c)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(150);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(600);};
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
const rd=(page,va)=>page.evaluate(a=>{const q=HEAPU32[a>>2];if(!(q>0x400000&&q<0x980000))return'<'+q+'>';let s='';for(let i=0;i<40;i++){const c=HEAPU8[q+i];if(!c)break;s+=String.fromCharCode(c);}return s;},va).catch(()=>'ERR');
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.split('\n')[0]));
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
  console.log(`boot menu="${await rd(page,0x46975c)}"`);
  for(const dir of ['ArrowRight','ArrowRight','ArrowRight','ArrowRight','ArrowRight','ArrowRight','ArrowRight','ArrowRight']){
    await key(page,dir); console.log(`  ${dir} -> "${await rd(page,0x46975c)}"`);
  }
  console.log('--- now Down/Up cycles ---');
  for(const dir of ['ArrowDown','ArrowDown','ArrowUp','ArrowUp']){
    await key(page,dir); console.log(`  ${dir} -> "${await rd(page,0x46975c)}"`);
  }
  console.log(`errs=${errs.length}`);
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
