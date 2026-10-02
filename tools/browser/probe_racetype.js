// QA: isolate the Race Type dialog trap. For a given mode, open Race Type, step ArrowRight one
// at a time from fresh boot, log errs + label after EACH key. Find the exact trapping index.
const {serve,key,alive,rd,menuLabel,boot}=require('./felib.js');
const {chromium}=require('./playwright'); const fs=require('fs');
const OUT='/tmp/qa_explore'; fs.mkdirSync(OUT,{recursive:true});
const MODE=parseInt(process.argv[2]||'0');       // 0=Wrecking 1=StockCar 2=DD
const NR=parseInt(process.argv[3]||'5');          // number of ArrowRight steps
// try several label VAs to find the type name
const lab=(page)=>page.evaluate(()=>{
  const cands=[0x46975c,0x469784,0x4697ac];let out=[];
  for(const a of cands){const q=HEAPU32[a>>2];if(q>0x400000&&q<0x980000){let s='';for(let i=0;i<40;i++){const c=HEAPU8[q+i];if(!c)break;s+=String.fromCharCode(c);}out.push(s);}else out.push('?');}
  return out;
}).catch(()=>['DEAD']);
let errs=[];
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,160)));
  page.on('crash',()=>errs.push('PAGE CRASHED'));
  await boot(page,server);
  await key(page,'Enter'); await page.waitForTimeout(600);   // Race Mode
  for(let i=0;i<MODE;i++) await key(page,'ArrowRight');
  await key(page,'Enter'); await page.waitForTimeout(1000);   // -> Race Type
  console.log(`mode=${MODE} Race Type open, errs=${errs.length} labels=${JSON.stringify(await lab(page))}`);
  await page.screenshot({path:`${OUT}/rt${MODE}_idx0.png`});
  for(let i=1;i<=NR;i++){
    const before=errs.length;
    await key(page,'ArrowRight');
    const now=[...new Set(errs)];
    console.log(`  R#${i} -> idx${i}: alive=${await alive(page)} newerr=${errs.length>before} labels=${JSON.stringify(await lab(page))} ${errs.length>before?JSON.stringify(now):''}`);
    await page.screenshot({path:`${OUT}/rt${MODE}_idx${i}.png`});
    if(!(await alive(page))){console.log('  DEAD after idx'+i);break;}
  }
  console.log('FINAL ERRS: '+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);process.exit(1);});
