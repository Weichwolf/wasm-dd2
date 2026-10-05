// Observe actual canvas bytes. References stay in Node, never in the engine.
const fs=require('fs'),path=require('path'),assert=require('assert'),crypto=require('crypto'),zlib=require('zlib');
const {serve,chromium}=require('./felib');
const build=path.resolve(process.argv[2]),capture=path.resolve(process.argv[3]),output=path.resolve(process.argv[4]);
assert(output.startsWith('/tmp/wasm-dd2/'),'Use /tmp/wasm-dd2/');fs.mkdirSync(output);
const manifest=JSON.parse(fs.readFileSync(path.join(capture,'movie-video-archive/manifest.json')));
const checkpoint=JSON.parse(fs.readFileSync(path.join(capture,'checkpoint.json')));
assert(manifest.pass_===true && manifest.frames===manifest.records.length && checkpoint.exe_modified===false && checkpoint.end_state.movie===0,'incomplete original');
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
const frameBytes=640*480*4,windowOffset=128+32768+320*192*3;
const diagnose=process.argv.includes('--diagnose');
function original(frame){
 const entry=manifest.records[frame];assert(entry && entry.serial===frame && entry.file===String(frame).padStart(6,'0')+'.zlib','extra/reordered canvas frame');
 const packed=fs.readFileSync(path.join(capture,'movie-video-archive',entry.file));
 assert(packed.length===entry.compressed_bytes && sha(packed)===entry.compressed_sha256,'changed compressed original');
 const raw=zlib.inflateSync(packed,{maxOutputLength:1446016});
 assert(raw.length===1446016 && sha(raw)===entry.raw_sha256 && raw.readUInt32LE(8)===frame,'changed original movie readback');
 return raw.subarray(windowOffset);
}
(async()=>{
 const server=serve(build);await new Promise(r=>server.listen(0,r));let browser,page,profiler;
 const started=Date.now();
 const pending=new Map(),seen=new Set(),digest=crypto.createHash('sha256');let next=0;
 const report={scope:'Literal actual canvas/original-window video in chronological order. No captured pixels/state feed the port. Presentation clocks, audio endpoints and physical display remain open.',pass_:false,frames:0,literal_window_argb_bytes:0,negative_controls:[]};
 try{
  browser=await chromium.launch({args:['--no-sandbox']});page=await browser.newPage();const errors=[];
  if(diagnose){profiler=await page.context().newCDPSession(page);await profiler.send('Profiler.enable');await profiler.send('Profiler.start');}
  page.on('pageerror',e=>errors.push(e.message));page.on('crash',()=>errors.push('renderer crash'));
  const acceptFrame=async(frame,actual)=>{
   assert(Number.isInteger(frame) && frame>=0 && !seen.has(frame),'duplicate canvas frame');seen.add(frame);
   assert(actual.length===frameBytes,'incomplete actual canvas frame');const expected=original(frame);
   assert(actual.equals(expected),`actual canvas frame ${frame} differs from original`);
   if(frame===0){
    const changed=Buffer.from(actual);changed[1000]^=1;
    assert(!changed.equals(expected),'accepted changed actual canvas bit');
    assert(!actual.subarray(0,-4).equals(expected),'accepted truncated actual canvas');
    report.negative_controls.push('canvas-bit','truncated-canvas');
   }
   pending.set(frame,actual);assert(pending.size<=64,'canvas comparison backlog exceeded bound');
   while(pending.has(next)){digest.update(pending.get(next));pending.delete(next++);}
   report.frames++;report.literal_window_argb_bytes+=actual.length;
   if(report.frames%256===0){
    fs.writeFileSync(path.join(output,'progress.json'),JSON.stringify({matchedOriginalFrames:report.frames,elapsedSeconds:(Date.now()-started)/1000})+'\n');
    console.log('Exact actual canvas/original frames:',report.frames);
   }
  };
  const staticRequest=server.listeners('request')[0];server.removeListener('request',staticRequest);
  server.on('request',(request,response)=>{
   const url=new URL(request.url,'http://localhost');
   if(url.pathname!=='/__dd2_observed_frame'){staticRequest(request,response);return;}
   const pieces=[];let bytes=0;
   request.on('data',part=>{bytes+=part.length;if(bytes>frameBytes)request.destroy();else pieces.push(part);});
   request.on('end',async()=>{
    try{await acceptFrame(Number(url.searchParams.get('frame')),Buffer.concat(pieces));response.end('OK');}
    catch(error){response.writeHead(400);response.end(String(error));}
   });
  });
  await page.addInitScript(()=>{
   window.__video={frames:0,pending:0,packed:[],packedBytes:0,errors:[],readbackMs:0,copyMs:0};let movie=false;
   const source=`const queue=[];let running=false;onmessage=event=>{queue.push(event.data);drain();};async function drain(){if(running)return;running=true;try{while(queue.length){const {frame,data}=queue.shift();const stream=new Blob([data]).stream().pipeThrough(new CompressionStream('deflate'));const packed=await new Response(stream).arrayBuffer();postMessage({frame,packed},[packed]);}}catch(error){postMessage({error:String(error)});}finally{running=false;}}`;
   const worker=new Worker(URL.createObjectURL(new Blob([source],{type:'application/javascript'})));
   worker.onmessage=event=>{const state=__video;if(event.data.error){state.errors.push(event.data.error);return;}state.packed[event.data.frame]=event.data.packed;state.packedBytes+=event.data.packed.byteLength;state.pending--;if(state.packedBytes>512*1024*1024)state.errors.push('compressed canvas capture exceeded memory bound');};
   worker.onerror=event=>__video.errors.push(event.message);
   function observe(imports){
    if(!imports?.env?.movie_present)return;const call=imports.env.movie_present;
    imports.env.movie_present=function(...args){movie=true;try{return call(...args);}finally{movie=false;}};
   }
   for(const name of ['instantiate','instantiateStreaming']){
    const call=WebAssembly[name];if(call)WebAssembly[name]=function(bytes,imports,...rest){observe(imports);return call.call(this,bytes,imports,...rest);};
   }
   const call=CanvasRenderingContext2D.prototype.putImageData;
   CanvasRenderingContext2D.prototype.putImageData=function(...args){
    const result=call.apply(this,args);
    if(movie && this.canvas.id==='canvas'){
     const begin=performance.now(),state=__video,frame=state.frames++,rgba=this.getImageData(0,0,640,480).data,argb=new Uint8Array(rgba.length),copied=performance.now();
     // Source window records are little-endian ARGB, canvas readbacks RGBA.
     for(let i=0;i<rgba.length;i+=4){argb[i]=rgba[i+2];argb[i+1]=rgba[i+1];argb[i+2]=rgba[i];argb[i+3]=rgba[i+3];}
     state.readbackMs+=copied-begin;state.copyMs+=performance.now()-copied;
     // Budget both queued raw readbacks and completed compressed frames.
     // A count of 64 can abort a valid movie while compression catches up.
     state.pending++;
     if(state.pending*640*480*4+state.packedBytes>1280*1024*1024){
      state.errors.push('movie readback memory budget exceeded');state.pending--;return result;
     }
     worker.postMessage({frame,data:argb.buffer},[argb.buffer]);
    }
    return result;
   };
  });
  await page.goto(`http://localhost:${server.address().port}/index.html?movie=INTRO.AVI`);
  await page.waitForFunction(()=>!!Module._dd2movieSource && HEAP32[0x462cd4>>2]===1,null,{timeout:15000});
  await page.click('#canvas');
  await page.waitForFunction(()=>HEAP32[0x462cd4>>2]===0 && !Module._dd2movieSource,null,{timeout:diagnose?15000:100000});
  console.log('Actual browser movie completed; draining bounded readback worker');
  await page.waitForFunction(()=>__video.pending===0 || __video.errors.length,null,{timeout:30000});
  const state=await page.evaluate(async()=>{
   const state=__video;
   if(state.errors.length)return {frames:state.frames,pending:state.pending,errors:state.errors};
   for(let frame=0;frame<state.frames;frame++){
    const stream=new Blob([state.packed[frame]]).stream().pipeThrough(new DecompressionStream('deflate'));
    const data=await new Response(stream).arrayBuffer();
    const reply=await fetch('/__dd2_observed_frame?frame='+frame,{method:'POST',body:data});
    if(!reply.ok)state.errors.push(await reply.text());
    state.packed[frame]=null;
   }
   return {frames:state.frames,pending:state.pending,errors:state.errors};
  });
  assert(!errors.length && !state.errors.length,JSON.stringify([...errors,...state.errors]));
  assert(state.pending===0 && state.frames===manifest.frames && report.frames===manifest.frames && next===manifest.frames && !pending.size,'actual canvas/original presentation count differs');
  report.window_sha256=digest.digest('hex');report.pass_=true;
  report.elapsed_seconds=(Date.now()-started)/1000;
  report.wasm_sha256=sha(fs.readFileSync(path.join(build,'index.wasm')));
  fs.writeFileSync(path.join(output,'report.json'),JSON.stringify(report,null,2)+'\n');
  console.log('Every actual canvas byte equals original window:',report.frames);
 }catch(error){
  if(profiler){const result=await profiler.send('Profiler.stop');fs.writeFileSync(path.join(output,'cpu-profile.json'),JSON.stringify(result.profile));}
  const state=page ? await page.evaluate(()=>({frames:__video?.frames,pending:__video?.pending,errors:__video?.errors,packedBytes:__video?.packedBytes,readbackMs:__video?.readbackMs,copyMs:__video?.copyMs,movie:typeof HEAP32!=='undefined'?HEAP32[0x462cd4>>2]:null,audioState:typeof Module!=='undefined'?Module._dd2movieClock?.state:null,audioTime:typeof Module!=='undefined'?Module._dd2movieClock?.currentTime:null})).catch(()=>null) : null;
  fs.writeFileSync(path.join(output,'failure.json'),JSON.stringify({error:String(error),matchedOriginalFrames:report.frames,state},null,2)+'\n');
  throw error;
 }finally{if(browser)await browser.close();await new Promise(r=>server.close(r));}
})().catch(e=>{console.error(e);process.exitCode=1});
