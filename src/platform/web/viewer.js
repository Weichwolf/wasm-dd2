'use strict';
const canvas = document.getElementById('canvas');
const status = document.getElementById('status');
const archive = document.getElementById('archive');
const level = document.getElementById('level');
const view = document.getElementById('view');
const carClass = document.getElementById('car-class');
const carRatings = document.getElementById('car-ratings');
const reset = document.getElementById('reset');
const pauseButton = document.getElementById('pause');
const finishButton = document.getElementById('finish');
const continueButton = document.getElementById('continue');
const musicFile = document.getElementById('music-file');
const musicPlay = document.getElementById('music-play');
const musicGain = document.getElementById('music-gain');
const effectsGain = document.getElementById('effects-gain');
const musicStatus = document.getElementById('music-status');
const savesOpen = document.getElementById('saves-open');
const saveSlot = document.getElementById('save-slot');
const saveName = document.getElementById('save-name');
const preferencesSave = document.getElementById('preferences-save');
const preferencesLoad = document.getElementById('preferences-load');
const saveDelete = document.getElementById('save-delete');
const savesReload = document.getElementById('saves-reload');
const saveStatus = document.getElementById('save-status');
const playerName = document.getElementById('player-name');
const playerApply = document.getElementById('player-apply');
const playerStatus = document.getElementById('player-status');
const profileSave = document.getElementById('profile-save');
const profileLoad = document.getElementById('profile-load');
let reflectedPlayer = null;
let savePending = null;
let saveEntries = '';
let musicLoading = false;
let applicationGeneration = 0;
let loaded = false;
let runtimeReady = false;
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
    runtimeReady = true;
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
    for (const control of [level, view, carClass, reset, pauseButton, finishButton]) control.disabled = !loaded;
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

carClass.addEventListener('change', () => {
  const accepted = Module._dd2_application_select_car(Number(carClass.value));
  status.textContent = accepted ? 'Car class selected.' : 'Leave driving or the championship before choosing a car class.';
  carClass.value = String(Module._dd2_application_current_car());
  canvas.focus();
});

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
  if (['ArrowLeft','ArrowRight','ArrowUp','ArrowDown','Tab','PageUp','PageDown',' ','Enter','F1'].includes(event.key)) event.preventDefault();
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
      carClass.value = String(Module._dd2_application_current_car());
      carClass.disabled = championship || Number(view.value) >= 2;
      const ratings = [0, 1, 2].map(index => Module._dd2_application_car_rating(index));
      carRatings.textContent = `Acceleration ${ratings[0]}/5 · Speed ${ratings[1]}/5 · Grip ${ratings[2]}/5`;
      pauseButton.disabled = Number(view.value) < 2;
      for (const mode of [4, 5]) view.querySelector(`option[value="${mode}"]`).disabled = current > 7;
      view.querySelector('option[value="6"]').disabled = current <= 7;
      const phase = Module._dd2_application_race_phase();
      const musicPhase = Module._dd2_application_music_phase();
      musicFile.disabled = musicLoading || musicPhase < 0;
      musicPlay.disabled = musicPhase <= 0;
      musicGain.disabled = musicPhase < 0;
      effectsGain.disabled = musicPhase < 0;
      musicGain.value = String(Module._dd2_application_music_gain());
      effectsGain.value = String(Module._dd2_application_effects_gain());
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
  reflectSaves();
  requestAnimationFrame(reflectSelection);
}
requestAnimationFrame(reflectSelection);

function beginSaveAction(accepted, action) {
  if (!accepted) {
    const result = Module._dd2_application_saves_poll();
    saveStatus.textContent = result === 9 ? 'That name is already used by another entry.' :
      result === 8 ? 'All fifteen entries are occupied.' : 'The save action could not start.';
    return;
  }
  savePending = action;
  saveStatus.textContent = action + '…';
}
savesOpen.addEventListener('click', () => {
  beginSaveAction(Module.ccall('dd2_application_saves_open', 'number', ['string'], ['wasm-dd2-saves-v1']), 'Opening saves');
});
saveSlot.addEventListener('change', () => {
  const pointer = Module._dd2_application_save_name(Number(saveSlot.value));
  saveName.value = pointer ? Module.UTF8ToString(pointer) : 'CONFIG';
});
preferencesSave.addEventListener('click', () => {
  const logical = Number(saveSlot.value), name = saveName.value;
  if (new TextEncoder().encode(name).length > 8) {
    saveStatus.textContent = 'Use a shorter save name.';
    return;
  }
  if (logical < Module._dd2_application_saves_count() && !window.confirm('Replace the selected entry with these audio preferences?')) {
    saveStatus.textContent = 'Replacement canceled.';
    return;
  }
  beginSaveAction(Module.ccall('dd2_application_save_preferences', 'number', ['number','string'], [logical,name]), 'Saving preferences');
});
preferencesLoad.addEventListener('click', () => {
  saveStatus.textContent = Module._dd2_application_load_preferences(Number(saveSlot.value)) ?
    'Audio preferences restored.' : 'The selected entry is not a valid audio configuration.';
  canvas.focus();
});
saveDelete.addEventListener('click', () => {
  if (!window.confirm('Delete the selected entry?')) {
    saveStatus.textContent = 'Deletion canceled.';
    return;
  }
  beginSaveAction(Module._dd2_application_delete_save(Number(saveSlot.value)), 'Deleting entry');
});
savesReload.addEventListener('click', () => {
  beginSaveAction(Module._dd2_application_reload_saves(), 'Reloading saves');
});
function reflectPlayer(force = false) {
  const player = loaded ? Module.UTF8ToString(Module._dd2_application_player_name()) : '';
  if (force || player !== reflectedPlayer) {
    playerName.value = player;
    reflectedPlayer = player;
  }
}
function reflectSaves() {
  const phase = loaded ? Module._dd2_application_saves_phase() : 0;
  const result = runtimeReady ? Module._dd2_application_saves_poll() : 0;
  if (savePending && result !== 1) {
    saveStatus.textContent = result === 2 ? savePending + ' completed.' :
      result === 6 ? 'Saves changed in another window. Reload saves and choose the entry again.' :
      result === 7 ? 'The save could not be confirmed. Reload saves before making another change.' :
      result === 3 ? 'The stored data is invalid.' : 'The save action failed; the previous preferences remain available.';
    savePending = null;
  }
  savesOpen.disabled = !loaded || phase !== 0;
  savesReload.disabled = !loaded || ![2,5].includes(phase);
  const ready = loaded && phase === 2;
  saveSlot.disabled = !ready;
  saveName.disabled = !ready;
  preferencesSave.disabled = !ready;
  const count = loaded ? Module._dd2_application_saves_count() : 0;
  const entries = Array.from({length: count}, (_, index) => {
    const pointer = Module._dd2_application_save_name(index);
    return {name: pointer ? Module.UTF8ToString(pointer) : '', kind: Module._dd2_application_save_kind(index)};
  });
  const signature = JSON.stringify(entries);
  if (signature !== saveEntries) {
    const selected = Number(saveSlot.value || 0);
    saveSlot.replaceChildren(...Array.from({length:15}, (_, index) => {
      const option = document.createElement('option');
      option.value = String(index);
      option.textContent = `${index+1} · ${entries[index] ? (entries[index].name || '(unnamed)') : 'Empty'}`;
      return option;
    }));
    saveSlot.value = String(Math.min(selected,14));
    saveEntries = signature;
    const pointer = loaded ? Module._dd2_application_save_name(Number(saveSlot.value)) : 0;
    saveName.value = pointer ? Module.UTF8ToString(pointer) : 'CONFIG';
  }
  const entry = entries[Number(saveSlot.value)];
  preferencesLoad.disabled = !ready || !entry || ![0x1010,0x1020].includes(entry.kind);
  saveDelete.disabled = !ready || !entry;
  const modal = loaded && Module._dd2_application_profile_phase() !== 0;
  const championship = loaded && Module._dd2_application_championship_phase() >= 0;
  reflectPlayer();
  playerName.disabled = !loaded || championship || modal;
  playerApply.disabled = playerName.disabled;
  profileSave.disabled = !ready || modal;
  profileLoad.disabled = preferencesLoad.disabled || championship || modal;
  level.disabled = !loaded || championship || modal;
  view.disabled = !loaded || modal;
  carClass.disabled = !loaded || championship || modal || [1,3,4].includes(phase) || Number(view.value) >= 2;
  if (modal) {
    for (const control of [reset, pauseButton, finishButton, continueButton, musicFile, musicPlay,
                          musicGain, effectsGain, savesOpen, savesReload, saveSlot, saveName,
                          preferencesSave, preferencesLoad, saveDelete]) control.disabled = true;
  }
}

playerApply.addEventListener('click', () => {
  const accepted = Module.ccall('dd2_application_set_player_name', 'number', ['string'], [playerName.value]);
  if (accepted) reflectPlayer(true);
  playerStatus.textContent = accepted ?
    'Player name applied.' : 'Use at most eight printable ASCII characters and leave the championship before changing your name.';
  canvas.focus();
});
profileSave.addEventListener('click', () => {
  const logical = Number(saveSlot.value), name = saveName.value;
  if (new TextEncoder().encode(name).length > 8) {
    saveStatus.textContent = 'Use a shorter save name.';
    return;
  }
  if (logical < Module._dd2_application_saves_count() && !window.confirm('Replace the selected entry with your player name and audio settings?')) {
    saveStatus.textContent = 'Replacement canceled.';
    return;
  }
  beginSaveAction(Module.ccall('dd2_application_save_profile', 'number', ['number','string'], [logical,name]), 'Saving player and audio');
});
profileLoad.addEventListener('click', () => {
  const accepted = Module._dd2_application_load_profile(Number(saveSlot.value));
  if (accepted) reflectPlayer(true);
  saveStatus.textContent = accepted ?
    'Player and audio restored.' : 'The selected entry cannot restore player and audio settings.';
  canvas.focus();
});
