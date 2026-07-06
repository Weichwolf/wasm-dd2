// QA: Select Track — cycle ALL tracks with F2, screenshot each, read track label + race_track.
// Then launch a race on 2 different tracks to confirm they load. Watch trap/hang.
const {serve,key,alive,rd,menuLabel,boot,gotoButton}=require('./felib.js');
const {chromium}=require('playwright'); const fs=require('fs');
const OUT='/tmp/qa_explore'; fs.mkdirSync(OUT,{recursive:true});
const tlabel=(page)=>rd(page,0x469784);
const rt=(page)=>page.evaluate(()=>HEAP32[0x4673fc>>2]).catch(()=>'?');
let errs=[];
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,140)));
  page.on('crash',()=>errs.push('PAGE CRASHED'));
  await boot(page,server);
  await gotoButton(page,'Select Track'); await key(page,'Enter'); await page.waitForTimeout(1000);
  console.log('Select Track open: label='+await tlabel(page)+' race_track='+await rt(page)+' errs='+errs.length);
  await page.screenshot({path:`${OUT}/trk_0.png`});
  let seen=[];
  for(let i=1;i<=12;i++){
    await key(page,'F2');
    const l=await tlabel(page), t=await rt(page);
    seen.push(t+':'+l);
    console.log(`  F2#${i}: race_track=${t} label="${l}" alive=${await alive(page)} errs=${errs.length}${errs.length?' '+JSON.stringify([...new Set(errs)]):''}`);
    await page.screenshot({path:`${OUT}/trk_f2_${i}.png`});
    if(!(await alive(page))||errs.length)break;
  }
  console.log('tracks seen: '+JSON.stringify(seen));
  // launch current track
  await gotoButton_fromTrack(page);
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);process.exit(1);});
// from Select Track screen, go down to Go! and launch (ArrowDown x3 then to Go!). Simplify: reboot approach not needed.
async function gotoButton_fromTrack(page){
  // navigate menu: from track screen the main-menu Go! is bottom-right. Use ArrowDown to leave field then find Go!.
  for(const k of ['ArrowDown','ArrowDown','ArrowDown']) await key(page,k);
  console.log('at menu button: '+await menuLabel(page));
  await key(page,'Enter'); await page.waitForTimeout(13000);
  console.log('launched track race: alive='+await alive(page)+' cf='+await page.evaluate(()=>HEAP32[0x462ff0>>2]).catch(()=>'DEAD')+' errs='+errs.length);
  await page.screenshot({path:`${OUT}/trk_race.png`});
}
