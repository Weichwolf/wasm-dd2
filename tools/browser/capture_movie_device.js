// Read-only movie/API observations with actual Chromium ALSA output enabled.
// This records existing context/source calls; no graph nodes or input PCM change.
const fs=require('fs'),path=require('path'),assert=require('assert'),crypto=require('crypto');
const {serve,chromium}=require('./felib');
const build=path.resolve(process.argv[2]),output=path.resolve(process.argv[3]);
const movie=process.argv[4]||'Intro.avi',skip=process.argv.includes('--skip');
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
(async()=>{
 const server=serve(build);await new Promise(r=>server.listen(0,r));let browser;
 const errors=[];
 try{
  browser=await chromium.launch({args:['--no-sandbox','--alsa-output-device=default'],
                                ignoreDefaultArgs:['--mute-audio']});
  const page=await browser.newPage();
  page.on('pageerror',e=>errors.push(e.message));page.on('crash',()=>errors.push('renderer crash'));
  await page.addInitScript(()=>{
   window.__deviceMovie={events:[],frames:[],source:null};
   const contexts=new WeakSet(),observed=new WeakSet();let preparing;
   function stamp(event,ac,extra={}){
    const out=ac.getOutputTimestamp();
    __deviceMovie.events.push({event,performance_ms:performance.now(),context_time:ac.currentTime,
                              output_context_time:out.contextTime,output_performance_ms:out.performanceTime,
                              state:ac.state,...extra});
   }
   const make=AudioContext.prototype.createBufferSource;
   AudioContext.prototype.createBufferSource=function(...args){
    const ac=this,source=make.apply(this,args),start=source.start;
    source.start=function(...values){
     if(preparing){
      const e=preparing,b=source.buffer;
      let mismatches=0;
      for(let ch=0;ch<e.channels;ch++){
       const samples=b.getChannelData(ch);
       for(let i=0;i<e.frames;i++)if(samples[i]!==HEAP16[(e.pointer>>1)+i*e.channels+ch]/32768)mismatches++;
      }
      __deviceMovie.source={frames:e.frames,rate:e.rate,channels:e.channels,
                            context_rate:ac.sampleRate,mismatches};
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
     const result=present.apply(this,args);
     __deviceMovie.frames.push({frame:__deviceMovie.frames.length,begin_performance_ms:before,
                               end_performance_ms:performance.now(),context_time:time});
     return result;
    };
   }
   for(const name of ['instantiate','instantiateStreaming']){
    const make=WebAssembly[name];if(make)WebAssembly[name]=function(bytes,imports,...rest){observe(imports);return make.call(this,bytes,imports,...rest);};
   }
  });
  await page.goto(`http://localhost:${server.address().port}/index.html?movie=${encodeURIComponent(movie.toUpperCase())}`);
  await page.waitForFunction(()=>!!Module._dd2movieSource&&HEAP32[0x462cd4>>2]===1,null,{timeout:30000});
  await page.click('#canvas');
  if(skip){
   await page.waitForFunction(()=>__deviceMovie.frames.length>=26);
   await page.keyboard.up('Escape');await page.waitForTimeout(200);
   assert(await page.evaluate(()=>HEAP32[0x462cd4>>2])===1,'keyup skipped movie');
   await page.keyboard.down('d');
  }
  await page.waitForFunction(()=>HEAP32[0x462cd4>>2]===0&&!Module._dd2movieSource&&
                               __deviceMovie.events.some(e=>e.event==='context-close'),null,{timeout:110000});
  const observed=await page.evaluate(()=>__deviceMovie);
  assert(!errors.length,JSON.stringify(errors));assert(observed.source?.mismatches===0,'movie source Float32 conversion changed');
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
                chromium_version:browser.version(),device_closed_before_browser_shutdown:true,
                observer_source_sha256:hash(__filename),...observed};
  fs.writeFileSync(path.join(output,'browser.json'),JSON.stringify(report,null,2)+'\n');
  console.log(`Observed unmuted browser ${movie} skip=${skip}: ${observed.frames.length} frames, device closed normally`);
 }finally{if(browser)await browser.close();await new Promise(r=>server.close(r));}
})().catch(e=>{console.error(e);process.exitCode=1});
