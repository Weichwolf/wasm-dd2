// Actual five-race Retire/Yes championship path. No engine state writes.
// This is the retirement case, not normal race completion or full A/V parity.
const assert=require('assert'),fs=require('fs'),path=require('path'),crypto=require('crypto');
const {serve,boot,chromium}=require('./felib');
const output=path.resolve(process.argv[3]||'');
assert(output.startsWith('/tmp/wasm-dd2/'),'captures must be under /tmp/wasm-dd2/');
assert(!fs.existsSync(output)||!fs.readdirSync(output).length,'use a fresh output directory');
fs.mkdirSync(output,{recursive:true});
const build=path.resolve(process.argv[2]||'web/dd2');
const save=fs.readFileSync(path.resolve(__dirname,'../../DestructionDerby2/SaveGames'));
const timingOption=process.argv.slice(4).find(value=>value.startsWith('--reference='));
const rngOption=process.argv.slice(4).find(value=>value.startsWith('--rng-layout='));
const rngLayout=rngOption?JSON.parse(fs.readFileSync(path.resolve(rngOption.slice('--rng-layout='.length)))):null;
if(rngLayout)assert.equal(rngLayout.wasm_sha256,crypto.createHash('sha256').update(fs.readFileSync(path.join(build,'index.wasm'))).digest('hex'),'RNG layout belongs to another binary');
let referenceTiming=null;
if(timingOption){
 const root=path.resolve(timingOption.slice('--reference='.length));assert(root.startsWith('/tmp/wasm-dd2/'));
 const nav=JSON.parse(fs.readFileSync(path.join(root,'navigation.json')));assert(nav.acknowledged_keys&&nav.input==='real X11 keys');
 assert.equal(nav.initial_save_sha256,crypto.createHash('sha256').update(save).digest('hex'));
 referenceTiming=Array.from({length:5},(_,race)=>{
  const index=11+17*race,directory=path.join(root,`step${String(index).padStart(2,'0')}-Escape`);
  const meta=JSON.parse(fs.readFileSync(path.join(directory,'checkpoint.json')));
  assert.equal(meta.exe_modified,false);assert.equal(meta.exe_sha256,'0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2');
  const image=fs.readFileSync(path.join(directory,'image.bin'));
  return {level:image.readInt32LE(0x936ff4-0x400000),cf:image.readInt32LE(0x462ff0-0x400000),ticks:image.readInt32LE(0x7746c0-0x400000)};
 });
}
const codes={Return:'Enter',Up:'ArrowUp',Down:'ArrowDown',Left:'ArrowLeft',Right:'ArrowRight',Escape:'Escape'};
const keys=[],checkpoints=[];
let index=0;
async function capture(page,key){
 const name=`step${String(index++).padStart(2,'0')}-${key||'boot'}`,directory=path.join(output,name);
 fs.mkdirSync(directory);
 await page.evaluate(()=>{window.__snapshot=null;window.__captureNext=true;});
 await page.waitForFunction(()=>!window.__captureNext,null,{timeout:10000});
 const shot=await page.evaluate(()=>window.__snapshot);
 assert.equal(shot.canvas_mismatches,0,'actual canvas differs from indexed framebuffer');
 for(const [region,size] of [['framebuf',307200],['palette',1024]]){
  const value=Buffer.from(shot[region],'base64');assert.equal(value.length,size);
  fs.writeFileSync(path.join(directory,region+'.bin'),value);delete shot[region];
 }
 fs.writeFileSync(path.join(directory,'checkpoint.json'),JSON.stringify(shot,null,2));
 checkpoints.push(name);return shot;
}
async function tap(page,key,timing=null,raceStart=null){
 const level=await page.evaluate(()=>HEAP32[0x936ff4>>2]);
 const racing=level>=1&&level<=12;
 if(!racing)await page.waitForFunction(()=>window.__slabReadyFrames>=16,null,{timeout:15000});
 const mask={Return:racing?1:0x4000,Escape:0x1008,Up:0x10,Down:0x40,Left:0x80,Right:0x20}[key];
 await page.waitForFunction(mask=>((HEAPU16[0x754448>>1]|HEAPU16[0x75444a>>1])&mask)===0,mask,{timeout:15000});
 await page.evaluate(({code,mask,timing})=>{
  const event={code,mask,level:HEAP32[0x936ff4>>2]};
  if(timing){
   if(HEAP32[0x462ff0>>2]>=timing.cf)throw new Error('Observed original pause point already passed');
   window.__scheduledInput={event,cf:timing.cf-1};
  }else{window.__releaseKey=event;window.dispatchEvent(new KeyboardEvent('keydown',{code}));}
 },{code:codes[key],mask,timing});
 if(timing){
  await page.waitForFunction(()=>window.__scheduledInput===null||window.__scheduleError,null,{timeout:15000});
  assert.equal(await page.evaluate(()=>window.__scheduleError),null,'original input point was skipped');
 }
 await page.waitForFunction(()=>window.__releaseKey===null,null,{timeout:15000});
 await page.waitForTimeout(700);
 if(raceStart!==null)await page.waitForFunction(level=>HEAP32[0x936ff4>>2]===level&&HEAP32[0x7746ac>>2]===0&&HEAP32[0x7746c0>>2]>0,raceStart,{timeout:15000});
 if(key==='Escape'&&racing)await page.waitForFunction(()=>window.__stablePhysicsFrames>=16&&HEAP32[0x7746ac>>2]===0&&HEAP32[0x7746c0>>2]>0,null,{timeout:15000});
 keys.push(key);
 const shot=await capture(page,key);
 if(timing)assert.equal(shot.cf,timing.cf,'pause did not occur at the observed original frame counter');
 return shot;
}
(async()=>{
 const server=serve(build);await new Promise(resolve=>server.listen(0,resolve));let browser;
 try{
  browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:560}}),errors=[];
  page.on('pageerror',e=>errors.push(e.message));page.on('crash',()=>errors.push('page crashed'));
  await page.route('**/index.html*',route=>{
   const html=fs.readFileSync(path.join(build,'index.html'),'utf8');
   const marker='<script async type="text/javascript" src="index.js"></script>';assert(html.includes(marker));
   const hook='<script>Module.preRun.push(function(){var sync=FS.syncfs;FS.syncfs=function(populate,done){'+
    'return sync.call(FS,populate,function(error){if(populate&&!error)FS.writeFile("/persist/SaveGames",'+
    'Uint8Array.from(atob('+JSON.stringify(save.toString('base64'))+'),c=>c.charCodeAt(0)));done(error);});};});</script>';
   return route.fulfill({status:200,contentType:'text/html',body:html.replace(marker,hook+marker)});
  });
  await page.addInitScript(rngLayout=>{
   window.__slabReadyFrames=0;window.__releaseKey=null;window.__captureNext=false;
   window.__scheduledInput=null;window.__scheduleError=null;window.__inputObservations=[];window.__scheduledFrames=[];
   window.__rngObservations=[];window.__rngError=null;
   window.__stablePhysicsFrames=0;let previousPhysics=null;
   const seeds=[1];let previousRng=null;
   const present=CanvasRenderingContext2D.prototype.putImageData;
   const readText=(a,n)=>{let text='';for(let i=0;i<n&&HEAPU8[a+i];i++)text+=String.fromCharCode(HEAPU8[a+i]);return text;};
   const encode=(a,n)=>{let text='';for(let i=0;i<n;i+=16384)text+=String.fromCharCode(...HEAPU8.subarray(a+i,a+Math.min(i+16384,n)));return btoa(text);};
   CanvasRenderingContext2D.prototype.putImageData=function(...args){
    const result=present.apply(this,args);
    if(this.canvas.id!=='canvas'||typeof HEAP16==='undefined')return result;
    const physical={level:HEAP32[0x936ff4>>2],ticks:HEAP32[0x7746c0>>2],quit:HEAP32[0x7746ac>>2]};
    window.__stablePhysicsFrames=physical.level>=1&&physical.level<=12&&physical.ticks>0&&physical.quit===0&&previousPhysics&&previousPhysics.level===physical.level&&previousPhysics.ticks===physical.ticks?window.__stablePhysicsFrames+1:0;
    previousPhysics=physical;
    let rngState=null;
    if(rngLayout){
     const count=HEAPU32[rngLayout.counter_address>>2],seed=HEAPU32[rngLayout.seed_address>>2];
     if(count>100000)window.__rngError='RNG observation exceeded 100,000 calls';
     else{
      while(seeds.length<=count)seeds.push((Math.imul(seeds.at(-1),1103515245)+12345)>>>0);
      if(seed!==seeds[count])window.__rngError='Actual RNG seed differs from the LCG at the observed counter';
     }
     rngState={count,seed};
     if(count!==previousRng){window.__rngObservations.push({...rngState,level:HEAP32[0x936ff4>>2],cf:HEAP32[0x462ff0>>2],ticks:HEAP32[0x7746c0>>2],race:HEAP32[0x93dec8>>2]});previousRng=count;}
    }
    window.__slabReadyFrames=HEAP16[0x46996c>>1]===0?window.__slabReadyFrames+1:0;
    if(window.__scheduledInput){
     const pending=window.__scheduledInput,cf=HEAP32[0x462ff0>>2];
     const observation={level:HEAP32[0x936ff4>>2],cf,ticks:HEAP32[0x7746c0>>2],frame_skip:HEAP32[0x7746b8>>2]};
     window.__scheduledFrames.push(observation);
     if(observation.level!==pending.event.level||cf>pending.cf)window.__scheduleError={expected_cf:pending.cf,actual:observation};
     else if(cf===pending.cf){
      window.__scheduledInput=null;window.__releaseKey=pending.event;
      window.__inputObservations.push({code:pending.event.code,level:pending.event.level,cf,ticks:HEAP32[0x7746c0>>2]});
      window.dispatchEvent(new KeyboardEvent('keydown',{code:pending.event.code}));
     }
    }
    if(window.__releaseKey&&(((HEAPU16[0x754448>>1]|HEAPU16[0x75444a>>1])&window.__releaseKey.mask)||HEAP32[0x936ff4>>2]!==window.__releaseKey.level)){
     const {code}=window.__releaseKey;window.__releaseKey=null;
     window.dispatchEvent(new KeyboardEvent('keyup',{code}));
    }
    if(window.__captureNext){
     const rgba=this.getImageData(0,0,640,480).data;let mismatches=0;
     for(let i=0;i<307200;i++){
      const c=0x700050+HEAPU8[0x700450+i]*4,p=i*4;
      if(rgba[p]!==HEAPU8[c]||rgba[p+1]!==HEAPU8[c+1]||rgba[p+2]!==HEAPU8[c+2]||rgba[p+3]!==255)mismatches++;
     }
     window.__snapshot={stage:'browser platform present',canvas_mismatches:mismatches,
      level:HEAP32[0x936ff4>>2],cf:HEAP32[0x462ff0>>2],ticks:HEAP32[0x7746c0>>2],quit:HEAP32[0x7746ac>>2],
      poly_list:HEAPU32[0x940010>>2],race_type:HEAP32[0x4673f4>>2],race_mode:HEAP32[0x4673f8>>2],
      race:HEAP32[0x93dec8>>2],season:HEAP32[0x93dec0>>2],num_races:HEAP32[0x467654>>2],
      division:HEAP32[0x46ad00>>2],stats:HEAP32[0x46741c>>2],actual_season:HEAP32[0x4682f4>>2],
      phase:HEAP32[0x4699cc>>2],retire_confirm:HEAP32[0x9376a8>>2],
      cars:Array.from({length:20},(_,i)=>({name:readText(0x93dee0+i*54,16),
       values:Array.from({length:7},(_,n)=>HEAP16[(0x93def0+i*54>>1)+n])})),
      rows:Array.from({length:5},(_,i)=>({name:readText(0x940290+i*26,26),points:readText(0x940240+i*16,16)})),
      framebuf:encode(0x700450,307200),palette:encode(0x700050,1024),rng:rngState};
     window.__captureNext=false;
    }
    return result;
   };
  },rngLayout);
  await boot(page,server);
  if(rngLayout){
   const first=await page.evaluate(()=>window.__rngObservations[0]);
   assert(first&&first.count===0&&first.seed===1,'RNG layout did not observe the actual boot seed/counter');
  }
  assert(Buffer.from(await page.evaluate(()=>Array.from(Module.FS.readFile('/SaveGames')))).equals(save));
  await capture(page,null);
  for(const key of ['Return','Return','Return','Return','Up','Left','Return','Down','Down'])await tap(page,key);
  let previous=Array(20).fill(0);
  const races=[];
  for(let race=0;race<5;race++){
   const started=await tap(page,'Return',null,[1,2,5,7,10][race]);
   assert.equal(started.level,[1,2,5,7,10][race],'wrong championship race');assert.equal(started.race,race);
   assert.equal(started.quit,0);assert(started.ticks>0,'captured loading instead of actual gameplay');
   await tap(page,'Escape',referenceTiming&&referenceTiming[race]);
   if(process.argv.includes('--stop-after-pause')){
    fs.writeFileSync(path.join(output,'diagnosis.json'),JSON.stringify({scope:'Focused actual pause-input scheduling diagnosis; not full-season acceptance',
     referenceTiming,observed:await page.evaluate(()=>({inputs:window.__inputObservations,frames:window.__scheduledFrames}))},null,2));
    return;
   }
   for(const key of ['Down','Down','Down'])await tap(page,key);
   const confirmation=await tap(page,'Return');assert.equal(confirmation.retire_confirm,1,'Retire confirmation was not selected');
   await tap(page,'Up');
   const result=await tap(page,'Return');
   assert.equal(result.level,15);assert.equal(result.race,race+1);assert.equal(result.num_races,5);
   assert.equal(result.poly_list,race<4?0x46bf38:0x46ae44,'wrong result/end-of-season screen');
   assert.equal(result.stats,1,'season statistics not initialized');
   result.cars.forEach((car,i)=>assert.equal(car.values[0],previous[i]+car.values[6],'cumulative championship points'));
   previous=result.cars.map(car=>car.values[0]);
   await tap(page,'Right');await tap(page,'Return');
   const divisions=[];
   for(let division=0;division<4;division++){
    const shot=JSON.parse(fs.readFileSync(path.join(output,checkpoints.at(-1),'checkpoint.json')));
    assert.equal(shot.poly_list,0x46ab30);assert.equal(shot.division,division);
    shot.rows.forEach((row,rank)=>{
     const car=shot.cars.find(car=>car.values[1]===division&&car.values[2]===rank);assert(car,'missing league/rank');
     assert.equal(row.name,'%R%JL%T/'+car.name);assert.equal(row.points,'%R%JL%T/'+car.values[0]);
    });
    divisions.push({checkpoint:checkpoints.at(-1),rows:shot.rows});await tap(page,'Right');
   }
   await tap(page,'Escape');await tap(page,'Down');await tap(page,'Down');
   races.push({race:race+1,level:started.level,points:previous,divisions});
   console.log('Championship completed race',race+1,'of 5');
  }
  const ended=await tap(page,'Return');
  assert.equal(ended.level,0);assert.equal(ended.poly_list,0x4696b0,'elimination did not return to frontend');
  assert.equal(ended.race,5);assert.equal(ended.season,0);
  assert.equal(ended.cars[0].values[1],3);assert.equal(ended.cars[0].values[2],4);
  assert.deepEqual(errors,[],'browser errors');
  if(rngLayout){
   assert.equal(await page.evaluate(()=>window.__rngError),null,'RNG observer failed');
   fs.writeFileSync(path.join(output,'rng-observations.json'),JSON.stringify({scope:'Read-only actual production WASM seed/counter observations; no causal caller trace or parity claim',layout:rngLayout,observations:await page.evaluate(()=>window.__rngObservations)},null,2));
  }
  fs.writeFileSync(path.join(output,'navigation.json'),JSON.stringify({keys,checkpoints,input:'browser keyboard events',
   initial_save_sha256:crypto.createHash('sha256').update(save).digest('hex'),
   wasm_sha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(build,'index.wasm'))).digest('hex'),
   reference_pause_timing:referenceTiming,input_observations:await page.evaluate(()=>window.__inputObservations),
   scope:'Complete five-race retirement championship UI; actual canvas checked; chronological racing video/audio and normal race completion excluded'},null,2));
  fs.writeFileSync(path.join(output,'season.json'),JSON.stringify({pass_:true,races,ended},null,2));
  console.log('PASS five actual Retire/Yes races, 100 league rows, cumulative points and elimination');
 }catch(error){
  if(browser){
   const pages=browser.contexts().flatMap(context=>context.pages());
   const observed=pages.length?await pages[0].evaluate(()=>({error:window.__scheduleError,inputs:window.__inputObservations,frames:window.__scheduledFrames,rng_error:window.__rngError,rng:window.__rngObservations})).catch(()=>null):null;
   fs.writeFileSync(path.join(output,'diagnosis.json'),JSON.stringify({scope:'Failed actual UI/input diagnosis; no acceptance claim',error:error.message,referenceTiming,observed},null,2));
  }
  throw error;
 }finally{if(browser)await browser.close();await new Promise(resolve=>server.close(resolve));}
})().catch(error=>{console.error(error);process.exitCode=1;});
