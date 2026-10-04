// Actual Retire/Yes season, natural arena, or natural first championship race.
// Clock/RNG inputs are explicit in --api-reference mode; no later engine writes.
// Passing this recorder validates API/input extent, not original image/PCM parity.
const assert=require('assert'),fs=require('fs'),path=require('path'),crypto=require('crypto');
const zlib=require('zlib');
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
const traceRng=process.argv.includes('--rng-call-trace');
assert(!traceRng||rngLayout,'--rng-call-trace requires --rng-layout');
const apiOption=process.argv.slice(4).find(value=>value.startsWith('--api-reference='));
let apiReference=null;
if(apiOption){
 const root=path.resolve(apiOption.slice('--api-reference='.length));assert(root.startsWith('/tmp/wasm-dd2/'));
 const meta=JSON.parse(fs.readFileSync(path.join(root,'history.json')));
 assert(meta.exe_modified===false&&meta.exe_sha256==='0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2');
 const normalArena=meta.normal_arena===true;
 const naturalChamp=meta.natural_championship===true;
 const naturalSeason=meta.natural_season===true;
 assert(!naturalSeason||naturalChamp&&meta.final_races.length===5,'Complete natural season required');
 assert(meta.acknowledged_keys&&(normalArena||naturalChamp?meta.natural_finish:meta.complete_retirement_season));
 const expectedKeys=normalArena?['Return','Right','Right','Return','Right','Return','Down','Down','Return']:['Return','Return','Return','Return','Up','Left','Return','Down','Down','Return'];
 if(naturalChamp)for(let race=0;race<(naturalSeason?5:1);race++)expectedKeys.push('Right','Return','Right','Right','Right','Right','Escape','Down','Down','Return');
 else if(!normalArena)for(let race=0;race<5;race++)expectedKeys.push('Escape','Down','Down','Down','Return','Up','Return','Right','Return','Right','Right','Right','Right','Escape','Down','Down','Return');
 assert.deepEqual(meta.keys,expectedKeys,'Actual complete championship input sequence required');
 if(naturalChamp){
  const actions=expectedKeys.slice(0,10);
  for(let race=0;race<(naturalSeason?5:1);race++)actions.push('natural-finish',...expectedKeys.slice(10+race*10,20+race*10));
  assert.deepEqual(meta.actions,actions);
 }
 assert.equal(meta.initial_save_sha256,crypto.createHash('sha256').update(save).digest('hex'));
 const ticks=fs.readFileSync(path.join(root,'ticks.bin')),random=fs.readFileSync(path.join(root,'random.bin'));
 assert.equal(ticks.length,meta.clock_calls*4);assert.equal(random.length,meta.rng_calls*12);
 assert(rngLayout&&rngLayout.clock_counter_address&&rngLayout.random_replay_counter_address,'API counter layout required');
 assert(!traceRng&&!timingOption,'Use API-input comparison separately from realtime debugger/timing diagnosis');
 apiReference={meta,ticks:ticks.toString('base64'),random:random.toString('base64'),
  seasonEnd:naturalSeason?JSON.parse(fs.readFileSync(path.join(root,meta.checkpoints.at(-1),'checkpoint.json'))):null,
  api_sha256:{clock:crypto.createHash('sha256').update(ticks).digest('hex'),random:crypto.createHash('sha256').update(random).digest('hex')}};
}
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
 const server=serve(build);await new Promise(resolve=>server.listen(0,resolve));let browser,tracer;
 const errors=[],errorDetails=[];
 try{
  browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:560}});
  page.on('pageerror',e=>{errors.push(e.message);errorDetails.push({message:e.message,stack:e.stack});});page.on('crash',()=>errors.push('page crashed'));
  if(apiReference)page.on('console',message=>{
   const text=message.text();fs.appendFileSync(path.join(output,'browser.log'),message.type()+': '+text+'\n');
   if(text.includes('program exited (with status: 1)')||/^\[(clock-replay|random-reference)\].*(requires|exhausted|partial|cannot|differs|invalid|unconsumed)/.test(text))errors.push(text);
  });
  await page.route('**/index.html*',route=>{
   const html=fs.readFileSync(path.join(build,'index.html'),'utf8');
   const marker='<script async type="text/javascript" src="index.js"></script>';assert(html.includes(marker));
   const hook='<script>Module.preRun.push(function(){var sync=FS.syncfs;FS.syncfs=function(populate,done){'+
    'return sync.call(FS,populate,function(error){if(populate&&!error)FS.writeFile("/persist/SaveGames",'+
    'Uint8Array.from(atob('+JSON.stringify(save.toString('base64'))+'),c=>c.charCodeAt(0)));done(error);});};});</script>';
   // Emscripten runs preRun callbacks in reverse registration order. Put this
   // first so it executes after the page's normal realtime configuration.
   const apiHook=apiReference?'<script>Module.preRun.unshift(function(){delete ENV.DD2_REALTIME;'+
    'FS.writeFile("/original-ticks.bin",Uint8Array.from(atob('+JSON.stringify(apiReference.ticks)+'),c=>c.charCodeAt(0)));'+
    'FS.writeFile("/original-random.bin",Uint8Array.from(atob('+JSON.stringify(apiReference.random)+'),c=>c.charCodeAt(0)));'+
    'ENV.DD2_TICK_REPLAY="/original-ticks.bin";ENV.DD2_RANDOM_REFERENCE="/original-random.bin";ENV.DD2_RANDOM_LEVEL="all";ENV.DD2_RANDOM_REQUIRE_INITIAL="1";});</script>':'';
   return route.fulfill({status:200,contentType:'text/html',body:html.replace(marker,hook+apiHook+marker)});
  });
  await page.addInitScript(({rngLayout,rngLimit,apiKeys,normalArena,naturalChamp,naturalSeason,seasonEnd,drivingInputs,fullVideo,firstPhase})=>{
   window.__slabReadyFrames=0;window.__releaseKey=null;window.__captureNext=false;
   window.__scheduledInput=null;window.__scheduleError=null;window.__inputObservations=[];window.__scheduledFrames=[];
   window.__rngObservations=[];window.__rngError=null;
   window.__stablePhysicsFrames=0;let previousPhysics=null;
   const seeds=[1];let previousRng=null;
   window.__apiHistory=apiKeys?{active:false,done:false,stage:fullVideo?'align':'settle',steady:0,index:0,shots:[],inputs:[],keys:apiKeys,error:null,raceFrames:[],presentations:[],drivingCursor:0,finalDrivers:[]}:null;
   window.__naturalPending=[];window.__captureYield=null;
   if(naturalChamp){
    const schedule=window.setTimeout;
    window.setTimeout=function(callback,delay,...args){
     const h=window.__apiHistory;
     // Hold the normal Asyncify yield callback, as an external observer would
     // stop the native debugger. No engine state or API returns are changed.
     if(h&&h.active&&(h.done||window.__naturalPending.length>=24)&&new Error().stack.includes('_emscripten_sleep')){
      window.__captureYield=()=>schedule(callback,delay,...args);
      return -1;
     }
     return schedule(callback,delay,...args);
    };
   }
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
     if(count>rngLimit)window.__rngError='RNG observation exceeded '+rngLimit+' calls';
     else{
      while(seeds.length<=count)seeds.push((Math.imul(seeds.at(-1),1103515245)+12345)>>>0);
      if(seed!==seeds[count])window.__rngError='Actual RNG seed differs from the LCG at the observed counter';
     }
     rngState={count,seed};
     if(count!==previousRng){window.__rngObservations.push({...rngState,level:HEAP32[0x936ff4>>2],cf:HEAP32[0x462ff0>>2],ticks:HEAP32[0x7746c0>>2],race:HEAP32[0x93dec8>>2]});previousRng=count;}
    }
    window.__slabReadyFrames=HEAP16[0x46996c>>1]===0?window.__slabReadyFrames+1:0;
    const history=window.__apiHistory;
    let recordHistory=false;
    if(history&&history.active&&history.stage==='align'&&physical.level===0&&HEAPU32[0x940010>>2]===0x4696b0&&HEAP32[0x4699cc>>2]===firstPhase){
     // Wait for the normal 64-frame blink cycle; never set its engine counter.
     history.stage='settle';history.steady=0;
    }
    const recordAll=history&&history.active&&fullVideo&&!history.done&&history.stage!=='align';
    const recordRace=history&&history.active&&(normalArena||naturalChamp)&&physical.level>=(naturalChamp?1:8)&&physical.level<=(naturalChamp&&!naturalSeason?7:12)&&physical.ticks>0&&physical.quit===0&&HEAPU32[rngLayout.clock_counter_address>>2]>0;
    if(recordRace||recordAll)window.__captureNext=true;
    if(history&&history.active&&!history.done){
     history.lastStack=new Error('Actual API-history presentation').stack;
     const masks=HEAPU16[0x754448>>1]|HEAPU16[0x75444a>>1];
     if(history.stage==='drive'&&physical.quit){
      if(naturalChamp){
       const readInt=address=>new DataView(HEAPU8.buffer).getInt32(address,true);
       history.finalDriver={speed:readInt(0x792a7a),heading:HEAPU16[0x78a792>>1]&4095,
        position:[readInt(0x78a744),readInt(0x78a74c)],lap:HEAPU16[0x795c48>>1],
        lap_progress:HEAPU16[0x795c4a>>1],dead:readInt(0x792ac6),
        planar_speed:readInt(0x792a76),finished_laps:HEAPU16[0x795c52>>1]};
       history.finalDrivers.push(history.finalDriver);
      }
      for(const code of naturalChamp?[]:['ArrowUp','ArrowRight']){
       window.dispatchEvent(new KeyboardEvent('keyup',{code}));history.inputs.push({action:history.index,code,down:false,level:physical.level,ticks:physical.ticks});
      }
      history.stage='settle';history.steady=0;
     }
     if(history.stage==='down'&&((masks&history.mask)||physical.level!==history.keyLevel)){
      window.dispatchEvent(new KeyboardEvent('keyup',{code:history.code}));
      history.inputs.push({action:history.index,code:history.code,down:false,level:physical.level,ticks:physical.ticks});
      history.stage='release';
     }else if(history.stage==='release'&&(masks&history.mask)===0){history.stage='settle';history.steady=0;}
     if(history.stage==='settle'){
      const go=naturalSeason?apiKeys[history.index]==='natural-finish'||history.index===apiKeys.length&&seasonEnd.level>0:naturalChamp?[10,21].includes(history.index):normalArena?history.index===apiKeys.length:history.index>=10&&history.index<=78&&(history.index-10)%17===0;
      const expectedLevel=naturalSeason?(history.index===apiKeys.length?seasonEnd.level:[1,2,5,7,10][(history.index-10)/11]):history.index===10?1:2;
      const resultAction=naturalChamp&&apiKeys[history.index-1]==='natural-finish';
      const resultScreen=naturalSeason&&history.index===55?0x46ae44:0x46bf38;
      const ready=go?(naturalChamp?physical.level===expectedLevel:normalArena?physical.level>=8&&physical.level<=12:physical.level===[1,2,5,7,10][(history.index-10)/17])&&physical.quit===0&&physical.ticks>0:HEAP16[0x46996c>>1]===0&&(!normalArena||history.index<=apiKeys.length||HEAPU32[0x940010>>2]===0x46a468)&&(!resultAction||HEAPU32[0x940010>>2]===resultScreen);
      history.steady=ready?history.steady+1:0;
      if(history.steady>=(go?2:16)){recordHistory=true;window.__captureNext=true;}
     }
    }
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
      countdown:HEAP32[0x784298>>2],frame_skip:HEAP32[0x7746b8>>2],finished:HEAP32[0x795df4>>2],retired:HEAP32[0x9376a8>>2],damage:new DataView(HEAPU8.buffer).getInt32(0x792a76,true),
      poly_list:HEAPU32[0x940010>>2],race_type:HEAP32[0x4673f4>>2],race_mode:HEAP32[0x4673f8>>2],
      race:HEAP32[0x93dec8>>2],season:HEAP32[0x93dec0>>2],num_races:HEAP32[0x467654>>2],
      division:HEAP32[0x46ad00>>2],stats:HEAP32[0x46741c>>2],actual_season:HEAP32[0x4682f4>>2],
      phase:HEAP32[0x4699cc>>2],retire_confirm:HEAP32[0x9376a8>>2],
      cars:Array.from({length:20},(_,i)=>({name:readText(0x93dee0+i*54,16),
       values:Array.from({length:7},(_,n)=>HEAP16[(0x93def0+i*54>>1)+n])})),
      rows:Array.from({length:5},(_,i)=>({name:readText(0x940290+i*26,26),points:readText(0x940240+i*16,16)})),
      framebuf:encode(0x700450,307200),palette:encode(0x700050,1024),rng:rngState,
      api_calls:apiKeys?{clock:HEAPU32[rngLayout.clock_counter_address>>2],random:HEAPU32[rngLayout.random_replay_counter_address>>2]}:null};
     window.__captureNext=false;
    }
    if(recordRace){
     if(naturalChamp){
      const frame=history.raceFrames.length,{framebuf,palette,...metadata}=window.__snapshot;
      history.raceFrames.push(metadata);window.__naturalPending.push({frame,framebuf,palette});
     }else history.raceFrames.push(window.__snapshot);
    }
    if(recordAll)history.presentations.push({...window.__snapshot,game:!!recordRace});
    if(naturalChamp&&history&&history.active&&!history.done){
     const frame=history.raceFrames.length-1;
     while(history.drivingCursor<drivingInputs.length){
      const event=drivingInputs[history.drivingCursor];
      if(event.frame<frame){history.error='Recorded driving presentation was skipped';break;}
      if(event.frame!==frame||(event.finish?!physical.quit:!recordRace))break;
      const code={a:'KeyA',z:'KeyZ',Left:'ArrowLeft',Right:'ArrowRight'}[event.key];
      if(!code){history.error='Unsupported original driving key';break;}
      history.inputs.push({action:history.index,code,down:event.down,frame,finish:!!event.finish,level:physical.level,ticks:physical.ticks});
      window.dispatchEvent(new KeyboardEvent(event.down?'keydown':'keyup',{code}));
      history.drivingCursor++;
     }
    }
    if(recordHistory){
     history.shots.push(window.__snapshot);
     if(naturalChamp&&apiKeys[history.index]==='natural-finish'){history.index++;history.stage='drive';history.steady=0;}
     else if(normalArena&&history.index===apiKeys.length){
      history.index++;history.stage='drive';history.steady=0;
      for(const code of ['ArrowUp','ArrowRight']){
       history.inputs.push({action:history.index,code,down:true,level:physical.level,ticks:physical.ticks});
       window.dispatchEvent(new KeyboardEvent('keydown',{code}));
      }
     }
     else if(history.index===history.keys.length+(normalArena?1:0)){history.done=true;}
     else{
      const key=history.keys[history.index++];
      history.code={Return:'Enter',Escape:'Escape',Up:'ArrowUp',Down:'ArrowDown',Left:'ArrowLeft',Right:'ArrowRight'}[key];
      history.keyLevel=physical.level;
      history.mask={Return:physical.level>=1&&physical.level<=12?1:0x4000,Escape:0x1008,Up:0x10,Down:0x40,Left:0x80,Right:0x20}[key];
      if(!history.code)history.error='Unknown reference key';
      else{
       history.stage='down';history.steady=0;
       history.inputs.push({action:history.index,code:history.code,down:true,level:physical.level,ticks:physical.ticks});
       window.dispatchEvent(new KeyboardEvent('keydown',{code:history.code}));
      }
     }
    }
    return result;
   };
  },{rngLayout,rngLimit:apiReference?apiReference.meta.rng_calls:100000,
     apiKeys:apiReference&&(apiReference.meta.natural_championship?apiReference.meta.actions:apiReference.meta.keys),normalArena:apiReference&&apiReference.meta.normal_arena,
     naturalChamp:apiReference&&apiReference.meta.natural_championship,drivingInputs:apiReference&&apiReference.meta.driving_inputs,
     naturalSeason:apiReference&&apiReference.meta.natural_season,seasonEnd:apiReference&&apiReference.seasonEnd,
     fullVideo:apiReference&&apiReference.meta.full_video,
     firstPhase:apiReference&&apiReference.meta.full_video?apiReference.meta.presentations[0].phase:null});
  await boot(page,server);
  if(rngLayout){
   const first=await page.evaluate(()=>window.__rngObservations[0]);
   assert(first&&first.count===0&&first.seed===1,'RNG layout did not observe the actual boot seed/counter');
  }
  assert(Buffer.from(await page.evaluate(()=>Array.from(Module.FS.readFile('/SaveGames')))).equals(save));
  if(apiReference){
   await page.evaluate(()=>{window.__apiHistory.active=true;});
   const streaming=apiReference.meta.natural_championship===true;
   let streamed=0,streamedBytes=0;
   const drain=async()=>{
    const batch=await page.evaluate(()=>window.__naturalPending.splice(0,16));
    for(const frame of batch){
     assert.equal(frame.frame,streamed,'Streaming race frame was skipped or reordered');
     const prefix='race'+String(streamed++).padStart(5,'0');
     const raw=Buffer.from(frame.framebuf,'base64'),palette=Buffer.from(frame.palette,'base64');
     assert.equal(raw.length,307200);assert.equal(palette.length,1024);
     const packed=zlib.deflateSync(raw,{level:6});
     streamedBytes+=packed.length+palette.length;
     // Reserve 64 MiB for navigation pictures, metadata and diagnostics.
     assert(streamedBytes<2*1024**3-64*1024**2,'Browser race capture exceeded its bounded output allowance');
     fs.writeFileSync(path.join(output,prefix+'.bin.z'),packed);
     fs.writeFileSync(path.join(output,prefix+'.pal'),palette);
    }
    await page.evaluate(()=>{
     if(!window.__apiHistory.done&&window.__captureYield&&window.__naturalPending.length<16){
      const resume=window.__captureYield;window.__captureYield=null;resume();
     }
    });
    return batch.length;
   };
   const deadline=Date.now()+(apiReference.meta.natural_season?7200000:streaming?900000:240000);let lastIndex=-1,lastChange=Date.now();
   while(true){
    if(streaming)await drain();
    const progress=await page.evaluate(layout=>{const h=window.__apiHistory;return {done:h.done,error:h.error||window.__rngError,index:h.index,stage:h.stage,steady:h.steady,checkpoints:h.shots.length,level:HEAP32[0x936ff4>>2],ticks:HEAP32[0x7746c0>>2],clock_calls:HEAPU32[layout.clock_counter_address>>2],random_calls:HEAPU32[layout.counter_address>>2],quit:HEAP32[0x7746ac>>2],pad:HEAPU32[0x754448>>2],last_stack:h.lastStack};},rngLayout);
    fs.writeFileSync(path.join(output,'api-progress.json'),JSON.stringify(progress,null,2));
    if(progress.error)throw new Error(progress.error);
    assert.deepEqual(errors,[],'Browser error during actual API-input sequence');
    if(progress.done)break;
    if(progress.index!==lastIndex||progress.stage==='drive'){lastIndex=progress.index;lastChange=Date.now();}
    if(Date.now()-lastChange>30000)throw new Error('Browser API-input sequence stopped advancing; see api-progress.json');
    if(Date.now()>=deadline)throw new Error('Browser API-input sequence timed out');
    await page.waitForTimeout(streaming?100:1000);
   }
   if(streaming){
    while(await drain()){}
    assert(await page.evaluate(()=>!!window.__captureYield),'Observer must stop at the normal final presentation yield');
   }
   assert.equal(await page.evaluate(()=>window.__apiHistory.error||window.__rngError),null);
   const total=await page.evaluate(()=>window.__apiHistory.shots.length);assert.equal(total,apiReference.meta.checkpoints.length);
   for(let i=0;i<total;i++){
    const name=apiReference.meta.checkpoints[i],directory=path.join(output,name);fs.mkdirSync(directory);
    const shot=await page.evaluate(i=>window.__apiHistory.shots[i],i);
    assert.equal(shot.canvas_mismatches,0);
    for(const [region,size] of [['framebuf',307200],['palette',1024]]){
     const bytes=Buffer.from(shot[region],'base64');assert.equal(bytes.length,size);
     fs.writeFileSync(path.join(directory,region+'.bin'),bytes);delete shot[region];
    }
    fs.writeFileSync(path.join(directory,'checkpoint.json'),JSON.stringify(shot,null,2));
   }
   if(apiReference.meta.normal_arena||apiReference.meta.natural_championship){
    for(const [property,stem,filename] of [['raceFrames','race','race-frames.json'],...(apiReference.meta.full_video?[['presentations','present','presentations.json']]:[])]){
     const n=await page.evaluate(property=>window.__apiHistory[property].length,property),frames=[];
     for(let i=0;i<n;i++){
      const shot=await page.evaluate(({property,i})=>window.__apiHistory[property][i],{property,i});assert.equal(shot.canvas_mismatches,0);
      const prefix=stem+String(i).padStart(5,'0');
      for(const [region,suffix,size] of streaming?[]:[['framebuf','bin',307200],['palette','pal',1024]]){
       const raw=Buffer.from(shot[region],'base64');assert.equal(raw.length,size);fs.writeFileSync(path.join(output,prefix+'.'+suffix),raw);delete shot[region];
      }
      frames.push({...shot,index:i,prefix});
     }
     fs.writeFileSync(path.join(output,filename),JSON.stringify(frames,null,2));
     if(streaming)assert.equal(n,streamed);
    }
   }
   const ended=JSON.parse(fs.readFileSync(path.join(output,apiReference.meta.checkpoints.at(-1),'checkpoint.json')));
   assert.equal(ended.api_calls.clock,apiReference.meta.clock_calls,'Actual clock-input extent differs');
   assert.equal(ended.api_calls.random,apiReference.meta.rng_calls,'Actual computed RNG-input extent differs');
   assert.equal(ended.rng.count,apiReference.meta.rng_calls);
   if(apiReference.meta.normal_arena){assert.equal(ended.poly_list,0x46a468);assert(ended.finished>14);assert.equal(ended.retired,0);}
   else if(apiReference.meta.natural_season){
    for(const key of ['level','race','season','poly_list'])assert.equal(ended[key],apiReference.seasonEnd[key],'Actual natural season ending differs: '+key);
    assert.equal(ended.retired,0);
    assert.equal(await page.evaluate(()=>window.__apiHistory.drivingCursor),apiReference.meta.driving_inputs.length);
    assert.deepEqual(await page.evaluate(()=>window.__apiHistory.finalDrivers),apiReference.meta.final_races.map(row=>row.driver));
   }
   else if(apiReference.meta.natural_championship){
    assert.equal(ended.level,2);assert.equal(ended.race,1);assert.equal(ended.retired,0);
    assert.equal(await page.evaluate(()=>window.__apiHistory.drivingCursor),apiReference.meta.driving_inputs.length);
   }
   else{
    assert.equal(ended.level,0);assert.equal(ended.race,5);assert.equal(ended.season,0);
    assert.equal(ended.cars[0].values[1],3);assert.equal(ended.cars[0].values[2],4);
   }
   assert.deepEqual(errors,[]);
   fs.writeFileSync(path.join(output,'navigation.json'),JSON.stringify({keys:apiReference.meta.keys,checkpoints:apiReference.meta.checkpoints,
    input:'browser keyboard events',initial_save_sha256:apiReference.meta.initial_save_sha256,wasm_sha256:rngLayout.wasm_sha256,
    normal_arena:apiReference.meta.normal_arena===true,
    natural_championship:apiReference.meta.natural_championship===true,
    natural_season:apiReference.meta.natural_season===true,
    final_driver:await page.evaluate(()=>window.__apiHistory.finalDriver||null),
    final_drivers:await page.evaluate(()=>window.__apiHistory.finalDrivers),
    actions:apiReference.meta.actions,
    racing_image_format:streaming?'indexed-zlib':'indexed-raw',
    observer_stop:streaming?'Normal Asyncify presentation yield callback held after the final checkpoint; engine state and API returns untouched':null,
    full_video:apiReference.meta.full_video===true,
    reference_presentation_boundary:apiReference.meta.presentation_boundary||null,
    scope:'Actual production WASM, autonomous normal DOM key sequence and original clock/RNG inputs; no later engine state injection or physical A/V parity claim',
    api_reference:{clock_calls:ended.api_calls.clock,computed_rng_calls:ended.api_calls.random,api_sha256:apiReference.api_sha256},
    input_observations:await page.evaluate(()=>window.__apiHistory.inputs)},null,2));
   fs.writeFileSync(path.join(output,'rng-observations.json'),JSON.stringify({layout:rngLayout,observations:await page.evaluate(()=>window.__rngObservations)},null,2));
   console.log('PASS actual browser '+(apiReference.meta.natural_championship?'natural first championship race':apiReference.meta.normal_arena?'natural arena':'complete championship')+' API/input history (image comparison separate):',ended.api_calls);
   return;
  }
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
   if(traceRng&&race<2){
    if(!tracer)tracer=await require('./wasm_rng_trace').createRngTrace(page,rngLayout);
    await tracer.arm(race+1);
   }
   const result=await tap(page,'Return');
   if(tracer&&race<2){
    await tracer.disarm();
    fs.writeFileSync(path.join(output,'rng-call-trace.json'),JSON.stringify(tracer.report(),null,2));
   }
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
  if(tracer)fs.writeFileSync(path.join(output,'rng-call-trace.json'),JSON.stringify(tracer.report(),null,2));
  if(browser){
   const pages=browser.contexts().flatMap(context=>context.pages());
   const observed=pages.length?await pages[0].evaluate(()=>({error:window.__scheduleError,inputs:window.__inputObservations,frames:window.__scheduledFrames,rng_error:window.__rngError,rng:window.__rngObservations,
    api:window.__apiHistory?{index:window.__apiHistory.index,stage:window.__apiHistory.stage,steady:window.__apiHistory.steady,checkpoints:window.__apiHistory.shots.length,inputs:window.__apiHistory.inputs}:null})).catch(()=>null):null;
   fs.writeFileSync(path.join(output,'diagnosis.json'),JSON.stringify({scope:'Failed actual UI/input diagnosis; no acceptance claim',error:{message:error.message,stack:error.stack},errors,errorDetails,referenceTiming,observed,
    wasm_sha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(build,'index.wasm'))).digest('hex')},null,2));
  }
  throw error;
 }finally{if(browser)await browser.close();await new Promise(resolve=>server.close(resolve));}
})().catch(error=>{console.error(error);process.exitCode=1;});
