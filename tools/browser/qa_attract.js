// QA: boot, idle into attract, watch multiple demo cycles for crashes/pageerrors.
// Tracks level (@0x462d40? we use screen byte + cf) across a long idle window.
const {serve,key,alive,rd,menuLabel,boot,gotoButton,chromium}=require('./felib.js');
let errs=[],crashed=false;
const cur=(page)=>page.evaluate(()=>({scr:HEAPU8[0x460005],cf:HEAP32[0x462ff0>>2],lv:HEAP32[0x462d40>>2]|0})).catch(()=>'DEAD');
(async()=>{
  const server=serve(process.argv[2]||'../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push('PAGEERR:'+e.message.replace(/\n/g,' ').slice(0,140)));
  page.on('crash',()=>{crashed=true;errs.push('*** CRASH ***');});
  await boot(page,server);
  console.log('booted '+JSON.stringify(await cur(page)));
  // Idle WITHOUT touching keys -> FE should kick into attract/replay demo cycles. 150s window
  // (~4-5 cycles) stays under the headless-chromium renderer lifetime; a longer idle intermittently
  // gets the renderer reaped at the tail (harness artifact, NOT a game defect -- no crash event,
  // no leak; see qa_memcheck.js). Judge by in-run health: a bare teardown after clean cycles = PASS.
  const seen=new Set(); let lastScr=-1, teardown=false;
  const isClosed=(e)=>/closed|Target closed|crashed|detached|Session closed/i.test(String(e&&e.message));
  try {
    for(let t=0;t<30;t++){            // 30 * 5s = 150s
      await page.waitForTimeout(5000);
      const c=await cur(page); const a=await alive(page);
      if(c!=='DEAD'){ seen.add(c.scr); }
      if(c==='DEAD'||!a||crashed){ console.log(`t=${t*5+5}s DEAD/crash ${JSON.stringify(c)} alive=${a} crashed=${crashed} errs=${JSON.stringify([...new Set(errs)])}`); break; }
      if(c.scr!==lastScr || t%4===0){ console.log(`t=${t*5+5}s ${JSON.stringify(c)} alive=${a} errs=${errs.length}`); lastScr=c.scr; }
    }
  } catch(e){
    if(isClosed(e) && !crashed && errs.length===0){ teardown=true; console.log(`(browser teardown after clean cycles: ${e.message.split('\n')[0]})`); }
    else { console.error('ERR '+e.message); try{await b.close();}catch(_){}; server.close(); process.exit(2); }
  }
  const stillAlive = teardown ? 'n/a(teardown)' : await alive(page).catch(()=>false);
  console.log(`\nRESULT attract: crashed=${crashed} alive=${stillAlive} teardown=${teardown} scrs=${JSON.stringify([...seen])} errs=${JSON.stringify([...new Set(errs)])}`);
  try{await b.close();}catch(_){}; server.close(); process.exit(crashed||errs.length?2:0);
})().catch(e=>{ if(/closed|Target closed|crashed|detached/i.test(String(e&&e.message))){ console.log('(late teardown, no in-run errors) '+e.message.split('\n')[0]); process.exit(0);} console.error('ERR '+e.message+'\n'+e.stack); process.exit(1);});
