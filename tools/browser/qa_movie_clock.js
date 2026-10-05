// Run unchanged production imports with real AudioBuffer/Context/Source nodes
// and declared render/output clock positions. This is a component regression;
// independent unmuted device captures establish the actual timing diagnosis.
const fs=require('fs'),path=require('path'),assert=require('assert');
const {serve,chromium}=require('./felib');
const root=path.resolve(process.argv[2]);
(async()=>{
 const server=serve(root);await new Promise(r=>server.listen(0,r));let browser;const cases=[];
 try{
  browser=await chromium.launch({args:['--no-sandbox']});
  for(const program of ['production','before']){
   const page=await browser.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));
   await page.goto(`http://localhost:${server.address().port}/${program}.html`);
   await page.waitForFunction(()=>window.fixtureReady);await page.click('button');
   const result=await page.evaluate(async()=>{
    const p=Module._malloc(22050*4);for(let i=0;i<44100;i++)HEAP16[(p>>1)+i]=(i%65536)-32768;
    let scheduled,exact=false,state='running';
    const NativeContext=AudioContext;
    window.AudioContext=class extends NativeContext{
     constructor(...args){super(...args);Object.defineProperty(this,'currentTime',{configurable:true,get:()=>1024/22050});
      Object.defineProperty(this,'state',{get:()=>state,configurable:true});}
    };
    const original=AudioContext.prototype.createBufferSource;
    AudioContext.prototype.createBufferSource=function(...args){
     const source=original.apply(this,args),start=source.start;
     source.start=function(time){
      scheduled=time;const b=source.buffer;exact=b.length===22050&&b.numberOfChannels===2&&b.sampleRate===22050;
      for(let c=0;c<2;c++)for(let i=0;i<22050;i++)
       if(b.getChannelData(c)[i]!==HEAP16[(p>>1)+i*2+c]/32768)exact=false;
      return start.call(this,time);
     };return source;
    };
    const opened=Module._dd2_movie_audio_start(p,22050,22050,2),ac=Module._dd2movieAc;
    const now=performance.now;let perf=123456.789;performance.now=()=>perf;
    let rendered=scheduled+.5,output={contextTime:0,performanceTime:0};
    Object.defineProperty(ac,'currentTime',{get:()=>rendered});
    Object.defineProperty(ac,'state',{get:()=>state,configurable:true});
    ac.getOutputTimestamp=()=>output;
    const rows=[];
    function read(phase){perf+=40.0005;rows.push({phase,performance_ms:perf,rendered,output:{...output},state,clock:Module._dd2_movie_now_ms(),microseconds:Module._dd2_movie_now_us?Module._dd2_movie_now_us():null,done:Module._dd2_movie_audio_done()});}
    read('no-output');
    output={contextTime:0,performanceTime:performance.now()-100};read('zero-output-position');
    output={contextTime:.00005,performanceTime:performance.now()-100};read('sub-millisecond-startup-position');
    output={contextTime:scheduled+.1,performanceTime:performance.now()-10};read('output-behind-render');
    Module._dd2movieSource.onended();read('render-ended-before-output');
    rendered=scheduled+2;output={contextTime:scheduled+.9,performanceTime:performance.now()};read('source-still-queued');
    output={contextTime:.2,performanceTime:performance.now()};read('old-output-timestamp');
    state='suspended';output={contextTime:scheduled+1.1,performanceTime:performance.now()};read('suspended-output');
    state='running';output={contextTime:scheduled+1.1,performanceTime:performance.now()};read('source-consumed');
    Module._dd2_movie_audio_stop();Module._dd2_movie_audio_stop();
    delete ac.state;
    const stopCleared=!Module._dd2movieAc&&!Module._dd2movieSource&&!Module._dd2movieOutputTime&&!Module._dd2movieVideoTime;
    let activation=null;
    if(Module._dd2movieVideoTime===null){
     state='suspended';perf=222222.222;
     assertStart(Module._dd2_movie_audio_start(p,22050,22050,2));
     const initial=Module._dd2_movie_now_us();perf+=2000;
     const held=Module._dd2_movie_now_us();state='running';
     const activated=Module._dd2_movie_now_us();perf+=40.5;
     const advanced=Module._dd2_movie_now_us();state='suspended';perf+=750;
     const laterSuspended=Module._dd2_movie_now_us();
     activation={initial,held,activated,advanced,laterSuspended};
     Module._dd2_movie_audio_stop();state='running';
    }
    function assertStart(result){if(result!==0)throw Error('declared activation audio start failed');}
    performance.now=()=>2**32+123.75;
    const wrappedFallback=Module._dd2_movie_now_ms();
    const preciseFallback=Module._dd2_movie_now_us?Module._dd2_movie_now_us():null;
    performance.now=()=>123456.789;
    const fractionalFallback=Module._dd2_movie_now_us?Module._dd2_movie_now_us():null;
    let drainTimer=null;
    if(Module._dd2_movie_drain_start){
     const begin=now.call(performance),fixed=Module._dd2_movie_now_us();
     Module._dd2_movie_drain_start();const immediate=Module._dd2_movie_drain_ready();
     await new Promise(resolve=>setTimeout(resolve,120));
     const ready=Module._dd2_movie_drain_ready(),elapsed=now.call(performance)-begin;
     const frozenClock=Module._dd2_movie_now_us()===fixed;
     Module._dd2_movie_drain_start();Module._dd2_movie_drain_cancel();
     await new Promise(resolve=>setTimeout(resolve,110));const cancelled=Module._dd2_movie_drain_ready();
     Module._dd2_movie_drain_start();await new Promise(resolve=>setTimeout(resolve,60));
     Module._dd2_movie_drain_start();const rearmBegin=now.call(performance);
     await new Promise(resolve=>setTimeout(resolve,60));
     const beforeRearmDeadline=Module._dd2_movie_drain_ready(),rearmElapsed=now.call(performance)-rearmBegin;
     await new Promise(resolve=>setTimeout(resolve,50));const afterRearmDeadline=Module._dd2_movie_drain_ready();
     Module._dd2_movie_drain_cancel();Module._dd2_movie_drain_cancel();
     drainTimer={immediate,ready,elapsed,frozenClock,cancelled,beforeRearmDeadline,rearmElapsed,afterRearmDeadline};
    }
    performance.now=now;
    Module._free(p);
    return {opened,sourceExact:exact,scheduled,rows,stopCleared,activation,wrappedFallback,preciseFallback,fractionalFallback,drainTimer};
   });
   assert(!errors.length,JSON.stringify(errors));assert(result.opened===0&&result.sourceExact&&result.stopCleared,'caller PCM or cleanup differs');
   assert(result.wrappedFallback===123,'unavailable-device performance clock did not wrap');
   const rows=Object.fromEntries(result.rows.map(r=>[r.phase,r]));
   if(program==='production'){
    const timer=result.drainTimer;
    assert(timer&&timer.immediate===0&&timer.ready===1&&timer.elapsed>=100&&timer.frozenClock,'real relative timer followed frozen movie clock');
    assert(timer.cancelled===0&&timer.beforeRearmDeadline===0&&timer.rearmElapsed<99.8&&timer.afterRearmDeadline===1,'cancelled/replaced wait leaked into a later drain');
    assert(result.preciseFallback===(2**32+123.75)*1000&&result.fractionalFallback===123456789,'microsecond clock lost fractional milliseconds or wrapped as a DWORD');
    validateVideo(result.rows);
    assert(rows['render-ended-before-output'].done===0&&rows['source-still-queued'].done===0,'render-ended event completed queued output');
    assert(result.scheduled===1024/22050,'declared source scheduling changed');
    assert(rows['suspended-output'].done===0,'suspended output advanced completion');
    assert(rows['source-consumed'].done===1,'consumed source failed to complete');
    const a=result.activation;
    assert(a&&a.initial===a.held&&a.activated===a.initial&&a.advanced-a.activated===40500&&
           a.laterSuspended-a.advanced===750000,'initial activation hold leaked into later playback');
    for(const mutation of ['audio-clock','missing-row','fractional-loss','suspended-clock']){
     const changed=JSON.parse(JSON.stringify(result.rows));
     if(mutation==='audio-clock')changed[0].clock=0;
     else if(mutation==='missing-row')changed.pop();
     else if(mutation==='fractional-loss')changed[0].microseconds=Math.floor(changed[0].performance_ms)*1000;
     else changed[7].microseconds=changed[6].microseconds;
     assert.throws(()=>validateVideo(changed),'changed video clock accepted: '+mutation);
    }
   }else{
    assert.throws(()=>validateVideo(result.rows),'old audio-dependent video clock accepted');
   }
   cases.push({program,...result});await page.close();
   console.log(`PASS ${program}: unchanged source PCM, independent video clock and audio completion cases`);
  }
  fs.writeFileSync(path.join(root,'browser-clock.json'),JSON.stringify({component_checks_passed:true,original_port_parity:'unproven',cases},null,2)+'\n');
 }finally{if(browser)await browser.close();await new Promise(r=>server.close(r));}
})().catch(e=>{console.error(e);process.exitCode=1});
function validateVideo(rows){
 assert(rows.length===9,'complete clock phases required');
 for(const row of rows){
  assert(row.clock===(Math.floor(row.performance_ms)>>>0),'video milliseconds depend on audio output');
  assert(row.microseconds===Math.floor(row.performance_ms*1000),'video microseconds depend on audio output');
 }
}
