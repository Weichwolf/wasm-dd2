// Real practice -> Save Replay -> named card file -> reload -> File Manager.
// Read-only engine/filesystem observations and real navigation through the key
// bridge; no state/file writes or forced persistence calls in the test.
const assert = require('assert'), fs = require('fs'), path = require('path');
const {createHash} = require('crypto');
const {serve, boot, menuLabel, waitRace, rd, chromium} = require('./felib');
const {installMenuInput, tap} = require('./menu_input');
fs.mkdirSync('/tmp/wasm-dd2', {recursive: true});
const output = path.resolve(process.argv[3] || `/tmp/wasm-dd2/browser-replay-save-${Date.now()}-${process.pid}`);
assert(output.startsWith('/tmp/wasm-dd2/'), 'verification output must use /tmp/wasm-dd2');
const parent = fs.realpathSync(path.dirname(output));
assert(parent === '/tmp/wasm-dd2' || parent.startsWith('/tmp/wasm-dd2/'), 'verification parent must remain inside /tmp/wasm-dd2');
fs.mkdirSync(output, {recursive: false});
const report = {scope: 'Actual browser replay saving, cancelled/confirmed overwrite, full-card persistence, loading, natural playback, configuration restoration and cancelled/confirmed deletion; no complete original video/audio parity claim', pass: false};
const metadata = page => page.evaluate(() => ({car: HEAP32[0x467400 >> 2], mode: HEAP32[0x4673f8 >> 2],
  type: HEAP32[0x4673f4 >> 2], season: HEAP32[0x93dec0 >> 2], level: HEAP32[0x936ff4 >> 2],
  replayLevel: HEAP32[0x9392bc >> 2], pad: HEAP32[0x467078 >> 2], end: HEAPU32[0x9392c4 >> 2],
  cars: HEAP32[0x46765c >> 2], replay: HEAP32[0x467074 >> 2], quit: HEAP32[0x7746ac >> 2], ticks: HEAP32[0x7746c0 >> 2],
  fileMode: HEAP32[0x93a318 >> 2], fileSlot: HEAP32[0x774680 >> 2]}));
const card = page => page.evaluate(async () => {
  const disk = FS.readFile('/SaveGames');
  const readName = a => {let s = ''; for (let i = 0; i < 28 && disk[a+i]; i++) s += String.fromCharCode(disk[a+i]); return s;};
  const slots = [];
  for (let i = 0; i < 15; i++) {
    const base = i*0x200;
    if (disk[base] === 1) {
      const payload = 0x2000+i*0x2000;
      slots.push({index: i, name: readName(base+4), packed: Array.from(disk.subarray(payload, payload+18+0x1c00+20))});
    }
  }
  const digest = await crypto.subtle.digest('SHA-256', disk);
  return {bytes: disk.length, sha256: Array.from(new Uint8Array(digest), b => b.toString(16).padStart(2, '0')).join(''),
    slots, engineMatchesFile: Array.from(disk).every((value, i) => value === HEAPU8[0x754460+i])};
});
(async () => {
  const server = serve(path.resolve(process.argv[2] || 'web/dd2'));
  await new Promise(resolve => server.listen(0, resolve));
  let browser, watchdog, page; const errors = [];
  try {
    browser = await chromium.launch({args: ['--no-sandbox']});
    watchdog = setTimeout(() => {process.exitCode = 1; browser.close().catch(() => {});}, 300000);
    page = await browser.newPage();
    page.on('pageerror', error => {errors.push(error.message); console.error('Browser runtime:', error.message);});
    page.on('crash', () => errors.push('renderer crash'));
    await installMenuInput(page);
    await page.addInitScript(() => {
      window.__loadedReplay = [];
      const present = CanvasRenderingContext2D.prototype.putImageData;
      CanvasRenderingContext2D.prototype.putImageData = function(...args) {
        const result = present.apply(this, args);
        if (this.canvas.id === 'canvas' && typeof HEAP32 !== 'undefined' && HEAP32[0x467074 >> 2] === 1 &&
            HEAP32[0x7746ac >> 2] === 0 && HEAP32[0x7746c0 >> 2] > 0) {
          window.__loadedReplay.push({ticks: HEAP32[0x7746c0 >> 2], car: HEAP32[0x467400 >> 2],
            mode: HEAP32[0x4673f8 >> 2], type: HEAP32[0x4673f4 >> 2], level: HEAP32[0x936ff4 >> 2],
            end: HEAPU32[0x9392c4 >> 2], pedal: new DataView(HEAPU8.buffer).getInt32(0x792a86, true),
            script: window.__loadedReplay.length ? undefined : Array.from(HEAPU8.subarray(0x9376b0, 0x9392b0)),
            order: window.__loadedReplay.length ? undefined : Array.from(HEAPU8.subarray(0x795c28, 0x795c3c))});
        }
        return result;
      };
    });
    await boot(page, server);
    report.initial = await card(page);
    // Choose a different, nonzero car through the actual selector.
    await tap(page, 'ArrowRight'); assert((await menuLabel(page)).includes('Select Car'), 'car selector missing');
    await tap(page, 'Enter'); await tap(page, 'ArrowRight'); await tap(page, 'Enter');
    report.selected = await metadata(page);
    assert(report.selected.car > 0, 'nonzero car required to expose magic DWORD overwrite');
    await tap(page, 'ArrowLeft'); await tap(page, 'ArrowDown'); await tap(page, 'ArrowDown');
    assert((await menuLabel(page)).includes('Go!'), 'Go selection failed');
    await tap(page, 'Enter', 1000); assert((await waitRace(page)).launched, 'practice did not launch');
    await page.waitForFunction(() => HEAP32[0x784298 >> 2] < 1, null, {timeout: 15000});
    await page.keyboard.down('a'); await page.waitForTimeout(3000); await page.keyboard.up('a');
    await tap(page, 'Escape');
    for (const code of ['ArrowDown', 'ArrowDown', 'ArrowDown', 'Enter', 'ArrowUp', 'Enter']) await tap(page, code);
    assert((await rd(page, 0x46a508)).includes('View Replay'), 'Practice Over missing');
    report.recorded = await metadata(page);
    report.script = await page.evaluate(() => Array.from(HEAPU8.subarray(0x9376b0, 0x9392b0)));
    report.order = await page.evaluate(() => Array.from(HEAPU8.subarray(0x795c28, 0x795c3c)));
    await tap(page, 'ArrowRight'); assert((await rd(page, 0x46a508)).includes('Save Replay'), 'Save Replay missing');
    await tap(page, 'Enter'); assert((await metadata(page)).fileMode === 5, 'replay file browser not entered');
    console.log('Save browser entered', await metadata(page));
    await tap(page, 'Enter');
    assert((await metadata(page)).fileSlot >= 0, 'no selected save slot');
    await tap(page, 'Enter');
    // Name "A", then choose the tick on the third row.
    for (const code of ['Enter', 'ArrowDown', 'ArrowDown', 'ArrowRight', 'Enter']) await tap(page, code);
    await page.waitForFunction(() => FS.readFile('/SaveGames')[0] === 1, null, {timeout: 15000});
    report.saved = await card(page);
    const saved = report.saved.slots.find(slot => slot.name === 'A');
    assert(saved, 'named replay file was not saved');
    const words = new DataView(Uint8Array.from(saved.packed).buffer);
    assert(words.getUint16(0, true) === 0x2020, 'saved replay magic incorrect');
    assert(words.getInt16(2, true) === report.recorded.car, 'saved replay car corrupted');
    assert(words.getUint32(4, true) === report.recorded.end, 'saved replay end pointer corrupted');
    assert(words.getInt16(8, true) === report.recorded.mode && words.getInt16(10, true) === report.recorded.type,
      'saved replay race configuration corrupted');
    assert(words.getInt16(12, true) === report.recorded.season && words.getInt16(14, true) === report.recorded.replayLevel,
      'saved replay season/level corrupted');
    assert.deepEqual(saved.packed.slice(18, 18+0x1c00), report.script, 'saved replay script differs');
    assert.deepEqual(saved.packed.slice(18+0x1c00), report.order, 'saved car order differs');
    assert(report.saved.engineMatchesFile, 'saved card disk bytes differ from engine');
    console.log('Saved nonzero-car replay', saved.name, words.getInt16(2, true));
    report.firstSaved = report.saved;
    await tap(page, 'Enter'); await tap(page, 'Enter');
    assert((await rd(page, 0x4672ac)).includes('Overwrite File'), 'overwrite confirmation missing');
    await tap(page, 'Escape'); report.cancelledOverwrite = await card(page);
    assert.deepEqual(report.cancelledOverwrite, report.firstSaved, 'cancelled overwrite changed card');
    for (const code of ['Enter', 'Enter', 'ArrowLeft', 'Enter']) await tap(page, code);
    // Existing A: backspace, select B, then confirm the tick.
    for (const code of ['ArrowDown', 'ArrowDown', 'Enter', 'ArrowUp', 'ArrowUp', 'ArrowRight', 'Enter', 'ArrowDown', 'ArrowDown', 'ArrowRight', 'Enter']) await tap(page, code);
    await page.waitForFunction(() => FS.readFile('/SaveGames')[4] === 66 && FS.readFile('/SaveGames')[5] === 0, null, {timeout: 15000});
    report.saved = await card(page);
    assert(report.saved.slots.length === 1 && report.saved.slots[0].name === 'B', 'overwrite did not rename the same card entry');
    assert.deepEqual(report.saved.slots[0].packed, saved.packed, 'overwrite changed the replay payload');
    // Normal navigation away invokes the production pagehide/unload persistence.
    // The test never calls syncfs or writes either the card or engine memory.
    await page.goto('about:blank');
    await boot(page, server);
    report.reloaded = await card(page);
    assert.deepEqual(report.reloaded, report.saved, 'saved replay did not survive normal page navigation');
    report.beforeLoading = await metadata(page);
    for (const code of ['ArrowRight', 'ArrowRight', 'ArrowRight']) await tap(page, code);
    assert((await menuLabel(page)).includes('File Manager'), 'File Manager selection failed');
    await tap(page, 'Enter'); assert((await metadata(page)).fileMode === 0, 'load browser not entered');
    await tap(page, 'Enter'); await tap(page, 'Enter', 600);
    await page.waitForFunction(() => window.__loadedReplay.length > 0, null, {timeout: 25000});
    await page.waitForFunction(() => HEAP32[0x467074 >> 2] === 0 && HEAP32[0x7746ac >> 2] === 1, null, {timeout: 25000});
    report.playback = await page.evaluate(() => window.__loadedReplay);
    assert(report.playback.length >= 10, 'loaded playback did not advance');
    assert(report.playback.every(row => row.car === report.recorded.car && row.mode === report.recorded.mode &&
      row.type === report.recorded.type && row.level === report.recorded.replayLevel && row.end === report.recorded.end), 'loaded replay metadata differs');
    assert(report.playback.some(row => row.pedal > 0), 'loaded replay did not apply recorded acceleration');
    assert.deepEqual(report.playback[0].script, report.script, 'loaded script differs');
    assert.deepEqual(report.playback[0].order, report.order, 'loaded order differs');
    // Play_Game clears replay before View_Frontend_Replay finishes reloading
    // the frontend. Wait for the caller's actual configuration restoration.
    await page.waitForFunction(before => HEAP32[0x467400 >> 2] === before.car &&
      HEAP32[0x4673f8 >> 2] === before.mode && HEAP32[0x4673f4 >> 2] === before.type &&
      HEAP32[0x46765c >> 2] === before.cars, report.beforeLoading, {timeout: 15000});
    report.returned = await metadata(page);
    for (const field of ['car', 'mode', 'type', 'cars'])
      assert(report.returned[field] === report.beforeLoading[field], 'frontend configuration was not restored: ' + field);
    assert((await menuLabel(page)).includes('File Manager'), 'loaded playback did not return to File Manager');
    // Exercise a second write to the already persisted/open card, including
    // cancelling deletion before confirming it through the real file browser.
    await tap(page, 'Enter'); await tap(page, 'ArrowRight');
    await tap(page, 'Enter'); await tap(page, 'Enter');
    assert((await rd(page, 0x4672ac)).includes('Delete File'), 'delete confirmation missing');
    await tap(page, 'Escape');
    report.cancelledDeletion = await card(page);
    assert.deepEqual(report.cancelledDeletion, report.saved, 'cancelled deletion changed the saved card');
    await tap(page, 'Enter'); await tap(page, 'Enter');
    assert((await rd(page, 0x4672ac)).includes('Delete File'), 'second delete confirmation missing');
    await tap(page, 'ArrowLeft'); await tap(page, 'Enter');
    await page.waitForFunction(() => FS.readFile('/SaveGames')[0] === 0, null, {timeout: 15000});
    report.deleted = await card(page);
    assert(report.deleted.slots.length === 0 && report.deleted.engineMatchesFile, 'confirmed deletion did not update card disk/RAM');
    await page.goto('about:blank'); await boot(page, server);
    report.deletedReloaded = await card(page);
    assert.deepEqual(report.deletedReloaded, report.deleted, 'confirmed deletion did not survive page navigation');
    report.errors = errors; assert.deepEqual(errors, [], 'browser runtime errors');
    report.pass = true;
    console.log('PASS Save Replay/overwrite, exact full-card persistence, loaded playback/return and cancelled/confirmed deletion');
  } catch (error) {
    report.error = error.message; report.errors = errors;
    if (page) {
      report.lastState = await metadata(page).catch(() => null);
      report.lastLabels = {main: await menuLabel(page), practice: await rd(page, 0x46a508), file: await rd(page, 0x467284), name: await rd(page, 0x4672ac)};
      await page.screenshot({path: path.join(output, 'failure.png')}).catch(() => {});
    }
    throw error;
  } finally {
    clearTimeout(watchdog);
    if (report.pass) {
      const summarize = bytes => ({bytes: bytes.length, sha256: createHash('sha256').update(Buffer.from(bytes)).digest('hex')});
      report.script = summarize(report.script);
      for (const name of ['initial', 'firstSaved', 'cancelledOverwrite', 'saved', 'reloaded', 'cancelledDeletion', 'deleted', 'deletedReloaded']) {
        for (const slot of report[name].slots) {
          slot.header = slot.packed.slice(0, 18);
          slot.payload = summarize(slot.packed);
          delete slot.packed;
        }
      }
      for (const row of report.playback) if (row.script) row.script = summarize(row.script);
    }
    fs.writeFileSync(path.join(output, 'report.json'), JSON.stringify(report, null, 2)+'\n');
    if (browser) await browser.close();
    await new Promise(resolve => server.close(resolve));
  }
})().catch(error => {console.error(error); process.exitCode = 1;});
