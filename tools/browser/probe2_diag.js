// Diagnostic: FE Go! race. Verify Escape pauses (cf freeze). Then retire, poll globals to see
// if Play_Game returns / Practice_Over reached. Globals: cf(0x462ff0), scr(0x460005),
// demo(demo_mode?), rt(race_type 0x4673f4), rf(_race_finished 0x467448), qf(_quit_flag).
const {serve,key,alive,boot,gotoButton,chromium}=require('./felib.js');
const fs=require('fs'); const OUT='/tmp/qa_explore2';
const G=(page)=>page.evaluate(()=>({cf:HEAP32[0x462ff0>>2],scr:HEAPU8[0x460005],rt:HEAP32[0x4673f4>>2]|0,rf:HEAP32[0x467448>>2]|0})).catch(()=>'DEAD');
let errs=[];
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,160)));
  await boot(page,server);
  await gotoButton(page,'Go!'); await key(page,'Enter',700); await page.waitForTimeout(15000);
  console.log('launched '+JSON.stringify(await G(page)));
  // pause freeze test
  await key(page,'Escape',300); const a=await G(page);
  await page.waitForTimeout(2000); const b=await G(page);
  console.log(`ESC pause: cf ${a.cf}->${b.cf} ${a.cf===b.cf?'FROZEN(paused OK)':'STILL RUNNING(NOT paused)'}`);
  // retire
  await key(page,'ArrowDown',400);await key(page,'ArrowDown',400);await key(page,'ArrowDown',400);
  await key(page,'Enter',400); await key(page,'ArrowDown',400); await key(page,'Enter',400);
  console.log('retire confirmed, polling 30s for screen change...');
  let prev=-1;
  for(let t=0;t<15;t++){
    await page.waitForTimeout(2000); const g=await G(page);
    if(g.scr!==prev){console.log(`  t=${t*2}s ${JSON.stringify(g)}`);prev=g.scr;}
  }
  await page.screenshot({path:`${OUT}/diag_final.png`});
  console.log('FINAL '+JSON.stringify(await G(page))+' errs='+JSON.stringify(errs));
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);try{server.close();}catch(_){}process.exit(1);});
