// QA Area 5: pause/resume odd moments, rapid toggle, F1/F2 camera mid-race, ESC-quit-to-FE.
const {serve,key,alive,rd,boot,gotoButton,chromium}=require('./felib.js');
const fs=require('fs'); const OUT='/tmp/qa_pause'; fs.mkdirSync(OUT,{recursive:true});
const buildDir=process.argv[2]||'../../web/dd2';
let errs=[],crashed=false;
const st=(page)=>page.evaluate(()=>({scr:HEAPU8[0x460005],cf:HEAP32[0x462ff0>>2]})).catch(()=>'DEAD');
const hold=async(page,c,ms)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(ms);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);};
async function launchRace(page){
  await boot(page,server); await gotoButton(page,'Go!');
  await key(page,'Enter',1500); await page.waitForTimeout(6000);
  return await st(page);
}
let server;
(async()=>{
  server=serve(buildDir); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push('PAGEERR:'+e.message.replace(/\n/g,' ').slice(0,140)));
  page.on('crash',()=>{crashed=true;errs.push('*** CRASH ***');});

  console.log('=== TEST A: pause immediately at race start ===');
  let s=await launchRace(page); console.log('  race launched: '+JSON.stringify(s));
  if(s.scr===201){console.log('  FAIL race not launched');}
  await key(page,'Escape',1500);
  const p1=await st(page); await page.waitForTimeout(2500); const p2=await st(page);
  console.log('  paused early: cf '+p1.cf+' -> '+p2.cf+' frozen='+(p1.cf===p2.cf)+' scr='+p2.scr);
  await page.screenshot({path:`${OUT}/A_paused.png`});
  await key(page,'Enter',1500);  // resume
  const r1=await st(page); await page.waitForTimeout(2000); const r2=await st(page);
  console.log('  resumed: cf '+r1.cf+' -> '+r2.cf+' advancing='+(r2.cf>r1.cf)+' scr='+r2.scr);

  console.log('=== TEST B: rapid pause/resume toggle x6 ===');
  for(let i=0;i<6;i++){ await key(page,'Escape',600); await key(page,'Enter',600); if(crashed)break; }
  const bst=await st(page); console.log('  after 6 toggles: '+JSON.stringify(bst)+' alive='+await alive(page)+' crashed='+crashed);
  await page.waitForTimeout(1500); const bst2=await st(page);
  console.log('  race still live after toggles: cf advancing='+(bst2.cf>bst.cf)+' '+JSON.stringify(bst2));

  console.log('=== TEST C: F1/F2 camera cycling mid-race ===');
  await hold(page,'KeyA',1500);  // accelerate to move
  const c0=await st(page);
  for(let i=0;i<4;i++){ await key(page,'F2',500); await key(page,'F1',500); if(crashed)break; }
  const c1=await st(page);
  console.log('  after F1/F2 x8: cf '+c0.cf+' -> '+c1.cf+' advancing='+(c1.cf>c0.cf)+' scr='+c1.scr+' crashed='+crashed+' alive='+await alive(page));
  await page.screenshot({path:`${OUT}/C_camera.png`});

  console.log('=== TEST D: ESC pause -> navigate to QUIT -> return to FE ===');
  s=await launchRace(page); console.log('  fresh race: '+JSON.stringify(s));
  await hold(page,'KeyA',1000);
  await key(page,'Escape',1500);  // pause menu
  const pm=await st(page); console.log('  pause menu scr='+pm.scr);
  await page.screenshot({path:`${OUT}/D_pausemenu.png`});
  // try each down option to find Quit; then Enter; check if we leave to FE (scr 201)
  for(let i=0;i<4;i++){ await key(page,'ArrowDown',500); }
  await key(page,'Enter',2000); await page.waitForTimeout(2500);
  let d1=await st(page); console.log('  after Down x4 + Enter: '+JSON.stringify(d1));
  // some quit paths ask confirm
  await key(page,'Enter',1500); await page.waitForTimeout(2000);
  const d2=await st(page); console.log('  after confirm Enter: '+JSON.stringify(d2)+' backAtFE(201)='+(d2.scr===201));
  await page.screenshot({path:`${OUT}/D_afterquit.png`});

  console.log('\nRESULT area5: crashed='+crashed+' alive='+await alive(page)+' errs='+JSON.stringify([...new Set(errs)]));
  await b.close(); server.close(); process.exit(crashed?2:0);
})().catch(e=>{console.error('ERR '+e.message+'\n'+e.stack);try{server.close();}catch(_){}process.exit(1);});
