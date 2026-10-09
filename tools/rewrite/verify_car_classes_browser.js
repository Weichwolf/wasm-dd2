'use strict';
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const {chromium} = require('../browser/playwright');
const [url, archive, output] = process.argv.slice(2);
if (!output || !path.resolve(output).startsWith('/tmp/wasm-dd2/')) throw new Error('Use /tmp/wasm-dd2/');
const report = {pass_:false, scope:'Actual class selection, keyboard and lifecycle; no original pixel parity or full campaign acceptance', cases:[]};
const errors = [];
const hash = bytes => crypto.createHash('sha256').update(bytes).digest('hex');
function check(condition, label) { if (!condition) throw new Error(label); }
async function selected(page, value) {
  await page.waitForFunction(value => Module._dd2_application_current_car() === value &&
    document.getElementById('car-class').value === String(value), value);
}
async function capture(page, label) {
  const bytes = await page.locator('#canvas').screenshot();
  fs.writeFileSync(path.join(output, label+'.png'), bytes);
  return hash(bytes);
}
(async () => {
  const browser = await chromium.launch({headless:true});
  try {
    const page = await browser.newPage();
    page.on('pageerror', error => errors.push(String(error)));
    page.on('console', message => { if (message.type() === 'error') errors.push(message.text()); });
    await page.goto(url + '?reference=1');
    await page.waitForFunction(() => !document.getElementById('archive').disabled, null, {timeout:60000});
    await page.locator('#archive').setInputFiles(archive);
    await page.waitForFunction(() => Module._dd2_application_current_level() === 1);
    await selected(page, 0);
    await page.locator('#level').selectOption('1');
    await page.locator('#view').selectOption('1');
    const ratings = [[1,2,5],[3,4,2],[5,5,1]];
    for (let car=0; car<3; ++car) {
      await page.locator('#level').selectOption('1');
      await page.locator('#car-class').selectOption(String(car));
      await selected(page, car);
      const actual = await page.evaluate(() => [0,1,2].map(index => Module._dd2_application_car_rating(index)));
      check(JSON.stringify(actual) === JSON.stringify(ratings[car]), 'Ratings differ');
      await page.waitForFunction(car => document.getElementById('car-ratings').textContent.includes(`Acceleration ${car===0?1:car===1?3:5}/5`), car);
      const image = await capture(page, 'browser-class-'+car);
      await page.locator('#level').selectOption('1');
      await selected(page, car);
      await page.locator('#view').selectOption('5');
      await page.waitForFunction(() => Module._dd2_application_vehicle_count() === 1);
      await page.waitForFunction(() => document.getElementById('car-class').disabled);
      check(await page.locator('#car-class').isDisabled(), 'Driving class choice is enabled');
      check(await page.evaluate(value => Module._dd2_application_select_car((value+1)%3), car) === 0, 'Playing class changed');
      await page.locator('#canvas').focus();
      await page.keyboard.press('F1');
      await selected(page, car);
      await page.locator('#reset').click();
      await selected(page, car);
      await page.locator('#view').selectOption('7');
      await page.waitForFunction(() => Module._dd2_application_championship_phase() >= 0);
      await page.waitForFunction(() => document.getElementById('car-class').disabled);
      check(await page.locator('#car-class').isDisabled(), 'Championship class choice is enabled');
      await page.locator('#reset').click();
      await selected(page, car);
      await page.locator('#view').selectOption('1');
      await page.waitForFunction(() => Module._dd2_application_championship_phase() < 0);
      await selected(page, car);
      report.cases.push({car, ratings:actual, preview_sha256:image, track_reset_and_session:true});
    }
    await page.locator('#canvas').focus();
    await page.keyboard.press('F1');
    await selected(page, 0);
    await page.keyboard.press('F2');
    await page.waitForFunction(() => Module._dd2_application_profile_phase() === 1);
    await page.waitForFunction(() => document.getElementById('car-class').disabled);
    check(await page.locator('#car-class').isDisabled(), 'Modal class choice is enabled');
    check(await page.evaluate(() => Module._dd2_application_select_car(1)) === 0, 'Modal class changed');
    await page.keyboard.press('Escape');
    await page.waitForFunction(() => Module._dd2_application_profile_phase() === 0);
    await selected(page, 0);
    check(await page.evaluate(() => Module._dd2_application_select_car(-1)) === 0, 'Negative class accepted');
    check(await page.evaluate(() => Module._dd2_application_select_car(3)) === 0, 'Past-end class accepted');
    check(new Set(report.cases.map(item => item.preview_sha256)).size === 3, 'Class previews are identical');
    check(errors.length === 0, 'Browser errors: '+errors.join('; '));
    report.pass_ = true;
  } finally {
    await browser.close();
    report.errors = errors;
    fs.writeFileSync(path.join(output, 'browser.json'), JSON.stringify(report,null,2)+'\n');
  }
})().catch(error => { console.error(error); process.exitCode=1; });
