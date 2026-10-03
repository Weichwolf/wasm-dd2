// Navigate the actual CD-player menu and verify delivered WebAudio PCM, not labels alone.
const fs=require('fs'),path=require('path'),assert=require('assert');
const {createHash}=require('crypto');
const {serve,key,rd,boot,gotoButton,chromium}=require('./felib');
const build=path.resolve(process.argv[2]||'web/dd2');
fs.mkdirSync('/tmp/wasm-dd2',{recursive:true});
const output=path.resolve(process.argv[3] || `/tmp/wasm-dd2/browser-redbook-${Date.now()}-${process.pid}`);
assert(output.startsWith('/tmp/wasm-dd2/'),'verification output must use /tmp/wasm-dd2');
const parent=fs.realpathSync(path.dirname(output));
assert(parent==='/tmp/wasm-dd2' || parent.startsWith('/tmp/wasm-dd2/'),'verification parent must remain inside /tmp/wasm-dd2');
fs.mkdirSync(output,{recursive:false});
const report={scope:'Real browser CD menu controls, track2/3 source PCM and C-to-WebAudio buffer equality; complete original mixed-output/timing acceptance is separate',pass:false,tracks:[]};
(async()=>{
 const server=serve(build);await new Promise(resolve=>server.listen(0,resolve));
 let browser;
 try{
  browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:560}});
  const errors=[];page.on('pageerror',error=>errors.push(error.message));
  page.on('console',message=>{if(message.text().includes('[CD]'))console.log('device:',message.text());});
  page.on('response',response=>{if(response.url().endsWith('.cdda'))console.log('CD fetch:',response.status(),response.url());});
  await page.addInitScript(()=>{
   window.__cdTest={track:0,parts:[],frames:0,totalBuffers:0,musicBuffers:0,mismatches:0,missingBuffers:0,otherBuffers:0,plays:[],presentations:0,zeroSlabFrames:0};
   let expected;
   const observeImports=imports=>{
    if(imports && imports.env && imports.env.dd2_present && !imports.env.dd2_present.__cdObserved){
     const present=imports.env.dd2_present;
     const observed=function(...args){
      __cdTest.presentations++;
      __cdTest.zeroSlabFrames=HEAPU16[0x46996c>>1]===0?__cdTest.zeroSlabFrames+1:0;
      return present.apply(this,args);
     };
     observed.__cdObserved=true;imports.env.dd2_present=observed;
    }
    if(!imports || !imports.env || !imports.env.dd2_audio_push || imports.env.dd2_audio_push.__cdObserved)return;
    const original=imports.env.dd2_audio_push;
    const observed=function(pointer,effects,music,frames,rate,musicFrames){
     expected={pointer,music,frames,rate,musicFrames,starts:0};
     try{return original.call(this,pointer,effects,music,frames,rate,musicFrames);}finally{
      if(Module._dd2ac && Module._dd2ac.state==='running' && expected.starts!==1)__cdTest.missingBuffers++;
      expected=undefined;
     }
    };
    observed.__cdObserved=true;imports.env.dd2_audio_push=observed;
   };
   for(const name of ['instantiate','instantiateStreaming']){
    const original=WebAssembly[name];
    if(original)WebAssembly[name]=function(bytes,imports,...rest){observeImports(imports);return original.call(this,bytes,imports,...rest);};
   }
   const log=console.log;
   console.log=function(...args){
    const message=args.join(' '),match=message.match(/\[CD\] play track=(\d+) frame=(\d+)/);
    if(match){const state=window.__cdTest;state.track=+match[1];state.parts=[];state.frames=0;state.plays.push({track:+match[1],frame:+match[2]});}
    log.apply(this,args);
   };
   const create=AudioContext.prototype.createBufferSource;
   AudioContext.prototype.createBufferSource=function(...args){
    const source=create.apply(this,args),start=source.start;
    source.start=function(...args){
     const state=window.__cdTest,buffer=source.buffer;
     if(buffer && expected){
      expected.starts++;state.totalBuffers++;
      let exact=buffer.length===expected.frames && buffer.sampleRate===expected.rate && buffer.numberOfChannels===2;
      for(let ch=0;ch<2;ch++){
       const samples=buffer.getChannelData(ch),bits=new Uint32Array(samples.buffer,samples.byteOffset,samples.length);
       for(let i=0;i<buffer.length;i++)if(bits[i]!==HEAPU32[(expected.pointer>>2)+i*2+ch])exact=false;
      }
      if(!exact)state.mismatches++;
      if(expected.musicFrames){
       state.musicBuffers++;
       if(state.frames<100000){
        const pcm=new Int16Array(expected.musicFrames*2);
        for(let i=0;i<pcm.length;i++){
         const sample=HEAPF32[(expected.music>>2)+i]*32768;
         if(sample!==Math.round(sample) || sample<-32768 || sample>32767)state.mismatches++;
         pcm[i]=sample;
        }
        state.parts.push(new Uint8Array(pcm.buffer));state.frames+=expected.musicFrames;
       }
      }
     }else if(buffer)state.otherBuffers++;
     return start.apply(this,args);
    };
    return source;
   };
  });
  // Same boot helper, with MCI diagnostics enabled before libc creates its environment.
  const originalGoto=page.goto.bind(page);
  page.goto=(url,options)=>originalGoto(url+'&cdlog',options);
  await boot(page,server);
  // Intro audio uses the separate movie sink before the CD menu is opened.
  // Reject any additional unobserved sink during the actual menu exercise.
  const priorOtherBuffers=await page.evaluate(()=>__cdTest.otherBuffers);
  assert(await gotoButton(page,'CD Audio Player'),'cannot reach CD player');
  await key(page,'Enter',700);
  // The label is installed before eight rotation and fifteen bounce frames.
  // Five zero-angle presentations extend past the four final bounce entries.
  await page.waitForFunction(()=>HEAP32[0x940010>>2]===0x469e2c && __cdTest.zeroSlabFrames>=5);
  async function cdKey(code,post=320){
   const masks={Enter:0x4000,ArrowRight:0x20,ArrowLeft:0x80};
   await page.evaluate(c=>window.dispatchEvent(new KeyboardEvent('keydown',{code:c})),code);
   await page.waitForFunction(mask=>(HEAPU16[0x754448>>1]&mask)!==0,masks[code]);
   await page.evaluate(c=>window.dispatchEvent(new KeyboardEvent('keyup',{code:c})),code);
   await page.waitForFunction(mask=>(HEAPU16[0x754448>>1]&mask)===0,masks[code]);
   await page.waitForTimeout(post);
  }
  const initialSelection=await page.evaluate(()=>HEAP32[0x469efc>>2]);
  assert(initialSelection>=0 && initialSelection<=16,'invalid initial CD selection');
  // The original image defaults to index 11. Reach track 2 through real Prev actions.
  for(let i=initialSelection;i>0;i--){
   await cdKey('Enter');
   await page.waitForFunction(index=>HEAP32[0x469efc>>2]===index,i-1);
  }
  assert(await page.evaluate(()=>HEAP32[0x469efc>>2])===0,'Prev actions did not reach first audio track');
  await cdKey('Enter',320);
  assert(await page.evaluate(()=>HEAP32[0x469efc>>2])===0,'Prev moved below first audio track');
  await cdKey('ArrowRight',700);
  assert((await rd(page,0x469d64)).includes('Play'),'Play category missing');
  async function playAndCompare(track){
   await cdKey('Enter',800);
   try { await page.waitForFunction(t=>window.__cdTest.track===t && window.__cdTest.frames>=80000,track,{timeout:8000}); }
   catch(error){console.log('CD failure state:',await page.evaluate(()=>({track:__cdTest.track,frames:__cdTest.frames,total:__cdTest.totalBuffers,plays:__cdTest.plays,ac:Module._dd2ac&&Module._dd2ac.state,enabled:HEAP32[0x462d74>>2],playing:HEAP32[0x462d70>>2],from:HEAP32[0x74f174>>2],env:ENV.DD2_CDLOG})));throw error;}
   const capture=await page.evaluate(()=>{
    const state=window.__cdTest,size=state.parts.reduce((sum,p)=>sum+p.length,0),bytes=new Uint8Array(size);
    let offset=0;for(const part of state.parts){bytes.set(part,offset);offset+=part.length;}
    let string='';for(let i=0;i<bytes.length;i++)string+=String.fromCharCode(bytes[i]);
    return {track:state.track,frames:state.frames,pcm:btoa(string),plays:state.plays,rate:Module._dd2ac.sampleRate};
   });
   const actual=Buffer.from(capture.pcm,'base64');
   const source=fs.readFileSync(path.join(build,'Redbook',`track${String(track).padStart(2,'0')}.cdda`));
   assert(actual.equals(source.subarray(0,actual.length)),`track ${track}: C music summand differs from CDDA`);
   assert(actual.some(byte=>byte!==0),'captured only silence');
   assert(capture.plays.at(-1).frame===0,'Play did not restart at the track beginning');
   assert(capture.rate===44100,'CD AudioContext did not preserve 44100Hz');
   fs.writeFileSync(path.join(output,`track${track}.pcm`),actual);
   report.tracks.push({track,bytes:actual.length,sha256:createHash('sha256').update(actual).digest('hex')});
   console.log(`PASS browser track${track}: ${actual.length} exact CD source bytes in the shared mixer`);
  }
  await playAndCompare(2);
  await cdKey('ArrowRight',700);await cdKey('Enter',700);
  const stopped=await page.evaluate(()=>({playing:HEAP32[0x462d70>>2],buffers:__cdTest.musicBuffers}));
  assert(stopped.playing===0,'Stop did not stop the engine CD device');
  await page.waitForTimeout(400);
  assert(await page.evaluate(()=>__cdTest.musicBuffers)===stopped.buffers,'CD still submits audio after Stop');
  await cdKey('ArrowRight',700);await cdKey('Enter',700);
  assert(await page.evaluate(()=>HEAP32[0x469efc>>2])===1,'Next Track did not change selection');
  await cdKey('ArrowLeft',400);await cdKey('ArrowLeft',400);
  await playAndCompare(3);
  await cdKey('ArrowLeft',400);await cdKey('Enter',700);
  assert(await page.evaluate(()=>HEAP32[0x469efc>>2])===0,'Prev Track did not change selection');
  assert(await page.evaluate(()=>__cdTest.plays.at(-1).track)===3,'selection unexpectedly changed playing track');
  await cdKey('ArrowRight',400);await cdKey('ArrowRight',400);await cdKey('Enter',700);
  await page.screenshot({path:path.join(output,'stopped.png')});
  const sink=await page.evaluate(()=>({mismatches:__cdTest.mismatches,missing:__cdTest.missingBuffers,other:__cdTest.otherBuffers,buffers:__cdTest.totalBuffers,separateCursor:Module._dd2cdt!==undefined}));
  sink.priorOtherBuffers=priorOtherBuffers;
  sink.otherDuringMenu=sink.other-priorOtherBuffers;
  report.sink=sink;
  assert(sink.buffers>0 && sink.mismatches===0 && sink.missing===0 && sink.otherDuringMenu===0 && !sink.separateCursor,'shared WebAudio output differs from the combined C mixer or uses a second sink');
  console.log('Shared WebAudio sink:',JSON.stringify(sink));
  assert.deepEqual(errors,[],'browser runtime errors');
  report.pass=true;
  console.log('PASS CD menu Play/Stop/Next/Prev; no runtime errors');
 }catch(error){report.error=error.message;throw error;}
 finally{
  if(browser)await browser.close();await new Promise(resolve=>server.close(resolve));
  fs.writeFileSync(path.join(output,'report.json'),JSON.stringify(report,null,2)+'\n');
  if(report.pass)for(const name of ['track2.pcm','track3.pcm','stopped.png'])fs.unlinkSync(path.join(output,name));
 }
})().catch(error=>{console.error(error);process.exitCode=1;});
