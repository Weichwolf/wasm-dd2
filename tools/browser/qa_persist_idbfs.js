// Verify IDBFS-backed SaveGames persistence across a page reload (same origin -> IndexedDB kept).
// 1) boot -> InitCardSystem creates /persist/SaveGames, /SaveGames symlinks to it.
// 2) write a distinctive marker into /persist/SaveGames + syncfs(false) -> IndexedDB.
// 3) reload -> engine reads the persisted card at boot (InitCardSystem fread -> 0x754460).
// 4) confirm the marker survived in BOTH the file AND the engine's in-memory card.
const {serve,alive,chromium}=require('./felib.js');
const buildDir=process.argv[2]||'../../web/dd2';
const OFF=0x40, MARK=0xA7;   // a card offset unlikely to be overwritten by boot, and a marker byte
(async()=>{
  const server=serve(buildDir); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']});
  const ctx=await b.newContext({viewport:{width:700,height:520}});   // one context -> IndexedDB persists across reload
  const page=await ctx.newPage();
  let errs=[]; page.on('pageerror',e=>errs.push(e.message.split('\n')[0])); page.on('crash',()=>errs.push('CRASH'));
  await page.goto(`http://localhost:${server.address().port}/index.html`,{waitUntil:'load'});
  for(let i=0;i<60;i++){ if(await alive(page))break; await page.waitForTimeout(1000); }
  await page.waitForTimeout(9000);   // let idbfs-load + InitCardSystem run
  const setup=await page.evaluate(()=>{
    var out={};
    try{ out.link=FS.readlink('/SaveGames'); }catch(e){ out.link='ERR:'+e.message; }
    try{ out.size=FS.stat('/persist/SaveGames').size; }catch(e){ out.size='ERR:'+e.message; }
    return out;
  });
  console.log('after boot:', JSON.stringify(setup));
  // write marker into the persisted card + flush to IndexedDB
  const wrote=await page.evaluate(({off,mark})=>{
    return new Promise(res=>{
      try{
        var d=FS.readFile('/persist/SaveGames');           // Uint8Array
        var before=d[off]; d[off]=mark; FS.writeFile('/persist/SaveGames', d);
        FS.syncfs(false, function(err){ res({before:before, after:mark, err:err?(''+err):null}); });
      }catch(e){ res({err:'EXC:'+e.message}); }
    });
  }, {off:OFF, mark:MARK});
  console.log('wrote marker:', JSON.stringify(wrote));
  // reload (same context => IndexedDB retained)
  await page.reload({waitUntil:'load'});
  for(let i=0;i<60;i++){ if(await alive(page))break; await page.waitForTimeout(1000); }
  await page.waitForTimeout(9000);
  const chk=await page.evaluate(({off})=>{
    var out={};
    try{ out.fileByte=FS.readFile('/persist/SaveGames')[off]; }catch(e){ out.fileByte='ERR:'+e.message; }
    try{ out.cardByte=HEAPU8[0x754460+off]; }catch(e){ out.cardByte='ERR:'+e.message; }   // engine in-memory card
    return out;
  }, {off:OFF});
  console.log('after reload:', JSON.stringify(chk));
  const filePersisted = chk.fileByte===MARK;
  const engineRead = chk.cardByte===MARK;   // engine's InitCardSystem read the persisted card
  console.log(`RESULT file-persisted=${filePersisted} engine-read-persisted=${engineRead} crashed=${errs.length>0} errs=${JSON.stringify(errs)} -> ${filePersisted&&engineRead?'PASS':'FAIL'}`);
  await b.close(); server.close(); process.exit(filePersisted&&engineRead?0:2);
})().catch(e=>{console.error('ERR',e.message);process.exit(1);});
