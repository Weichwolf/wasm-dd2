// Observe a real player race until its natural end; no engine state writes.
// This is input/finish diagnosis, not original A/V acceptance.
const assert=require('assert'),fs=require('fs'),path=require('path'),crypto=require('crypto');
const {serve,boot,key,waitRace,chromium}=require('./felib');
const {createApiRecorder}=require('./wasm_api_record');
const build=path.resolve(process.argv[2]||'web/dd2'),out=path.resolve(process.argv[3]||'');
const driverSourceSha256=crypto.createHash('sha256').update(fs.readFileSync(__filename)).digest('hex');
assert(out.startsWith('/tmp/wasm-dd2/'));
assert(!fs.existsSync(out)||fs.readdirSync(out).length===0,'Fresh diagnostic directory required');
fs.mkdirSync(out,{recursive:true});
const save=fs.readFileSync(path.resolve(__dirname,'../../DestructionDerby2/SaveGames'));
const total=process.argv.includes('--total');
const championship=process.argv.includes('--championship');
const straight=process.argv.includes('--straight');
const steadyTrack=process.argv.includes('--steady-track');
const traceTraps=process.argv.includes('--trap-trace');
const apiLayoutArg=process.argv.find(a=>a.startsWith('--api-layout='));
const apiLayout=apiLayoutArg?JSON.parse(fs.readFileSync(path.resolve(apiLayoutArg.slice('--api-layout='.length)))):null;
const followTrack=process.argv.includes('--follow-track')||steadyTrack;
assert(!followTrack||championship,'Track following requires a road championship');
const durationArg=process.argv.find(a=>a.startsWith('--duration='));
const durationSeconds=durationArg?Number(durationArg.split('=')[1]):180;
assert(Number.isFinite(durationSeconds)&&durationSeconds>0&&durationSeconds<=600);
assert(!championship||!total,'Use championship or Total Destruction');
(async()=>{
 const server=serve(build);await new Promise(r=>server.listen(0,r));let browser,page,apiRecorder;
 const errors=[],errorDetails=[],consoleErrors=[],observations=[],inputs=[],traps=[];
 try{
  browser=await chromium.launch({args:['--no-sandbox']});
  page=await browser.newPage({viewport:{width:700,height:560}});
  await page.addInitScript(()=>{
   window.__diagnosticMenuReady=0;
   const present=CanvasRenderingContext2D.prototype.putImageData;
   CanvasRenderingContext2D.prototype.putImageData=function(...args){
    const result=present.apply(this,args);
    if(this.canvas.id==='canvas'&&typeof HEAP16!=='undefined'){
     window.__diagnosticMenuReady=HEAP32[0x936ff4>>2]===0&&HEAP16[0x46996c>>1]===0?window.__diagnosticMenuReady+1:0;
    }
    return result;
   };
  });
  if(apiLayout)apiRecorder=await createApiRecorder(page,apiLayout,build,out);
  page.on('pageerror',e=>{errors.push(e.message);errorDetails.push({message:e.message,stack:e.stack});});
  page.on('console',m=>{if(m.type()==='error'&&consoleErrors.length<50)consoleErrors.push(m.text());});
  const armTrapTrace=async()=>{if(traceTraps){
   const cdp=await page.context().newCDPSession(page);
   cdp.on('Debugger.paused',async event=>{
    try{
     assert(traps.length<10,'Bounded exception trace exceeded');
     const trap={reason:event.reason,data:event.data,frames:event.callFrames.map(frame=>({function:frame.functionName,location:frame.location})),locals:[]};
     traps.push(trap);
     for(const frame of event.callFrames.slice(0,3)){
      for(const scope of frame.scopeChain.filter(scope=>scope.type==='local')){
       const result=await cdp.send('Runtime.getProperties',{objectId:scope.object.objectId,ownProperties:true,generatePreview:true});
       trap.locals.push({location:frame.location,properties:result.result.slice(0,64)});
      }
     }
     fs.writeFileSync(path.join(out,'traps.json'),JSON.stringify({scope:'Read-only uncaught exception debugger stack/locals; no original parity claim',traps},null,2));
    }catch(error){consoleErrors.push('Trap observer: '+error.message);}
    finally{await cdp.send('Debugger.resume').catch(()=>{});}
   });
   await cdp.send('Debugger.enable');
   await cdp.send('Debugger.setPauseOnExceptions',{state:'uncaught'});
  }};
  await page.route('**/index.html*',route=>{
   const html=fs.readFileSync(path.join(build,'index.html'),'utf8'),marker='<script async type="text/javascript" src="index.js"></script>';
   assert(html.includes(marker));
   const hook='<script>Module.preRun.push(function(){var sync=FS.syncfs;FS.syncfs=function(populate,done){return sync.call(FS,populate,function(error){if(populate&&!error)FS.writeFile("/persist/SaveGames",Uint8Array.from(atob('+JSON.stringify(save.toString('base64'))+'),c=>c.charCodeAt(0)));done(error);});};});</script>';
   return route.fulfill({status:200,contentType:'text/html',body:html.replace(marker,hook+marker)});
  });
  const state=()=>page.evaluate(steadyTrack=>{
   const road=[];
   if(HEAP32[0x7746ac>>2]===0&&HEAP32[0x936ff4>>2]>=1&&HEAP32[0x936ff4>>2]<=7){
    const stripBase=HEAPU32[0x77cef8>>2],vertexBase=HEAPU32[0x77cef4>>2];
    let offset=HEAP32[0x7926a4>>2];
    for(let ahead=0;ahead<=(steadyTrack?12:6);ahead++){
     const strip=stripBase+4+offset,type=HEAPU8[strip],lanes=HEAPU8[strip+1],first=HEAPU16[(strip+16)>>1];
     const a=first+HEAP32[(0x463dcc+type*8)>>2],b=first+HEAP32[(0x463dd0+type*8)>>2]+lanes+1;
     const points=[a,a+lanes,b,b+lanes].map(i=>[HEAP32[(vertexBase+i*12)>>2],HEAP32[(vertexBase+i*12+8)>>2]]);
     road.push({offset,lanes,heading:(3072-HEAPU8[strip+5]*16)&4095,
      center:[points.reduce((n,p)=>n+p[0],0)/4,points.reduce((n,p)=>n+p[1],0)/4]});
     offset=HEAP32[(strip+20)>>2];
    }
   }
   return {road,level:HEAP32[0x936ff4>>2],ticks:HEAP32[0x7746c0>>2],cf:HEAP32[0x462ff0>>2],quit:HEAP32[0x7746ac>>2],
   mode:HEAP32[0x4673f8>>2],type:HEAP32[0x4673f4>>2],poly:HEAPU32[0x940010>>2],planar_speed:new DataView(HEAPU8.buffer).getInt32(0x792a76,true),
   dead:new DataView(HEAPU8.buffer).getInt32(0x792ac6,true),finished:HEAP32[0x795df4>>2],retire:HEAP32[0x9376a8>>2],position:[HEAP32[0x78a744>>2],HEAP32[0x78a74c>>2]],
   player:HEAP32[0x93ded0>>2],pad:HEAPU16[0x754448>>1],pad_copy:HEAPU16[0x75444c>>1],pad_type:HEAPU8[0x754451],
   throttle:new DataView(HEAPU8.buffer).getInt32(0x792a86+HEAP32[0x93ded0>>2]*434,true),
   speed:new DataView(HEAPU8.buffer).getInt32(0x792a7a+HEAP32[0x93ded0>>2]*434,true),
   countdown:HEAP32[0x784298>>2],heading:HEAPU16[(0x78a792+HEAP32[0x93ded0>>2]*636)>>1]&4095,
   steering:new DataView(HEAPU8.buffer).getInt32(0x792a82+HEAP32[0x93ded0>>2]*434,true),
   yaw_rate:new DataView(HEAPU8.buffer).getInt32(0x792a04,true)/8192,
   front_damage:new DataView(HEAPU8.buffer).getInt32(0x792aee,true),rear_damage:new DataView(HEAPU8.buffer).getInt32(0x792af6,true),
   finished_laps:HEAPU16[0x795c52>>1],
   required_laps:HEAPU16[(0x466df4+HEAP32[0x936ff4>>2]*6)>>1],
   track_strips:HEAPU16[(0x466df2+HEAP32[0x936ff4>>2]*6)>>1],
   race_position:HEAPU16[0x795c42>>1],race_points:HEAPU16[0x795c46>>1],
   lap:HEAP16[0x795c48>>1],lap_progress:HEAP16[0x795c4a>>1],strip:HEAP32[0x7926ac>>2],
   keymap:Array.from(HEAPU8.subarray(0x46302c,0x46302c+14))};},steadyTrack);
  const tap=async code=>{
   await page.waitForFunction(()=>window.__diagnosticMenuReady>=16,null,{timeout:30000});
   inputs.push({code,down:true,before:await state()});await key(page,code,900);inputs.push({code,down:false,after:await state()});
  };
  await boot(page,server);
  if(championship){
   for(const code of ['Enter','Enter','Enter','Enter','ArrowUp','ArrowLeft','Enter'])await tap(code);
  }else{
   await tap('Enter');await tap('ArrowRight');await tap('ArrowRight');await tap('Enter');
  }
  if(total){
   const label=()=>page.evaluate(()=>{const a=HEAPU32[0x46a6f0>>2];let s='';for(let i=0;i<80&&HEAPU8[a+i];i++)s+=String.fromCharCode(HEAPU8[a+i]);return s;});
   for(let i=0;i<4&&!/Total/i.test(await label());i++)await tap('ArrowRight');
   assert(/Total/i.test(await label()),'Total Destruction label missing');
  }
  if(!championship)await tap('Enter');
  const selected=await state();assert.equal(selected.mode,championship?0:2,'Wrong race mode');assert.equal(selected.type,championship?4:total?2:0);
  await tap('ArrowDown');await tap('ArrowDown');await tap('Enter');
  const started=await waitRace(page);assert(started.launched,'Player race did not start');assert(championship?started.lvl===1:started.lvl>=8);
  // Play_Game resets its frame/tick counters at green. Advancing countdown
  // physics alone does not show that the player's controls are being applied.
  await page.waitForFunction(()=>HEAP32[0x784298>>2]<0&&HEAP32[0x7746c0>>2]>0,null,{timeout:30000});
  await armTrapTrace();
  if(process.argv.includes('--probe-controls')){
   const controls=[];
   for(const code of ['KeyA','KeyZ','KeyA']){
    const before=await state();await page.keyboard.down(code);await page.waitForTimeout(2500);const held=await state();
    await page.keyboard.up(code);await page.waitForTimeout(300);controls.push({code,before,held,released:await state()});
   }
   for(const row of controls){
    const mask=row.code==='KeyA'?0x4000:0x8000;
    assert(row.before.countdown<0&&row.held.countdown<0,'Probe ran before green');
    assert.equal(row.held.pad_copy,mask,'Actual held gameplay pad differs');
    assert.equal(row.held.throttle,row.code==='KeyA'?32768:-32768,'Actual player throttle differs');
    assert.equal(row.released.pad_copy,0,'Gameplay pad did not consume the release');
    assert.equal(row.released.throttle,0,'Released throttle remained active');
   }
   assert(controls[0].held.speed>0&&controls[0].held.position.some((value,i)=>Math.abs(value-controls[0].before.position[i])>500),'Accelerator did not produce movement');
   assert.deepEqual(errors,[]);
   if(apiRecorder)await apiRecorder.flush();
   fs.writeFileSync(path.join(out,'controls.json'),JSON.stringify({scope:'Actual live trusted browser keyboard accelerator/reverse observation after green; no original parity claim',
    wasm_sha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(build,'index.wasm'))).digest('hex'),
    initial_save_sha256:crypto.createHash('sha256').update(save).digest('hex'),driver_source_sha256:driverSourceSha256,pass_:true,controls,errors},null,2));
   console.log(JSON.stringify({pass_:true,held:controls.map(row=>({code:row.code,pad:row.held.pad_copy,throttle:row.held.throttle,speed:row.held.speed})),errors}));return;
  }
  await page.screenshot({path:path.join(out,'race-start.png')});
  const driveCodes=straight||followTrack?['KeyA']:championship?['KeyA','ArrowRight']:['ArrowUp','ArrowRight'];
  const held=new Set(driveCodes);
  for(const code of driveCodes)await page.keyboard.down(code);
  inputs.push({code:driveCodes.join('+'),down:true,after:await state()});
  const deadline=Date.now()+durationSeconds*1000;
  let lastPosition=null,lastMovementTick=0,reverseUntil=0,lastProgress=null,lastProgressTick=0;
  while(Date.now()<deadline){
   const current=await state();observations.push(current);
   if(current.quit||current.level===15||current.level===0){
    fs.writeFileSync(path.join(out,'progress.json'),JSON.stringify({current,observations:observations.length,errors},null,2));break;
   }
   assert.deepEqual(errors,[]);
   if(followTrack){
    const firstAhead=Math.min(6,Math.max(2,Math.floor(Math.max(current.speed,0)/40)));
    const targets=steadyTrack?current.road.slice(firstAhead,firstAhead+5):[current.road[Math.min(6,Math.max(2,Math.floor(current.speed/100)))]];
    const target=[0,1].map(j=>targets.reduce((n,p)=>n+p.center[j],0)/targets.length);
    const dx=target[0]-current.position[0],dz=target[1]-current.position[1];
    const heading=Math.atan2(dx,dz)*4096/(2*Math.PI);
    const difference=((heading-current.heading+6144)%4096)-2048;
    current.target_heading=heading;current.heading_error=difference;
    const want=new Set();
    if(current.speed<(steadyTrack?250:400))want.add('KeyA');
    // The engine's positive steering rotates towards decreasing actor yaw.
    if(steadyTrack){
     current.target_strips=[firstAhead,firstAhead+4];
     const steering=Math.max(-192,Math.min(192,-difference*0.6+current.yaw_rate*6));
     current.target_steering=steering;
     if(current.steering>steering+40)want.add('ArrowLeft');else if(current.steering<steering-40)want.add('ArrowRight');
    }else if(difference>70)want.add('ArrowLeft');else if(difference< -70)want.add('ArrowRight');
    if(steadyTrack){
     if(!lastPosition||current.position.reduce((n,value,i)=>n+Math.abs(value-lastPosition[i]),0)>500){
      lastPosition=current.position;lastMovementTick=current.ticks;
     }
     const progress=current.strip;
     const advance=lastProgress===null?0:(progress-lastProgress+current.track_strips)%current.track_strips;
     if(lastProgress===null||advance>0&&advance<current.track_strips/2){
      lastProgressTick=current.ticks;
     }
     lastProgress=progress;
     if(current.ticks>=reverseUntil&&(current.ticks-lastMovementTick>=75||current.ticks-lastProgressTick>=200)){
      reverseUntil=current.ticks+100;lastMovementTick=reverseUntil;lastProgressTick=reverseUntil;
     }
     if(current.ticks<reverseUntil){
      want.clear();want.add('KeyZ');want.add(difference>0?'ArrowRight':'ArrowLeft');current.manoeuvre='reverse';
     }else current.manoeuvre='forward';
     const backwards=current.speed< -5||Math.abs(current.speed)<=5&&current.ticks<reverseUntil;
     const limit=backwards?96:192;
     let steering=-difference*0.6+current.yaw_rate*6;
     if(backwards)steering=-steering;
     current.target_steering=Math.max(-limit,Math.min(limit,steering));
     current.steering_direction=backwards?'backwards':'forwards';
     want.delete('ArrowLeft');want.delete('ArrowRight');
     const tolerance=backwards||current.speed<40?24:40;
     if(current.steering>current.target_steering+tolerance)want.add('ArrowLeft');
     else if(current.steering<current.target_steering-tolerance)want.add('ArrowRight');
    }
    for(const code of held)if(!want.has(code)){await page.keyboard.up(code);inputs.push({code,down:false,after:current});held.delete(code);}
   for(const code of want)if(!held.has(code)){await page.keyboard.down(code);inputs.push({code,down:true,before:current});held.add(code);}
   }
   fs.writeFileSync(path.join(out,'progress.json'),JSON.stringify({current,observations:observations.length,errors},null,2));
   await page.waitForTimeout(steadyTrack?50:followTrack?100:1000);
  }
  for(const code of held)await page.keyboard.up(code);
  inputs.push({code:driveCodes.join('+'),down:false,after:await state()});
  await page.waitForTimeout(2000);const ended=await state();
  await page.screenshot({path:path.join(out,'race-end.png')});
  const natural=observations.some(row=>row.quit===1&&row.finished>14&&row.retire===0);
  if(apiRecorder)await apiRecorder.flush();
  fs.writeFileSync(path.join(out,'diagnosis.json'),JSON.stringify({scope:'Actual live production browser player inputs and natural finish diagnosis; no original A/V parity claim',
   wasm_sha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(build,'index.wasm'))).digest('hex'),initial_save_sha256:crypto.createHash('sha256').update(save).digest('hex'),
   driver_source_sha256:driverSourceSha256,natural_finish:natural,started,ended,inputs,observations,errors,errorDetails,traps},null,2));
  console.log(JSON.stringify({natural_finish:natural,ended,errors}));
 }catch(error){
  if(apiRecorder)await apiRecorder.flush().catch(e=>consoleErrors.push('API recording: '+e.message));
  fs.writeFileSync(path.join(out,'failure.json'),JSON.stringify({scope:'Failed live browser driving diagnosis; no original parity or completed race acceptance',
   error:{message:error.message,stack:error.stack},errors,errorDetails,consoleErrors,traps,inputs,observations,
   wasm_sha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(build,'index.wasm'))).digest('hex'),
   initial_save_sha256:crypto.createHash('sha256').update(save).digest('hex'),driver_source_sha256:driverSourceSha256},null,2));
  if(page)await page.screenshot({path:path.join(out,'failure.png'),timeout:5000}).catch(()=>{});
  throw error;
 }finally{if(browser)await browser.close();await new Promise(r=>server.close(r));}
})().catch(e=>{if(!fs.existsSync(path.join(out,'failure.json')))fs.writeFileSync(path.join(out,'failure.json'),JSON.stringify({error:e.message},null,2));console.error(e);process.exitCode=1;});
