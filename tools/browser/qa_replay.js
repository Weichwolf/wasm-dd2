// Real practice -> drive with changing controls -> Retire/Yes -> View Replay.
// Read-only observations; no engine writes. This is functional browser QA,
// separate from the original-x86 replay component comparison.
const assert = require('assert'), fs = require('fs'), path = require('path');
const {serve, boot, menuLabel, waitRace, rd, chromium} = require('./felib');
const {installMenuInput, tap} = require('./menu_input');
const output = path.resolve(process.argv[3]);
assert(output.startsWith('/tmp/wasm-dd2/'), 'verification output must use /tmp/wasm-dd2');
fs.mkdirSync(output, {recursive: false});
const report = {scope: 'Real browser practice recording and View Replay with changing keyboard controls; no full original gameplay/audio parity claim', pass: false};
const input = (page, code, down) => page.evaluate(({code, down}) => window.dispatchEvent(new KeyboardEvent(down ? 'keydown' : 'keyup', {code})), {code, down});
(async () => {
  const server = serve(path.resolve(process.argv[2] || 'web/dd2'));
  await new Promise(resolve => server.listen(0, resolve));
  let browser, watchdog, page; const errors = [];
  try {
    browser = await chromium.launch({args: ['--no-sandbox']});
    watchdog = setTimeout(() => { process.exitCode = 1; browser.close().catch(() => {}); }, 240000);
    page = await browser.newPage();
    page.on('pageerror', error => {errors.push(error.message); console.error('Browser runtime:', error.message);});
    page.on('crash', () => errors.push('renderer crash'));
    await installMenuInput(page);
    await page.addInitScript(() => {
      window.__replayFrames = {live: {}, replay: {}};
      const present = CanvasRenderingContext2D.prototype.putImageData;
      CanvasRenderingContext2D.prototype.putImageData = function(...args) {
        const result = present.apply(this, args);
        if (this.canvas.id === 'canvas' && typeof HEAP32 !== 'undefined' &&
            HEAP32[0x7746ac >> 2] === 0 && HEAP32[0x7746c0 >> 2] > 0 && HEAP32[0x784298 >> 2] < 1 &&
            HEAP32[0x936ff4 >> 2] >= 1 && HEAP32[0x936ff4 >> 2] <= 10) {
          const replay = HEAP32[0x467074 >> 2] !== 0, ticks = HEAP32[0x7746c0 >> 2];
          const positions = Array.from({length: 20}, (_, car) => Array.from({length: 4}, (_, i) => HEAP32[(0x78a744 + car * 0x27c + i * 4) >> 2]));
          window.__replayFrames[replay ? 'replay' : 'live'][ticks] = {ticks, cf: HEAP32[0x462ff0 >> 2], positions,
            steering: new DataView(HEAPU8.buffer).getInt32(0x792a82, true), pedal: new DataView(HEAPU8.buffer).getInt32(0x792a86, true),
            cursor: HEAPU32[0x9392b4 >> 2], repeat: HEAPU32[0x46707c >> 2]};
        }
        return result;
      };
    });
    await boot(page, server);
    await tap(page, 'ArrowDown'); await tap(page, 'ArrowDown');
    assert((await menuLabel(page)).includes('Go!'), 'Go selection failed');
    await tap(page, 'Enter', 1000);
    assert((await waitRace(page)).launched, 'practice did not launch');
    await page.waitForFunction(() => HEAP32[0x784298 >> 2] < 1, null, {timeout: 15000});
    await input(page, 'KeyA', true); await page.waitForTimeout(1200);
    await input(page, 'ArrowLeft', true); await page.waitForTimeout(800); await input(page, 'ArrowLeft', false);
    await input(page, 'ArrowRight', true); await page.waitForTimeout(800); await input(page, 'ArrowRight', false);
    await page.waitForTimeout(1200); await input(page, 'KeyA', false);
    await page.waitForTimeout(400);
    report.live = await page.evaluate(() => Object.values(window.__replayFrames.live));
    console.log('Live recording:', report.live.length, 'observed frames');
    assert(report.live.some(row => row.pedal > 0) && report.live.some(row => row.steering !== 0), 'live driving controls were not recorded');
    await tap(page, 'Escape');
    for (const code of ['ArrowDown', 'ArrowDown', 'ArrowDown', 'Enter', 'ArrowUp', 'Enter']) await tap(page, code);
    await page.waitForFunction(() => HEAP32[0x467074 >> 2] === 0 && HEAP32[0x7746ac >> 2] === 1, null, {timeout: 25000});
    const label = await rd(page, 0x46a508);
    console.log('Practice result action:', label);
    assert(label.includes('View Replay'), 'Practice View Replay action missing');
    report.script = await page.evaluate(() => ({cursor: HEAPU32[0x9392b4 >> 2], end: HEAPU32[0x9392c4 >> 2],
      words: Array.from(HEAPU16.subarray(0x9376b0 >> 1, (HEAPU32[0x9392c4 >> 2] + 2) >> 1))}));
    assert(report.script.words.length > 4 && report.script.words.length <= 0xe00, 'recorded script extent invalid');
    assert(report.script.words.every(word => (word & 0x7ff) > 0), 'recorded script contains zero-length gap packets');
    await tap(page, 'Enter', 600);
    await page.waitForFunction(() => HEAP32[0x467074 >> 2] === 1 && HEAP32[0x7746ac >> 2] === 0 && HEAP32[0x7746c0 >> 2] > 0, null, {timeout: 25000});
    await page.waitForFunction(() => HEAP32[0x467074 >> 2] === 0 && HEAP32[0x7746ac >> 2] === 1, null, {timeout: 25000});
    report.replay = await page.evaluate(() => Object.values(window.__replayFrames.replay));
    console.log('Playback:', report.replay.length, 'observed frames');
    assert(report.replay.length >= 10, 'replay did not advance');
    assert(report.replay.some(row => row.pedal > 0), 'replay never applied recorded acceleration');
    assert(report.replay.some(row => row.steering !== 0), 'replay never applied recorded steering');
    const live = new Map(report.live.map(row => [row.ticks, row]));
    const compared = report.replay.filter(row => live.has(row.ticks));
    report.commonTicks = compared.length;
    report.worldDifferences = compared.filter(row => JSON.stringify(row.positions) !== JSON.stringify(live.get(row.ticks).positions)).map(row => row.ticks);
    report.returnedAction = await rd(page, 0x46a508);
    assert(report.returnedAction.includes('View Replay'), 'natural playback completion did not return to Practice Over');
    report.errors = errors; assert.deepEqual(errors, [], 'browser runtime errors');
    report.pass = true;
    console.log('PASS actual recording/View Replay controls; common phases:', report.commonTicks, 'world-state differences:', report.worldDifferences.length);
  } catch (error) {
    if (page) {
      report.errors = errors;
      report.lastState = await page.evaluate(() => ({level: HEAP32[0x936ff4 >> 2], cf: HEAP32[0x462ff0 >> 2],
        ticks: HEAP32[0x7746c0 >> 2], quit: HEAP32[0x7746ac >> 2], replay: HEAP32[0x467074 >> 2],
        clutCacheByte: HEAPU8[0x460005]})).catch(() => null);
      report.replay = await page.evaluate(() => Object.values(window.__replayFrames.replay)).catch(() => []);
    }
    report.error = error.message; throw error;
  } finally {
    clearTimeout(watchdog);
    fs.writeFileSync(path.join(output, 'report.json'), JSON.stringify(report, null, 2) + '\n');
    if (browser) await browser.close();
    await new Promise(resolve => server.close(resolve));
  }
})().catch(error => {console.error(error); process.exitCode = 1;});
