// QA: full race lifecycle repeated across modes/types: configure -> Go! -> drive -> pause/resume ->
// retire -> results/replay -> back to FE -> repeat with a different mode. Odd-moment pauses too.
const {serve,key,alive,rd,menuLabel,boot,gotoButton,chromium}=require('./felib.js');
let errs=[],crashed=false;
const cur=(page)=>page.evaluate(()=>({scr:HEAPU8[0x460005],mode:HEAP32[0x4673f8>>2],type:HEAP32[0x4673f4>>2],cf:HEAP32[0x462ff0>>2]})).catch(()=>'DEAD');
const hold=(page,c,d)=>page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c).then(()=>page.waitForTimeout(d)).then(()=>page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c));
async function toMenu(page){ for(let i=0;i<6;i++){ if((await cur(page)).scr===201)return true; await key(page,'Escape',700);} return (await cur(page)).scr===201; }
async function configMode(page,modeIdx,typeIdx){ // from main menu
  if(!await gotoButton(page,'Wrecking'))return false;
  await key(page,'Enter',800);                          // Race MODE
  for(let i=0;i<modeIdx;i++) await key(page,'ArrowRight',600);
  await key(page,'Enter',800);                          // Race TYPE
  for(let i=0;i<typeIdx;i++) await key(page,'ArrowRight',600);
  await key(page,'Enter',1400);                         // confirm (typeIdx1=Single = no name entry)
  return true;
}
async function launch(page){ if(!await gotoButton(page,'Go!'))return false; await key(page,'Enter',1500); await page.waitForTimeout(7000); return (await cur(page)).scr!==201; }
async function retire(page){ await key(page,'Escape',700); for(let i=0;i<3;i++) await key(page,'ArrowDown',450);
  await key(page,'Enter',600); await key(page,'ArrowUp',500); await key(page,'Enter',1600); }
(async()=>{
  const server=serve(process.argv[2]||'../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push('PAGEERR:'+e.message.replace(/\n/g,' ').slice(0,140)));
  page.on('crash',()=>{crashed=true;errs.push('*** CRASH ***');});
  await boot(page,server);
  const modes=[[0,'Wrecking'],[1,'StockCar'],[2,'DestDerby']];
  for(const [mi,mn] of modes){
    if(crashed)break;
    errs.length=0;
    await toMenu(page);
    if(!await configMode(page,mi,1)){console.log(`${mn}: FAIL config`);continue;}
    if(!await launch(page)){console.log(`${mn}: FAIL launch ${JSON.stringify(await cur(page))}`);await toMenu(page);continue;}
    const s0=await cur(page); console.log(`${mn}: launched ${JSON.stringify(s0)}`);
    // drive
    await hold(page,'KeyA',3000);
    // pause immediately + resume
    await key(page,'Escape',1000); console.log(`   paused ${JSON.stringify(await cur(page))}`);
    await key(page,'Enter',1000);  console.log(`   resumed ${JSON.stringify(await cur(page))}`);
    await hold(page,'KeyA',1500);
    if(crashed||!await alive(page)){console.log(`${mn}: DIED mid-race errs=${JSON.stringify([...new Set(errs)])}`);break;}
    // retire -> results
    await retire(page);
    await page.waitForTimeout(4000);
    const rs=await cur(page); console.log(`   after retire ${JSON.stringify(rs)} alive=${await alive(page)}`);
    // advance through results/replay back to menu
    const back=await toMenu(page);
    console.log(`${mn}: back-to-menu=${back} ${JSON.stringify(await cur(page))} alive=${await alive(page)} crashed=${crashed} errs=${errs.length} ${errs.length?JSON.stringify([...new Set(errs)]):''}`);
  }
  console.log(`\nRESULT lifecycle: crashed=${crashed} alive=${await alive(page)} errs=${JSON.stringify([...new Set(errs)])}`);
  await b.close(); server.close(); process.exit(crashed?2:0);
})().catch(e=>{console.error('ERR '+e.message+'\n'+e.stack);process.exit(1);});
