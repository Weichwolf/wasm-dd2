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
    const lap=await page.evaluate(()=>({current:Module._dd2_application_current_lap(),required:Module._dd2_application_required_laps(),
      completed:Module._dd2_application_completed_laps(),ticks:Module._dd2_application_lap_steps(),finished:Module._dd2_application_laps_finished()}));
    const expected=index<=8 ? [10,5,5,5,8,7,5][index-2] : 0;
    if(lap.current!==(expected ? 1 : 0)||lap.required!==expected||lap.completed!==0||lap.ticks!==0||lap.finished!==0)
      throw new Error('Initial lap state differs: '+JSON.stringify({code,lap,expected}));
  }
  await page.keyboard.press('PageUp');await match(page,'1','driving');
  await page.waitForFunction(()=>document.querySelector('#view').value==='2' && document.querySelector('#level').value==='1');
  const baseline=digest(await pixels(page));
  await page.keyboard.down('w');
  try {if(await stable(page)!==baseline)throw new Error('Paused browser vehicle moves');}
  finally {await page.keyboard.up('w');}
  await page.locator('#pause').click();
  await page.keyboard.down('w');
  try {
    await changed(page,baseline);
    await page.waitForFunction(()=>Module._dd2_application_lap_steps()>0,null,{timeout:15000});
  } finally {await page.keyboard.up('w');}
  await page.keyboard.press('p');
  const lapTicks=await page.evaluate(()=>Module._dd2_application_lap_steps());
  if(lapTicks<=0)throw new Error('Actual forward input never starts lap timing');
  await pause(300);
  if(await page.evaluate(()=>Module._dd2_application_lap_steps())!==lapTicks)throw new Error('Pause advances lap timing');
  report.laps={real_start_line_crossing:true,pause_freezes:true,ticks:lapTicks,initial_states_all_levels:true};
  if(await stable(page)===baseline)throw new Error('Browser throttle did not move vehicle');
  await page.keyboard.press('r');await match(page,'1','driving');
  if(await page.evaluate(()=>Module._dd2_application_lap_steps()!==0||Module._dd2_application_completed_laps()!==0||Module._dd2_application_laps_finished()!==0))
    throw new Error('Reset retains lap state');
  report.laps.reset_clears=true;
  await page.keyboard.press('p');
  await page.waitForFunction(()=>Module._dd2_application_is_paused()===0);
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
async function raceChecks(page) {
  await page.keyboard.press('F5');
  await page.waitForFunction(()=>Module._dd2_application_current_view()===3);
  await page.evaluate(()=>{Module._dd2_application_set_paused(1);Module._dd2_application_reset_camera();});
  let index=1;
  for(const code of '123456789AB') {
    await page.selectOption('#level',String(index++));
    await match(page,code,'race-start');
    const before=await page.evaluate(()=>({phase:Module._dd2_application_race_phase(),ticks:Module._dd2_application_race_steps(),
      lap:Module._dd2_application_lap_steps(),place:Module._dd2_application_race_place()}));
    if(before.phase!==0||before.ticks!==0||before.lap!==0||before.place<1||before.place>20)throw new Error('Initial race state invalid');
    await page.keyboard.down('w');
    try {await pause(120);if(await page.evaluate(()=>Module._dd2_application_race_steps())!==0)throw new Error('Paused countdown advances');}
    finally {await page.keyboard.up('w');}
    await page.locator('#finish').click();await match(page,code,'race-results');
    const points=await page.evaluate(()=>Module._dd2_application_race_points());
    await page.locator('#pause').click();
    await page.keyboard.down('w');
    try {
      const baseline=digest(await pixels(page));
      if(await stable(page)!==baseline || await page.evaluate(()=>Module._dd2_application_race_points())!==points)
        throw new Error('Published race results advance');
    } finally {await page.keyboard.up('w');}
    await page.locator('#pause').click();
    await page.locator('#reset').click();await match(page,code,'race-start');
  }
  const rejected=await page.evaluate(()=>({bad:Module._dd2_application_start_race(1),view:Module._dd2_application_current_view(),
    phase:Module._dd2_application_race_phase(),badMode:Module._dd2_application_start_race(2)}));
  if(rejected.bad!==0||rejected.badMode!==0||rejected.view!==3||rejected.phase!==0)throw new Error('Invalid race changed arena state');
  await page.selectOption('#level','1');
  await page.keyboard.press('F6');
  await page.waitForFunction(()=>Module._dd2_application_current_view()===4);
  await page.evaluate(()=>{Module._dd2_application_set_paused(1);Module._dd2_application_reset_camera();});
  await match(page,'1','race-start');
  await page.keyboard.press('F7');
  await page.waitForFunction(()=>Module._dd2_application_race_phase()===3);
  if(await page.evaluate(()=>Module._dd2_application_race_points())!==100)throw new Error('Stockcar source grid points differ');
  // Real held throttle through the countdown: clocks cannot start before GO.
  await page.keyboard.press('r');
  await page.keyboard.press('p');
  await page.waitForFunction(()=>Module._dd2_application_is_paused()===0);
  await page.keyboard.down('w');
  try {
    await page.waitForFunction(()=>Module._dd2_application_race_steps()>20);
    const during=await page.evaluate(()=>({phase:Module._dd2_application_race_phase(),lap:Module._dd2_application_lap_steps(),
      collisions:Module._dd2_application_collision_count()}));
    if(during.phase!==0||during.lap!==0||during.collisions!==0)throw new Error('Countdown permits physical progress');
    await page.waitForFunction(()=>Module._dd2_application_race_phase()===1,null,{timeout:15000});
    await page.waitForFunction(()=>Module._dd2_application_lap_steps()>0,null,{timeout:15000});
  } finally {await page.keyboard.up('w');}
  await page.keyboard.press('p');
  const ticks=await page.evaluate(()=>Module._dd2_application_race_steps());
  await pause(200);
  if(await page.evaluate(()=>Module._dd2_application_race_steps())!==ticks)throw new Error('Pause advances active race');
  await page.locator('#finish').click();
  await page.waitForFunction(()=>Module._dd2_application_race_phase()===3);
  await page.locator('#canvas').screenshot({path:path.join(output,'browser-race-results.png')});
  await page.selectOption('#view','2');
  if(await page.evaluate(()=>Module._dd2_application_race_phase())!==-1)throw new Error('Free driving retains race');
  await page.keyboard.press('Enter');await match(page,'1','scene');
  report.race={pass_:true,all_levels:true,native_keyboard_modes:true,source_stockcar_points:true,
    countdown_holds_field:true,real_throttle_after_go:true,pause:true,frozen_results:true,reset:true,invalid_mode_rollback:true};
}
async function trialChecks(page) {
  await page.keyboard.press('F8');
  await page.waitForFunction(()=>Module._dd2_application_current_view()===5);
  await page.evaluate(()=>{Module._dd2_application_set_paused(1);Module._dd2_application_reset_camera();});
  for(const code of '1234567') {
    await page.selectOption('#level',code);
    await match(page,code,'trial-start');
    const grid=await page.evaluate(()=>({count:Module._dd2_application_vehicle_count(),
      required:Module._dd2_application_required_laps(),current:Module._dd2_application_current_lap(),
      best:Module._dd2_application_best_lap_steps(),last:Module._dd2_application_last_lap_steps()}));
    if(JSON.stringify(grid)!==JSON.stringify({count:1,required:0,current:1,best:0,last:0}))throw new Error('Time Trial grid/rules invalid');
    await page.locator('#finish').click();await match(page,code,'trial-results');
    if(await page.evaluate(()=>Module._dd2_application_race_points())!==0)throw new Error('Time Trial awards circuit bonus');
    await page.locator('#pause').click();
    const baseline=digest(await pixels(page));
    if(await stable(page)!==baseline)throw new Error('Time Trial result changes');
    await page.locator('#pause').click();
    await page.locator('#reset').click();await match(page,code,'trial-start');
  }
  await page.selectOption('#level','8');await match(page,'8','race-start');
  const rejected=await page.evaluate(()=>({bad:Module._dd2_application_start_race(2),count:Module._dd2_application_vehicle_count(),view:Module._dd2_application_current_view()}));
  if(rejected.bad!==0||rejected.count!==20||rejected.view!==3)throw new Error('Arena Time Trial rejection changed state');
  await page.selectOption('#level','1');
  await page.selectOption('#view','5');
  await page.evaluate(()=>{Module._dd2_application_set_paused(1);Module._dd2_application_reset_camera();});
  await match(page,'1','trial-start');
  await page.keyboard.press('p');
  await page.waitForFunction(()=>Module._dd2_application_is_paused()===0);
  await page.keyboard.down('w');
  try {
    await page.waitForFunction(()=>Module._dd2_application_lap_steps()>0,null,{timeout:15000});
    if(await page.evaluate(()=>Module._dd2_application_vehicle_count())!==1)throw new Error('Time Trial resurrects opponents');
  } catch(error) {
    report.time_trial_failure=await page.evaluate(()=>({paused:Module._dd2_application_is_paused(),
      phase:Module._dd2_application_race_phase(),ticks:Module._dd2_application_race_steps(),
      count:Module._dd2_application_vehicle_count(),lap:Module._dd2_application_current_lap(),
      time:Module._dd2_application_lap_steps(),health:Module._dd2_application_engine_health(),
      collisions:Module._dd2_application_collision_count()}));
    await page.locator('#canvas').screenshot({path:path.join(output,'failed-time-trial-drive.png')});
    throw error;
  } finally {await page.keyboard.up('w');}
  await page.keyboard.press('p');
  const clock=await page.evaluate(()=>Module._dd2_application_lap_steps());
  await pause(200);
  if(await page.evaluate(()=>Module._dd2_application_lap_steps())!==clock)throw new Error('Time Trial pause advances clock');
  await page.locator('#finish').click();
  if(await page.evaluate(()=>Module._dd2_application_lap_steps())!==clock)throw new Error('Time Trial withdrawal loses current time');
  await page.locator('#canvas').screenshot({path:path.join(output,'browser-trial-results.png')});
  await page.selectOption('#view','4');
  if(await page.evaluate(()=>Module._dd2_application_vehicle_count()!==20||Module._dd2_application_required_laps()!==10))throw new Error('Stockcar loses field/rules after Time Trial');
  await page.selectOption('#view','5');await page.selectOption('#view','2');
  if(await page.evaluate(()=>Module._dd2_application_vehicle_count()!==20||Module._dd2_application_required_laps()!==10))throw new Error('Free driving loses field/rules after Time Trial');
  await page.keyboard.press('Enter');await match(page,'1','scene');
  report.time_trial={pass_:true,all_circuits:true,one_car:true,continuous_laps:true,real_keyboard_throttle:true,
                    clocks_pause:true,results:true,reset:true,arena_rejected:true,finite_rules_restored:true};
}
async function totalChecks(page) {
  await page.selectOption('#level','8');
  await page.keyboard.press('F9');
  await page.waitForFunction(()=>Module._dd2_application_current_view()===6);
  await page.evaluate(()=>{Module._dd2_application_set_paused(1);Module._dd2_application_reset_camera();});
  for(const [index,code] of [...'89AB'].entries()) {
    await page.selectOption('#level',String(index+8));
    await match(page,code,'total-start');
    const grid=await page.evaluate(()=>({count:Module._dd2_application_vehicle_count(),alive:Module._dd2_application_race_alive(),
      ticks:Module._dd2_application_survival_steps(),phase:Module._dd2_application_race_phase(),view:Module._dd2_application_current_view()}));
    if(JSON.stringify(grid)!==JSON.stringify({count:20,alive:20,ticks:0,phase:0,view:6}))throw new Error('Total Destruction grid invalid');
    await pause(150);
    if(await page.evaluate(()=>Module._dd2_application_survival_steps())!==0)throw new Error('Paused survival time advances');
    await page.locator('#finish').click();await match(page,code,'total-results');
    if(await page.evaluate(()=>Module._dd2_application_race_points())!==0)throw new Error('Total Destruction awards placement points');
    await page.locator('#pause').click();
    const baseline=digest(await pixels(page));
    if(await stable(page)!==baseline)throw new Error('Total Destruction results advance');
    await page.locator('#pause').click();
    await page.locator('#reset').click();await match(page,code,'total-start');
  }
  await page.selectOption('#level','1');await match(page,'1','race-start');
  const rejected=await page.evaluate(()=>({bad:Module._dd2_application_start_race(3),view:Module._dd2_application_current_view(),
    disabled:document.querySelector('#view option[value="6"]').disabled}));
  if(rejected.bad!==0||rejected.view!==3||!rejected.disabled)throw new Error('Circuit Total Destruction rejection invalid');
  await page.selectOption('#level','8');await page.selectOption('#view','6');
  await page.evaluate(()=>{Module._dd2_application_set_paused(1);Module._dd2_application_reset_camera();});
  await match(page,'8','total-start');
  await page.keyboard.press('p');
  await page.waitForFunction(()=>Module._dd2_application_is_paused()===0);
  await page.waitForFunction(()=>Module._dd2_application_race_steps()>20);
  if(await page.evaluate(()=>Module._dd2_application_survival_steps())!==0)throw new Error('Survival time runs during countdown');
  await page.waitForFunction(()=>Module._dd2_application_survival_steps()>40,null,{timeout:15000});
  await page.keyboard.press('p');
  await page.waitForFunction(()=>Module._dd2_application_is_paused()===1);
  const ticks=await page.evaluate(()=>Module._dd2_application_survival_steps());
  await pause(200);
  if(await page.evaluate(()=>Module._dd2_application_survival_steps())!==ticks)throw new Error('Pause advances survival clock');
  await page.keyboard.press('F7');
  await page.waitForFunction(()=>Module._dd2_application_race_phase()===3);
  if(await page.evaluate(()=>Module._dd2_application_survival_steps())!==ticks)throw new Error('Withdrawal loses survival time');
  await page.locator('#canvas').screenshot({path:path.join(output,'browser-total-results.png')});
  await page.selectOption('#view','2');
  await page.selectOption('#level','1');
  await page.keyboard.press('Enter');await match(page,'1','scene');
  report.total_destruction={pass_:true,all_arenas:true,twenty_cars:true,real_keyboard_mode:true,browser_selector:true,
    countdown_holds_clock:true,survival_clock_runs:true,pause:true,results:true,reset:true,circuit_rejected:true};
}

async function championshipChecks(page) {
  const cases=[];
  for (const [key,selected] of [['c','7'],['n','8']]) {
    await page.keyboard.press(key);
    await page.waitForFunction(()=>Module._dd2_application_championship_phase()===1);
    await page.keyboard.press('p');
    await page.waitForFunction(()=>Module._dd2_application_is_paused()===1);
    const snapshot=()=>page.evaluate(()=>({level:Module._dd2_application_current_level(),
      view:Module._dd2_application_current_view(),steps:Module._dd2_application_race_steps(),
      points:Module._dd2_application_championship_points(0),
      round:Module._dd2_application_championship_round(),
      division:Module._dd2_application_championship_division(),
      season:Module._dd2_application_championship_season(),
      count:Module._dd2_application_vehicle_count(),
      trackLocked:document.querySelector('#level').disabled,
      continueDisabled:document.querySelector('#continue').disabled}));
    await page.waitForFunction(()=>document.querySelector('#level').disabled);
    const initial=await snapshot();
    if(initial.level!==1||String(initial.view)!==selected||initial.points!==0||initial.round!==1||
      initial.division!==4||initial.season!==1||initial.count!==20||!initial.trackLocked||!initial.continueDisabled)
      throw new Error('Championship entry differs: '+JSON.stringify(initial));
    await page.keyboard.press('PageUp');
    await page.keyboard.down('w');
    try {await pause(200);if(JSON.stringify(await snapshot())!==JSON.stringify(initial))throw new Error('Paused scheduled championship advanced');}
    finally {await page.keyboard.up('w');}
    // Observe the real click after the production listener resets the owner.
    // Playwright can spend longer than the countdown waiting for the subsequent
    // Pause click; its eventual snapshot cannot certify the reset's initial tick.
    await page.evaluate(()=>{
      window.championshipRestartObservation=null;
      document.querySelector('#reset').addEventListener('click',event=>{
        window.championshipRestartObservation={trusted:event.isTrusted,
          phase:Module._dd2_application_championship_phase(),
          steps:Module._dd2_application_race_steps(),
          round:Module._dd2_application_championship_round(),
          points:Module._dd2_application_championship_points(0),
          count:Module._dd2_application_vehicle_count(),
          paused:Module._dd2_application_is_paused()};
      },{once:true});
    });
    await page.locator('#reset').click();
    const resetObserved=await page.evaluate(()=>window.championshipRestartObservation);
    if(!resetObserved||!resetObserved.trusted||resetObserved.phase!==1||resetObserved.steps!==0||
      resetObserved.round!==1||resetObserved.points!==0||resetObserved.count!==20||resetObserved.paused!==0)
      throw new Error('Trusted championship restart did not reset the actual field: '+JSON.stringify(resetObserved));
    await page.locator('#pause').click();
    await page.waitForFunction(()=>Module._dd2_application_is_paused()===1);
    const restarted=await snapshot();
    if(restarted.round!==1||restarted.points!==0)
      throw new Error('Unfinished restart lost championship: '+JSON.stringify({key,initial,restarted}));
    if(key==='c')await page.locator('#finish').click();else await page.keyboard.press('Escape');
    await page.waitForFunction(()=>Module._dd2_application_championship_phase()===-1);
    await match(page,'1','scene');
    cases.push({key,initial,resetObserved,restarted,unscored_exit:true});
  }
  for(const selected of ['7','8']) {
    await page.selectOption('#view',selected);
    await page.waitForFunction(()=>Module._dd2_application_championship_phase()===1);
    await page.locator('#finish').click();
    await match(page,'1','scene');
  }
  await page.selectOption('#level','8');
  await page.selectOption('#view','8');
  await page.waitForFunction(()=>Module._dd2_application_championship_phase()===1);
  await page.selectOption('#view','4');
  await page.waitForFunction(()=>Module._dd2_application_championship_phase()===-1 && Module._dd2_application_current_view()===4);
  if(await page.evaluate(()=>Module._dd2_application_current_level())!==1)throw new Error('Practice mode selected the old arena instead of the visible scheduled circuit');
  await page.selectOption('#view','0');await match(page,'1','scene');
  await page.selectOption('#level','8');await page.selectOption('#view','7');
  await page.waitForFunction(()=>Module._dd2_application_championship_phase()===1);
  await page.selectOption('#view','2');
  await page.waitForFunction(()=>Module._dd2_application_championship_phase()===-1);
  if(await page.evaluate(()=>Module._dd2_application_current_level())!==1)throw new Error('Free driving lost the visible scheduled circuit');
  await page.selectOption('#view','0');await match(page,'1','scene');
  report.championship={pass_:true,practice_mode_uses_visible_track:true,real_keys:true,browser_selector:true,schedule_locked:true,
    pause_and_restart:true,trusted_restart_zero_tick:true,unscored_exit:true,cases};
}

async function modalResizeChecks(page) {
  await page.selectOption('#view','0');
  await page.selectOption('#level','1');
  await page.selectOption('#view','5');
  await page.locator('#canvas').click();
  await page.waitForFunction(()=>Module._dd2_application_race_phase()===1,null,{timeout:15000});
  await page.keyboard.press('F2');
  await page.waitForFunction(()=>Module._dd2_application_profile_phase()===1);
  await page.keyboard.press('x');
  await stable(page);
  const state=()=>page.evaluate(()=>({phase:Module._dd2_application_profile_phase(),
    draft:Module.UTF8ToString(Module._dd2_application_profile_draft()),
    level:Module._dd2_application_current_level(),racePhase:Module._dd2_application_race_phase(),
    steps:Module._dd2_application_race_steps(),gain:Module._dd2_application_music_gain(),
    frame:Module._dd2_application_music_frame(),fraction:Module._dd2_application_music_fraction()}));
  const before=await state(),expected=await pixels(page);
  if(!expected.some(value=>value!==0)||!before.draft.endsWith('x'))throw new Error('Modal visual baseline unavailable');
  const cases=[];
  const preserved=async label=>{
    await pause(300);
    if(!(await pixels(page)).equals(expected))throw new Error(label+': modal framebuffer changed or cleared');
    if(JSON.stringify(await state())!==JSON.stringify(before))throw new Error(label+': modal/game/audio state changed');
    cases.push({label,canvas_sha256:digest(expected),state_preserved:true});
  };
  await page.setViewportSize({width:680,height:900});await preserved('ordinary viewport resize');
  await page.evaluate(()=>window.dispatchEvent(new Event('resize')));await preserved('same-size resize');
  await page.screenshot({path:path.join(output,'browser-modal-fullpage.png'),fullPage:true});
  await preserved('full-page capture/resize');
  await page.locator('#canvas').screenshot({path:path.join(output,'browser-modal-after-resize.png')});
  await page.setViewportSize({width:680,height:1000});await preserved('restore viewport');
  await page.keyboard.press('Escape');await page.waitForFunction(()=>Module._dd2_application_profile_phase()===0);
  await page.selectOption('#view','0');await match(page,'1','scene');
  report.modal_resize={pass_:true,cases,before,scope:'Actual modal pixels, draft and frozen game/audio state across viewport/canvas invalidation'};
}

async function main() {
  const browser=await chromium.launch({headless:true});
  const page=await browser.newPage({viewport:{width:680,height:1000}});
  page.on('pageerror',error=>errors.push(String(error)));
  page.on('console',message=>{if(message.type()==='error')errors.push(message.text());});
  try {
    await page.goto(url + '?reference=1');
    await page.waitForFunction(()=>!document.querySelector('#archive').disabled,null,{timeout:60000});
    report.cross_origin_isolated=await page.evaluate(()=>crossOriginIsolated);
    if(!report.cross_origin_isolated)throw new Error('Browser workers are not isolated');
    await page.setInputFiles('#archive',{name:'Dirinfo',mimeType:'application/octet-stream',buffer:Buffer.alloc(1024)});
    await page.waitForFunction(()=>document.querySelector('#status').textContent.includes('could not be loaded'),null,{timeout:15000});
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
    const canvasBounds = await page.locator('#canvas').boundingBox();
    if (!canvasBounds) throw new Error('Canvas has no visible bounds');
    await page.mouse.move(canvasBounds.x + canvasBounds.width / 2, canvasBounds.y + canvasBounds.height / 2);
    await page.mouse.wheel(0,-120);await changed(page,baseline);
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
    await raceChecks(page);
    await trialChecks(page);
    await totalChecks(page);
    await championshipChecks(page);
    await modalResizeChecks(page);
    await page.locator('#canvas').screenshot({path:path.join(output,'browser-scene.png')});
    await page.keyboard.press('Escape');await page.waitForFunction(()=>Module._dd2_application_current_level()===0);
    await page.waitForFunction(()=>document.querySelector('#level').disabled);
    await page.setInputFiles('#archive',archive);
    await page.waitForFunction(()=>Module._dd2_application_current_level()===1);await match(page,'1','scene');
    report.shutdown_restart=true;
    await page.evaluate(()=>Module._dd2_application_close());
    await page.waitForFunction(()=>Module._dd2_application_current_level()===0);
    await pause(200);
    await page.setInputFiles('#archive',archive);
    await page.waitForFunction(()=>Module._dd2_application_current_level()===1);
    await match(page,'1','scene');
    report.owned_loop_api_close_reopen=true;
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
