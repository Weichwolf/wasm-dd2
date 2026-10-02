// QA: select a Race Type by index, Enter, then drive to Go! and launch. Observe trap/launch.
// arg2 = type index (0..3). Mode fixed = Wrecking(0).
const {serve,key,alive,rd,menuLabel,boot,gotoButton}=require('./felib.js');
const {chromium}=require('./playwright'); const fs=require('fs');
const OUT='/tmp/qa_explore'; fs.mkdirSync(OUT,{recursive:true});
const TI=parseInt(process.argv[2]||'2');
const st=(page)=>page.evaluate(()=>({mode:HEAP32[0x4673f8>>2],type:HEAP32[0x4673f4>>2],fe:HEAP8[0x460005],cf:HEAP32[0x462ff0>>2],ncars:HEAP32[0x46765c>>2],lvl:HEAP32[0x936ff4>>2]})).catch(()=>'DEAD');
let errs=[];
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,160)));
  page.on('crash',()=>errs.push('PAGE CRASHED'));
  await boot(page,server);
  await key(page,'Enter'); await page.waitForTimeout(600);   // Race Mode
  await key(page,'Enter'); await page.waitForTimeout(900);   // confirm Wrecking -> Race Type
  for(let i=0;i<TI;i++){ await key(page,'ArrowRight'); }
  console.log(`type idx=${TI}: `+JSON.stringify(await st(page))+' errs='+errs.length+' '+JSON.stringify([...new Set(errs)]));
  await page.screenshot({path:`${OUT}/tl${TI}_sel.png`});
  await key(page,'Enter'); await page.waitForTimeout(1500);   // confirm type
  console.log(`after type Enter: `+JSON.stringify(await st(page))+' errs='+errs.length+' fe='+JSON.stringify([...new Set(errs)]));
  await page.screenshot({path:`${OUT}/tl${TI}_confirm.png`});
  const feNow=await page.evaluate(()=>HEAP8[0x460005]).catch(()=>'DEAD');
  // if back at main menu (fe==201/-55), goto Go! and launch; else press Enter to advance
  if(feNow===-55||feNow===201){
    const ok=await gotoButton(page,'Go!'); console.log('at Go!='+ok);
    await key(page,'Enter'); await page.waitForTimeout(14000);
  }else{
    await key(page,'Enter'); await page.waitForTimeout(14000);
  }
  console.log(`after launch attempt: `+JSON.stringify(await st(page))+' errs='+errs.length+' '+JSON.stringify([...new Set(errs)]));
  await page.screenshot({path:`${OUT}/tl${TI}_launch.png`});
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);process.exit(1);});
