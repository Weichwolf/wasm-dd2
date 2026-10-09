'use strict';
const fs = require('fs');
const path = require('path');
const {chromium} = require('../browser/playwright');
const [url, output] = process.argv.slice(2);
if (!output || !path.resolve(output).startsWith('/tmp/wasm-dd2/')) throw new Error('Use /tmp/wasm-dd2/');
(async () => {
  const browser = await chromium.launch({headless: true});
  const errors = [], results = [];
  try {
    const page = await browser.newPage({viewport: {width: 900, height: 700}});
    page.on('pageerror', error => errors.push(String(error)));
    page.on('console', message => { if (message.type() === 'error') errors.push(message.text()); });
    await page.goto(url);
    await page.waitForFunction(() => document.querySelector('#status').textContent === 'Vehicle ready');
    for (const [label, detail, cockpit] of [['full', 0, 0], ['exterior', 1, 0], ['npc', 2, 0], ['cockpit', 0, 1]]) {
      const record = await page.evaluate(async selection => {
        if (!Module._dd2_content_select(selection.detail, selection.cockpit)) throw new Error('Selection failed');
        for (let index = 0; index < 3; ++index) Module._dd2_content_present();
        const milliseconds = [];
        for (let index = 0; index < 30; ++index) {
          await new Promise(requestAnimationFrame);
          const start = performance.now();
          if (!Module._dd2_content_present()) throw new Error('Presentation failed');
          milliseconds.push(performance.now() - start);
        }
        return {milliseconds, crossOriginIsolated, hardware_concurrency: navigator.hardwareConcurrency};
      }, {detail, cockpit});
      results.push({label, ...record});
    }
    await page.evaluate(() => Module._dd2_content_close());
    if (errors.length) throw new Error(errors.join('\n'));
    fs.writeFileSync(output, JSON.stringify({pass_: true, scope: 'Warm authored preview render/resolve/readback/presentation submission CPU calls; no simulation or compositor/complete-game FPS acceptance', results}, null, 2) + '\n');
  } finally { await browser.close(); }
})().catch(error => { console.error(error); process.exitCode = 1; });
