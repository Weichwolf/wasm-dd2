const assert=require('assert'),fs=require('fs'),path=require('path');
const {createHash}=require('crypto');
const ROOT=path.resolve(__dirname,'../..');
const {serve,boot,chromium}=require(ROOT+'/tools/browser/felib');
const {installMenuInput}=require(ROOT+'/tools/browser/menu_input');
const build=path.resolve(process.argv[2]),output=path.resolve(process.argv[3]);
assert(process.argv.length===5,'usage: BUILD OUTPUT ORIGINAL_CAPTURE');
const originalCapture=path.resolve(process.argv[4]);
const {execFileSync,spawn}=require('child_process');
const layout=JSON.parse(fs.readFileSync(ROOT+'/tools/championship_save_layout.json'));
assert(output.startsWith('/tmp/wasm-dd2/')&&!output.split('/').includes('..'));fs.mkdirSync(output);
const sha=b=>createHash('sha256').update(b).digest('hex');
const original=JSON.parse(fs.readFileSync(originalCapture+'/report.json'));
const card=fs.readFileSync(originalCapture+'/initial.card');
assert(original.pass_&&original.target==='original'&&!original.engine_state_writes,'Completed unmodified original lap/name/save capture required');
assert(sha(card)===original.initial_card_sha256&&card.length===0x20000,'Same declared initial card required');
const report={scope:'Actual browser time-trial lap, Fastest Lap name entry, configuration overwrite and restart from a declared edited saved-time file input; real trusted Playwright keys and read-only observations. Independent driving does not establish chronological race, audio/video parity or a lap beating the provisioned default record.',target:'browser',pass_:false,engine_state_writes:false,input_keys:[],checkpoints:[],trusted_keyboard_events:[],binary_sha256:sha(fs.readFileSync(build+'/index.wasm')),helper_sha256:sha(fs.readFileSync(ROOT+'/tools/driver_name_input.py')),initial_card_sha256:sha(card),observer_sha256:sha(fs.readFileSync(__filename)),original_capture:originalCapture,initial_file_edits:original.initial_file_edits,original_source_fixture:original.original_source_fixture,original_source_card_sha256:original.original_source_card_sha256,driver_sha256:sha(fs.readFileSync(ROOT+'/tools/live_lap_record_driver.py')),rpc_sha256:sha(fs.readFileSync(ROOT+'/tools/live_lap_record_rpc.py'))};
const nav={Left:'ArrowLeft',Right:'ArrowRight',Up:'ArrowUp',Down:'ArrowDown',Return:'Enter',Escape:'Escape'};
async function ready(page,poly){await page.waitForFunction(poly=>HEAP16[0x46996c>>1]===0 && window.__nameReadyFrames>=16 && (poly===undefined || HEAPU32[0x940010>>2]===poly),poly);}
async function key(page,code){
  const input=await page.evaluate(code=>{
  const vk={Left:37,Right:39,Up:38,Down:40,Return:13,Escape:27}[code];
  const bits=[1,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768];
  const index=[0,1,2,3,4,5,8,6,9,7,10,11,12,13].find(i=>HEAPU8[0x46302c+i]===vk);
  if(index===undefined)throw new Error('Unmapped navigation key');
  return {mask:bits[index],level:HEAP32[0x936ff4>>2],live:HEAP32[0x936ff4>>2]>=1&&HEAP32[0x936ff4>>2]<=10&&HEAP32[0x7746ac>>2]===0};
},code);
const mask=input.mask;
if(!input.live)await ready(page);
  await page.waitForFunction(mask=>((HEAPU16[0x754448>>1]|HEAPU16[0x75444a>>1])&mask)===0,mask);
  await page.keyboard.down(nav[code]);
  try{await page.waitForFunction(({mask,level})=>((HEAPU16[0x754448>>1]|HEAPU16[0x75444a>>1])&mask)!==0||HEAP32[0x936ff4>>2]!==level,input);}
  finally{await page.keyboard.up(nav[code]);}
  await page.waitForFunction(mask=>(HEAPU16[0x754448>>1]&mask)===0,mask);
  // Count fresh presentations of the current menu before another real key.
  // A menu pointer can change before its next slab animation starts.
  await page.evaluate(()=>{window.__nameReadyFrames=0;});
  await page.waitForTimeout(300);report.input_keys.push(code);
}
async function observed(page){return page.evaluate(layout=>{
  const d=new DataView(HEAPU8.buffer),hex=(a,n)=>Array.from(HEAPU8.subarray(a,a+n),b=>b.toString(16).padStart(2,'0')).join('');
  const state={joy_present:HEAPU8[0x754451]};
  for(const [name,a] of layout.fields)state[name]=d.getInt32(a,true);
  for(const [name,a,n] of layout.regions)state[name]=hex(a,n);
  return {state,ui:{menu:HEAPU32[0x940010>>2],cursor:[HEAP16[0x469f34>>1],HEAP16[0x469f36>>1]],entered:UTF8ToString(HEAPU32[0x469fd4>>2]+8),title:UTF8ToString(HEAPU32[0x46a024>>2]),names:hex(0x93e318,120),type:HEAP32[0x4673f4>>2],playable_tracks:HEAP32[0x467404>>2],playable_bowls:HEAP32[0x467408>>2]}};
},layout);}
async function checkpoint(page,name,cycle=false){
  await ready(page);const row={name,input_end:report.input_keys.length,...await observed(page)};
  if(cycle){
    const directory=output+'/'+name+'/cycle';fs.mkdirSync(directory,{recursive:true});
    await page.evaluate(()=>{window.__nameFrames=[];window.__captureName=true;});
    await page.waitForFunction(()=>!window.__captureName,null,{timeout:15000});
    const frames=await page.evaluate(()=>window.__nameFrames),metadata=[];
    assert(frames.length===64 && new Set(frames.map(f=>f.phase)).size===64);
    for(let i=0;i<frames.length;i++){
      const f=frames[i],prefix=`frame${String(i).padStart(3,'0')}`;
      assert(f.canvas_mismatches===0,'Actual canvas differs');
      for(const region of ['framebuf','palette'])fs.writeFileSync(directory+'/'+prefix+'-'+region+'.bin',Buffer.from(f[region],'base64'));
      const {framebuf,palette,...state}=f;metadata.push({...state,index:i,prefix});
    }
    checkBudget();fs.writeFileSync(directory+'/cycle.json',JSON.stringify({stage:'browser platform present',frames:metadata},null,2)+'\n');row.cycle=directory;
  }
  report.checkpoints.push(row);console.log(name,row.ui.entered);
}
// The browser owns normal game state. Host control never writes its heap.
const rpcProcess=spawn('python3',[ROOT+'/tools/live_lap_record_rpc.py'],{stdio:['pipe','pipe','pipe']});
const rpcQueue=[];let rpcError=null;
require('readline').createInterface({input:rpcProcess.stdout}).on('line',line=>{const waiter=rpcQueue.shift();if(waiter)waiter.resolve(JSON.parse(line));});
rpcProcess.stderr.on('data',data=>{rpcError=String(data);});
rpcProcess.on('exit',(code)=>{if(code!==0){rpcError=rpcError||`Road observer exited ${code}`;for(const waiter of rpcQueue.splice(0))waiter.reject(new Error(rpcError));}});
function rpc(data){if(rpcError)return Promise.reject(new Error(rpcError));return new Promise((resolve,reject)=>{rpcQueue.push({resolve,reject});rpcProcess.stdin.write(JSON.stringify(data)+'\n');});}
function timePacked(value){return ((value[0]<<24)|(value[1]<<16)|value[2])>>>0;}
const drivingInputs=[],held=[];let finished=null,driveError=null,observations=0;
const drivingLog=fs.createWriteStream(output+'/driving.jsonl');
function checkBudget(){
  function size(directory){return fs.readdirSync(directory,{withFileTypes:true}).reduce((total,entry)=>{const name=path.join(directory,entry.name);return total+(entry.isSymbolicLink()?0:entry.isDirectory()?size(name):fs.statSync(name).size);},0);}
  assert(size(output)<2*1024**3,'Verification run exceeded 2 GiB');
  const disk=fs.statfsSync(output);assert(disk.bavail*disk.bsize>=1024**3,'Less than 1 GiB free');
}

const drivingKeys={a:'a',z:'z',space:'Space',Left:'ArrowLeft',Right:'ArrowRight'};
(async()=>{
  const server=serve(build),regular=server.listeners('request')[0];server.removeAllListeners('request');
  server.on('request',(request,response)=>{if(request.url==='/fixture.html'){response.writeHead(200,{'Content-Type':'text/html'});response.end('<!doctype html><title>Initial file setup</title>');}else regular(request,response);});
  await new Promise(r=>server.listen(0,r));let browser,watchdog;
  try{
    const disk=fs.statfsSync(output);assert(disk.bavail*disk.bsize>=1024**3);
    browser=await chromium.launch({args:['--no-sandbox']});watchdog=setTimeout(()=>browser.close().catch(()=>{}),700000);
    const page=await browser.newPage();await installMenuInput(page);
    await page.goto(`http://localhost:${server.address().port}/fixture.html`);
    await page.evaluate(bytes=>new Promise((resolve,reject)=>{const request=indexedDB.open('/persist',21);
      request.onupgradeneeded=()=>{const store=request.result.createObjectStore('FILE_DATA');store.createIndex('timestamp','timestamp',{unique:false});};
      request.onerror=()=>reject(request.error);request.onsuccess=()=>{const db=request.result,tx=db.transaction('FILE_DATA','readwrite');tx.objectStore('FILE_DATA').put({timestamp:new Date(),mode:0o100644,contents:Uint8Array.from(bytes)},'/persist/SaveGames');tx.oncomplete=()=>{db.close();resolve();};tx.onerror=()=>reject(tx.error);};
    }),Array.from(card));
    async function dispatch(key,down){assert(drivingKeys[key],`Unsupported driving key ${key}`);await page.keyboard[down?'down':'up'](drivingKeys[key]);const tick=await page.evaluate(()=>HEAP32[0x7746c0>>2]);drivingInputs.push({tick,key,down});}
    async function release(){for(const key of held.splice(0))await dispatch(key,false);}
    await page.exposeFunction('observeTrial',async snapshot=>{
      try{
        if(finished||driveError)return;
        const row=await rpc({operation:'observe',segments:snapshot.segments});observations++;
        drivingLog.write(JSON.stringify({...row,runtime:snapshot.runtime,cf:snapshot.cf})+'\n');
        if(observations%250===0){checkBudget();fs.writeFileSync(output+'/progress.json',JSON.stringify({observations,row,runtime:snapshot.runtime},null,2)+'\n');console.log('Browser trial',row.tick,row.lap,row.lap_progress,row.speed);}
        assert(snapshot.level===1&&snapshot.type===1&&snapshot.quit===0,'Actual live first-track time trial required');
        assert(!row.dead&&row.lap<4,'No better lap before destruction/three laps');
        if(row.lap>=2&&timePacked(snapshot.runtime)<timePacked(snapshot.saved)){finished={...snapshot,segments:undefined,driver:row};await release();return;}
        const wanted=row.wanted;
        for(const key of held.slice())if(!wanted.includes(key)){await dispatch(key,false);held.splice(held.indexOf(key),1);}
        for(const key of wanted)if(!held.includes(key)){await dispatch(key,true);held.push(key);}
      }catch(error){driveError=error.stack;await release();}
    });
    await page.exposeFunction('observeNameKey',event=>report.trusted_keyboard_events.push(event));
    await page.addInitScript(()=>{
      window.__captureName=false;window.__nameReadyFrames=0;window.__nameLastMenu=null;window.__trialDrive=false;window.__trialControlPending=false;window.__trialTick=null;
      for(const type of ['keydown','keyup'])window.addEventListener(type,e=>window.observeNameKey({type,code:e.code,trusted:e.isTrusted}));
      const present=CanvasRenderingContext2D.prototype.putImageData;
      const encode=(a,n)=>{let t='';for(let i=0;i<n;i+=16384)t+=String.fromCharCode(...HEAPU8.subarray(a+i,a+Math.min(i+16384,n)));return btoa(t);};
      CanvasRenderingContext2D.prototype.putImageData=function(...args){
        const result=present.apply(this,args);
        if(this.canvas.id==='canvas' && typeof HEAP16!=='undefined'){
          const menu=HEAPU32[0x940010>>2];
          window.__nameReadyFrames=HEAP16[0x46996c>>1]===0 ? (menu===window.__nameLastMenu ? window.__nameReadyFrames+1 : 1) : 0;
          window.__nameLastMenu=menu;
          const d=new DataView(HEAPU8.buffer),i32=a=>d.getInt32(a,true);
          if(window.__trialDrive&&!window.__trialControlPending&&i32(0x936ff4)===1&&i32(0x784298)<0&&i32(0x7746c0)!==window.__trialTick){
            window.__trialControlPending=true;window.__trialTick=i32(0x7746c0);
            const ranges=[[0x792690,44],[0x7929e0,288],[0x78a744,80],[0x795c40,24],
              [0x936ff4,4],[0x93ded0,4],[0x7746b8,4],[0x46704c,12],[0x7746c0,4],[0x466e3c,12]];
            const runtime=[i32(0x466e3c),i32(0x466e40),i32(0x466e44)];
            const saved=[d.getUint16(0x4680ca,true),d.getUint16(0x4680cc,true),d.getUint16(0x4680ce,true)];
            window.observeTrial({segments:ranges.map(([a,n])=>[a,encode(a,n)]),runtime,saved,cf:i32(0x462ff0),level:i32(0x936ff4),type:i32(0x4673f4),quit:i32(0x7746ac)})
              .catch(error=>{window.__trialError=String(error);}).finally(()=>{window.__trialControlPending=false;});
          }
        }
        if(this.canvas.id==='canvas' && window.__captureName){
          let mismatches=0;const rgba=this.getImageData(0,0,640,480).data;
          for(let i=0;i<307200;i++){const pal=0x700050+HEAPU8[0x700450+i]*4,pixel=i*4;if(rgba[pixel]!==HEAPU8[pal]||rgba[pixel+1]!==HEAPU8[pal+1]||rgba[pixel+2]!==HEAPU8[pal+2]||rgba[pixel+3]!==255)mismatches++;}
          window.__nameFrames.push({phase:HEAP32[0x4699cc>>2],card_phase:HEAP32[0x467390>>2]%255,cf:HEAP32[0x462ff0>>2],level:HEAP32[0x936ff4>>2],poly_list:HEAPU32[0x940010>>2],sound_volume:HEAP32[0x467410>>2],working_sound_volume:HEAP32[0x93fd20>>2],master_sfx_volume:HEAP32[0x462d84>>2],framebuf:encode(0x700450,307200),palette:encode(0x700050,1024),canvas_mismatches:mismatches});
          if(window.__nameFrames.length===64)window.__captureName=false;
        }return result;
      };
    });
    const errors=[];page.on('pageerror',e=>errors.push(e.message));await boot(page,server);
    async function keys(codes){for(const code of codes)await key(page,code);}
    await keys(['Return','Return','Right','Right','Return','Right','Return']);
    let carType=await page.evaluate(()=>HEAP32[0x467400>>2]);
    for(let i=0;i<3&&carType!==1;i++){await key(page,'Right');carType=await page.evaluate(()=>HEAP32[0x467400>>2]);}
    await key(page,'Return');assert.strictEqual(carType,1);report.before=(await observed(page)).state;
    await keys(['Down','Down','Return']);
    await page.waitForFunction(()=>HEAP32[0x936ff4>>2]===1&&HEAP32[0x4673f4>>2]===1&&HEAP32[0x46765c>>2]===1&&HEAP32[0x784298>>2]<0,null,{timeout:60000});
    const geometry=await page.evaluate(()=>{
      const data=new DataView(HEAPU8.buffer),i32=a=>data.getInt32(a,true),u32=a=>data.getUint32(a,true);
      const base=u32(0x77cef8),vertices=u32(0x77cef4),segments=[],vertexIndices=new Set(),offsets=new Set();
      const encode=(a,n)=>{assertBounds(a,n);let text='';for(let i=0;i<n;i++)text+=String.fromCharCode(HEAPU8[a+i]);return btoa(text);};
      function assertBounds(a,n){if(a<0||a+n>HEAPU8.length)throw new Error('Readonly geometry bounds');}
      const current=i32(0x7926a4),lane=HEAP8[0x7926ba],initialLanes=HEAPU8[base+4+current+1];
      if(initialLanes<1||lane<0||lane>=initialLanes)throw new Error('Actual initial road lane required');
      const fraction=(lane+.5)/initialLanes;
      const globalLane=(lane+HEAPU8[base+4+current+3])&255;let offset=0;
      for(let index=0;index<4096;index++){
        if(offset<0||offset>=0x100000||offsets.has(offset))throw new Error('Road loop bounds');offsets.add(offset);
        const a=base+4+offset;assertBounds(a,28);segments.push([a,encode(a,28)]);
        const kind=HEAPU8[a],lanes=HEAPU8[a+1],first=data.getUint16(a+16,true);
        if(kind>9||lanes<1||lanes>32)throw new Error('Road strip shape');
        const firstA=first+i32(0x463dcc+kind*8),firstB=first+i32(0x463dd0+kind*8)+lanes+1;
        const requested=((globalLane-HEAPU8[a+3]+128)&255)-128;
        const selected=Math.max(0,Math.min(lanes-1,requested));
        for(const k of [firstA,firstA+lanes,firstB,firstB+lanes,firstA+selected,firstA+selected+1,firstB+selected,firstB+selected+1])vertexIndices.add(k);
        offset=i32(a+20);if(offset===0)break;
      }
      if(offset!==0)throw new Error('Road loop did not close');
      for(const k of vertexIndices){if(k<0||k>100000)throw new Error('Vertex bounds');segments.push([vertices+k*12,encode(vertices+k*12,12)]);}
      segments.push([0x77cef4,encode(0x77cef4,8)],[0x463dcc,encode(0x463dcc,80)],[0x466df8,encode(0x466df8,2)]);
      return {segments,strips:offsets.size,vertices:vertexIndices.size,lane_fraction:fraction,global_lane:globalLane};
    });
    report.geometry={strips:geometry.strips,vertices:geometry.vertices,lane_fraction:geometry.lane_fraction,global_lane:geometry.global_lane};
    await rpc({operation:'geometry',segments:geometry.segments});
    await page.evaluate(()=>{window.__trialDrive=true;});
    const started=Date.now();
    while(!finished&&!driveError&&Date.now()-started<300000)await page.waitForTimeout(500);
    await page.evaluate(()=>{window.__trialDrive=false;});await release();
    assert(!driveError,driveError);assert(finished,'Actual better completed lap required');
    report.drive={final:finished,driving_inputs:drivingInputs,observations,engine_state_writes:false};
    await keys(['Escape','Down','Down','Down','Return','Up','Return']);await ready(page,0x469f70);
    assert.strictEqual((await observed(page)).ui.title,'%R%JC%T/Fastest Lap');
    await checkpoint(page,'record-name-empty',true);
    const namePlan=JSON.parse(execFileSync('python3',['-c',"import sys,json;sys.path.insert(0,sys.argv[1]);from driver_name_input import name_actions;print(json.dumps(name_actions('D')))",ROOT+'/tools'],{encoding:'utf8'}));
    for(const a of namePlan.slice(0,-1)){await key(page,a.key);const state=(await observed(page)).ui;assert.deepStrictEqual(state.cursor,a.cursor);assert.strictEqual(state.entered,a.entered);}
    await checkpoint(page,'record-name-typed',true);await key(page,'Return');await ready(page,0x4696b0);
    report.after=(await observed(page)).state;
    const before=Buffer.from(report.before.fastest,'hex'),after=Buffer.from(report.after.fastest,'hex');
    assert(after.subarray(0,2).equals(Buffer.from('D\0'))&&after.subarray(16,80).equals(before.subarray(0,64))&&after.subarray(80).equals(before.subarray(80)));
    assert.deepStrictEqual([after.readUInt16LE(10),after.readUInt16LE(12),after.readUInt16LE(14)],finished.runtime);
    await key(page,'Up');assert((await page.evaluate(()=>UTF8ToString(HEAPU32[0x46975c>>2]))).includes('Configuration'));
    await keys(['Return','Right','Right','Return']);await ready(page,0x4671ec);
    report.saved_state=(await observed(page)).state;
    const expected=await page.evaluate(()=>Array.from(HEAPU8.subarray(0x93a490,0x93a490+0x197e)));
    const packed=Buffer.alloc(0x197e);packed.writeUInt16LE(0x1010);packed.writeUInt16LE(report.saved_state.joy_present,18);
    for(const [name,,offset] of layout.fields)packed.writeUInt16LE(report.saved_state[name]&0xffff,offset);
    for(const [name,,size,offset] of layout.regions){const bytes=Buffer.from(report.saved_state[name],'hex');assert(bytes.length===size);bytes.copy(packed,offset);}
    assert(packed.equals(Buffer.from(expected)),'Complete live configuration packing differs');report.payload_sha256=sha(packed);
    await keys(['Return','Right','Return','Left','Return']);
    await keys(['Down','Down','Return','Up','Up','Right','Return','Down','Down','Right','Return']);
    await page.waitForFunction(expected=>FS.readFile('/SaveGames').subarray(0x4000,0x597e).every((value,index)=>value===expected[index]),expected);
    const actual=await page.evaluate(()=>({raw:Array.from(FS.readFile('/SaveGames')),ram:Array.from(HEAPU8.subarray(0x754460,0x774460))}));
    const savedCard=Buffer.from(actual.raw);assert(savedCard.equals(Buffer.from(actual.ram))&&savedCard.subarray(0x4000,0x597e).equals(Buffer.from(expected)));
    assert(savedCard.subarray(0,0x200).equals(card.subarray(0,0x200))&&savedCard.subarray(0x2000,0x4000).equals(card.subarray(0x2000,0x4000)));
    fs.writeFileSync(output+'/saved.card',savedCard);report.card_sha256=sha(savedCard);
    await page.evaluate(()=>new Promise((resolve,reject)=>FS.syncfs(false,error=>error?reject(error):resolve())));
    await boot(page,server);report.reloaded_state=(await observed(page)).state;
    assert.deepStrictEqual(report.reloaded_state,report.saved_state,'Reload did not restore actual new record/configuration');
    const reloaded=await page.evaluate(()=>({raw:Array.from(FS.readFile('/SaveGames')),ram:Array.from(HEAPU8.subarray(0x754460,0x774460))}));
    assert(savedCard.equals(Buffer.from(reloaded.raw))&&savedCard.equals(Buffer.from(reloaded.ram)),'Reload changed actual persisted file/card RAM');
    assert(errors.length===0,errors.join('; '));assert(report.trusted_keyboard_events.every(e=>e.trusted));
    report.pass_=true;report.engine_matches_file=true;report.configuration_restored=true;
  }catch(error){report.error=error.stack;throw error;}
  finally{drivingLog.end();rpcProcess.kill();clearTimeout(watchdog);fs.writeFileSync(output+'/report.json',JSON.stringify(report,null,2)+'\n');if(browser)await browser.close();await new Promise(r=>server.close(r));}
})();
