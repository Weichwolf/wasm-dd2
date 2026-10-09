'use strict';
const canvas = document.querySelector('#canvas');
const detail = document.querySelector('#detail');
const camera = document.querySelector('#camera');
const pose = document.querySelector('#pose');
const status = document.querySelector('#status');
let ready = false;
function refresh() {
  if (!ready || Module._dd2_content_detail() < 0) return;
  detail.value = String(Module._dd2_content_detail());
  camera.value = String(Module._dd2_content_cockpit());
  pose.setAttribute('aria-pressed', String(Boolean(Module._dd2_content_pose())));
}
function select() {
  if (!ready) return;
  if (!Module._dd2_content_select(Number(detail.value), Number(camera.value))) throw new Error('Could not draw selected view');
  refresh();
  canvas.focus();
}
detail.addEventListener('change', () => { camera.value = '0'; select(); });
camera.addEventListener('change', select);
pose.addEventListener('click', () => {
  Module._dd2_content_set_pose(!Module._dd2_content_pose());
  refresh(); canvas.focus();
});
document.querySelector('#reset').addEventListener('click', () => {
  Module._dd2_content_set_pose(0); select();
});
canvas.addEventListener('keydown', event => {
  if (['Tab', ' ', 'ArrowLeft', 'ArrowRight', 'ArrowUp', 'ArrowDown', 'PageUp', 'PageDown'].includes(event.key)) event.preventDefault();
});
window.addEventListener('resize', () => {
  if (ready) requestAnimationFrame(() => Module._dd2_content_present());
});
function observe() { refresh(); requestAnimationFrame(observe); }
var Module = {
  canvas,
  print: message => console.log(message),
  printErr: message => console.error(message),
  onAbort: message => { ready = false; status.textContent = 'Vehicle preview could not start.'; console.error(message); },
  onRuntimeInitialized() {
    Module.callMain([]);
    if (Module._dd2_content_detail() < 0) throw new Error('Vehicle owner did not open');
    ready = true;
    for (const control of document.querySelectorAll('select, button')) control.disabled = false;
    status.textContent = 'Vehicle ready';
    canvas.focus();
    requestAnimationFrame(observe);
  }
};
