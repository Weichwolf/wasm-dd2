// Priority 1 remainder: Control Method dialog (Configuration->Enter->1st icon) icon nav R/L;
// plus Race Type 4th icon (2-Player) Enter confirm without trap.
const {serve,key,alive,rd,boot,gotoButton,chromium}=require('./felib.js');
const fs=require('fs'); const OUT='/tmp/qa_explore2';
let errs=[];
const snap=(page,t)=>page.screenshot({path:`${OUT}/ctl_${t}.png`});
const st=async(page,k,l,post=650)=>{const b=errs.length;await key(page,k,post);console.log(`  ${l} (${k}): alive=${await alive(page)} newerr=${errs.length-b} ${errs.length>b?JSON.stringify([...new Set(errs.slice(b))]):''}`);await snap(page,l);};
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,160)));
  page.on('crash',()=>errs.push('PAGE CRASHED'));
  await boot(page,server);

  console.log('== CONFIGURATION -> Control Method icon nav ==');
  await gotoButton(page,'Configuration');
  await st(page,'Enter','A0_config_open');       // Configuration dialog (icons)
  // 1st icon should be Control Method; open it
  await st(page,'Enter','A1_ctlmethod_open');
  for(let i=1;i<=5;i++) await st(page,'ArrowRight',`A_R${i}`);
  for(let i=1;i<=5;i++) await st(page,'ArrowLeft',`A_L${i}`);
  await st(page,'Enter','A2_confirm');
  await st(page,'Escape','A3_back');
  await st(page,'Escape','A4_back2');

  console.log('== RACE TYPE 4th icon (2-Player) Enter ==');
  // fresh: back to main menu, Wrecking -> Enter (mode) -> Enter (type dialog)
  await st(page,'Escape','B_reset');
  await gotoButton(page,'Wrecking');
  await st(page,'Enter','B0_mode');
  await st(page,'Enter','B1_type');
  await st(page,'ArrowRight','B2_idx1');
  await st(page,'ArrowRight','B3_idx2');
  await st(page,'ArrowRight','B4_idx3_2player');
  await st(page,'Enter','B5_confirm_2player');
  await st(page,'Escape','B6_back');
  console.log('FINAL alive='+await alive(page)+' errs='+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);try{server.close();}catch(_){}process.exit(1);});
