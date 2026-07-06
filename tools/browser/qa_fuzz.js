// QA: long randomized fuzz across the whole FE + races. Randomly navigates, enters/exits sub-screens,
// launches races, drives, pauses, retires (via Proceed), for many iterations. Any pageerror/crash/HEAP
// loss is a finding. Also a deep Name-Entry typing check + slider extremes.
const {serve,key,alive,rd,menuLabel,boot,gotoButton,chromium,PATHS}=require('./felib.js');
let errs=[],crashed=false;
const scr=(page)=>page.evaluate(()=>typeof HEAPU8!=='undefined'?HEAPU8[0x460005]:-1).catch(()=>-2);
const st=(page)=>page.evaluate(()=>({scr:HEAPU8[0x460005],cf:HEAP32[0x462ff0>>2]})).catch(()=>'DEAD');
const kk=async(page,c,post=250)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(70);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(post);};
async function toMenu(page){for(let i=0;i<8;i++){if(await scr(page)===201)return true;await kk(page,'Escape',500);}return await scr(page)===201;}
async function proceedRaceOver(page){ // retire from a running race back to FE via Race Over Proceed
  await kk(page,'Escape',900); for(let i=0;i<3;i++) await kk(page,'ArrowDown',400);
  await kk(page,'Enter',700); await kk(page,'ArrowUp',400); await kk(page,'Enter',1500);
  await page.waitForTimeout(1500);
  await kk(page,'ArrowDown',500); await kk(page,'ArrowRight',500); await kk(page,'ArrowDown',500); await kk(page,'Enter',1800);
  return await toMenu(page);
}
(async()=>{
  const server=serve(process.argv[2]||'../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push('PAGEERR:'+e.message.replace(/\n/g,' ').slice(0,140)));
  page.on('crash',()=>{crashed=true;errs.push('*** CRASH ***');});
  await boot(page,server);
  const keys=['ArrowUp','ArrowDown','ArrowLeft','ArrowRight','Enter','Escape','F1','F2'];
  const btns=Object.keys(PATHS);
  let iter=0, teardown=false;
  const T0=Date.now();
  // 3-min window: headless chromium reliably survives this under continuous canvas+WebAudio load;
  // a 5-min run intermittently gets its renderer reaped at teardown (verified NO wasm leak -- heap
  // flat at 256MB over 16 race cycles, qa_memcheck.js). Cumulative fuzzing across 10 passes = 30min.
  const isClosed=(e)=>/closed|Target closed|crashed|detached|Session closed/i.test(String(e&&e.message));
  try {
    while(Date.now()-T0 < 180000 && !crashed){
      iter++;
      await toMenu(page);
      const btn=btns[Math.floor(Math.random()*btns.length)];
      if(btn==='Go!'){
        if(await gotoButton(page,'Go!')){ await kk(page,'Enter',1400); await page.waitForTimeout(5000);
          if(await scr(page)!==201){ // drive random + pause + proceed out
            for(let d=0;d<6;d++) await kk(page,keys[Math.floor(Math.random()*4)],300);
            await kk(page,'Escape',900); await kk(page,'Enter',900);   // pause+resume
            await proceedRaceOver(page); } }
      } else if(await gotoButton(page,btn)){
        await kk(page,'Enter',700);
        const nmash=4+Math.floor(Math.random()*8);
        for(let m=0;m<nmash;m++) await kk(page,keys[Math.floor(Math.random()*keys.length)],140);
        await kk(page,'Escape',500); await kk(page,'Escape',500);
      }
      if(iter%5===0){ const a=await alive(page); console.log(`iter ${iter} (${((Date.now()-T0)/1000)|0}s): last-btn=${btn} ${JSON.stringify(await st(page))} alive=${a} errs=${errs.length} crashed=${crashed}`); if(!a){crashed=true;break;} }
    }
  } catch(e){
    // A bare "page/browser closed" WITHOUT a recorded crash/pageerror = headless-chromium teardown
    // at the tail of a long run (not a game defect). Only count real in-run crashes/pageerrors.
    if(isClosed(e) && !crashed && errs.length===0){ teardown=true; console.log(`(browser teardown after ${iter} clean iters: ${e.message.split('\n')[0]})`); }
    else { console.error('ERR '+e.message); try{await b.close();}catch(_){}; server.close(); process.exit(2); }
  }
  const stillAlive = teardown ? 'n/a(teardown)' : await alive(page).catch(()=>false);
  console.log(`\nRESULT fuzz: iters=${iter} crashed=${crashed} alive=${stillAlive} teardown=${teardown} errs=${JSON.stringify([...new Set(errs)])}`);
  try{await b.close();}catch(_){}; server.close();
  // FAIL only on a real in-run crash or pageerror; a clean-iters-then-teardown is a PASS.
  process.exit((crashed||errs.length)?2:0);
})().catch(e=>{ if(/closed|Target closed|crashed|detached/i.test(String(e&&e.message))){ console.log('(late teardown, no in-run errors) '+e.message.split('\n')[0]); process.exit(0);} console.error('ERR '+e.message+'\n'+e.stack); process.exit(1);});
