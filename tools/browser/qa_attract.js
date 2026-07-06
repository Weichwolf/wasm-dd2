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
  // Idle a long time WITHOUT touching keys -> FE should kick into attract/replay demo cycles.
  const seen=new Set(); let lastScr=-1;
  for(let t=0;t<48;t++){            // 48 * 5s = 240s ~= several demo cycles
    await page.waitForTimeout(5000);
    const c=await cur(page); const a=await alive(page);
    if(c!=='DEAD'){ seen.add(c.scr); }
    if(c==='DEAD'||!a||crashed){ console.log(`t=${t*5+5}s DEAD/crash ${JSON.stringify(c)} alive=${a} crashed=${crashed} errs=${JSON.stringify([...new Set(errs)])}`); break; }
    if(c.scr!==lastScr || t%4===0){ console.log(`t=${t*5+5}s ${JSON.stringify(c)} alive=${a} errs=${errs.length}`); lastScr=c.scr; }
  }
  console.log(`\nRESULT attract: crashed=${crashed} alive=${await alive(page)} scrs=${JSON.stringify([...seen])} errs=${JSON.stringify([...new Set(errs)])}`);
  await b.close(); server.close(); process.exit(crashed||errs.length?2:0);
})().catch(e=>{console.error('ERR '+e.message+'\n'+e.stack);process.exit(1);});
