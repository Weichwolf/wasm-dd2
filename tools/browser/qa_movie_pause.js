// Actual engine movie playback with declared AudioContext suspend/resume.
// Forward clock/present imports unchanged; no source PCM or engine-state writes.
const fs=require('fs'),path=require('path'),assert=require('assert'),crypto=require('crypto');
const {serve,chromium}=require('./felib');
const build=path.resolve(process.argv[2]),before=path.resolve(process.argv[3]),output=path.resolve(process.argv[4]);
assert(output.startsWith('/tmp/wasm-dd2/'),'Use /tmp/wasm-dd2/');
fs.mkdirSync(output,{recursive:false});
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
(async()=>{
 const cases=[];
 for(const [program,mode,directory] of [['before','normal',before],['production','normal',build],['production','gesture',build]]){
  const server=serve(directory);await new Promise(r=>server.listen(0,r));let browser;
  try{
   browser=await chromium.launch({args:['--no-sandbox',...(mode==='gesture'?[
    '--autoplay-policy=document-user-activation-required',
    '--disable-features=PreloadMediaEngagementData,MediaEngagementBypassAutoplayPolicies']:[
    '--autoplay-policy=no-user-gesture-required'])]});
   const page=await browser.newPage(),errors=[];page.on('pageerror',e=>errors.push(e.message));
   let initialAudio;
   const opened=new Promise(resolve=>{page.on('console',m=>{
    if(m.text().startsWith('DD2_CLOCK_AUDIO_STATE:')){initialAudio=m.text().slice(22);resolve();}
   });});
   await page.addInitScript(()=>{
    window.__clockPause={frames:0,clock_calls:0,last_movie_us:null};
    const seen=new WeakSet();
    function observe(imports){
     if(!imports?.env?.movie_clock_us||seen.has(imports.env))return;
     seen.add(imports.env);
     const clock=imports.env.movie_clock_us,present=imports.env.movie_present,audio=imports.env.movie_audio_start;
     imports.env.movie_audio_start=function(...args){
      const value=audio.apply(this,args);console.log('DD2_CLOCK_AUDIO_STATE:'+Module._dd2movieAc?.state);return value;
     };
     imports.env.movie_clock_us=function(...args){
      const value=clock.apply(this,args);__clockPause.last_movie_us=value;__clockPause.clock_calls++;return value;
     };
     imports.env.movie_present=function(...args){
      const result=present.apply(this,args);__clockPause.frames++;return result;
     };
    }
    for(const name of ['instantiate','instantiateStreaming']){
     const make=WebAssembly[name];if(make)WebAssembly[name]=function(bytes,imports,...rest){observe(imports);return make.call(this,bytes,imports,...rest);};
    }
   });
   await page.goto(`http://localhost:${server.address().port}/index.html?movie=INTRO.AVI`);
   const sample=()=>page.evaluate(()=>({...__clockPause,performance_ms:performance.now(),
    audio_state:Module._dd2movieAc.state,audio_context_time:Module._dd2movieAc.currentTime,
    movie_playing:HEAP32[0x462cd4>>2]}));
   let activation=null;
   if(mode==='gesture'){
    await opened;await page.waitForTimeout(150);
    const cdp=await page.context().newCDPSession(page);
    const unactivated=async()=>JSON.parse((await cdp.send('Runtime.evaluate',{
     expression:'JSON.stringify({...__clockPause,performance_ms:performance.now(),audio_state:Module._dd2movieAc.state,audio_context_time:Module._dd2movieAc.currentTime,movie_playing:HEAP32[0x462cd4>>2]})',
     returnByValue:true,userGesture:false})).result.value);
    const begin=await unactivated();await page.waitForTimeout(750);const end=await unactivated();
    fs.writeFileSync(path.join(output,'activation.json'),JSON.stringify({initialAudio,begin,end},null,2)+'\n');
    assert(end.frames===1&&begin.last_movie_us===end.last_movie_us,'initial autoplay advanced video before activation');
    activation={initialAudio,begin,end};await cdp.detach();await page.click('#canvas');
   }
   await page.waitForFunction(()=>__clockPause.frames>=26&&Module._dd2movieAc.state==='running',null,{timeout:30000});
   const running=await sample();
   await page.evaluate(()=>Module._dd2movieAc.suspend());await page.waitForTimeout(100);
   const pausedBegin=await sample();await page.waitForTimeout(750);const pausedEnd=await sample();
   assert(pausedEnd.audio_state==='suspended'&&pausedEnd.movie_playing===1&&
          pausedEnd.audio_context_time===pausedBegin.audio_context_time,'declared audio suspension missing');
   const elapsed=pausedEnd.performance_ms-pausedBegin.performance_ms;
   assert(elapsed>=750,'independent performance time did not advance');
   const videoElapsed=(pausedEnd.last_movie_us-pausedBegin.last_movie_us)/1000;
   if(program==='production'){
    assert(pausedEnd.frames-pausedBegin.frames>=16&&Math.abs(videoElapsed-elapsed)<12,
           'actual video still follows suspended audio output');
   }else{
    assert(pausedEnd.frames===pausedBegin.frames&&videoElapsed===0,'old audio-dependent freeze not reproduced');
   }
   await page.evaluate(()=>Module._dd2movieAc.resume());await page.waitForTimeout(750);
   const resumed=await sample();assert(resumed.frames>pausedEnd.frames,'actual movie did not advance after resume');
   await page.keyboard.up('Escape');await page.waitForTimeout(100);
   assert(await page.evaluate(()=>HEAP32[0x462cd4>>2])===1,'key-up cancelled playback');
   await page.keyboard.down('d');
   await page.waitForFunction(()=>HEAP32[0x462cd4>>2]===0&&!Module._dd2movieSource,null,{timeout:5000});
   assert(!errors.length,JSON.stringify(errors));
   cases.push({program,mode,browser_version:await browser.version(),wasm_sha256:hash(path.join(directory,'index.wasm')),
    activation,running,paused_begin:pausedBegin,paused_end:pausedEnd,resumed,performance_elapsed_ms:elapsed,
    video_elapsed_ms:videoElapsed,old_clock_rejected:program==='before',key_up_retained:true,key_down_closed:true,errors});
   fs.writeFileSync(path.join(output,'report.json'),JSON.stringify({scope:'Actual browser engine clock/present imports with declared AudioContext suspend/resume and initial activation. Muted component diagnosis; no physical PCM, original whole-movie or synchronized A/V parity claim.',
    pass_:cases.length===3,original_port_av_parity:'unproven',observer_sha256:hash(__filename),cases},null,2)+'\n');
   console.log(`PASS ${program}/${mode}: ${pausedEnd.frames-pausedBegin.frames} frames across ${elapsed.toFixed(1)} ms audio suspension`);
  }finally{if(browser)await browser.close();await new Promise(r=>server.close(r));}
 }
})().catch(e=>{console.error(e);process.exitCode=1});
