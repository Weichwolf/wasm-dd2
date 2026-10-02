// Comprehensive FE crawler: visit every main button + sub-screen, exercise nav, report breaks.
// Fresh page reload per top-level button for deterministic nav. Prints PASS/FAIL summary.
// Usage: node fecrawl.js [buildDir]
const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('./playwright');
const buildDir=process.argv[2]||'../../web/dd2'; const OUT='/tmp/fecrawl'; fs.mkdirSync(OUT,{recursive:true});
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
let errs=[];
const key=async(page,c)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(140);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(560);};
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
const rd=(page,va)=>page.evaluate(a=>{const q=HEAPU32[a>>2];if(!(q>0x400000&&q<0x980000))return'';let s='';for(let i=0;i<40;i++){const c=HEAPU8[q+i];if(!c)break;s+=String.fromCharCode(c);}return s;},va).catch(()=>'ERR');
const results=[];
async function boot(page){
  await page.goto(`http://localhost:${server.address().port}/index.html?t=`+Math.floor(performance.now()),{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(8000); await page.click('#canvas');
}
// run a named scenario: navKeys to reach+exercise; record errs + final liveness
async function scenario(page,name,keys,opts={}){
  errs=[];
  await boot(page);
  if(opts.forceStats) await page.evaluate(()=>{HEAP32[0x46741c>>2]=1;});
  let died=false;
  for(const k of keys){ await key(page,k); if(!(await alive(page))){died=true;break;} }
  const a=await alive(page);
  const uniq=[...new Set(errs)];
  await page.screenshot({path:`${OUT}/${name}.png`});
  const pass = a && !died && uniq.length===0;
  results.push({name,pass,alive:a,died,errs:uniq});
  console.log(`${pass?'PASS':'FAIL'} ${name}: alive=${a} died=${died} errs=${uniq.length} ${uniq.length?JSON.stringify(uniq):''}`);
}
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,120)));
  // paths from boot (top-left race-opts). grid: [raceopts car track filemgr / cdplayer info config go]
  await scenario(page,'01_raceopts_mode', ['Enter','Enter','Escape','Escape']);          // race-opts -> mode dialog
  await scenario(page,'02_car',           ['ArrowRight','Enter','F2','F2','Escape']);      // Select Car + cycle
  await scenario(page,'03_track',         ['ArrowRight','ArrowRight','Enter','F2','Escape']); // Select Track + cycle
  await scenario(page,'04_filemgr',       ['ArrowRight','ArrowRight','ArrowRight','Enter','ArrowDown','ArrowUp','Escape']);
  await scenario(page,'05_cdplayer',      ['ArrowDown','Enter','ArrowRight','Enter','ArrowRight','Enter','ArrowLeft','Escape']);
  await scenario(page,'06_info_laptimes', ['ArrowDown','ArrowRight','Enter','Enter','Escape','Escape']);
  await scenario(page,'07_info_credits',  ['ArrowDown','ArrowRight','Enter','ArrowRight','Enter','Escape','Escape']);
  await scenario(page,'08_info_stats',    ['ArrowDown','ArrowRight','Enter','Enter','ArrowRight','ArrowRight','ArrowLeft','Escape','Escape'],{forceStats:true});
  await scenario(page,'09_config_ctrl',   ['ArrowDown','ArrowRight','ArrowRight','Enter','Enter','ArrowDown','ArrowUp','Escape','Escape']);
  await scenario(page,'10_config_sound',  ['ArrowDown','ArrowRight','ArrowRight','Enter','ArrowRight','Enter','ArrowRight','ArrowLeft','Escape','Escape']);
  await scenario(page,'11_config_save',   ['ArrowDown','ArrowRight','ArrowRight','Enter','ArrowRight','ArrowRight','Enter','Escape','Escape']);
  await scenario(page,'12_go_race',       ['ArrowDown','ArrowRight','ArrowRight','ArrowRight','Enter','Enter']); // Go! -> launch race
  console.log('\n=== SUMMARY ===');
  const fails=results.filter(r=>!r.pass);
  console.log(`${results.length-fails.length}/${results.length} PASS`);
  if(fails.length) fails.forEach(f=>console.log(`  FAIL ${f.name}: ${JSON.stringify(f.errs)}`));
  await browser.close(); server.close();
  process.exit(fails.length?2:0);
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
