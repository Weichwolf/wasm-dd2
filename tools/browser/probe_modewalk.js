// QA: step through Race Mode dialog for a chosen mode index, screenshot every screen after each
// Enter, enumerate whatever dialog follows. arg2 = mode index (0=Wrecking,1=StockCar,2=DD)
const {serve,key,alive,rd,menuLabel,boot,gotoButton}=require('./felib.js');
const {chromium}=require('playwright'); const fs=require('fs');
const OUT='/tmp/qa_explore'; fs.mkdirSync(OUT,{recursive:true});
const MODE=parseInt(process.argv[2]||'0');
const st=(page)=>page.evaluate(()=>({mode:HEAP32[0x4673f8>>2],type:HEAP32[0x4673f4>>2],track:HEAP32[0x4673fc>>2],car:HEAP32[0x467400>>2],fe:HEAP8[0x460005],cf:HEAP32[0x462ff0>>2],ncars:HEAP32[0x46765c>>2]})).catch(()=>'DEAD');
let errs=[];
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,160)));
  page.on('crash',()=>errs.push('PAGE CRASHED'));
  await boot(page,server);
  await key(page,'Enter'); await page.waitForTimeout(800);   // open Race Mode dialog
  console.log('Race Mode dialog: '+JSON.stringify(await st(page)));
  for(let i=0;i<MODE;i++) await key(page,'ArrowRight');       // select mode icon
  await page.screenshot({path:`${OUT}/mw${MODE}_0mode.png`});
  await key(page,'Enter'); await page.waitForTimeout(1200);   // confirm mode
  console.log('after mode-Enter: '+JSON.stringify(await st(page))+' errs='+errs.length+' '+JSON.stringify([...new Set(errs)]));
  await page.screenshot({path:`${OUT}/mw${MODE}_1next.png`});
  // enumerate the next dialog: try arrows + F2 to see values change
  for(const k of ['ArrowRight','ArrowRight','ArrowLeft','ArrowDown','ArrowUp','F2','F1']){
    await key(page,k);
    console.log(`  ${k}: `+JSON.stringify(await st(page))+' errs='+errs.length);
    if(!(await alive(page))||errs.length){console.log('  BROKE: '+JSON.stringify([...new Set(errs)]));break;}
  }
  await page.screenshot({path:`${OUT}/mw${MODE}_2enum.png`});
  await key(page,'Enter'); await page.waitForTimeout(1500);   // confirm -> whatever comes next
  console.log('after next-Enter: '+JSON.stringify(await st(page))+' errs='+errs.length);
  await page.screenshot({path:`${OUT}/mw${MODE}_3after.png`});
  await key(page,'Enter'); await page.waitForTimeout(1500);
  console.log('after Enter#2: '+JSON.stringify(await st(page))+' errs='+errs.length);
  await page.screenshot({path:`${OUT}/mw${MODE}_4after2.png`});
  console.log('ALL ERRS: '+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);process.exit(1);});
