// CORRECTED: retire/quit confirmation = ArrowUp selects Yes. Test real results flow.
// arg2 = retire | quit
const {serve,key,alive,boot,gotoButton,chromium}=require('./felib.js');
const fs=require('fs'); const OUT='/tmp/qa_explore2'; const SC=process.argv[2]||'retire';
const G=(page)=>page.evaluate(()=>({cf:HEAP32[0x462ff0>>2],scr:HEAPU8[0x460005],ml:(function(){const q=HEAPU32[0x46975c>>2];if(!(q>0x400000&&q<0x980000))return'';let s='';for(let i=0;i<40;i++){const c=HEAPU8[q+i];if(!c)break;s+=String.fromCharCode(c);}return s;})()})).catch(()=>'DEAD');
let errs=[];
const snap=(page,t)=>page.screenshot({path:`${OUT}/res2_${SC}_${t}.png`});
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,160)));
  page.on('crash',()=>errs.push('PAGE CRASHED'));
  await boot(page,server);
  await gotoButton(page,'Go!'); await key(page,'Enter',700); await page.waitForTimeout(15000);
  console.log(`[${SC}] launched `+JSON.stringify(await G(page)));
  await key(page,'Escape',500);
  const idx = SC==='retire'?3:4;
  for(let i=0;i<idx;i++) await key(page,'ArrowDown',450);
  await snap(page,'01_item'); await key(page,'Enter',500); await snap(page,'02_sure');
  await key(page,'ArrowUp',500);   // UP = Yes
  await snap(page,'03_yes'); await key(page,'Enter',700);
  console.log(`  confirmed(UP=Yes) `+JSON.stringify(await G(page)));
  // watch for results menu (scr change from race)
  let prev=-1;
  for(let t=0;t<26;t++){
    await page.waitForTimeout(1500); const g=await G(page); const a=await alive(page);
    if(g.scr!==prev){console.log(`  t=${(t*1.5).toFixed(1)}s ${JSON.stringify(g)} alive=${a} errs=${errs.length}`);await snap(page,`w_${t}_scr${g.scr}`);prev=g.scr;}
    if(!a){console.log('DEAD');break;}
    if(g.scr===201){console.log('  -> MAIN MENU'); break;}
  }
  await snap(page,'zz_final');
  // exercise whatever menu is present
  console.log('--- exercise menu ---');
  for(const k of ['ArrowDown','ArrowRight','ArrowLeft','ArrowUp','Enter']){await key(page,k,600);const g=await G(page);console.log(`  ${k} ${JSON.stringify(g)} errs=${errs.length}`);await snap(page,`m_${k}`);}
  console.log('FINAL '+JSON.stringify(await G(page))+' errs='+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);try{server.close();}catch(_){}process.exit(1);});
