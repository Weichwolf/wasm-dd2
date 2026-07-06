// Distinguish "game memory leak" from "headless-chromium resource limit": launch+retire a race
// repeatedly, sampling wasm heap size (HEAPU8.length, grows only if the engine allocates) and the
// JS heap. A steady linear climb = real leak (bug); flat = chromium lifetime limit (harness).
const {serve,key,alive,boot,gotoButton,waitRace,chromium}=require('./felib.js');
const buildDir=process.argv[2]||'../../web/dd2';
const mem=(page)=>page.evaluate(()=>({wasm:HEAPU8.length, js:(performance.memory?performance.memory.usedJSHeapSize:0)})).catch(()=>({wasm:-1,js:-1}));
const kk=async(page,c,post=400)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(70);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(post);};
async function retire(page){ // race -> Race Over -> Proceed>> -> FE
  await kk(page,'Escape',900); for(let i=0;i<3;i++) await kk(page,'ArrowDown',400);
  await kk(page,'Enter',700); await kk(page,'ArrowUp',400); await kk(page,'Enter',1600); await page.waitForTimeout(1200);
  await kk(page,'ArrowDown',500); await kk(page,'ArrowRight',500); await kk(page,'ArrowDown',500); await kk(page,'Enter',1800);
  for(let i=0;i<8;i++){ if(await page.evaluate(()=>HEAPU8[0x460005]).catch(()=>-1)===201)break; await kk(page,'Escape',500);} }
(async()=>{
  const server=serve(buildDir); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox','--js-flags=--expose-gc']}); const page=await b.newPage({viewport:{width:700,height:520}});
  let crashed=false,errs=[]; page.on('pageerror',e=>errs.push(e.message.split('\n')[0])); page.on('crash',()=>{crashed=true;});
  await boot(page,server);
  const m0=await mem(page); console.log(`baseline wasm=${(m0.wasm/1048576).toFixed(1)}MB js=${(m0.js/1048576).toFixed(1)}MB`);
  const samples=[];
  for(let r=0; r<16 && !crashed; r++){
    if(!await gotoButton(page,'Go!')){ console.log(`r${r}: could not reach Go!`); break; }
    await kk(page,'Enter',1200); const lr=await waitRace(page,20000);
    for(let d=0;d<4;d++) await kk(page,'KeyA',300);
    await retire(page);
    const m=await mem(page); samples.push(m.wasm);
    console.log(`race ${r}: launched=${lr.launched} wasm=${(m.wasm/1048576).toFixed(1)}MB js=${(m.js/1048576).toFixed(1)}MB crashed=${crashed} errs=${errs.length}`);
  }
  const first=samples[0], last=samples[samples.length-1], grew=((last-first)/1048576).toFixed(1);
  console.log(`RESULT wasm-growth over ${samples.length} races = ${grew}MB (${first} -> ${last} bytes) crashed=${crashed} errs=${JSON.stringify([...new Set(errs)])}`);
  console.log(grew>16 ? 'LIKELY LEAK (wasm heap grew >16MB)' : 'wasm heap STABLE (no engine leak; any browser death = chromium resource limit)');
  await b.close(); server.close();
})().catch(e=>{console.error('ERR',e.message);process.exit(1);});
