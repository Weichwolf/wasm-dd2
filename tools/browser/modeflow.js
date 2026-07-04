// E2E validation of the 816/817 dialog fixes: pick a race MODE + TYPE via the nested dialogs,
// then Go! -> race, verify crash-free. Tests alternate game modes (Destruction Derby etc.).
const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('playwright');
const buildDir=process.argv[2]||'../../web/dd2'; const OUT='/tmp/modeflow'; fs.mkdirSync(OUT,{recursive:true});
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const key=async(page,c)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(150);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(650);};
const down=(page,c)=>page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);
const up=(page,c)=>page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
const st=(page)=>page.evaluate(()=>[HEAP32[0x4673f8>>2],HEAP32[0x4673f4>>2],HEAP32[0x462ff0>>2]]).catch(()=>'DEAD'); // mode,type,cf
let errs=[];
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.split('\n')[0])); page.on('crash',()=>errs.push('PAGE CRASHED'));
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(9000); await page.click('#canvas');
  // open MODE dialog, pick Destruction Derby (Down x2), confirm
  await key(page,'Enter'); await key(page,'ArrowDown'); await key(page,'ArrowDown'); await key(page,'Enter');
  await page.waitForTimeout(800);
  console.log('after mode-select [mode,type,cf]='+await st(page)+' errs='+errs.length);
  // now in TYPE dialog: pick Practice (rec-1, Down x1 from Championship), confirm
  await key(page,'ArrowDown'); await key(page,'Enter');
  await page.waitForTimeout(800);
  console.log('after type-select [mode,type,cf]='+await st(page)+' errs='+errs.length);
  await page.screenshot({path:`${OUT}/config.png`});
  // navigate to Go! and launch: ArrowRight x2, ArrowDown x3, Enter
  for(const k of ['ArrowRight','ArrowRight','ArrowDown','ArrowDown','ArrowDown']) await key(page,k);
  await key(page,'Enter'); await page.waitForTimeout(14000);
  console.log('race launch [mode,type,cf]='+await st(page)+' errs='+errs.length);
  await page.screenshot({path:`${OUT}/race.png`});
  // drive 30s
  await down(page,'KeyA');
  for(let t=0;t<15;t++){const d=['ArrowLeft','ArrowRight'][t%2];await down(page,d);await page.waitForTimeout(1000);await up(page,d);
    if(t%5===0){const a=await alive(page);console.log(`  t=${t*2}s ${await st(page)} alive=${a} errs=${errs.length}`);if(!a||errs.length)break;}}
  await up(page,'KeyA');
  console.log('FINAL alive='+await alive(page)+' errs='+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
