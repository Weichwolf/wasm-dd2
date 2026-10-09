'use strict';
const fs = require('fs'), path = require('path'), crypto = require('crypto');
const {chromium} = require('../browser/playwright');
const [url, archive, redbook, output] = process.argv.slice(2);
if (!output || !path.resolve(output).startsWith('/tmp/wasm-dd2/')) throw new Error('Use /tmp/wasm-dd2/');
const input = JSON.parse(fs.readFileSync(path.join(output,'browser-input.json')));
const bytes = name => Buffer.from(input[name], 'hex');
const hash = value => crypto.createHash('sha256').update(value).digest('hex');
const report = {scope:'Actual preference UI, original-compatible complete cards, public C lifetime, Chromium process restart and live restored music PCM; not full configuration/game restoration.',cases:[],pcm:[],errors:[],complete:false};
const database = 'wasm-dd2-saves-v1';
let context;
function check(value,message) { if (!value) throw new Error(message); }
async function launch() {
  context=await chromium.launchPersistentContext(path.join(output,'chromium-profile'),{headless:true,viewport:{width:1120,height:1050},args:['--no-sandbox','--disable-dev-shm-usage']});
  return openPage();
}
async function openPage() {
  const page=await context.newPage();
  page.on('pageerror',error=>report.errors.push(String(error)));
  await page.goto(url);
  await page.waitForFunction(()=>!document.querySelector('#archive').disabled);
  await page.setInputFiles('#archive',archive);
  await page.waitForFunction(()=>Module._dd2_application_current_level()===1 && !document.querySelector('#saves-open').disabled);
  return page;
}
async function openSaves(page) {
  await page.locator('#saves-open').click();
  await page.waitForFunction(()=>Module._dd2_application_saves_phase()===2 && !document.querySelector('#preferences-save').disabled);
}
async function wait(page) {
  await page.waitForFunction(()=>Module._dd2_application_saves_poll()!==1 && !document.querySelector('#preferences-save').disabled);
}
async function gains(page,effects,music) {
  await page.locator('#effects-gain').evaluate((element,value)=>{element.value=String(value);element.dispatchEvent(new Event('input'));},effects);
  await page.locator('#music-gain').evaluate((element,value)=>{element.value=String(value);element.dispatchEvent(new Event('input'));},music);
}
async function current(page,effects,music,label) {
  const state=await page.evaluate(()=>({effects:Module._dd2_application_effects_gain(),music:Module._dd2_application_music_gain(),level:Module._dd2_application_current_level()}));
  check(state.effects===effects && state.music===music,label+' live gain differs: '+JSON.stringify(state));
  report.cases.push({label,...state});
}
async function card(page, expected,label) {
  const actual=Buffer.from(await page.evaluate(name=>new Promise((resolve,reject)=>{
    const request=indexedDB.open(name,1);
    request.onerror=()=>reject(request.error);
    request.onsuccess=()=>{const db=request.result,tx=db.transaction('images','readonly');let value;tx.objectStore('images').get('SaveGames').onsuccess=event=>value=event.target.result;tx.oncomplete=()=>{db.close();resolve(value===undefined?[]:[...new Uint8Array(value)]);};tx.onabort=()=>{db.close();reject(tx.error);};};
  }),database));
  check(actual.equals(expected),label+' independent complete IndexedDB image differs');
  report.cases.push({label,bytes:actual.length,sha256:hash(actual)});
}
async function inject(page, image) {
  await page.evaluate(({name,bytes})=>new Promise((resolve,reject)=>{
    const request=indexedDB.open(name,1);
    request.onerror=()=>reject(request.error);
    request.onsuccess=()=>{const db=request.result,tx=db.transaction('images','readwrite',{durability:'strict'});tx.objectStore('images').put(new Uint8Array(bytes).buffer,'SaveGames');tx.oncomplete=()=>{db.close();resolve();};tx.onabort=()=>{db.close();reject(tx.error);};};
  }),{name:database,bytes:[...image]});
  await page.locator('#saves-reload').click();await wait(page);
}
function put(before,logical,name,payload) {
  const image=Buffer.from(before), occupied=[];
  for(let physical=0;physical<15;++physical) if(image.readUInt32LE(physical*512)) occupied.push(physical);
  if(logical<occupied.length) {image.writeUInt32LE(0,occupied[logical]*512);image[occupied[logical]*512+4]=0;}
  let physical=0;while(image.readUInt32LE(physical*512))++physical;
  image.writeUInt32LE(1,physical*512);image.write(name+'\0',physical*512+4,'utf8');payload.copy(image,(physical+1)*8192);
  return image;
}
function extension(music) {
  const result=Buffer.alloc(16);result.write('D2CF');result.writeUInt16LE(1,4);result.writeUInt16LE(16,6);result.writeUInt16LE(music,8);
  let value=2166136261;for(let i=0;i<12;++i)value=Math.imul(value^result[i],16777619)>>>0;
  result.writeUInt32LE(value,12);return result;
}
function rounded(value,divisor) { return Math.sign(value)*Math.floor((Math.abs(value)+Math.floor(divisor/2))/divisor); }
async function musicOutput(page,gain) {
  const source=fs.readFileSync(redbook), frames=source.length/4;
  await page.locator('#canvas').click();
  await page.waitForFunction(()=>Module.SDL2.audioContext.state==='running');
  await page.evaluate(()=>{
    const node=Module.SDL2.audio.scriptProcessorNode,original=node.onaudioprocess;
    window.preferencePcm=[];
    node.onaudioprocess=event=>{
      const before={frame:Module._dd2_application_music_frame(),fraction:Module._dd2_application_music_fraction(),rate:Module._dd2_application_music_rate()};
      original(event);
      if(window.preferencePcm.length>=8)return;
      const left=Array.from(event.outputBuffer.getChannelData(0)),right=Array.from(event.outputBuffer.getChannelData(1));
      if(left.some(value=>value!==0)||right.some(value=>value!==0))window.preferencePcm.push({...before,left,right});
    };
    window.restorePcm=()=>{node.onaudioprocess=original;};
  });
  await page.waitForFunction(()=>window.preferencePcm.length>=8,null,{timeout:15000});
  const chunks=await page.evaluate(()=>{window.restorePcm();return window.preferencePcm;});
  for(const chunk of chunks) {
    const pcm=Buffer.alloc(chunk.left.length*4);
    for(let i=0;i<chunk.left.length;++i) {
      const phase=chunk.frame*chunk.rate+chunk.fraction+i*44100;
      const frame=Math.floor(phase/chunk.rate)%frames,fraction=phase%chunk.rate,next=(frame+1)%frames;
      for(let channel=0;channel<2;++channel) {
        const value=rounded(rounded(source.readInt16LE(frame*4+channel*2)*(chunk.rate-fraction)+source.readInt16LE(next*4+channel*2)*fraction,chunk.rate)*gain,256);
        const actual=Math.round((channel?chunk.right:chunk.left)[i]*32768);
        check(actual===value,'Restored live music PCM differs');pcm.writeInt16LE(actual,i*4+channel*2);
      }
    }
    report.pcm.push({gain,frames:chunk.left.length,frame:chunk.frame,fraction:chunk.fraction,rate:chunk.rate,sha256:hash(pcm)});
  }
}
(async()=>{
  try {
    let page=await launch();await openSaves(page);
    await card(page,Buffer.alloc(0),'open-does-not-create-card');
    await gains(page,128,64);await page.locator('#save-name').fill('A');await page.locator('#preferences-save').click();await wait(page);
    const a=put(bytes('empty'),0,'A',bytes('a'));
    await card(page,a,'A-committed');
    await gains(page,32,16);await page.locator('#save-slot').selectOption('1');await page.locator('#save-name').fill('B');await page.locator('#preferences-save').click();await wait(page);
    await card(page,bytes('ab'),'A-B-committed');
    await page.locator('#save-slot').selectOption('0');await page.locator('#preferences-load').click();
    await current(page,128,64,'A-live-audio-restored');
    await page.locator('#save-slot').selectOption('2');await page.locator('#save-name').fill('B');await page.locator('#preferences-save').click();
    await page.waitForFunction(()=>document.querySelector('#save-status').textContent.includes('already used'));
    await card(page,bytes('ab'),'duplicate-rejected');
    await page.locator('#save-slot').selectOption('0');
    page.once('dialog',dialog=>dialog.dismiss());await page.locator('#preferences-save').click();
    await card(page,bytes('ab'),'replacement-cancellation');
    page.once('dialog',dialog=>dialog.dismiss());await page.locator('#save-delete').click();
    await card(page,bytes('ab'),'deletion-cancellation');
    page.once('dialog',dialog=>dialog.accept());await page.locator('#save-delete').click();await wait(page);
    await card(page,bytes('remaining'),'delete-compacts-visible-B');
    check((await page.locator('#save-slot option').first().textContent()).includes('B'),'physical B not reflected at logical zero');
    await page.locator('#preferences-load').click();await current(page,32,16,'compacted-B-restores-selected-payload');
    await context.close();context=null;
    page=await launch();await openSaves(page);
    await current(page,256,256,'fresh-process-starts-with-owned-defaults');
    await page.locator('#preferences-load').click();await current(page,32,16,'fresh-process-restores-B');
    await card(page,bytes('remaining'),'fresh-process-complete-card');
    await page.locator('#level').selectOption('2');
    await page.locator('#view').selectOption('1');
    await page.locator('#view').selectOption('0');
    await page.locator('#view').selectOption('8');
    await page.waitForFunction(()=>Module._dd2_application_championship_phase()===1);
    await page.locator('#finish').click();
    await page.waitForFunction(()=>Module._dd2_application_championship_phase()===-1);
    await page.locator('#level').selectOption('1');
    await page.locator('#saves-reload').click();await wait(page);
    await current(page,32,16,'navigation-retains-owned-preferences-and-store');
    await card(page,bytes('remaining'),'navigation-retains-complete-card');
    const second=await openPage();await openSaves(second);
    await page.evaluate(()=>{
      window.originalPreferencePut=IDBObjectStore.prototype.put;
      IDBObjectStore.prototype.put=function(...args){const request=window.originalPreferencePut.apply(this,args);this.transaction.abort();return request;};
    });
    await gains(page,128,64);page.once('dialog',dialog=>dialog.accept());await page.locator('#preferences-save').click();
    await page.waitForFunction(()=>document.querySelector('#save-status').textContent.includes('failed'));
    await current(page,128,64,'write-abort-keeps-live-edits');await card(page,bytes('remaining'),'write-abort-keeps-previous-card');
    await page.evaluate(()=>{IDBObjectStore.prototype.put=window.originalPreferencePut;});
    const pending=await page.evaluate(()=>{
      const accepted=Module.ccall('dd2_application_save_preferences','number',['number','string'],[1,'C']);
      return {accepted,closed:Module._dd2_application_close(),level:Module._dd2_application_current_level()};
    });
    check(pending.accepted===1 && pending.closed===0 && pending.level===1,'pending application was freed');
    await wait(page);
    const cb=put(bytes('remaining'),1,'C',bytes('a'));
    await card(page,cb,'close-refused-until-write-completes');
    check(await second.evaluate(()=>Module.ccall('dd2_application_save_preferences','number',['number','string'],[1,'STALE']))===1,'stale begin rejected');
    await second.waitForFunction(()=>Module._dd2_application_saves_phase()===5);
    await card(second,cb,'stale-application-cannot-overwrite');
    check(await second.locator('#preferences-save').isDisabled(),'stale application can retry');
    await second.locator('#saves-reload').click();await wait(second);
    await second.close();
    let legacy=put(bytes('empty'),0,'LEGACY',bytes('patterned'));await inject(page,legacy);
    await page.locator('#save-slot').selectOption('0');await page.locator('#preferences-load').click();
    await current(page,230,64,'original-volume-and-music-fallback');
    await gains(page,230,65); // Editing effects here intentionally adopts the slider's quantization.
    await page.locator('#preferences-load').click(); // Restore exact original 3681, retaining music 65.
    const edited=Buffer.from(bytes('patterned'));edited.writeUInt16LE(0x1010);extension(65).copy(edited,6526);
    await page.locator('#save-name').fill('EDIT');page.once('dialog',dialog=>dialog.accept());await page.locator('#preferences-save').click();await wait(page);
    await card(page,put(legacy,0,'EDIT',edited),'legacy-source-fields-and-volume-retained');
    const invalid=Buffer.from(edited);invalid[6541]^=1;
    await inject(page,put(bytes('empty'),0,'BAD',invalid));
    await page.locator('#preferences-load').click();await current(page,230,65,'bad-extension-keeps-live-audio');
    await page.waitForFunction(()=>document.querySelector('#save-status').textContent.includes('not a valid'));
    const game=Buffer.from(bytes('factory'));game.writeUInt16LE(0x3030);
    await inject(page,put(bytes('empty'),0,'GAME',game));
    check(await page.locator('#preferences-load').isDisabled(),'saved game offered as audio configuration');
    check(await page.evaluate(()=>Module._dd2_application_load_preferences(0))===0,'GAME changed live preferences');
    await current(page,230,65,'GAME-is-not-configuration');
    await inject(page,put(bytes('empty'),0,'A',bytes('a')));
    await page.locator('#preferences-load').click();await current(page,128,64,'restored-A-before-PCM');
    await page.setInputFiles('#music-file',redbook);
    await page.waitForFunction(()=>Module._dd2_application_music_track()===2);
    await musicOutput(page,64);
    await page.locator('fieldset').screenshot({path:path.join(output,'preferences-loaded.png')});
    check(await page.evaluate(()=>Module._dd2_application_close())===1,'terminal application did not close');
    check(await page.evaluate(()=>Module._dd2_application_saves_poll())===2,'close erased terminal storage receipt');
    await page.waitForFunction(()=>document.querySelector('#preferences-save').disabled && !document.querySelector('#archive').disabled);
    check(report.errors.length===0,'Browser errors: '+report.errors.join('; '));
    report.complete=true;
  } finally {
    if(context)await context.close();
    fs.writeFileSync(path.join(output,'browser-report.json'),JSON.stringify(report,null,2)+'\n');
  }
})().catch(error=>{console.error(error);process.exitCode=1;});
