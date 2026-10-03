// Real Chromium keyboard input through the generated shell and actual engine
// startup/movie/window procedure. No engine writes or diagnostic entry flags.
const fs=require('fs'),path=require('path'),assert=require('assert'),crypto=require('crypto');
const {serve,chromium,menuLabel}=require('./felib');
const build=path.resolve(process.argv[2]||'web/dd2'),output=path.resolve(process.argv[3]);
fs.mkdirSync(output,{recursive:false});
const keys=[['ShiftLeft',0xa0],['ShiftRight',0xa1],['ControlLeft',0xa2],['ControlRight',0xa3],['AltLeft',0xa4],['AltRight',0xa5],['F10',0x79],['F3',0x72],['F12',0x7b],['F24',0x87],['Home',0x24],['Tab',9],['Delete',0x2e],['Backspace',8]];
const transitions=[...fs.readFileSync(path.join(__dirname,'../keyboard_events.h'),'utf8').matchAll(/\{(0x[0-9a-f]+),(0|1),"([^"]+)"\}/g)].map(m=>({vk:Number(m[1]),down:Number(m[2]),code:m[3]}));
assert(transitions.length===104);
(async()=>{
 const server=serve(build);await new Promise(r=>server.listen(0,r));
 const report={scope:'Real Chromium side-specific modifier/function/editing/navigation keyup retains normal intro; keydown closes movie context and reaches original main menu. Shell forwards all 104 declared USER32-fixture transitions. Message/state equivalence is established separately by verify_keyboard; original full output clocks and physical keyboard layout remain open.',wasm_sha256:crypto.createHash('sha256').update(fs.readFileSync(path.join(build,'index.wasm'))).digest('hex'),cases:[]};
 const browser=await chromium.launch({args:['--no-sandbox']});
 try{
  for(const [code,vk] of keys){
   const context=await browser.newContext(),page=await context.newPage(),errors=[];
   const cdp=await context.newCDPSession(page);
   const transition=async(code,down)=>{
    // Debian Playwright1.38 knows F1-F12 only. The browser protocol supplies
    // trusted DOM key input for the other named function keys; modifiers are
    // released before these fixture steps. No synthetic DOM events or C calls.
    if(/^F(1[3-9]|2[0-4])$/.test(code))await cdp.send('Input.dispatchKeyEvent',{
     type:down?'keyDown':'keyUp',code,key:code,windowsVirtualKeyCode:111+Number(code.slice(1)),nativeVirtualKeyCode:111+Number(code.slice(1))});
    else if(down)await page.keyboard.down(code);else await page.keyboard.up(code);
   };
   try{
    page.on('pageerror',e=>errors.push(e.message));page.on('crash',()=>errors.push('renderer crash'));
    await page.goto(`http://localhost:${server.address().port}/index.html`);
    await page.waitForFunction(()=>typeof HEAP32!=='undefined' && HEAP32[0x462cd4>>2]===1 && !!Module._dd2movieSource,null,{timeout:30000});
    await page.click('#canvas');await page.waitForFunction(()=>Module._dd2movieAc.state==='running');
    await page.evaluate(()=>{
     window.__keyboard=[];window.__trustedKeys=[];window.__introContext=Module._dd2movieAc;
     for(const name of ['keydown','keyup'])window.addEventListener(name,e=>__trustedKeys.push({code:e.code,down:e.type==='keydown'?1:0,trusted:e.isTrusted}));
     const ccall=Module.ccall;
     Module.ccall=function(name,type,types,args){
      const result=ccall.apply(this,arguments);
      if(name==='dd2_browser_key_event')__keyboard.push({code:args[0],down:args[1],vk:result});
      return result;
     };
    });
    await transition(code,0);await page.waitForTimeout(250);
    assert(await page.evaluate(()=>HEAP32[0x462cd4>>2])===1,`${code}: release cancelled intro`);
    await transition(code,1);
    await page.waitForFunction(()=>HEAP32[0x462cd4>>2]===0 && !Module._dd2movieSource && __introContext.state==='closed',null,{timeout:10000});
    await transition(code,0);
    await page.waitForFunction(()=>HEAP32[0x462d68>>2]===1 && HEAP32[0x936ff4>>2]===0 && HEAPU8[0x460005]===201,null,{timeout:20000});
    await page.waitForTimeout(500);assert((await menuLabel(page)).includes('Wrecking'),`${code}: real main menu not reached`);
    assert.deepStrictEqual(await page.evaluate(()=>__keyboard),[{code,down:0,vk},{code,down:1,vk},{code,down:0,vk}]);
    if(report.cases.length===0){
     await page.evaluate(()=>{__keyboard.length=0;__trustedKeys.length=0;});
     for(const e of transitions)await transition(e.code,e.down);
     const observed=await page.evaluate(()=>__keyboard);
     fs.writeFileSync(path.join(output,'shell-transitions.json'),JSON.stringify(observed,null,2)+'\n');
     assert.deepStrictEqual(observed,transitions,'browser shell lost/misrouted declared key transitions');
     assert.deepStrictEqual(await page.evaluate(()=>__trustedKeys),transitions.map(e=>({code:e.code,down:e.down,trusted:true})));
     report.shell_transitions=transitions.length;report.all_transitions_trusted=true;
    }
    assert.deepStrictEqual(errors,[]);
    const result={code,vk,release_retained:true,press_cancelled:true,movie_context_closed:true,original_main_menu:true};
    report.cases.push(result);fs.writeFileSync(path.join(output,'report.json'),JSON.stringify(report,null,2)+'\n');
    console.log(`PASS real browser ${code}: release retains intro, press closes movie and reaches original main menu`);
   }finally{await context.close();}
  }
 }finally{await browser.close();await new Promise(r=>server.close(r));}
})().catch(e=>{console.error(e);process.exitCode=1;});
