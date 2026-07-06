// Pin the Control Method crash: log scr byte after each Enter; test selecting Keyboard(idx0)
// vs Joystick(idx1). arg2 = right-steps before final select.
const {serve,key,alive,boot,gotoButton,chromium}=require('./felib.js');
const G=(page)=>page.evaluate(()=>typeof HEAPU8!=='undefined'?({scr:HEAPU8[0x460005],cf:HEAP32[0x462ff0>>2]}):'DEAD').catch(()=>'DEAD');
let log=[];
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>log.push('PAGEERR: '+e.message.replace(/\n/g,' ').slice(0,300)));
  page.on('crash',()=>log.push('*** PAGE CRASHED ***'));
  page.on('console',m=>log.push('CONSOLE: '+m.text().slice(0,200)));
  await boot(page,server);
  const RS=parseInt(process.argv[2]||'0');
  await gotoButton(page,'Configuration');
  console.log('at Configuration button, scr='+JSON.stringify(await G(page)));
  await key(page,'Enter',700); console.log('Enter#1 (open Config dialog): '+JSON.stringify(await G(page)));
  await page.screenshot({path:'/tmp/qa_explore2/cc2_1.png'});
  await key(page,'Enter',700); console.log('Enter#2 (open Control Method): '+JSON.stringify(await G(page)));
  await page.screenshot({path:'/tmp/qa_explore2/cc2_2.png'});
  for(let i=0;i<RS;i++){await key(page,'ArrowRight',450);console.log(`  R#${i+1}: `+JSON.stringify(await G(page)));}
  console.log(`selecting (RS=${RS}) with Enter...`);
  await key(page,'Enter',900);
  console.log('after select-Enter: '+JSON.stringify(await G(page)));
  await page.waitForTimeout(1500);
  console.log('final: '+JSON.stringify(await G(page)));
  console.log('LOG:\n'+log.join('\n'));
  await browser.close(); server.close();
})().catch(e=>{console.error('CAUGHT '+e.message);console.log('LOG:\n'+log.join('\n'));try{server.close();}catch(_){}process.exit(1);});
