// Run the actual browser engine with observed original clock/audio services.
// Recorded control values are assertions in C; no engine state is supplied.
const fs=require('fs'),path=require('path'),assert=require('assert'),crypto=require('crypto');
const {serve,chromium}=require('./felib');
const [buildArg,outputArg,servicesArg,clockArg,fixtureArg,scheduleArg,layoutArg,videoArg]=process.argv.slice(2);
assert([6,8,9,10].includes(process.argv.length),'usage: capture_engine_audio.js BUILD OUTPUT SERVICES CLOCK [REPLAY_FIXTURE NATIVE_REPLAY_CHECKPOINT [WASM_LAYOUT [ORIGINAL_VIDEO_REPORT]]]');
const build=path.resolve(buildArg),output=path.resolve(outputArg);
const services=fs.readFileSync(path.resolve(servicesArg)),clock=fs.readFileSync(path.resolve(clockArg));
const hash=bytes=>crypto.createHash('sha256').update(bytes).digest('hex');
const save=fs.readFileSync(fixtureArg?path.join(path.resolve(fixtureArg),'original.card'):
 path.resolve(__dirname,'../../DestructionDerby2/SaveGames'));
let replay=null;
if(fixtureArg){
 const producer=JSON.parse(fs.readFileSync(path.join(path.resolve(fixtureArg),'report.json')));
 const reference=JSON.parse(fs.readFileSync(path.resolve(scheduleArg)));
 const layout=layoutArg?JSON.parse(fs.readFileSync(path.resolve(layoutArg))):null;
 const video=videoArg?JSON.parse(fs.readFileSync(path.resolve(videoArg))):null;
 if(video)assert(video.pass_ && video.debugger===false && video.engine_state_writes===false &&
  video.game_clock_sha256===hash(clock) && video.frame_count===video.frames.length &&
  video.frame_count<=(video.video_archive_manifest_sha256?60000:4096) && layout,
  'actual original video/clock observation required');
 if(layout)assert(layout.wasm_sha256===hash(fs.readFileSync(path.join(build,'index.wasm'))) &&
  Number.isInteger(layout.clock_counter_address) && layout.clock_counter_address>=10485760,'actual WASM clock layout differs');
 if(reference.keyboard_input_sha256)assert(layout,'observed source keyboard comparison requires actual WASM clock layout');
 assert(producer.pass_ && producer.operation==='generate' && producer.card_sha256===hash(save));
 assert(reference.pass_ && reference.scenario==='original-replay' && reference.engine_state_writes===false);
 assert(reference.initial_save_sha256===hash(save) && reference.audio_services_sha256===hash(services) && reference.game_clock_sha256===hash(clock));
 assert.deepStrictEqual(reference.schedule.map(p=>p.key),['Right','Right','Right','Return','Return','Return']);
 replay={schedule:reference.schedule,keyboard_input_sha256:reference.keyboard_input_sha256,
  clock_counter_address:layout?.clock_counter_address,video,
  end:producer.recorded.end,actions:reference.schedule.flatMap((p,i)=>[
  {action:i,key:p.key==='Right'?'ArrowRight':'Enter',down:true,pending_flip:p.flip-1,clock_calls:p.clock_calls},
  {action:i,key:p.key==='Right'?'ArrowRight':'Enter',down:false,pending_flip:(p.release_flip??p.flip+1)-1,
   clock_calls:p.release_clock_calls??p.clock_calls}])};
 assert(replay.actions.every((p,i)=>!i || p.pending_flip>replay.actions[i-1].pending_flip));
}
assert(output.startsWith('/tmp/wasm-dd2/'),'diagnostics must be under /tmp/wasm-dd2/');
const parent=fs.realpathSync(path.dirname(output));
assert(parent==='/tmp/wasm-dd2' || parent.startsWith('/tmp/wasm-dd2/'),'output parent escapes temporary workspace');
fs.mkdirSync(output,{recursive:false});
function budget(){
 const size=dir=>fs.readdirSync(dir,{withFileTypes:true}).reduce((n,e)=>{
  const p=path.join(dir,e.name);return n+(e.isDirectory()?size(p):e.isFile()?fs.statSync(p).size:0);
 },0);
 const disk=fs.statfsSync(output);
 assert(size(output)<2*1024**3 && disk.bavail*disk.bsize>=1024**3,'verification space budget exceeded');
}
budget();
(async()=>{
 const server=serve(build);await new Promise(resolve=>server.listen(0,resolve));let browser,videoFile;
 try{
  browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage(),errors=[];
  if(replay?.video){
   const {VideoRecords}=require('./video_record_reader');
   videoFile=new VideoRecords(path.resolve(replay.video.capture_directory),replay.video);
   assert(replay.video.record_bytes===128+307200+2048,'actual original video record format differs');
   await page.exposeFunction('__compareEngineVideo',(index,pixels,palette)=>{
    assert(Number.isInteger(index) && index>=0 && index<replay.video.frame_count,'invalid browser video index');
    const expected=videoFile.record(index).subarray(128,128+307200+1024);
    assert(Buffer.concat([Buffer.from(pixels,'base64'),Buffer.from(palette,'base64')]).equals(expected),`actual browser video bytes differ at frame ${index}`);
    return true;
   });
  }
  page.on('pageerror',error=>errors.push(error.message));page.on('crash',()=>errors.push('renderer crash'));
  page.on('console',message=>{
   const text=message.text();fs.appendFileSync(path.join(output,'browser.log'),message.type()+': '+text+'\n');
   if(text.includes('DD2_AUDIO_SERVICES:') || text.includes('[clock-replay] clock input exhausted') || text.includes('program exited (with status: 1)')){
    errors.push(text);page.evaluate(text=>{window.__engineAudioError=text;},text).catch(()=>{});
   }
  });
  await page.addInitScript(replay=>{
   window.__engineAudio={paused:false,parts:[],frames:0,buffers:0,mismatches:0,missing:0,dropped:0};
   if(replay){
    window.__replayInput={flips:0,pending_flip:-1,index:0,paused:false,resume:null,events:[],observations:[]};
    for(const type of ['keydown','keyup'])window.addEventListener(type,e=>{
     if(['ArrowRight','Enter'].includes(e.code))__replayInput.events.push({type,code:e.code,trusted:e.isTrusted,
      pending_flip:__replayInput.pending_flip,held:HEAPU16[0x754448>>1],pressed:HEAPU16[0x75444a>>1]});
     if(replay.clock_counter_address && ['ArrowRight','Enter'].includes(e.code))
      __replayInput.events[__replayInput.events.length-1].clock_calls=HEAPU32[replay.clock_counter_address>>2];
    });
    const present=CanvasRenderingContext2D.prototype.putImageData;
    if(replay.video)window.__engineVideo={frames:[],pending:[],source:null};
    CanvasRenderingContext2D.prototype.putImageData=function(...args){
     const result=present.apply(this,args);
     if(this.canvas.id==='canvas' && typeof Module!=='undefined' && args[0]===Module._dd2img){
      if(replay.video){
       const source=__engineVideo.source;assertVideo(!!source,'Actual presentation import missing');
       const pixels=HEAPU8.slice(source.fb,source.fb+307200),palette=HEAPU8.slice(source.pal,source.pal+1024);
       const rgba=this.getImageData(0,0,640,480).data;
       for(let i=0;i<307200;i++){
        const p=i*4,c=pixels[i]*4;
        if(rgba[p]!==palette[c] || rgba[p+1]!==palette[c+1] || rgba[p+2]!==palette[c+2] || rgba[p+3]!==255)
         throw Error('Actual canvas pixel differs at '+i);
       }
       const row={index:__replayInput.flips,flip:__replayInput.flips+1,level:HEAP32[0x936ff4>>2],cf:HEAP32[0x462ff0>>2],
        movie:HEAP32[0x462cd4>>2],poly_list:HEAPU32[0x940010>>2],restart_cd_audio:HEAP32[0x467420>>2],
        ticks:HEAP32[0x7746c0>>2],replay:HEAP32[0x467074>>2],quit:HEAP32[0x7746ac>>2],script_cursor:HEAPU32[0x9392b4>>2],
        clock_calls:HEAPU32[replay.clock_counter_address>>2]};
       __engineVideo.frames.push(row);
       const digest=async bytes=>Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',bytes)),b=>b.toString(16).padStart(2,'0')).join('');
       const encode=bytes=>{let text='';for(let i=0;i<bytes.length;i+=32768)text+=String.fromCharCode(...bytes.subarray(i,i+32768));return btoa(text);};
       __engineVideo.pending.push(Promise.all([digest(pixels),digest(palette),digest(rgba),
        __compareEngineVideo(row.index,encode(pixels),encode(palette))]).then(hashes=>{
        [row.framebuffer_sha256,row.palette_sha256,row.canvas_rgba_sha256]=hashes;
        row.original_bytes_compared=hashes[3]===true;
       }));
      }
      __replayInput.pending_flip=__replayInput.flips++;
     }
     return result;
    };
   }
   function assertVideo(condition,message){if(!condition)throw Error(message);}
   let expected;
   const observe=imports=>{
    if(replay?.video && imports?.env?.dd2_present && !imports.env.dd2_present.__videoObserved){
     const present=imports.env.dd2_present;
     const wrapped=function(fb,pal){
      __engineVideo.source={fb,pal};try{return present.call(this,fb,pal);}finally{__engineVideo.source=null;}
     };
     wrapped.__videoObserved=true;imports.env.dd2_present=wrapped;
    }
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
    if(typeof Module!=='undefined' && new Error().stack.includes('_emscripten_sleep')){
     if(Module.FS?.analyzePath('/tmp/wasm-dd2/clock-complete.json').exists){
      __engineAudio.paused=true;return -1;
     }
     if(replay && __replayInput.index<replay.actions.length){
      const action=replay.actions[__replayInput.index];
      if(__replayInput.pending_flip>action.pending_flip)throw Error('Missed real keyboard presentation');
      if(__replayInput.pending_flip===action.pending_flip){
       __replayInput.paused=true;
       __replayInput.resume=()=>{
        __replayInput.resume=null;__replayInput.paused=false;__replayInput.index++;
        schedule(callback,delay,...args);
       };
       return -1;
      }
     }
    }
    return schedule(callback,delay,...args);
   };
  },replay);
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
  let startSettings;
  const settings=()=>page.evaluate(()=>({mode:HEAP32[0x4673f8>>2],type:HEAP32[0x4673f4>>2],car:HEAP32[0x467400>>2],
   track:HEAP32[0x4673fc>>2],sound:HEAP32[0x467410>>2],pad_option:HEAP32[0x467414>>2],
   saved:Array.from(HEAPU8.subarray(0x46757a,0x46758c)),active:Array.from(HEAPU8.subarray(0x46302c,0x46303a))}));
  if(replay){
   startSettings=await settings();
   for(const [i,action] of replay.actions.entries()){
    budget();
    await page.waitForFunction(()=>__replayInput.paused || window.__engineAudioError,null,{timeout:30000});
    assert.deepStrictEqual(errors,[],'browser engine errors');
    const observed=await page.evaluate(address=>({index:__replayInput.index,pending_flip:__replayInput.pending_flip,
     held:HEAPU16[0x754448>>1],pressed:HEAPU16[0x75444a>>1],clock_calls:address?HEAPU32[address>>2]:null}),replay.clock_counter_address);
    assert(observed.index===i && observed.pending_flip===action.pending_flip,'real keyboard position differs');
    if(replay.clock_counter_address)assert(observed.clock_calls===action.clock_calls,'real keyboard game-clock position differs');
    if(!action.down)assert(observed.held&(action.key==='ArrowRight'?32:16384),'engine did not consume genuine keyboard input');
    if(action.down)await page.keyboard.down(action.key);else await page.keyboard.up(action.key);
    await page.evaluate(observed=>{__replayInput.observations.push(observed);__replayInput.resume();},observed);
   }
  }
  await page.waitForFunction(()=>__engineAudio.paused || window.__engineAudioError,null,{timeout:180000});
  assert.deepStrictEqual(errors,[],'browser engine errors');
  const end=await state();
  if(replay)assert(end.level===0 && end.cf>0,'actual replay did not return to frontend');
  else assert(end.level===9 && end.cf>=60,'actual attract race not reached');
  let replayCheckpoint;
  if(replay){
   const restored=await settings();assert.deepStrictEqual(restored,startSettings,'replay settings not restored');
   replayCheckpoint=await page.evaluate(()=>({completion:{script_cursor:HEAPU32[0x9392b4>>2],first_time:HEAP32[0x9392b0>>2]},
    replay:HEAP32[0x467074>>2],quit:HEAP32[0x7746ac>>2],
    script:Array.from(HEAPU8.subarray(0x9376b0,0x9392b0)),order:Array.from(HEAPU8.subarray(0x795c28,0x795c3c)),
    card:Array.from(HEAPU8.subarray(0x754460,0x774460)),file:Array.from(Module.FS.readFile('/SaveGames')),
    inputs:{flips:__replayInput.flips,index:__replayInput.index,events:__replayInput.events,observations:__replayInput.observations}}));
   if(replay.clock_counter_address)replayCheckpoint.clock_calls=await page.evaluate(address=>HEAPU32[address>>2],replay.clock_counter_address);
   assert.deepStrictEqual(replayCheckpoint.completion,{script_cursor:replay.end,first_time:1});
   assert(replayCheckpoint.replay===0 && replayCheckpoint.quit===1);
   assert(Buffer.from(replayCheckpoint.script).equals(save.subarray(0x2012,0x3c12)) &&
    Buffer.from(replayCheckpoint.order).equals(save.subarray(0x3c12,0x3c26)),'loaded replay script/order differs');
   assert(Buffer.from(replayCheckpoint.card).equals(save) && Buffer.from(replayCheckpoint.file).equals(save),'card changed');
   assert(replayCheckpoint.inputs.index===replay.actions.length && replayCheckpoint.inputs.events.length===replay.actions.length &&
    replayCheckpoint.inputs.events.every(e=>e.trusted),'genuine trusted input history missing');
   delete replayCheckpoint.card;delete replayCheckpoint.file;delete replayCheckpoint.script;delete replayCheckpoint.order;
   replayCheckpoint.restored=restored;
  }
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
  if(replay){
   report.scope='Actual replay engine with trusted Playwright keys and observed clock/audio services; key positions and first Enter release are diagnostic scheduling, original OS event identity/video/physical timing remain open';
   Object.assign(report,{pass_:true,scenario:'original-replay',schedule:replay.schedule,start_settings:startSettings,...replayCheckpoint});
   if(replay.keyboard_input_sha256){
    report.keyboard_input_sha256=replay.keyboard_input_sha256;
    report.scope='Actual replay engine with trusted Playwright keys at original-observed window-procedure message positions; own engine controls and observed clock/audio services. Physical OS timing, chronological video and full-game parity remain open';
   }
   if(replay.video){
    const frames=await page.evaluate(async()=>{await Promise.all(__engineVideo.pending);return __engineVideo.frames;});
    assert(frames.length===captured.complete.completed_flips,'complete bounded browser video required');
    for(const row of frames){
     const expected=replay.video.frames[row.index];assert(expected,'extra browser video');
     for(const key of ['flip','level','cf','movie','poly_list','restart_cd_audio','ticks','replay','quit','script_cursor','clock_calls',
      'framebuffer_sha256','palette_sha256'])assert(row[key]===expected[key],`original browser video ${row.index} differs: ${key}`);
    }
    report.video={pass_:true,scope:'Every bounded indexed frame/palette and actual canvas pixels in the same audio/key run; intro and physical display timing excluded',
     original_video_sha256:replay.video.video_sha256,original_trace_sha256:replay.video.trace_sha256,frames};
   }
   assert(captured.complete.completed_flips===replayCheckpoint.inputs.flips,'presentation observer count differs from actual C endpoint');
  }
  fs.writeFileSync(path.join(output,'checkpoint.json'),JSON.stringify(report,null,2)+'\n');budget();console.log(JSON.stringify(report,null,2));
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
 }finally{if(browser)await browser.close();if(videoFile!==undefined)videoFile.close();await new Promise(resolve=>server.close(resolve));}
})().catch(error=>{console.error(error);process.exitCode=1;});
