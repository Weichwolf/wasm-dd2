// Actual persisted statistics menus; genuine keys and read-only engine/canvas observations.
const assert = require('assert'), fs = require('fs'), path = require('path');
const {createHash} = require('crypto');
const {serve, boot, chromium} = require('./felib');
const {installMenuInput} = require('./menu_input');
assert(process.argv.length === 5, 'usage: BUILD OUTPUT ORIGINAL_STATISTICS_FIXTURE');
const build = path.resolve(process.argv[2]), output = path.resolve(process.argv[3]);
const fixture = path.resolve(process.argv[4]);
assert(output.startsWith('/tmp/wasm-dd2/'), 'verification captures belong in /tmp/wasm-dd2');
const parent = fs.realpathSync(path.dirname(output));
assert(parent === '/tmp/wasm-dd2' || parent.startsWith('/tmp/wasm-dd2/'), 'output parent escapes work area');
fs.mkdirSync(output);
const sha = data => createHash('sha256').update(data).digest('hex');
const planBytes = fs.readFileSync(path.join(__dirname,'../statistics_ui.json'));
const plan = JSON.parse(planBytes), layout = JSON.parse(fs.readFileSync(path.join(__dirname,'../championship_save_layout.json')));
const producer = JSON.parse(fs.readFileSync(path.join(fixture,'report.json')));
const card = fs.readFileSync(path.join(fixture,'original.card'));
assert(producer.operation === 'generate' && producer.pass_ && !producer.engine_state_writes &&
  producer.binary_sha256 === '0f993e063436262e37c03b914882a442936fa298b4ea0999ed51bfd00e0658b2');
assert(card.length === 0x20000 && sha(card) === producer.card_sha256, 'original fixture card changed');
const report = {scope:plan.scope,operation:'capture',target:'browser',pass_:false,engine_state_writes:false,
  initial_card_sha256:sha(card),plan_sha256:sha(planBytes),binary_sha256:sha(fs.readFileSync(path.join(build,'index.wasm'))),
  input_keys:[],checkpoints:[],trusted_keyboard_events:[]};
function checkBudget() {
  function bytes(directory) {
    let total = 0;
    for (const entry of fs.readdirSync(directory,{withFileTypes:true})) {
      const file = path.join(directory,entry.name);
      try { if (entry.isDirectory()) total += bytes(file); else if (entry.isFile()) total += fs.statSync(file).size; }
      catch (error) { if (error.code !== 'ENOENT') throw error; }
    }
    return total;
  }
  const disk = fs.statfsSync(output);
  assert(bytes(output) < 2*1024**3, 'capture exceeds 2 GiB');
  assert(disk.bavail*disk.bsize >= 1024**3, 'leave at least 1 GiB free in /tmp');
}
const nav = {Left:'ArrowLeft',Right:'ArrowRight',Up:'ArrowUp',Down:'ArrowDown',Return:'Enter',Escape:'Escape'};
async function ready(page, poly) {
  await page.waitForFunction(poly => HEAP16[0x46996c>>1] === 0 && window.__slabReadyFrames >= 16 &&
    (poly === undefined || HEAPU32[0x940010>>2] === poly), poly);
}
async function key(page, code) {
  await ready(page);
  const mask = {Left:0x80,Right:0x20,Up:0x10,Down:0x40,Return:0x4000,Escape:0x1000}[code];
  await page.waitForFunction(mask => ((HEAPU16[0x754448>>1] | HEAPU16[0x75444a>>1]) & mask) === 0, mask);
  await page.keyboard.down(nav[code]);
  try { await page.waitForFunction(mask => ((HEAPU16[0x754448>>1] | HEAPU16[0x75444a>>1]) & mask) !== 0, mask); }
  finally { await page.keyboard.up(nav[code]); }
  await page.waitForFunction(mask => (HEAPU16[0x754448>>1] & mask) === 0, mask);
  await page.waitForTimeout(300); report.input_keys.push(code);
}
async function savedState(page) {
  return page.evaluate(layout => {
    const data = new DataView(HEAPU8.buffer);
    const hex = (address,size) => Array.from(HEAPU8.subarray(address,address+size),b => b.toString(16).padStart(2,'0')).join('');
    const state = {joy_present:HEAPU8[0x754451]};
    for (const [name,address] of layout.fields) state[name] = data.getInt32(address,true);
    for (const [name,address,size] of layout.regions) state[name] = hex(address,size);
    return state;
  },layout);
}
async function displayed(page, action) {
  return page.evaluate(action => {
    const raw = address => UTF8ToString(address), text = address => UTF8ToString(HEAPU32[address>>2]);
    const row = {name:action.checkpoint,menu:HEAPU32[0x940010>>2]};
    if ('label' in action) row.label = text(action.label_address);
    if ('driver' in action) Object.assign(row,{driver:HEAP32[0x467eec>>2],sprite:raw(0x93e7a0),
      driver_name:raw(0x93e680),car:raw(0x93e6f0),car_type:HEAPU8[0x466a0c+action.driver],
      seasons:Array.from({length:5},(_,i) => Object.fromEntries(
        [['season',0x93e700],['wins',0x93e6a0],['kills',0x93e750],['dnf',0x93e630]].map(([name,base]) => [name,raw(base+i*16)])))});
    if ('track' in action) Object.assign(row,{track:HEAP32[0x4685e0>>2],caption:text(0x4683e8),
      seasons:Array.from({length:5},(_,i) => Object.fromEntries(
        [['season',0x93fb5c,12],['winner',0x93fa80,32],['kills',0x93fb98,12],['dnf',0x93fb20,12]]
          .map(([name,base,stride]) => [name,raw(base+i*stride)])))});
    if ('championship' in action) Object.assign(row,{championship:HEAP32[0x467bf4>>2],caption:raw(0x93e390),
      standings:Array.from({length:20},(_,i) => raw(0x93e3b0+i*32))});
    return row;
  },action);
}
async function captureCycle(page, action) {
  const directory = path.join(output,action.checkpoint,'cycle'); fs.mkdirSync(directory,{recursive:true});
  await page.evaluate(() => {window.__statisticsFrames=[];window.__captureStatistics=true;});
  await page.waitForFunction(() => !window.__captureStatistics,null,{timeout:15000});
  const frames = await page.evaluate(() => window.__statisticsFrames), metadata = [];
  assert(frames.length === 64 && new Set(frames.map(f => f.phase)).size === 64);
  for (let index = 0; index < frames.length; index++) {
    const frame = frames[index], prefix = `frame${String(index).padStart(3,'0')}`;
    assert(frame.canvas_mismatches === 0, 'actual canvas pixels differ');
    for (const region of ['framebuf','palette']) fs.writeFileSync(path.join(directory,prefix+'-'+region+'.bin'),Buffer.from(frame[region],'base64'));
    const {framebuf,palette,...state} = frame; metadata.push({...state,index,prefix});
  }
  fs.writeFileSync(path.join(directory,'cycle.json'),JSON.stringify({stage:'browser platform present',frames:metadata},null,2)+'\n');
  return directory;
}
(async () => {
  const server = serve(build), regular = server.listeners('request')[0]; server.removeAllListeners('request');
  server.on('request',(request,response) => {
    if (request.url === '/fixture.html') {response.writeHead(200,{'Content-Type':'text/html'});response.end('<!doctype html><title>Initial file setup</title>');}
    else regular(request,response);
  });
  await new Promise(resolve => server.listen(0,resolve));
  let browser, guard, watchdog; const errors=[];
  try {
    checkBudget(); browser = await chromium.launch({args:['--no-sandbox']});
    guard = setInterval(() => {try {checkBudget();} catch(error) {errors.push(error.message);browser.close().catch(() => {});}},1000);
    watchdog = setTimeout(() => {errors.push('capture exceeded time limit');browser.close().catch(() => {});},700000);
    const page = await browser.newPage(); await installMenuInput(page);
    await page.goto(`http://localhost:${server.address().port}/fixture.html`);
    await page.evaluate(bytes => new Promise((resolve,reject) => {
      const request = indexedDB.open('/persist',21);
      request.onupgradeneeded = () => {const store=request.result.createObjectStore('FILE_DATA');store.createIndex('timestamp','timestamp',{unique:false});};
      request.onerror = () => reject(request.error);
      request.onsuccess = () => {
        const db=request.result,transaction=db.transaction('FILE_DATA','readwrite');
        transaction.objectStore('FILE_DATA').put({timestamp:new Date(),mode:0o100644,contents:Uint8Array.from(bytes)},'/persist/SaveGames');
        transaction.oncomplete=()=>{db.close();resolve();};transaction.onerror=()=>reject(transaction.error);
      };
    }),Array.from(card));
    await page.exposeFunction('observeStatisticsKey',event => report.trusted_keyboard_events.push(event));
    await page.addInitScript(() => {
      window.__captureStatistics=false;
      for (const type of ['keydown','keyup']) window.addEventListener(type,e => window.observeStatisticsKey({type,code:e.code,trusted:e.isTrusted}));
      const present=CanvasRenderingContext2D.prototype.putImageData;
      const encode=(address,size) => {let text='';for(let i=0;i<size;i+=16384) text+=String.fromCharCode(...HEAPU8.subarray(address+i,address+Math.min(i+16384,size)));return btoa(text);};
      CanvasRenderingContext2D.prototype.putImageData=function(...args) {
        const result=present.apply(this,args);
        if(this.canvas.id==='canvas' && window.__captureStatistics) {
          let mismatches=0;const rgba=this.getImageData(0,0,640,480).data;
          for(let i=0;i<307200;i++) {const pal=0x700050+HEAPU8[0x700450+i]*4,pixel=i*4;
            if(rgba[pixel]!==HEAPU8[pal] || rgba[pixel+1]!==HEAPU8[pal+1] || rgba[pixel+2]!==HEAPU8[pal+2] || rgba[pixel+3]!==255) mismatches++;}
          window.__statisticsFrames.push({phase:HEAP32[0x4699cc>>2],card_phase:HEAP32[0x467390>>2]%255,
            cf:HEAP32[0x462ff0>>2],level:HEAP32[0x936ff4>>2],poly_list:HEAPU32[0x940010>>2],
            sound_volume:HEAP32[0x467410>>2],working_sound_volume:HEAP32[0x93fd20>>2],master_sfx_volume:HEAP32[0x462d84>>2],
            framebuf:encode(0x700450,307200),palette:encode(0x700050,1024),canvas_mismatches:mismatches});
          if(window.__statisticsFrames.length===64) window.__captureStatistics=false;
        }
        return result;
      };
    });
    page.on('pageerror',e => errors.push(e.message)); await boot(page,server);
    report.loaded_state = await savedState(page); assert.deepStrictEqual(report.loaded_state,producer.saved_state,'startup did not restore complete configuration');
    for (const action of plan.actions) {
      for (const code of action.keys) await key(page,code);
      await ready(page,action.menu);
      const row = await displayed(page,action);
      for (const name of ['driver','track','championship']) if (name in action) assert(row[name] === action[name], 'wrong '+name);
      if ('label' in action) assert(row.label.includes(action.label), 'wrong selected statistics category');
      if (action.cycle) row.cycle = await captureCycle(page,action);
      report.checkpoints.push(row); console.log('Statistics checkpoint',row.name);
    }
    assert.deepStrictEqual(await savedState(page),producer.saved_state,'statistics navigation changed saved data');
    report.statistics_unchanged = true;
    const disk = await page.evaluate(() => {const raw=FS.readFile('/SaveGames');return {raw:Array.from(raw),matches:raw.every((v,i) => v === HEAPU8[0x754460+i])};});
    report.card_unchanged = Buffer.from(disk.raw).equals(card); report.engine_matches_file = disk.matches;
    assert(report.card_unchanged && report.engine_matches_file,'statistics navigation changed card');
    checkBudget(); assert(errors.length===0,errors.join('; ')); report.pass_=true;
    console.log('Actual browser persisted statistics: PASS');
  } catch(error) {report.error=error.stack;throw error;}
  finally {
    clearInterval(guard); clearTimeout(watchdog);
    fs.writeFileSync(path.join(output,'report.json'),JSON.stringify(report,null,2)+'\n');
    if(browser) await browser.close(); await new Promise(resolve => server.close(resolve));
  }
})().catch(error => {console.error(error);process.exitCode=1;});
