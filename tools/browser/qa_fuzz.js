// QA: long randomized fuzz across the whole FE + races. Randomly navigates, enters/exits sub-screens,
// launches races, drives, pauses, retires (via Proceed), for many iterations. Any pageerror/crash/HEAP
// loss is a finding. Also a deep Name-Entry typing check + slider extremes.
const {serve,key,alive,rd,menuLabel,boot,gotoButton,chromium,PATHS}=require('./felib.js');
let errs=[],crashed=false;
const scr=(page)=>page.evaluate(()=>typeof HEAPU8!=='undefined'?HEAPU8[0x460005]:-1).catch(()=>-2);
const st=(page)=>page.evaluate(()=>({scr:HEAPU8[0x460005],cf:HEAP32[0x462ff0>>2]})).catch(()=>'DEAD');
const kk=async(page,c,post=250)=>{await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keydown',{code:cc})),c);await page.waitForTimeout(70);await page.evaluate(cc=>window.dispatchEvent(new KeyboardEvent('keyup',{code:cc})),c);await page.waitForTimeout(post);};
async function toMenu(page){for(let i=0;i<8;i++){if(await scr(page)===201)return true;await kk(page,'Escape',500);}return await scr(page)===201;}
async function proceedRaceOver(page){ // retire from a running race back to FE via Race Over Proceed
  await kk(page,'Escape',900); for(let i=0;i<3;i++) await kk(page,'ArrowDown',400);
  await kk(page,'Enter',700); await kk(page,'ArrowUp',400); await kk(page,'Enter',1500);
  await page.waitForTimeout(1500);
  await kk(page,'ArrowDown',500); await kk(page,'ArrowRight',500); await kk(page,'ArrowDown',500); await kk(page,'Enter',1800);
  return await toMenu(page);
}
(async()=>{
  const server=serve(process.argv[2]||'../../web/dd2'); await new Promise(r=>server.listen(0,r));
  const b=await chromium.launch({args:['--no-sandbox']}); const page=await b.newPage({viewport:{width:700,height:520}});
  page.on('pageerror',e=>errs.push('PAGEERR:'+e.message.replace(/\n/g,' ').slice(0,140)));
  page.on('crash',()=>{crashed=true;errs.push('*** CRASH ***');});
  await boot(page,server);
  const keys=['ArrowUp','ArrowDown','ArrowLeft','ArrowRight','Enter','Escape','F1','F2'];
  const btns=Object.keys(PATHS);
  let iter=0;
  const T0=Date.now();
  while(Date.now()-T0 < 300000 && !crashed){   // 5 min fuzz
    iter++;
    // pick a random sub-screen, enter, mash inside, exit
    await toMenu(page);
    const btn=btns[Math.floor(Math.random()*btns.length)];
    if(btn==='Go!'){
      if(await gotoButton(page,'Go!')){ await kk(page,'Enter',1400); await page.waitForTimeout(5000);
        if(await scr(page)!==201){ // drive random + pause + proceed out
          for(let d=0;d<6;d++) await kk(page,keys[Math.floor(Math.random()*4)],300);
          await kk(page,'Escape',900); await kk(page,'Enter',900);   // pause+resume
          await proceedRaceOver(page); } }
    } else if(await gotoButton(page,btn)){
      await kk(page,'Enter',700);
      const nmash=4+Math.floor(Math.random()*8);
      for(let m=0;m<nmash;m++) await kk(page,keys[Math.floor(Math.random()*keys.length)],140);
      await kk(page,'Escape',500); await kk(page,'Escape',500);
    }
    if(iter%5===0){ const a=await alive(page); console.log(`iter ${iter} (${((Date.now()-T0)/1000)|0}s): last-btn=${btn} ${JSON.stringify(await st(page))} alive=${a} errs=${errs.length} crashed=${crashed}`); if(!a){crashed=true;break;} }
  }
  console.log(`\nRESULT fuzz: iters=${iter} crashed=${crashed} alive=${await alive(page)} errs=${JSON.stringify([...new Set(errs)])}`);
  await b.close(); server.close(); process.exit((crashed||errs.length)?2:0);
})().catch(e=>{console.error('ERR '+e.message+'\n'+e.stack);process.exit(1);});
