const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('./playwright');
const buildDir=process.argv[2]||'../../web/dd2'; const OUT='/tmp/stats2'; fs.mkdirSync(OUT,{recursive:true});
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const key=async(page,c)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(150);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(650);};
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
let errs=[],crashed=false;
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>{errs.push(e.message.split(String.fromCharCode(10))[0]); console.log('STACK: '+((e.stack||'').split(String.fromCharCode(10)).slice(0,8).join(' || ')));}); page.on('crash',()=>crashed=true);
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(9000); await page.click('#canvas');
  await key(page,'ArrowDown'); await key(page,'ArrowRight');       // Info button
  await key(page,'Enter'); await page.waitForTimeout(700);          // open Info dialog
  await page.screenshot({path:`${OUT}/0_dialog.png`});
  await key(page,'ArrowLeft'); await key(page,'ArrowLeft');        // move to rec-0 (leftmost = View Statistics)
  await page.screenshot({path:`${OUT}/0b_rec0.png`});
  await key(page,'Enter'); await page.waitForTimeout(900);          // confirm View Statistics
  await page.screenshot({path:`${OUT}/1_stats.png`});
  console.log('View Statistics: alive='+await alive(page)+' crashed='+crashed+' errs='+errs.length+' '+JSON.stringify([...new Set(errs)]));
  await key(page,'ArrowRight'); await page.screenshot({path:`${OUT}/2_cat_next.png`});
  console.log('after next category: alive='+await alive(page)+' errs='+errs.length);
  await key(page,'ArrowRight'); await page.screenshot({path:`${OUT}/3_cat2.png`});
  await key(page,'Escape'); await page.waitForTimeout(700);
  console.log('after exit: alive='+await alive(page)+' crashed='+crashed+' errs='+errs.length);
  console.log('FINAL: '+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
