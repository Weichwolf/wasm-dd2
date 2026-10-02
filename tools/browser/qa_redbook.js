// Navigate the actual CD-player menu and verify delivered WebAudio PCM, not labels alone.
const fs=require('fs'),path=require('path'),assert=require('assert');
const {serve,key,rd,boot,gotoButton,chromium}=require('./felib');
const build=path.resolve(process.argv[2]||'web/dd2');
const output='/tmp/dd2-browser-redbook';fs.mkdirSync(output,{recursive:true});
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
   window.__cdTest={track:0,parts:[],frames:0,totalBuffers:0,plays:[]};
   let effects=false;
   const observeImports=imports=>{
    if(!imports || !imports.env || !imports.env.dd2_audio_push || imports.env.dd2_audio_push.__cdObserved)return;
    const original=imports.env.dd2_audio_push;
    const observed=function(...args){effects=true;try{return original.apply(this,args);}finally{effects=false;}};
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
     if(buffer && buffer.sampleRate===44100 && !effects){
      state.totalBuffers++;
      if(state.frames<100000){
       const left=buffer.getChannelData(0),right=buffer.getChannelData(1);
       const pcm=new Int16Array(buffer.length*2);
       for(let i=0;i<buffer.length;i++){pcm[i*2]=Math.round(left[i]*32768);pcm[i*2+1]=Math.round(right[i]*32768);}
       state.parts.push(new Uint8Array(pcm.buffer));state.frames+=buffer.length;
      }
     }
     return start.apply(this,args);
    };
    return source;
   };
  });
  // Same boot helper, with MCI diagnostics enabled before libc creates its environment.
  const originalGoto=page.goto.bind(page);
  page.goto=(url,options)=>originalGoto(url+'&cdlog',options);
  await boot(page,server);
  assert(await gotoButton(page,'CD Audio Player'),'cannot reach CD player');
  await key(page,'Enter',700);
  const initialSelection=await page.evaluate(()=>HEAP32[0x469efc>>2]);
  assert(initialSelection>=0 && initialSelection<=16,'invalid initial CD selection');
  // The original image defaults to index 11. Reach track 2 through real Prev actions.
  for(let i=0;i<initialSelection;i++) await key(page,'Enter',320);
  assert(await page.evaluate(()=>HEAP32[0x469efc>>2])===0,'Prev actions did not reach first audio track');
  await key(page,'Enter',320);
  assert(await page.evaluate(()=>HEAP32[0x469efc>>2])===0,'Prev moved below first audio track');
  await key(page,'ArrowRight',700);
  assert((await rd(page,0x469d64)).includes('Play'),'Play category missing');
  async function playAndCompare(track){
   await key(page,'Enter',800);
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
   assert(actual.equals(source.subarray(0,actual.length)),`track ${track}: delivered WebAudio PCM differs from CDDA`);
   assert(actual.some(byte=>byte!==0),'captured only silence');
   assert(capture.plays.at(-1).frame===0,'Play did not restart at the track beginning');
   assert(capture.rate===44100,'CD AudioContext did not preserve 44100Hz');
   fs.writeFileSync(path.join(output,`track${track}.pcm`),actual);
   console.log(`PASS browser track${track}: ${actual.length} exact PCM bytes delivered to WebAudio`);
  }
  await playAndCompare(2);
  await key(page,'ArrowRight',700);await key(page,'Enter',700);
  const stopped=await page.evaluate(()=>({playing:HEAP32[0x462d70>>2],sources:Module._dd2cdsources.length,buffers:__cdTest.totalBuffers}));
  assert(stopped.playing===0 && stopped.sources===0,'Stop did not stop the engine/device sources');
  await page.waitForTimeout(400);
  assert(await page.evaluate(()=>__cdTest.totalBuffers)===stopped.buffers,'CD still submits audio after Stop');
  await key(page,'ArrowRight',700);await key(page,'Enter',700);
  assert(await page.evaluate(()=>HEAP32[0x469efc>>2])===1,'Next Track did not change selection');
  await key(page,'ArrowLeft',400);await key(page,'ArrowLeft',400);
  await playAndCompare(3);
  await key(page,'ArrowLeft',400);await key(page,'Enter',700);
  assert(await page.evaluate(()=>HEAP32[0x469efc>>2])===0,'Prev Track did not change selection');
  assert(await page.evaluate(()=>__cdTest.plays.at(-1).track)===3,'selection unexpectedly changed playing track');
  await key(page,'ArrowRight',400);await key(page,'ArrowRight',400);await key(page,'Enter',700);
  await page.screenshot({path:path.join(output,'stopped.png')});
  assert.deepEqual(errors,[],'browser runtime errors');
  console.log('PASS CD menu Play/Stop/Next/Prev; no runtime errors');
 }finally{if(browser)await browser.close();await new Promise(resolve=>server.close(resolve));}
})().catch(error=>{console.error(error);process.exitCode=1;});
