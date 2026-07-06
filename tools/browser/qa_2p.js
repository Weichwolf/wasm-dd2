// QA: TWO PLAYER / MULTI (race_type=3) via FE. 2P = type idx3 -> Select_Multi -> Enter_Driver_Names(1,10).
// The multi name-entry loops for up to 10 players; you commit each name and finish with an EMPTY commit
// (navigate to EX cell row4/col13 with no letters typed) which exits + sets race_type=3.
const {serve,key,alive,rd,menuLabel,boot,gotoButton,chromium}=require('./felib.js');
const buildDir=process.argv[2]||'../../web/dd2';
let crashed=false,errs=[];
const st=(page)=>page.evaluate(()=>({scr:HEAPU8[0x460005],mode:HEAP32[0x4673f8>>2],type:HEAP32[0x4673f4>>2],ncars:HEAP32[0x46765c>>2],lvl:HEAP32[0x936ff4>>2],cf:HEAP32[0x462ff0>>2]})).catch(()=>'DEAD');
const gc=(page)=>page.evaluate(()=>({col:(HEAPU16[0x469f34>>1]-0x30)>>4,row:Math.round((HEAPU16[0x469f36>>1]-123)/17)})).catch(()=>({col:-9,row:-9}));
const nameBuf=(page)=>page.evaluate(()=>{const p=HEAPU32[0x469fd4>>2];let s='';for(let i=0;i<12;i++){const c=HEAPU8[p+8+i];if(!c)break;s+=String.fromCharCode(c);}return s;}).catch(()=>'ERR');
async function tap(page,c){await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(45);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(850);}
async function toEX(page){ // navigate cursor to EX (row4,col13)
  for(let i=0;i<40;i++){if((await gc(page)).row===4)break;await tap(page,'ArrowDown');}
  for(let i=0;i<40;i++){if((await gc(page)).col===13)break;await tap(page,'ArrowRight');}
}
(async()=>{
  const server=serve(buildDir); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>{errs.push(e.message.replace(/\n/g,' ').slice(0,160));});
  page.on('crash',()=>{crashed=true;errs.push('RENDERER CRASH');});
  await boot(page,server);
  if(!await gotoButton(page,'Wrecking')){console.log('FAIL reach Wrecking menu');process.exit(2);}
  await key(page,'Enter',900);                      // Race MODE dialog
  await key(page,'Enter',900);                      // -> Race TYPE dialog
  for(let i=0;i<3;i++) await key(page,'ArrowRight',700);   // 2P = idx3
  console.log('at 2P icon st='+JSON.stringify(await st(page)));
  await key(page,'Enter',1400);                     // confirm 2P -> Enter_Driver_Names(1,10) name-entry
  console.log('after 2P confirm st='+JSON.stringify(await st(page))+' cursor='+JSON.stringify(await gc(page)));
  await page.screenshot({path:'/tmp/qa_2p_nameentry.png'});
  // Enter player 1 and player 2 names, then EMPTY commit to finish.
  for(let pl=1;pl<=2;pl++){
    for(let i=0;i<3;i++) await key(page,'Enter',400);    // 3 letters via home cursor (col0/row0 default = 'A')
    const nm=await nameBuf(page);
    console.log('player'+pl+' name-buf="'+nm+'" cursor='+JSON.stringify(await gc(page)));
    if(nm.length<1){console.log('FAIL player'+pl+' name not registered (multi name entry broken)');await page.screenshot({path:'/tmp/qa_2p_noname.png'});await b.close();server.close();process.exit(2);}
    await toEX(page); await key(page,'Enter',1400);       // commit this player's name
    console.log('after commit p'+pl+' st='+JSON.stringify(await st(page))+' cursor='+JSON.stringify(await gc(page)));
  }
  // EMPTY commit to exit the loop
  await toEX(page); await key(page,'Enter',1600);
  const sAfter=await st(page);
  console.log('after EMPTY-commit(exit) st='+JSON.stringify(sAfter)+' race_type(expect 3)='+sAfter.type);
  if(sAfter.type!==3){console.log('NOTE: race_type != 3 after multi name entry (got '+sAfter.type+')');}
  // Back on menu -> Go! -> launch
  if(!await gotoButton(page,'Go!')){console.log('FAIL cannot reach Go! after 2P names; st='+JSON.stringify(await st(page)));await page.screenshot({path:'/tmp/qa_2p_nogo.png'});await b.close();server.close();process.exit(2);}
  await key(page,'Enter',1500); await page.waitForTimeout(7000);
  const s2=await st(page);
  await page.screenshot({path:'/tmp/qa_2p_race.png'});
  const launched=(s2!=='DEAD') && (s2.lvl>0) && (s2.cf>0);
  if(!launched){console.log('FAIL race-not-launched st='+JSON.stringify(s2)+' errs='+JSON.stringify([...new Set(errs)]));await b.close();server.close();process.exit(2);}
  console.log('LAUNCHED st='+JSON.stringify(s2));
  // drive ~8s
  await page.evaluate(()=>window.dispatchEvent(new KeyboardEvent('keydown',{code:'KeyA'})));
  const cfs=[];
  for(let i=0;i<8;i++){await page.waitForTimeout(1000);const s=await st(page);cfs.push(s==='DEAD'?'DEAD':s.cf);if(i===3)await key(page,'ArrowLeft',300);}
  await page.evaluate(()=>window.dispatchEvent(new KeyboardEvent('keyup',{code:'KeyA'})));
  const s3=await st(page);
  await page.screenshot({path:'/tmp/qa_2p_driving.png'});
  const ok=!crashed && await alive(page) && s3!=='DEAD' && s3.cf>s2.cf;
  console.log('cf-trace='+cfs.join(',')+' finalst='+JSON.stringify(s3));
  console.log('errs='+JSON.stringify([...new Set(errs)]));
  console.log((ok?'PASS':'FAIL')+' two-player drove-clean='+ok+' crashed='+crashed);
  await b.close(); server.close();
  process.exit(ok?0:2);
})().catch(e=>{console.error('ERR '+e.message);server.close();process.exit(1);});
