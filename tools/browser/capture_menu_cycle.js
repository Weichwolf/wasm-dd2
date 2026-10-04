// Capture complete highlight cycles after normal keyboard navigation.
// No engine-state writes; each input is released after a presented frame.
const assert=require('assert'),fs=require('fs'),path=require('path'),crypto=require('crypto');
const {serve,boot,chromium}=require('./felib');
const codes={Left:'ArrowLeft',Right:'ArrowRight',Up:'ArrowUp',Down:'ArrowDown',Return:'Enter',Escape:'Escape',F1:'F1',F2:'F2',Space:'Space',
 ...Object.fromEntries(Array.from({length:26},(_,i)=>[String.fromCharCode(65+i),'Key'+String.fromCharCode(65+i)])),
 ...Object.fromEntries(Array.from({length:10},(_,i)=>[String(i),'Digit'+i]))};
const output=process.argv[3] && path.resolve(process.argv[3]),options=process.argv.slice(4);
const cycleOption=options.find(value=>value.startsWith('--cycle-frames='));
const cycleFrames=cycleOption?Number(cycleOption.split('=')[1]):64;
assert(cycleFrames===64 || cycleFrames===256,'cycle frames must be 64 or 256');
const keys=options.filter(value=>value!==cycleOption);
const save=fs.readFileSync(path.resolve(__dirname,'../../DestructionDerby2/SaveGames'));
const saveHash=crypto.createHash('sha256').update(save).digest('hex');
if(!output || keys.some(key=>!codes[key]))throw new Error('Usage: node capture_menu_cycle.js <web build> <fresh output> [Left Right Up Down Return Escape F1 F2 ...]');
assert(output.startsWith('/tmp/wasm-dd2/'),'verification output must be under /tmp/wasm-dd2/');
if(fs.existsSync(output) && fs.readdirSync(output).length)throw new Error('Use a fresh capture directory');
fs.mkdirSync(output,{recursive:true});
async function tap(page,key){
 await page.evaluate(code=>{
  window.__releaseKey=code;
  window.dispatchEvent(new KeyboardEvent('keydown',{code}));
 },codes[key]);
 await page.waitForFunction(()=>window.__releaseKey===null,null,{timeout:3000});
 await page.waitForTimeout(700);
}
async function capture(page,index,key){
 const name=`step${String(index).padStart(2,'0')}-${key||'boot'}`,directory=path.join(output,name,'cycle');
 fs.mkdirSync(directory,{recursive:true});
 await page.evaluate(()=>{window.__cycle=[];window.__cycleCapture=true;});
 await page.waitForFunction(()=>window.__cycleCapture===false,null,{timeout:cycleFrames===256?30000:10000});
 const frames=await page.evaluate(()=>window.__cycle);
 assert.equal(frames.length,cycleFrames,'incomplete browser cycle');
 assert.equal(new Set(frames.map(frame=>frame.phase)).size,64,'highlight phases missing/repeated');
 const metadata=[];
 for(let i=0;i<frames.length;i++){
  const frame=frames[i],prefix=`frame${String(i).padStart(3,'0')}`;
  assert(frame.phase>=0 && frame.phase<64,'highlight is not settled');
  assert.equal(frame.level,0,'menu cycle unexpectedly entered a race');
  assert.equal(frame.canvas_mismatches,0,'canvas pixels differ from captured framebuffer/palette');
  for(const [region,size] of [['framebuf',307200],['palette',1024]]){
   const data=Buffer.from(frame[region],'base64');assert.equal(data.length,size);
   fs.writeFileSync(path.join(directory,`${prefix}-${region}.bin`),data);
  }
  metadata.push({index:i,prefix,phase:frame.phase,cf:frame.cf,level:frame.level,canvas_mismatches:frame.canvas_mismatches,
   race_car:frame.race_car,car_angles:frame.car_angles,poly_list:frame.poly_list,
   race_mode:frame.race_mode,race_type:frame.race_type,race_track:frame.race_track,
   playable_tracks:frame.playable_tracks,playable_bowls:frame.playable_bowls,
   track_locked:frame.track_locked,saved_track:frame.saved_track,
   keyboard_binding:frame.keyboard_binding});
 }
 fs.writeFileSync(path.join(directory,'cycle.json'),JSON.stringify({stage:'browser platform present',frames:metadata,scope:'complete rendered cycle with observed car pose; audio and wall-clock timing not compared'},null,2));
 await page.screenshot({path:path.join(output,name,'screenshot.png')});
 console.log(`Browser cycle ${name}: ${cycleFrames} rendered presentations`);
 return name;
}
(async()=>{
 const server=serve(path.resolve(process.argv[2]));await new Promise(resolve=>server.listen(0,resolve));
 let browser;
 try{
  browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:560}}),errors=[];
  page.on('pageerror',error=>errors.push(error.message));
  await page.route('**/index.html*',route=>{
   const html=fs.readFileSync(path.join(path.resolve(process.argv[2]),'index.html'),'utf8');
   const marker='<script async type="text/javascript" src="index.js"></script>';
   assert(html.includes(marker),'missing production runtime script');
   const hook='<script>Module.preRun.push(function(){var sync=FS.syncfs;'+
    'FS.syncfs=function(populate,done){return sync.call(FS,populate,function(error){'+
    'if(populate&&!error)FS.writeFile("/persist/SaveGames",Uint8Array.from(atob('+JSON.stringify(save.toString('base64'))+'),c=>c.charCodeAt(0)));'+
    'done(error);});};});</script>';
   return route.fulfill({status:200,contentType:'text/html',body:html.replace(marker,hook+marker)});
  });
  await page.addInitScript(limit=>{
   window.__releaseKey=null;window.__cycleCapture=false;
   const present=CanvasRenderingContext2D.prototype.putImageData;
   const encode=(address,size)=>{
    let text='';for(let i=0;i<size;i+=16384)text+=String.fromCharCode(...HEAPU8.subarray(address+i,address+Math.min(i+16384,size)));
    return btoa(text);
   };
   CanvasRenderingContext2D.prototype.putImageData=function(...args){
    const result=present.apply(this,args);
    if(this.canvas.id==='canvas'){
     if(window.__cycleCapture){
      let mismatches=0;const rgba=this.getImageData(0,0,640,480).data;
      for(let i=0;i<307200;i++){
       const palette=0x700050+HEAPU8[0x700450+i]*4,pixel=i*4;
       if(rgba[pixel]!==HEAPU8[palette] || rgba[pixel+1]!==HEAPU8[palette+1] ||
          rgba[pixel+2]!==HEAPU8[palette+2] || rgba[pixel+3]!==255)mismatches++;
      }
      window.__cycle.push({phase:HEAP32[0x4699cc>>2],cf:HEAP32[0x462ff0>>2],level:HEAP32[0x936ff4>>2],
       race_car:HEAP32[0x467400>>2],car_angles:[0,1,2].map(i=>HEAP16[(0x468eb4>>1)+i]),poly_list:HEAPU32[0x940010>>2],
       race_mode:HEAP32[0x4673f8>>2],race_type:HEAP32[0x4673f4>>2],race_track:HEAP32[0x4673fc>>2],
       playable_tracks:HEAP32[0x467404>>2],playable_bowls:HEAP32[0x467408>>2],
       track_locked:HEAPU8[0x46a920],saved_track:HEAP32[0x940224>>2],
       keyboard_binding:{saved:Array.from(HEAPU8.subarray(0x46757a,0x46758c)),
        working:Array.from(HEAPU8.subarray(0x93fd90,0x93fda2)),
        active:Array.from(HEAPU8.subarray(0x46302c,0x46303a)),pad_option:HEAP32[0x467414>>2]},
       framebuf:encode(0x700450,307200),palette:encode(0x700050,1024),canvas_mismatches:mismatches});
      if(window.__cycle.length===limit)window.__cycleCapture=false;
     }
     if(window.__releaseKey){
      const code=window.__releaseKey;window.__releaseKey=null;
      window.dispatchEvent(new KeyboardEvent('keyup',{code}));
     }
    }
    return result;
   };
  },cycleFrames);
  await boot(page,server);
  assert(Buffer.from(await page.evaluate(()=>Array.from(Module.FS.readFile('/SaveGames')))).equals(save),
   'initial save input differs from supplied original/native file');
  const checkpoints=[await capture(page,0,null)];
  for(let i=0;i<keys.length;i++){
   await tap(page,keys[i]);checkpoints.push(await capture(page,i+1,keys[i]));
  }
  assert.deepEqual(errors,[],'browser runtime errors');
  fs.writeFileSync(path.join(output,'navigation.json'),JSON.stringify({keys,checkpoints,input:'browser keyboard events',
   initial_save_sha256:saveHash,wasm_sha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(path.resolve(process.argv[2]),'index.wasm'))).digest('hex')},null,2));
  console.log(`Browser menu captures: ${output}`);
 }finally{if(browser)await browser.close();await new Promise(resolve=>server.close(resolve));}
})().catch(error=>{console.error(error);process.exitCode=1;});
