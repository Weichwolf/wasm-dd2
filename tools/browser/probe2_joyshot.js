// Select Joystick in Control Method (clean path) and screenshot the destination; then confirm
// Keyboard-select crashes one more time for determinism.
const {serve,key,alive,boot,gotoButton,chromium}=require('./felib.js');
let log=[];
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>log.push('PAGEERR '+e.message.slice(0,200)));
  page.on('crash',()=>log.push('CRASH'));
  await boot(page,server);
  await gotoButton(page,'Configuration'); await key(page,'Enter',700); await key(page,'Enter',700);
  await key(page,'ArrowRight',500);   // to Joystick (idx1)
  await key(page,'Enter',900);
  console.log('Joystick selected: alive='+await alive(page)+' log='+JSON.stringify(log));
  await page.screenshot({path:'/tmp/qa_explore2/joyselect_dest.png'});
  await key(page,'Escape',600); await key(page,'Escape',600);
  console.log('recovered to menu: alive='+await alive(page));
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);try{server.close();}catch(_){}process.exit(1);});
