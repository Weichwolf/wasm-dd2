// Actual original engine Play_Movie -> movie AudioBuffer/canvas sinks.
// All accepted PCM samples and every canvas pixel are checked directly.
const fs=require('fs'),path=require('path'),assert=require('assert'),crypto=require('crypto');
const {serve,chromium}=require('./felib');
const build=path.resolve(process.argv[2]||'web/dd2'),source=path.resolve(process.argv[3]),output=path.resolve(process.argv[4]);
const capturePCM=process.argv.includes('--capture-pcm');
const originalPaths={};
for(const [file,name] of [['Intro.avi','original-intro'],['Outro.avi','original-outro']]){
 const option=process.argv.find(s=>s.startsWith('--'+name+'='));
 if(option)originalPaths[file]=path.resolve(option.slice(name.length+3));
}
fs.mkdirSync(output,{recursive:false});
const originals=JSON.parse(fs.readFileSync(path.join(source,'report.json')));
(async()=>{
 const server=serve(build);await new Promise(r=>server.listen(0,r));let browser;
 const report={scope:'Actual browser engine MCI movie/canvas/22050Hz WebAudio source output and reset before successful notification; original final clocks and physical DAC/display remain open',wasm_sha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(build,'index.wasm'))).digest('hex'),cases:[]};
 try{
  browser=await chromium.launch({args:['--no-sandbox']});
  for(const film of originals.films)for(const skip of [false,true]){
   let referenceFrames=null,originalManifestSha256=null;
   if(originalPaths[film.file]){
    const directory=originalPaths[film.file],outro=film.file==='Outro.avi';
    const provenance=JSON.parse(fs.readFileSync(path.join(directory,outro?'report.json':'checkpoint.json')));
    const bytes=fs.readFileSync(path.join(directory,'movie-video-archive/manifest.json'));
    const manifest=JSON.parse(bytes),begin=outro?provenance.outro_archive_begin:0;
    assert(provenance.exe_modified===false&&provenance.exe_sha256==='0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2', 'original executable provenance differs');
    assert(outro?provenance.pass_&&provenance.original_process_exited:provenance.end_state.movie===0,'original movie incomplete');
    assert(manifest.pass_&&Number.isInteger(begin)&&begin>=0,'original window manifest incomplete');
    referenceFrames=manifest.records.slice(begin).map(r=>r.window_argb_sha256);
    assert(referenceFrames.length===film.metadata.frames-1&&referenceFrames.every(h=>/^[0-9a-f]{64}$/.test(h)), 'original chronological frame extent differs');
    originalManifestSha256=crypto.createHash('sha256').update(bytes).digest('hex');
   }
   const page=await browser.newPage(),errors=[];
   page.on('pageerror',e=>errors.push(e.message));page.on('crash',()=>errors.push('renderer crash'));
   await page.addInitScript(()=>{
    window.__movie={frames:0,pixels:0,pixelErrors:0,audioBuffers:0,audioErrors:0,pcm:[],rates:[],stops:0,resets:[],canvasHashes:[]};
    window.__movieContexts=[];
    const observed=new WeakSet();
    let audio,frame;
    function observe(imports){
     if(!imports?.env?.movie_audio_start || observed.has(imports.env))return;
     observed.add(imports.env);
     const a=imports.env.movie_audio_start,p=imports.env.movie_present,s=imports.env.movie_audio_stop;
     imports.env.movie_audio_start=function(pointer,frames,rate,channels){
      audio={pointer,frames,rate,channels};
      try{return a(pointer,frames,rate,channels);}finally{audio=undefined;}
     };
     imports.env.movie_present=function(pointer){
      frame={pointer};try{return p(pointer);}finally{frame=undefined;}
     };
     imports.env.movie_audio_stop=function(...args){
      const reset={moviePlaying:HEAP32[0x462cd4>>2],sourcePresent:!!Module._dd2movieSource};
      try{return s(...args);}finally{
       reset.sourceCleared=!Module._dd2movieSource;__movie.resets.push(reset);
      }
     };
    }
    for(const name of ['instantiate','instantiateStreaming']){
     const create=WebAssembly[name];if(create)WebAssembly[name]=function(bytes,imports,...rest){observe(imports);return create.call(this,bytes,imports,...rest);};
    }
    const create=AudioContext.prototype.createBufferSource;
    AudioContext.prototype.createBufferSource=function(...args){
     const context=this,source=create.apply(this,args),start=source.start,stop=source.stop;
     let isMovie=false;
     source.start=function(...args){
      if(audio){
       isMovie=true;
       __movieContexts.push(context);
       const state=__movie,b=source.buffer,e=audio;state.audioBuffers++;state.rates.push(context.sampleRate);
       let exact=b.length===e.frames && b.sampleRate===e.rate && context.sampleRate===e.rate && b.numberOfChannels===e.channels;
       const pcm=new Int16Array(e.frames*e.channels);
       for(let ch=0;ch<e.channels;ch++){
        const samples=b.getChannelData(ch);
        for(let i=0;i<e.frames;i++){
         const original=HEAP16[(e.pointer>>1)+i*e.channels+ch];
         if(samples[i]!==original/32768)exact=false;
         pcm[i*e.channels+ch]=samples[i]*32768;
        }
       }
       if(!exact)state.audioErrors++;state.pcm=Array.from(new Uint8Array(pcm.buffer));
      }
      return start.apply(this,args);
     };
     source.stop=function(...args){if(isMovie)__movie.stops++;return stop.apply(this,args);};
     return source;
    };
    const put=CanvasRenderingContext2D.prototype.putImageData;
    CanvasRenderingContext2D.prototype.putImageData=function(...args){
     const result=put.apply(this,args);
     if(frame && this.canvas.id==='canvas'){
      const state=__movie,actual=this.getImageData(0,0,640,480).data,index=state.frames++;state.canvasHashes.push(null);
      const argb=new Uint8Array(actual.length);
      for(let i=0;i<640*480;i++){
       const value=HEAPU32[(frame.pointer>>2)+i],p=i*4;
       if(actual[p]!==((value>>>16)&255) || actual[p+1]!==((value>>>8)&255) || actual[p+2]!== (value&255) || actual[p+3]!==255)state.pixelErrors++;
       argb[p]=actual[p+2];argb[p+1]=actual[p+1];argb[p+2]=actual[p];argb[p+3]=actual[p+3];
      }
      crypto.subtle.digest('SHA-256',argb).then(hash=>{
       state.canvasHashes[index]=Array.from(new Uint8Array(hash),v=>v.toString(16).padStart(2,'0')).join('');
      });
      state.pixels+=640*480;
     }
     return result;
    };
   });
   await page.goto(`http://localhost:${server.address().port}/index.html?movie=${encodeURIComponent(film.file.toUpperCase())}`);
   await page.waitForFunction(()=>!!Module._dd2movieSource && HEAP32[0x462cd4>>2]===1,null,{timeout:15000});
   await page.click('#canvas');
   if(skip){
    await page.waitForFunction(()=>__movie.frames>=26);
    await page.keyboard.up('Escape');await page.waitForTimeout(200);
    assert(await page.evaluate(()=>HEAP32[0x462cd4>>2])===1,'key-up skipped movie');
    await page.keyboard.down('d');
   }
   await page.waitForFunction(()=>HEAP32[0x462cd4>>2]===0 && !Module._dd2movieSource &&
                              __movieContexts.length===1 && __movieContexts[0].state==='closed',null,{timeout:100000});
   await page.waitForFunction(()=>__movie.canvasHashes.every(h=>h!==null));
   const result=await page.evaluate(()=>__movie);
   const diagnosis={...result};delete diagnosis.pcm;
   fs.writeFileSync(path.join(output,`${path.parse(film.file).name.toLowerCase()}-${skip?'skip':'full'}.json`),JSON.stringify(diagnosis,null,2)+'\n');
   assert(!errors.length,JSON.stringify(errors));assert(result.audioBuffers===1 && result.audioErrors===0 && result.pixelErrors===0,'actual movie sink bytes differ');
   assert(result.rates.length===1 && result.rates[0]===22050,'movie introduced output resampling');
   assert(result.resets.length===(skip?1:2) && result.resets[0].sourcePresent &&
          result.resets[0].sourceCleared && result.resets[0].moviePlaying===(skip?0:1),
          'movie audio not reset before completion notification');
   assert(result.stops===1 && result.resets.every(r=>r.sourceCleared), 'idempotent device reset failed');
   assert(skip ? result.frames>=26 && result.frames<film.metadata.frames-1 : result.frames===film.metadata.frames-1,'wrong actual default MCI movie frame extent');
   if(referenceFrames){
    assert.deepStrictEqual(result.canvasHashes,referenceFrames.slice(0,result.frames),'actual canvas differs from retained original chronological hashes');
    result.originalManifestSha256=originalManifestSha256;result.matchedOriginalFrames=result.frames;
    const changed=[...result.canvasHashes];changed[0]='0'.repeat(64);
    assert.notDeepStrictEqual(changed,referenceFrames.slice(0,result.frames),'changed frame digest accepted');
   }
   const pcm=Buffer.from(result.pcm),reference=fs.readFileSync(path.join(source,path.parse(film.file).name.toLowerCase(),'wine.pcm'));
   assert(crypto.createHash('sha256').update(reference).digest('hex')===film.pcm_sha256,'changed actual source PCM');
   assert(pcm.equals(reference),'actual WebAudio movie source differs from whole Wine ACM PCM');
   delete result.pcm;result.file=film.file;result.skip=skip;result.pcmBytes=pcm.length;
   result.acceptedPcmSha256=crypto.createHash('sha256').update(pcm).digest('hex');
   if(capturePCM){
    result.pcmFile=`${path.parse(film.file).name.toLowerCase()}-${skip?'skip':'full'}.pcm`;
    fs.writeFileSync(path.join(output,result.pcmFile),pcm);
   }
   result.keyUpRetained=skip;result.keyDownClosed=skip;
   result.movieContextClosed=true;result.resetBeforeSuccessfulNotify=!skip;
   report.cases.push(result);fs.writeFileSync(path.join(output,'report.json'),JSON.stringify(report,null,2)+'\n');
   await page.screenshot({path:path.join(output,`${path.parse(film.file).name.toLowerCase()}-${skip?'skip':'full'}.png`)});
   console.log(`PASS browser ${film.file} skip=${skip}: ${result.frames} actual canvas frames, ${result.pixels} exact pixels, ${pcm.length} exact accepted PCM bytes at 22050Hz`);
   await page.close();
  }
 }finally{if(browser)await browser.close();await new Promise(r=>server.close(r));}
})().catch(e=>{console.error(e);process.exitCode=1});
