// QA priority 1: VERIFY-FIXED. Navigate ALL icons in Race MODE, Race TYPE, and Control Method
// dialogs with ArrowRight then ArrowLeft back. Confirm no trap/crash, all render.
// Also confirm Time Trials (3rd) and 2-Player (4th) can be Entered without trap.
const {serve,key,alive,rd,boot,gotoButton,chromium}=require('./felib.js');
const fs=require('fs'); const OUT='/tmp/qa_explore2'; fs.mkdirSync(OUT,{recursive:true});
const buildDir=process.argv[2]||'../../web/dd2';
let errs=[];
const snap=async(page,tag)=>{await page.screenshot({path:`${OUT}/${tag}.png`});};
const st=async(page,k,l,post=700)=>{const before=errs.length;await key(page,k,post);const a=await alive(page);
  const now=[...new Set(errs.slice(before))];
  console.log(`   ${l} (${k}): alive=${a} newerr=${errs.length-before} ${now.length?JSON.stringify(now):''}`);
  await snap(page,l); return a;};
(async()=>{
  const server=serve(buildDir); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,160)));
  page.on('crash',()=>errs.push('PAGE CRASHED'));
  await boot(page,server);

  console.log('== RACE MODE dialog (Wrecking -> Enter) ==');
  await gotoButton(page,'Wrecking');
  await st(page,'Enter','A0_mode_open');
  for(let i=1;i<=4;i++) await st(page,'ArrowRight',`A${i}_mode_R${i}`);
  for(let i=1;i<=4;i++) await st(page,'ArrowLeft',`A${i+4}_mode_L${i}`);

  console.log('== RACE TYPE dialog (Enter to confirm mode) ==');
  await st(page,'Enter','B0_type_open');
  for(let i=1;i<=4;i++) await st(page,'ArrowRight',`B${i}_type_R${i}`);
  for(let i=1;i<=4;i++) await st(page,'ArrowLeft',`B${i+4}_type_L${i}`);
  // Go to 3rd icon (Time Trials) = ArrowRight x2 from idx0, confirm
  await st(page,'ArrowRight','C1_toIdx1');
  await st(page,'ArrowRight','C2_toIdx2_TimeTrial');
  await st(page,'Enter','C3_confirm_TimeTrial');
  await st(page,'Escape','C4_back');
  await snap(page,'C5_afterback');

  await b.close(); server.close();
  console.log('DONE icons. total distinct errs='+[...new Set(errs)].length);
})().catch(e=>{console.error('FATAL '+e.message);try{server.close();}catch(_){}process.exit(1);});
