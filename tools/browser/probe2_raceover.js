// From the Race Over results menu (after retire), navigate icons + activate each option.
const {serve,key,alive,boot,gotoButton,chromium}=require('./felib.js');
const fs=require('fs'); const OUT='/tmp/qa_explore2';
const G=(page)=>page.evaluate(()=>({cf:HEAP32[0x462ff0>>2],scr:HEAPU8[0x460005]})).catch(()=>'DEAD');
let errs=[];
const snap=(page,t)=>page.screenshot({path:`${OUT}/raceover_${t}.png`});
const st=async(page,k,l,post=650)=>{const b=errs.length;await key(page,k,post);const g=await G(page);console.log(`  ${l} (${k}): ${JSON.stringify(g)} alive=${await alive(page)} newerr=${errs.length-b} ${errs.length>b?JSON.stringify([...new Set(errs.slice(b))]):''}`);await snap(page,l);};
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,160)));
  page.on('crash',()=>errs.push('PAGE CRASHED'));
  await boot(page,server);
  await gotoButton(page,'Go!'); await key(page,'Enter',700); await page.waitForTimeout(15000);
  // retire (UP=Yes)
  await key(page,'Escape',500);
  await key(page,'ArrowDown',450);await key(page,'ArrowDown',450);await key(page,'ArrowDown',450);
  await key(page,'Enter',500); await key(page,'ArrowUp',500); await key(page,'Enter',900);
  console.log('Race Over reached '+JSON.stringify(await G(page)));
  await snap(page,'00_raceover');
  // navigate icons with arrows (all directions)
  await st(page,'ArrowRight','01_right');
  await st(page,'ArrowRight','02_right2');
  await st(page,'ArrowLeft','03_left');
  await st(page,'ArrowDown','04_down');
  await st(page,'ArrowUp','05_up');
  // Activate VIEW REPLAY: go to leftmost icon then Enter, watch replay, then exit
  await st(page,'ArrowLeft','06_toLeftmost');
  await st(page,'Enter','07_viewreplay',1200);
  await page.waitForTimeout(6000); await snap(page,'08_replaying');
  console.log('  during replay '+JSON.stringify(await G(page)));
  // exit replay: press fire/back
  await st(page,'Enter','09_replay_enter',1000);
  await st(page,'Escape','10_replay_esc',1000);
  await st(page,'KeyA','11_replay_A',1000);
  console.log('  back? '+JSON.stringify(await G(page)));
  console.log('FINAL '+JSON.stringify(await G(page))+' errs='+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);try{server.close();}catch(_){}process.exit(1);});
