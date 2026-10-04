// Load the same actual original card through production IDBFS and real keys.
// Seed only the initial file, before starting the engine; observers are read-only.
const assert=require('assert'),fs=require('fs'),path=require('path'),{createHash}=require('crypto');
const {serve,boot,chromium}=require('./felib');
const {installMenuInput}=require('./menu_input');
const build=path.resolve(process.argv[2]),output=path.resolve(process.argv[3]);
const fixture=path.resolve(process.argv[4]),reference=path.resolve(process.argv[5]);
const layoutBytes=fs.readFileSync(path.join(__dirname,'../championship_save_layout.json'));
const layout=JSON.parse(layoutBytes),sha=b=>createHash('sha256').update(b).digest('hex');
assert(output.startsWith('/tmp/wasm-dd2/'));
const parent=fs.realpathSync(path.dirname(output));
assert(parent==='/tmp/wasm-dd2'||parent.startsWith('/tmp/wasm-dd2/'));
fs.mkdirSync(output);
const card=fs.readFileSync(path.join(fixture,'original.card'));
const producer=JSON.parse(fs.readFileSync(path.join(fixture,'report.json')));
const source=JSON.parse(fs.readFileSync(path.join(reference,'report.json')));
assert(producer.pass_&&producer.operation==='generate'&&producer.card_sha256===sha(card));
assert(source.pass_&&source.target==='original'&&source.initial_save_sha256===sha(card));
assert(source.binary_sha256==='0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2');
assert(fs.readFileSync(path.join(build,'index.js'),'utf8').includes('DB_VERSION:21'),
  'initial file seeder requires the Emscripten IDBFS v21 schema');
const report={scope:source.scope,operation:'capture',target:'browser',pass_:false,
  engine_state_writes:false,initial_file_setup:'original.card seeded in IndexedDB before engine startup',
  initial_save_sha256:sha(card),layout_sha256:sha(layoutBytes),checkpoints:[],
  binary_sha256:sha(fs.readFileSync(path.join(build,'index.wasm'))),trusted_keyboard_events:[]};
const nav={Left:'ArrowLeft',Right:'ArrowRight',Up:'ArrowUp',Down:'ArrowDown',Return:'Enter',Escape:'Escape'};
function budget(){
  function size(p){return fs.readdirSync(p,{withFileTypes:true}).reduce((n,e)=>{
    const f=path.join(p,e.name);return n+(e.isDirectory()?size(f):e.isFile()?fs.statSync(f).size:0);
  },0);}
  const disk=fs.statfsSync(output);
  assert(size(output)<2*1024**3&&disk.bavail*disk.bsize>=1024**3,'verification budget exceeded');
}
async function key(page,code){
  const input=await page.evaluate(code=>{
    const vk={Left:37,Right:39,Up:38,Down:40,Return:13,Escape:27}[code];
    const bits=[1,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768];
    const index=[0,1,2,3,4,5,8,6,9,7,10,11,12,13].find(i=>HEAPU8[0x46302c+i]===vk);
    if(index===undefined)throw new Error('unmapped navigation key');
    return {mask:bits[index],level:HEAP32[0x936ff4>>2],
      live:HEAP32[0x936ff4>>2]>=1&&HEAP32[0x936ff4>>2]<=10&&HEAP32[0x7746ac>>2]===0};
  },code);
  if(!input.live)await page.waitForFunction(()=>HEAP16[0x46996c>>1]===0&&window.__slabReadyFrames>=16);
  await page.waitForFunction(mask=>((HEAPU16[0x754448>>1]|HEAPU16[0x75444a>>1])&mask)===0,input.mask);
  await page.keyboard.down(nav[code]);
  try{await page.waitForFunction(({mask,level})=>
    ((HEAPU16[0x754448>>1]|HEAPU16[0x75444a>>1])&mask)!==0||HEAP32[0x936ff4>>2]!==level,input);}
  finally{await page.keyboard.up(nav[code]);}
  await page.waitForFunction(mask=>(HEAPU16[0x754448>>1]&mask)===0,input.mask);
  await page.waitForTimeout(300);console.log('Browser key',code);
}
async function checkpoint(page,name){
  await page.waitForFunction(()=>HEAP16[0x46996c>>1]===0&&window.__slabReadyFrames>=16);
  const row=await page.evaluate(name=>({name,menu:HEAPU32[0x940010>>2],
    file_mode:HEAP32[0x93a318>>2],file_slot:HEAP32[0x774680>>2],file_ui:{
      caption:UTF8ToString(HEAPU32[0x46725c>>2]),detail:UTF8ToString(HEAPU32[0x4672c0>>2]),
      selection:UTF8ToString(HEAPU32[0x467284>>2]),
      name_cursor:[HEAP16[0x4673dc>>1],HEAP16[0x4673de>>1]],
      ring:[HEAP16[0x467198>>1],HEAP16[0x46719a>>1]]}}),name);
  const wanted=JSON.parse(fs.readFileSync(path.join(source.checkpoints.find(r=>r.name===name).cycle,'cycle.json')))
    .frames.map(f=>[f.phase,f.card_phase]);
  await page.evaluate(wanted=>{
    window.__saveFrames=[];window.__saveEntries=0;
    window.__saveWanted=new Set(wanted.map(p=>p.join(':')));window.__captureSave=true;
  },wanted);
  await page.waitForFunction(()=>!window.__captureSave,null,{timeout:420000});
  const frames=await page.evaluate(()=>window.__saveFrames);
  assert(frames.length===64&&new Set(frames.map(f=>f.phase)).size===64);
  const directory=path.join(output,name,'cycle');fs.mkdirSync(directory,{recursive:true});
  const metadata=[];
  for(const [index,frame] of frames.entries()){
    assert(frame.canvas_mismatches===0,'actual canvas differs');
    const prefix=`frame${String(index).padStart(3,'0')}`;
    for(const region of ['framebuf','palette'])fs.writeFileSync(path.join(directory,prefix+'-'+region+'.bin'),Buffer.from(frame[region],'base64'));
    const {framebuf,palette,...state}=frame;metadata.push({...state,index,prefix});
  }
  fs.writeFileSync(path.join(directory,'cycle.json'),JSON.stringify({stage:'browser platform present',frames:metadata,
    observed_entries:await page.evaluate(()=>window.__saveEntries),wanted_pairs:wanted},null,2)+'\n');
  row.cycle=directory;report.checkpoints.push(row);budget();
}
(async()=>{
  const server=serve(build);
  const regular=server.listeners('request')[0];server.removeListener('request',regular);
  server.on('request',(request,response)=>{
    if(request.url==='/fixture.html'){
      response.writeHead(200,{'Content-Type':'text/html'});
      response.end('<!doctype html><title>Initial file setup</title>');
    }else regular(request,response);
  });
  await new Promise(r=>server.listen(0,r));
  let browser,watchdog,guard;const errors=[];
  try{
    budget();browser=await chromium.launch({args:['--no-sandbox']});
    watchdog=setTimeout(()=>{errors.push('verification timed out');browser.close().catch(()=>{});},1000000);
    guard=setInterval(()=>{try{budget();}catch(e){errors.push(e.message);browser.close().catch(()=>{});}},1000);
    const page=await browser.newPage();await installMenuInput(page);
    await page.goto(`http://localhost:${server.address().port}/fixture.html`);
    await page.evaluate(bytes=>new Promise((resolve,reject)=>{
      const request=indexedDB.open('/persist',21);
      request.onupgradeneeded=()=>{const store=request.result.createObjectStore('FILE_DATA');store.createIndex('timestamp','timestamp',{unique:false});};
      request.onerror=()=>reject(request.error);
      request.onsuccess=()=>{
        const db=request.result,transaction=db.transaction('FILE_DATA','readwrite');
        transaction.objectStore('FILE_DATA').put({timestamp:new Date(),mode:0o100644,contents:Uint8Array.from(bytes)},'/persist/SaveGames');
        transaction.oncomplete=()=>{db.close();resolve();};transaction.onerror=()=>reject(transaction.error);
      };
    }),Array.from(card));
    await page.exposeFunction('observeSaveKey',event=>report.trusted_keyboard_events.push(event));
    await page.addInitScript(()=>{
      window.__captureSave=false;
      for(const type of ['keydown','keyup'])window.addEventListener(type,e=>window.observeSaveKey({type,code:e.code,trusted:e.isTrusted}));
      const present=CanvasRenderingContext2D.prototype.putImageData;
      const encode=(a,n)=>{let s='';for(let i=0;i<n;i+=16384)s+=String.fromCharCode(...HEAPU8.subarray(a+i,a+Math.min(i+16384,n)));return btoa(s);};
      CanvasRenderingContext2D.prototype.putImageData=function(...args){
        const result=present.apply(this,args);
        if(this.canvas.id==='canvas'&&window.__captureSave){
          if(++window.__saveEntries>3328)throw new Error('requested animation pairs not reached');
          const phase=HEAP32[0x4699cc>>2],card_phase=HEAP32[0x467390>>2]%255;
          if(!window.__saveWanted.delete(phase+':'+card_phase))return result;
          const rgba=this.getImageData(0,0,640,480).data;let mismatches=0;
          for(let i=0;i<307200;i++){const p=0x700050+HEAPU8[0x700450+i]*4,j=i*4;
            if(rgba[j]!==HEAPU8[p]||rgba[j+1]!==HEAPU8[p+1]||rgba[j+2]!==HEAPU8[p+2]||rgba[j+3]!==255)mismatches++;}
          window.__saveFrames.push({phase,card_phase,cf:HEAP32[0x462ff0>>2],level:HEAP32[0x936ff4>>2],
            poly_list:HEAPU32[0x940010>>2],sound_volume:HEAP32[0x467410>>2],
            working_sound_volume:HEAP32[0x93fd20>>2],master_sfx_volume:HEAP32[0x462d84>>2],
            framebuf:encode(0x700450,307200),palette:encode(0x700050,1024),canvas_mismatches:mismatches});
          if(window.__saveFrames.length===64)window.__captureSave=false;
        }
        return result;
      };
    });
    page.on('pageerror',e=>errors.push(e.message));await boot(page,server);
    report.initial=await page.evaluate(()=>({mode:HEAP32[0x4673f8>>2],type:HEAP32[0x4673f4>>2],car:HEAP32[0x467400>>2],
      track:HEAP32[0x4673fc>>2],sound:HEAP32[0x467410>>2],pad_option:HEAP32[0x467414>>2],
      saved:Array.from(HEAPU8.subarray(0x46757a,0x46758c)),active:Array.from(HEAPU8.subarray(0x46302c,0x46303a))}));
    assert(report.initial.mode===0&&report.initial.type===0&&report.initial.car===0,'game auto-loaded as configuration');
    for(const code of ['Right','Right','Right','Return'])await key(page,code);
    await checkpoint(page,'manager');await key(page,'Return');
    await page.waitForFunction(()=>UTF8ToString(HEAPU32[0x46725c>>2]).includes('Select')&&HEAP32[0x774680>>2]===0);
    await checkpoint(page,'selected');await key(page,'Return');
    await page.waitForFunction(()=>HEAP32[0x7746c0>>2]>=8&&HEAP32[0x7746ac>>2]===0&&
      HEAP32[0x936ff4>>2]>=1&&HEAP32[0x936ff4>>2]<=10,null,{timeout:60000});
    await key(page,'Escape');report.pause_ticks=[];
    for(let i=0;i<3;i++){report.pause_ticks.push(await page.evaluate(()=>HEAP32[0x7746c0>>2]));await page.waitForTimeout(200);}
    assert(new Set(report.pause_ticks).size===1,'loaded race did not pause');
    report.loaded=await page.evaluate(layout=>{
      const data=new DataView(HEAPU8.buffer),hex=(a,n)=>Array.from(HEAPU8.subarray(a,a+n),b=>b.toString(16).padStart(2,'0')).join('');
      const state={};for(const [name,a]of layout.fields)state[name]=data.getInt32(a,true);
      for(const [name,a,n]of layout.regions)state[name]=hex(a,n);
      return {...state,joy_present:HEAPU8[0x754451],level:HEAP32[0x936ff4>>2],replay_level:HEAP32[0x9392bc>>2],
        quit:HEAP32[0x7746ac>>2],replay:HEAP32[0x467074>>2],active_bindings:hex(0x46302c,14),order:hex(0x795c28,20)};
    },layout);
    const disk=await page.evaluate(()=>{const raw=FS.readFile('/SaveGames');return {raw:Array.from(raw),matches:raw.every((b,i)=>b===HEAPU8[0x754460+i])};});
    report.card_unchanged=Buffer.from(disk.raw).equals(card);report.engine_matches_file=disk.matches;
    assert(report.card_unchanged&&report.engine_matches_file);assert(errors.length===0,errors.join('; '));
    assert(report.trusted_keyboard_events.length>0&&report.trusted_keyboard_events.every(e=>e.trusted));
    report.pass_=true;budget();
  }finally{
    clearTimeout(watchdog);clearInterval(guard);if(browser)await browser.close();server.close();
    fs.writeFileSync(path.join(output,'report.json'),JSON.stringify({...report,errors},null,2)+'\n');
  }
  console.log('Actual browser championship loading: PASS');
})().catch(e=>{console.error(e.stack);process.exitCode=1;});
