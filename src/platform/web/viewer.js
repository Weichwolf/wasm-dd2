'use strict';
const canvas = document.getElementById('canvas');
const status = document.getElementById('status');
const archive = document.getElementById('archive');
const level = document.getElementById('level');
const view = document.getElementById('view');
const reset = document.getElementById('reset');
const pauseButton = document.getElementById('pause');
const finishButton = document.getElementById('finish');
const continueButton = document.getElementById('continue');
const musicFile = document.getElementById('music-file');
const musicPlay = document.getElementById('music-play');
const musicGain = document.getElementById('music-gain');
const effectsGain = document.getElementById('effects-gain');
const musicStatus = document.getElementById('music-status');
let musicLoading = false;
let applicationGeneration = 0;
let loaded = false;
let reflectedView = -1;
let reflectedPhase = -1;
let reflectedChampionshipPhase = -1;
var Module = {
  canvas,
  noInitialRun: true,
  print: message => { status.textContent = message; },
  printErr: message => { console.error(message); },
  onAbort: () => { status.textContent = 'The renderer could not start. Reload the page.'; },
  onRuntimeInitialized: () => {
    if (!crossOriginIsolated) {
      status.textContent = 'Open this page through the provided local server.';
      return;
    }
    archive.disabled = false;
    status.textContent = 'Select the original Dirinfo file.';
  }
};

archive.addEventListener('change', async () => {
  const file = archive.files[0];
  if (!file) return;
  if (file.size === 0 || file.size > 64 * 1024 * 1024) {
    status.textContent = 'The selected file is empty or too large.';
    return;
  }
  archive.disabled = true;
  try {
    if (loaded && Module._dd2_application_current_level()) {
      status.textContent = 'Reload the page to open another original file.';
      return;
    }
    Module.FS.writeFile('/Dirinfo', new Uint8Array(await file.arrayBuffer()));
    try { Module.callMain(['/Dirinfo']); }
    finally { Module.FS.unlink('/Dirinfo'); }
    loaded = Module._dd2_application_current_level() !== 0;
    for (const control of [level, view, reset, pauseButton, finishButton]) control.disabled = !loaded;
    if (loaded) {
      ++applicationGeneration;
      musicGain.value = '256';
      effectsGain.value = '256';
      level.value = String(Module._dd2_application_current_level());
      view.value = '0';
      status.textContent = 'Track view ready. Click the image to control the camera.';
      canvas.focus();
    }
  } catch (error) {
    console.error(error);
    status.textContent = 'The original file could not be opened.';
  } finally { archive.disabled = false; }
});

musicFile.addEventListener('change', async () => {
  const file = musicFile.files[0];
  const generation = applicationGeneration;
  if (!file || musicLoading || !loaded) return;
  const match = /^track(\d{2})\.cdda$/i.exec(file.name);
  const track = match ? Number(match[1]) : 0;
  if (track < 2 || track > 19 || file.size === 0 || file.size > 64 * 1024 * 1024 || file.size % 4) {
    musicStatus.textContent = 'Select a complete file from track02.cdda to track19.cdda.';
    musicFile.value = '';
    return;
  }
  musicLoading = true;
  musicFile.disabled = true;
  try {
    const bytes = new Uint8Array(await file.arrayBuffer());
    // The view may have closed while the local file was being read.
    if (!loaded || generation !== applicationGeneration || !Module._dd2_application_current_level()) return;
    Module.FS.writeFile('/Music.cdda', bytes);
    try {
      if (!Module._dd2_application_load_music(track)) throw new Error('The Redbook track could not be loaded.');
    } finally { Module.FS.unlink('/Music.cdda'); }
    musicStatus.textContent = `Redbook track ${track} · repeating`;
    canvas.focus();
  } catch (error) {
    musicStatus.textContent = error.message;
  } finally {
    musicLoading = false;
    musicFile.value = '';
  }
});
musicPlay.addEventListener('click', () => {
  Module._dd2_application_set_music_playing(Module._dd2_application_music_phase() === 2 ? 0 : 1);
  canvas.focus();
});
musicGain.addEventListener('input', () => Module._dd2_application_set_music_gain(Number(musicGain.value)));
musicGain.addEventListener('change', () => canvas.focus());
effectsGain.addEventListener('input', () => Module._dd2_application_set_effects_gain(Number(effectsGain.value)));
effectsGain.addEventListener('change', () => canvas.focus());

level.addEventListener('change', () => {
  if (!Module._dd2_application_select_level(Number(level.value))) {
    status.textContent = 'The track could not be loaded.';
    level.value = String(Module._dd2_application_current_level());
  }
  canvas.focus();
});
view.addEventListener('change', () => {
  const selected = Number(view.value);
  let changed;
  if (selected >= 7) changed = Module._dd2_application_start_championship(selected === 8 ? 1 : 0);
  else if (selected >= 3) changed = Module._dd2_application_start_race(selected - 3);
  else if (selected === 2) changed = Module._dd2_application_set_driving(1);
  else changed = Module._dd2_application_show_car(selected);
  if (!changed) status.textContent = 'The view could not be loaded.';
  canvas.focus();
});
finishButton.addEventListener('click', () => {
  const phase = Module._dd2_application_championship_phase();
  const changed = phase >= 0 ? Module._dd2_application_exit_championship() : Module._dd2_application_withdraw_race();
  if (!changed) status.textContent = 'The current view has been retained. Leaving the race failed.';
  canvas.focus();
});
continueButton.addEventListener('click', () => {
  if (!Module._dd2_application_continue_championship()) {
    status.textContent = 'The next race could not be loaded. Your results have been retained.';
  }
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
      const championshipPhase = Module._dd2_application_championship_phase();
      const championship = championshipPhase >= 0;
      level.disabled = championship;
      view.value = String(Module._dd2_application_current_view());
      pauseButton.disabled = Number(view.value) < 2;
      for (const mode of [4, 5]) view.querySelector(`option[value="${mode}"]`).disabled = current > 7;
      view.querySelector('option[value="6"]').disabled = current <= 7;
      const phase = Module._dd2_application_race_phase();
      const musicPhase = Module._dd2_application_music_phase();
      musicFile.disabled = musicLoading || musicPhase < 0;
      musicPlay.disabled = musicPhase <= 0;
      musicGain.disabled = musicPhase < 0;
      effectsGain.disabled = musicPhase < 0;
      musicPlay.textContent = musicPhase === 2 ? 'Pause music' : 'Play music';
      if (musicPhase < 0) musicStatus.textContent = 'Audio output is unavailable.';
      finishButton.disabled = Number(view.value) < 3 || (!championship && phase === 3);
      finishButton.textContent = championship ? 'Leave championship' : 'Leave race';
      continueButton.disabled = !championship || ![2, 3].includes(championshipPhase);
      continueButton.textContent = championshipPhase === 3 ? 'Continue season' : 'Next race';
      reset.disabled = championship && championshipPhase !== 1;
      pauseButton.textContent = Module._dd2_application_is_paused() ? 'Resume' : 'Pause';
      reset.textContent = championship ? 'Restart race' : (Number(view.value) >= 2 ? 'Return to start' : 'Reset camera');
      const selected = Number(view.value);
      if (selected !== reflectedView || phase !== reflectedPhase || championshipPhase !== reflectedChampionshipPhase) {
        status.textContent = selected >= 3 ? ['Countdown. Throttle unlocks at GO.', 'Race running. W accelerates, P pauses.', 'Race ended. Results follow …', 'Results. You are highlighted in yellow; R restarts.'][phase] : selected === 2 ? 'Free driving ready. Click the image; W accelerates, P pauses.' : 'View ready. Click the image to control the camera.';
        if (championship) {
          const season = Module._dd2_application_championship_season();
          const round = Module._dd2_application_championship_round();
          const division = Module._dd2_application_championship_division();
          const points = Module._dd2_application_championship_points(0);
          const message = ['Preparing race.', (phase === 0 ? 'Countdown. Throttle unlocks at GO; R restarts.' : phase === 2 ? 'Race ended. Results follow …' : 'Race running. W accelerates; P pauses; R restarts.'), 'Round complete. Enter continues.', 'Season complete. Enter confirms the outcome.', 'Champion. Enter or Escape returns to the track view.', 'Eliminated. Enter or Escape returns to the track view.', 'Championship ended.'][championshipPhase];
          status.textContent = `Season ${season} · Race ${round} · Division ${division} · ${points} points. ${message}`;
        }
        canvas.setAttribute('aria-label', selected >= 2 ? 'Driving. W accelerates, A and D steer, Space brakes.' : 'Track view. Rotate with the arrow keys.');
        reflectedView = selected;
        reflectedPhase = phase;
        reflectedChampionshipPhase = championshipPhase;
      }
    } else {
      loaded = false;
      archive.value = '';
      for (const control of [level, view, reset, pauseButton, finishButton, continueButton]) control.disabled = true;
      for (const control of [musicFile, musicPlay, musicGain, effectsGain]) control.disabled = true;
      musicStatus.textContent = 'Select track02.cdda to track19.cdda from the Redbook folder.';
      status.textContent = 'View closed. Dirinfo can be opened again.';
    }
  }
  requestAnimationFrame(reflectSelection);
}
requestAnimationFrame(reflectSelection);
