// Capture complete highlight cycles after normal keyboard navigation.
// No engine-state writes; each input is released after a presented frame.
const assert=require('assert'),fs=require('fs'),path=require('path');
const {serve,boot,chromium}=require('./felib');
const codes={Left:'ArrowLeft',Right:'ArrowRight',Up:'ArrowUp',Down:'ArrowDown',Return:'Enter',Escape:'Escape',F1:'F1',F2:'F2'};
const output=process.argv[3] && path.resolve(process.argv[3]),keys=process.argv.slice(4);
if(!output || keys.some(key=>!codes[key]))throw new Error('Usage: node capture_menu_cycle.js <web build> <fresh output> [Left Right Up Down Return Escape F1 F2 ...]');
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
 await page.waitForFunction(()=>window.__cycleCapture===false,null,{timeout:10000});
 const frames=await page.evaluate(()=>window.__cycle);
 assert.equal(frames.length,64,'incomplete browser cycle');
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
  metadata.push({index:i,prefix,phase:frame.phase,cf:frame.cf,level:frame.level,canvas_mismatches:frame.canvas_mismatches});
 }
 fs.writeFileSync(path.join(directory,'cycle.json'),JSON.stringify({stage:'browser platform present',frames:metadata,scope:'complete rendered highlight cycle; audio and wall-clock timing not compared'},null,2));
 await page.screenshot({path:path.join(output,name,'screenshot.png')});
 console.log(`Browser cycle ${name}: 64 rendered phases`);
 return name;
}
(async()=>{
 const server=serve(path.resolve(process.argv[2]));await new Promise(resolve=>server.listen(0,resolve));
 let browser;
 try{
  browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:560}}),errors=[];
  page.on('pageerror',error=>errors.push(error.message));
  await page.addInitScript(()=>{
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
      let mismatches=0;const rgba=args[0].data;
      for(let i=0;i<307200;i++){
       const palette=0x700050+HEAPU8[0x700450+i]*4,pixel=i*4;
       if(rgba[pixel]!==HEAPU8[palette] || rgba[pixel+1]!==HEAPU8[palette+1] ||
          rgba[pixel+2]!==HEAPU8[palette+2] || rgba[pixel+3]!==255)mismatches++;
      }
      window.__cycle.push({phase:HEAP32[0x4699cc>>2],cf:HEAP32[0x462ff0>>2],level:HEAP32[0x936ff4>>2],
       framebuf:encode(0x700450,307200),palette:encode(0x700050,1024),canvas_mismatches:mismatches});
      if(window.__cycle.length===64)window.__cycleCapture=false;
     }
     if(window.__releaseKey){
      const code=window.__releaseKey;window.__releaseKey=null;
      window.dispatchEvent(new KeyboardEvent('keyup',{code}));
     }
    }
    return result;
   };
  });
  await boot(page,server);
  const checkpoints=[await capture(page,0,null)];
  for(let i=0;i<keys.length;i++){
   await tap(page,keys[i]);checkpoints.push(await capture(page,i+1,keys[i]));
  }
  assert.deepEqual(errors,[],'browser runtime errors');
  fs.writeFileSync(path.join(output,'navigation.json'),JSON.stringify({keys,checkpoints,input:'browser keyboard events'},null,2));
  console.log(`Browser menu captures: ${output}`);
 }finally{if(browser)await browser.close();await new Promise(resolve=>server.close(resolve));}
})().catch(error=>{console.error(error);process.exitCode=1;});
