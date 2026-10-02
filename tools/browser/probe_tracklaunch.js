// QA: pick track N via menu F2, then Go! -> launch, confirm the race loads (cf advances, 3D view).
// arg2 = number of F2 presses (track index).
const {serve,key,alive,boot,gotoButton}=require('./felib.js');
const {chromium}=require('./playwright'); const fs=require('fs');
const OUT='/tmp/qa_explore';
const TN=parseInt(process.argv[2]||'2');
const rt=(page)=>page.evaluate(()=>HEAP32[0x4673fc>>2]).catch(()=>'?');
const cf=(page)=>page.evaluate(()=>HEAP32[0x462ff0>>2]).catch(()=>'DEAD');
let errs=[];
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,140)));
  page.on('crash',()=>errs.push('PAGE CRASHED'));
  await boot(page,server);
  await gotoButton(page,'Select Track');
  for(let i=0;i<TN;i++) await key(page,'F2');
  console.log('track set race_track='+await rt(page));
  // from Select Track button (row0,col2) go to Go! (row1,col3): ArrowDown then ArrowRight.
  await browser2Go(page);
  console.log('at button, menuLabel check via Go path');
  await key(page,'Enter'); await page.waitForTimeout(14000);
  let a=await cf(page); await page.waitForTimeout(2000); let b=await cf(page);
  console.log('race launched track='+await rt(page)+' cf '+a+'->'+b+(b>a?' ADVANCING(loaded)':' frozen')+' alive='+await alive(page)+' errs='+errs.length+' '+JSON.stringify([...new Set(errs)]));
  await page.screenshot({path:`${OUT}/tracklaunch_${TN}.png`});
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);process.exit(1);});
// Navigate from the Select-Track button (top row, col 2) to Go! (bottom row, col 3):
async function browser2Go(page){ for(const k of ['ArrowDown','ArrowDown','ArrowDown']) await key(page,k); }
