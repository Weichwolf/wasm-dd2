// QA: rapid ENTER+BACK-OUT of every FE sub-screen REPEATEDLY, plus LEFT-nav at every menu edge
// (Info screens had a LEFT-nav OOB class). Fresh boot per screen to avoid state carry.
const {serve,key,alive,rd,menuLabel,boot,gotoButton,chromium}=require('./felib.js');
let errs=[],crashed=false;
const scr=(page)=>page.evaluate(()=>HEAPU8[0x460005]).catch(()=>-1);
const chk=async(page,tag)=>{const a=await alive(page);const e=[...new Set(errs)];console.log(`  ${tag}: scr=${await scr(page)} alive=${a} crashed=${crashed} ${e.length?'ERRS='+JSON.stringify(e):''}`);return a&&!crashed&&e.length===0;};
// each sub-screen: [button, entryKeys(after Enter to open), then we hammer arrows]
const SUBS=[
  ['Select Car',[]],['Select Track',[]],['File Manager',[]],['CD Audio Player',[]],
  ['Information',[]],['Configuration',[]],['Wrecking',[]],
];
(async()=>{
  const server=serve(process.argv[2]||'../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push('PAGEERR:'+e.message.replace(/\n/g,' ').slice(0,140)));
  page.on('crash',()=>{crashed=true;errs.push('*** CRASH ***');});
  await boot(page,server);

  // 1) enter/exit each screen 3x in a row without reboot (state carry stress)
  for(const [btn] of SUBS){
    errs.length=0;
    await boot(page,server);
    if(!await gotoButton(page,btn)){console.log(`SKIP ${btn}: reach FAIL`);continue;}
    for(let rep=0;rep<3;rep++){
      await key(page,'Enter',700);
      // hammer arrows inside (incl LEFT at edges) + F2
      for(const k of ['ArrowLeft','ArrowLeft','ArrowLeft','ArrowUp','ArrowRight','ArrowDown','F2']) await key(page,k,260);
      await key(page,'Escape',600);
      if(crashed||!await alive(page))break;
    }
    await chk(page,`${btn} x3`);
    if(crashed)break;
  }

  // 2) Information deep: open, then LEFT-nav hard at the sub-screen edges (OOB class)
  if(!crashed){
    errs.length=0; await boot(page,server);
    if(await gotoButton(page,'Information')){
      await key(page,'Enter',800);
      for(let i=0;i<10;i++) await key(page,'ArrowLeft',220);   // spam LEFT at edge
      await chk(page,'Info LEFT-spam');
      await key(page,'Enter',800);                             // into a stat page
      for(let i=0;i<10;i++) await key(page,'ArrowLeft',220);
      await chk(page,'Info page LEFT-spam');
      await key(page,'Escape',500); await key(page,'Escape',500);
    }
  }

  // 3) Configuration deep: cycle right across all config items + into each, LEFT-spam
  if(!crashed){
    errs.length=0; await boot(page,server);
    if(await gotoButton(page,'Configuration')){
      await key(page,'Enter',800);
      for(let i=0;i<6;i++){ const l=await rd(page,0x469158); console.log(`   config item ${i}: "${l}"`); await key(page,'ArrowRight',500); }
      for(let i=0;i<8;i++) await key(page,'ArrowLeft',300);   // back to start + spam left edge
      await chk(page,'Config nav sweep');
      await key(page,'Escape',600);
    }
  }

  console.log(`\nRESULT subscreens: crashed=${crashed} alive=${await alive(page)} errs=${JSON.stringify([...new Set(errs)])}`);
  await b.close(); server.close(); process.exit(crashed?2:0);
})().catch(e=>{console.error('ERR '+e.message+'\n'+e.stack);process.exit(1);});
