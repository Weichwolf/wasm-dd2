'use strict';
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const {chromium} = require('../browser/playwright');
const [url, output] = process.argv.slice(2);
if (!output || !path.resolve(output).startsWith('/tmp/wasm-dd2/')) throw new Error('Use /tmp/wasm-dd2/');
const report = {pass_:false, scope:'Actual original-free prepared browser and trusted input',comparisons:[]};
const errors = [];
const digest = data => crypto.createHash('sha256').update(data).digest('hex');
const pause = ms => new Promise(resolve=>setTimeout(resolve,ms));
async function pixels(page) {
  const encoded = await page.evaluate(() => {
    const canvas=document.querySelector('#canvas');
    if(canvas.width!==640 || canvas.height!==360) throw new Error('Prepared canvas extent differs');
    const rgba=canvas.getContext('2d').getImageData(0,0,640,360).data;
    const rgb=new Uint8Array(640*360*3);
    for(let source=0,target=0;source<rgba.length;source+=4) {
      rgb[target++]=rgba[source];rgb[target++]=rgba[source+1];rgb[target++]=rgba[source+2];
    }
    let binary='';
    for(let index=0;index<rgb.length;index+=32768) binary+=String.fromCharCode(...rgb.subarray(index,index+32768));
    return btoa(binary);
  });
  return Buffer.from(encoded,'base64');
}
function comparison(expected,actual) {
  if(expected.length!==actual.length) throw new Error('Prepared reference extent differs');
  let changed=0,total=0;
  for(let offset=0;offset<actual.length;offset+=3) {
    let different=false;
    for(let channel=0;channel<3;channel++) {
      const error=Math.abs(expected[offset+channel]-actual[offset+channel]);
      total+=error;different||=error!==0;
    }
    changed+=different;
  }
  return {changed_pixels:changed,mean_channel_error:total/actual.length,
          pass_:changed<=640*360*.01 && total/actual.length<=.5,
          expected_sha256:digest(expected),actual_sha256:digest(actual)};
}
async function match(page,code,mode) {
  const header=Buffer.from('P6\n640 360\n255\n');
  const raw=fs.readFileSync(path.join(output,`wasm-${code}-${mode}.ppm`));
  if(!raw.subarray(0,header.length).equals(header)) throw new Error('Invalid prepared preview');
  let result;
  for(let attempt=0;attempt<80;attempt++) {
    result=comparison(raw.subarray(header.length),await pixels(page));
    if(result.pass_) break;
    await pause(100);
  }
  report.comparisons.push({level:code,mode,...result});
  if(!result.pass_) {
    await page.locator('#canvas').screenshot({path:path.join(output,`failed-browser-${code}-${mode}.png`)});
    throw new Error(`Prepared browser ${code}/${mode} differs: ${JSON.stringify(result)}`);
  }
}
async function main() {
  const browser=await chromium.launch({headless:true});
  const page=await browser.newPage({viewport:{width:680,height:1200}});
  page.on('pageerror',error=>errors.push(String(error)));
  page.on('console',message=>{if(message.type()==='error')errors.push(message.text());});
  try {
    await page.goto(url);
    await page.waitForFunction(()=>typeof Module!=='undefined' && Module._dd2_application_current_level &&
      Module._dd2_application_current_level()===1,null,{timeout:120000});
    report.initial=await page.evaluate(()=>({isolated:crossOriginIsolated,archiveHidden:document.querySelector('#archive').closest('label').hidden,
      noDirinfo:!Module.FS.analyzePath('/Dirinfo').exists,tracks:Module.FS.readdir('/content/roads').filter(n=>n.endsWith('.dd2road')).length,
      level:Module._dd2_application_current_level(),width:canvas.width,height:canvas.height}));
    if(!report.initial.isolated || !report.initial.archiveHidden || !report.initial.noDirinfo || report.initial.tracks!==11) {
      throw new Error('Prepared default startup/reference boundary failed');
    }
    await page.evaluate(()=>{window.observedKeys=[];document.querySelector('#canvas').addEventListener('keydown',event=>window.observedKeys.push({key:event.key,trusted:event.isTrusted}));});
    let level=1;
    for(const code of '123456789AB') {
      await page.selectOption('#level',String(level++));
      await page.selectOption('#view','0');await match(page,code,'world');
      await page.selectOption('#view','1');await match(page,code,'car');
      await page.selectOption('#view','2');
      await page.evaluate(()=>Module._dd2_application_set_paused(1));
      await page.click('#reset');await match(page,code,'start');
      await page.selectOption('#view','0');
    }
    await page.selectOption('#level','1');await page.selectOption('#view','2');
    await page.evaluate(()=>Module._dd2_application_set_paused(1));await page.click('#reset');
    const baseline=await pixels(page);
    await page.click('#pause');await page.locator('#canvas').focus();
    await page.keyboard.down('w');await pause(600);await page.keyboard.up('w');
    await page.keyboard.press('p');await pause(300);
    const moved=await pixels(page);
    if(digest(moved)===digest(baseline)) throw new Error('Prepared trusted driving did not move');
    await page.locator('#canvas').screenshot({path:path.join(output,'browser-moving.png')});
    const steps=await page.evaluate(()=>Module._dd2_application_race_steps());
    await page.keyboard.down('w');await pause(300);await page.keyboard.up('w');
    if(digest(await pixels(page))!==digest(moved)) throw new Error('Prepared paused canvas moved');
    await page.click('#reset');await match(page,'1','start');
    report.driving={pass_:true,baseline_sha256:digest(baseline),moved_sha256:digest(moved),paused_steps:steps};
    for(const mode of ['7','8']) {
      await page.selectOption('#view',mode);
      await page.waitForFunction(()=>Module._dd2_application_championship_phase()===1);
      await page.evaluate(()=>{Module._dd2_application_set_paused(1); if(!Module._dd2_application_restart_championship()) throw new Error('Restart rejected');Module._dd2_application_set_paused(1);});
      if(await page.evaluate(()=>Module._dd2_application_race_steps())!==0) throw new Error('Prepared championship restart failed');
      await page.locator('#canvas').screenshot({path:path.join(output,`browser-championship-${mode}.png`)});
      await page.click('#finish');
      await page.waitForFunction(()=>Module._dd2_application_championship_phase()===-1);
    }
    report.championship={pass_:true,start_restart_exit:true,whole_campaign:false};
    await page.selectOption('#view','0');await page.locator('#canvas').focus();
    await page.keyboard.press('F2');
    await page.waitForFunction(()=>Module._dd2_application_profile_phase()===1);
    await page.locator('#canvas').screenshot({path:path.join(output,'browser-profile.png')});
    await page.keyboard.press('Escape');
    await page.waitForFunction(()=>Module._dd2_application_profile_phase()===0);
    const names=await page.evaluate(()=>Module.FS.readdir('/content/reference/scenes'));
    if(!names.includes('level-2.dd2scene')) throw new Error('Prepared browser scene missing');
    const failure=await page.evaluate(()=>{
      Module.FS.rename('/content/reference/scenes/level-2.dd2scene','/content/reference/scenes/temporarily-hidden.dd2scene');
      try { const before=Module._dd2_application_current_level();const accepted=Module._dd2_application_select_level(2);return {before,accepted,after:Module._dd2_application_current_level()}; }
      finally { Module.FS.rename('/content/reference/scenes/temporarily-hidden.dd2scene','/content/reference/scenes/level-2.dd2scene'); }
    });
    if(failure.accepted || failure.before!==failure.after) throw new Error('Missing prepared asset changed owner/fell back');
    report.transactional_missing_scene=failure;
    report.keys=await page.evaluate(()=>window.observedKeys);
    if(!report.keys.some(k=>k.key==='w'&&k.trusted)) throw new Error('No trusted throttle key');
    if(errors.length) throw new Error(errors.join('\n'));
    if(!await page.evaluate(()=>Module._dd2_application_close())) throw new Error('Prepared browser close rejected');
    await pause(100);
    if(await page.evaluate(()=>Module._dd2_application_current_level())!==0) throw new Error('Prepared close retained active game');
    report.pass_=true;
  } finally {
    report.errors=errors;
    fs.writeFileSync(path.join(output,'browser.json'),JSON.stringify(report,null,2)+'\n');
    await browser.close();
  }
}
main().catch(error=>{console.error(error);process.exitCode=1;});
