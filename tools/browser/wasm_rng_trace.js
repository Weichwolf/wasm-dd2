// Read-only Chromium debugger diagnosis of actual production WASM rand callers.
// Pauses alter wall-clock time. This is a causal trace, not A/V acceptance.
const assert=require('assert'),crypto=require('crypto');

async function createRngTrace(page,layout){
 const cdp=await page.context().newCDPSession(page),scripts=[],calls=[];
 let error=null,breakpoint=null,armed=false,race=null;
 cdp.on('Debugger.scriptParsed',script=>{if(script.scriptLanguage==='WebAssembly')scripts.push(script);});
 cdp.on('Debugger.paused',async event=>{
  try{
   if(!armed||!event.hitBreakpoints.includes(breakpoint))throw new Error('Unexpected debugger pause');
   if(calls.length>=500)throw new Error('RNG caller trace exceeded 500 calls');
   const expression=`({seed:HEAPU32[${layout.seed_address}>>2],count:HEAPU32[${layout.counter_address}>>2],level:HEAP32[0x936ff4>>2],cf:HEAP32[0x462ff0>>2],ticks:HEAP32[0x7746c0>>2],frame_skip:HEAP32[0x7746b8>>2],quit:HEAP32[0x7746ac>>2],countdown:HEAP32[0x784298>>2]})`;
   const read=await cdp.send('Runtime.evaluate',{expression,returnByValue:true});
   assert(!read.exceptionDetails,'Cannot read actual RNG/engine state during pause');
   calls.push({race,...read.result.value,stack:event.callFrames.map(frame=>({function:frame.functionName,location:frame.location}))});
  }catch(e){error=e.message;}
  finally{await cdp.send('Debugger.resume').catch(e=>{error=e.message;});}
 });
 await cdp.send('Debugger.enable');
 assert.equal(scripts.length,1,'One actual production WASM module required');
 const script=scripts[0],source=await cdp.send('Debugger.getScriptSource',{scriptId:script.scriptId});
 assert(source.bytecode,'Actual debugger WASM bytecode required');
 assert.equal(crypto.createHash('sha256').update(Buffer.from(source.bytecode,'base64')).digest('hex'),layout.wasm_sha256,'Debugger module differs from production WASM');
 const ordinal=Number(layout.function.match(/^\(func \$(\d+)\b/)[1]);
 const disassembly=await cdp.send('Debugger.disassembleWasmModule',{scriptId:script.scriptId});
 const offsets=disassembly.functionBodyOffsets;
 assert(offsets.length>2*ordinal+1,'Actual RNG function body missing');
 const start={scriptId:script.scriptId,lineNumber:0,columnNumber:offsets[2*ordinal]};
 const end={scriptId:script.scriptId,lineNumber:0,columnNumber:offsets[2*ordinal+1]};
 const possible=await cdp.send('Debugger.getPossibleBreakpoints',{start,end,restrictToFunction:true});
 assert(possible.locations.length,'Actual RNG function has no debugger location');
 const location=possible.locations[0];
 return {
  async arm(nextRace){assert(!armed);race=nextRace;const result=await cdp.send('Debugger.setBreakpoint',{location});breakpoint=result.breakpointId;armed=true;},
  async disarm(){if(armed){await cdp.send('Debugger.removeBreakpoint',{breakpointId:breakpoint});armed=false;}assert.equal(error,null,'RNG caller trace failed');},
  report(){return {scope:'Read-only debugger call stacks in actual production WASM; pauses alter physical timing; no parity claim',wasm_sha256:layout.wasm_sha256,rand_definition_ordinal:ordinal,rand_breakpoint:location,calls,error};},
  async close(){await cdp.detach();}
 };
}
module.exports={createRngTrace};
