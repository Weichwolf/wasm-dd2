// QA priority 2: PAUSE menu + RESULTS. arg2 = scenario: nav | retire | quit
// nav: pause, walk items 0..4, adjust Draw Distance + SFX Volume, resume.
// retire: pause, go to Retire(item3), Enter, sure?->Yes, observe results/replay screen + exercise it.
// quit:   pause, go to Quit(item4), Enter, sure?->Yes, observe result (title/results).
const http=require('http'),fs=require('fs'),path=require('path'); const {chromium}=require('./playwright');
const buildDir='../../web/dd2'; const SC=process.argv[2]||'nav';
const OUT='/tmp/qa_explore2'; fs.mkdirSync(OUT,{recursive:true});
const MIME={'.html':'text/html','.js':'text/javascript','.wasm':'application/wasm','.data':'application/octet-stream'};
const server=http.createServer((req,res)=>{let p=decodeURIComponent(req.url.split('?')[0]);if(p==='/')p='/index.html';
  fs.readFile(path.join(buildDir,p),(e,b)=>{if(e){res.writeHead(404);res.end();return;}res.writeHead(200,{'Content-Type':MIME[path.extname(p)]||'application/octet-stream'});res.end(b);});});
const key=async(page,c,post=650)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(150);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(post);};
const alive=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined').catch(()=>false);
const cf=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined'?HEAP32[0x462ff0>>2]:'DEAD').catch(()=>'DEAD');
const fe=(page)=>page.evaluate(()=>typeof HEAPU8!=='undefined'?HEAPU8[0x460005]:'?').catch(()=>'?'); // 201=FE main menu
let errs=[];
const snap=async(page,t)=>{await page.screenshot({path:`${OUT}/pause_${SC}_${t}.png`});};
const step=async(page,t)=>{const a=await alive(page);console.log(`  ${t}: alive=${a} cf=${await cf(page)} fe005=${await fe(page)} errs=${errs.length} ${errs.length?JSON.stringify([...new Set(errs)]):''}`);await snap(page,t);};
(async()=>{
  await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,160)));
  page.on('crash',()=>errs.push('PAGE CRASHED'));
  await page.goto(`http://localhost:${server.address().port}/index.html?race=9`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.click('#canvas'); await page.waitForTimeout(13000);
  console.log(`[${SC}] race running cf=`+await cf(page)+' errs='+errs.length);
  // enter pause
  await key(page,'Escape'); await page.waitForTimeout(800);
  const p1=await cf(page); await page.waitForTimeout(1200); const p2=await cf(page);
  console.log(`  pause: cf ${p1}->${p2} ${p1===p2?'FROZEN(paused OK)':'STILL ADVANCING(not paused!)'}`);
  await step(page,'00_paused');

  if(SC==='nav'){
    for(let i=1;i<=4;i++){await key(page,'ArrowDown');await step(page,`0${i}_down${i}`);}
    // now on item4(Quit). go back up to item1 (Draw Distance)
    await key(page,'ArrowUp');await key(page,'ArrowUp');await key(page,'ArrowUp');await step(page,'05_up_to_item1');
    // adjust Draw Distance with Right/Left
    await key(page,'ArrowRight');await key(page,'ArrowRight');await step(page,'06_drawdist_right');
    await key(page,'ArrowLeft');await step(page,'07_drawdist_left');
    // to item2 SFX Volume
    await key(page,'ArrowDown');await step(page,'08_item2_sfx');
    await key(page,'ArrowLeft');await key(page,'ArrowLeft');await step(page,'09_sfx_left');
    await key(page,'ArrowRight');await step(page,'10_sfx_right');
    // back to Continue (item0) and select -> resume
    await key(page,'ArrowUp');await key(page,'ArrowUp');await step(page,'11_item0_continue');
    await key(page,'Enter');await page.waitForTimeout(1500);
    const r1=await cf(page);await page.waitForTimeout(1500);const r2=await cf(page);
    console.log(`  resume via Continue: cf ${r1}->${r2} ${r2>r1?'RESUMED OK':'STILL FROZEN(hang!)'}`);
    await step(page,'12_resumed');
  }
  if(SC==='retire'){
    // item3 = Retire: ArrowDown x3
    await key(page,'ArrowDown');await key(page,'ArrowDown');await key(page,'ArrowDown');await step(page,'01_on_retire');
    await key(page,'Enter');await step(page,'02_sure_dialog');    // sure? Yes/No
    // Yes: local_1c starts 0(No). Down to Yes(1) then Enter
    await key(page,'ArrowDown');await step(page,'03_sure_yes_hi');
    await key(page,'Enter');await page.waitForTimeout(3000);await step(page,'04_after_retire_confirm');
    // let it settle then observe results screen; try navigating it
    await page.waitForTimeout(4000);await step(page,'05_settle');
    for(let i=0;i<4;i++){await key(page,'ArrowDown');await step(page,`06_res_down${i}`);}
    await key(page,'ArrowUp');await key(page,'Enter');await page.waitForTimeout(2500);await step(page,'07_res_enter');
    await key(page,'Escape');await page.waitForTimeout(1500);await step(page,'08_res_esc');
  }
  if(SC==='quit'){
    // item4 = Quit: ArrowDown x4
    for(let i=0;i<4;i++)await key(page,'ArrowDown');await step(page,'01_on_quit');
    await key(page,'Enter');await step(page,'02_sure2_dialog');
    await key(page,'ArrowDown');await step(page,'03_sure2_yes_hi');
    await key(page,'Enter');await page.waitForTimeout(4000);await step(page,'04_after_quit');
    await page.waitForTimeout(3000);await step(page,'05_settle');
  }
  console.log(`FINAL [${SC}] alive=`+await alive(page)+' errs='+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);try{server.close();}catch(_){}process.exit(1);});
