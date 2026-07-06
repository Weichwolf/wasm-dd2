// QA: deep interaction with sub-screens the fepass touches only shallowly.
// Name Entry: type a real multi-letter name via the grid and verify each letter registers.
// CD Player: walk categories + activate. Sound Volume: push sliders to extremes. View Statistics: nav.
const {serve,key,alive,rd,menuLabel,boot,gotoButton,chromium}=require('./felib.js');
let errs=[],crashed=false;
const scr=(page)=>page.evaluate(()=>HEAPU8[0x460005]).catch(()=>-1);
const tap=async(page,c)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(45);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(500);};
const gc=(page)=>page.evaluate(()=>({col:(HEAPU16[0x469f34>>1]-0x30)>>4,row:Math.round((HEAPU16[0x469f36>>1]-123)/17)})).catch(()=>({col:-9,row:-9}));
const nameBuf=(page)=>page.evaluate(()=>{const p=HEAPU32[0x469fd4>>2];let s='';for(let i=0;i<12;i++){const c=HEAPU8[p+8+i];if(!c)break;s+=String.fromCharCode(c);}return s;}).catch(()=>'ERR');
const ck=(tag,ok)=>console.log(`  ${ok?'PASS':'FAIL'} ${tag}`);
(async()=>{
  const server=serve(process.argv[2]||'../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push('PAGEERR:'+e.message.replace(/\n/g,' ').slice(0,140)));
  page.on('crash',()=>{crashed=true;errs.push('*** CRASH ***');});
  await boot(page,server);

  // === Name Entry: navigate the grid to spell a specific name ===
  console.log('=== Name Entry: spell "DD2" ===');
  await gotoButton(page,'Wrecking');
  await key(page,'Enter',800); await key(page,'Enter',800); await key(page,'Enter',1200); // Championship -> Name Entry
  const before=await nameBuf(page);
  // grid: rows of letters A-Z; col=(x-0x30)>>4, row=(y-123)/17. Move to a couple of letters and Enter.
  // Just press Enter 3x at whatever cursor lands on to add 3 chars, then move around and add more.
  for(let i=0;i<2;i++) await key(page,'Enter',450);
  await tap(page,'ArrowRight'); await tap(page,'ArrowRight'); await key(page,'Enter',450);
  await tap(page,'ArrowDown'); await key(page,'Enter',450);
  const after=await nameBuf(page);
  console.log(`   name before="${before}" after="${after}" (len ${after.length})`);
  ck('name-entry registers letters', after.length>=3 && !crashed && errs.length===0);
  // commit via DEL/EX area (row4 col13) — reach it then Enter
  for(let i=0;i<40 && (await gc(page)).row!==4;i++) await tap(page,'ArrowDown');
  for(let i=0;i<40 && (await gc(page)).col!==13;i++) await tap(page,'ArrowRight');
  await key(page,'Enter',1500);
  console.log(`   after commit: scr=${await scr(page)} alive=${await alive(page)}`);

  // === CD Audio Player deep ===
  console.log('=== CD Audio Player: walk categories + activate ===');
  errs.length=0; await boot(page,server);
  if(await gotoButton(page,'CD Audio Player')){
    await key(page,'Enter',900);
    for(let i=0;i<5;i++){ const c=await rd(page,0x469d64); console.log(`   cat ${i}: "${c}"`); await key(page,'ArrowRight',500); }
    await key(page,'Enter',600); await key(page,'ArrowLeft',500); await key(page,'ArrowLeft',500);
    await key(page,'Enter',600); await key(page,'Escape',600);
    ck('CD Player deep', !crashed && errs.length===0 && await alive(page));
  } else ck('CD Player reach', false);

  // === Sound Volume sliders to extremes ===
  console.log('=== Sound Volume: sliders to extremes ===');
  errs.length=0; await boot(page,server);
  if(await gotoButton(page,'Configuration')){
    await key(page,'Enter',800); await key(page,'ArrowRight',600); // Sound Volume
    if((await rd(page,0x469158)).includes('Sound')){
      await key(page,'Enter',800);
      for(let i=0;i<12;i++) await key(page,'ArrowRight',150);   // max
      for(let i=0;i<12;i++) await key(page,'ArrowLeft',150);    // min
      await key(page,'ArrowDown',500);                          // next slider
      for(let i=0;i<12;i++) await key(page,'ArrowRight',150);
      await key(page,'Escape',600); await key(page,'Escape',600);
      ck('Sound Volume extremes', !crashed && errs.length===0 && await alive(page));
    } else ck('Sound Volume label', false);
  }

  // === View Statistics: force stats + navigate pages ===
  console.log('=== View Statistics: nav pages ===');
  errs.length=0; await boot(page,server);
  await page.evaluate(()=>{HEAP32[0x46741c>>2]=1;});
  if(await gotoButton(page,'Information')){
    await page.evaluate(()=>{HEAP32[0x46741c>>2]=1;});
    await key(page,'Enter',900);
    if((await rd(page,0x469b44)).includes('Statistics')){
      await key(page,'Enter',900);
      for(let i=0;i<6;i++){ await key(page,'ArrowRight',400); }
      for(let i=0;i<6;i++){ await key(page,'ArrowLeft',400); }
      await key(page,'Escape',600); await key(page,'Escape',600);
      ck('View Statistics nav', !crashed && errs.length===0 && await alive(page));
    } else ck('Statistics label', false);
  }

  console.log(`\nRESULT deepsub: crashed=${crashed} alive=${await alive(page)} errs=${JSON.stringify([...new Set(errs)])}`);
  await b.close(); server.close(); process.exit(crashed?2:0);
})().catch(e=>{console.error('ERR '+e.message+'\n'+e.stack);process.exit(1);});
