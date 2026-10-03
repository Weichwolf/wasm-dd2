// Observe normal application/intro/frontend startup with actual keyboard input.
// The test-only shell hook selects log paths; it never writes engine state.
const fs=require('fs'),path=require('path'),assert=require('assert'),crypto=require('crypto');
const {serve,chromium}=require('./felib');
const build=path.resolve(process.argv[2]||'web/dd2');
const output=path.resolve(process.argv[3]);
const clock=process.argv[4]?fs.readFileSync(path.resolve(process.argv[4])):null;
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
  // Hook the loaded shell before its async WASM script. Engine, assets,
  // application arguments and user-input paths stay the production ones.
  await page.route('**/index.html',route=>{
   const html=fs.readFileSync(path.join(build,'index.html'),'utf8');
   const marker='<script async type="text/javascript" src="index.js"></script>';
   assert(html.includes(marker),'missing production runtime script');
   const hook='<script>Module.preRun.push(function(){'+
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
  await page.click('#canvas');await page.keyboard.press('Escape');
  await page.waitForFunction(()=>HEAP32[0x462cd4>>2]===0 && HEAP32[0x936ff4>>2]===0 &&
   HEAP32[0x940010>>2]===0x4696b0 && HEAP32[0x467420>>2]===0 && HEAP32[0x462d70>>2]===1,
   null,{timeout:30000});
  const start=await state();
  if(clock)await page.waitForFunction(()=>Module.FS.analyzePath('/tmp/wasm-dd2/clock-complete.json').exists,
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
   engine_state_writes:false,errors,
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
  fs.writeFileSync(path.join(output,'checkpoint.json'),JSON.stringify(report,null,2)+'\n');
  console.log(JSON.stringify(report,null,2));
 }finally{
  if(browser)await browser.close();await new Promise(resolve=>server.close(resolve));
 }
})().catch(error=>{console.error(error);process.exitCode=1});
