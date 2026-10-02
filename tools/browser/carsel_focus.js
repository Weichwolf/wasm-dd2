const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('./playwright');
const buildDir=process.argv[2]||'../../web/dd2'; const OUT='/tmp/carsel'; fs.mkdirSync(OUT,{recursive:true});
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const key=async(page,c)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(150);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(700);};
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
const rc=(page)=>page.evaluate(()=>HEAP32[0x467400>>2]).catch(()=>'?');
let errs=[]; 
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.split('\n')[0]));
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
  await page.screenshot({path:`${OUT}/0_main.png`});
  await key(page,'ArrowRight');                         // move to car button
  await page.screenshot({path:`${OUT}/1_car_hover.png`});
  console.log('at car button, errs='+errs.length+' race_car='+await rc(page));
  await key(page,'Enter');                              // activate Select_Car
  await page.waitForTimeout(1500);
  await page.screenshot({path:`${OUT}/2_car_active.png`});
  console.log('after Enter(Select_Car), alive='+await alive(page)+' errs='+errs.length);
  // cycle cars inside Select_Car with Left/Right
  for(const k of ['ArrowRight','ArrowRight','ArrowLeft']){ await key(page,k); }
  await page.screenshot({path:`${OUT}/3_car_cycled.png`});
  console.log('after cycling, alive='+await alive(page)+' race_car='+await rc(page)+' errs='+errs.length);
  // confirm the car (accept)
  await key(page,'Enter'); await page.waitForTimeout(1500);
  await page.screenshot({path:`${OUT}/4_car_confirmed.png`});
  console.log('after confirm, alive='+await alive(page)+' errs='+errs.length);
  console.log('ALL ERRS: '+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
