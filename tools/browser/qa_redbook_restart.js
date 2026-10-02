// Live race keys -> original CD_Pause/Restart -> sector source -> shared WebAudio.
// Read-only observations; physical DAC and original queued-control timing remain open.
const assert=require('assert'),fs=require('fs'),path=require('path');
const {serve,chromium}=require('./felib');
const build=path.resolve(process.argv[2]||'web/dd2');
const output=process.argv[3]?path.resolve(process.argv[3]):fs.mkdtempSync('/tmp/dd2-browser-restart-');
fs.mkdirSync(output,{recursive:true});
if(fs.readdirSync(output).length)throw new Error('Use an empty output directory');
(async()=>{
 const server=serve(build);await new Promise(resolve=>server.listen(0,resolve));
 let browser;
 try{
  browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage(),errors=[];
  page.on('pageerror',e=>errors.push(e.message));page.on('crash',()=>errors.push('renderer crash'));
  page.on('console',m=>{if(m.text().includes('[CD]'))console.log(m.text());});
  await page.addInitScript(()=>{
   window.__restart={plays:[],stops:[],parts:[],frames:0,musicBuffers:0,buffers:0,mismatches:0,missing:0,other:0};
   window.__releaseKey=null;
   const present=CanvasRenderingContext2D.prototype.putImageData;
   CanvasRenderingContext2D.prototype.putImageData=function(...args){
    const result=present.apply(this,args);
    if(this.canvas.id==='canvas' && __releaseKey){const code=__releaseKey;__releaseKey=null;window.dispatchEvent(new KeyboardEvent('keyup',{code}));}
    return result;
   };
   const log=console.log;
   console.log=function(...args){
    const message=args.join(' '),play=message.match(/\[CD\] play track=(\d+) frame=(\d+)/),stop=message.match(/\[CD\] stop track=(\d+) frame=(\d+)/);
    if(play){__restart.plays.push({track:+play[1],frame:+play[2]});__restart.parts=[];__restart.frames=0;}
    if(stop)__restart.stops.push({track:+stop[1],frame:+stop[2]});
    return log.apply(this,args);
   };
   let expected;
   const observe=imports=>{
    if(!imports?.env?.dd2_audio_push || imports.env.dd2_audio_push.__restartObserved)return;
    const push=imports.env.dd2_audio_push;
    const wrapper=function(pointer,effects,music,frames,rate,musicFrames){
     expected={pointer,music,frames,rate,musicFrames,starts:0};
     try{return push.call(this,pointer,effects,music,frames,rate,musicFrames);}finally{
      if(Module._dd2ac?.state==='running' && expected.starts!==1)__restart.missing++;
      expected=undefined;
     }
    };
    wrapper.__restartObserved=true;imports.env.dd2_audio_push=wrapper;
   };
   for(const name of ['instantiate','instantiateStreaming']){
    const instantiate=WebAssembly[name];
    if(instantiate)WebAssembly[name]=function(bytes,imports,...rest){observe(imports);return instantiate.call(this,bytes,imports,...rest);};
   }
   const create=AudioContext.prototype.createBufferSource;
   AudioContext.prototype.createBufferSource=function(...args){
    const source=create.apply(this,args),start=source.start;
    source.start=function(...args){
     const state=__restart,buffer=source.buffer;
     if(buffer && expected){
      expected.starts++;state.buffers++;
      let exact=buffer.length===expected.frames && buffer.sampleRate===expected.rate && buffer.numberOfChannels===2;
      for(let ch=0;ch<2;ch++){
       const samples=buffer.getChannelData(ch),bits=new Uint32Array(samples.buffer,samples.byteOffset,samples.length);
       for(let i=0;i<buffer.length;i++)if(bits[i]!==HEAPU32[(expected.pointer>>2)+i*2+ch])exact=false;
      }
      if(!exact)state.mismatches++;
      if(expected.musicFrames){
       state.musicBuffers++;
       if(state.plays.length>=2 && state.frames<16000){
        const pcm=new Int16Array(expected.musicFrames*2);
        for(let i=0;i<pcm.length;i++){
         const sample=HEAPF32[(expected.music>>2)+i]*32768;
         if(sample!==Math.round(sample) || sample<-32768 || sample>32767)state.mismatches++;
         pcm[i]=sample;
        }
        state.parts.push(new Uint8Array(pcm.buffer));state.frames+=expected.musicFrames;
       }
      }
     }else if(buffer)state.other++;
     return start.apply(this,args);
    };
    return source;
   };
  });
  await page.goto(`http://localhost:${server.address().port}/index.html?race=1&cdlog`,{waitUntil:'load'});
  await page.waitForFunction(()=>typeof HEAP32!=='undefined' && HEAP32[0x936ff4>>2]===1 && HEAP32[0x784298>>2]<0,null,{timeout:30000});
  await page.click('#canvas');await page.waitForFunction(()=>Module._dd2ac?.state==='running');
  async function tap(code){
   await page.evaluate(code=>{__releaseKey=code;window.dispatchEvent(new KeyboardEvent('keydown',{code}));},code);
   await page.waitForFunction(()=>__releaseKey===null,null,{timeout:3000});await page.waitForTimeout(250);
  }
  let stopped,paused;
  for(let attempt=0;attempt<3;attempt++){
   const before=await page.evaluate(()=>HEAP32[0x462ff0>>2]);
   await page.waitForFunction(cf=>HEAP32[0x462ff0>>2]>=cf+17,before,{timeout:10000});
   const count=await page.evaluate(()=>__restart.stops.length);
   await tap('Escape');await page.waitForFunction(n=>__restart.stops.length>n,count);
   stopped=await page.evaluate(()=>__restart.stops.at(-1));
   const state=await page.evaluate(()=>({cf:HEAP32[0x462ff0>>2],playing:HEAP32[0x462d70>>2],buffers:__restart.musicBuffers}));
   assert(state.playing===0,'Pause did not issue the original CD Stop');paused=state.cf;
   await page.waitForTimeout(400);
   assert(await page.evaluate(()=>HEAP32[0x462ff0>>2])===paused,'race counter did not pause');
   assert(await page.evaluate(()=>__restart.musicBuffers)===state.buffers,'CD advanced during pause');
   await tap('Enter');await page.waitForFunction(cf=>HEAP32[0x462ff0>>2]>cf,paused);
   if(stopped.frame%588)break;
   stopped=undefined;
  }
  assert(stopped,'did not exercise a fractional stopped sector');
  const resumed=await page.evaluate(()=>__restart.plays.at(-1));
  assert(resumed.track===stopped.track && resumed.frame===Math.floor(stopped.frame/588)*588,'live restart retained fractional samples or restarted a different track');
  await page.waitForFunction(()=>__restart.frames>=2352,null,{timeout:5000});
  const report=await page.evaluate(()=>{
   const bytes=new Uint8Array(__restart.parts.reduce((n,p)=>n+p.length,0));
   let offset=0;for(const part of __restart.parts){bytes.set(part,offset);offset+=part.length;}
   let text='';for(let i=0;i<bytes.length;i+=16384)text+=String.fromCharCode(...bytes.subarray(i,i+16384));
   return {plays:__restart.plays,stops:__restart.stops,frames:__restart.frames,buffers:__restart.buffers,mismatches:__restart.mismatches,
    missing:__restart.missing,other:__restart.other,rate:Module._dd2ac.sampleRate,pcm:btoa(text),separateCursor:Module._dd2cdt!==undefined};
  });
  const actual=Buffer.from(report.pcm,'base64'),raw=fs.readFileSync(path.join(build,'Redbook',`track${String(resumed.track).padStart(2,'0')}.cdda`));
  assert(actual.equals(raw.subarray(resumed.frame*4,resumed.frame*4+actual.length)),'actual resumed music summand differs from the public CD sector');
  assert(report.rate===44100 && report.buffers>0 && report.mismatches===0 && report.missing===0 && report.other===0 && !report.separateCursor,'shared WebAudio output differs from C');
  assert.deepEqual(errors,[],'browser runtime errors');
  delete report.pcm;report.stopped=stopped;report.resumed=resumed;report.verified_source_bytes=actual.length;
  report.scope='actual browser keys/live race/sector restart and shared accepted WebAudio; original transport timing and hardware pending';
  fs.writeFileSync(path.join(output,'resumed-source.pcm'),actual);fs.writeFileSync(path.join(output,'report.json'),JSON.stringify(report,null,2)+'\n');
  await page.screenshot({path:path.join(output,'resumed.png')});
  console.log('PASS browser live Pause/Resume:',JSON.stringify(report));
 }finally{if(browser)await browser.close();await new Promise(resolve=>server.close(resolve));}
})().catch(error=>{console.error(error);process.exitCode=1;});
