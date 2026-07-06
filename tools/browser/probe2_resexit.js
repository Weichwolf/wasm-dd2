// After retire from FE single race, sweep candidate keys to exit the replay and reach the
// results menu (Practice_Over). Watch screen byte 0x460005 for change; screenshot each key.
const {serve,key,alive,boot,gotoButton,chromium}=require('./felib.js');
const fs=require('fs'); const OUT='/tmp/qa_explore2';
const G=(page)=>page.evaluate(()=>({cf:HEAP32[0x462ff0>>2],scr:HEAPU8[0x460005]})).catch(()=>'DEAD');
let errs=[];
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,160)));
  await boot(page,server);
  await gotoButton(page,'Go!'); await key(page,'Enter',700); await page.waitForTimeout(15000);
  // retire
  await key(page,'Escape',400);
  await key(page,'ArrowDown',350);await key(page,'ArrowDown',350);await key(page,'ArrowDown',350);
  await key(page,'Enter',350); await key(page,'ArrowDown',350); await key(page,'Enter',600);
  console.log('retired '+JSON.stringify(await G(page)));
  // let replay play a while
  await page.waitForTimeout(12000);
  console.log('after 12s replay '+JSON.stringify(await G(page)));
  await page.screenshot({path:`${OUT}/resexit_00_replay.png`});
  // sweep keys
  for(const k of ['KeyA','Space','Enter','Escape','KeyA','Enter','Escape','Enter']){
    const before=await G(page);
    await key(page,k,900);
    const after=await G(page);
    console.log(`  ${k}: scr ${before.scr}->${after.scr} cf ${before.cf}->${after.cf} ${after.scr!==before.scr?'*** SCREEN CHANGED ***':''}`);
    await page.screenshot({path:`${OUT}/resexit_${k}_${after.scr}.png`});
  }
  // final long watch
  for(let t=0;t<10;t++){await page.waitForTimeout(2000);const g=await G(page);console.log(`  watch t=${t*2}s ${JSON.stringify(g)}`);if(g.scr===201){console.log('  -> MAIN MENU');break;}}
  console.log('FINAL '+JSON.stringify(await G(page))+' errs='+JSON.stringify(errs));
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);try{server.close();}catch(_){}process.exit(1);});
