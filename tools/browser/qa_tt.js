// QA: TIME TRIAL (race_type=1) via FE menu flow. Launch + drive + confirm no crash.
const {serve,key,alive,rd,menuLabel,boot,gotoButton,chromium}=require('./felib.js');
const buildDir=process.argv[2]||'../../web/dd2';
let crashed=false,errs=[];
const st=(page)=>page.evaluate(()=>({scr:HEAPU8[0x460005],mode:HEAP32[0x4673f8>>2],type:HEAP32[0x4673f4>>2],ncars:HEAP32[0x46765c>>2],lvl:HEAP32[0x936ff4>>2],cf:HEAP32[0x462ff0>>2]})).catch(()=>'DEAD');
(async()=>{
  const server=serve(buildDir); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>{errs.push(e.message.replace(/\n/g,' ').slice(0,160));});
  page.on('crash',()=>{crashed=true;errs.push('RENDERER CRASH');});
  await boot(page,server);
  if(!await gotoButton(page,'Wrecking')){console.log('FAIL reach Wrecking menu');process.exit(2);}
  await key(page,'Enter',900);                      // Race MODE dialog (Wrecking idx0 selected)
  console.log('mode-dialog st='+JSON.stringify(await st(page)));
  await key(page,'Enter',900);                      // -> Race TYPE dialog
  console.log('type-dialog(open) st='+JSON.stringify(await st(page)));
  // TimeTrials = type idx2 in the TYPE dialog (Championship/Single/TimeTrials/2P)
  await key(page,'ArrowRight',700);
  console.log('after R1 type='+JSON.stringify(await st(page)));
  await key(page,'ArrowRight',700);
  console.log('after R2 (should be TimeTrials) type='+JSON.stringify(await st(page)));
  // try reading a few candidate type-dialog label addresses
  for(const a of [0x46a3a4,0x469784,0x46975c,0x46a1a4]){const l=await rd(page,a);if(l&&l!=='')console.log('  lbl@'+a.toString(16)+'="'+l+'"');}
  await key(page,'Enter',1400);                     // confirm TimeTrials
  const sSel=await st(page); console.log('after confirm st='+JSON.stringify(sSel));
  // May land on menu (Go!) OR on name entry. Check screen byte.
  if(!await gotoButton(page,'Go!')){
    console.log('note: could not reach Go! directly; st='+JSON.stringify(await st(page)));
    // maybe name-entry-like screen; try Enter to accept then re-check
    await key(page,'Enter',1200);
    if(!await gotoButton(page,'Go!')){console.log('FAIL cannot reach Go! after TimeTrials select');await page.screenshot({path:'/tmp/qa_tt_stuck.png'});await b.close();server.close();process.exit(2);}
  }
  await key(page,'Enter',1500); await page.waitForTimeout(7000);
  const s2=await st(page);
  await page.screenshot({path:'/tmp/qa_tt_race.png'});
  if(s2==='DEAD'||s2.scr===201){console.log('FAIL race-not-launched st='+JSON.stringify(s2)+' errs='+JSON.stringify([...new Set(errs)]));await b.close();server.close();process.exit(2);}
  console.log('LAUNCHED st='+JSON.stringify(s2));
  // drive ~8s (accel held + steer)
  await page.evaluate(()=>window.dispatchEvent(new KeyboardEvent('keydown',{code:'KeyA'})));
  const cfs=[];
  for(let i=0;i<8;i++){await page.waitForTimeout(1000);const s=await st(page);cfs.push(s==='DEAD'?'DEAD':s.cf);if(i===3)await key(page,'ArrowLeft',300);}
  await page.evaluate(()=>window.dispatchEvent(new KeyboardEvent('keyup',{code:'KeyA'})));
  const s3=await st(page);
  await page.screenshot({path:'/tmp/qa_tt_driving.png'});
  const ok=!crashed && await alive(page);
  console.log('cf-trace='+cfs.join(',')+' finalst='+JSON.stringify(s3));
  console.log('errs='+JSON.stringify([...new Set(errs)]));
  console.log((ok&&s3!=='DEAD'&&s3.scr!==201?'PASS':'FAIL')+' time-trial drove-clean='+ok+' crashed='+crashed);
  await b.close(); server.close();
  process.exit(ok?0:2);
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
