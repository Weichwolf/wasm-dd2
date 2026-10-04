// Real keyboard -> Save Configuration -> full Chromium restart -> player race.
// Only read engine/FS state; persistence uses the production IDBFS write hook.
const assert = require('assert'), fs = require('fs'), path = require('path');
const {createHash} = require('crypto');
const {serve, boot, chromium} = require('./felib');
const {installMenuInput} = require('./menu_input');
const build = path.resolve(process.argv[2]), output = path.resolve(process.argv[3]);
assert(output.startsWith('/tmp/wasm-dd2/'), 'output must remain in /tmp/wasm-dd2');
const parent = fs.realpathSync(path.dirname(output));
assert(parent === '/tmp/wasm-dd2' || parent.startsWith('/tmp/wasm-dd2/'), 'output parent escapes work area');
fs.mkdirSync(output);
function checkBudget() {
  function bytes(directory) {
    let total = 0;
    for (const entry of fs.readdirSync(directory, {withFileTypes: true})) {
      const file = path.join(directory, entry.name);
      try {
        if (entry.isDirectory()) total += bytes(file);
        else if (entry.isFile()) total += fs.statSync(file).size;
      } catch (error) { if (error.code !== 'ENOENT') throw error; }
    }
    return total;
  }
  const disk = fs.statfsSync(output);
  assert(bytes(output) < 2*1024**3, 'capture exceeds 2 GiB');
  assert(disk.bavail*disk.bsize >= 1024**3, '/tmp has less than 1 GiB free');
}
const sha = raw => createHash('sha256').update(raw).digest('hex');
const report = {scope: 'Actual browser configuration save, complete browser process restart, normal IndexedDB loading and remapped player controls; no timed video/audio parity claim',
  target: 'browser', pass_: false, engine_state_writes: false,
  binary_sha256: sha(fs.readFileSync(path.join(build, 'index.wasm')))};
const bindings = [['B', 5], ['C', 3], ['D', 8], ['E', 12], ['F', 13]];
const settings = page => page.evaluate(() => ({mode: HEAP32[0x4673f8>>2], type: HEAP32[0x4673f4>>2],
  car: HEAP32[0x467400>>2], track: HEAP32[0x4673fc>>2], sound: HEAP32[0x467410>>2], pad_option: HEAP32[0x467414>>2],
  saved: Array.from(HEAPU8.subarray(0x46757a, 0x46758c)), active: Array.from(HEAPU8.subarray(0x46302c, 0x46303a))}));
async function card(page, name) {
  const observed = await page.evaluate(() => {
    const raw = FS.readFile('/SaveGames');
    return {raw: Array.from(raw), engine_matches_file: raw.length === 0x20000 && raw.every((v,i) => v === HEAPU8[0x754460+i])};
  });
  assert(observed.engine_matches_file, 'actual browser card differs from engine RAM');
  const raw = Buffer.from(observed.raw), file = path.join(output, name+'.card'), slots = [];
  fs.writeFileSync(file, raw);
  for (let index = 0; index < 15; index++) {
    const base = index*0x200;
    if (raw.readUInt32LE(base) === 1) {
      const payload = raw.subarray(0x2000+index*0x2000, 0x2000+index*0x2000+0x197e);
      const end = raw.indexOf(0, base+4);
      slots.push({index, name: raw.subarray(base+4, Math.min(end, base+32)).toString('ascii'),
        magic: payload.readUInt16LE(0), binding: Array.from(payload.subarray(0x196c)), payload_sha256: sha(payload)});
    }
  }
  return {bytes: raw.length, sha256: sha(raw), engine_matches_file: true, slots, file};
}
async function key(page, code, expectedPoly) {
  await page.waitForFunction(() => window.__slabReadyFrames >= 16);
  const mask = {ArrowDown: 0x40, ArrowUp: 0x10, ArrowLeft: 0x80, ArrowRight: 0x20, Enter: 0x4000, Escape: 0x1000}[code];
  assert(mask, 'known menu key required');
  await page.waitForFunction(mask => ((HEAPU16[0x754448>>1] | HEAPU16[0x75444a>>1]) & mask) === 0, mask);
  await page.keyboard.down(code);
  try {
    await page.waitForFunction(mask => ((HEAPU16[0x754448>>1] | HEAPU16[0x75444a>>1]) & mask) !== 0 || HEAP32[0x936ff4>>2] > 0, mask);
  } finally { await page.keyboard.up(code); }
  await page.waitForFunction(mask => (HEAPU16[0x754448>>1] & mask) === 0, mask);
  await page.waitForTimeout(300);
  if (expectedPoly !== undefined) {
    await page.waitForFunction(poly => HEAPU32[0x940010>>2] === poly &&
      HEAP16[0x46996c>>1] === 0 && window.__slabReadyFrames >= 16, expectedPoly);
  }
  console.log('Browser key', code, await page.evaluate(() => HEAPU32[0x940010>>2].toString(16)));
}
async function save(page) {
  report.initial = await settings(page); report.initial_card = await card(page, 'initial');
  report.initial_save_sha256 = report.initial_card.sha256;
  assert(report.initial_card.slots.length === 0, 'fresh provisioned card required');
  for (const [code, poly] of [['ArrowDown',0x4696b0], ['ArrowRight',0x4696b0], ['ArrowRight',0x4696b0],
    ['Enter',0x4690c4], ['Enter',0x46a134], ['Enter',0x469298]]) await key(page, code, poly);
  for (const [letter, offset] of bindings) {
    await page.waitForFunction(() => HEAPU32[0x940010>>2] === 0x469298 && window.__slabReadyFrames >= 16);
    await page.keyboard.down(letter.toLowerCase());
    try { await page.waitForFunction(({letter, offset}) => HEAPU8[0x93fd90+offset] === letter.charCodeAt(0), {letter, offset}); }
    finally { await page.keyboard.up(letter.toLowerCase()); }
    await page.waitForTimeout(100);
  }
  await key(page, 'Enter', 0x4690c4); report.changed = await settings(page);
  const expected = report.initial.saved.slice();
  for (const [letter, offset] of bindings) expected[offset] = letter.charCodeAt(0);
  expected[9] = 68;
  assert.deepEqual(report.changed.saved, expected); assert(report.changed.pad_option === 0);
  for (const code of ['ArrowRight','ArrowRight','Enter']) await key(page, code);
  await page.waitForFunction(() => HEAP32[0x93a318>>2] === 3);
  const packed = Buffer.from(await page.evaluate(() => Array.from(HEAPU8.subarray(0x93a490, 0x93a490+0x197e))));
  assert(packed.readUInt16LE(0) === 0x1010); assert.deepEqual(Array.from(packed.subarray(0x196c)), expected);
  report.packed_sha256 = sha(packed);
  for (const code of ['Enter','Enter','Enter','ArrowDown','ArrowDown','ArrowRight','Enter']) await key(page, code);
  await page.waitForFunction(() => FS.readFile('/SaveGames')[0] === 1);
  report.saved_card = await card(page, 'saved');
  assert(report.saved_card.slots.length === 1 && report.saved_card.slots[0].name === 'A');
  assert(report.saved_card.slots[0].payload_sha256 === sha(packed));
  await key(page, 'Escape', 0x4690c4); await key(page, 'Escape', 0x4696b0);
}
async function race(page) {
  for (const code of ['ArrowDown', 'ArrowDown', 'Enter']) await key(page, code);
  await page.waitForFunction(() => HEAP32[0x936ff4>>2] >= 1 && HEAP32[0x936ff4>>2] <= 10 &&
    HEAP32[0x7746c0>>2] > 0 && HEAP32[0x784298>>2] < 1, null, {timeout: 60000});
  const initial = await page.evaluate(() => ({player: HEAP32[0x93ded0>>2], level: HEAP32[0x936ff4>>2],
    demo: HEAP32[0x46385c>>2], replay: HEAP32[0x467074>>2], quit: HEAP32[0x7746ac>>2],
    active: Array.from(HEAPU8.subarray(0x46302c, 0x46303a)), saved: Array.from(HEAPU8.subarray(0x46757a,0x467588))}));
  assert(initial.player >= 0 && initial.player < 20 && initial.demo === 0 && initial.replay === 0 && initial.quit === 0);
  assert.deepEqual(initial.active, initial.saved);
  const state = () => page.evaluate(player => {
    const view = new DataView(HEAPU8.buffer), stride = player*0x1b2;
    return {ticks: HEAP32[0x7746c0>>2], held: HEAPU16[0x754448>>1],
      throttle: view.getInt32(0x792a86+stride, true), steering: view.getInt32(0x792a82+stride, true),
      position: [view.getInt32(0x792a30+stride,true), view.getInt32(0x792a38+stride,true)]};
  }, initial.player);
  const probes = [];
  const cases = [[['E'],0x4000,'throttle',32768], [['F'],0x8000,'throttle',-32768],
    [['B'],0x80,'steering',-256], [['C'],0x20,'steering',256],
    [['D','B'],0x480,'steering',-511], [['D','C'],0x420,'steering',511],
    [['A','Z','space'],0,'throttle',0]];
  for (const [keys, mask, field, value] of cases) {
    const before = await state();
    for (const k of keys) await page.keyboard.down(k === 'space' ? 'Space' : k.toLowerCase());
    let held;
    try {
      await page.waitForFunction(({player, mask, field, value}) => {
        const view = new DataView(HEAPU8.buffer), v = view.getInt32((field === 'throttle' ? 0x792a86 : 0x792a82)+player*0x1b2,true);
        return HEAPU16[0x754448>>1] === mask && v === value;
      }, {player: initial.player, mask, field, value}, {timeout: 15000});
      const start = (await state()).ticks;
      await page.waitForFunction(start => HEAP32[0x7746c0>>2] >= start+4, start);
      held = await state(); assert(held.held === mask);
      if (keys.join('') === 'E') assert.notDeepEqual(held.position, before.position, 'acceleration did not move player');
    } finally {
      for (const k of keys.slice().reverse()) await page.keyboard.up(k === 'space' ? 'Space' : k.toLowerCase());
    }
    await page.waitForFunction(player => {
      const view = new DataView(HEAPU8.buffer), stride = player*0x1b2;
      return HEAPU16[0x754448>>1] === 0 && view.getInt32(0x792a86+stride,true) === 0 && view.getInt32(0x792a82+stride,true) === 0;
    }, initial.player);
    probes.push({keys, mask, before, held, released: await state()});
  }
  return {player: initial.player, level: initial.level, active: initial.active, probes};
}
(async () => {
  const server = serve(build); await new Promise(resolve => server.listen(0, resolve));
  let context, watchdog, resourceGuard; const errors = []; report.trusted_keyboard_events = [];
  const launch = async () => {
    context = await chromium.launchPersistentContext(path.join(output, 'profile'), {args: ['--no-sandbox'], headless: true});
    const page = await context.newPage();
    await page.exposeFunction('observeTrustedKey', event => report.trusted_keyboard_events.push(event));
    await page.addInitScript(() => {
      for (const type of ['keydown','keyup']) window.addEventListener(type, e => window.observeTrustedKey({type, code: e.code, trusted: e.isTrusted}));
    });
    await installMenuInput(page);
    page.on('pageerror', error => errors.push(error.message));
    await boot(page, server); return page;
  };
  try {
    checkBudget();
    resourceGuard = setInterval(() => {
      try { checkBudget(); }
      catch (error) {
        errors.push(error.message); clearInterval(resourceGuard);
        process.exitCode = 1; context?.close().catch(() => {});
      }
    }, 1000);
    watchdog = setTimeout(() => {process.exitCode = 1; context?.close().catch(() => {});}, 360000);
    let page = await launch(); await save(page);
    await page.goto('about:blank'); await context.close(); context = null;
    page = await launch(); report.restored = await settings(page); report.reloaded_card = await card(page, 'reloaded');
    assert.deepEqual(report.restored, report.changed, 'normal startup did not restore saved configuration');
    assert(report.reloaded_card.sha256 === report.saved_card.sha256, 'browser restart changed saved card');
    report.race = await race(page);
    checkBudget();
    assert(errors.length === 0, errors.join('; '));
    assert(report.trusted_keyboard_events.length > 0 && report.trusted_keyboard_events.every(e => e.trusted), 'real Playwright keyboard input required');
    report.pass_ = true;
  } finally {
    clearTimeout(watchdog); clearInterval(resourceGuard);
    if (context) await context.close(); server.close();
    fs.writeFileSync(path.join(output,'report.json'), JSON.stringify({...report, errors},null,2)+'\n');
  }
  console.log('Actual browser configuration persistence: PASS');
})().catch(error => {console.error(error.stack); process.exitCode = 1;});
