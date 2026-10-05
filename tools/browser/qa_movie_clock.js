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
    let scheduled,exact=false;
    const NativeContext=AudioContext;
    window.AudioContext=class extends NativeContext{
     constructor(...args){super(...args);Object.defineProperty(this,'currentTime',{configurable:true,get:()=>1024/22050});}
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
    let rendered=scheduled+.5,state='running',output={contextTime:0,performanceTime:0};
    Object.defineProperty(ac,'currentTime',{get:()=>rendered});
    Object.defineProperty(ac,'state',{get:()=>state,configurable:true});
    ac.getOutputTimestamp=()=>output;
    const rows=[];
    function read(phase){rows.push({phase,rendered,output:{...output},state,clock:Module._dd2_movie_now_ms(),microseconds:Module._dd2_movie_now_us?Module._dd2_movie_now_us():null,done:Module._dd2_movie_audio_done()});}
    read('no-output');
    output={contextTime:0,performanceTime:performance.now()-100};read('zero-output-position');
    output={contextTime:.00005,performanceTime:performance.now()-100};read('sub-millisecond-startup-position');
    output={contextTime:scheduled+.1,performanceTime:performance.now()-10};read('output-behind-render');
    Module._dd2movieSource.onended();read('render-ended-before-output');
    rendered=scheduled+2;output={contextTime:scheduled+.9,performanceTime:performance.now()};read('source-still-queued');
    output={contextTime:.2,performanceTime:performance.now()};read('old-output-timestamp');
    state='suspended';output={contextTime:scheduled+1.1,performanceTime:performance.now()};read('suspended-output');
    state='running';output={contextTime:scheduled+1.1,performanceTime:performance.now()};read('source-consumed');
    Module._dd2_movie_audio_stop();Module._dd2_movie_audio_stop();Module._free(p);
    delete ac.state;
    const stopCleared=!Module._dd2movieAc&&!Module._dd2movieSource&&!Module._dd2movieOutputTime;
    const now=performance.now;performance.now=()=>2**32+123.75;
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
    return {opened,sourceExact:exact,scheduled,rows,stopCleared,wrappedFallback,preciseFallback,fractionalFallback,drainTimer};
   });
   assert(!errors.length,JSON.stringify(errors));assert(result.opened===0&&result.sourceExact&&result.stopCleared,'caller PCM or cleanup differs');
   assert(result.wrappedFallback===123,'unavailable-device performance clock did not wrap');
   const rows=Object.fromEntries(result.rows.map(r=>[r.phase,r]));
   if(program==='production'){
    const timer=result.drainTimer;
    assert(timer&&timer.immediate===0&&timer.ready===1&&timer.elapsed>=100&&timer.frozenClock,'real relative timer followed frozen movie clock');
    assert(timer.cancelled===0&&timer.beforeRearmDeadline===0&&timer.rearmElapsed<99.8&&timer.afterRearmDeadline===1,'cancelled/replaced wait leaked into a later drain');
    assert(result.preciseFallback===(2**32+123.75)*1000&&result.fractionalFallback===123456789,'microsecond clock lost fractional milliseconds or wrapped as a DWORD');
    assert(rows['no-output'].microseconds===0&&rows['zero-output-position'].microseconds===0,'missing output advanced microsecond clock');
    assert(rows['no-output'].clock===0&&rows['zero-output-position'].clock===0,'unavailable output advanced movie');
    assert(rows['sub-millisecond-startup-position'].clock===0,'clamped startup position advanced movie through silent prefill');
    assert(rows['output-behind-render'].clock>=100&&rows['output-behind-render'].clock<500,'movie clock used rendered audio');
    assert(rows['render-ended-before-output'].done===0&&rows['source-still-queued'].done===0,'render-ended event completed queued output');
    assert(result.scheduled===1024/22050&&rows['source-still-queued'].clock>=900&&rows['source-still-queued'].clock<1000,'movie clock retained the declared pre-source render offset');
    assert(rows['old-output-timestamp'].clock>=rows['source-still-queued'].clock,'output timestamp regression moved clock backward');
    assert(rows['suspended-output'].clock===rows['old-output-timestamp'].clock&&rows['suspended-output'].done===0,'suspended output advanced clock or completion');
    assert(rows['source-consumed'].done===1,'consumed source failed to complete');
   }else{
    assert(rows['no-output'].clock===Math.floor((result.scheduled+.5)*1000)&&rows['render-ended-before-output'].done===1,'old render-clock/early-completion failures not reproduced');
   }
   cases.push({program,...result});await page.close();
   console.log(`PASS ${program}: unchanged source PCM, declared output-clock and completion cases`);
  }
  fs.writeFileSync(path.join(root,'browser-clock.json'),JSON.stringify({component_checks_passed:true,original_port_parity:'unproven',cases},null,2)+'\n');
 }finally{if(browser)await browser.close();await new Promise(r=>server.close(r));}
})().catch(e=>{console.error(e);process.exitCode=1});
