// Run headed on an isolated Xvfb display; all focus/key observations are trusted DOM events.
const fs=require('fs'),path=require('path'),assert=require('assert'),crypto=require('crypto');
// Playwright enables focus emulation on its OWN protocol session. Disable
// that request before page creation; a separate client cannot clear its mode.
const {createRequire}=require('module');
const requireCore=createRequire(require.resolve('playwright'));
const core=path.dirname(requireCore.resolve('playwright-core'));
const {CRSession}=require(path.join(core,'lib/server/chromium/crConnection.js'));
const sendProtocol=CRSession.prototype.send;let disabledFocusRequests=0;
CRSession.prototype.send=function(method,params){
 if(method==='Emulation.setFocusEmulationEnabled'){params={...params,enabled:false};disabledFocusRequests++;}
 return sendProtocol.call(this,method,params);
};
const {serve,chromium}=require('./felib');
const web=path.resolve(process.argv[2]),out=path.resolve(process.argv[3]);
assert(out.startsWith('/tmp/wasm-dd2/'),'Use /tmp/wasm-dd2/');
fs.mkdirSync(out,{recursive:false});
const report={scope:'Actual WASM application with the provided shell, trusted Chromium key and tab focus transitions. Read-only live engine observations, no PCM/render/whole-game acceptance.',pass_:false,target:'browser',operation:'window-focus-capture',engine_state_writes:false,original_port_full_parity:'unproven',sources:{'tools/browser/capture_window_focus.js':crypto.createHash('sha256').update(fs.readFileSync(__filename)).digest('hex')},samples:[],html_sha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(web,'index.html'))).digest('hex'),js_sha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(web,'index.js'))).digest('hex'),wasm_sha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(web,'index.wasm'))).digest('hex')};
(async()=>{
 const server=serve(web);await new Promise(r=>server.listen(0,r));
 const browser=await chromium.launch({headless:false,args:['--no-sandbox']});
 try{
  const context=await browser.newContext(),page=await context.newPage(),errors=[];
  page.on('pageerror',e=>errors.push(e.message));
  await (await context.newCDPSession(page)).send('Emulation.setFocusEmulationEnabled',{enabled:false});
  await page.goto(`http://localhost:${server.address().port}/index.html`);
  await page.waitForFunction(()=>typeof HEAP32!=='undefined' && HEAP32[0x462cd4>>2]===1,null,{timeout:30000});
  await page.click('#canvas');await page.keyboard.press('Escape');
  await page.waitForFunction(()=>HEAP32[0x462cd4>>2]===0 && HEAPU32[0x940010>>2]===0x4696b0 && HEAP32[0x467420>>2]===0,null,{timeout:30000});
  await page.evaluate(()=>{
   window.__dd2FocusEvents=[];
   for(const name of ['blur','focus','keydown','keyup'])window.addEventListener(name,e=>__dd2FocusEvents.push({type:e.type,code:e.code||null,trusted:e.isTrusted}));
   document.addEventListener('visibilitychange',e=>__dd2FocusEvents.push({type:e.type,visible:document.visibilityState,trusted:e.isTrusted}));
  });
  const sample=async label=>report.samples.push(await page.evaluate(label=>({label,active:HEAP32[0x46042c>>2],timer:HEAP32[0x460474>>2],timer_fires:HEAP32[0x460484>>2],phase:HEAP32[0x4699cc>>2],cf:HEAP32[0x462ff0>>2],flags:Array.from(HEAPU8.slice(0x46303f,0x463050)).map(b=>b.toString(16).padStart(2,'0')).join(''),visible:document.visibilityState,focused:document.hasFocus()}),label));
  await page.bringToFront();await sample('baseline');
  await page.keyboard.down('ArrowLeft');await page.waitForFunction(()=>HEAPU8[0x463045]===1);await sample('key-down-active');
  const sink=await context.newPage();await (await context.newCDPSession(sink)).send('Emulation.setFocusEmulationEnabled',{enabled:false});await sink.goto('about:blank');await sink.bringToFront();
  await page.waitForFunction(()=>!document.hasFocus());await sample('inactive-start');
  await new Promise(r=>setTimeout(r,1300));await sample('inactive-held');
  await sink.keyboard.up('ArrowLeft');await new Promise(r=>setTimeout(r,300));await sample('released-in-sink');
  await page.bringToFront();await page.waitForFunction(()=>document.hasFocus());await sample('reactivated-after-outside-release');
  report.events=await page.evaluate(()=>__dd2FocusEvents);
  assert(report.events.some(e=>e.type==='blur' && e.trusted));
  assert(report.events.some(e=>e.type==='keydown' && e.code==='ArrowLeft' && e.trusted));
  assert.deepStrictEqual(errors,[]);assert(disabledFocusRequests>0);report.playwright_focus_emulation_disabled_requests=disabledFocusRequests;report.pass_=true;
  await context.close();
 }finally{
  await browser.close();await new Promise(r=>server.close(r));fs.writeFileSync(path.join(out,'report.json'),JSON.stringify(report,null,2)+'\n');
 }
 console.log('Actual browser focus diagnosis completed');
})().catch(e=>{console.error(e);process.exitCode=1;});
