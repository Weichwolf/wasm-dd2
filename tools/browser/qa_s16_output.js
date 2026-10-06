// Observe actual production movie imports and declared codec candidates at the
// real Chromium ALSA endpoint. No reference samples enter the audio observer.
const fs=require('fs'),path=require('path'),assert=require('assert'),crypto=require('crypto');
const {serve,chromium}=require('./felib');
const root=path.resolve(process.argv[2]);
const pulse=process.argv.includes('--pulse');
const hash=p=>crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex');
(async()=>{
 const server=serve(root);await new Promise(r=>server.listen(0,r));let browser;
 const errors=[];
 try{
  browser=await chromium.launch({args:['--no-sandbox',...(pulse?[]:['--alsa-output-device=default']),
                                      '--autoplay-policy=no-user-gesture-required'],
                                ignoreDefaultArgs:['--mute-audio']});
  const page=await browser.newPage();
  page.on('pageerror',e=>errors.push(e.message));page.on('crash',()=>errors.push('renderer crash'));
  await page.addInitScript(()=>{
   window.__s16={events:[]};
   const make=AudioContext.prototype.createBufferSource;
   AudioContext.prototype.createBufferSource=function(...args){
    const ac=this,source=make.apply(this,args),start=source.start;
    source.start=function(...values){
     window.__s16Buffer=source.buffer;window.__s16Context=ac;
     __s16.events.push({event:'start',scheduled_time:values[0],context_time:ac.currentTime,
                       performance_ms:performance.now(),sample_rate:ac.sampleRate});
     source.addEventListener('ended',()=>__s16.events.push({event:'ended',context_time:ac.currentTime,
                                                          performance_ms:performance.now()}));
     return start.apply(this,values);
    };
    return source;
   };
   const close=AudioContext.prototype.close;
   AudioContext.prototype.close=function(...args){
    __s16.events.push({event:'close',context_time:this.currentTime,performance_ms:performance.now()});
    return close.apply(this,args);
   };
   window.__startS16Fixture=()=>{
    const frames=65536,p=Module._malloc(frames*4);
    window.__s16Pointer=p;
    for(let i=0;i<frames;i++){HEAP16[(p>>1)+i*2]=i-32768;HEAP16[(p>>1)+i*2+1]=32767-i;}
    window.__s16Result=Module._dd2_movie_audio_start(p,frames,22050,2);
   };
  });
  await page.goto(`http://localhost:${server.address().port}/index.html`);
  await page.waitForFunction(()=>window.__s16Result!==undefined);
  assert(await page.evaluate(()=>__s16Result)===0,'production audio import did not start');
  await page.waitForFunction(()=>Module._dd2_movie_audio_done()===1,null,{timeout:15000});
  await page.evaluate(()=>{Module._dd2_movie_audio_stop();Module._free(__s16Pointer);});
  await page.waitForFunction(()=>__s16Context.state==='closed');
  // Inspect after playback; walking all source samples during start would
  // itself advance the real-time context before the source is scheduled.
  const observed=await page.evaluate(()=>{
   const b=__s16Buffer,interleaved=new Float32Array(b.length*b.numberOfChannels);
   for(let ch=0;ch<b.numberOfChannels;ch++){
    const values=b.getChannelData(ch);
    for(let i=0;i<b.length;i++)interleaved[i*b.numberOfChannels+ch]=values[i];
   }
   const bytes=new Uint8Array(interleaved.buffer);let text='';
   for(let i=0;i<bytes.length;i+=32768)text+=String.fromCharCode(...bytes.subarray(i,i+32768));
   return {events:__s16.events,frames:b.length,channels:b.numberOfChannels,sample_rate:b.sampleRate,
           closed:__s16Context.state==='closed',source:btoa(text)};
  });
  assert(!errors.length,JSON.stringify(errors));
  fs.writeFileSync(path.join(root,'actual-source.f32'),Buffer.from(observed.source,'base64'));
  delete observed.source;
  const deadline=performance.now()+20000;let closed=false;
  while(performance.now()<deadline){
   const journals=fs.readdirSync(path.join(root,'audio')).filter(n=>pulse?/^pulse-.*\.jsonl$/.test(n):/^(?:played|stream)-.*\.jsonl$/.test(n));
   if(journals.length===(pulse?1:2)&&journals.every(n=>fs.readFileSync(path.join(root,'audio',n),'utf8').includes('"event":"close"'))){closed=true;break;}
   await new Promise(r=>setTimeout(r,100));
  }
  assert(closed,'output stream did not close naturally before browser shutdown');
  fs.writeFileSync(path.join(root,'browser.json'),JSON.stringify({observations_valid:true,
   chromium_version:browser.version(),backend:pulse?'pulse':'alsa',
   ...(pulse?{accepted_stream_closed_before_browser_shutdown:true}:{device_closed_before_browser_shutdown:true}),
   wasm_sha256:hash(path.join(root,'fixture.wasm')),source_sha256:hash(path.join(root,'actual-source.f32')),
   observer_source_sha256:hash(__filename),...observed},null,2)+'\n');
 }finally{if(browser)await browser.close();await new Promise(r=>server.close(r));}
})().catch(e=>{console.error(e);process.exitCode=1});
