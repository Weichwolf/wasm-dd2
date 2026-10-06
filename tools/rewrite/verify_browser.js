'use strict';
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const {chromium} = require('../browser/playwright');
const [url, archive, output] = process.argv.slice(2);
if (!output || !path.resolve(output).startsWith('/tmp/wasm-dd2/')) throw new Error('Use /tmp/wasm-dd2/');
const report = {pass_:false, scope:'Actual rewrite browser canvas/input, not original parity', comparisons:[]};
const errors = [];
const pause = ms => new Promise(resolve => setTimeout(resolve, ms));
const digest = data => crypto.createHash('sha256').update(data).digest('hex');
function reference(code, mode) {
  const raw = fs.readFileSync(path.join(output, `${code}-${mode}.ppm`));
  const header = Buffer.from('P6\n640 480\n255\n');
  if (!raw.subarray(0,header.length).equals(header)) throw new Error('Invalid reference');
  return raw.subarray(header.length);
}
async function pixels(page) {
  const encoded = await page.evaluate(() => {
    const rgba = document.querySelector('#canvas').getContext('2d').getImageData(0,0,640,480).data;
    const rgb = new Uint8Array(640*480*3);
    for (let source=0,target=0;source<rgba.length;source+=4) {
      rgb[target++]=rgba[source];rgb[target++]=rgba[source+1];rgb[target++]=rgba[source+2];
    }
    let binary='';
    for(let i=0;i<rgb.length;i+=32768) binary+=String.fromCharCode(...rgb.subarray(i,i+32768));
    return btoa(binary);
  });
  return Buffer.from(encoded,'base64');
}
function compare(expected,actual) {
  if(actual.length!==expected.length) throw new Error('Canvas size differs');
  let changed=0,total=0;
  for(let i=0;i<actual.length;i+=3) {
    let different=false;
    for(let channel=0;channel<3;channel++) {
      const error=Math.abs(actual[i+channel]-expected[i+channel]);
      total+=error;different||=error!==0;
    }
    if(different)changed++;
  }
  return {changed_pixels:changed,mean_channel_error:total/actual.length,
          pass_:changed<=640*480*.01 && total/actual.length<=.5,
          expected_sha256:digest(expected),actual_sha256:digest(actual)};
}
async function match(page,code,mode) {
  const expected=reference(code,mode), deadline=Date.now()+15000;
  let result;
  while(Date.now()<deadline) {
    result=compare(expected,await pixels(page));
    if(result.pass_) {report.comparisons.push({code,mode,...result});return;}
    await pause(60);
  }
  await page.locator('#canvas').screenshot({path:path.join(output,'failed-canvas.png')});
  throw new Error(`Canvas ${code}/${mode} differs: ${JSON.stringify(result)}`);
}
async function stable(page) {
  let previous,since=Date.now();
  const deadline=Date.now()+10000;
  while(Date.now()<deadline) {
    const current=digest(await pixels(page));
    if(current!==previous)since=Date.now();previous=current;
    if(Date.now()-since>=1000)return current;
    await pause(60);
  }
  throw new Error('Camera keeps moving after input release');
}
async function changed(page,baseline) {
  const deadline=Date.now()+10000;
  while(Date.now()<deadline) {
    if(digest(await pixels(page))!==baseline)return;
    await pause(50);
  }
  throw new Error('Actual browser keyboard did not move camera');
}
async function drivingChecks(page) {
  await page.evaluate(()=>Module._dd2_application_set_paused(1));
  await page.selectOption('#view','2');
  let index=1;
  for(const code of '123456789AB') {
    await page.selectOption('#level',String(index++));
    await match(page,code,'driving');
  }
  await page.keyboard.press('PageUp');await match(page,'1','driving');
  await page.waitForFunction(()=>document.querySelector('#view').value==='2' && document.querySelector('#level').value==='1');
  const baseline=digest(await pixels(page));
  await page.keyboard.down('w');
  try {if(await stable(page)!==baseline)throw new Error('Paused browser vehicle moves');}
  finally {await page.keyboard.up('w');}
  await page.locator('#pause').click();
  await page.keyboard.down('w');
  try {await changed(page,baseline);await pause(1000);} finally {await page.keyboard.up('w');}
  await page.keyboard.press('p');
  if(await stable(page)===baseline)throw new Error('Browser throttle did not move vehicle');
  await page.keyboard.press('r');await match(page,'1','driving');
  await page.keyboard.press('p');
  await page.keyboard.down('w');await changed(page,baseline);
  await page.locator('#reset').focus();await page.keyboard.up('w');
  await stable(page);
  await page.evaluate(()=>Module._dd2_application_set_paused(1));
  await page.locator('#reset').click();await match(page,'1','driving');
  const bad=await page.evaluate(()=>({drive:Module._dd2_application_set_driving(2),pause:Module._dd2_application_set_paused(-1),view:Module._dd2_application_current_view(),paused:Module._dd2_application_is_paused()}));
  if(bad.drive!==0||bad.pause!==0||bad.view!==2||bad.paused!==1)throw new Error('Invalid driving control changes state');
  await page.locator('#canvas').screenshot({path:path.join(output,'browser-driving.png')});
  if(await page.evaluate(()=>Module._dd2_application_vehicle_count())!==20)throw new Error('Starter field is incomplete');
  if(await page.evaluate(()=>Module._dd2_application_pair_collision_count())!==0)throw new Error('Reset retains old pair contacts');
  await page.locator('#pause').click();
  await page.keyboard.down('s');
  try {
    await page.waitForFunction(()=>Module._dd2_application_pair_collision_count()>0 &&
      Array.from({length:6},(_,i)=>Module._dd2_application_region_damage(i)).some(value=>value>0),null,{timeout:15000});
  } finally {await page.keyboard.up('s');}
  await page.keyboard.press('p');await stable(page);
  report.car_pair_collision={real_reverse_key:true,vehicles:20,contacts:await page.evaluate(()=>Module._dd2_application_pair_collision_count())};
  const damage=await page.evaluate(()=>({health:Module._dd2_application_engine_health(),
    regions:Array.from({length:6},(_,i)=>Module._dd2_application_region_damage(i)),
    points:Module._dd2_application_accident_points(),destructions:Module._dd2_application_destructions(),
    windows:Module._dd2_application_accident_windows()}));
  if(![damage.health,...damage.regions].every(value=>Number.isFinite(value)&&value>=0&&value<=1))throw new Error('Invalid vehicle damage');
  if(!Number.isInteger(damage.points)||damage.points<0||damage.points>999||
    !Number.isInteger(damage.destructions)||damage.destructions<0||damage.destructions>19||damage.windows<1)throw new Error('Impact attribution/score invalid');
  await pause(300);
  const frozen=await page.evaluate(()=>({health:Module._dd2_application_engine_health(),
    regions:Array.from({length:6},(_,i)=>Module._dd2_application_region_damage(i)),
    points:Module._dd2_application_accident_points(),destructions:Module._dd2_application_destructions(),
    windows:Module._dd2_application_accident_windows()}));
  if(JSON.stringify(frozen)!==JSON.stringify(damage))throw new Error('Paused damage changes');
  report.damage={...damage,real_reverse_key:true,pause_freezes:true};
  await page.locator('#canvas').screenshot({path:path.join(output,'browser-car-pair.png')});
  await page.locator('#reset').click();await match(page,'1','driving');
  if(await page.evaluate(()=>Module._dd2_application_pair_collision_count())!==0)throw new Error('Pair contacts survive reset');
  if(await page.evaluate(()=>Module._dd2_application_engine_health()!==1 ||
    Array.from({length:6},(_,i)=>Module._dd2_application_region_damage(i)).some(value=>value!==0)))throw new Error('Damage survives reset');
  report.damage.reset_clears=true;
  if(await page.evaluate(()=>Module._dd2_application_accident_points()!==0 ||
    Module._dd2_application_destructions()!==0 || Module._dd2_application_accident_windows()!==0))throw new Error('Accident scores survive reset');
  report.accidents={points:damage.points,destructions:damage.destructions,active_windows:damage.windows,
    real_reverse_key:true,pause_freezes:true,reset_clears:true};
  await page.selectOption('#level','8');
  await page.locator('#reset').click();
  await page.locator('#pause').click();
  await page.keyboard.down('s');
  try {
    await page.waitForFunction(()=>Module._dd2_application_collision_count()>0,null,{timeout:15000});
  } finally {await page.keyboard.up('s');}
  await page.keyboard.press('p');await stable(page);
  report.barrier_collision={real_reverse_key:true,contacts:await page.evaluate(()=>Module._dd2_application_collision_count())};
  await page.locator('#reset').click();await match(page,'8','driving');
  if(await page.evaluate(()=>Module._dd2_application_collision_count())!==0)throw new Error('Reset retains old collision events');
  await page.selectOption('#level','1');await match(page,'1','driving');
  await page.keyboard.press('Enter');await match(page,'1','scene');
  report.driving={pass_:true,real_throttle:true,pause_freezes:true,focus_loss_freezes:true,
                  deterministic_reset:true,track_wrap:true,selection_sync:true,invalid_controls_rejected:true};
}
async function main() {
  const browser=await chromium.launch({headless:true});
  const page=await browser.newPage({viewport:{width:680,height:1000}});
  page.on('pageerror',error=>errors.push(String(error)));
  page.on('console',message=>{if(message.type()==='error')errors.push(message.text());});
  try {
    await page.goto(url);
    await page.waitForFunction(()=>!document.querySelector('#archive').disabled,null,{timeout:60000});
    report.cross_origin_isolated=await page.evaluate(()=>crossOriginIsolated);
    if(!report.cross_origin_isolated)throw new Error('Browser workers are not isolated');
    await page.setInputFiles('#archive',{name:'Dirinfo',mimeType:'application/octet-stream',buffer:Buffer.alloc(1024)});
    await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('konnte nicht geladen'),null,{timeout:15000});
    if(await page.evaluate(()=>Module._dd2_application_current_level())!==0)throw new Error('Invalid archive accepted');
    await page.setInputFiles('#archive',archive);
    await page.waitForFunction(()=>Module._dd2_application_current_level()===1,null,{timeout:60000});
    report.invalid_file_recovery=true;
    await page.evaluate(()=>{window.observedKeys=[];document.querySelector('#canvas').addEventListener('keydown',e=>window.observedKeys.push({key:e.key,trusted:e.isTrusted}));});
    let index=1;
    for(const code of '123456789AB') {
      await page.selectOption('#level',String(index++));await match(page,code,'scene');
      await page.selectOption('#view','1');await match(page,code,'car');
      await page.selectOption('#view','0');await match(page,code,'scene');
    }
    await page.keyboard.press('PageUp');await match(page,'1','scene');
    await page.waitForFunction(()=>document.querySelector('#level').value==='1');
    await page.keyboard.press('Tab');await match(page,'1','car');
    await page.waitForFunction(()=>document.querySelector('#view').value==='1');
    await page.keyboard.press('Tab');await match(page,'1','scene');
    report.keyboard_selection_sync=true;
    const baseline=digest(await pixels(page));
    for(const key of ['ArrowRight','ArrowUp','a','w','+','-']) {
      await page.keyboard.down(key);
      try {await changed(page,baseline);} finally {await page.keyboard.up(key);}
      if(await stable(page)===baseline)throw new Error('Camera did not move: '+key);
      await page.keyboard.press('r');await match(page,'1','scene');
    }
    await page.mouse.move(330,500);await page.mouse.wheel(0,-120);await changed(page,baseline);
    await page.locator('#reset').click();await match(page,'1','scene');
    await page.keyboard.down('ArrowRight');await changed(page,baseline);
    await page.locator('#reset').focus();await page.keyboard.up('ArrowRight');
    const blurred=await stable(page);await page.locator('#canvas').focus();
    if(await stable(page)!==blurred)throw new Error('Camera continues after canvas focus loss');
    await page.locator('#reset').click();await match(page,'1','scene');
    report.camera_motion_release=true;report.canvas_focus_release=true;report.wheel=true;
    const state=await page.evaluate(()=>({badLevel:Module._dd2_application_select_level(0),badView:Module._dd2_application_show_car(2),level:Module._dd2_application_current_level(),view:Module._dd2_application_current_view()}));
    if(state.badLevel!==0||state.badView!==0||state.level!==1||state.view!==0)throw new Error('Invalid selection changed active state');
    report.transactional_invalid_selection=true;
    await drivingChecks(page);
    await page.locator('#canvas').screenshot({path:path.join(output,'browser-scene.png')});
    await page.keyboard.press('Escape');await page.waitForFunction(()=>Module._dd2_application_current_level()===0);
    await page.waitForFunction(()=>document.querySelector('#level').disabled);
    await page.setInputFiles('#archive',archive);
    await page.waitForFunction(()=>Module._dd2_application_current_level()===1);await match(page,'1','scene');
    report.shutdown_restart=true;
    report.keyboard_events=await page.evaluate(()=>window.observedKeys);
    if(!report.keyboard_events.length||report.keyboard_events.some(event=>!event.trusted))throw new Error('Actual trusted browser keys required');
    if(errors.length)throw new Error('Browser errors: '+errors.join('\n'));
    report.browser_errors=[];report.pass_=true;
  } finally {
    report.errors=errors;
    fs.writeFileSync(path.join(output,'browser.json'),JSON.stringify(report,null,2)+'\n');
    await browser.close();
  }
}
main().catch(error=>{console.error(error);process.exitCode=1;});
