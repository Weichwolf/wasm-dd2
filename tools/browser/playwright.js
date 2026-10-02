// Shared launcher for npm Playwright and Debian's node-playwright + Chromium.
const fs = require('fs');
const path = require('path');
const {createRequire} = require('module');
const entry = require.resolve('playwright');

// Debian 1.38 bundles callback-based rimraf but calls its Promise API at teardown.
// Adapt that package locally without changing files owned by the package manager.
if (entry.startsWith('/usr/share/nodejs/playwright/')) {
  const core = createRequire(entry).resolve('playwright-core');
  const bundle = require(path.join(path.dirname(core), 'lib/utilsBundle.js'));
  if (bundle.rimraf.length >= 3) {
    const remove = bundle.rimraf;
    bundle.rimraf = (dir, options) => new Promise((resolve, reject) => {
      remove(dir, options, error => error ? reject(error) : resolve());
    });
  }
}

const playwright = require('playwright');
const chromium = playwright.chromium;
function launchOptions(options = {}) {
  if (options.executablePath) return options;
  const executable = process.env.PLAYWRIGHT_CHROMIUM_EXECUTABLE_PATH ||
    (!fs.existsSync(chromium.executablePath()) && fs.existsSync('/usr/bin/chromium')
      ? '/usr/bin/chromium' : undefined);
  return executable ? {...options, executablePath: executable} : options;
}
const launch = chromium.launch.bind(chromium);
chromium.launch = options => launch(launchOptions(options));
const launchPersistentContext = chromium.launchPersistentContext.bind(chromium);
chromium.launchPersistentContext = (dir, options) =>
  launchPersistentContext(dir, launchOptions(options));

module.exports = playwright;
