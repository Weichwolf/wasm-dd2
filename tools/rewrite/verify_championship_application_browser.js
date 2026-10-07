'use strict';
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');
const {chromium} = require('../browser/playwright');
const [url, mode, fixture, output] = process.argv.slice(2);
if (!output || !path.resolve(output).startsWith('/tmp/wasm-dd2/')) throw new Error('Use /tmp/wasm-dd2/');
const digest = data => crypto.createHash('sha256').update(data).digest('hex');
const report = {pass_:false, scope:'Real shared application and canvas; ordinary AI analog input, no result injection'};
const errors = [];
async function main() {
  const browser = await chromium.launch({headless:true});
  const page = await browser.newPage({viewport:{width:680,height:560}});
  page.on('pageerror', error => errors.push(String(error)));
  page.on('console', message => {
    if (message.type() === 'error') errors.push(message.text());
    if (message.type() === 'log') console.log(message.text());
  });
  try {
    await page.goto(url + `/harness.html?mode=${mode}&fixture=${fixture}`);
    await page.waitForFunction(() => window.ready, null, {timeout:60000});
    await page.locator('#start').click();
    await page.waitForFunction(() => window.started, null, {timeout:60000});
    const deadline = Date.now() + 1200000;
    let phase = 0;
    while (phase === 0 && Date.now() < deadline) {
      phase = await page.evaluate(() => Module._dd2_champ_app_step());
    }
    if (phase !== 1) throw new Error('Actual application did not finish its natural ten-lap race');
    const capture = async () => {
      const rows = await page.evaluate(() => Module.FS.readdir('/captures').filter(name=>name.endsWith('.ppm')));
      for (const name of rows) {
        const data = await page.evaluate(name => Array.from(Module.FS.readFile('/captures/'+name)), name);
        fs.writeFileSync(path.join(output,name),Buffer.from(data));
      }
    };
    await capture();
    const canvas = await page.evaluate(() => {
      const rgba = document.querySelector('#canvas').getContext('2d').getImageData(0,0,640,480).data;
      const rgb = [];
      for (let offset=0; offset<rgba.length; offset+=4) rgb.push(rgba[offset],rgba[offset+1],rgba[offset+2]);
      return rgb;
    });
    const expected = fs.readFileSync(path.join(output,'results.ppm')).subarray(Buffer.byteLength('P6\n640 480\n255\n'));
    if (!Buffer.from(canvas).equals(expected)) throw new Error('Actual canvas differs from the rendered result framebuffer');
    report.result_canvas_sha256 = digest(Buffer.from(canvas));
    report.exact_canvas_presentation = true;
    if (await page.evaluate(() => Module._dd2_champ_app_finish()) !== 1) throw new Error('Application continuation/rollback/exit failed');
    await capture();
    if (errors.length) throw new Error('Browser errors: '+errors.join('\n'));
    report.cross_origin_isolated = await page.evaluate(()=>crossOriginIsolated);
    report.pass_ = report.cross_origin_isolated;
  } finally {
    report.errors = errors;
    fs.writeFileSync(path.join(output,'canvas.json'),JSON.stringify(report,null,2)+'\n');
    await browser.close();
  }
}
main().catch(error=>{console.error(error);process.exitCode=1;});
