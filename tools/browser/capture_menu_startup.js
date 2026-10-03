// Observe normal application/intro/frontend startup with actual keyboard input.
// The test-only shell hook selects log paths; it never writes engine state.
const fs=require('fs'),path=require('path'),assert=require('assert'),crypto=require('crypto');
const {serve,chromium}=require('./felib');
const build=path.resolve(process.argv[2]||'web/dd2');
const output=path.resolve(process.argv[3]);
const options=process.argv.slice(4);
const videoOption=options.find(value=>value.startsWith('--video-frames='));
const videoFrames=videoOption?Number(videoOption.split('=')[1]):0;
const clockOption=options.find(value=>!value.startsWith('--'));
const clock=clockOption?fs.readFileSync(path.resolve(clockOption)):null;
assert(options.length===(videoOption?1:0)+(clockOption?1:0),'unknown/duplicate startup capture options');
assert(!videoOption || (Number.isInteger(videoFrames) && videoFrames>=64 && videoFrames<=512 && !clock),
       'video capture requires 64..512 presentations and no device clock replay');
const save=fs.readFileSync(path.resolve(__dirname,'../../DestructionDerby2/SaveGames'));
const saveHash=crypto.createHash('sha256').update(save).digest('hex');
assert(output.startsWith('/tmp/wasm-dd2/'),'diagnostics must be under /tmp/wasm-dd2/');
fs.mkdirSync(output,{recursive:false});
(async()=>{
 const server=serve(build);await new Promise(resolve=>server.listen(0,resolve));
 let browser;
 try{
  browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage(),errors=[];
  page.on('pageerror',error=>errors.push(error.message));
  page.on('crash',()=>errors.push('renderer crash'));
  if(videoFrames)await page.addInitScript(limit=>{
   window.__startupVideo=[];
   const observe=imports=>{
    if(!imports?.env?.dd2_present || imports.env.dd2_present.__startupObserved)return;
    const present=imports.env.dd2_present;
    const wrapped=function(framebuffer,palette){
     const result=present(framebuffer,palette);
     if(__startupVideo.length<limit){
      const pixels=HEAPU8.slice(framebuffer,framebuffer+307200),colors=HEAPU8.slice(palette,palette+1024);
      const actual=Module.canvas.getContext('2d').getImageData(0,0,640,480).data;
      let mismatches=0;
      for(let i=0;i<307200;i++){
       const c=pixels[i]*4,p=i*4;
       if(actual[p]!==colors[c] || actual[p+1]!==colors[c+1] || actual[p+2]!==colors[c+2] || actual[p+3]!==255)mismatches++;
      }
      __startupVideo.push({index:__startupVideo.length,cf:HEAP32[0x462ff0>>2],level:HEAP32[0x936ff4>>2],
       poly_list:HEAPU32[0x940010>>2],restart_cd_audio:HEAP32[0x467420>>2],
       cd_playing:HEAP32[0x462d70>>2],highlight_phase:HEAP32[0x4699cc>>2],
       framebuffer:pixels,palette:colors,canvas_mismatches:mismatches});
     }
     return result;
    };
    wrapped.__startupObserved=true;imports.env.dd2_present=wrapped;
   };
   for(const name of ['instantiate','instantiateStreaming']){
    const original=WebAssembly[name];if(original)WebAssembly[name]=function(bytes,imports,...rest){
     observe(imports);return original.call(this,bytes,imports,...rest);
    };
   }
  },videoFrames);
  // Hook the loaded shell before its async WASM script. Engine, assets,
  // application arguments and user-input paths stay the production ones.
  await page.route('**/index.html',route=>{
   const html=fs.readFileSync(path.join(build,'index.html'),'utf8');
   const marker='<script async type="text/javascript" src="index.js"></script>';
   assert(html.includes(marker),'missing production runtime script');
   const hook='<script>Module.preRun.push(function(){'+
    'var sync=FS.syncfs;FS.syncfs=function(populate,done){return sync.call(FS,populate,function(error){'+
    'if(populate&&!error)FS.writeFile("/persist/SaveGames",Uint8Array.from(atob('+JSON.stringify(save.toString('base64'))+'),c=>c.charCodeAt(0)));'+
    'done(error);});};'+
    'FS.mkdirTree("/tmp/wasm-dd2");ENV.DD2_SNDLOG="/tmp/wasm-dd2/sound.log";'+
    (clock?'FS.writeFile("/tmp/wasm-dd2/clock.bin",Uint8Array.from(atob('+JSON.stringify(clock.toString('base64'))+'),c=>c.charCodeAt(0)));'+
     'ENV.DD2_AUDIO_FRAME_CLOCK="/tmp/wasm-dd2/clock.bin";'+
     'ENV.DD2_AUDIO_CLOCK_REPORT="/tmp/wasm-dd2/clock-complete.json";'+
     'ENV.DD2_MIXPCM="/tmp/wasm-dd2/mixed.pcm";':'')+
    '});</script>';
   return route.fulfill({status:200,contentType:'text/html',body:html.replace(marker,hook+marker)});
  });
  await page.goto(`http://localhost:${server.address().port}/index.html`);
  await page.waitForFunction(()=>typeof HEAP32!=='undefined' && HEAP32[0x462cd4>>2]===1 &&
                             !!Module._dd2movieSource,null,{timeout:30000});
  const state=()=>page.evaluate(()=>({
   level:HEAP32[0x936ff4>>2],cf:HEAP32[0x462ff0>>2],movie:HEAP32[0x462cd4>>2],
   menu:{poly_list:HEAPU32[0x940010>>2],restart_cd_audio:HEAPU32[0x467420>>2]},
   cd:{enabled:HEAP32[0x462d74>>2],playing:HEAP32[0x462d70>>2],
       from:HEAP32[0x74f174>>2],to:HEAP32[0x74f178>>2]}
  }));
  const intro=await state();
  const actualSave=await page.evaluate(()=>Array.from(Module.FS.readFile('/SaveGames')));
  assert(Buffer.from(actualSave).equals(save),'browser initial save input differs from the supplied original/native file');
  await page.click('#canvas');await page.keyboard.press('Escape');
  await page.waitForFunction(()=>HEAP32[0x462cd4>>2]===0 && HEAP32[0x936ff4>>2]===0 &&
   HEAP32[0x940010>>2]===0x4696b0 && HEAP32[0x467420>>2]===0 && HEAP32[0x462d70>>2]===1,
   null,{timeout:30000});
  const start=await state();
  if(videoFrames)await page.waitForFunction(limit=>__startupVideo.length===limit,videoFrames,{timeout:60000});
  else if(clock)await page.waitForFunction(()=>Module.FS.analyzePath('/tmp/wasm-dd2/clock-complete.json').exists,
                                     null,{timeout:60000});
  else await page.waitForTimeout(300);
  const end=await state();
  assert(end.level===0 && end.movie===0 && end.menu.poly_list===0x4696b0 &&
         end.menu.restart_cd_audio===0 && end.cd.playing===1,'capture left actual main menu');
  const log=await page.evaluate(()=>Module.FS.readFile('/tmp/wasm-dd2/sound.log',{encoding:'utf8'}));
  fs.writeFileSync(path.join(output,'sound.log'),log);
  assert.deepStrictEqual(errors,[],'normal browser startup errors');
  const report={scope:'Actual browser application startup with real keyboard intro skip; '+
   'control timing diagnostics, no original parity claim',intro,start_state:start,end_state:end,
   engine_state_writes:false,errors,initial_save_sha256:saveHash,
   wasm_sha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(build,'index.wasm'))).digest('hex')};
  if(clock){
   const captured=await page.evaluate(()=>{
    const bytes=Module.FS.readFile('/tmp/wasm-dd2/mixed.pcm');let text='';
    for(let i=0;i<bytes.length;i+=16384)text+=String.fromCharCode(...bytes.subarray(i,i+16384));
    return {pcm:btoa(text),metadata:Module.FS.readFile('/tmp/wasm-dd2/mixed.pcm.json',{encoding:'utf8'}),
     complete:JSON.parse(Module.FS.readFile('/tmp/wasm-dd2/clock-complete.json',{encoding:'utf8'}))};
   });
   fs.writeFileSync(path.join(output,'mixed.pcm'),Buffer.from(captured.pcm,'base64'));
   fs.writeFileSync(path.join(output,'mixed.pcm.json'),captured.metadata);
   fs.writeFileSync(path.join(output,'clock-complete.json'),JSON.stringify(captured.complete)+'\n');
   report.device_clock=captured.complete;
   report.device_clock_sha256=crypto.createHash('sha256').update(clock).digest('hex');
  }
  if(videoFrames){
   const directory=path.join(output,'startup');fs.mkdirSync(directory);
   const records=[];
   for(let first=0;first<videoFrames;first+=8){
    const batch=await page.evaluate(first=>{
     const encode=bytes=>{let text='';for(let i=0;i<bytes.length;i+=16384)text+=String.fromCharCode(...bytes.subarray(i,i+16384));return btoa(text);};
     return __startupVideo.slice(first,first+8).map(frame=>({...frame,
      framebuffer:encode(frame.framebuffer),palette:encode(frame.palette)}));
    },first);
    for(const frame of batch){
     assert(frame.index===records.length && frame.canvas_mismatches===0,'incomplete or incorrect actual browser presentations');
     const prefix=`frame${String(frame.index).padStart(5,'0')}`;
     const pixels=Buffer.from(frame.framebuffer,'base64'),palette=Buffer.from(frame.palette,'base64');
     assert(pixels.length===307200 && palette.length===1024,'incomplete startup framebuffer/palette');
     fs.writeFileSync(path.join(directory,prefix+'.bin'),pixels);fs.writeFileSync(path.join(directory,prefix+'.pal'),palette);
     delete frame.framebuffer;delete frame.palette;records.push({...frame,prefix});
    }
   }
   fs.writeFileSync(path.join(directory,'startup.json'),JSON.stringify({frames:records,
    stage:'Actual browser platform presentation / canvas readback',initial_movie_observed:true,
    intro_skip:true,input:'real browser Escape',engine_state_writes:false,
    initial_save_sha256:saveHash,wasm_sha256:report.wasm_sha256,
    scope:'First frontend presentations; audio/live timing excluded'},null,2)+'\n');
  }
  fs.writeFileSync(path.join(output,'checkpoint.json'),JSON.stringify(report,null,2)+'\n');
  console.log(JSON.stringify(report,null,2));
 }finally{
  if(browser)await browser.close();await new Promise(resolve=>server.close(resolve));
 }
})().catch(error=>{console.error(error);process.exitCode=1});
