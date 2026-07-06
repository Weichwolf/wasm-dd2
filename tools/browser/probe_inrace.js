// QA: launch a race via the menu Go!, verify car moves under KeyA/arrows, test pause(Esc & Enter)
// + resume, run ~30s. Screenshots before/after. Watch pageerror/hang.
const {serve,key,alive,boot,gotoButton}=require('./felib.js');
const {chromium}=require('playwright'); const fs=require('fs');
const OUT='/tmp/qa_explore'; fs.mkdirSync(OUT,{recursive:true});
const down=(page,c)=>page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);
const up=(page,c)=>page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);
const cf=(page)=>page.evaluate(()=>typeof HEAP32!=='undefined'?HEAP32[0x462ff0>>2]:'DEAD').catch(()=>'DEAD');
// car0 world x,z (approx position) to prove movement
const pos=(page)=>page.evaluate(()=>[HEAP32[0x792a24>>2],HEAP32[0x792a34>>2]]).catch(()=>'DEAD');
let errs=[];
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.replace(/\n/g,' ').slice(0,140)));
  page.on('crash',()=>errs.push('PAGE CRASHED'));
  await boot(page,server);
  await gotoButton(page,'Go!'); await key(page,'Enter'); await page.waitForTimeout(14000);
  let c0=await cf(page); console.log('race launched cf='+c0+' pos='+await pos(page)+' errs='+errs.length);
  await page.screenshot({path:`${OUT}/race_0start.png`});
  const p0=await pos(page);
  // accelerate 4s
  await down(page,'KeyA'); await page.waitForTimeout(4000);
  const p1=await pos(page); console.log('after 4s KeyA: cf='+await cf(page)+' pos='+p1+' moved='+(JSON.stringify(p0)!==JSON.stringify(p1)));
  await page.screenshot({path:`${OUT}/race_1accel.png`});
  // steer left 2s
  await down(page,'ArrowLeft'); await page.waitForTimeout(2000); await up(page,'ArrowLeft');
  const p2=await pos(page); console.log('after steer: pos='+p2);
  await up(page,'KeyA');
  // PAUSE via Escape
  await key(page,'Escape'); await page.waitForTimeout(1200);
  let a=await cf(page); await page.waitForTimeout(1500); let b=await cf(page);
  console.log('Escape pause: cf '+a+'->'+b+(a===b?' FROZEN(paused)':' STILL ADVANCING'));
  await page.screenshot({path:`${OUT}/race_2pause_esc.png`});
  // resume
  await key(page,'Enter'); await page.waitForTimeout(1500);
  let r1=await cf(page); await page.waitForTimeout(1500); let r2=await cf(page);
  console.log('resume: cf '+r1+'->'+r2+(r2>r1?' RESUMED':' STILL FROZEN'));
  // PAUSE via Enter
  await key(page,'Enter'); await page.waitForTimeout(1200);
  let e1=await cf(page); await page.waitForTimeout(1500); let e2=await cf(page);
  console.log('Enter pause: cf '+e1+'->'+e2+(e1===e2?' FROZEN(paused)':' STILL ADVANCING'));
  await page.screenshot({path:`${OUT}/race_3pause_enter.png`});
  await key(page,'Escape'); await page.waitForTimeout(1000);  // resume/back
  // long run
  await down(page,'KeyA');
  for(let t=0;t<10;t++){const d=['ArrowLeft','ArrowRight'][t%2];await down(page,d);await page.waitForTimeout(1200);await up(page,d);
    if(t%3===0)console.log(`  t=${t*1.5}s cf=${await cf(page)} alive=${await alive(page)} errs=${errs.length}`);
    if(!(await alive(page))||errs.length)break;}
  await up(page,'KeyA');
  await page.screenshot({path:`${OUT}/race_4end.png`});
  console.log('FINAL cf='+await cf(page)+' alive='+await alive(page)+' ERRS='+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);process.exit(1);});
