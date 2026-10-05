// Exercise production movie platform imports in real Chromium with declared
// caller PCM and allocation/open/rate faults. No original-parity claim.
const fs=require('fs'),path=require('path'),assert=require('assert');
const {serve,chromium}=require('./felib');
const root=path.resolve(process.argv[2]);
(async()=>{
 const server=serve(root);await new Promise(r=>server.listen(0,r));const cases=[];
 try{
  for(const [program,mode] of [['production','normal'],['before','normal'],['production','gesture'],
                             ['production','buffer-error'],['production','context-error'],['production','rate-error']]){
   const browser=await chromium.launch({args:['--no-sandbox',...(mode==='gesture'?[
    '--autoplay-policy=document-user-activation-required',
    '--disable-features=PreloadMediaEngagementData,MediaEngagementBypassAutoplayPolicies']:[])]});
   try{
    const page=await browser.newPage(),errors=[];
    page.on('pageerror',e=>errors.push(e.message));
    await page.addInitScript(mode=>{
     window.__ready={buffers:0,contexts:[],attempts:0,prepared:false,sourceExact:false};
     window.__startReadyFixture=()=>{
      const frames=65536,p=Module._malloc(frames*4),samples=new Int16Array(frames*2);
      for(let i=0;i<frames;i++){samples[i*2]=i-32768;samples[i*2+1]=32767-i;}
      HEAP16.set(samples,p>>1);window.__readyInput={frames,rate:22050,samples};window.__readyPointer=p;
      const result=Module._dd2_movie_audio_start(p,frames,22050,2);
      return {result,prepared:__ready.prepared,sourceExact:__ready.sourceExact,attempts:__ready.attempts,
              buffers:__ready.buffers,contexts:__ready.contexts.length,state:__ready.contexts[0]?.state??null};
     };
     const Buffer=window.AudioBuffer,Context=window.AudioContext;
     window.AudioBuffer=class extends Buffer{
      constructor(options){if(mode==='buffer-error')throw Error('declared buffer allocation error');
       super(options);__ready.buffers++;window.__readyBuffer=this;}
     };
     window.AudioContext=class extends Context{
      constructor(options){
       __ready.attempts++;
       const b=window.__readyBuffer,e=window.__readyInput;
       if(b&&e){
        let exact=b.length===e.frames&&b.sampleRate===e.rate&&b.numberOfChannels===2;
        for(let ch=0;ch<2;ch++){
         const samples=b.getChannelData(ch);
         for(let i=0;i<e.frames;i++)if(samples[i]!==e.samples[i*2+ch]/32768)exact=false;
        }
        __ready.prepared=exact;
       }
       if(mode==='context-error')throw Error('declared context open error');
       super(options);__ready.contexts.push(this);
      }
      get sampleRate(){return mode==='rate-error'?48000:super.sampleRate;}
     };
     const make=Context.prototype.createBufferSource;
     Context.prototype.createBufferSource=function(...args){
      const source=make.apply(this,args),start=source.start;
      source.start=function(...values){
       const b=source.buffer,e=__readyInput;let exact=b.length===e.frames&&b.sampleRate===e.rate&&b.numberOfChannels===2;
       for(let ch=0;ch<2;ch++){
        const samples=b.getChannelData(ch);
        for(let i=0;i<e.frames;i++)if(samples[i]!==e.samples[i*2+ch]/32768)exact=false;
       }
       __ready.sourceExact=exact;return start.apply(this,values);
      };
      return source;
     };
    },mode);
    await page.goto(`http://localhost:${server.address().port}/${program}.html`);
    await page.waitForFunction(()=>window.fixtureReady===true);
    // Startup runs from the runtime callback, before Playwright evaluation
    // can supply the activation that this gesture test must initially lack.
    const initial=await page.evaluate(()=>window.__fixtureResult);
    assert(!errors.length,JSON.stringify(errors));
    if(mode.endsWith('error')){
     assert(initial.result===-1,'declared audio failure must preserve video-only fallback');
     assert(initial.contexts===(mode==='rate-error'?1:0),'failed preparation/open leaked a context');
     assert(mode!=='buffer-error'||initial.attempts===0,'buffer error opened a real-time context');
    }else{
     assert(initial.result===0&&initial.sourceExact&&initial.contexts===1,'complete caller PCM/source required');
     assert(initial.prepared===(program==='production'),'old context-before-PCM order must fail preparation invariant');
     if(mode==='gesture'){
      assert(initial.state==='suspended','gesture case started without activation');
      await page.click('button');await page.waitForFunction(()=>__ready.contexts[0].state==='running');
     }
    }
    await page.evaluate(()=>{Module._dd2_movie_audio_stop();Module._dd2_movie_audio_stop();Module._free(__readyPointer);});
    await page.waitForFunction(()=>__ready.contexts.every(ac=>ac.state==='closed'));
    const ended=await page.evaluate(()=>({closed:__ready.contexts.every(ac=>ac.state==='closed'),
                                        sourceCleared:!Module._dd2movieSource,contextCleared:!Module._dd2movieAc}));
    assert(ended.closed&&ended.sourceCleared&&ended.contextCleared,'idempotent close failed');
    cases.push({program,mode,...initial,...ended});
    fs.writeFileSync(path.join(root,'browser-ready.json'),JSON.stringify({observations_valid:true,cases},null,2)+'\n');
    console.log(`PASS production movie readiness ${program}/${mode}: ${JSON.stringify(initial)}`);
   }finally{await browser.close();}
  }
 }finally{await new Promise(r=>server.close(r));}
})().catch(e=>{console.error(e);process.exitCode=1});
