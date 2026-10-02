// Synthetic Gamepad API -> shell poll -> WinMM -> live player physics.
// This excludes physical HID hardware; all engine state is read-only.
const assert=require('assert'),path=require('path');
const {serve,chromium}=require('./felib');
const build=path.resolve(process.argv[2]||'web/dd2');
const state=page=>page.evaluate(()=>{
 const player=HEAP32[0x93ded0>>2],offset=player*0x1b2;
 const view=new DataView(HEAPU8.buffer);
 // Original 442dc0..442de7: world X/Z are struct+0x30/+0x38.
 return {player,level:HEAP32[0x936ff4>>2],cf:HEAP32[0x462ff0>>2],countdown:HEAP32[0x784298>>2],
  pad:HEAPU16[0x754448>>1],analog:HEAPU8[0x754450],mode:HEAPU8[0x46303e],
  joystick:HEAP32[0x463024>>2],x:view.getInt32(0x792a30+offset,true),z:view.getInt32(0x792a38+offset,true),
  steering:view.getInt32(0x792a82+offset,true),throttle:view.getInt32(0x792a86+offset,true)};
});
(async()=>{
 const server=serve(build);await new Promise(resolve=>server.listen(0,resolve));
 let browser;
 try{
  browser=await chromium.launch({args:['--no-sandbox']});
  const page=await browser.newPage();const errors=[];
  page.on('pageerror',error=>errors.push(error.message));page.on('crash',()=>errors.push('renderer crash'));
  await page.addInitScript(()=>{
   window.__pad={id:'Synthetic Xbox',index:0,connected:true,mapping:'standard',
    axes:[0,0,0,0],buttons:Array.from({length:16},()=>({pressed:false,value:0}))};
   navigator.getGamepads=()=>[window.__pad];
  });
  await page.goto(`http://localhost:${server.address().port}/index.html?race=9&pad`,{waitUntil:'load'});
  await page.waitForFunction(()=>typeof HEAP32!=='undefined' && HEAP32[0x936ff4>>2]===9 && HEAP32[0x784298>>2]<0,null,{timeout:30000});
  const before=await state(page);console.log('Before pad:',JSON.stringify(before));
  assert(before.mode===1 && before.joystick===0,'boot-time controller detection failed');
  await page.evaluate(()=>{__pad.buttons[0]={pressed:true,value:1};__pad.axes[0]=-1;});
  await page.waitForFunction(()=>HEAPU16[0x754448>>1]===0x4080 && HEAPU8[0x754450]===0,null,{timeout:5000});
  await page.waitForFunction(cf=>HEAP32[0x462ff0>>2]>=cf+75,before.cf,{timeout:10000});
  const driven=await state(page);console.log('Driven pad:',JSON.stringify(driven));
  assert(driven.throttle===32768,'accelerator did not reach live car control');
  assert(driven.steering===-512,'full-left analogue steering did not reach car control');
  assert(driven.x!==before.x || driven.z!==before.z,'player did not move under gamepad control');
  await page.evaluate(()=>{__pad.buttons[0]={pressed:false,value:0};__pad.buttons[1]={pressed:true,value:1};__pad.axes[0]=1;});
  await page.waitForFunction(()=>HEAPU16[0x754448>>1]===0x8020 && HEAPU8[0x754450]===255,null,{timeout:5000});
  await page.waitForFunction(()=>new DataView(HEAPU8.buffer).getInt32(0x792a86+HEAP32[0x93ded0>>2]*0x1b2,true)===-32768,null,{timeout:5000});
  const braking=await state(page);console.log('Brake/right pad:',JSON.stringify(braking));
  assert(braking.steering===496,'full-right analogue steering did not reach car control');
  await page.evaluate(()=>{__pad.buttons[1]={pressed:false,value:0};__pad.axes[0]=0;});
  await page.waitForFunction(()=>HEAPU16[0x754448>>1]===0 && HEAPU8[0x754450]===127,null,{timeout:5000});
  await page.waitForFunction(()=>new DataView(HEAPU8.buffer).getInt32(0x792a86+HEAP32[0x93ded0>>2]*0x1b2,true)===0,null,{timeout:5000});
  assert.deepEqual(errors,[],'browser runtime errors');
  console.log('PASS boot-time gamepad detection, both full-range steering endpoints, accelerate/movement, brake and release');
 }finally{if(browser)await browser.close();await new Promise(resolve=>server.close(resolve));}
})().catch(error=>{console.error(error);process.exitCode=1;});
