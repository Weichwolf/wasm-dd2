'use strict';
const canvas = document.getElementById('canvas');
const status = document.getElementById('status');
const archive = document.getElementById('archive');
const level = document.getElementById('level');
const view = document.getElementById('view');
const reset = document.getElementById('reset');
const pauseButton = document.getElementById('pause');
let loaded = false;
let reflectedView = -1;
var Module = {
  canvas,
  noInitialRun: true,
  print: message => { status.textContent = message; },
  printErr: message => { console.error(message); },
  onAbort: () => { status.textContent = 'Renderer konnte nicht gestartet werden. Bitte Seite neu laden.'; },
  onRuntimeInitialized: () => {
    if (!crossOriginIsolated) {
      status.textContent = 'Die Seite muss über den mitgelieferten lokalen Server geöffnet werden.';
      return;
    }
    archive.disabled = false;
    status.textContent = 'Wähle die Originaldatei Dirinfo.';
  }
};

archive.addEventListener('change', async () => {
  const file = archive.files[0];
  if (!file) return;
  if (file.size === 0 || file.size > 64 * 1024 * 1024) {
    status.textContent = 'Die ausgewählte Datei ist leer oder zu groß.';
    return;
  }
  archive.disabled = true;
  try {
    if (loaded && Module._dd2_application_current_level()) {
      status.textContent = 'Bitte Seite neu laden, um eine andere Originaldatei zu öffnen.';
      return;
    }
    Module.FS.writeFile('/Dirinfo', new Uint8Array(await file.arrayBuffer()));
    try { Module.callMain(['/Dirinfo']); }
    finally { Module.FS.unlink('/Dirinfo'); }
    loaded = Module._dd2_application_current_level() !== 0;
    for (const control of [level, view, reset, pauseButton]) control.disabled = !loaded;
    if (loaded) {
      level.value = String(Module._dd2_application_current_level());
      view.value = '0';
      status.textContent = 'Streckenansicht bereit. Klicke ins Bild, um die Kamera zu steuern.';
      canvas.focus();
    }
  } catch (error) {
    console.error(error);
    status.textContent = 'Originaldatei konnte nicht geöffnet werden.';
  } finally { archive.disabled = false; }
});

level.addEventListener('change', () => {
  if (!Module._dd2_application_select_level(Number(level.value))) {
    status.textContent = 'Strecke konnte nicht geladen werden.';
    level.value = String(Module._dd2_application_current_level());
  }
  canvas.focus();
});
view.addEventListener('change', () => {
  const selected = Number(view.value);
  const changed = selected === 2 ? Module._dd2_application_set_driving(1) : Module._dd2_application_show_car(selected);
  if (!changed) status.textContent = 'Ansicht konnte nicht geladen werden.';
  canvas.focus();
});
reset.addEventListener('click', () => { Module._dd2_application_reset_camera(); canvas.focus(); });
pauseButton.addEventListener('click', () => { Module._dd2_application_set_paused(Module._dd2_application_is_paused() ? 0 : 1); canvas.focus(); });
canvas.addEventListener('focus', () => { if (loaded) Module._dd2_application_resume_input(); });
canvas.addEventListener('pointerdown', () => canvas.focus());
canvas.addEventListener('blur', () => {
  if (loaded) Module._dd2_application_release_input();
});
canvas.addEventListener('wheel', event => event.preventDefault(), {passive:false});
canvas.addEventListener('keydown', event => {
  if (['ArrowLeft','ArrowRight','ArrowUp','ArrowDown','Tab','PageUp','PageDown',' ','Enter'].includes(event.key)) event.preventDefault();
});
window.addEventListener('blur', () => {
  if (loaded) Module._dd2_application_release_input();
});
document.addEventListener('visibilitychange', () => {
  if (document.hidden && loaded) Module._dd2_application_release_input();
});

function reflectSelection() {
  if (loaded) {
    const current = Module._dd2_application_current_level();
    if (current) {
      level.value = String(current);
      view.value = String(Module._dd2_application_current_view());
      pauseButton.disabled = view.value !== '2';
      pauseButton.textContent = Module._dd2_application_is_paused() ? 'Weiterfahren' : 'Pause';
      reset.textContent = view.value === '2' ? 'An den Start' : 'Kamera zurücksetzen';
      const selected = Number(view.value);
      if (selected !== reflectedView) {
        status.textContent = selected === 2 ? 'Freifahrt bereit. Klicke ins Bild; W gibt Gas, P pausiert.' : 'Ansicht bereit. Klicke ins Bild, um die Kamera zu steuern.';
        canvas.setAttribute('aria-label', selected === 2 ? 'Freifahrt. W gibt Gas, A und D lenken, Leertaste bremst.' : 'Streckenansicht. Mit den Pfeiltasten drehen.');
        reflectedView = selected;
      }
    } else {
      loaded = false;
      for (const control of [level, view, reset, pauseButton]) control.disabled = true;
      if (status.textContent.includes('bereit')) status.textContent = 'Ansicht geschlossen. Dirinfo kann erneut geöffnet werden.';
    }
  }
  requestAnimationFrame(reflectSelection);
}
requestAnimationFrame(reflectSelection);
