// Visual menu tour: boots the FE, walks all 8 sub-screens, enters each (accept),
// screenshots every state, and reports which screen traps. Robust: polls HEAP for
// boot, keys via real KeyboardEvents, per-step pageerror capture.
const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('./playwright');
const buildDir=process.argv[2]||'../../web/dd2';
const OUT='/tmp/menutour'; fs.mkdirSync(OUT,{recursive:true});
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
let curStep='boot', errsByStep={};
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
const key=async(page,c,hold=120)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(hold);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(550);};
// screen index heuristic: read the selection-ring X (_DAT_0046965c, short) as a state signal
const ringx=(page)=>page.evaluate(()=>{const v=(HEAPU16?HEAPU16:new Uint16Array(HEAP8.buffer))[0x46965c>>1];return v;}).catch(()=>'?');
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>{(errsByStep[curStep]=errsByStep[curStep]||[]).push(e.message.split('\n')[0]);});
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.waitForTimeout(9000); await page.click('#canvas'); await page.waitForTimeout(1500);
  curStep='main'; await page.screenshot({path:`${OUT}/00_main.png`});
  console.log('booted, ring_x='+await ringx(page));
  // Walk the 8 screens: press Right to move the selection ring across the menu row,
  // screenshot each, then Enter to activate, screenshot, Escape back.
  const names=['s0_race','s1_car','s2_track','s3_filemgr','s4_cdplayer','s5_info','s6_config','s7_go'];
  for(let i=0;i<8;i++){
    curStep=names[i];
    await page.screenshot({path:`${OUT}/${String(i+1).padStart(2,'0')}a_${names[i]}_hover.png`});
    const rx=await ringx(page);
    // activate
    await key(page,'Enter'); await page.waitForTimeout(1200);
    await page.screenshot({path:`${OUT}/${String(i+1).padStart(2,'0')}b_${names[i]}_enter.png`});
    const a=await alive(page);
    console.log(`screen ${i} ${names[i]}: ring_x=${rx} alive_after_enter=${a} errs=${(errsByStep[curStep]||[]).length}`);
    // back out
    await key(page,'Escape'); await page.waitForTimeout(800);
    if(!a){console.log('  -> DEAD, reloading'); break;}
    // move to next screen
    await key(page,'ArrowRight');
  }
  console.log('\n=== PAGEERRORS BY SCREEN ===');
  for(const k of Object.keys(errsByStep)) console.log(`  ${k}: ${JSON.stringify([...new Set(errsByStep[k])])}`);
  await browser.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
