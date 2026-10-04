// Shared actual configuration/card UI scenario, real keys and browser restarts.
// Read-only observers; selected complete indexed frames and canvas conversion.
const assert = require('assert'), fs = require('fs'), path = require('path');
const {createHash} = require('crypto');
const {serve, boot, chromium} = require('./felib');
const {installMenuInput} = require('./menu_input');
const build = path.resolve(process.argv[2]), output = path.resolve(process.argv[3]);
assert(output.startsWith('/tmp/wasm-dd2/'), 'verification outputs belong in /tmp/wasm-dd2');
const parent = fs.realpathSync(path.dirname(output));
assert(parent === '/tmp/wasm-dd2' || parent.startsWith('/tmp/wasm-dd2/'), 'output parent escapes work area');
fs.mkdirSync(output);
const planBytes = fs.readFileSync(path.join(__dirname,'../configuration_card_ui.json'));
const plan = JSON.parse(planBytes), sha = data => createHash('sha256').update(data).digest('hex');
const reference = process.argv[4] ? JSON.parse(fs.readFileSync(path.join(process.argv[4],'report.json'))) : null;
if (reference) assert(reference.pass_ && reference.plan_sha256 === sha(planBytes), 'completed original scenario required');
const report = {scope: plan.scope, target: 'browser', pass_: false, engine_state_writes: false,
  plan_sha256: sha(planBytes), binary_sha256: sha(fs.readFileSync(path.join(build,'index.wasm'))), checkpoints: []};
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
async function key(page, key) {
  await ready(page);
  const mask = {Left:0x80,Right:0x20,Up:0x10,Down:0x40,Return:0x4000,Escape:0x1000}[key];
  await page.waitForFunction(mask => ((HEAPU16[0x754448>>1] | HEAPU16[0x75444a>>1]) & mask) === 0, mask);
  await page.keyboard.down(nav[key]);
  try { await page.waitForFunction(mask => ((HEAPU16[0x754448>>1] | HEAPU16[0x75444a>>1]) & mask) !== 0, mask); }
  finally { await page.keyboard.up(nav[key]); }
  await page.waitForFunction(mask => (HEAPU16[0x754448>>1] & mask) === 0, mask);
  await page.waitForTimeout(300);
  console.log('Browser key',key);
}
async function select(page, slot) {
  await key(page,'Return');
  await page.waitForFunction(() => UTF8ToString(HEAPU32[0x46725c>>2]).includes('Select') && HEAP32[0x774680>>2] >= 0);
  for (const code of [...Array(Math.floor(slot/3)).fill('Down'),...Array(slot%3).fill('Right')]) await key(page,code);
  assert(await page.evaluate(() => HEAP32[0x774680>>2]) === slot, 'wrong selected memory-card block');
  await key(page,'Return');
}
async function enterName(page, letter, replace=false) {
  if (replace) for (const code of ['Down','Down','Return','Up','Up']) await key(page,code);
  const index = letter.charCodeAt(0)-65, row = Math.floor(index/13), col = index%13;
  for (const code of [...Array(col).fill('Right'),...Array(row).fill('Down'),'Return',
    ...Array(2-row).fill('Down'),'Right','Return']) await key(page,code);
}
async function card(page, name) {
  const observed = await page.evaluate(() => {
    const raw = FS.readFile('/SaveGames');
    return {raw:Array.from(raw), matches:raw.length === 0x20000 && raw.every((v,i) => v === HEAPU8[0x754460+i])};
  });
  assert(observed.matches, 'actual disk card differs from engine RAM');
  const raw = Buffer.from(observed.raw), file = path.join(output,name+'.card'), slots = [];
  fs.writeFileSync(file,raw);
  for (let index = 0; index < 15; index++) {
    const base = index*0x200;
    if (raw.readUInt32LE(base) === 1) {
      const payload = raw.subarray(0x2000+index*0x2000,0x2000+index*0x2000+0x197e);
      slots.push({index,name:raw.subarray(base+4,Math.min(raw.indexOf(0,base+4),base+32)).toString('ascii'),
        magic:payload.readUInt16LE(0),binding:Array.from(payload.subarray(0x196c)),payload_sha256:sha(payload),sound:payload.readInt16LE(16)});
    }
  }
  return {bytes:raw.length,sha256:sha(raw),engine_matches_file:true,slots,file};
}
async function captureCycle(page,action) {
  const name=action.checkpoint;
  const directory = path.join(output,name,'cycle'); fs.mkdirSync(directory,{recursive:true});
  let wanted=null;
  if (action.card_highlight && reference) {
    const source=reference.checkpoints.find(row => row.name===name);
    wanted=JSON.parse(fs.readFileSync(path.join(source.cycle,'cycle.json'))).frames.map(f => [f.phase,f.card_phase]);
    assert(new Set(wanted.map(pair => pair.join(':'))).size===64);
  }
  await page.evaluate(wanted => {
    window.__volumeCycle=[];window.__volumeCycleEntries=0;
    window.__volumeCycleWanted=wanted ? new Set(wanted.map(pair => pair.join(':'))) : null;
    window.__captureVolumeCycle=true;
  },wanted);
  await page.waitForFunction(() => !window.__captureVolumeCycle,null,{timeout:wanted ? 420000 : 15000});
  const observed=await page.evaluate(() => window.__volumeCycleEntries);
  const frames = await page.evaluate(() => window.__volumeCycle), metadata = [];
  assert(frames.length === 64 && new Set(frames.map(f => f.phase)).size === 64);
  for (let index = 0; index < frames.length; index++) {
    const frame = frames[index], prefix = `frame${String(index).padStart(3,'0')}`;
    assert(frame.canvas_mismatches === 0, 'actual canvas pixels differ');
    for (const region of ['framebuf','palette']) fs.writeFileSync(path.join(directory,prefix+'-'+region+'.bin'),Buffer.from(frame[region],'base64'));
    const {framebuf,palette,...state} = frame; metadata.push({...state,index,prefix});
  }
  fs.writeFileSync(path.join(directory,'cycle.json'),JSON.stringify({stage:'browser platform present',frames:metadata,
    observed_entries:observed,wanted_pairs:wanted},null,2)+'\n');
  return directory;
}
async function snapshot(page,action) {
  await ready(page);
  const row = await page.evaluate(name => {
    const text = address => {const pointer=HEAPU32[address>>2];return pointer ? UTF8ToString(pointer) : '';};
    return {name,settings:{mode:HEAP32[0x4673f8>>2],type:HEAP32[0x4673f4>>2],car:HEAP32[0x467400>>2],
      track:HEAP32[0x4673fc>>2],sound:HEAP32[0x467410>>2],pad_option:HEAP32[0x467414>>2],
      saved:Array.from(HEAPU8.subarray(0x46757a,0x46758c)),active:Array.from(HEAPU8.subarray(0x46302c,0x46303a))},
      working:HEAP32[0x93fd20>>2],master:HEAP32[0x462d84>>2],menu:HEAPU32[0x940010>>2],
      file_mode:HEAP32[0x93a318>>2],file_slot:HEAP32[0x774680>>2],prompt:text(0x4672ac)};
  },action.checkpoint);
  row.card = await card(page,action.checkpoint);
  assert(row.settings.sound === action.sound, 'stored volume differs at '+row.name);
  for (const field of ['working','master']) if (field in action) assert(row[field] === action[field], field+' volume differs at '+row.name);
  assert.deepEqual(row.card.slots.map(s => [s.index,s.name,s.sound]), action.cards, 'card directory differs at '+row.name);
  if (action.prompt) assert(row.prompt.includes(action.prompt));
  if (action.cycle) row.cycle = await captureCycle(page,action);
  report.checkpoints.push(row);
  console.log('Configuration checkpoint',row.name,'sound',row.settings.sound,'working',row.working,'master',row.master);
}
async function drive(page,actions) {
  for (const action of actions) {
    for (const code of action.keys || []) await key(page,code);
    if ('menu' in action) await ready(page,action.menu);
    if (action.limit) {
      assert(await page.evaluate(() => HEAPU32[0x940010>>2]) === 0x46898c);
      await page.keyboard.down(nav[action.limit]);
      try {
        await page.waitForFunction(v => HEAP32[0x93fd20>>2] === v,action.value);
        await page.waitForTimeout(300);
        assert(await page.evaluate(() => HEAP32[0x93fd20>>2]) === action.value, 'volume failed to clamp');
      } finally { await page.keyboard.up(nav[action.limit]); }
      await page.waitForFunction(() => (HEAPU16[0x754448>>1] | HEAPU16[0x75444a>>1]) === 0);
    }
    if ('select' in action) await select(page,action.select);
    if (action.save) {
      await select(page,action.save[0]); await enterName(page,action.save[1]);
      await page.waitForFunction(slot => FS.readFile('/SaveGames')[slot*0x200] === 1, action.save[0]);
    }
    if (action.name) await enterName(page,action.name,action.replace);
    if (action.checkpoint) await snapshot(page,action);
  }
}
(async () => {
  const server = serve(build); await new Promise(resolve => server.listen(0,resolve));
  let context, guard, watchdog; const errors=[]; report.trusted_keyboard_events=[];
  try {
    checkBudget();
    guard = setInterval(() => {try {checkBudget();} catch(error) {errors.push(error.message);clearInterval(guard);context?.close().catch(() => {});}},1000);
    watchdog = setTimeout(() => {errors.push('capture exceeded time limit');context?.close().catch(() => {});},1800000);
    for (const actions of plan.sessions) {
      context = await chromium.launchPersistentContext(path.join(output,'profile'),{args:['--no-sandbox'],headless:true});
      const page = await context.newPage(); await installMenuInput(page);
      await page.exposeFunction('observeVolumeKey',event => report.trusted_keyboard_events.push(event));
      await page.addInitScript(() => {
        window.__captureVolumeCycle=false;
        for (const type of ['keydown','keyup']) window.addEventListener(type,e => window.observeVolumeKey({type,code:e.code,trusted:e.isTrusted}));
        const present=CanvasRenderingContext2D.prototype.putImageData;
        const encode=(address,size) => {let text='';for(let i=0;i<size;i+=16384) text+=String.fromCharCode(...HEAPU8.subarray(address+i,address+Math.min(i+16384,size)));return btoa(text);};
        CanvasRenderingContext2D.prototype.putImageData=function(...args) {
          const result=present.apply(this,args);
          if(this.canvas.id==='canvas' && window.__captureVolumeCycle) {
            const phase=HEAP32[0x4699cc>>2],card_phase=HEAP32[0x467390>>2]%255;
            window.__volumeCycleEntries++;
            if(window.__volumeCycleEntries>3328) throw new Error('card/highlight phases did not reach requested pairs');
            if(window.__volumeCycleWanted && !window.__volumeCycleWanted.delete(phase+':'+card_phase)) return result;
            let mismatches=0;const rgba=this.getImageData(0,0,640,480).data;
            for(let i=0;i<307200;i++) {const pal=0x700050+HEAPU8[0x700450+i]*4,pixel=i*4;
              if(rgba[pixel]!==HEAPU8[pal] || rgba[pixel+1]!==HEAPU8[pal+1] || rgba[pixel+2]!==HEAPU8[pal+2] || rgba[pixel+3]!==255) mismatches++;}
            window.__volumeCycle.push({phase,card_phase,cf:HEAP32[0x462ff0>>2],level:HEAP32[0x936ff4>>2],
              poly_list:HEAPU32[0x940010>>2],sound_volume:HEAP32[0x467410>>2],working_sound_volume:HEAP32[0x93fd20>>2],master_sfx_volume:HEAP32[0x462d84>>2],
              framebuf:encode(0x700450,307200),palette:encode(0x700050,1024),canvas_mismatches:mismatches});
            if(window.__volumeCycle.length===64) window.__captureVolumeCycle=false;
          }
          return result;
        };
      });
      page.on('pageerror',e => errors.push(e.message));
      await boot(page,server);
      if (!report.initial_save_sha256) report.initial_save_sha256=await page.evaluate(async () => {
        const hash=await crypto.subtle.digest('SHA-256',FS.readFile('/SaveGames'));
        return Array.from(new Uint8Array(hash),b => b.toString(16).padStart(2,'0')).join('');
      });
      await drive(page,actions); await page.goto('about:blank'); await context.close(); context=null;
    }
    checkBudget(); assert(errors.length===0,errors.join('; '));
    assert(report.trusted_keyboard_events.length>0 && report.trusted_keyboard_events.every(e => e.trusted));
    report.pass_=true;
  } finally {
    clearInterval(guard);clearTimeout(watchdog);if(context) await context.close();server.close();
    fs.writeFileSync(path.join(output,'report.json'),JSON.stringify({...report,errors},null,2)+'\n');
  }
  console.log('Actual browser configuration/card UI: PASS');
})().catch(error => {console.error(error.stack);process.exitCode=1;});
