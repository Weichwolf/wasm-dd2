// Verify each race MODE (Wrecking/StockCar/DestDerby) + Single Race type launches + drives clean.
const {serve,key,alive,rd,menuLabel,boot,gotoButton,waitRace,chromium}=require('./felib.js');
const fs=require('fs'); const OUT='/tmp/modes'; fs.mkdirSync(OUT,{recursive:true});
const buildDir=process.argv[2]||'../../web/dd2';
let crashed=false,errs=[];
const st=(page)=>page.evaluate(()=>({scr:HEAPU8[0x460005],mode:HEAP32[0x4673f8>>2],type:HEAP32[0x4673f4>>2]})).catch(()=>'DEAD');
(async()=>{
  const server=serve(buildDir); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.split('\n')[0]));
  page.on('crash',()=>{crashed=true;});
  for(const [modeIdx,name] of [[0,'Wrecking'],[1,'StockCar'],[2,'DestDerby']]){
    crashed=false;errs=[];
    await boot(page,server);
    if(!await gotoButton(page,'Wrecking')){console.log(`${name}: FAIL reach`);continue;}
    await key(page,'Enter',900);                                  // Race MODE dialog
    for(let i=0;i<modeIdx;i++) await key(page,'ArrowRight',700);   // select mode
    const modeLbl=await rd(page,0x46a3a4).catch(()=>'');           // mode dialog label
    await key(page,'Enter',900);                                  // -> Race TYPE dialog
    await key(page,'ArrowRight',700);                             // Championship -> Single Race (type idx1)
    await key(page,'Enter',1400);                                 // confirm Single (no name entry) -> menu
    const s1=await st(page);
    if(!await gotoButton(page,'Go!')){console.log(`${name}: FAIL Go! (after type; st=${JSON.stringify(s1)})`);continue;}
    await key(page,'Enter',1500);
    const r=await waitRace(page,25000);
    await page.screenshot({path:`${OUT}/${name}_race.png`});
    if(!r.launched){console.log(`${name}: FAIL race-not-launched (sb=${r.sb} cf=${r.cf} lvl=${r.lvl})`);continue;}
    const s2=await st(page);
    // drive
    await page.evaluate(()=>window.dispatchEvent(new KeyboardEvent('keydown',{code:'KeyA'})));
    await page.waitForTimeout(5000);
    await page.evaluate(()=>window.dispatchEvent(new KeyboardEvent('keyup',{code:'KeyA'})));
    await page.screenshot({path:`${OUT}/${name}_driving.png`});
    const ok=!crashed && await alive(page);
    console.log(`${name}: mode=${s2.mode} type=${s2.type} scr=${s2.scr} drove-clean=${ok} crashed=${crashed} errs=${errs.length} ${ok?'PASS':'FAIL'}`);
  }
  await b.close(); server.close();
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
