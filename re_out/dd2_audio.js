// dd2_audio.js — Emscripten JS library: BANK1.SBK -> WebAudio shim for the DSOUND COM layer.
// Build with `--js-library re_out/dd2_audio.js`. The C DirectSound COM shim (dd2_com.c) calls:
//   _dd2_audio_load(wavPtr,len)            -> decode a RIFF/WAVE blob -> soundId (>=0) or -1
//   _dd2_audio_play(ch,soundId,pr1000,gain1000,loop) -> start a voice on channel ch
//   _dd2_audio_set_rate(ch,pr1000)         -> playbackRate*1000 (pitch) for a live voice
//   _dd2_audio_set_gain(ch,gain1000)       -> gain*1000 (volume) for a live voice
//   _dd2_audio_set_pan(ch,pan1000)         -> pan*1000 (-1000..1000) for a live voice
//   _dd2_audio_stop(ch)                    -> stop/release the voice
// The C shim converts DSound units to these normalized ints (so the unit math stays in C, faithful):
//   DSBVOLUME dB (-10000..0) -> gain1000 = round(1000 * 10^(dB/2000))
//   target Hz / authoredRate -> pr1000   = round(1000 * Hz/authoredRate)
//   DSound pan (-10000..10000) -> pan1000 = pan/10
// Headless (node) safe: with no AudioContext it tracks state and no-ops sound (game runs crash-free);
// in a browser it plays. SBK format (verified): count@0xc=45; dir@0x10 stride 0x1c (entry[0]=off,[1]=len,
// [3]=authored rate); each blob = standalone RIFF/WAVE, 8-bit UNSIGNED mono PCM @ 11025/22050/44100 Hz.

var DD2Audio = {
  $DD2A: {
    ctx: null, master: null, sounds: [], ch: [], inited: false,
    ensureCtx: function () {
      if (DD2A.inited) return DD2A.ctx;
      DD2A.inited = true;
      var AC = (typeof AudioContext !== 'undefined') ? AudioContext
             : (typeof webkitAudioContext !== 'undefined') ? webkitAudioContext
             : (typeof globalThis !== 'undefined' && globalThis.AudioContext) ? globalThis.AudioContext : null;
      if (AC) { try { DD2A.ctx = new AC(); DD2A.master = DD2A.ctx.createGain();
                      DD2A.master.gain.value = 1.0; DD2A.master.connect(DD2A.ctx.destination); }
                catch (e) { DD2A.ctx = null; } }
      return DD2A.ctx;
    },
    decodeWav: function (bytes) {
      var dv = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
      if (dv.getUint32(0, false) !== 0x52494646 || dv.getUint32(8, false) !== 0x57415645) return null; // RIFF/WAVE
      var pos = 12, rate = 22050, bits = 8, dataOff = -1, dataLen = 0;
      while (pos + 8 <= bytes.byteLength) {
        var cid = dv.getUint32(pos, false), csz = dv.getUint32(pos + 4, true);
        if (cid === 0x666d7420) { rate = dv.getUint32(pos + 12, true); bits = dv.getUint16(pos + 22, true); }     // 'fmt '
        else if (cid === 0x64617461) { dataOff = pos + 8; dataLen = csz; }                                         // 'data'
        pos += 8 + csz + (csz & 1);
      }
      if (dataOff < 0) return null;
      var n = (bits === 8) ? dataLen : (dataLen >> 1), f = new Float32Array(n);
      if (bits === 8) for (var i = 0; i < n; i++) f[i] = (bytes[dataOff + i] - 128) / 128;
      else for (var j = 0; j < n; j++) f[j] = dv.getInt16(dataOff + j * 2, true) / 32768;
      return { pcm: f, rate: rate };
    },
    stopCh: function (c) { var st = DD2A.ch[c]; if (st && st.src) { try { st.src.stop(); } catch (e) {} try { st.src.disconnect(); } catch (e) {} } DD2A.ch[c] = null; }
  },

  dd2_audio_load__deps: ['$DD2A'],
  dd2_audio_load: function (wavPtr, len) {
    var dec = DD2A.decodeWav(HEAPU8.subarray(wavPtr, wavPtr + len));
    if (!dec) return -1;
    var id = DD2A.sounds.length, buf = null, ctx = DD2A.ensureCtx();
    if (ctx) { try { buf = ctx.createBuffer(1, dec.pcm.length, dec.rate); buf.getChannelData(0).set(dec.pcm); } catch (e) { buf = null; } }
    DD2A.sounds[id] = { buf: buf, pcm: dec.pcm, rate: dec.rate };
    return id;
  },

  dd2_audio_play__deps: ['$DD2A'],
  dd2_audio_play: function (ch, soundId, hz, gain1000, loop) {
    var ctx = DD2A.ensureCtx(); if (ch < 0) ch = 0;
    DD2A.stopCh(ch);
    var s = DD2A.sounds[soundId]; if (!s) return;
    var st = { src: null, gain: null, pan: null, rate: s.rate };
    if (ctx && s.buf) {
      var src = ctx.createBufferSource(); src.buffer = s.buf; src.loop = !!loop;
      var g = ctx.createGain(); var p = ctx.createStereoPanner ? ctx.createStereoPanner() : null;
      src.playbackRate.value = (hz > 0 && s.rate ? hz / s.rate : 1.0);   /* hz = (authoredRate*param)>>12 -> rate = param/4096 */
      g.gain.value = (gain1000 > 0 ? gain1000 / 1000 : 0);
      if (p) { src.connect(p); p.connect(g); } else { src.connect(g); }
      g.connect(DD2A.master);
      try { src.start(); } catch (e) {}
      st.src = src; st.gain = g; st.pan = p;
    }
    DD2A.ch[ch] = st;
  },

  dd2_audio_set_rate__deps: ['$DD2A'],
  dd2_audio_set_rate: function (ch, hz) { var st = DD2A.ch[ch]; if (st && st.src) st.src.playbackRate.value = (hz > 0 && st.rate ? hz / st.rate : 1.0); },
  dd2_audio_set_gain__deps: ['$DD2A'],
  dd2_audio_set_gain: function (ch, gain1000) { var st = DD2A.ch[ch]; if (st && st.gain) st.gain.gain.value = (gain1000 > 0 ? gain1000 / 1000 : 0); },
  dd2_audio_set_pan__deps: ['$DD2A'],
  dd2_audio_set_pan: function (ch, pan1000) { var st = DD2A.ch[ch]; if (st && st.pan) { var v = pan1000 / 1000; st.pan.pan.value = v < -1 ? -1 : v > 1 ? 1 : v; } },
  dd2_audio_stop__deps: ['$DD2A'],
  dd2_audio_stop: function (ch) { DD2A.stopCh(ch); }
};

// === SAMPLE-INDEX -> EVENT MAP (45 SBK samples, for fidelity verification) ===
//  idx 7        = ENGINE drive loop (per car; looping; pitch-modulated each frame via Modify_Sound
//                 from the RPM curve Amplitude@0x447dfc/FUN_004480bc: revs 0..0x1ff -> ~0xc18..14000 Hz).
//  idx 3        = car IMPACT/CRASH (general, most common; 3D, pan by car screen-x).
//  idx 1        = HEAVY/HEAD-ON collision.
//  idx 6        = race-start rev/ambient (loop);  idx 0xf(15) = race-start klaxon/horn.
//  idx 8,9,10,11= COUNTDOWN 3-2-1-GO (8=first, 0xb=GO).
//  idx 0x11,0x12,0x24 = collision-result cues (T-bone/spin/wreck).
//  idx 0xc..0x27= spoken COMMENTARY / PA (announcer; FUN_00448228, gated by race state; vol/pan fixed).
//  idx 0x29(41) = UI click/confirm;  0x2a(42)=menu slab whoosh;  0x2b(43)=menu transition;
//  idx 0x2c(44) = UI cursor/nav beep (most common; pan center 0x800).
//  SKID/TYRE = runtime-selected index (computed sites dd2.c:15662/19065/28653) -> in the unmapped set.
// 2D SFX: Play_Sound(1|-1, 0x901010, idx, vol, pan, loop).  3D SFX: Allocate_Sound_Effect(idx, range, &pos).

if (typeof mergeInto !== 'undefined') { mergeInto(LibraryManager.library, DD2Audio); }
else if (typeof module !== 'undefined') { module.exports = DD2Audio; }  // node unit-test access
