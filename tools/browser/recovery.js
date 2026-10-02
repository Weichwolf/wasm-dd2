// Does a deep-action trap crash the tab or does the FE recover?
const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('./playwright');
const buildDir=process.argv[2]||'../../web/dd2'; const OUT='/tmp/recov'; fs.mkdirSync(OUT,{recursive:true});
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const key=async(page,c)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(150);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(650);};
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
const cf=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined'?HEAP32[0x462ff0>>2]:'DEAD').catch(()=>'DEAD');
let errs=[],crashed=false;
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.split('\n')[0])); page.on('crash',()=>{crashed=true;});
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(9000); await page.click('#canvas');
  // go to Config (Right x2, Down x3 lands near... use Down+Right to bottom row wrench)
  await key(page,'ArrowDown'); await key(page,'ArrowRight'); await key(page,'ArrowRight');
  await key(page,'Enter');  // enter Config dialog
  await page.waitForTimeout(800);
  console.log('in Config: alive='+await alive(page)+' errs='+errs.length+' crashed='+crashed);
  // confirm a config sub-option (Enter on the deepest action) -> may trap
  await key(page,'Enter'); await page.waitForTimeout(1000);
  console.log('after confirm sub-option: alive='+await alive(page)+' errs='+errs.length+' crashed='+crashed+' '+JSON.stringify([...new Set(errs)]));
  // RECOVERY: press Esc a few times to back out, then try to launch a race
  await key(page,'Escape'); await key(page,'Escape'); await key(page,'Escape');
  await page.waitForTimeout(800);
  const aliveAfter=await alive(page);
  console.log('after Esc x3: alive='+aliveAfter+' crashed='+crashed);
  // try navigate + launch a race to prove FE still works
  await key(page,'ArrowUp'); await key(page,'ArrowRight'); await key(page,'ArrowRight');
  await key(page,'ArrowDown'); await key(page,'ArrowDown'); await key(page,'ArrowDown');
  await key(page,'Enter'); await page.waitForTimeout(12000);
  console.log('RECOVERY race launch: alive='+await alive(page)+' cf='+await cf(page)+' crashed='+crashed);
  await page.screenshot({path:`${OUT}/recovered.png`});
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
