// QA: Select Car — cycle all cars with F2 (both dirs), screenshot, confirm no crash/trap.
const {serve,key,alive,rd,menuLabel,boot,gotoButton}=require('./felib.js');
const {chromium}=require('playwright'); const fs=require('fs');
const OUT='/tmp/qa_explore'; fs.mkdirSync(OUT,{recursive:true});
const carlabel=(page)=>rd(page,0x469784);
const rc=(page)=>page.evaluate(()=>HEAP32[0x467400>>2]).catch(()=>'?');
let errs=[];
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,140)));
  page.on('crash',()=>errs.push('PAGE CRASHED'));
  await boot(page,server);
  await gotoButton(page,'Select Car'); await key(page,'Enter'); await page.waitForTimeout(1000);
  console.log('Select Car open: label='+await carlabel(page)+' race_car='+await rc(page)+' errs='+errs.length);
  await page.screenshot({path:`${OUT}/car_0.png`});
  for(let i=1;i<=10;i++){
    await key(page,'F2');
    console.log(`  F2#${i}: race_car=${await rc(page)} label="${await carlabel(page)}" alive=${await alive(page)} errs=${errs.length}${errs.length?' '+JSON.stringify([...new Set(errs)]):''}`);
    if(i<=6) await page.screenshot({path:`${OUT}/car_f2_${i}.png`});
    if(!(await alive(page))||errs.length)break;
  }
  // reverse with F1
  for(let i=1;i<=4;i++){
    await key(page,'F1');
    console.log(`  F1#${i}: race_car=${await rc(page)} alive=${await alive(page)} errs=${errs.length}`);
    if(!(await alive(page))||errs.length)break;
  }
  console.log('FINAL ERRS: '+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);process.exit(1);});
