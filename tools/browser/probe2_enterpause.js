// Priority 4: does Enter EVER pause mid-race (vs Escape)? Launch race, tap Enter x5 with cf
// checks, then confirm Escape DOES pause. Precise behavior only.
const {serve,key,alive,boot,gotoButton,chromium}=require('./felib.js');
const cf=(page)=>page.evaluate(()=>HEAP32[0x462ff0>>2]).catch(()=>'DEAD');
let errs=[];
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,160)));
  await boot(page,server);
  await gotoButton(page,'Go!'); await key(page,'Enter',700); await page.waitForTimeout(15000);
  console.log('racing cf='+await cf(page));
  for(let i=0;i<5;i++){
    const a=await cf(page); await key(page,'Enter',400); const b=await cf(page);
    await page.waitForTimeout(1200); const c=await cf(page);
    console.log(`  Enter#${i}: cf ${a} -> ${b} -> +1.2s ${c}  ${b===c?'FROZEN after Enter':'still advancing'}`);
  }
  // now Escape (should pause)
  const e0=await cf(page); await key(page,'Escape',400); const e1=await cf(page);
  await page.waitForTimeout(1500); const e2=await cf(page);
  console.log(`  Escape: cf ${e0} -> ${e1} -> +1.5s ${e2}  ${e1===e2?'FROZEN(paused OK)':'NOT paused'}`);
  await page.screenshot({path:'/tmp/qa_explore2/enterpause_esc.png'});
  console.log('errs='+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);try{server.close();}catch(_){}process.exit(1);});
