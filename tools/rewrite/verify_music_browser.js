'use strict';
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const {chromium} = require('../browser/playwright');
const [url, archive, redbook, output, forcedRate] = process.argv.slice(2);
if (!output || !path.resolve(output).startsWith('/tmp/wasm-dd2/')) throw new Error('Use /tmp/wasm-dd2/');
const report = {pass_: false, scope: 'Actual SDL2/WebAudio output and local Redbook playback, not original audio engine parity', comparisons: []};
const errors = [];
const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
function expect(value, message) { if (!value) throw new Error(message); }
const digest = bytes => crypto.createHash('sha256').update(bytes).digest('hex');
function rounded(numerator, denominator) {
  return Math.sign(numerator) * Math.floor((Math.abs(numerator) + Math.floor(denominator / 2)) / denominator);
}
function compare(chunk, source, gain) {
  const frames = source.length / 4;
  const actual = Buffer.alloc(chunk.left.length * 4);
  let phase = chunk.frame * chunk.rate + chunk.fraction;
  let nonzero = 0;
  for (let index = 0; index < chunk.left.length; ++index, phase += 44100) {
    const frame = Math.floor(phase / chunk.rate) % frames, fraction = phase % chunk.rate;
    const next = (frame + 1) % frames;
    for (let channel = 0; channel < 2; ++channel) {
      const sample = source.readInt16LE(frame * 4 + channel * 2);
      const following = source.readInt16LE(next * 4 + channel * 2);
      const expected = rounded(rounded(sample * (chunk.rate - fraction) + following * fraction, chunk.rate) * gain, 256);
      const value = Math.round((channel ? chunk.right : chunk.left)[index] * 32768);
      expect(value === expected, `WebAudio PCM mismatch: ${JSON.stringify({index,channel,expected,value,frame,fraction,chunkFrame:chunk.frame,rate:chunk.rate})}`);
      actual.writeInt16LE(value, index * 4 + channel * 2);
      nonzero += value !== 0;
    }
  }
  return {frames: chunk.left.length, nonzero_samples: nonzero, frame: chunk.frame,
          fraction: chunk.fraction, rate: chunk.rate, gain, pcm_sha256: digest(actual)};
}
async function snapshot(page) {
  return page.evaluate(() => ({track: Module._dd2_application_music_track(),
    phase: Module._dd2_application_music_phase(), frame: Module._dd2_application_music_frame(),
    fraction: Module._dd2_application_music_fraction()}));
}
async function fixed(page, action, label) {
  await action();
  await sleep(150);
  const before = await snapshot(page);
  await sleep(250);
  const after = await snapshot(page);
  expect(before.frame === after.frame && before.fraction === after.fraction, label + ' moved cursor');
  return after;
}
async function advances(page) {
  const before = await snapshot(page);
  await page.waitForFunction(previous => Module._dd2_application_music_frame() !== previous.frame ||
    Module._dd2_application_music_fraction() !== previous.fraction, before, {timeout: 10000});
}
async function attach(page) {
  await page.evaluate(() => {
    const node = Module.SDL2.audio.scriptProcessorNode;
    const process = node.onaudioprocess;
    window.musicCapture = {armed: false, chunks: []};
    node.onaudioprocess = event => {
      const capture = window.musicCapture;
      const before = {frame: Module._dd2_application_music_frame(), fraction: Module._dd2_application_music_fraction(),
        rate: Module._dd2_application_music_rate(), track: Module._dd2_application_music_track()};
      process(event);
      if (!capture.armed || capture.chunks.length >= 16) return;
      const left = Array.from(event.outputBuffer.getChannelData(0));
      const right = Array.from(event.outputBuffer.getChannelData(1));
      if (capture.silence || capture.chunks.length < 2 || left.some(value => value !== 0) || right.some(value => value !== 0))
        capture.chunks.push({...before, left, right});
    };
  });
}
async function verifyOutput(page, source, gain = 256) {
  await page.evaluate(silence => { window.musicCapture.chunks = []; window.musicCapture.silence = silence; window.musicCapture.armed = true; }, gain === 0);
  await page.waitForFunction(() => window.musicCapture.chunks.length >= 8, null, {timeout: 15000});
  const chunks = await page.evaluate(() => { window.musicCapture.armed = false; return window.musicCapture.chunks; });
  const rows = chunks.map(chunk => compare(chunk, source, gain));
  expect(rows.reduce((total, row) => total + row.nonzero_samples, 0) > 0 || gain === 0, 'WebAudio output remained silent');
  report.comparisons.push({track: chunks[0].track, source_sha256: digest(source), callbacks: rows});
}
(async () => {
  const browser = await chromium.launch({headless: true});
  try {
    const page = await browser.newPage();
    if (forcedRate) {
      expect(forcedRate === '48000', 'Unsupported forced browser device rate');
      await page.addInitScript(rate => {
        const BaseContext = window.AudioContext;
        window.AudioContext = class extends BaseContext {
          constructor(options) { super({...options, sampleRate: rate}); }
        };
      }, Number(forcedRate));
    }
    page.on('pageerror', error => errors.push(String(error)));
    await page.goto(url);
    await page.waitForFunction(() => !document.querySelector('#archive').disabled);
    await page.setInputFiles('#archive', archive);
    await page.waitForFunction(() => Module._dd2_application_current_level() === 1 && !document.querySelector('#music-file').disabled);
    await attach(page);
    for (const track of [2, 3]) {
      const file = path.join(redbook, `track${String(track).padStart(2, '0')}.cdda`);
      await page.setInputFiles('#music-file', file);
      await page.locator('#canvas').click();
      await page.waitForFunction(number => Module._dd2_application_music_track() === number &&
        Module.SDL2.audioContext.state === 'running', track);
      await advances(page);
      await verifyOutput(page, fs.readFileSync(file));
    }
    const paused = await fixed(page, () => page.locator('#music-play').click(), 'Transport pause');
    expect(paused.phase === 3, 'Music pause did not change phase');
    await page.locator('#music-play').click(); await advances(page);
    await fixed(page, () => page.locator('#archive').focus(), 'Canvas blur');
    await page.locator('#canvas').click(); await advances(page);
    await page.selectOption('#view', '2');
    await fixed(page, () => page.locator('#pause').click(), 'Game pause');
    await page.locator('#pause').click(); await advances(page);
    // A malformed C-level load must preserve the selected/playing source.
    const preserved = await page.evaluate(() => {
      const before = Module._dd2_application_music_track();
      Module.FS.writeFile('/Music.cdda', new Uint8Array([1, 2, 3]));
      const result = Module._dd2_application_load_music(4);
      Module.FS.unlink('/Music.cdda');
      return !result && Module._dd2_application_music_track() === before && Module._dd2_application_music_phase() === 2;
    });
    expect(preserved, 'Bad load changed current music');
    await advances(page);
    // Short independent stereo pattern exercises repeated wrap and replacement.
    const pattern = Buffer.alloc(97 * 4);
    for (let frame = 0; frame < 97; ++frame) {
      pattern.writeInt16LE(frame * 317 - 14000, frame * 4);
      pattern.writeInt16LE(9000 - frame * 173, frame * 4 + 2);
    }
    await page.selectOption('#view', '0');
    await page.setInputFiles('#music-file', {name: 'track19.cdda', mimeType: 'application/octet-stream', buffer: pattern});
    await page.locator('#canvas').click();
    await page.waitForFunction(() => Module._dd2_application_music_track() === 19);
    await verifyOutput(page, pattern);
    await page.locator('#music-gain').evaluate(element => { element.value = '128'; element.dispatchEvent(new Event('input', {bubbles: true})); });
    await verifyOutput(page, pattern, 128);
    await page.locator('#music-gain').evaluate(element => { element.value = '0'; element.dispatchEvent(new Event('input', {bubbles: true})); });
    await verifyOutput(page, pattern, 0);
    await advances(page); // muting must not freeze the transport
    await page.keyboard.press('F10');
    await page.waitForFunction(() => Module._dd2_application_music_phase() === 3, null, {timeout: 10000});
    // Hold an actual asynchronous local-file read across close/reopen. The
    // completed read belongs to the old application and must be discarded.
    await page.evaluate(() => {
      const read = File.prototype.arrayBuffer;
      File.prototype.arrayBuffer = function() {
        if (this.name !== 'track18.cdda') return read.call(this);
        return read.call(this).then(bytes => new Promise(resolve => { window.releaseMusicRead = () => resolve(bytes); }));
      };
    });
    await page.setInputFiles('#music-file', {name: 'track18.cdda', mimeType: 'application/octet-stream', buffer: pattern});
    await page.waitForFunction(() => typeof window.releaseMusicRead === 'function');
    await page.keyboard.press('Escape');
    await page.waitForFunction(() => Module._dd2_application_current_level() === 0 && document.querySelector('#music-file').disabled);
    await page.setInputFiles('#archive', archive);
    await page.waitForFunction(() => Module._dd2_application_current_level() === 1);
    await page.evaluate(() => window.releaseMusicRead());
    await page.waitForFunction(() => !document.querySelector('#music-file').disabled);
    expect((await snapshot(page)).phase === 0, 'Reopen retained stale/pending music');
    expect(await page.locator('#music-gain').inputValue() === '256', 'Reopen volume UI differs from mixer');
    await attach(page);
    await page.setInputFiles('#music-file', {name: 'track19.cdda', mimeType: 'application/octet-stream', buffer: pattern});
    await page.locator('#canvas').click();
    await page.waitForFunction(() => Module._dd2_application_music_track() === 19);
    await verifyOutput(page, pattern);
    await page.keyboard.press('Escape');
    await page.waitForFunction(() => Module._dd2_application_current_level() === 0);
    expect(!errors.length, 'Browser errors: ' + errors.join('\n'));
    report.controls = ['actual local original tracks 2/3', 'transport pause/resume', 'focus freeze/resume',
      'game pause/resume', 'malformed load rollback', 'repeat wrap', 'gain/mute', 'F10', 'close/reopen', 'pending local read cancellation'];
    report.pass_ = true;
  } finally {
    report.errors = errors;
    fs.writeFileSync(path.join(output, `browser-music-${forcedRate || 'default'}.json`), JSON.stringify(report, null, 2) + '\n');
    await browser.close();
  }
})().catch(error => { console.error(error); process.exitCode = 1; });
