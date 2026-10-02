// QA: enumerate the Wrecking / race-options dialog. Open it, cycle mode+type with F2/F1 and
// arrows, read race_mode(0x4673f8) race_type(0x4673f4), screenshot every state.
const {serve,key,alive,rd,menuLabel,boot,gotoButton}=require('./felib.js');
const {chromium}=require('./playwright'); const fs=require('fs');
const OUT='/tmp/qa_explore'; fs.mkdirSync(OUT,{recursive:true});
const st=(page)=>page.evaluate(()=>[HEAP32[0x4673f8>>2],HEAP32[0x4673f4>>2],HEAP8[0x460005],HEAP32[0x462ff0>>2]]).catch(()=>'DEAD');
let errs=[];
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,160)));
  page.on('crash',()=>errs.push('PAGE CRASHED'));
  await boot(page,server);
  console.log('booted, menuLabel='+await menuLabel(page)+' state[mode,type,fe,cf]='+await st(page));
  const ok=await gotoButton(page,'Wrecking');
  console.log('gotoButton Wrecking='+ok+' label='+await menuLabel(page));
  await key(page,'Enter'); await page.waitForTimeout(1000);
  await page.screenshot({path:`${OUT}/ro_00_open.png`});
  console.log('opened dialog: alive='+await alive(page)+' state='+await st(page)+' errs='+errs.length);
  // Try F2 cycling (mode). Enumerate up to 10 presses.
  for(let i=1;i<=8;i++){
    await key(page,'F2');
    console.log(`  F2#${i}: alive=${await alive(page)} state=${await st(page)} errs=${errs.length} ${errs.length?JSON.stringify([...new Set(errs)]):''}`);
    await page.screenshot({path:`${OUT}/ro_f2_${i}.png`});
    if(!(await alive(page))||errs.length)break;
  }
  // Try F1 cycling back
  for(let i=1;i<=4;i++){
    await key(page,'F1');
    console.log(`  F1#${i}: alive=${await alive(page)} state=${await st(page)} errs=${errs.length}`);
    if(!(await alive(page))||errs.length)break;
  }
  // Try arrows (mode/type nav in a nested dialog)
  for(const k of ['ArrowDown','ArrowDown','ArrowUp','ArrowRight','ArrowLeft']){
    await key(page,k);
    console.log(`  ${k}: alive=${await alive(page)} state=${await st(page)} errs=${errs.length}`);
    await page.screenshot({path:`${OUT}/ro_${k}.png`});
    if(!(await alive(page))||errs.length)break;
  }
  console.log('ALL ERRS: '+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);process.exit(1);});
