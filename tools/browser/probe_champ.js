// QA: Championship flow (Race Type idx0). Select mode, Championship, Enter -> expect name-entry
// alphabet grid. Try entering letters + confirm. Screenshot each step. Watch for trap/hang.
const {serve,key,alive,rd,menuLabel,boot,gotoButton}=require('./felib.js');
const {chromium}=require('./playwright'); const fs=require('fs');
const OUT='/tmp/qa_explore'; fs.mkdirSync(OUT,{recursive:true});
const st=(page)=>page.evaluate(()=>({mode:HEAP32[0x4673f8>>2],type:HEAP32[0x4673f4>>2],fe:HEAP8[0x460005],cf:HEAP32[0x462ff0>>2],lvl:HEAP32[0x936ff4>>2]})).catch(()=>'DEAD');
let errs=[];
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,160)));
  page.on('crash',()=>errs.push('PAGE CRASHED'));
  await boot(page,server);
  await key(page,'Enter'); await page.waitForTimeout(500);   // Race Mode
  await key(page,'Enter'); await page.waitForTimeout(800);   // Wrecking -> Race Type (idx0=Championship)
  console.log('Race Type, at Championship: '+JSON.stringify(await st(page))+' errs='+errs.length);
  await key(page,'Enter'); await page.waitForTimeout(1500);  // confirm Championship
  console.log('after Championship Enter: '+JSON.stringify(await st(page))+' errs='+errs.length+' '+JSON.stringify([...new Set(errs)]));
  await page.screenshot({path:`${OUT}/champ_1.png`});
  // if a name-entry grid appears, try picking a few letters + navigating
  for(const k of ['ArrowRight','Enter','ArrowRight','Enter','ArrowDown','Enter']){
    await key(page,k);
    console.log(`  ${k}: `+JSON.stringify(await st(page))+' errs='+errs.length+(errs.length?' '+JSON.stringify([...new Set(errs)]):''));
    if(!(await alive(page))){console.log('  DEAD');break;}
  }
  await page.screenshot({path:`${OUT}/champ_2.png`});
  // try to confirm/done (Escape or Enter-hold)
  await key(page,'Escape'); await page.waitForTimeout(1000);
  console.log('after Escape: '+JSON.stringify(await st(page))+' errs='+errs.length);
  await page.screenshot({path:`${OUT}/champ_3.png`});
  console.log('FINAL ERRS: '+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);process.exit(1);});
