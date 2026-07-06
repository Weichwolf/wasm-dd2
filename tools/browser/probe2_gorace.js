// QA: launch a REAL race via FE Go!, then retire/quit and watch for a results/league menu.
// arg2 = end action: retire | quit | complete
const {serve,key,alive,rd,boot,gotoButton,menuLabel,chromium}=require('./felib.js');
const fs=require('fs'); const OUT='/tmp/qa_explore2'; fs.mkdirSync(OUT,{recursive:true});
const SC=process.argv[2]||'retire';
const info=(page)=>page.evaluate(()=>({cf:HEAP32[0x462ff0>>2],fe005:HEAPU8[0x460005],ml:(function(){const q=HEAPU32[0x46975c>>2];if(!(q>0x400000&&q<0x980000))return'';let s='';for(let i=0;i<40;i++){const c=HEAPU8[q+i];if(!c)break;s+=String.fromCharCode(c);}return s;})()})).catch(()=>'DEAD');
let errs=[];
const snap=async(page,t)=>{await page.screenshot({path:`${OUT}/gorace_${SC}_${t}.png`});};
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,160)));
  page.on('crash',()=>errs.push('PAGE CRASHED'));
  await boot(page,server);
  await gotoButton(page,'Go!'); await key(page,'Enter',700); await page.waitForTimeout(15000);
  console.log(`[${SC}] race launched ${JSON.stringify(await info(page))} errs=${errs.length}`);
  await snap(page,'00_racing');
  if(SC==='retire'||SC==='quit'){
    await key(page,'Escape');await page.waitForTimeout(700);
    const idx = SC==='retire'?3:4;
    for(let i=0;i<idx;i++)await key(page,'ArrowDown');
    await key(page,'Enter');await snap(page,'01_sure');
    await key(page,'ArrowDown');await key(page,'Enter');await page.waitForTimeout(2500);
    console.log(`  confirmed ${SC} ${JSON.stringify(await info(page))}`);
    await snap(page,'02_after');
  }
  // watch for results/league menu
  let prev='';
  for(let t=0;t<24;t++){
    await page.waitForTimeout(2000);
    const nfo=await info(page);const a=await alive(page);
    const sig=nfo.fe005+'|'+nfo.ml.slice(0,20);
    if(sig!==prev){console.log(`  t=${t*2}s ${JSON.stringify(nfo)} alive=${a} errs=${errs.length}`);await snap(page,`w_t${t*2}`);prev=sig;}
    if(!a){console.log('  DEAD');break;}
    if(nfo.fe005===201){console.log('  -> back at MAIN MENU');break;}
  }
  // try to interact with any menu present
  console.log('--- probe menu keys ---');
  for(const k of ['ArrowDown','ArrowUp','Enter','ArrowDown','Enter','Escape']){await key(page,k,600);const n=await info(page);console.log(`  ${k} ${JSON.stringify(n)} errs=${errs.length}`);await snap(page,`k_${k}`);}
  console.log('FINAL alive='+await alive(page)+' errs='+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);try{server.close();}catch(_){}process.exit(1);});
