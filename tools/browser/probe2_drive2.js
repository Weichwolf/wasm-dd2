// Priority 3: launch a race, drive (KeyA -> position/speed changes), pause(Esc)+resume(Continue).
// arg2 = # of F2 track cycles (different track each run).
const {serve,key,alive,boot,gotoButton,chromium}=require('./felib.js');
const fs=require('fs'); const OUT='/tmp/qa_explore2'; const TN=parseInt(process.argv[2]||'0');
const down=(page,c)=>page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);
const up=(page,c)=>page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);
const cf=(page)=>page.evaluate(()=>HEAP32[0x462ff0>>2]).catch(()=>'DEAD');
const rt=(page)=>page.evaluate(()=>HEAP32[0x4673fc>>2]|0).catch(()=>'?'); // race_track
const pos=(page)=>page.evaluate(()=>[HEAP32[0x792a24>>2],HEAP32[0x792a34>>2]]).catch(()=>'?');
let errs=[];
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,160)));
  page.on('crash',()=>errs.push('PAGE CRASHED'));
  await boot(page,server);
  await gotoButton(page,'Select Track');
  for(let i=0;i<TN;i++) await key(page,'F2',450);
  console.log(`[track F2x${TN}] race_track=`+await rt(page));
  // Select Track (row0 col2) -> Go! (row1 col3)
  for(const k of ['ArrowDown','ArrowDown','ArrowDown']) await key(page,k,450);
  await key(page,'Enter',700); await page.waitForTimeout(15000);
  const c0=await cf(page),p0=await pos(page);
  console.log(`  launched cf=${c0} pos=${p0} errs=${errs.length}`);
  await page.screenshot({path:`${OUT}/drive2_${TN}_start.png`});
  // drive
  await down(page,'KeyA'); await page.waitForTimeout(4000);
  await down(page,'ArrowLeft'); await page.waitForTimeout(1500); await up(page,'ArrowLeft');
  await up(page,'KeyA');
  const p1=await pos(page);
  console.log(`  after drive: pos=${p1} moved=${JSON.stringify(p0)!==JSON.stringify(p1)} cf=${await cf(page)}`);
  await page.screenshot({path:`${OUT}/drive2_${TN}_drove.png`});
  // pause + resume
  await key(page,'Escape',600);
  const a=await cf(page); await page.waitForTimeout(1500); const b=await cf(page);
  console.log(`  Esc pause: cf ${a}->${b} ${a===b?'FROZEN OK':'not paused'}`);
  await key(page,'Enter',700);   // Continue (item0 selected)
  const r1=await cf(page); await page.waitForTimeout(1500); const r2=await cf(page);
  console.log(`  resume: cf ${r1}->${r2} ${r2>r1?'RESUMED OK':'frozen'}`);
  console.log('FINAL alive='+await alive(page)+' errs='+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);try{server.close();}catch(_){}process.exit(1);});
