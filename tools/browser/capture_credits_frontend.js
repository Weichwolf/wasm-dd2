// Actual name-grid CREDITZ! routing, full Outro presentations and main return.
// No movie shortcut, engine state injection, or PCM/timing acceptance.
const assert=require('assert'),fs=require('fs'),path=require('path'),crypto=require('crypto');
const {serve,boot,chromium}=require('./felib');
const {installMenuInput}=require('./menu_input');
const {execFileSync}=require('child_process');
const ROOT=path.resolve(__dirname,'../..');
const [buildArg,outputArg,originalArg]=process.argv.slice(2);
assert(buildArg&&outputArg&&originalArg,'usage: BUILD OUTPUT ORIGINAL_CAPTURE');
const build=path.resolve(buildArg),output=path.resolve(outputArg),original=JSON.parse(fs.readFileSync(path.join(originalArg,'report.json')));
assert(output.startsWith('/tmp/wasm-dd2/')&&!fs.existsSync(output));fs.mkdirSync(output);
const sha=data=>crypto.createHash('sha256').update(data).digest('hex');
const actions=JSON.parse(execFileSync('python3',['-c',`import sys,json;sys.path.insert(0,${JSON.stringify(ROOT+'/tools')});from driver_name_input import name_actions;print(json.dumps(name_actions('CREDITZ!')))`],{encoding:'utf8'}));
assert(original.pass_&&original.exe_modified===false&&original.engine_state_writes===false&&original.original_process_exited&&original.outro_started&&original.card_unchanged);
assert.equal(original.exe_sha256,'0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2');
assert.deepEqual(original.input_keys,['Return','Return','Return',...actions.map(a=>a.key)]);
assert.equal(original.name_helper_sha256,sha(fs.readFileSync(ROOT+'/tools/driver_name_input.py')));
const manifestBytes=fs.readFileSync(path.join(originalArg,'movie-video-archive/manifest.json'));
assert.equal(sha(manifestBytes),original.manifest_sha256);
const manifest=JSON.parse(manifestBytes);
const referenceFrames=manifest.records.slice(original.outro_archive_begin);
assert(manifest.pass_&&referenceFrames.length===original.outro_frames);
const card=fs.readFileSync(ROOT+'/DestructionDerby2/SaveGames');assert.equal(sha(card),original.initial_save_sha256);
const report={scope:'Actual browser CREDITZ! driver-name inputs, every chronological canvas frame SHA256 compared with retained original window observations, and actual exit(0). Read-only grid observations and trusted Playwright keys; no direct movie shortcut, engine state injection, PCM or physical timing acceptance.',target:'browser',pass_:false,engine_state_writes:false,input_keys:[],grid_observations:[],original_report:path.resolve(originalArg,'report.json'),original_manifest_sha256:sha(manifestBytes),wasm_sha256:sha(fs.readFileSync(build+'/index.wasm'))};
const codes={Return:'Enter',Left:'ArrowLeft',Right:'ArrowRight',Up:'ArrowUp',Down:'ArrowDown'};
(async()=>{
 const server=serve(build);await new Promise(resolve=>server.listen(0,resolve));let browser,watchdog,page;
 try{
  browser=await chromium.launch({args:['--no-sandbox']});watchdog=setTimeout(()=>browser.close().catch(()=>{}),300000);
  page=await browser.newPage();await installMenuInput(page);
  const errors=[];report.page_errors=[];page.on('pageerror',error=>{errors.push(error.message);report.page_errors.push({message:error.message,stack:error.stack});});
  page.on('console',message=>fs.appendFileSync(output+'/browser.log',message.type()+': '+message.text()+'\n'));
  await page.addInitScript(()=>{
   window.__creditsActive=false;window.__creditsFrames=0;window.__creditsKeys=[];window.__creditsExits=[];
   window.__creditsHashes=[];window.__creditsHashPending=0;window.__creditsHashErrors=[];let presenting=false;
   for(const type of ['keydown','keyup'])window.addEventListener(type,event=>window.__creditsKeys.push({type,code:event.code,trusted:event.isTrusted}));
   function observe(imports){
    if(!imports?.env?.movie_present)return;const call=imports.env.movie_present;
    imports.env.movie_present=function(...args){
     const previous=presenting;presenting=window.__creditsActive;
     if(presenting)window.__creditsFrames++;
     try{return call(...args);}finally{presenting=previous;}
    };
    const exit=imports.env.exit;
    if(exit)imports.env.exit=function(code){window.__creditsExits.push(code);return exit(code);};
   }
   const put=CanvasRenderingContext2D.prototype.putImageData;
   CanvasRenderingContext2D.prototype.putImageData=function(...args){
    const result=put.apply(this,args);
    if(presenting&&this.canvas.id==='canvas'){
     const frame=__creditsHashes.length;__creditsHashes.push(null);
     const rgba=this.getImageData(0,0,640,480).data,argb=new Uint8Array(rgba.length);
     for(let i=0;i<rgba.length;i+=4){argb[i]=rgba[i+2];argb[i+1]=rgba[i+1];argb[i+2]=rgba[i];argb[i+3]=rgba[i+3];}
     __creditsHashPending++;
     if(__creditsHashPending>32)throw Error('Credits readback backlog exceeded 40 MiB');
     crypto.subtle.digest('SHA-256',argb).then(hash=>{
      __creditsHashes[frame]=Array.from(new Uint8Array(hash),v=>v.toString(16).padStart(2,'0')).join('');
     }).catch(error=>__creditsHashErrors.push(String(error))).finally(()=>__creditsHashPending--);
    }
    return result;
   };
   for(const name of ['instantiate','instantiateStreaming']){
    const call=WebAssembly[name];if(call)WebAssembly[name]=function(bytes,imports,...rest){observe(imports);return call.call(this,bytes,imports,...rest);};
   }
  });
  await boot(page,server);
  async function ready(){await page.waitForFunction(()=>HEAP16[0x46996c>>1]===0&&window.__slabReadyFrames>=16,null,{timeout:15000});}
  async function key(code){
   await ready();const mask={Return:0x4000,Left:0x80,Right:0x20,Up:0x10,Down:0x40}[code];
   await page.waitForFunction(mask=>((HEAPU16[0x754448>>1]|HEAPU16[0x75444a>>1])&mask)===0,mask);
   await page.keyboard.down(codes[code]);
   try{await page.waitForFunction(mask=>((HEAPU16[0x754448>>1]|HEAPU16[0x75444a>>1])&mask)!==0,mask);}
   finally{await page.keyboard.up(codes[code]);}
   await page.waitForFunction(mask=>(HEAPU16[0x754448>>1]&mask)===0,mask);
   await page.evaluate(()=>{window.__slabReadyFrames=0;});await page.waitForTimeout(300);report.input_keys.push(code);
  }
  for(const code of ['Return','Return','Return'])await key(code);
  assert.equal(await page.evaluate(()=>HEAPU32[0x940010>>2]),0x469f70);
  for(const action of actions.slice(0,-1)){
   await key(action.key);
   const state=await page.evaluate(()=>({name:UTF8ToString(HEAPU32[0x469fd4>>2]+8),cursor:[HEAP16[0x469f34>>1],HEAP16[0x469f36>>1]]}));
   assert.equal(state.name,action.entered);assert.deepEqual(state.cursor,action.cursor);report.grid_observations.push(state);
  }
  report.before_accept=await page.evaluate(()=>({name:UTF8ToString(HEAPU32[0x469fd4>>2]+8),movie:HEAP32[0x462cd4>>2],menu:HEAPU32[0x940010>>2]}));
  assert.deepEqual(report.before_accept,original.before_accept);await ready();
  await page.evaluate(()=>{window.__creditsActive=true;});
  await page.keyboard.press('Enter',{delay:50});report.input_keys.push('Return');
  await page.waitForFunction(()=>HEAP32[0x462cd4>>2]===1&&!!Module._dd2movieSource,null,{timeout:15000});
  const started=Date.now();
  await page.waitForFunction(()=>window.__creditsExits.length>0&&EXITSTATUS===0&&HEAP32[0x462cd4>>2]===0&&!Module._dd2movieSource,null,{timeout:120000});
  report.outro_elapsed_seconds=(Date.now()-started)/1000;
  report.outro_frames=await page.evaluate(()=>window.__creditsFrames);
  assert.equal(report.outro_frames,original.outro_frames);assert(report.outro_elapsed_seconds>70);
  report.exit_calls=await page.evaluate(()=>window.__creditsExits);assert.deepEqual(report.exit_calls,[0]);
  await page.waitForFunction(()=>__creditsHashPending===0,null,{timeout:15000});
  assert.deepEqual(await page.evaluate(()=>__creditsHashErrors),[]);
  report.canvas_frame_sha256=await page.evaluate(()=>__creditsHashes);
  assert.deepEqual(report.canvas_frame_sha256,referenceFrames.map(frame=>frame.window_argb_sha256));
  report.matched_canvas_frames=report.canvas_frame_sha256.length;
  report.trusted_keyboard_events=await page.evaluate(()=>window.__creditsKeys);
  assert(report.trusted_keyboard_events.every(e=>e.trusted));assert.deepEqual(errors,[]);
  assert.deepEqual(report.input_keys,original.input_keys);
  assert(Buffer.from(await page.evaluate(()=>Array.from(FS.readFile('/SaveGames')))).equals(card));
  report.pass_=true;report.game_exited=true;report.exit_code=0;report.card_unchanged=true;
 }catch(error){
  report.error=error.stack;
  if(page)report.observed_end=await page.evaluate(()=>({frames:window.__creditsFrames,exit_code:typeof EXITSTATUS==='undefined'?null:EXITSTATUS,movie:typeof HEAP32==='undefined'?null:HEAP32[0x462cd4>>2],has_movie_source:!!Module._dd2movieSource})).catch(()=>null);
  throw error;
 }
 finally{clearTimeout(watchdog);fs.writeFileSync(output+'/report.json',JSON.stringify(report,null,2)+'\n');if(browser)await browser.close();await new Promise(resolve=>server.close(resolve));}
 console.log('Browser actual CREDITZ! grid, full Outro and game return PASS');
})().catch(error=>{console.error(error);process.exitCode=1;});
