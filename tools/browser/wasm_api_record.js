// Observe production WASM clock returns, DOM inputs and LCG state.
// No engine/binary writes or replacement return values; explicit diagnostic
// inputs, not physical timing or original A/V acceptance.
const assert=require('assert'),fs=require('fs'),path=require('path'),crypto=require('crypto');

async function createApiRecorder(page,layout,build,out){
 assert.equal(crypto.createHash('sha256').update(fs.readFileSync(path.join(build,'index.wasm'))).digest('hex'),layout.wasm_sha256,'API layout belongs to another WASM');
 assert(Number.isInteger(layout.clock_function_ordinal)&&Number.isInteger(layout.imported_function_count));
 assert(Number.isInteger(layout.seed_address)&&Number.isInteger(layout.counter_address));
 await page.addInitScript(layout=>{
  const log=window.__actualApiRecord={ticks:[],inputs:[],presentations:[],rng:[],error:null,clock_imports:0};
  const seeds=[1],wrapped=new WeakSet();
  const clockMarker='wasm-function['+(layout.clock_function_ordinal+layout.imported_function_count)+']';
  let previousCount=-1;
  const state=()=>{
   if(typeof HEAPU32==='undefined')return null;
   const count=HEAPU32[layout.counter_address>>2],seed=HEAPU32[layout.seed_address>>2];
   if(count>1000000){log.error='RNG recording exceeds its bounded extent';return null;}
   while(seeds.length<=count)seeds.push((Math.imul(seeds.at(-1),1103515245)+12345)>>>0);
   if(seed!==seeds[count])log.error='Observed RNG differs from seed-one LCG';
   const row={level:HEAP32[0x936ff4>>2],cf:HEAP32[0x462ff0>>2],ticks:HEAP32[0x7746c0>>2],
    countdown:HEAP32[0x784298>>2],frame_skip:HEAP32[0x7746b8>>2],quit:HEAP32[0x7746ac>>2],
    poly_list:HEAPU32[0x940010>>2],movie:HEAP32[0x462cd4>>2],
    clock_count:log.ticks.length,rng_count:count,rng_seed:seed};
   const memory=new DataView(HEAPU8.buffer),readInt=address=>memory.getInt32(address,true);
   Object.assign(row,{speed:readInt(0x792a7a),planar_speed:readInt(0x792a76),
    heading:HEAPU16[0x78a792>>1]&4095,x:readInt(0x78a744),z:readInt(0x78a74c),
    dead:readInt(0x792ac6),front_damage:readInt(0x792aee),rear_damage:readInt(0x792af6),
    lap:HEAPU16[0x795c48>>1],lap_progress:HEAPU16[0x795c4a>>1],
    finished_laps:HEAPU16[0x795c52>>1],race_points:HEAPU16[0x795c46>>1]});
   if(count!==previousCount){log.rng.push(row);previousCount=count;}
   return row;
  };
  const instrument=imports=>{
   for(const namespace of Object.values(imports||{})){
    const original=namespace.emscripten_get_now;
    if(typeof original!=='function'||wrapped.has(original))continue;
    const observer=function(...args){
     const result=original.apply(this,args);
     if(new Error().stack.includes(clockMarker)){
      if(log.ticks.length<10000000)log.ticks.push(result>>>0);
      else log.error='Clock recording exceeds its bounded extent';
      if(typeof HEAPU32!=='undefined'&&HEAPU32[layout.counter_address>>2]!==previousCount)state();
     }
     return result;
    };
    wrapped.add(observer);namespace.emscripten_get_now=observer;log.clock_imports++;
   }
  };
  for(const name of ['instantiate','instantiateStreaming']){
   const original=WebAssembly[name];
   if(original)WebAssembly[name]=function(source,imports){instrument(imports);return original.call(this,source,imports);};
  }
  for(const name of ['keydown','keyup'])window.addEventListener(name,event=>{
   if(log.inputs.length>=20000){log.error='Input recording exceeds its bounded extent';return;}
   log.inputs.push({code:event.code,down:name==='keydown',repeat:event.repeat,trusted:event.isTrusted,
    presentation:log.presentations.length-1,state:state()});
  },true);
  const present=CanvasRenderingContext2D.prototype.putImageData;
  CanvasRenderingContext2D.prototype.putImageData=function(...args){
   const result=present.apply(this,args);
   if(this.canvas.id==='canvas'){
    if(log.presentations.length>=30000)log.error='Presentation recording exceeds its bounded extent';
    else log.presentations.push(state());
   }
   return result;
  };
  window.__flushActualApiRecord=()=>{state();return log;};
 },layout);
 let flushed=false;
 return {async flush(){
  if(flushed)return;
  const record=await page.evaluate(()=>window.__flushActualApiRecord());
  assert(record,'API observer was not installed');
  assert.equal(record.error,null,'Actual API observer failed');
  assert.equal(record.clock_imports,1,'Actual clock import must be wrapped exactly once');
  assert(record.ticks.length>0&&record.rng.length>0,'Actual clock/RNG observations required');
  const ticks=Buffer.alloc(record.ticks.length*4);
  record.ticks.forEach((value,i)=>ticks.writeUInt32LE(value,i*4));
  const extent=record.rng.reduce((maximum,row)=>Math.max(maximum,row.rng_count),0);
  const random=Buffer.alloc(extent*12);let seed=1;
  for(let i=0;i<extent;i++){
   const after=(Math.imul(seed,1103515245)+12345)>>>0;
   random.writeUInt32LE(seed,i*12);random.writeUInt32LE(after,i*12+4);random.writeUInt32LE((after>>>16)&32767,i*12+8);seed=after;
  }
  fs.writeFileSync(path.join(out,'actual-ticks.bin'),ticks);
  fs.writeFileSync(path.join(out,'calculated-random.bin'),random);
  const {ticks:values,...metadata}=record;
  fs.writeFileSync(path.join(out,'api-record.json'),JSON.stringify({scope:'Observed production WASM GetTickCount returns and actual DOM input boundaries; seed-one LCG triples reconstructed and checked against observed seed/counter checkpoints. No original, whole-game or physical A/V parity claim',
   layout,clock_calls:values.length,rng_calls:extent,clock_sha256:crypto.createHash('sha256').update(ticks).digest('hex'),
   random_sha256:crypto.createHash('sha256').update(random).digest('hex'),...metadata},null,2));
  flushed=true;
 }};
}
module.exports={createApiRecorder};
