'use strict';
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const {chromium} = require('../browser/playwright');
const [url, output] = process.argv.slice(2);
if (!output || !path.resolve(output).startsWith('/tmp/wasm-dd2/')) throw new Error('Use /tmp/wasm-dd2/');
const digest = data => crypto.createHash('sha256').update(data).digest('hex');
const pause = ms => new Promise(resolve => setTimeout(resolve, ms));
const report = {pass_: false, scope: 'Actual authored vehicle canvas/input only; full standalone gameplay/60 FPS remain pending', comparisons: [], errors: []};
async function pixels(page) {
  const encoded = await page.evaluate(() => {
    const rgba = document.querySelector('#canvas').getContext('2d').getImageData(0, 0, 640, 360).data;
    const rgb = new Uint8Array(640 * 360 * 3);
    for (let src = 0, dst = 0; src < rgba.length; src += 4) {
      rgb[dst++] = rgba[src]; rgb[dst++] = rgba[src + 1]; rgb[dst++] = rgba[src + 2];
    }
    let data = '';
    for (let index = 0; index < rgb.length; index += 32768) data += String.fromCharCode(...rgb.subarray(index, index + 32768));
    return btoa(data);
  });
  return Buffer.from(encoded, 'base64');
}
function compare(left, right) {
  if (left.length !== right.length) throw new Error('Image extent differs');
  let changed = 0, error = 0;
  for (let index = 0; index < left.length; index += 3) {
    let different = false;
    for (let channel = 0; channel < 3; ++channel) {
      error += Math.abs(left[index + channel] - right[index + channel]);
      different ||= left[index + channel] !== right[index + channel];
    }
    changed += different;
  }
  return {changed_pixels: changed, mean_channel_error: error / left.length,
          pass_: changed === 0,
          expected_sha256: digest(left), actual_sha256: digest(right)};
}
async function match(page, label) {
  const [view, pose] = label.split('-');
  const state = {detail: {full: 0, exterior: 1, npc: 2, cockpit: 0}[view],
                 cockpit: Number(view === 'cockpit'), pose: Number(pose === 'steer')};
  await page.waitForFunction(expected => Module._dd2_content_detail() === expected.detail &&
    Module._dd2_content_cockpit() === expected.cockpit && Module._dd2_content_pose() === expected.pose,
    state, {timeout: 15000});
  const file = fs.readFileSync(path.join(output, `wasm-${label}.ppm`));
  const header = Buffer.from('P6\n640 360\n255\n');
  if (!file.subarray(0, header.length).equals(header)) throw new Error('Invalid reference header');
  const expected = file.subarray(header.length);
  const deadline = Date.now() + 15000;
  let result;
  do {
    result = compare(expected, await pixels(page));
    if (result.pass_) { report.comparisons.push({label, ...result}); return; }
    await pause(60);
  } while (Date.now() < deadline);
  await page.locator('#canvas').screenshot({path: path.join(output, 'failed-browser.png')});
  throw new Error(`Canvas ${label}: ${JSON.stringify(result)}`);
}
async function stable(page) {
  let previous, since = Date.now();
  const deadline = Date.now() + 15000;
  while (Date.now() < deadline) {
    const current = digest(await pixels(page));
    if (current !== previous) since = Date.now();
    previous = current;
    if (Date.now() - since >= 1000) return current;
    await pause(60);
  }
  throw new Error('Camera continues moving after release');
}
(async () => {
  const browser = await chromium.launch({headless: true});
  try {
    const page = await browser.newPage({viewport: {width: 900, height: 700}});
    page.on('pageerror', error => report.errors.push(String(error)));
    page.on('response', response => { if (response.status() >= 400) report.errors.push(`${response.status()} ${response.url()}`); });
    page.on('console', message => { if (message.type() === 'error') report.errors.push(message.text()); });
    await page.goto(url);
    await page.waitForFunction(() => typeof Module !== 'undefined' && Module._dd2_content_detail && Module._dd2_content_detail() === 0, {timeout: 60000});
    await page.waitForFunction(() => document.querySelector('#status').textContent === 'Vehicle ready');
    if (!await page.evaluate(() => crossOriginIsolated)) throw new Error('Workers are not isolated');
    await match(page, 'full-rest');
    await page.locator('#pose').click(); await match(page, 'full-steer');
    await page.locator('#pose').click(); await match(page, 'full-rest');
    await page.selectOption('#camera', '1'); await match(page, 'cockpit-rest');
    await page.locator('#canvas').screenshot({path: path.join(output, 'browser-cockpit.png')});
    await page.locator('#pose').click(); await match(page, 'cockpit-steer');
    await page.locator('#reset').click(); await match(page, 'cockpit-rest');
    await page.selectOption('#detail', '1'); await match(page, 'exterior-rest');
    await page.keyboard.press('Space'); await match(page, 'exterior-steer');
    await page.keyboard.press('Space'); await match(page, 'exterior-rest');
    await page.keyboard.press('PageUp'); await match(page, 'npc-rest');
    await page.keyboard.press('Space'); await match(page, 'npc-steer');
    await page.keyboard.press('r'); await match(page, 'npc-rest');
    await page.keyboard.press('PageUp'); await match(page, 'full-rest');
    await page.locator('#canvas').screenshot({path: path.join(output, 'browser-exterior.png')});
    const initial = digest(await pixels(page));
    await page.keyboard.down('ArrowRight'); await pause(180); await page.keyboard.up('ArrowRight');
    if (await stable(page) === initial) throw new Error('Camera input did not change image');
    await page.keyboard.press('r'); await match(page, 'full-rest');
    await page.keyboard.press('Tab'); await match(page, 'cockpit-rest');
    await page.setViewportSize({width: 750, height: 610}); await match(page, 'cockpit-rest');
    const invalid = await page.evaluate(() => [Module._dd2_content_select(3, 0), Module._dd2_content_select(0, 2), Module._dd2_content_set_pose(2)]);
    if (invalid.some(value => value !== 0)) throw new Error('Invalid control accepted');
    await match(page, 'cockpit-rest');
    await page.evaluate(() => Module._dd2_content_close()); await pause(1000);
    if (await page.evaluate(() => Module._dd2_content_detail()) !== -1) throw new Error('Close retained owner');
    if (report.errors.length) throw new Error(report.errors.join('\n'));
    report.pass_ = true;
  } finally {
    fs.writeFileSync(path.join(output, 'browser-report.json'), JSON.stringify(report, null, 2) + '\n');
    await browser.close();
  }
  console.log(JSON.stringify({pass_: true, comparisons: report.comparisons.length}));
})().catch(error => { console.error(error); process.exitCode = 1; });
