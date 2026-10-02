const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('./playwright');
const buildDir=process.argv[2]||'../../web/dd2'; const OUT='/tmp/raceflow'; fs.mkdirSync(OUT,{recursive:true});
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const key=async(page,c)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(150);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(700);};
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
const st=(page)=>page.evaluate(()=>[HEAP32[0x4673f8>>2],HEAP32[0x4673f4>>2]]).catch(()=>'?'); // race_mode, race_type
let errs=[];
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.split('\n')[0]));
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
  await key(page,'Enter');                                  // open MODE dialog
  console.log('mode dialog open, errs='+errs.length+' [mode,type]='+await st(page));
  await key(page,'Enter');                                  // confirm mode -> opens TYPE dialog
  await page.waitForTimeout(1000);
  await page.screenshot({path:`${OUT}/1_type.png`});
  console.log('after confirm mode -> type dialog, alive='+await alive(page)+' errs='+errs.length+' [mode,type]='+await st(page));
  for(const k of ['ArrowDown','ArrowDown','ArrowUp']){       // cycle race types
    await key(page,k);
    console.log('  after '+k+': alive='+await alive(page)+' errs='+errs.length);
  }
  await page.screenshot({path:`${OUT}/2_type_cycled.png`});
  await key(page,'Enter');                                   // confirm a type
  await page.waitForTimeout(1200);
  await page.screenshot({path:`${OUT}/3_after.png`});
  console.log('after confirm type, alive='+await alive(page)+' errs='+errs.length+' [mode,type]='+await st(page));
  console.log('ALL ERRS: '+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
