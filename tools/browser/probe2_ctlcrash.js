// Reproduce + diagnose the Control Method confirm crash. Capture console, pageerror, crash.
const {serve,key,alive,boot,gotoButton,chromium}=require('./felib.js');
const cf=(page)=>page.evaluate(()=>HEAP32[0x462ff0>>2]).catch(()=>'DEAD');
let log=[];
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>log.push('PAGEERR: '+e.message.replace(/\n/g,' ').slice(0,300)));
  page.on('crash',()=>log.push('*** PAGE CRASHED ***'));
  page.on('console',m=>{const t=m.text();if(/abort|trap|error|out of bounds|unreachable|RuntimeError|signature/i.test(t))log.push('CONSOLE: '+t.slice(0,300));});
  await boot(page,server);
  const NAV=parseInt(process.argv[2]||'0');   // # of ArrowRight before confirming
  console.log(`nav=${NAV} steps before Enter-confirm`);
  await gotoButton(page,'Configuration');
  await key(page,'Enter',700);   // opens Configuration/Control Method dialog
  await page.screenshot({path:'/tmp/qa_explore2/ctlcrash_open.png'});
  console.log('opened, alive='+await alive(page));
  // maybe need one more Enter to get INTO Control Method sub-dialog
  await key(page,'Enter',700);
  const label=await page.evaluate(()=>{const q=HEAPU32[0x46975c>>2];if(!(q>0x400000&&q<0x980000))return'?';let s='';for(let i=0;i<40;i++){const c=HEAPU8[q+i];if(!c)break;s+=String.fromCharCode(c);}return s;}).catch(()=>'DEAD');
  console.log('after 2nd Enter alive='+await alive(page)+' label='+JSON.stringify(label));
  await page.screenshot({path:'/tmp/qa_explore2/ctlcrash_sub.png'});
  for(let i=0;i<NAV;i++) await key(page,'ArrowRight',450);
  console.log('about to confirm-Enter, alive='+await alive(page));
  await key(page,'Enter',900);
  console.log('after confirm-Enter alive='+await alive(page)+' cf='+await cf(page));
  await page.waitForTimeout(1500);
  console.log('after +1.5s alive='+await alive(page));
  console.log('LOG:\n'+log.join('\n'));
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);console.log('LOG:\n'+log.join('\n'));try{server.close();}catch(_){}process.exit(1);});
