#!/usr/bin/env python3
"""Synthesize new PCM effects, engine loops and an original music arrangement.

All excitation, oscillators, instruments and sequencing are authored here.
No original DD2 banks, CDDA, recordings, samples or external synthesizer is read.
"""
import argparse
import hashlib
import json
from pathlib import Path
import wave

import numpy as np
from generate_textures import hash_grid

ROOT = Path(__file__).resolve().parents[2]
RATE = 48000


def excitation(frames, seed):
    return hash_grid(np.arange(frames, dtype=np.uint32), np.zeros(frames, dtype=np.uint32), seed) * 2.0 - 1.0


def band_noise(frames, seed, low, high):
    bins = np.fft.rfftfreq(frames, 1.0 / RATE)
    shape = np.exp(-np.power(bins / high, 4))
    if low:
        shape *= 1.0 - np.exp(-np.power(bins / low, 4))
    signal = np.fft.irfft(np.fft.rfft(excitation(frames, seed)) * shape, n=frames)
    return signal / max(np.sqrt(np.mean(signal * signal)), 1e-12)


def stereo(signal, delay=19, loop=True):
    if loop:
        return np.stack((signal, np.roll(signal, delay)), axis=-1)
    result = np.zeros((len(signal) + delay, 2))
    result[:len(signal), 0] = signal
    result[delay:, 1] = signal
    return result


def envelope(signal, attack=.003, release=.04):
    result = np.array(signal, copy=True)
    beginning = min(round(attack * RATE), len(result))
    ending = min(round(release * RATE), len(result))
    if beginning:
        result[:beginning] *= np.linspace(0.0, 1.0, beginning)
    if ending:
        result[-ending:] *= np.linspace(1.0, 0.0, ending)
    return result


def engine(rpm):
    frames = RATE * 2
    time = np.arange(frames) / RATE
    cycle = time * rpm / 120.0
    pulses = np.zeros(frames)
    for firing, strength in enumerate((1.0, .78, .91, .85, .99, .80, .87, .94)):
        phase = (cycle - firing / 8.0 + .5) % 1.0 - .5
        pulses += strength * np.exp(-np.square(phase / .019))
    pulses -= np.mean(pulses)
    spectrum = np.fft.rfftfreq(frames, 1.0 / RATE)
    # Exhaust resonances and intake texture remain cyclic after filtering.
    response = .20 + .45 * np.exp(-np.square((spectrum - 175) / 85))
    response += .21 * np.exp(-np.square((spectrum - 430) / 150))
    response *= np.exp(-np.power(spectrum / 2800, 4))
    exhaust = np.fft.irfft(np.fft.rfft(pulses) * response, n=frames)
    intake = band_noise(frames, 871 if rpm == 900 else 977, 70, 2100)
    rumble = np.sin(2 * np.pi * (rpm / 60) * time) * .06
    return stereo(np.tanh((exhaust * 3.5 + intake * .019 + rumble) * 1.8))


def tone(frequency, duration, seed=0):
    time = np.arange(round(duration * RATE)) / RATE
    signal = np.sin(2 * np.pi * frequency * time)
    signal += .12 * np.sin(2 * np.pi * frequency * 2 * time)
    return stereo(envelope(signal * np.exp(-time * 12), release=.025), 7 + seed, loop=False)


def music():
    # Sixteen original bars at 120 BPM: power chords, bass, syncopated lead,
    # kick/snare and alternating hats. Event tails wrap into the next cycle.
    result = np.zeros((RATE * 32, 2))
    roots = (82.406889, 65.406391, 97.998859, 73.416192)
    melody = (0, 7, 12, 10, 7, 3, 5, 7)

    def add(signal, seconds, left=1.0, right=1.0):
        index = (round(seconds * RATE) + np.arange(len(signal))) % len(result)
        result[index, 0] += signal * left
        result[index, 1] += signal * right

    for bar in range(16):
        root = roots[bar % 4]
        for eighth in range(8):
            start = bar * 2.0 + eighth * .25
            time = np.arange(round(.7 * RATE)) / RATE
            chord = np.zeros_like(time)
            for note in (root * 2, root * 3, root * 4):
                chord += np.sin(2 * np.pi * note * time)
                chord += .32 * np.sin(2 * np.pi * note * 2 * time)
            chord = envelope(np.tanh(chord * .8) * np.exp(-time * 8), release=.12)
            accent = .105 if eighth % 2 == 0 else .072
            add(chord * accent, start, .9, .67)
            add(chord * accent, start + .004, .37, .72)
            hat_time = np.arange(round(.13 * RATE)) / RATE
            hat = envelope(band_noise(len(hat_time), 9300 + bar * 8 + eighth, 4800, 12000)
                           * np.exp(-hat_time * 44), release=.03)
            add(hat * .012, start, .55 if eighth % 2 else .95, .95 if eighth % 2 else .55)
        for beat in range(4):
            start = bar * 2.0 + beat * .5
            time = np.arange(round(.55 * RATE)) / RATE
            bass = np.sin(2 * np.pi * root * time) + .2 * np.sin(2 * np.pi * root * 2 * time)
            add(envelope(bass * np.exp(-time * 4), release=.05) * .10, start)
            kick_time = np.arange(round(.4 * RATE)) / RATE
            kick_phase = 2 * np.pi * (48 * kick_time + 130 * .035 * (1 - np.exp(-kick_time / .035)))
            add(envelope(np.sin(kick_phase) * np.exp(-kick_time * 14), release=.03) * .17, start)
            if beat % 2:
                time = np.arange(round(.28 * RATE)) / RATE
                snare = band_noise(len(time), 5300 + bar * 4 + beat, 550, 9000) * .07
                snare += np.sin(2 * np.pi * 185 * time) * .025
                add(envelope(snare * np.exp(-time * 20), release=.04), start)
        if bar >= 4:
            for step in range(4):
                note = roots[(bar // 2) % 4] * 4 * 2 ** (melody[(bar * 4 + step) % 8] / 12)
                time = np.arange(round(.6 * RATE)) / RATE
                lead = np.sin(2 * np.pi * note * time + .015 * np.sin(2 * np.pi * 5 * time))
                lead += .25 * np.sin(2 * np.pi * note * 2 * time)
                add(envelope(lead * np.exp(-time * 5), release=.06) * .037,
                    bar * 2 + step * .5 + .125, .70, 1.0)
    return result


def clips():
    yield 'engine-idle', engine(900), True, .63, {'rpm': 900, 'cylinders': 8, 'cycle_degrees': 720}
    yield 'engine-load', engine(3000), True, .70, {'rpm': 3000, 'cylinders': 8, 'cycle_degrees': 720}
    frames = RATE * 2
    time = np.arange(frames) / RATE
    wind = band_noise(frames, 8821, 20, 1100)
    yield 'wind', stereo(wind * (.82 + .18 * np.sin(2 * np.pi * 2 * time)), 53), True, .22, {}
    skid = band_noise(frames, 1471, 1400, 4200) * .22
    skid += np.sin(2 * np.pi * 760 * time + 8 * np.sin(2 * np.pi * 2 * time)) * .11
    yield 'tire-skid', stereo(skid, 31), True, .53, {}
    time = np.arange(round(.85 * RATE)) / RATE
    impact = band_noise(len(time), 2111, 30, 9000) * np.exp(-time * 16) * .45
    for frequency, strength in ((67, .7), (119, .32), (267, .12)):
        impact += np.sin(2 * np.pi * frequency * time) * np.exp(-time * (7 + frequency / 90)) * strength
    yield 'body-impact', stereo(envelope(impact, .001, .07), 11, loop=False), False, .86, {}
    time = np.arange(round(.70 * RATE)) / RATE
    metal = sum(np.sin(2 * np.pi * frequency * time) * np.exp(-time * 11)
                for frequency in (811, 1271, 2017, 3541)) * .2
    yield 'metal-impact', stereo(envelope(metal, .001, .05), loop=False), False, .74, {}
    for name, frequency, duration, peak in (
            ('ui-navigate', 660, .07, .28), ('ui-confirm', 990, .15, .35),
            ('ui-cancel', 330, .20, .33), ('countdown', 440, .25, .46), ('race-start', 880, .40, .52)):
        yield name, tone(frequency, duration), False, peak, {'frequency_hz': frequency}
    yield 'music-foundry-run', music(), True, .80, {'title': 'Foundry Run', 'tempo_bpm': 120,
                                                  'bars': 16, 'composition': 'Original authored event arrangement'}


def generate(output):
    folder = output / 'runtime/audio'
    folder.mkdir(parents=True, exist_ok=True)
    entries = []
    for name, signal, loop, peak, parameters in clips():
        if not np.all(np.isfinite(signal)):
            raise ValueError('Nonfinite synthesized PCM: ' + name)
        if loop:
            # Nonlinear exhaust saturation can reintroduce a constant offset.
            # Removing the cyclic mean preserves the seamless loop boundary.
            signal -= np.mean(signal, axis=0)
        signal *= peak / max(float(np.max(np.abs(signal))), 1e-12)
        pcm = np.rint(signal * 32767).astype('<i2')
        path = folder / (name + '.wav')
        with wave.open(str(path), 'wb') as stream:
            stream.setnchannels(2)
            stream.setsampwidth(2)
            stream.setframerate(RATE)
            stream.writeframes(pcm.tobytes())
        entries.append({'name': name, 'path': str(path.relative_to(output / 'runtime')),
                        'sha256': hashlib.sha256(path.read_bytes()).hexdigest(), 'frames': len(pcm),
                        'sample_rate': RATE, 'channels': 2, 'bits': 16, 'loop': loop,
                        'peak_limit': peak, 'parameters': parameters, 'runtime_integrated': False})
    metadata = {'schema': 1, 'generator': 'tools/assets/generate_audio.py',
                'authorship': 'New numeric synthesis and original composition; no recordings or external samples',
                'audible_game_review': 'Pending actual Native/browser runtime integration', 'clips': entries}
    (folder / 'audio.json').write_text(json.dumps(metadata, indent=2) + '\n')
    return metadata


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'assets')
    args = parser.parse_args()
    metadata = generate(args.output.resolve())
    print(json.dumps({'generated_clips': len(metadata['clips']), 'rate': RATE,
                      'original_inputs': False, 'runtime_integrated': False}))


if __name__ == '__main__':
    main()
