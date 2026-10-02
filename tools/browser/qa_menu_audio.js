// Real menu navigation must produce audible effects while the race counter is fixed.
const assert=require('assert'),path=require('path');
const {serve,key,boot,menuLabel,chromium}=require('./felib');
(async()=>{
 const server=serve(path.resolve(process.argv[2]||'web/dd2'));
 await new Promise(resolve=>server.listen(0,resolve));
 let browser;
 try{
  browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage();const errors=[];
  page.on('pageerror',error=>errors.push(error.message));
  await page.addInitScript(()=>{
   window.__menuAudio={buffers:0,frames:0,nonzero:0};
   const create=AudioContext.prototype.createBufferSource;
   AudioContext.prototype.createBufferSource=function(...args){
    const source=create.apply(this,args),start=source.start;
    source.start=function(...args){
     const buffer=source.buffer,state=window.__menuAudio;
     if(buffer && buffer.sampleRate===22050){
      state.buffers++;state.frames+=buffer.length;
      for(let channel=0;channel<buffer.numberOfChannels;channel++)
       for(const sample of buffer.getChannelData(channel)) if(sample!==0)state.nonzero++;
     }
     return start.apply(this,args);
    };
    return source;
   };
  });
  await boot(page,server);
  const initial=await page.evaluate(()=>({cf:HEAP32[0x462ff0>>2],level:HEAP32[0x936ff4>>2]}));
  const first=await menuLabel(page);
  await key(page,'ArrowRight',220);
  assert.notEqual(await menuLabel(page),first,'menu navigation did not change selection');
  for(const code of ['ArrowLeft','ArrowDown','ArrowUp'])await key(page,code,220);
  const result=await page.evaluate(()=>({...__menuAudio,cf:HEAP32[0x462ff0>>2],level:HEAP32[0x936ff4>>2],rate:Module._dd2ac&&Module._dd2ac.sampleRate}));
  console.log('Menu audio:',JSON.stringify(result));
  assert(initial.level===0 && result.level===0 && result.cf===initial.cf,'test left the menu or advanced the race counter');
  assert(result.buffers>0 && result.nonzero>0,'navigation produces no audible effect buffers');
  assert(result.rate===44100,'shared audio device must preserve CD rate');
  assert.deepEqual(errors,[],'browser runtime errors');
  console.log('PASS menu navigation effects with fixed race counter');
 }finally{if(browser)await browser.close();await new Promise(resolve=>server.close(resolve));}
})().catch(error=>{console.error(error);process.exitCode=1;});
