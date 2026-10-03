// Release each real DOM key after the engine acknowledges its pad poll.
// Sixteen steady slab presentations avoid actions ignored during animation.
async function installMenuInput(page) {
  await page.addInitScript(() => {
    window.__releaseKey = null;
    window.__slabReadyFrames = 0;
    const present = CanvasRenderingContext2D.prototype.putImageData;
    CanvasRenderingContext2D.prototype.putImageData = function(...args) {
      const result = present.apply(this, args);
      if (this.canvas.id !== 'canvas' || typeof HEAP16 === 'undefined') return result;
      window.__slabReadyFrames = HEAP16[0x46996c >> 1] === 0 ? window.__slabReadyFrames + 1 : 0;
      if (window.__releaseKey &&
          (((HEAPU16[0x754448 >> 1] | HEAPU16[0x75444a >> 1]) & window.__releaseKey.mask) ||
           HEAP32[0x936ff4 >> 2] !== window.__releaseKey.level)) {
        const {code} = window.__releaseKey;
        window.__releaseKey = null;
        window.dispatchEvent(new KeyboardEvent('keyup', {code}));
      }
      return result;
    };
  });
}

async function tap(page, code, settle = 450) {
  const racing = await page.evaluate(() => HEAP32[0x936ff4 >> 2] >= 1 && HEAP32[0x936ff4 >> 2] <= 12 &&
    HEAP32[0x7746ac >> 2] === 0 && HEAP32[0x7746c0 >> 2] > 0);
  if (!racing) await page.waitForFunction(() => window.__slabReadyFrames >= 16, null, {timeout: 15000});
  const mask = {Enter: racing ? 1 : 0x4000, Escape: 0x1008,
    ArrowUp: 0x10, ArrowDown: 0x40, ArrowLeft: 0x80, ArrowRight: 0x20}[code];
  if (!mask) throw new Error('Unknown navigation key: ' + code);
  await page.waitForFunction(mask => ((HEAPU16[0x754448 >> 1] | HEAPU16[0x75444a >> 1]) & mask) === 0, mask, {timeout: 15000});
  await page.evaluate(({code, mask}) => {
    window.__releaseKey = {code, mask, level: HEAP32[0x936ff4 >> 2]};
    window.dispatchEvent(new KeyboardEvent('keydown', {code}));
  }, {code, mask});
  await page.waitForFunction(() => window.__releaseKey === null, null, {timeout: 15000});
  await page.waitForTimeout(settle);
}

module.exports = {installMenuInput, tap};
