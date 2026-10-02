const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('./playwright');
const buildDir=process.argv[2]||'../../web/dd2'; const OUT='/tmp/confinfo'; fs.mkdirSync(OUT,{recursive:true});
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const key=async(page,c)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(150);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(650);};
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
let errs=[];
async function run(page,label,navKeys){
  errs=[];
  for(const k of navKeys) await key(page,k);           // navigate to the target button
  await page.screenshot({path:`${OUT}/${label}_hover.png`});
  await key(page,'Enter');                              // enter the screen
  await page.waitForTimeout(1000);
  await page.screenshot({path:`${OUT}/${label}_enter.png`});
  console.log(`${label}: nav=[${navKeys}] alive=${await alive(page)} errs_on_enter=${errs.length} ${JSON.stringify([...new Set(errs)])}`);
  // cycle within the dialog
  await key(page,'ArrowDown'); await key(page,'ArrowUp');
  await page.screenshot({path:`${OUT}/${label}_cycled.png`});
  console.log(`  after cycle: alive=${await alive(page)} errs=${errs.length}`);
  await key(page,'Escape'); await page.waitForTimeout(600);   // back to main
}
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.split('\n')[0]));
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
  // Info (document icon, bottom row 2nd): Down then Right
  await run(page,'info',['ArrowDown','ArrowRight']);
  // Config (wrench, bottom row 3rd): Right from info
  await run(page,'config',['ArrowRight']);
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
