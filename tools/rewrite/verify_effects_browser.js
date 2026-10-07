'use strict';
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const {chromium} = require('../browser/playwright');
const [url, archive, output, forcedRate] = process.argv.slice(2);
if (!output || !path.resolve(output).startsWith('/tmp/wasm-dd2/')) throw new Error('Use /tmp/wasm-dd2/');
const report = {pass_: false, scope: 'Actual game events and SDL/WebAudio effects from the original bank; no original audio-engine parity', callbacks: []};
const errors = [];
const sleep = ms => new Promise(resolve => setTimeout(resolve, ms));
const expect = (condition, message) => { if (!condition) throw new Error(message); };
const digest = bytes => crypto.createHash('sha256').update(bytes).digest('hex');
const rounded = (value, divisor) => Math.sign(value) * Math.floor((Math.abs(value) + Math.floor(divisor / 2)) / divisor);
function sounds() {
  const bytes = fs.readFileSync(archive);
  let bank;
  for (let offset = 0; offset < 0x2808; offset += 24) {
    const name = bytes.subarray(offset, offset + 18).toString('ascii').split('\0')[0];
    if (name === 'VAGS\\BANK1.SBK') {
      const start = bytes.readUInt16LE(offset + 18) * 2048;
      bank = bytes.subarray(start, start + bytes.readUInt32LE(offset + 20)); break;
    }
  }
  expect(bank, 'Missing original effect bank');
  report.bank_sha256 = digest(bank);
  const result = [];
  for (let index = 0; index < bank.readUInt32LE(12); ++index) {
    const record = 16 + index * 28;
    const wave = bank.subarray(bank.readUInt32LE(record), bank.readUInt32LE(record) + bank.readUInt32LE(record + 4));
    let data, rate;
    for (let offset = 12; offset < wave.length;) {
      const tag = wave.subarray(offset, offset + 4).toString('ascii'), length = wave.readUInt32LE(offset + 4);
      if (tag === 'fmt ') {
        expect(wave.readUInt16LE(offset + 8) === 1 && wave.readUInt16LE(offset + 10) === 1 && wave.readUInt16LE(offset + 22) === 8, 'Unsupported original effect PCM');
        rate = wave.readUInt32LE(offset + 12);
      } else if (tag === 'data') data = wave.subarray(offset + 8, offset + 8 + length);
      offset += 8 + length + (length % 2);
    }
    expect(data && rate, 'Invalid original WAVE');
    result.push({data, rate});
  }
  return result;
}
function compare(chunk, bank) {
  const bytes = Buffer.alloc(chunk.left.length * 4);
  for (let frame = 0; frame < chunk.left.length; ++frame) {
    for (let channel = 0; channel < 2; ++channel) {
      let mixed = 0;
      for (const voice of chunk.voices) {
        const [sample, playing, start, fraction, frequency, gain, pan, loop] = voice;
        if (!playing) continue;
        const pcm = bank[sample].data;
        const phase = start * chunk.rate + fraction + frame * frequency;
        let index = Math.floor(phase / chunk.rate);
        if (!loop && index >= pcm.length) continue;
        index %= pcm.length;
        const next = index + 1 < pcm.length ? index + 1 : loop ? 0 : index;
        const position = phase % chunk.rate;
        const first = (pcm[index] - 128) * 256, second = (pcm[next] - 128) * 256;
        const interpolated = rounded(first * (chunk.rate - position) + second * position, chunk.rate);
        const attenuation = channel === 0 ? (pan > 0 ? 256 - pan : 256) : (pan < 0 ? 256 + pan : 256);
        mixed += rounded(interpolated * gain * attenuation, 256 * 256);
      }
      const expected = Math.max(-32768, Math.min(32767, mixed));
      const actual = Math.round((channel ? chunk.right : chunk.left)[frame] * 32768);
      expect(actual === expected, 'Effects PCM mismatch: ' + JSON.stringify({frame, channel, expected, actual, rate: chunk.rate, voices: chunk.voices}));
      bytes.writeInt16LE(actual, frame * 4 + channel * 2);
    }
  }
  return {rate: chunk.rate, frames: chunk.left.length, voices: chunk.voices, pcm_sha256: digest(bytes), nonzero: bytes.some(value => value !== 0)};
}
async function cursor(page) {
  return page.evaluate(() => [Module._dd2_application_effect_voice(2), Module._dd2_application_effect_voice(3)]);
}
async function frozen(page, action) {
  await page.evaluate(() => { window.effectsCapture.armed = false; });
  await action(); await sleep(120);
  const before = await cursor(page); await sleep(200);
  expect(JSON.stringify(before) === JSON.stringify(await cursor(page)), 'Paused engine cursor moved');
}
async function verifyCaptured(page, bank) {
  const chunks = await page.evaluate(() => { const value = window.effectsCapture.chunks; window.effectsCapture.chunks = []; return value; });
  expect(chunks.length > 0, 'No actual WebAudio callbacks');
  report.callbacks.push(...chunks.map(chunk => compare(chunk, bank)));
}
(async () => {
  const bank = sounds();
  const browser = await chromium.launch({headless: true});
  try {
    const page = await browser.newPage();
    if (forcedRate) await page.addInitScript(rate => {
      const Base = window.AudioContext;
      window.AudioContext = class extends Base { constructor(options) { super({...options, sampleRate: rate}); } };
    }, Number(forcedRate));
    page.on('pageerror', error => errors.push(String(error)));
    await page.goto(url);
    await page.waitForFunction(() => !document.querySelector('#archive').disabled);
    await page.setInputFiles('#archive', archive);
    await page.waitForFunction(() => Module._dd2_application_current_level() === 1);
    await page.selectOption('#level', '8');
    await page.locator('#canvas').click();
    await page.evaluate(() => {
      window.effectsCapture = {armed: true, chunks: [], counts: {}};
      const node = Module.SDL2.audio.scriptProcessorNode, process = node.onaudioprocess;
      node.onaudioprocess = event => {
        const capture = window.effectsCapture;
        const voices = Array.from({length: 4}, (_, channel) => Array.from({length: 8}, (_, field) => Module._dd2_application_effect_voice(channel * 8 + field)));
        const rate = Module._dd2_application_music_rate();
        process(event);
        if (!capture.armed || capture.chunks.length >= 64) return;
        const active = voices.filter(voice => voice[1]).map(voice => voice[0]);
        const key = active.join(',') + ':' + voices.map(voice => voice[5]).join(',');
        if (!active.length || (capture.counts[key] || 0) >= 3) return;
        capture.counts[key] = (capture.counts[key] || 0) + 1;
        capture.chunks.push({voices, rate, left: Array.from(event.outputBuffer.getChannelData(0)), right: Array.from(event.outputBuffer.getChannelData(1))});
      };
    });
    await page.selectOption('#view', '3');
    await page.waitForFunction(() => Module._dd2_application_race_phase() === 1 && Module._dd2_application_sound_cues(4) === 1, null, {timeout: 15000});
    const cueCounts = await page.evaluate(() => Array.from({length: 5}, (_, cue) => Module._dd2_application_sound_cues(cue)));
    expect(cueCounts.slice(1).every(count => count === 1), 'Countdown cues duplicated/missing: ' + cueCounts);
    await verifyCaptured(page, bank);
    const observed = new Set(report.callbacks.flatMap(row => row.voices.filter(voice => voice[1]).map(voice => voice[0])));
    for (const sample of [0, 11, 10, 9, 8]) expect(observed.has(sample), 'Countdown sample not heard: ' + sample);
    const idle = await page.evaluate(() => Module._dd2_application_effect_voice(4));
    await page.keyboard.down('s');
    await page.waitForFunction(previous => Module._dd2_application_effect_voice(4) > previous, idle);
    await page.waitForFunction(() => Module._dd2_application_sound_cues(0) > 0, null, {timeout: 15000});
    await sleep(200); await page.keyboard.up('s');
    await verifyCaptured(page, bank);
    expect(report.callbacks.some(row => row.voices.some(voice => voice[1] && voice[0] === 3)), 'Physical impact was not heard');
    await frozen(page, () => page.locator('#pause').click());
    await page.locator('#pause').click();
    await frozen(page, () => page.locator('#archive').focus());
    await page.locator('#canvas').click();
    await page.locator('#effects-gain').evaluate(element => { element.value = '0'; element.dispatchEvent(new Event('input', {bubbles: true})); });
    const before = await cursor(page);
    await page.evaluate(() => { window.effectsCapture.counts = {}; window.effectsCapture.armed = true; });
    await sleep(180); await verifyCaptured(page, bank);
    expect(JSON.stringify(before) !== JSON.stringify(await cursor(page)), 'Muted engine stopped its cursor');
    const muted = report.callbacks.slice(-3);
    expect(muted.every(row => !row.nonzero), 'Muted effects are audible');
    await page.evaluate(() => { window.effectsCapture.armed = false; });
    await page.locator('#pause').click();
    await page.locator('#reset').click();
    expect((await page.evaluate(() => Array.from({length: 5}, (_, cue) => Module._dd2_application_sound_cues(cue)))).every(count => count === 0), 'Reset kept stale events');
    expect((await page.evaluate(() => Array.from({length: 4}, (_, channel) => Module._dd2_application_effect_voice(channel * 8 + 1)))).every(value => value === 0), 'Reset kept stale voices');
    await page.selectOption('#view', '0');
    await page.keyboard.press('Escape');
    await page.waitForFunction(() => Module._dd2_application_current_level() === 0);
    expect(!errors.length, errors.join('\n'));
    report.cues = cueCounts;
    report.controls = ['source-timed countdown', 'real reverse throttle changes motor pitch', 'physical impacts', 'pause/focus freezing', 'effects mute preserves clock', 'reset removes stale effects', 'close'];
    report.pass_ = true;
  } finally {
    report.errors = errors;
    fs.writeFileSync(path.join(output, `effects-browser-${forcedRate || 'default'}.json`), JSON.stringify(report, null, 2) + '\n');
    await browser.close();
  }
})().catch(error => { console.error(error); process.exitCode = 1; });
