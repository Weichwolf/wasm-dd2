// Does the browser key path actually set dd2_keystate for D(0x44) and Q(0x51)? Read keystate live
// while holding each key. If keystate stays 0, it's a real browser key-map gap (dd2_input.c);
// if it goes 1, the rebind engine (native-proven) would bind it and the stall is pure event timing.
const {serve,key,alive,boot,gotoButton,chromium}=require('./felib.js');
const buildDir=process.argv[2]||'../../web/dd2';
// dd2_keystate is a C global; find its address via the symbol map if exposed, else probe by effect.
// Simpler: check GetKeyState effect indirectly — press key, read whether the poller's debounce byte
// at 0x93fda2+vk flips (poller sets it to 1 on a detected press).
const dbnc=(page,vk)=>page.evaluate(a=>HEAPU8[a],0x93fda2+ (0)).catch(()=>-1); // placeholder
(async()=>{
  const server=serve(buildDir); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:700,height:520}});
  await boot(page,server);
  await gotoButton(page,'Configuration'); await key(page,'Enter',900); await key(page,'Enter',900); await key(page,'Enter',1200);
  // For each candidate, hold it down and sample the poller debounce byte @0x93fda2+vk (poller sets =1 when it detects the press)
  const test=async(code,vk,name)=>{
    await page.evaluate(c=>window.dispatchEvent(new KeyboardEvent('keydown',{code:c})),code);
    let seen=0; for(let i=0;i<8;i++){ const d=await page.evaluate(a=>HEAPU8[a],0x93fda2+vk).catch(()=>-1); if(d===1)seen=1; await page.waitForTimeout(150);}
    await page.evaluate(c=>window.dispatchEvent(new KeyboardEvent('keyup',{code:c})),code); await page.waitForTimeout(400);
    console.log(`  ${name} code=${code} vk=0x${vk.toString(16)}: poller-detected-press=${seen?'YES':'NO'}`);
    return seen;
  };
  console.log('poller-detection per key (proves browser key->keystate->GetKeyState path):');
  const rW=await test('KeyW',0x57,'W'), rA=await test('KeyA',0x41,'A'), rD=await test('KeyD',0x44,'D'), rQ=await test('KeyQ',0x51,'Q'), rE=await test('KeyE',0x45,'E');
  console.log(`RESULT W=${rW} A=${rA} D=${rD} Q=${rQ} E=${rE}  (all YES => key map fine, stall is event ordering not a real gap)`);
  await b.close(); server.close();
})().catch(e=>{console.error('ERR',e.message);process.exit(1);});
