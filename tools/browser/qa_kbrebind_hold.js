// Prove keyboard rebind end-to-end after the byte-store fix: hold each key LONG enough to span a
// headless poll cycle (edge-triggered poller FUN_0044fe64 needs GetKeyState==pressed at a poll).
// Success = all 5 prompts advance to the pressed VK (no adjacent-byte corruption) AND Enter commits
// the working map @0x93fd90 back to the active map @0x46757a and returns to the FE (scr 201).
const {serve,key,alive,boot,gotoButton,chromium}=require('./felib.js');
const buildDir=process.argv[2]||'../../web/dd2';
const rdmap=(page,addr)=>page.evaluate(a=>{let m=[];for(let i=0;i<18;i++)m.push(HEAPU8[a+i]);return m;},addr).catch(()=>'ERR');
const scr=(page)=>page.evaluate(()=>HEAPU8[0x460005]).catch(()=>-1);
// hold a key down ~700ms (>= 2 headless poll cycles), then release and settle
const hold=async(page,code,down=800,up=500)=>{ await page.evaluate(c=>window.dispatchEvent(new KeyboardEvent('keydown',{code:c})),code); await page.waitForTimeout(down); await page.evaluate(c=>window.dispatchEvent(new KeyboardEvent('keyup',{code:c})),code); await page.waitForTimeout(up); };
(async()=>{
  const server=serve(buildDir); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:700,height:520}});
  let errs=[]; page.on('pageerror',e=>errs.push(e.message.split('\n')[0])); page.on('crash',()=>errs.push('CRASH'));
  await boot(page,server);
  await gotoButton(page,'Configuration'); await key(page,'Enter',900); await key(page,'Enter',900); await key(page,'Enter',1200); // -> rebind screen
  const before=await rdmap(page,0x46757a);
  console.log('active map BEFORE:', JSON.stringify(before));
  const seq=['KeyW','KeyS','KeyA','KeyD','KeyQ'], vk={KeyW:87,KeyS:83,KeyA:65,KeyD:68,KeyQ:81}, OFF=[5,3,8,12,13];
  for(let i=0;i<seq.length;i++){ await hold(page,seq[i]); const w=await rdmap(page,0x93fd90);
    console.log(`  prompt${i} ${seq[i]}(vk${vk[seq[i]]}): work[+${OFF[i]}]=${w[OFF[i]]} adjacent-intact=${JSON.stringify(w.slice(6,8))} full=${JSON.stringify(w)}`); }
  // commit
  await key(page,'Enter',1500); await page.waitForTimeout(500);
  const after=await rdmap(page,0x46757a), sc=await scr(page);
  const committed = after[5]===87 && after[3]===83 && after[8]===65 && after[12]===68 && after[13]===81;
  console.log('active map AFTER commit:', JSON.stringify(after));
  console.log(`RESULT committed=${committed} backAtMenu(scr201)=${sc===201} adjacent-preserved=${after[6]===112&&after[7]===113} crashed=${errs.length>0} errs=${JSON.stringify(errs)}`);
  await b.close(); server.close(); process.exit(committed?0:2);
})().catch(e=>{console.error('ERR',e.message);process.exit(1);});
