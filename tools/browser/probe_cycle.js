// QA: determine the correct cycle interaction for Track/Car. Test (A) main-menu F2 on the
// highlighted button (no Enter), and (B) inside the opened screen: arrows + F2.
const {serve,key,alive,rd,menuLabel,boot,gotoButton}=require('./felib.js');
const {chromium}=require('./playwright'); const fs=require('fs');
const OUT='/tmp/qa_explore';
const rt=(page)=>page.evaluate(()=>HEAP32[0x4673fc>>2]).catch(()=>'?');   // race_track
const rc=(page)=>page.evaluate(()=>HEAP32[0x467400>>2]).catch(()=>'?');   // race_car
const lbl2=(page)=>rd(page,0x469784);
let errs=[];
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,140)));
  await boot(page,server);
  // ---- (A) main-menu F2 on Select Track (trackselect.js model) ----
  await gotoButton(page,'Select Track');
  console.log('[A] menu Select Track highlighted, race_track='+await rt(page)+' lbl='+await lbl2(page));
  for(let i=1;i<=5;i++){ await key(page,'F2'); console.log(`   menu F2#${i}: race_track=${await rt(page)} lbl="${await lbl2(page)}" errs=${errs.length}`); }
  await page.screenshot({path:`${OUT}/cyc_track_menuF2.png`});
  // ---- (B) open the screen, try arrows ----
  await key(page,'Enter'); await page.waitForTimeout(900);
  console.log('[B] opened Track Select, race_track='+await rt(page));
  for(const k of ['ArrowRight','ArrowRight','F2','ArrowLeft']){ await key(page,k); console.log(`   in-screen ${k}: race_track=${await rt(page)} lbl="${await lbl2(page)}" errs=${errs.length}`); }
  await page.screenshot({path:`${OUT}/cyc_track_inscreen.png`});
  // ---- CARS: main-menu F2 on Select Car ----
  // reboot fresh for clean nav
  await boot(page,server);
  await gotoButton(page,'Select Car');
  console.log('[C] menu Select Car highlighted, race_car='+await rc(page)+' lbl='+await lbl2(page));
  for(let i=1;i<=5;i++){ await key(page,'F2'); console.log(`   menu F2#${i}: race_car=${await rc(page)} lbl="${await lbl2(page)}" errs=${errs.length}`); }
  await page.screenshot({path:`${OUT}/cyc_car_menuF2.png`});
  console.log('ERRS: '+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);process.exit(1);});
