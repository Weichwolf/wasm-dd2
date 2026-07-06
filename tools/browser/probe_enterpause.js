// QA: does ENTER pause mid-race (CLAUDE.md claims ESC/ENTER=pause)? Clean isolated test.
const {serve,key,alive,boot,gotoButton}=require('./felib.js');
const {chromium}=require('playwright'); const fs=require('fs');
const OUT='/tmp/qa_explore';
const cf=(page)=>page.evaluate(()=>HEAP32[0x462ff0>>2]).catch(()=>'DEAD');
let errs=[];
(async()=>{
  const server=serve('../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push(e.message.slice(0,120)));
  await page.goto(`http://localhost:${server.address().port}/index.html?race=9`,{waitUntil:'load'});
  for(let i=0;i<60;i++){if(await alive(page))break;await page.waitForTimeout(1000);}
  await page.click('#canvas'); await page.waitForTimeout(12000);
  let a=await cf(page); await page.waitForTimeout(1500); let b=await cf(page);
  console.log('running: cf '+a+'->'+b+(b>a?' OK':' not running'));
  // ENTER only
  await key(page,'Enter'); await page.waitForTimeout(1200);
  let e1=await cf(page); await page.waitForTimeout(1800); let e2=await cf(page);
  console.log('after 1x ENTER: cf '+e1+'->'+e2+(e1===e2?' PAUSED':' NOT paused (still running)'));
  await page.screenshot({path:`${OUT}/enterpause.png`});
  console.log('errs='+JSON.stringify([...new Set(errs)]));
  await browser.close(); server.close();
})().catch(e=>{console.error('FATAL '+e.message);process.exit(1);});
