// Verify Race Type 4th icon (2-Player) Enter confirms without trap.
const {serve,key,alive,boot,gotoButton,chromium}=require('./felib.js');
let errs=[];
const G=(page)=>page.evaluate(()=>typeof HEAPU8!=='undefined'?({scr:HEAPU8[0x460005]}):'DEAD').catch(()=>'DEAD');
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,160)));
  page.on('crash',()=>errs.push('PAGE CRASHED'));
  await boot(page,server);
  await gotoButton(page,'Wrecking');
  await key(page,'Enter',700);   // Race Mode
  await key(page,'Enter',700);   // -> Race Type (idx0 Championship)
  await key(page,'ArrowRight',500);await key(page,'ArrowRight',500);await key(page,'ArrowRight',500); // idx3 = 2-Player
  await page.screenshot({path:'/tmp/qa_explore2/twoP_idx3.png'});
  console.log('on 2-Player icon: alive='+await alive(page)+' '+JSON.stringify(await G(page))+' errs='+errs.length);
  await key(page,'Enter',900);   // confirm 2-Player
  console.log('after confirm 2-Player: alive='+await alive(page)+' '+JSON.stringify(await G(page))+' errs='+JSON.stringify([...new Set(errs)]));
  await page.screenshot({path:'/tmp/qa_explore2/twoP_confirm.png'});
  await page.waitForTimeout(2000);
  console.log('after +2s: alive='+await alive(page)+' '+JSON.stringify(await G(page))+' errs='+JSON.stringify([...new Set(errs)]));
  await page.screenshot({path:'/tmp/qa_explore2/twoP_after.png'});
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);try{server.close();}catch(_){}process.exit(1);});
