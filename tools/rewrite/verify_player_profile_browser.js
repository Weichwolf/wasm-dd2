'use strict';
const fs=require('fs'),path=require('path'),crypto=require('crypto');
const {chromium}=require('../browser/playwright');
const [url,archive,output]=process.argv.slice(2);
if(!output||!path.resolve(output).startsWith('/tmp/wasm-dd2/'))throw new Error('Use /tmp/wasm-dd2/');
const input=JSON.parse(fs.readFileSync(path.join(output,'browser-input.json')));
const bytes=name=>Buffer.from(input[name],'hex');
const hash=value=>crypto.createHash('sha256').update(value).digest('hex');
const report={scope:'Actual player/car/audio UI, complete independently predicted IndexedDB images, Native-style keyboard dialogs, process restart and championship identity lock. Not full configuration, playable saved-game restoration or natural named season results.',cases:[],errors:[],complete:false};
const database='wasm-dd2-saves-v1';
let context;
function check(value,message){if(!value)throw new Error(message);}
function put(before,logical,name,payload){
  const image=Buffer.from(before),occupied=[];
  for(let p=0;p<15;++p)if(image.readUInt32LE(p*512))occupied.push(p);
  if(logical<occupied.length){image.writeUInt32LE(0,occupied[logical]*512);image[occupied[logical]*512+4]=0;}
  let p=0;while(image.readUInt32LE(p*512))++p;
  image.writeUInt32LE(1,p*512);image.write(name+'\0',p*512+4,'ascii');payload.copy(image,(p+1)*8192);
  return image;
}
function remove(before,logical){
  const image=Buffer.from(before),occupied=[];
  for(let p=0;p<15;++p)if(image.readUInt32LE(p*512))occupied.push(p);
  check(logical<occupied.length,'Independent delete target is absent');
  image.fill(0,occupied[logical]*512,occupied[logical]*512+5);
  return image;
}
function named(payload,name){const result=Buffer.from(payload);result.write(name+'\0',5828,'ascii');return result;}
function extension(music){
  const result=Buffer.alloc(16);result.write('D2CF');result.writeUInt16LE(1,4);result.writeUInt16LE(16,6);result.writeUInt16LE(music,8);
  let value=2166136261;for(let i=0;i<12;++i)value=Math.imul(value^result[i],16777619)>>>0;
  result.writeUInt32LE(value,12);return result;
}
async function presented(page){
  await page.evaluate(()=>new Promise(resolve=>{
    requestAnimationFrame(()=>requestAnimationFrame(resolve));
  }));
}
async function classPixels(page){
  return Buffer.from(await page.evaluate(()=>{
    const canvas=document.querySelector('#canvas');
    const top=Math.ceil(canvas.height*0.7);
    const rgba=canvas.getContext('2d').getImageData(0,top,canvas.width,canvas.height-top).data;
    let binary='';for(let i=0;i<rgba.length;i+=32768)binary+=String.fromCharCode(...rgba.subarray(i,i+32768));
    return btoa(binary);
  }),'base64');
}
async function launch(){
  context=await chromium.launchPersistentContext(path.join(output,'chromium-profile'),{headless:true,viewport:{width:1120,height:1100},args:['--no-sandbox','--disable-dev-shm-usage']});
  const page=await context.newPage();page.on('pageerror',error=>report.errors.push(String(error)));
  await page.goto(url);await page.waitForFunction(()=>!document.querySelector('#archive').disabled);
  await page.setInputFiles('#archive',archive);
  await page.waitForFunction(()=>Module._dd2_application_current_level()===1&&!document.querySelector('#saves-open').disabled);
  return page;
}
async function open(page){await page.locator('#saves-open').click();await wait(page);}
async function wait(page){await page.waitForFunction(()=>Module._dd2_application_saves_phase()===2&&!document.querySelector('#profile-save').disabled);}
async function current(page,name,effects,music,label,car){
  const row=await page.evaluate(()=>({name:Module.UTF8ToString(Module._dd2_application_player_name()),effects:Module._dd2_application_effects_gain(),music:Module._dd2_application_music_gain(),car:Module._dd2_application_current_car()}));
  check(row.name===name&&row.effects===effects&&row.music===music&&(car===undefined||row.car===car),label+' live state differs: '+JSON.stringify(row));report.cases.push({label,...row});
}
async function player(page,name){
  const before=await page.evaluate(()=>[Module._dd2_application_current_view(),Module._dd2_application_championship_phase(),Module._dd2_application_current_level()]);
  await page.locator('#player-name').fill('');
  if(name)await page.keyboard.type(name);
  await page.locator('#player-apply').click();
  const reflected=await page.evaluate(()=>({live:Module.UTF8ToString(Module._dd2_application_player_name()),field:document.querySelector('#player-name').value}));
  check(reflected.live===reflected.field,'Accepted name action left a stale HTML field');
  const after=await page.evaluate(()=>[Module._dd2_application_current_view(),Module._dd2_application_championship_phase(),Module._dd2_application_current_level()]);
  check(JSON.stringify(before)===JSON.stringify(after),'Typing in the HTML name field dispatched game commands');
}
async function gains(page,effects,music){
  for(const [id,value] of [['effects-gain',effects],['music-gain',music]])await page.locator('#'+id).evaluate((element,value)=>{element.value=String(value);element.dispatchEvent(new Event('input'));},value);
}
async function card(page,expected,label){
  const actual=Buffer.from(await page.evaluate(name=>new Promise((resolve,reject)=>{
    const request=indexedDB.open(name,1);request.onerror=()=>reject(request.error);
    request.onsuccess=()=>{const db=request.result,tx=db.transaction('images','readonly');let value;
      tx.objectStore('images').get('SaveGames').onsuccess=event=>value=event.target.result;
      tx.oncomplete=()=>{db.close();resolve(value===undefined?[]:[...new Uint8Array(value)]);};tx.onabort=()=>{db.close();reject(tx.error);};};
  }),database));
  check(actual.equals(expected),label+' complete IndexedDB image differs');report.cases.push({label,bytes:actual.length,sha256:hash(actual)});
}
async function writeStored(page,image){
  await page.evaluate(({name,bytes})=>new Promise((resolve,reject)=>{
    const request=indexedDB.open(name,1);request.onerror=()=>reject(request.error);
    request.onsuccess=()=>{const db=request.result,tx=db.transaction('images','readwrite',{durability:'strict'});tx.objectStore('images').put(new Uint8Array(bytes).buffer,'SaveGames');tx.oncomplete=()=>{db.close();resolve();};tx.onabort=()=>{db.close();reject(tx.error);};};
  }),{name:database,bytes:[...image]});
}
async function inject(page,image){
  await writeStored(page,image);
  await page.locator('#saves-reload').click();await wait(page);
  await page.locator('#save-slot').selectOption('0');
}
async function phase(page,value,draft=null,slot=null){
  await page.waitForFunction(({value,draft,slot})=>Module._dd2_application_profile_phase()===value&&(draft===null||Module.UTF8ToString(Module._dd2_application_profile_draft())===draft)&&(slot===null||Module._dd2_application_profile_slot()===slot),{value,draft,slot});
}
async function editor(page,before,after){
  for(let i=0;i<before.length;++i)await page.keyboard.press('Backspace');
  await phase(page,await page.evaluate(()=>Module._dd2_application_profile_phase()),'');
  if(after)await page.keyboard.type(after);
  await phase(page,await page.evaluate(()=>Module._dd2_application_profile_phase()),after);
}
(async()=>{
  try{
    let page=await launch();await open(page);await card(page,Buffer.alloc(0),'opening-missing-saves-keeps-no-image');
    await player(page,'Racer_7!');await gains(page,128,64);await current(page,'Racer_7!',128,64,'new-eight-byte-name');
    await page.locator('#save-name').fill('A');await page.locator('#profile-save').click();await wait(page);
    const a=put(bytes('empty'),0,'A',bytes('a'));await card(page,a,'profile-A-durable');
    await player(page,'LOCAL');await gains(page,32,16);
    await page.locator('#preferences-load').click();await current(page,'LOCAL',128,64,'audio-only-restore-keeps-active-name');
    await page.locator('#profile-load').click();await current(page,'Racer_7!',128,64,'full-profile-restores-name-and-audio');
    await player(page,'');await gains(page,32,16);await current(page,'PLAYER',32,16,'empty-name-resolves-owned-default');
    await page.locator('#save-slot').selectOption('1');await page.locator('#save-name').fill('B');await page.locator('#profile-save').click();await wait(page);
    await card(page,bytes('ab'),'two-profiles-retain-bytes-after-name-terminator');
    await page.locator('#save-slot').selectOption('0');
    page.once('dialog',dialog=>dialog.dismiss());await page.locator('#profile-save').click();await card(page,bytes('ab'),'profile-overwrite-cancel');
    const invalid=await page.evaluate(()=>['123456789','Bad\n','ä'].map(name=>Module.ccall('dd2_application_set_player_name','number',['string'],[name])));
    check(invalid.every(result=>result===0),'invalid name accepted');await current(page,'PLAYER',32,16,'invalid-name-rollback');
    await context.close();context=null;page=await launch();await open(page);
    await current(page,'PLAYER',256,256,'fresh-browser-process-owned-defaults');
    await page.locator('#profile-load').click();await current(page,'Racer_7!',128,64,'fresh-browser-process-restores-selected-profile');
    await card(page,bytes('ab'),'fresh-browser-process-retains-complete-image');
    await page.locator('#view').selectOption('8');await page.waitForFunction(()=>Module._dd2_application_championship_phase()===1);
    check(await page.locator('#player-apply').isDisabled()&&await page.locator('#profile-load').isDisabled(),'championship identity controls enabled');
    check(await page.locator('#level').isDisabled(),'championship track selector enabled');
    const before=await page.evaluate(()=>({name:Module.UTF8ToString(Module._dd2_application_player_name()),phase:Module._dd2_application_championship_phase(),season:Module._dd2_application_championship_season(),round:Module._dd2_application_championship_round()}));
    check(await page.evaluate(()=>Module.ccall('dd2_application_set_player_name','number',['string'],['OTHER']))===0,'championship name setter accepted');
    check(await page.evaluate(()=>Module._dd2_application_load_profile(1))===0,'championship profile load accepted');
    await page.locator('#save-slot').selectOption('1');await page.locator('#preferences-load').click();await current(page,'Racer_7!',32,16,'championship-audio-only-load-keeps-name');
    const after=await page.evaluate(()=>({name:Module.UTF8ToString(Module._dd2_application_player_name()),phase:Module._dd2_application_championship_phase(),season:Module._dd2_application_championship_season(),round:Module._dd2_application_championship_round()}));
    check(JSON.stringify(before)===JSON.stringify(after),'championship identity/schedule mutated');
    await page.locator('#finish').click();await page.waitForFunction(()=>Module._dd2_application_championship_phase()===-1);
    const legacy=put(bytes('empty'),0,'LEGACY',bytes('legacy'));await inject(page,legacy);await page.locator('#profile-load').click();
    await current(page,'LongName_11',256,256,'eleven-byte-legacy-identity-preserved');
    await page.locator('#save-name').fill('EDIT');page.once('dialog',dialog=>dialog.accept());await page.locator('#profile-save').click();await wait(page);
    await card(page,put(legacy,0,'EDIT',bytes('legacy')),'unmodified-legacy-profile-resave-retains-full-name');
    const patterned=put(bytes('empty'),0,'SOURCE',bytes('patterned'));await inject(page,patterned);await page.locator('#profile-load').click();
    await current(page,'LongName_11',230,256,'patterned-source-exact-effects-and-legacy-name',2);
    const saved=Buffer.from(bytes('patterned'));saved.writeUInt16LE(0x1010);extension(256).copy(saved,6526);
    await page.locator('#save-name').fill('EDIT');page.once('dialog',dialog=>dialog.accept());await page.locator('#profile-save').click();await wait(page);
    await card(page,put(patterned,0,'EDIT',saved),'complete-patterned-fields-reserved-bytes-and-volume-retained');
    await player(page,'LOCAL');await gains(page,32,16);
    for(const [label,text] of [['control',Buffer.from('BAD\n\0')],['utf8',Buffer.from('ä\0')],['unterminated',Buffer.alloc(12,88)]]){
      const payload=Buffer.from(bytes('a'));text.copy(payload,5828);const image=put(bytes('empty'),0,'BAD',payload);
      await inject(page,image);await page.locator('#profile-load').click();await current(page,'LOCAL',32,16,label+'-profile-rollback');await card(page,image,label+'-retained-image');
    }
    for(const invalidCar of [-32768,-1,3,32767]){
      await gains(page,32,16);
      const payload=Buffer.from(bytes('a'));payload.writeInt16LE(invalidCar,6);
      const image=put(bytes('empty'),0,'BAD',payload);await inject(page,image);
      await page.locator('#profile-load').click();
      await current(page,'LOCAL',32,16,'invalid-car-'+invalidCar+'-profile-rollback',2);
      await card(page,image,'invalid-car-'+invalidCar+'-retained-image');
      await page.locator('#preferences-load').click();
      await current(page,'LOCAL',128,64,'audio-only-invalid-dormant-car-'+invalidCar,2);
      await page.locator('#save-name').fill('AUDIO');page.once('dialog',dialog=>dialog.accept());
      await page.locator('#preferences-save').click();await wait(page);
      const audioOnly=put(image,0,'AUDIO',payload);
      await card(page,audioOnly,'audio-save-retains-dormant-car-'+invalidCar);
      const owned=named(payload,'LOCAL');owned.writeInt16LE(2,6);
      await page.locator('#save-name').fill('OWNED');page.once('dialog',dialog=>dialog.accept());
      await page.locator('#profile-save').click();await wait(page);
      await card(page,put(audioOnly,0,'OWNED',owned),'profile-save-captures-live-class-'+invalidCar);
    }
    await inject(page,a);await page.locator('#profile-load').click();
    // Actual SDL keyboard modal in the browser: no direct mutation of editor or
    // game owners. Observe the public read-only phase, draft and selected slot.
    await page.locator('#canvas').focus();await page.keyboard.press('F2');await phase(page,1,'Racer_7!');
    await page.waitForFunction(()=>document.querySelector('#level').disabled&&document.querySelector('#profile-save').disabled);
    check(await page.locator('#level').isDisabled()&&await page.locator('#profile-save').isDisabled(),'modal browser controls enabled');
    await editor(page,'Racer_7!','CANCEL');await page.keyboard.press('Escape');await phase(page,0);
    await current(page,'Racer_7!',128,64,'keyboard-name-dialog-cancel');
    await page.keyboard.press('F2');await phase(page,1,'Racer_7!');await editor(page,'Racer_7!','Browser!');await page.keyboard.press('Enter');await phase(page,0);
    await current(page,'Browser!',128,64,'keyboard-name-dialog-accept');
    await page.keyboard.press('F3');await phase(page,4,null,0);
    for(let i=0;i<14;++i)await page.keyboard.press('ArrowRight');await phase(page,4,null,14);
    for(let i=0;i<14;++i)await page.keyboard.press('ArrowLeft');await phase(page,4,null,0);
    await page.keyboard.press('Enter');await phase(page,6,'A');await editor(page,'A','KEY');await page.keyboard.press('Enter');await phase(page,7);
    await page.keyboard.press('Escape');await phase(page,0);await card(page,a,'keyboard-all-15-slots-and-overwrite-cancel');
    await page.keyboard.press('F3');await phase(page,4);await page.keyboard.press('Enter');await phase(page,6,'A');await editor(page,'A','KEY');await page.keyboard.press('Enter');await phase(page,7);
    await page.keyboard.press('Enter');await phase(page,9);
    const keyboard=put(a,0,'KEY',named(bytes('a'),'Browser!'));await card(page,keyboard,'keyboard-overwrite-confirm-durable');
    await page.locator('#canvas').screenshot({path:path.join(output,'player-profile.png')});
    await page.keyboard.press('Escape');await phase(page,0);
    await card(page,keyboard,'Escape-dismisses-completion-and-retains-durable-save');
    await player(page,'LOCAL');
    await page.locator('#canvas').focus();await page.keyboard.press('F4');await phase(page,5);await page.keyboard.press('Enter');await phase(page,9);
    await current(page,'Browser!',128,64,'keyboard-F4-restores-player-and-audio');await page.keyboard.press('Enter');await phase(page,0);
    // Real IndexedDB abort: publication fails without changing the accepted
    // image. Live edits remain owned, then an explicit load restores the old one.
    await player(page,'LOCAL');await page.evaluate(()=>{
      window.profilePut=IDBObjectStore.prototype.put;
      IDBObjectStore.prototype.put=function(...args){const request=window.profilePut.apply(this,args);this.transaction.abort();return request;};
    });
    page.once('dialog',dialog=>dialog.accept());await page.locator('#profile-save').click();
    await page.waitForFunction(()=>document.querySelector('#save-status').textContent.includes('failed'));
    await current(page,'LOCAL',128,64,'profile-write-abort-keeps-live-edit');await card(page,keyboard,'profile-write-abort-retains-durable-image');
    await page.evaluate(()=>{IDBObjectStore.prototype.put=window.profilePut;});
    await page.locator('#profile-load').click();await current(page,'Browser!',128,64,'aborted-write-can-restore-prior-profile');
    // The actual canvas Delete route operates on physical entries, regardless
    // of payload kind or duplicate filename. No C setter drives the menu.
    const game=Buffer.from(bytes('factory'));game.writeUInt16LE(0x3030);game[8191]=91;
    const replay=Buffer.from(bytes('factory'));replay.writeUInt16LE(0x2020);replay[8191]=92;
    let inventory=put(put(keyboard,1,'GAME',game),2,'REPLAY',replay);
    inventory.write('KEY\0',512+4,'ascii');await inject(page,inventory);
    await page.locator('#canvas').focus();await page.keyboard.press('Delete');await phase(page,11,null,0);
    for(let i=0;i<14;++i)await page.keyboard.press('ArrowRight');await phase(page,11,null,14);
    await page.keyboard.press('Enter');await phase(page,11,null,14);await card(page,inventory,'delete-empty-slot-refuses');
    for(let i=0;i<14;++i)await page.keyboard.press('ArrowLeft');await phase(page,11,null,0);
    await page.keyboard.press('Enter');await phase(page,12,null,0);
    await page.locator('#canvas').screenshot({path:path.join(output,'delete-confirm.png')});
    await page.keyboard.press('Escape');await phase(page,0);await card(page,inventory,'delete-separate-confirmation-cancel');
    await page.keyboard.press('Delete');await phase(page,11);await page.keyboard.press('Enter');await phase(page,12);
    const changed=put(inventory,3,'WRITER',bytes('factory'));await writeStored(page,changed);
    await page.keyboard.press('Enter');await phase(page,9);await card(page,changed,'delete-external-writer-conflict');
    await page.keyboard.press('Enter');await phase(page,0);
    await page.keyboard.press('Delete');await phase(page,11);await page.keyboard.press('ArrowRight');await phase(page,11,null,1);
    await page.keyboard.press('Enter');await phase(page,12,null,1);await page.keyboard.press('Enter');await phase(page,9);
    inventory=remove(changed,1);await card(page,inventory,'delete-selected-physical-game-duplicate-name');
    await current(page,'Browser!',128,64,'delete-keeps-active-player-and-audio');
    check(await page.evaluate(()=>Module._dd2_application_saves_count())===3,'Delete did not compact logical inventory');
    await page.keyboard.press('Escape');await phase(page,0);await card(page,inventory,'dismiss-delete-status-keeps-durable-image');
    await page.evaluate(()=>{
      window.profilePut=IDBObjectStore.prototype.put;
      IDBObjectStore.prototype.put=function(...args){const request=window.profilePut.apply(this,args);this.transaction.abort();return request;};
    });
    await page.keyboard.press('Delete');await phase(page,11);await page.keyboard.press('Enter');await phase(page,12);
    await page.keyboard.press('Enter');await phase(page,9);await card(page,inventory,'delete-transaction-abort-retains-full-image');
    check(await page.evaluate(()=>Module._dd2_application_saves_count())===3,'Aborted delete published candidate inventory');
    await page.evaluate(()=>{IDBObjectStore.prototype.put=window.profilePut;});
    await page.keyboard.press('Enter');await phase(page,0);
    // Keep a genuine strict IndexedDB transaction alive while real keys arrive.
    // The application must retain its old accepted inventory and pending owner.
    await page.evaluate(()=>{
      window.holdDelete=true;window.deletePending=null;
      IDBObjectStore.prototype.put=function(...args){
        const request=window.profilePut.apply(this,args),store=this;
        window.deletePending={phase:Module._dd2_application_profile_phase(),close:Module._dd2_application_close(),count:Module._dd2_application_saves_count()};
        function keep(){const read=store.get('SaveGames');read.onsuccess=()=>{if(window.holdDelete)keep();};}keep();return request;
      };
    });
    await page.keyboard.press('Delete');await phase(page,11);await page.keyboard.press('ArrowRight');await phase(page,11,null,1);
    await page.keyboard.press('Enter');await phase(page,12,null,1);await page.keyboard.press('Enter');await phase(page,13);
    await page.waitForFunction(()=>window.deletePending!==null);
    const pending=await page.evaluate(()=>window.deletePending);
    check(pending.phase===13&&pending.close===0&&pending.count===3,'Pending delete published data or released application: '+JSON.stringify(pending));
    await page.keyboard.press('Escape');await page.keyboard.press('Enter');
    await page.evaluate(()=>new Promise(resolve=>{let frames=0;function tick(){if(++frames===4)resolve();else requestAnimationFrame(tick);}requestAnimationFrame(tick);}));
    await phase(page,13);
    check(await page.locator('#profile-save').isDisabled(),'Pending deletion enabled other save controls');
    await page.evaluate(()=>{window.holdDelete=false;IDBObjectStore.prototype.put=window.profilePut;});await phase(page,9);
    inventory=remove(inventory,1);await card(page,inventory,'pending-delete-retains-owner-until-strict-commit');
    await page.locator('#canvas').screenshot({path:path.join(output,'delete-completed.png')});
    report.cases.push({label:'pending-delete-refuses-close-and-dismissal',...pending});
    await page.keyboard.press('Enter');await phase(page,0);
    // Restart observes the durable deletion before deleting the remaining rows.
    await context.close();context=null;page=await launch();await open(page);
    await card(page,inventory,'fresh-browser-process-retains-deleted-physical-entries');
    check(await page.evaluate(()=>Module._dd2_application_saves_count())===2,'Restart reconstructed wrong logical inventory');
    await page.locator('#canvas').focus();
    for(const label of ['delete-first-entry','delete-only-entry']){
      await page.keyboard.press('Delete');await phase(page,11);await page.keyboard.press('Enter');await phase(page,12);
      await page.keyboard.press('Enter');await phase(page,9);inventory=remove(inventory,0);await card(page,inventory,label);
      await page.keyboard.press('Enter');await phase(page,0);
    }
    await page.keyboard.press('Delete');await phase(page,11);await page.keyboard.press('Enter');await phase(page,11);
    await card(page,inventory,'empty-card-delete-refuses-without-mutation');await page.keyboard.press('Escape');await phase(page,0);
    // Persist all three owned classes, then restore each in a fresh browser
    // process. Complete images use the documented signed source car field.
    await gains(page,128,64);
    for(let carClass=0;carClass<3;++carClass){
      await page.locator('#car-class').selectOption(String(carClass));
      await player(page,'C'+carClass);
      await page.locator('#save-slot').selectOption(String(carClass));
      await page.locator('#save-name').fill('C'+carClass);await page.locator('#profile-save').click();await wait(page);
      const payload=named(bytes('factory'),'C'+carClass);payload.writeInt16LE(carClass,6);
      payload.writeUInt16LE(Math.floor((128*4090+128)/256),16);extension(64).copy(payload,6526);
      inventory=put(inventory,carClass,'C'+carClass,payload);
      await card(page,inventory,'all-class-save-'+carClass);
      await current(page,'C'+carClass,128,64,'all-class-owned-'+carClass,carClass);
    }
    for(let carClass=0;carClass<3;++carClass){
      await context.close();context=null;page=await launch();await open(page);
      await current(page,'PLAYER',256,256,'class-restart-default-'+carClass,0);
      await page.locator('#save-slot').selectOption(String(carClass));await page.locator('#profile-load').click();
      await current(page,'C'+carClass,128,64,'class-restart-restores-'+carClass,carClass);
      await page.waitForFunction(value=>document.querySelector('#car-class').value===String(value),carClass);
      await card(page,inventory,'class-restart-complete-image-'+carClass);
      await page.locator('#view').selectOption('2');await page.waitForFunction(()=>Module._dd2_application_current_view()===2);
      await page.locator('#pause').click();await page.waitForFunction(()=>Module._dd2_application_is_paused()===1);
      await player(page,'LOCAL');await gains(page,32,16);
      const steps=await page.evaluate(()=>Module._dd2_application_race_steps());
      await page.locator('#save-slot').selectOption(String((carClass+1)%3));await page.locator('#profile-load').click();
      await current(page,'LOCAL',32,16,'different-class-driving-rollback-'+carClass,carClass);
      await page.locator('#preferences-load').click();await current(page,'LOCAL',128,64,'driving-audio-only-keeps-class-'+carClass,carClass);
      await page.locator('#save-slot').selectOption(String(carClass));await page.locator('#profile-load').click();
      await current(page,'C'+carClass,128,64,'same-class-driving-restores-profile-'+carClass,carClass);
      check(await page.evaluate(()=>Module._dd2_application_race_steps())===steps,'Same-class load reset paused driving clock');
      await page.locator('#view').selectOption('1');await page.waitForFunction(()=>Module._dd2_application_current_view()===1);
      await page.locator('#car-class').selectOption(String((carClass+1)%3));
      await presented(page);
      const previous=await classPixels(page);
      await page.locator('#canvas').focus();await page.keyboard.press('F4');await phase(page,5);
      for(let i=0;i<carClass;++i)await page.keyboard.press('ArrowRight');await phase(page,5,null,carClass);
      await page.keyboard.press('Enter');await phase(page,9);
      await current(page,'C'+carClass,128,64,'keyboard-modal-restores-class-'+carClass,carClass);
      await page.locator('#canvas').screenshot({path:path.join(output,'class-profile-'+carClass+'.png')});
      const modal=await classPixels(page);await page.keyboard.press('Enter');await phase(page,0);await presented(page);
      const closed=await classPixels(page);
      check(!modal.equals(previous)&&modal.equals(closed),'Restored modal class paint/ratings differ: '+JSON.stringify({sameAsPrevious:modal.equals(previous),matchesClosed:modal.equals(closed),previous:hash(previous),modal:hash(modal),closed:hash(closed)}));
      report.cases.push({label:'restored-modal-class-pixels-'+carClass,rgba_sha256:hash(modal),canvas:await page.evaluate(()=>{const c=document.querySelector('#canvas');return {width:c.width,height:c.height};}),matches_closed_world:true,differs_from_previous_class:true});
    }
    check(await page.evaluate(()=>Module._dd2_application_close())===1,'terminal application did not close');
    check(report.errors.length===0,'Browser errors: '+report.errors.join('; '));report.complete=true;
  }finally{
    if(context)await context.close();fs.writeFileSync(path.join(output,'browser-report.json'),JSON.stringify(report,null,2)+'\n');
  }
})().catch(error=>{console.error(error);process.exitCode=1;});
