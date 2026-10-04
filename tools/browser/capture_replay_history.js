// Compare actual replay racing presentations with explicit original API inputs.
// Seed only the initial file, before starting the engine; observers are read-only.
const assert=require('assert'),fs=require('fs'),path=require('path'),{createHash}=require('crypto');
const {serve,boot,chromium}=require('./felib');
const {installMenuInput}=require('./menu_input');
const build=path.resolve(process.argv[2]),output=path.resolve(process.argv[3]);
const fixture=path.resolve(process.argv[4]),reference=path.resolve(process.argv[5]);
const layout=JSON.parse(fs.readFileSync(path.resolve(process.argv[6]))),zlib=require('zlib');
const sha=b=>createHash('sha256').update(b).digest('hex');
assert(output.startsWith('/tmp/wasm-dd2/'));
const parent=fs.realpathSync(path.dirname(output));
assert(parent==='/tmp/wasm-dd2'||parent.startsWith('/tmp/wasm-dd2/'));
fs.mkdirSync(output);
const card=fs.readFileSync(path.join(fixture,'original.card'));
const producer=JSON.parse(fs.readFileSync(path.join(fixture,'report.json')));
const capture=JSON.parse(fs.readFileSync(path.join(reference,'report.json')));
const source=JSON.parse(fs.readFileSync(path.join(reference,'history/history.json')));
const ticks=fs.readFileSync(path.join(reference,'history/ticks.bin'));
const random=fs.readFileSync(path.join(reference,'history/random.bin'));
assert(producer.pass_&&producer.operation==='generate'&&producer.card_sha256===sha(card));
assert(source.pass_&&source.target==='original'&&capture.initial_save_sha256===sha(card));
assert(ticks.length===source.clock_calls*4&&random.length===source.rng_calls*12);
assert(layout.wasm_sha256===sha(fs.readFileSync(path.join(build,'index.wasm'))));
assert(capture.binary_sha256==='0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2');
assert(fs.readFileSync(path.join(build,'index.js'),'utf8').includes('DB_VERSION:21'),
  'initial file seeder requires the Emscripten IDBFS v21 schema');
const report={scope:source.scope,operation:'capture',target:'browser',pass_:false,
  engine_state_writes:false,initial_file_setup:'original.card seeded in IndexedDB before engine startup',
  initial_save_sha256:sha(card),frames:[],
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
(async()=>{
  const server=serve(build);
  const regular=server.listeners('request')[0];server.removeListener('request',regular);
  server.on('request',(request,response)=>{
    if(request.url==='/fixture.html'){
      response.writeHead(200,{'Content-Type':'text/html'});
      response.end('<!doctype html><title>Initial file setup</title>');
    }else if(request.url.startsWith('/index.html')){
      const html=fs.readFileSync(path.join(build,'index.html'),'utf8');
      const marker='<script async type="text/javascript" src="index.js"></script>';assert(html.includes(marker));
      const hook='<script>Module.preRun.unshift(function(){delete ENV.DD2_REALTIME;'+
        'FS.writeFile("/original-ticks.bin",Uint8Array.from(atob('+JSON.stringify(ticks.toString('base64'))+'),c=>c.charCodeAt(0)));'+
        'FS.writeFile("/original-random.bin",Uint8Array.from(atob('+JSON.stringify(random.toString('base64'))+'),c=>c.charCodeAt(0)));'+
        'ENV.DD2_TICK_REPLAY="/original-ticks.bin";ENV.DD2_RANDOM_REFERENCE="/original-random.bin";'+
        'ENV.DD2_RANDOM_LEVEL="all";ENV.DD2_RANDOM_REQUIRE_INITIAL="1";});</script>';
      response.writeHead(200,{'Content-Type':'text/html'});response.end(html.replace(marker,hook+marker));
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
    await page.addInitScript(layout=>{
      window.__racingFrames=[];window.__recordRacing=false;window.__historyDone=false;
      const regions=[['car_fd',0x792690,20*44],
        ['car_state',0x792a00,20*0x1b2],['wheel_fd',0x794be8,20*0xb0]];
      const encode=(a,n)=>{let s='';for(let i=0;i<n;i+=16384)s+=String.fromCharCode(...HEAPU8.subarray(a+i,a+Math.min(i+16384,n)));return btoa(s);};
      for(const type of ['keydown','keyup'])window.addEventListener(type,e=>window.observeSaveKey({type,code:e.code,trusted:e.isTrusted}));
      const present=CanvasRenderingContext2D.prototype.putImageData;
      CanvasRenderingContext2D.prototype.putImageData=function(...args){
        const result=present.apply(this,args);
        if(this.canvas.id==='canvas'&&window.__recordRacing&&HEAP32[0x936ff4>>2]===1&&
           HEAP32[0x467074>>2]===1&&HEAP32[0x7746ac>>2]===0&&HEAP32[0x7746c0>>2]>0&&
           HEAPU32[layout.clock_counter_address>>2]>0){
          if(window.__racingFrames.length>=1000)throw new Error('replay frame budget exceeded');
          const rgba=this.getImageData(0,0,640,480).data;let mismatches=0;
          for(let i=0;i<307200;i++){const p=0x700050+HEAPU8[0x700450+i]*4,j=i*4;
            if(rgba[j]!==HEAPU8[p]||rgba[j+1]!==HEAPU8[p+1]||rgba[j+2]!==HEAPU8[p+2]||rgba[j+3]!==255)mismatches++;}
          const memory=new DataView(HEAPU8.buffer);
          const row={level:HEAP32[0x936ff4>>2],cf:HEAP32[0x462ff0>>2],ticks:HEAP32[0x7746c0>>2],
            countdown:HEAP32[0x784298>>2],frame_skip:HEAP32[0x7746b8>>2],quit:HEAP32[0x7746ac>>2],
            replay:HEAP32[0x467074>>2],car:HEAP32[0x467400>>2],mode:HEAP32[0x4673f8>>2],type:HEAP32[0x4673f4>>2],
            end:HEAPU32[0x9392c4>>2],script_cursor:HEAPU32[0x9392b4>>2],first_time:HEAP32[0x9392b0>>2],
            pedal:memory.getInt32(0x792a86,true),poly_list:HEAPU32[0x940010>>2],phase:HEAP32[0x4699cc>>2],
            render_buffer:HEAP32[0x462fec>>2],
            clock_calls:HEAPU32[layout.clock_counter_address>>2],rng_calls:HEAPU32[layout.random_replay_counter_address>>2],
            rng_seed:HEAPU32[layout.seed_address>>2],canvas_mismatches:mismatches,
            framebuf:encode(0x700450,307200),palette:encode(0x700050,1024),regions:{}};
          for(const [name,a,n] of regions)row.regions[name]=encode(a,n);
          let pose='';for(let car=0;car<20;car++)pose+=String.fromCharCode(...HEAPU8.subarray(0x78a744+car*0x27c,0x78a79c+car*0x27c));
          row.regions.car_pose=btoa(pose);row.primitive_storage=encode(0x78a520,20*0x27c);
          window.__racingFrames.push(row);
        }
        if(window.__recordRacing&&window.__racingFrames.length&&HEAP32[0x467074>>2]===0&&
           HEAP32[0x7746ac>>2]===1&&HEAP32[0x936ff4>>2]===0){
          window.__historyDone=true;window.__recordRacing=false;
        }
        return result;
      };
    },layout);
    page.on('pageerror',e=>errors.push(e.message));await boot(page,server);
    report.initial=await page.evaluate(()=>({mode:HEAP32[0x4673f8>>2],type:HEAP32[0x4673f4>>2],car:HEAP32[0x467400>>2],
      track:HEAP32[0x4673fc>>2],sound:HEAP32[0x467410>>2],pad_option:HEAP32[0x467414>>2],
      saved:Array.from(HEAPU8.subarray(0x46757a,0x46758c)),active:Array.from(HEAPU8.subarray(0x46302c,0x46303a))}));
    assert(report.initial.mode===0&&report.initial.type===0&&report.initial.car===0,'replay auto-loaded as configuration');
    for(const code of ['Right','Right','Right','Return'])await key(page,code);
    await key(page,'Return');
    await page.waitForFunction(()=>UTF8ToString(HEAPU32[0x46725c>>2]).includes('Select')&&HEAP32[0x774680>>2]===0);
    await page.evaluate(()=>{window.__recordRacing=true;});await key(page,'Return');
    await page.waitForFunction(()=>window.__historyDone,null,{timeout:60000});
    report.completion=await page.evaluate(()=>({script_cursor:HEAPU32[0x9392b4>>2],first_time:HEAP32[0x9392b0>>2]}));
    assert.deepStrictEqual(report.completion,{script_cursor:card.readUInt32LE(0x2004),first_time:1});
    report.clock_calls=await page.evaluate(layout=>HEAPU32[layout.clock_counter_address>>2],layout);
    report.rng_calls=await page.evaluate(layout=>HEAPU32[layout.random_replay_counter_address>>2],layout);
    assert(report.clock_calls===source.clock_calls&&report.rng_calls===source.rng_calls,'API extents differ');
    const rows=await page.evaluate(()=>window.__racingFrames);assert(rows.length>0);
    const directory=path.join(output,'history');fs.mkdirSync(directory);
    for(const [index,row] of rows.entries()){
      assert(row.canvas_mismatches===0,'actual canvas differs');
      const prefix=`frame${String(index).padStart(4,'0')}`,raw=Buffer.from(row.framebuf,'base64'),palette=Buffer.from(row.palette,'base64');
      fs.writeFileSync(path.join(directory,prefix+'.bin.z'),zlib.deflateSync(raw));
      fs.writeFileSync(path.join(directory,prefix+'.pal'),palette);
      const regions={};for(const [name,value]of Object.entries(row.regions))regions[name]=sha(Buffer.from(value,'base64'));
      const {framebuf,palette:unused,regions:ignored,primitive_storage,...state}=row;
      report.frames.push({...state,index,prefix,regions,framebuffer_sha256:sha(raw),palette_sha256:sha(palette),
        primitive_storage_sha256:sha(Buffer.from(primitive_storage,'base64'))});
    }
    // The production random oracle checks every computed triple; the observed
    // counters/seeds additionally expose its extent at each presentation.
    let seed=1;const seeds=[seed];for(let i=0;i<source.rng_calls;i++){seed=(Math.imul(seed,1103515245)+12345)>>>0;seeds.push(seed);}
    assert(report.frames.every(f=>f.rng_seed===seeds[f.rng_calls]),'calculated RNG seed differs');
    fs.writeFileSync(path.join(directory,'input-ticks.bin'),ticks);
    fs.writeFileSync(path.join(directory,'input-random.bin'),random);
    report.api_inputs={clock_sha256:sha(ticks),random_sha256:sha(random),
      method:'original clock file input; production oracle checks every computed RNG triple; read-only counters and seeds observed at each actual canvas presentation'};
    const disk=await page.evaluate(()=>{const raw=FS.readFile('/SaveGames');return {raw:Array.from(raw),matches:raw.every((b,i)=>b===HEAPU8[0x754460+i])};});
    report.card_unchanged=Buffer.from(disk.raw).equals(card);report.engine_matches_file=disk.matches;
    assert(report.card_unchanged&&report.engine_matches_file);assert(errors.length===0,errors.join('; '));
    assert(report.trusted_keyboard_events.length>0&&report.trusted_keyboard_events.every(e=>e.trusted));
    report.pass_=true;budget();
    fs.writeFileSync(path.join(output,'history/history.json'),JSON.stringify(report,null,2)+'\n');
  }finally{
    clearTimeout(watchdog);clearInterval(guard);if(browser)await browser.close();server.close();
    fs.writeFileSync(path.join(output,'report.json'),JSON.stringify({...report,errors},null,2)+'\n');
  }
  console.log('Actual browser replay racing history captured; exact comparison pending');
})().catch(e=>{console.error(e.stack);process.exitCode=1;});
