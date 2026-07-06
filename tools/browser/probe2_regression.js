// Priority 3: regression sweep. Open each main button + a sub-screen, back out, confirm clean.
// Then launch a race, drive (KeyA), pause(Esc)+resume. Fresh boot per phase to avoid stale state.
const {serve,key,alive,rd,menuLabel,boot,gotoButton,PATHS,chromium}=require('./felib.js');
const fs=require('fs'); const OUT='/tmp/qa_explore2';
const cf=(page)=>page.evaluate(()=>HEAP32[0x462ff0>>2]).catch(()=>'DEAD');
let errs=[];
const chk=async(page,tag)=>{const a=await alive(page);const e=[...new Set(errs)];console.log(`  ${tag}: alive=${a} errs=${e.length} ${e.length?JSON.stringify(e):''}`);await page.screenshot({path:`${OUT}/reg_${tag}.png`});return a&&e.length===0;};
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,160)));
  page.on('crash',()=>errs.push('PAGE CRASHED'));
  await boot(page,server);
  console.log('== open each main button dialog + back ==');
  // buttons that open a dialog we navigate then Escape out of
  const btns=['Wrecking','Select Car','Select Track','File Manager','CD Audio Player','Information'];
  for(const bn of btns){
    errs=[];
    const ok=await gotoButton(page,bn);
    await key(page,'Enter',700);              // open
    await key(page,'ArrowRight',500);         // nav a bit
    await key(page,'ArrowLeft',500);
    await chk(page,'btn_'+bn.replace(/\W/g,''));
    await key(page,'Escape',500); await key(page,'Escape',500);  // back to main
    // re-verify main menu
  }
  // Configuration: open + Control Method (open+nav, DO NOT select Keyboard) + Sound Volume
  errs=[];
  await gotoButton(page,'Configuration'); await key(page,'Enter',700);
  await chk(page,'cfg_open');
  await key(page,'Enter',700);   // Control Method dialog
  await key(page,'ArrowRight',500); await key(page,'ArrowLeft',500);
  await chk(page,'cfg_ctlmethod_nav');
  await key(page,'Escape',600);  // back out WITHOUT selecting
  await key(page,'ArrowRight',500); await key(page,'Enter',700);  // 2nd icon (Sound Volume)
  await chk(page,'cfg_soundvol');
  await key(page,'ArrowLeft',500); await key(page,'ArrowRight',500);
  await key(page,'Escape',600); await key(page,'Escape',600); await key(page,'Escape',600);
  console.log('== main menu still navigable? ==');
  const ml=await menuLabel(page); console.log('  menuLabel="'+ml+'" alive='+await alive(page));
  await browser.close(); server.close();
  console.log('SWEEP errs total (distinct across phases logged above).');
})().catch(e=>{console.error('FATAL '+e.message);try{server.close();}catch(_){}process.exit(1);});
