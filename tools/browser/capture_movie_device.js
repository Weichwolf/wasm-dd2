// Read-only movie/API observations with actual Chromium ALSA output enabled.
// This records existing context/source calls; no graph nodes or input PCM change.
const fs=require('fs'),path=require('path'),assert=require('assert'),crypto=require('crypto');
const {serve,chromium}=require('./felib');
const build=path.resolve(process.argv[2]),output=path.resolve(process.argv[3]);
const movie=process.argv[4]||'Intro.avi',skip=process.argv.includes('--skip');
const clockProfile=process.argv.includes('--clock-profile');
const delayOption=process.argv.find(a=>a.startsWith('--source-delay-ms='));
const sourceDelayMs=delayOption?Number(delayOption.split('=')[1]):0;
assert(Number.isFinite(sourceDelayMs)&&sourceDelayMs>=0&&sourceDelayMs<=200,'declared source preparation work must be 0..200 ms');
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
(async()=>{
 const server=serve(build);await new Promise(r=>server.listen(0,r));let browser;
 const errors=[];
 const clockBounds=[],hostClock=[];
 function checkHostClock(){
  if(!clockProfile)return;
  const {execFileSync}=require('child_process');
  for(let i=0;i<3;i++){
   const begin=process.hrtime.bigint(),value=BigInt(execFileSync(path.join(output,'clock-probe'),{encoding:'utf8'}).trim()),end=process.hrtime.bigint();
   assert(begin<=value&&value<=end,'Node hrtime does not bracket native CLOCK_MONOTONIC');
   hostClock.push({begin_ns:String(begin),native_monotonic_ns:String(value),end_ns:String(end)});
  }
 }
 async function sampleClock(page,phase){
  if(!clockProfile)return;
  for(let i=0;i<8;i++){
   const begin=process.hrtime.bigint();
   const value=await page.evaluate(()=>({performance_ms:performance.now(),time_origin_ms:performance.timeOrigin}));
   const end=process.hrtime.bigint();
   clockBounds.push({phase,begin_ns:String(begin),end_ns:String(end),...value});
  }
 }
 checkHostClock();
 fs.writeFileSync(path.join(output,'observer-source.js'),fs.readFileSync(__filename));
 try{
  browser=await chromium.launch({args:['--no-sandbox','--alsa-output-device=default'],
                                ignoreDefaultArgs:['--mute-audio']});
  const page=await browser.newPage();
  page.on('pageerror',e=>errors.push(e.message));page.on('crash',()=>errors.push('renderer crash'));
  await page.addInitScript(({clockProfile,sourceDelayMs})=>{
   window.__deviceMovie={events:[],frames:[],source:null};
   if(clockProfile)__deviceMovie.clock_calls=0;
   const contexts=new WeakSet(),observed=new WeakSet();let preparing;
   function snapshot(ac){
    const begin=performance.now(),context=ac.currentTime,out=ac.getOutputTimestamp(),end=performance.now();
    return {begin_performance_ms:begin,end_performance_ms:end,context_time:context,
            output_context_time:out.contextTime,output_performance_ms:out.performanceTime,
            base_latency:ac.baseLatency,output_latency:ac.outputLatency,state:ac.state};
   }
   function stamp(event,ac,extra={}){
    const out=ac.getOutputTimestamp();
    __deviceMovie.events.push({event,performance_ms:performance.now(),context_time:ac.currentTime,
                              output_context_time:out.contextTime,output_performance_ms:out.performanceTime,
                              state:ac.state,...extra});
   }
   const make=AudioContext.prototype.createBufferSource;
   AudioContext.prototype.createBufferSource=function(...args){
    const ac=this,source=make.apply(this,args),start=source.start;
    // Optional declared main-thread work exposes a nonzero scheduled start;
    // preserve actual nodes, context clocks and every source PCM sample.
    if(preparing&&sourceDelayMs){const began=performance.now();while(performance.now()-began<sourceDelayMs){}}
    source.start=function(...values){
     if(preparing){
      const e=preparing,b=source.buffer;
      __deviceMovie.source={frames:e.frames,rate:e.rate,channels:e.channels,
                            context_rate:ac.sampleRate};
      window.__deviceMovieBuffer=b;
      contexts.add(ac);stamp('source-start',ac,{scheduled_time:values[0]??null});
      source.addEventListener('ended',()=>stamp('source-ended',ac));
     }
     return start.apply(this,values);
    };
    return source;
   };
   const close=AudioContext.prototype.close;
   AudioContext.prototype.close=function(...args){
    if(contexts.has(this))stamp('context-close',this,{movie_playing:HEAP32[0x462cd4>>2]});
    return close.apply(this,args);
   };
   function observe(imports){
    if(!imports?.env?.movie_audio_start||observed.has(imports.env))return;
    observed.add(imports.env);
    const audio=imports.env.movie_audio_start,present=imports.env.movie_present;
    imports.env.movie_audio_start=function(pointer,frames,rate,channels){
     preparing={pointer,frames,rate,channels};
     try{return audio(pointer,frames,rate,channels);}finally{preparing=undefined;}
    };
    imports.env.movie_present=function(...args){
     const ac=Module._dd2movieAc,before=performance.now(),time=ac?.currentTime??null;
     const beforeClock=clockProfile&&ac?snapshot(ac):null;
     const result=present.apply(this,args);
     __deviceMovie.frames.push({frame:__deviceMovie.frames.length,begin_performance_ms:before,
                               end_performance_ms:performance.now(),context_time:time,
                               ...(clockProfile?{movie_clock_ms:__deviceMovie.last_movie_clock_ms,
                                                ...(__deviceMovie.movie_clock_unit==='microseconds'?{movie_clock_us:__deviceMovie.last_movie_clock_us}:{}),
                                                before_clock:beforeClock,after_clock:ac?snapshot(ac):null}:{})});
     return result;
    };
    if(clockProfile){
     const name=typeof imports.env.movie_clock_us==='function'?'movie_clock_us':'movie_clock';
     const clock=imports.env[name],precise=name==='movie_clock_us';
     assertClock(clock);__deviceMovie.movie_clock_unit=precise?'microseconds':'milliseconds';
     imports.env[name]=function(...args){
      const value=clock.apply(this,args);__deviceMovie.clock_calls++;
      if(__deviceMovie.clock_calls===1){
       __deviceMovie.initial_movie_clock_ms=precise?value/1000:value;
       if(precise)__deviceMovie.initial_movie_clock_us=value;
      }
      __deviceMovie.last_movie_clock_ms=precise?value/1000:value;
      if(precise)__deviceMovie.last_movie_clock_us=value;
      return value;
     };
    }
   }
   function assertClock(clock){if(typeof clock!=='function')throw Error('production movie clock import missing');}
   for(const name of ['instantiate','instantiateStreaming']){
    const make=WebAssembly[name];if(make)WebAssembly[name]=function(bytes,imports,...rest){observe(imports);return make.call(this,bytes,imports,...rest);};
   }
  },{clockProfile,sourceDelayMs});
  await page.goto(`http://localhost:${server.address().port}/index.html?movie=${encodeURIComponent(movie.toUpperCase())}`);
  await page.waitForFunction(()=>!!Module._dd2movieSource&&HEAP32[0x462cd4>>2]===1,null,{timeout:30000});
  await page.click('#canvas');
  await sampleClock(page,'playing');
  if(skip){
   await page.waitForFunction(()=>__deviceMovie.frames.length>=26);
   await page.keyboard.up('Escape');await page.waitForTimeout(200);
   assert(await page.evaluate(()=>HEAP32[0x462cd4>>2])===1,'keyup skipped movie');
   await page.keyboard.down('d');
  }
  await page.waitForFunction(()=>HEAP32[0x462cd4>>2]===0&&!Module._dd2movieSource&&
                               __deviceMovie.events.some(e=>e.event==='context-close'),null,{timeout:110000});
  await sampleClock(page,'finished');checkHostClock();
  // Inspect source PCM after playback. Walking a whole AudioBuffer inside
  // source.start would itself add empty rendered quanta before scheduling.
  const observed=await page.evaluate(()=>{
   const state=__deviceMovie,b=__deviceMovieBuffer,e=state.source;
   const pcm=new Int16Array(e.frames*e.channels);let errors=0;
   for(let ch=0;ch<e.channels;ch++){
    const samples=b.getChannelData(ch);
    for(let i=0;i<e.frames;i++){
     const n=samples[i]*32768;
     if(!Number.isInteger(n)||n<-32768||n>32767)errors++;
     pcm[i*e.channels+ch]=n;
    }
   }
   state.source.canonical_encoding_errors=errors;
   const bytes=new Uint8Array(pcm.buffer);let text='';
   for(let i=0;i<bytes.length;i+=32768)text+=String.fromCharCode(...bytes.subarray(i,i+32768));
   return {...state,source_pcm:btoa(text)};
  });
  assert(!errors.length,JSON.stringify(errors));assert(observed.source?.canonical_encoding_errors===0,'movie source is not canonical S16 Float32');
  fs.writeFileSync(path.join(output,'movie-source.pcm'),Buffer.from(observed.source_pcm,'base64'));
  delete observed.source_pcm;
  observed.source.pcm_file='movie-source.pcm';observed.source.pcm_sha256=hash(path.join(output,'movie-source.pcm'));
  // Chromium pools output streams after closing an AudioContext. Wait for
  // actual device close journals; browser termination must not forge EOF.
  const deadline=performance.now()+20000;
  let closed=false;
  while(performance.now()<deadline){
   const journals=fs.readdirSync(path.join(output,'audio')).filter(n=>/^(?:played|stream)-.*\.jsonl$/.test(n));
   if(journals.length>=2&&journals.every(n=>fs.readFileSync(path.join(output,'audio',n),'utf8').includes('"event":"close"'))){closed=true;break;}
   await new Promise(r=>setTimeout(r,100));
  }
  assert(closed,'Chromium device lifetime did not close before browser termination');
  const report={scope:'Actual unmuted Chromium ALSA movie device and unchanged source/API observations; no original parity claim',
                observations_valid:true,original_port_parity:'unproven',movie,skip,
                key_up_retained:skip,wasm_sha256:hash(path.join(build,'index.wasm')),
                source_schedule_delay_ms:sourceDelayMs,
                chromium_version:browser.version(),device_closed_before_browser_shutdown:true,
                observer_source_sha256:hash(path.join(output,'observer-source.js')),
                ...(clockProfile?{clock_profile:true,clock_bounds:clockBounds,host_clock_bounds:hostClock,
                                  host_clock_probe_sha256:hash(path.join(output,'clock_probe.c'))}:{}),...observed};
  fs.writeFileSync(path.join(output,'browser.json'),JSON.stringify(report,null,2)+'\n');
  console.log(`Observed unmuted browser ${movie} skip=${skip}: ${observed.frames.length} frames, device closed normally`);
 }finally{if(browser)await browser.close();await new Promise(r=>server.close(r));}
})().catch(e=>{console.error(e);process.exitCode=1});
