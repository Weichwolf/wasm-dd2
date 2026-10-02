// QA: after the Race Type idx2 trap, is the FE hung? Try Escape/arrows/Enter and watch if ANY
// observable state changes (fe byte, menu label). Baseline: a healthy dialog responds to keys.
const {serve,key,alive,rd,menuLabel,boot}=require('./felib.js');
const {chromium}=require('./playwright');
const readFE=(page)=>page.evaluate(()=>HEAP8[0x460005]).catch(()=>'DEAD');
let errs=[];
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,120)));
  await boot(page,server);
  await key(page,'Enter'); await key(page,'Enter'); await page.waitForTimeout(600); // Race Type
  console.log('healthy Race Type: fe='+await readFE(page)+' errs='+errs.length);
  // healthy baseline: ArrowRight once (idx1) should be fine
  await key(page,'ArrowRight');
  console.log('idx1 (healthy): fe='+await readFE(page)+' errs='+errs.length);
  // now trigger trap: ArrowRight to idx2
  await key(page,'ArrowRight');
  console.log('idx2 (TRAP): fe='+await readFE(page)+' errs='+errs.length+' '+JSON.stringify([...new Set(errs)]));
  // recovery attempts
  for(const k of ['Escape','ArrowLeft','ArrowLeft','Enter','Escape','Enter']){
    const before=errs.length;
    await key(page,k);
    console.log(`  recover ${k}: fe=${await readFE(page)} errs=${errs.length} (newTrap=${errs.length>before})`);
  }
  console.log('FINAL fe='+await readFE(page)+' (healthy main menu fe=201/-55). errs='+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);process.exit(1);});
