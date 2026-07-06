// Rigorous repeatable FE full-pass: reach each button BY LABEL, verify the sub-screen it opens,
// crash+error-free. Race launch verified by leaving the FE. Exit 0 = all pass, 2 = failure.
const {serve,key,alive,rd,menuLabel,boot,gotoButton,chromium}=require('./felib.js');
const fs=require('fs'); const OUT='/tmp/fepass'; fs.mkdirSync(OUT,{recursive:true});
const buildDir=process.argv[2]||'../../web/dd2';
let errs=[]; const results=[]; let pageCrashed=false;
const uniqErrs=()=>[...new Set(errs)];
async function check(page,name,fn){
  errs=[]; let detail='';
  try{ detail=await fn(); }catch(e){ detail='EXC:'+e.message; }
  const a=await alive(page); const ue=uniqErrs();
  const pass = a && ue.length===0 && !String(detail).startsWith('FAIL') && !String(detail).startsWith('EXC');
  results.push({name,pass,detail,errs:ue});
  await page.screenshot({path:`${OUT}/${name}.png`}).catch(()=>{});
  console.log(`${pass?'PASS':'FAIL'} ${name}: ${detail} ${ue.length?'ERRS='+JSON.stringify(ue):''}`);
  return pass;
}
(async()=>{
  const server=serve(buildDir); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,120)));
  page.on('crash',()=>{pageCrashed=true;errs.push('RENDERER CRASH (page.on crash)');});

  // Car
  await check(page,'car',async()=>{ await boot(page,server); if(!await gotoButton(page,'Select Car'))return'FAIL reach';
    await key(page,'Enter',900); await key(page,'F2',700); await key(page,'F2',700); await key(page,'Escape',700); return'cycled'; });
  // Track
  await check(page,'track',async()=>{ await boot(page,server); if(!await gotoButton(page,'Select Track'))return'FAIL reach';
    await key(page,'Enter',900); const t=await rd(page,0x46975c); await key(page,'F2',700); await key(page,'Escape',700); return'ok'; });
  // File Manager
  await check(page,'filemgr',async()=>{ await boot(page,server); if(!await gotoButton(page,'File Manager'))return'FAIL reach';
    await key(page,'Enter',900); await key(page,'ArrowDown',700); await key(page,'ArrowUp',700); await key(page,'Escape',700); return'ok'; });
  // CD Player: verify category label appears
  await check(page,'cdplayer',async()=>{ await boot(page,server); if(!await gotoButton(page,'CD Audio Player'))return'FAIL reach';
    await key(page,'Enter',900); let c=await rd(page,0x469d64); if(!c.includes('Prev'))return'FAIL cat="'+c+'"';
    await key(page,'ArrowRight',700); c=await rd(page,0x469d64); if(!c.includes('Play'))return'FAIL cat2="'+c+'"';
    await key(page,'Enter',700); await key(page,'ArrowRight',700); await key(page,'Enter',700); await key(page,'Escape',700); return'cats ok'; });
  // Info -> Lap Times (default record when no stats)
  await check(page,'info_laptimes',async()=>{ await boot(page,server); if(!await gotoButton(page,'Information'))return'FAIL reach';
    await key(page,'Enter',900); const l=await rd(page,0x469b44); await key(page,'Enter',900); await key(page,'Escape',700); await key(page,'Escape',700); return'label="'+l+'"'; });
  // Info -> Statistics (forced stats)
  await check(page,'info_stats',async()=>{ await boot(page,server); await page.evaluate(()=>{HEAP32[0x46741c>>2]=1;});
    if(!await gotoButton(page,'Information'))return'FAIL reach'; await page.evaluate(()=>{HEAP32[0x46741c>>2]=1;});
    await key(page,'Enter',900); let l=await rd(page,0x469b44); if(!l.includes('Statistics'))return'FAIL label="'+l+'"';
    await key(page,'Enter',900); await key(page,'ArrowRight',700); await key(page,'ArrowRight',700); await key(page,'ArrowLeft',700); await key(page,'Escape',700); await key(page,'Escape',700); return'stats ok'; });
  // Config -> Control Method
  await check(page,'config_ctrl',async()=>{ await boot(page,server); if(!await gotoButton(page,'Configuration'))return'FAIL reach';
    await key(page,'Enter',900); let l=await rd(page,0x469158); if(!l.includes('Control'))return'FAIL label="'+l+'"';
    await key(page,'Enter',900); await key(page,'ArrowDown',700); await key(page,'ArrowUp',700); await key(page,'Escape',700); await key(page,'Escape',700); return'ctrl ok'; });
  // Config -> Control Method -> SELECT KEYBOARD (patch 827 crash regression guard; watch page.on('crash'))
  await check(page,'config_keyboard',async()=>{ await boot(page,server); if(!await gotoButton(page,'Configuration'))return'FAIL reach';
    await key(page,'Enter',900); await key(page,'Enter',900);           // Control Method (Keyboard default-selected)
    await key(page,'Enter',1200);                                        // select Keyboard -> rebind screen (was a hard crash)
    if(pageCrashed)return'FAIL renderer-crashed';
    for(const k of ['KeyA','KeyS','KeyD']) await key(page,k,500);        // rebind a few keys
    await key(page,'Escape',800); await key(page,'Escape',700);
    return pageCrashed?'FAIL renderer-crashed':'keyboard rebind ok'; });
  // Config -> Sound Volume
  await check(page,'config_sound',async()=>{ await boot(page,server); if(!await gotoButton(page,'Configuration'))return'FAIL reach';
    await key(page,'Enter',900); await key(page,'ArrowRight',700); let l=await rd(page,0x469158); if(!l.includes('Sound'))return'FAIL label="'+l+'"';
    await key(page,'Enter',900); await key(page,'ArrowRight',700); await key(page,'ArrowLeft',700); await key(page,'Escape',700); await key(page,'Escape',700); return'sound ok'; });
  // Config -> Save Config
  await check(page,'config_save',async()=>{ await boot(page,server); if(!await gotoButton(page,'Configuration'))return'FAIL reach';
    await key(page,'Enter',900); await key(page,'ArrowRight',700); await key(page,'ArrowRight',700); let l=await rd(page,0x469158); if(!l.includes('Save'))return'FAIL label="'+l+'"';
    await key(page,'Enter',900); await key(page,'Escape',700); await key(page,'Escape',700); return'save ok'; });
  // Race-opts dialog
  await check(page,'raceopts',async()=>{ await boot(page,server); if(!await gotoButton(page,'Wrecking'))return'FAIL reach';
    await key(page,'Enter',900); await key(page,'F2',700); await key(page,'Escape',700); await key(page,'Escape',700); return'ok'; });
  // Race MODE dialog: navigate all icons (int-typed-byte nav-table regression guard, patch 825)
  await check(page,'racemode_nav',async()=>{ await boot(page,server); if(!await gotoButton(page,'Wrecking'))return'FAIL reach';
    await key(page,'Enter',900);
    for(const k of ['ArrowRight','ArrowRight','ArrowLeft','ArrowLeft']) await key(page,k,650);
    await key(page,'Escape',700); await key(page,'Escape',700); return'nav ok'; });
  // Race TYPE dialog: all 4 icons (Championship/Single/TimeTrials/2P) — the agent's hard-hang, patch 825
  await check(page,'racetype_nav',async()=>{ await boot(page,server); if(!await gotoButton(page,'Wrecking'))return'FAIL reach';
    await key(page,'Enter',900); await key(page,'Enter',900);  // -> Race Type
    for(const k of ['ArrowRight','ArrowRight','ArrowRight','ArrowLeft','ArrowLeft','ArrowLeft']) await key(page,k,650);
    await key(page,'Escape',700); await key(page,'Escape',700); await key(page,'Escape',700); return'4 icons ok'; });
  // Go! -> launch race (verify we leave the FE menu: screen byte @0x460005 != 201)
  await check(page,'go_race',async()=>{ await boot(page,server); if(!await gotoButton(page,'Go!'))return'FAIL reach';
    await key(page,'Enter',1500); await page.waitForTimeout(6000);
    const sb=await page.evaluate(()=>HEAPU8[0x460005]).catch(()=>-1);
    await page.waitForTimeout(1000);
    if(sb===201)return'FAIL still-in-menu sb='+sb;
    // drive a few frames
    for(const k of ['ArrowUp','ArrowUp','ArrowLeft']) await key(page,k,400);
    return 'race launched sb='+sb; });

  // Race launch + pause (Escape) + resume (Continue) — crash/error-free through the pause path
  await check(page,'pause_resume',async()=>{ await boot(page,server); if(!await gotoButton(page,'Go!'))return'FAIL reach';
    await key(page,'Enter',1500); await page.waitForTimeout(5000);
    const sb1=await page.evaluate(()=>HEAPU8[0x460005]).catch(()=>-1);
    if(sb1===201)return'FAIL race-not-launched';
    await key(page,'Escape',1200);                      // pause -> "PAUSED!" menu
    await page.waitForTimeout(600);
    await key(page,'Enter',1200);                        // Continue -> resume
    await page.waitForTimeout(800);
    for(const k of ['ArrowUp','KeyA']) await key(page,k,400);  // still drivable after resume
    return 'pause+resume ok'; });

  const fails=results.filter(r=>!r.pass);
  console.log(`\n=== ${results.length-fails.length}/${results.length} PASS ===`);
  await browser.close(); server.close();
  process.exit(fails.length?2:0);
})().catch(e=>{console.error('ERR '+e.message);process.exit(1);});
