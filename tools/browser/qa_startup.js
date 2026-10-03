// Normal original WinMain/ddmain entry, with full/skip and required gestures.
// Read-only sink observations; no engine writes or alternate startup ENV.
const fs=require('fs'),path=require('path'),assert=require('assert'),crypto=require('crypto');
const {serve,chromium,key,menuLabel,waitRace}=require('./felib');
const build=path.resolve(process.argv[2]||'web/dd2'),source=path.resolve(process.argv[3]),output=path.resolve(process.argv[4]);
fs.mkdirSync(output,{recursive:false});
const proof=JSON.parse(fs.readFileSync(path.join(source,'report.json'))),film=proof.films.find(f=>f.file.toUpperCase()==='INTRO.AVI');
const reference=fs.readFileSync(path.join(source,'intro/wine.pcm'));
assert(crypto.createHash('sha256').update(reference).digest('hex')===film.pcm_sha256,'changed actual Wine reference');
(async()=>{
 const server=serve(build);await new Promise(r=>server.listen(0,r));
 const report={scope:'Normal original startup, complete/skip/gesture intro, exact submitted source PCM/canvas/shared-mix bytes and real menu/race input; original complete output clocks and physical sinks remain open',cases:[]};
 try{
  for(const mode of ['gesture','full','skip']){
   const browser=await chromium.launch({args:['--no-sandbox',...(mode==='gesture'?[
    '--autoplay-policy=document-user-activation-required',
    '--disable-features=PreloadMediaEngagementData,MediaEngagementBypassAutoplayPolicies']:[])]});
   try{
    const page=await browser.newPage(),errors=[];
    page.on('pageerror',e=>errors.push(e.message));page.on('crash',()=>errors.push('renderer crash'));
    await page.addInitScript(()=>{
     window.__startup={movieFrames:0,indexedFrames:0,pixels:0,pixelErrors:0,movieAudio:0,mixedBuffers:0,mixedFrames:0,audioErrors:0,mixedDuringMovie:0,contexts:[],pcm:null};
     const originalContext=window.AudioContext;
     window.AudioContext=class extends originalContext{
      constructor(...args){super(...args);this.__serial=__startup.contexts.length;__startup.contexts.push({rate:this.sampleRate,initialState:this.state,closed:false});}
      close(){__startup.contexts[this.__serial].closed=true;return super.close();}
     };
     let audio,movieFrame;
     const observe=imports=>{
      if(!imports?.env?.movie_audio_start)return;
      const movie=imports.env.movie_audio_start,mixed=imports.env.dd2_audio_push,present=imports.env.movie_present;
      imports.env.movie_audio_start=function(pointer,frames,rate,channels){
       audio={movie:true,pointer,frames,rate,channels};try{return movie(pointer,frames,rate,channels);}finally{audio=undefined;}
      };
      imports.env.dd2_audio_push=function(pointer,effects,music,frames,rate,musicFrames){
       audio={movie:false,pointer,frames,rate,channels:2};try{return mixed(pointer,effects,music,frames,rate,musicFrames);}finally{audio=undefined;}
      };
      imports.env.movie_present=function(pointer){movieFrame=pointer;try{return present(pointer);}finally{movieFrame=undefined;}};
     };
     for(const name of ['instantiate','instantiateStreaming']){
      const instantiate=WebAssembly[name];if(instantiate)WebAssembly[name]=function(bytes,imports,...rest){observe(imports);return instantiate.call(this,bytes,imports,...rest);};
     }
     const create=originalContext.prototype.createBufferSource;
     originalContext.prototype.createBufferSource=function(...args){
      const context=this,source=create.apply(this,args),start=source.start;
      source.start=function(...args){
       const state=__startup,b=source.buffer,e=audio;
       if(!e){state.audioErrors++;return start.apply(this,args);}
       let exact=b.length===e.frames && b.sampleRate===e.rate && context.sampleRate===e.rate && b.numberOfChannels===e.channels;
       const pcm=e.movie?new Int16Array(e.frames*e.channels):null;
       for(let ch=0;ch<e.channels;ch++){
        const data=b.getChannelData(ch),bits=new Uint32Array(data.buffer,data.byteOffset,data.length);
        for(let i=0;i<e.frames;i++){
         if(e.movie){const sample=HEAP16[(e.pointer>>1)+i*e.channels+ch];if(data[i]!==sample/32768)exact=false;pcm[i*e.channels+ch]=data[i]*32768;}
         else if(bits[i]!==HEAPU32[(e.pointer>>2)+i*e.channels+ch])exact=false;
        }
       }
       if(!exact)state.audioErrors++;
       if(e.movie){state.movieAudio++;state.pcm=new Uint8Array(pcm.buffer);}
       else{state.mixedBuffers++;state.mixedFrames+=e.frames;if(HEAP32[0x462cd4>>2]===1)state.mixedDuringMovie++;}
       return start.apply(this,args);
      };
      return source;
     };
     const put=CanvasRenderingContext2D.prototype.putImageData;
     CanvasRenderingContext2D.prototype.putImageData=function(...args){
      const result=put.apply(this,args);
      if(this.canvas.id==='canvas' && typeof HEAPU8!=='undefined'){
       const state=__startup,actual=this.getImageData(0,0,640,480).data;
       if(movieFrame!==undefined)state.movieFrames++;else state.indexedFrames++;
       for(let i=0;i<307200;i++){
        let r,g,b;
        if(movieFrame!==undefined){const c=HEAPU32[(movieFrame>>2)+i];r=(c>>>16)&255;g=(c>>>8)&255;b=c&255;}
        else{const p=0x700050+HEAPU8[0x700450+i]*4;r=HEAPU8[p];g=HEAPU8[p+1];b=HEAPU8[p+2];}
        if(actual[i*4]!==r || actual[i*4+1]!==g || actual[i*4+2]!==b || actual[i*4+3]!==255)state.pixelErrors++;
       }
       state.pixels+=307200;
      }
      return result;
     };
    });
    await page.goto(`http://localhost:${server.address().port}/index.html`);
    let heldForGesture=false;
    if(mode==='gesture'){
     // Playwright's evaluate/waitForFunction set Runtime userGesture:true.
     // Those can grant autoplay before the movie context is created. Poll
     // read-only CDP with userGesture:false until a real click is intended.
     const cdp=await page.context().newCDPSession(page);
     const read=async expression=>(await cdp.send('Runtime.evaluate',{expression,returnByValue:true,userGesture:false})).result.value;
     let ready=false;
     for(let i=0;i<150;i++){
      ready=await read("typeof HEAP32!=='undefined' && HEAP32[0x462cd4>>2]===1 && !!Module._dd2movieSource && __startup.movieFrames===1");
      if(ready)break;await page.waitForTimeout(200);
     }
     assert(ready,'normal gesture startup did not reach the first movie frame');
     assert(await read("Module._dd2movieAc.state==='suspended' && !navigator.userActivation.hasBeenActive"),'gesture fixture did not preserve actual suspended/unactivated state');
     await page.waitForTimeout(500);
     assert(await read('__startup.movieFrames')===1,'video advanced while movie audio clock awaited a gesture');
     await cdp.detach();
     heldForGesture=true;
    }else await page.waitForFunction(()=>typeof HEAP32!=='undefined' && HEAP32[0x462cd4>>2]===1 && !!Module._dd2movieSource,null,{timeout:30000});
    await page.click('#canvas');await page.waitForFunction(()=>Module._dd2movieAc.state==='running');
    if(mode!=='full'){
     await page.waitForFunction(()=>__startup.movieFrames>=26);
     await page.keyboard.up('Escape');await page.waitForTimeout(200);
     assert(await page.evaluate(()=>HEAP32[0x462cd4>>2])===1,'release skipped normal intro');
     await page.keyboard.press('Escape');
    }
    await page.waitForFunction(()=>HEAP32[0x462cd4>>2]===0 && !Module._dd2movieSource,null,{timeout:100000});
    await page.waitForFunction(()=>HEAP32[0x462d68>>2]===1 && HEAP32[0x936ff4>>2]===0 && __startup.indexedFrames>=200 && __startup.mixedBuffers>10,null,{timeout:30000});
    await page.waitForTimeout(1000);
    assert((await menuLabel(page)).includes('Wrecking'),'normal startup did not reach the real main menu');
    await key(page,'ArrowRight',420);assert((await menuLabel(page)).includes('Select Car'),'main menu Right did not navigate');
    await key(page,'ArrowLeft',420);assert((await menuLabel(page)).includes('Wrecking'),'main menu Left did not navigate');
    await key(page,'ArrowDown',420);await key(page,'ArrowDown',420);
    assert((await menuLabel(page)).includes('Go!'),'main menu did not select Go!');
    await key(page,'Enter',420);const race=await waitRace(page);assert(race.launched && race.nc===20,'normal frontend could not launch a populated race');
    const result=await page.evaluate(()=>{
     let text='';for(let i=0;i<__startup.pcm.length;i+=16384)text+=String.fromCharCode(...__startup.pcm.subarray(i,i+16384));
     return {...__startup,pcm:btoa(text),moviePlaying:HEAP32[0x462cd4>>2],drawMode:HEAP32[0x463010>>2],soundReady:HEAP32[0x462d68>>2],mixedRate:Module._dd2ac.sampleRate};
    });
    const pcm=Buffer.from(result.pcm,'base64');assert(pcm.equals(reference),'normal intro WebAudio source differs from complete actual Wine PCM');delete result.pcm;
    assert(result.movieAudio===1 && result.audioErrors===0 && result.pixelErrors===0 && result.mixedDuringMovie===0,'normal startup sink bytes or device epochs differ');
    assert(mode==='full'?result.movieFrames===film.metadata.frames:result.movieFrames>=26 && result.movieFrames<film.metadata.frames,'incorrect normal intro extent');
    assert(result.drawMode===0 && result.soundReady===1 && result.mixedRate===44100 && result.moviePlaying===0,'normal movie-to-game device transition failed');
    const movies=result.contexts.filter(c=>c.rate===22050);assert(movies.length===1 && movies[0].closed,'normal movie context was not closed');
    assert.deepEqual(errors,[],'normal browser startup runtime errors');
    result.mode=mode;result.pcmBytes=pcm.length;result.heldForGesture=heldForGesture;result.race=race;report.cases.push(result);
    fs.writeFileSync(path.join(output,'report.json'),JSON.stringify(report,null,2)+'\n');
    await page.screenshot({path:path.join(output,`${mode}-race.png`)});
    console.log(`PASS browser normal ${mode}: ${result.movieFrames} intro frames, ${result.pixels} exact pixels, ${pcm.length} exact source PCM bytes, ${result.mixedBuffers} exact shared buffers and live race`);
   }finally{await browser.close();}
  }
 }finally{await new Promise(r=>server.close(r));}
})().catch(e=>{console.error(e);process.exitCode=1});
