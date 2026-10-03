// Observe a real player arena race until its natural end; no engine state writes.
// This is input/finish diagnosis, not original A/V acceptance.
const assert=require('assert'),fs=require('fs'),path=require('path'),crypto=require('crypto');
const {serve,boot,key,waitRace,chromium}=require('./felib');
const build=path.resolve(process.argv[2]||'web/dd2'),out=path.resolve(process.argv[3]||'');
assert(out.startsWith('/tmp/wasm-dd2/'));
assert(!fs.existsSync(out)||fs.readdirSync(out).length===0,'Fresh diagnostic directory required');
fs.mkdirSync(out,{recursive:true});
const save=fs.readFileSync(path.resolve(__dirname,'../../DestructionDerby2/SaveGames'));
const total=process.argv.includes('--total');
(async()=>{
 const server=serve(build);await new Promise(r=>server.listen(0,r));let browser;
 const errors=[],observations=[],inputs=[];
 try{
  browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage({viewport:{width:700,height:560}});
  page.on('pageerror',e=>errors.push(e.message));
  await page.route('**/index.html*',route=>{
   const html=fs.readFileSync(path.join(build,'index.html'),'utf8'),marker='<script async type="text/javascript" src="index.js"></script>';
   assert(html.includes(marker));
   const hook='<script>Module.preRun.push(function(){var sync=FS.syncfs;FS.syncfs=function(populate,done){return sync.call(FS,populate,function(error){if(populate&&!error)FS.writeFile("/persist/SaveGames",Uint8Array.from(atob('+JSON.stringify(save.toString('base64'))+'),c=>c.charCodeAt(0)));done(error);});};});</script>';
   return route.fulfill({status:200,contentType:'text/html',body:html.replace(marker,hook+marker)});
  });
  const state=()=>page.evaluate(()=>({level:HEAP32[0x936ff4>>2],ticks:HEAP32[0x7746c0>>2],cf:HEAP32[0x462ff0>>2],quit:HEAP32[0x7746ac>>2],
   mode:HEAP32[0x4673f8>>2],type:HEAP32[0x4673f4>>2],poly:HEAPU32[0x940010>>2],health:new DataView(HEAPU8.buffer).getInt32(0x792a76,true),
   dead:new DataView(HEAPU8.buffer).getInt32(0x792ac6,true),finished:HEAP32[0x795df4>>2],retire:HEAP32[0x9376a8>>2],position:[HEAP32[0x78a744>>2],HEAP32[0x78a74c>>2]]}));
  const tap=async code=>{inputs.push({code,down:true,before:await state()});await key(page,code,900);inputs.push({code,down:false,after:await state()});};
  await boot(page,server);
  await tap('Enter');await tap('ArrowRight');await tap('ArrowRight');await tap('Enter');
  if(total){
   const label=()=>page.evaluate(()=>{const a=HEAPU32[0x46a6f0>>2];let s='';for(let i=0;i<80&&HEAPU8[a+i];i++)s+=String.fromCharCode(HEAPU8[a+i]);return s;});
   for(let i=0;i<4&&!/Total/i.test(await label());i++)await tap('ArrowRight');
   assert(/Total/i.test(await label()),'Total Destruction label missing');
  }
  await tap('Enter');
  const selected=await state();assert.equal(selected.mode,2,'Destruction Derby mode not selected');assert.equal(selected.type,total?2:0);
  await tap('ArrowDown');await tap('ArrowDown');await tap('Enter');
  const started=await waitRace(page);assert(started.launched,'Arena race did not start');assert(started.lvl>=8);
  await page.screenshot({path:path.join(out,'race-start.png')});
  await page.evaluate(()=>{window.dispatchEvent(new KeyboardEvent('keydown',{code:'ArrowUp'}));window.dispatchEvent(new KeyboardEvent('keydown',{code:'ArrowRight'}));});
  inputs.push({code:'ArrowUp+ArrowRight',down:true,after:await state()});
  const deadline=Date.now()+180000;
  while(Date.now()<deadline){
   const current=await state();observations.push(current);
   fs.writeFileSync(path.join(out,'progress.json'),JSON.stringify({current,observations:observations.length,errors},null,2));
   if(current.quit||current.level===15||current.level===0)break;
   assert.deepEqual(errors,[]);
   await page.waitForTimeout(1000);
  }
  await page.evaluate(()=>{window.dispatchEvent(new KeyboardEvent('keyup',{code:'ArrowUp'}));window.dispatchEvent(new KeyboardEvent('keyup',{code:'ArrowRight'}));});
  inputs.push({code:'ArrowUp+ArrowRight',down:false,after:await state()});
  await page.waitForTimeout(2000);const ended=await state();
  await page.screenshot({path:path.join(out,'race-end.png')});
  const natural=observations.some(row=>row.quit===1&&row.finished>14&&row.retire===0);
  fs.writeFileSync(path.join(out,'diagnosis.json'),JSON.stringify({scope:'Actual live production browser normal arena player inputs and natural finish diagnosis; no original A/V parity claim',
   wasm_sha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(build,'index.wasm'))).digest('hex'),initial_save_sha256:crypto.createHash('sha256').update(save).digest('hex'),
   natural_finish:natural,started,ended,inputs,observations,errors},null,2));
  console.log(JSON.stringify({natural_finish:natural,ended,errors}));
 }finally{if(browser)await browser.close();await new Promise(r=>server.close(r));}
})().catch(e=>{fs.writeFileSync(path.join(out,'failure.json'),JSON.stringify({error:e.message},null,2));console.error(e);process.exitCode=1;});
