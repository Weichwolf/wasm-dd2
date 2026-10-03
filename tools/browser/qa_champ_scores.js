// Reach championship results through actual selection, name entry and retirement.
// Wait for the slab transition to finish, then release navigation keys after
// the engine polls them. Animation frames poll but ignore menu actions;
// Pause_Mode reads held bits each iteration.
const assert=require('assert'),fs=require('fs'),path=require('path');
const {serve,boot,menuLabel,waitRace,rd,chromium}=require('./felib');
const output=process.argv[3] ? path.resolve(process.argv[3]) : fs.mkdtempSync('/tmp/dd2-champ-scores-');
if(fs.existsSync(output) && fs.readdirSync(output).length)throw new Error('Output directory must be empty; use a fresh capture directory');
fs.mkdirSync(output,{recursive:true});
async function tap(page,code,settle=450){
 const paused=await page.evaluate(()=>HEAPU8[0x460005]===89);
 if(!paused)await page.waitForFunction(()=>window.__slabReadyFrames>=16,null,{timeout:15000});
 const mask={Enter:paused?0x1:0x4000,Escape:0x1008,ArrowUp:0x10,ArrowDown:0x40,ArrowLeft:0x80,ArrowRight:0x20}[code];
 assert(mask,'unknown navigation key');
 // A previous held/pressed bit can persist throughout an animation. Wait for
 // a fresh control poll with that bit clear before pressing it again.
 await page.waitForFunction(mask=>((HEAPU16[0x754448>>1]|HEAPU16[0x75444a>>1])&mask)===0,mask,{timeout:15000});
 await page.evaluate(({code,mask})=>{
  window.__releaseKey={code,mask,level:HEAP32[0x936ff4>>2]};
  window.dispatchEvent(new KeyboardEvent('keydown',{code}));
 },{code,mask});
 await page.waitForFunction(()=>window.__releaseKey===null,null,{timeout:15000});
 await page.waitForTimeout(settle);
}
const state=page=>page.evaluate(()=>({type:HEAP32[0x4673f4>>2],level:HEAP32[0x936ff4>>2],cf:HEAP32[0x462ff0>>2],screen:HEAPU8[0x460005],stats:HEAP32[0x46741c>>2]}));
(async()=>{
 const server=serve(path.resolve(process.argv[2]||'web/dd2'));await new Promise(r=>server.listen(0,r));
 let browser, menuThrottle, watchdog;
 try{
  browser=await chromium.launch({args:['--no-sandbox']});
  watchdog=setTimeout(()=>{
   console.error('Championship navigation exceeded its five-minute deadline');
   process.exitCode=1;
   browser.close().catch(()=>{});
   setTimeout(()=>process.exit(1),5000).unref();
  },300000);
  const page=await browser.newPage({viewport:{width:700,height:560}}),errors=[];
  if(process.argv[4]==='--slow-menus'){
   menuThrottle=await page.context().newCDPSession(page);
   await menuThrottle.send('Emulation.setCPUThrottlingRate',{rate:3});
  }
  page.on('pageerror',e=>errors.push(e.message));
  await page.addInitScript(()=>{
   window.__releaseKey=null;
   window.__slabReadyFrames=0;
   const present=CanvasRenderingContext2D.prototype.putImageData;
   CanvasRenderingContext2D.prototype.putImageData=function(...args){
    const result=present.apply(this,args);
    if(this.canvas.id==='canvas' && typeof HEAP16!=='undefined'){
     // The on-transition ends with four zero-angle bounce samples; the
     // longest Button_Pressed sequence before an off-transition is eleven
     // frames. Sixteen steady face-on presentations exclude both intervals.
     window.__slabReadyFrames=HEAP16[0x46996c>>1]===0?window.__slabReadyFrames+1:0;
    }
    if(this.canvas.id==='canvas' && window.__releaseKey &&
       (((HEAPU16[0x754448>>1]|HEAPU16[0x75444a>>1])&window.__releaseKey.mask) ||
        HEAP32[0x936ff4>>2]!==window.__releaseKey.level)){
     // Retire/Yes restores the saved pad before returning; the next
     // presentation is already in the results level with that bit cleared.
     const {code}=window.__releaseKey;window.__releaseKey=null;
     window.dispatchEvent(new KeyboardEvent('keyup',{code}));
    }
    return result;
   };
  });
  await boot(page,server);
  console.log('Frontend ready');
  for(const code of ['Enter','Enter','Enter','Enter'])await tap(page,code,700);
  await page.waitForFunction(()=>HEAP16[0x469f34>>1]===48 && HEAP16[0x469f36>>1]===123,null,{timeout:15000});
  await tap(page,'ArrowUp',700);
  await page.waitForFunction(()=>HEAP16[0x469f36>>1]===191,null,{timeout:15000});
  await tap(page,'ArrowLeft',700);
  await page.waitForFunction(()=>HEAP16[0x469f34>>1]===256,null,{timeout:15000});
  await tap(page,'Enter',700);
  await page.waitForFunction(()=>HEAP32[0x4673f4>>2]===4,null,{timeout:15000});
  assert((await state(page)).type===4,'name confirmation did not select championship');
  console.log('Championship name confirmed');
  await tap(page,'ArrowDown');await tap(page,'ArrowDown');
  assert((await menuLabel(page)).includes('Go!'),'Go selection failed');
  // Stress the slab navigation separately from the realtime racing clock.
  if(menuThrottle)await menuThrottle.send('Emulation.setCPUThrottlingRate',{rate:1});
  await tap(page,'Enter',1500);
  assert((await waitRace(page,25000)).launched,'championship race did not start');
  assert((await state(page)).stats===1,'race did not initialize season statistics');
  console.log('Championship race started');
  await tap(page,'Escape');
  await page.screenshot({path:path.join(output,'paused.png')});
  for(const code of ['ArrowDown','ArrowDown','ArrowDown','Enter'])await tap(page,code);
  await page.screenshot({path:path.join(output,'retire-confirm.png')});
  console.log('Retire confirmation:',await state(page));
  assert(await page.evaluate(()=>HEAP32[0x9376a8>>2])===1,'Retire action was not activated');
  await tap(page,'ArrowUp');await tap(page,'Enter',1200);
  await page.screenshot({path:path.join(output,'after-retire.png')});
  console.log('After retire:',await state(page));
  await page.waitForFunction(()=>HEAP32[0x936ff4>>2]===15,null,{timeout:25000});
  await page.waitForTimeout(1000);
  await page.screenshot({path:path.join(output,'results.png')});
  console.log('Results:',await state(page));
  console.log('Result action:',await rd(page,0x46c01c));
  await tap(page,'ArrowRight');
  assert((await rd(page,0x46c01c)).includes('View League'),'View League action not reached');
  await tap(page,'Enter',800);
  const divisions=[];
  for(let division=0;division<4;division++){
   const scores=await page.evaluate(()=>{
    const read=a=>{let s='';for(let i=0;i<64;i++){const c=HEAPU8[a+i];if(!c)break;s+=String.fromCharCode(c);}return s;};
    return {division:HEAP32[0x46ad00>>2],rows:Array.from({length:5},(_,i)=>({name:read(0x940290+i*26),points:read(0x940240+i*16)}))};
   });
   console.log('Scores:',JSON.stringify(scores));divisions.push(scores);
   assert(scores.division===division,'division navigation failed');
   assert(scores.rows.every(row=>row.name.length>8 && /\d/.test(row.points)),'blank championship score rows');
   await page.screenshot({path:path.join(output,`division${division}.png`)});
   const pixels=await page.evaluate(()=>{
    const encode=(a,n)=>{let s='';for(let i=0;i<n;i+=16384)s+=String.fromCharCode(...HEAPU8.subarray(a+i,a+Math.min(i+16384,n)));return btoa(s);};
    return {framebuffer:encode(0x700450,307200),palette:encode(0x700050,1024)};
   });
   for(const [name,data] of Object.entries(pixels))fs.writeFileSync(path.join(output,`division${division}-${name}.bin`),Buffer.from(data,'base64'));
   await tap(page,'ArrowRight',600);
  }
  const image=await page.evaluate(()=>{
   let s='';const bytes=HEAPU8.subarray(0x400000,0x980400);
   for(let i=0;i<bytes.length;i+=16384)s+=String.fromCharCode(...bytes.subarray(i,i+16384));
   return btoa(s);
  });
  fs.writeFileSync(path.join(output,'image.bin'),Buffer.from(image,'base64'));
  fs.writeFileSync(path.join(output,'scores.json'),JSON.stringify(divisions,null,2));
  await tap(page,'Escape',700);
  assert((await rd(page,0x46c01c)).includes('View League'),'score viewer did not return to results menu');
  assert.deepEqual(errors,[],'browser runtime errors');
  console.log('PASS championship selection, real Retire/Yes and all four populated score divisions');
  console.log('Artifacts:',output);
 }finally{clearTimeout(watchdog);if(browser)await browser.close();await new Promise(r=>server.close(r));}
})().catch(e=>{console.error(e);process.exitCode=1;});
