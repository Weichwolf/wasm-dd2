// Run the actual browser engine with observed original clock/audio services.
// Recorded control values are assertions in C; no engine state is supplied.
const fs=require('fs'),path=require('path'),assert=require('assert'),crypto=require('crypto');
const {serve,chromium}=require('./felib');
const [buildArg,outputArg,servicesArg,clockArg]=process.argv.slice(2);
assert(process.argv.length===6,'usage: capture_engine_audio.js BUILD OUTPUT SERVICES CLOCK');
const build=path.resolve(buildArg),output=path.resolve(outputArg);
const services=fs.readFileSync(path.resolve(servicesArg)),clock=fs.readFileSync(path.resolve(clockArg));
const save=fs.readFileSync(path.resolve(__dirname,'../../DestructionDerby2/SaveGames'));
const hash=bytes=>crypto.createHash('sha256').update(bytes).digest('hex');
assert(output.startsWith('/tmp/wasm-dd2/'),'diagnostics must be under /tmp/wasm-dd2/');
fs.mkdirSync(output,{recursive:false});
(async()=>{
 const server=serve(build);await new Promise(resolve=>server.listen(0,resolve));let browser;
 try{
  browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage(),errors=[];
  page.on('pageerror',error=>errors.push(error.message));page.on('crash',()=>errors.push('renderer crash'));
  page.on('console',message=>{
   const text=message.text();fs.appendFileSync(path.join(output,'browser.log'),message.type()+': '+text+'\n');
   if(text.includes('DD2_AUDIO_SERVICES:') || text.includes('[clock-replay] clock input exhausted') || text.includes('program exited (with status: 1)')){
    errors.push(text);page.evaluate(text=>{window.__engineAudioError=text;},text).catch(()=>{});
   }
  });
  await page.addInitScript(()=>{
   window.__engineAudio={paused:false,parts:[],frames:0,buffers:0,mismatches:0,missing:0,dropped:0};
   let expected;
   const observe=imports=>{
    if(!imports?.env?.dd2_audio_push || imports.env.dd2_audio_push.__engineObserved)return;
    const push=imports.env.dd2_audio_push;
    const wrapped=function(pointer,effects,music,frames,rate,musicFrames){
     __engineAudio.parts.push(HEAPU8.slice(pointer,pointer+frames*8));__engineAudio.frames+=frames;
     expected={pointer,frames,rate,starts:0};
     try{return push.call(this,pointer,effects,music,frames,rate,musicFrames);}finally{
      if(Module._dd2ac?.state==='running' && expected.starts!==1)__engineAudio.missing++;
      if(Module._dd2ac?.state!=='running')__engineAudio.dropped++;
      expected=undefined;
     }
    };
    wrapped.__engineObserved=true;imports.env.dd2_audio_push=wrapped;
   };
   for(const name of ['instantiate','instantiateStreaming']){
    const instantiate=WebAssembly[name];if(instantiate)WebAssembly[name]=function(bytes,imports,...rest){
     observe(imports);return instantiate.call(this,bytes,imports,...rest);
    };
   }
   const create=AudioContext.prototype.createBufferSource;
   AudioContext.prototype.createBufferSource=function(...args){
    const source=create.apply(this,args),start=source.start;
    source.start=function(...args){
     if(expected){
      expected.starts++;__engineAudio.buffers++;
      const buffer=source.buffer;
      let exact=!!buffer && buffer.length===expected.frames && buffer.sampleRate===expected.rate && buffer.numberOfChannels===2;
      if(exact)for(let ch=0;ch<2;ch++){
       const samples=buffer.getChannelData(ch),bits=new Uint32Array(samples.buffer,samples.byteOffset,samples.length);
       for(let i=0;i<samples.length;i++)if(bits[i]!==HEAPU32[(expected.pointer>>2)+i*2+ch])exact=false;
      }
      if(!exact)__engineAudio.mismatches++;
     }
     return start.apply(this,args);
    };return source;
   };
   // Stop observation at the next ordinary Asyncify yield after the bounded
   // input completes, before the application can request further services.
   const schedule=window.setTimeout;
   window.setTimeout=function(callback,delay,...args){
    if(typeof Module!=='undefined' && Module.FS?.analyzePath('/tmp/wasm-dd2/clock-complete.json').exists &&
       new Error().stack.includes('_emscripten_sleep')){
     __engineAudio.paused=true;return -1;
    }
    return schedule(callback,delay,...args);
   };
  });
  await page.route('**/verification-services.bin',route=>route.fulfill({status:200,body:services}));
  await page.route('**/verification-clock.bin',route=>route.fulfill({status:200,body:clock}));
  await page.route('**/index.html',route=>{
   const html=fs.readFileSync(path.join(build,'index.html'),'utf8');
   const marker='<script async type="text/javascript" src="index.js"></script>';assert(html.includes(marker));
   const hook='<script>Module.preRun.push(function(){var sync=FS.syncfs;'+
    'FS.syncfs=function(populate,done){return sync.call(FS,populate,function(error){'+
    'if(populate&&!error)FS.writeFile("/persist/SaveGames",Uint8Array.from(atob('+JSON.stringify(save.toString('base64'))+'),c=>c.charCodeAt(0)));done(error);});};});'+
    'Module.preRun.unshift(function(){delete ENV.DD2_REALTIME;FS.mkdirTree("/tmp/wasm-dd2");'+
    'ENV.DD2_AUDIO_SERVICES="/tmp/wasm-dd2/services.bin";ENV.DD2_TICK_REPLAY="/tmp/wasm-dd2/ticks.bin";'+
    'ENV.DD2_AUDIO_SERVICE_REPORT="/tmp/wasm-dd2/clock-complete.json";ENV.DD2_MIXPCM="/tmp/wasm-dd2/mixed.pcm";'+
    'ENV.DD2_SNDLOG="/tmp/wasm-dd2/sound.log";ENV.DD2_RACE_STREAM="/tmp/wasm-dd2/race-stream.jsonl";'+
    'Module.addRunDependency("engine-audio-inputs");'+
    'Promise.all([fetch("/verification-services.bin"),fetch("/verification-clock.bin")].map(async request=>{const r=await request;if(!r.ok)throw Error("verification input HTTP error");return new Uint8Array(await r.arrayBuffer());})).then(function(inputs){'+
    'FS.writeFile("/tmp/wasm-dd2/services.bin",inputs[0]);FS.writeFile("/tmp/wasm-dd2/ticks.bin",inputs[1]);Module.removeRunDependency("engine-audio-inputs");}).catch(function(error){console.error(error);});});</script>';
   return route.fulfill({status:200,contentType:'text/html',body:html.replace(marker,hook+marker)});
  });
  await page.goto(`http://localhost:${server.address().port}/index.html`);
  await page.waitForFunction(()=>window.__engineAudioError || (typeof HEAP32!=='undefined' && HEAP32[0x462cd4>>2]===1 && !!Module._dd2movieSource),null,{timeout:30000});
  assert.deepStrictEqual(errors,[],'browser engine errors');
  const state=()=>page.evaluate(()=>({level:HEAP32[0x936ff4>>2],cf:HEAP32[0x462ff0>>2],movie:HEAP32[0x462cd4>>2],
   menu:{poly_list:HEAPU32[0x940010>>2],restart_cd_audio:HEAP32[0x467420>>2]},
   cd:{enabled:HEAP32[0x462d74>>2],playing:HEAP32[0x462d70>>2],from:HEAP32[0x74f174>>2],to:HEAP32[0x74f178>>2]}}));
  const intro=await state();
  assert(Buffer.from(await page.evaluate(()=>Array.from(Module.FS.readFile('/SaveGames')))).equals(save),'initial save input differs');
  await page.click('#canvas');await page.keyboard.press('Escape');
  await page.waitForFunction(()=>window.__engineAudioError || (HEAP32[0x462cd4>>2]===0 && HEAP32[0x936ff4>>2]===0 &&
   HEAP32[0x940010>>2]===0x4696b0 && HEAP32[0x467420>>2]===0 && HEAP32[0x462d70>>2]===1),null,{timeout:30000});
  assert.deepStrictEqual(errors,[],'browser engine errors');
  const start=await state();
  await page.waitForFunction(()=>__engineAudio.paused || window.__engineAudioError,null,{timeout:180000});
  assert.deepStrictEqual(errors,[],'browser engine errors');
  const end=await state();assert(end.level===9 && end.cf>=60,'actual attract race not reached');
  const captured=await page.evaluate(()=>{
   const encode=bytes=>{let text='';for(let i=0;i<bytes.length;i+=16384)text+=String.fromCharCode(...bytes.subarray(i,i+16384));return btoa(text);};
   const all=new Uint8Array(__engineAudio.frames*8);let off=0;
   for(const part of __engineAudio.parts){all.set(part,off);off+=part.length;}
   return {pcm:encode(Module.FS.readFile('/tmp/wasm-dd2/mixed.pcm')),sink_pcm:encode(all),
    metadata:Module.FS.readFile('/tmp/wasm-dd2/mixed.pcm.json',{encoding:'utf8'}),
    complete:JSON.parse(Module.FS.readFile('/tmp/wasm-dd2/clock-complete.json',{encoding:'utf8'})),
    sound:Module.FS.readFile('/tmp/wasm-dd2/sound.log',{encoding:'utf8'}),race:Module.FS.readFile('/tmp/wasm-dd2/race-stream.jsonl',{encoding:'utf8'}),
    sink:{frames:__engineAudio.frames,buffers:__engineAudio.buffers,mismatches:__engineAudio.mismatches,missing:__engineAudio.missing,dropped:__engineAudio.dropped}};
  });
  const pcm=Buffer.from(captured.pcm,'base64');assert(pcm.equals(Buffer.from(captured.sink_pcm,'base64')),'C output differs from complete sink input');
  assert(captured.sink.mismatches===0 && captured.sink.missing===0,'scheduled WebAudio buffers differ');
  assert.deepStrictEqual(errors,[],'browser engine errors');
  for(const [name,text] of [['sound.log',captured.sound],['race-stream.jsonl',captured.race],['mixed.pcm.json',captured.metadata]])fs.writeFileSync(path.join(output,name),text);
  fs.writeFileSync(path.join(output,'mixed.pcm'),pcm);
  fs.writeFileSync(path.join(output,'clock-complete.json'),JSON.stringify(captured.complete)+'\n');
  const report={scope:'Actual browser startup and attract race with own engine controls and observed services; interactive scheduling/video/hardware are separate',
   intro,start_state:start,end_state:end,engine_state_writes:false,errors,initial_save_sha256:hash(save),
   wasm_sha256:hash(fs.readFileSync(path.join(build,'index.wasm'))),game_clock_sha256:hash(clock),audio_services_sha256:hash(services),
   audio_services:captured.complete,sink:captured.sink,accepted_sha256:hash(pcm)};
  fs.writeFileSync(path.join(output,'checkpoint.json'),JSON.stringify(report,null,2)+'\n');console.log(JSON.stringify(report,null,2));
 }catch(error){
  if(browser){
   const page=browser.contexts()[0]?.pages()[0];
   if(page)try{
    const failure=await page.evaluate(()=>{
     const files={};
     for(const name of ['sound.log','race-stream.jsonl','clock-complete.json']){
      const filename='/tmp/wasm-dd2/'+name;
      if(typeof Module!=='undefined' && Module.FS?.analyzePath(filename).exists)files[name]=Module.FS.readFile(filename,{encoding:'utf8'});
     }
     return {files,level:typeof HEAP32==='undefined'?null:HEAP32[0x936ff4>>2],cf:typeof HEAP32==='undefined'?null:HEAP32[0x462ff0>>2],
      sink:typeof __engineAudio==='undefined'?null:{frames:__engineAudio.frames,buffers:__engineAudio.buffers,dropped:__engineAudio.dropped,paused:__engineAudio.paused}};
    });
    for(const [name,text] of Object.entries(failure.files))fs.writeFileSync(path.join(output,name),text);
    delete failure.files;failure.error=String(error);fs.writeFileSync(path.join(output,'failure.json'),JSON.stringify(failure,null,2)+'\n');
   }catch(captureError){console.error('failure observation:',captureError.message);}
  }
  throw error;
 }finally{if(browser)await browser.close();await new Promise(resolve=>server.close(resolve));}
})().catch(error=>{console.error(error);process.exitCode=1;});
