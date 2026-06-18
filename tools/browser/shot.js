// Load the WASM build in headless Chromium (SwiftShader WebGL) and screenshot the canvas.
//   node shot.js <out.png> [waitMs]
const { chromium } = require('playwright');
const PORT = process.env.PORT || 8131;
(async () => {
  const out = process.argv[2] || 'shot.png';
  const wait = parseInt(process.argv[3] || '8000', 10);
  const browser = await chromium.launch({
    args: ['--use-gl=angle','--use-angle=swiftshader','--enable-unsafe-swiftshader',
           '--no-sandbox','--disable-gpu-sandbox','--ignore-gpu-blocklist']
  });
  const page = await browser.newPage({ viewport:{width:1000,height:820} });
  page.on('console', m => console.log('[page]', m.text()));
  page.on('pageerror', e => console.log('[pageerror]', e.message));
  await page.goto(`http://localhost:${PORT}/index.html`, { waitUntil:'load', timeout:60000 });
  await page.waitForTimeout(wait);
  await page.screenshot({ path: out });
  await browser.close();
  console.log('screenshot saved ->', out);
})().catch(e => { console.error('ERR', e); process.exit(1); });
