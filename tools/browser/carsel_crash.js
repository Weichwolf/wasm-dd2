const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('./playwright');
const buildDir=process.argv[2]||'../../web/dd2';
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const key=async(page,c,hold=130)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(hold);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(600);};
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
const rc=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined'?HEAP32[0x467400>>2]:'DEAD').catch(()=>'DEAD');
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  let errs=0; page.on('pageerror',e=>{errs++;console.log('PAGEERR '+e.message.split('\n').slice(0,3).join(' | '));});
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(9000); await page.click('#canvas');
  // main menu -> RIGHT to Select Car screen (screen1). Nav: default screen0 -> Right = screen1 (car)
  await key(page,'ArrowRight');
  await page.waitForTimeout(800);
  console.log('at car-select, alive='+await alive(page));
  // cycle cars with arrows (Left/Right may cycle; also try Up/Down)
  for(const k of ['ArrowLeft','ArrowLeft','ArrowRight','ArrowRight','ArrowUp','ArrowDown']){
    await key(page,k); const a=await alive(page); if(!a){console.log('DIED after '+k);break;}
  }
  console.log('after cycling, alive='+await alive(page)+' race_car='+await rc(page));
  await page.screenshot({path:'/tmp/carsel_cycled.png'});
  // select the car (Enter) -> should proceed to next screen / start race
  await key(page,'Enter'); await page.waitForTimeout(3000);
  console.log('after Enter(select), alive='+await alive(page));
  await page.waitForTimeout(4000);
  console.log('final alive='+await alive(page)+' errs='+errs);
  await page.screenshot({path:'/tmp/carsel_after_select.png'});
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
