#!/usr/bin/env python3
"""Observe the original Outro reached through the real CREDITZ! name grid.

No original files, engine state, clocks or RNG are changed. Both movies are
observed: startup Intro is skipped with a real Escape; Outro plays to process
exit. The packet observer reports the last decoded packet; zero-sized AVI
packets retain that packet and the preceding decoded image. Paint ordinals and
literal window readbacks identify Outro independently. This capture is evidence, not
original/port A/V parity.
"""
import argparse
import fcntl
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import time

from artifacts import check_space
from driver_name_input import name_actions
from reference.audio import summarize_audio
from verify_championship_save import setup
from verify_configuration_persistence import (
    ROOT, EXE_SHA256, WINE_WORK, OriginalUI, original_args, run_original, require)
from reference.capture import original_pid
from verify_movie_codec import packets
from verify_movie_window import unpack_record


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    initial = (ROOT/'DestructionDerby2/SaveGames').read_bytes()
    out, game = setup(args, initial)
    report = dict(scope=__doc__, pass_=False, engine_state_writes=False,
                  exe_modified=False, exe_sha256=EXE_SHA256, input_keys=[],
                  observer_sha256=sha(Path(__file__)),
                  name_helper_sha256=sha(ROOT/'tools/driver_name_input.py'),
                  initial_save_sha256=hashlib.sha256(initial).hexdigest())

    def driver(pid, output, env, deadline, rundir):
        ui = OriginalUI(pid, rundir, output, env, deadline)
        try:
            for key in ['Return']*3:
                ui.key(key); report['input_keys'].append(key)
            require(ui.integer(0x940010) == 0x469f70, 'Actual driver-name grid required')
            actions = name_actions('CREDITZ!')
            for action in actions[:-1]:
                ui.key(action['key']); report['input_keys'].append(action['key'])
                pointer = ui.integer(0x469fd4)
                entered = ui.read(pointer+8, 10).split(b'\0')[0].decode('ascii')
                require(entered == action['entered'], 'Actual credit name differs')
                require(list(struct.unpack('<hh', ui.read(0x469f34, 4))) ==
                        action['cursor'], 'Actual credit grid cursor differs')
            require(entered == 'CREDITZ!', 'Actual built-in credit name required')
            report['before_accept'] = dict(name=entered, movie=ui.integer(0x462cd4),
                                           menu=ui.integer(0x940010))
            # Accept leaves ReadPad for the movie loop and then exits. Use an
            # ordinary X11 key rather than waiting for a nonexistent next pad
            # poll. Key-down is handled by the name dialog; key-up is harmless.
            subprocess.run(['xdotool', 'search', '--name', 'PC-DD2', 'windowfocus',
                            'key', '--delay', '50', 'Return'], env=env, check=True, timeout=5)
            report['input_keys'].append('Return')
            ui.wait(lambda: ui.integer(0x462cd4) == 1, 15)
            report['outro_started'] = True
            start = time.monotonic()
            while original_pid(WINE_WORK/'prefix') == pid:
                require(time.monotonic() < deadline, 'Actual Outro timed out')
                check_space(out); time.sleep(.1)
            report['outro_elapsed_seconds_after_observed_start'] = time.monotonic()-start
            require(report['outro_elapsed_seconds_after_observed_start'] > 70,
                    'Actual Outro was skipped or exited early')
            report['original_process_exited'] = True
            report['card_unchanged'] = (rundir/'SaveGames').read_bytes() == initial
        finally:
            ui.stop()

    WINE_WORK.mkdir(parents=True, exist_ok=True)
    with (WINE_WORK/'capture.lock').open('w') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        options = original_args()
        options.mode = 'audio'; options.timeout = 420
        options.wine_debug = '-all,+iccvid,+mciavi'
        options.audio = True; options.audio_rate = 22050
        options.audio_device = 'clock'; options.reset_errors = False
        options.trace_movie_video = True; options.trace_movie_timing = False
        run_original(game, out, options, on_menu=driver)
    require(original_pid(WINE_WORK/'prefix') is None, 'Original still running')
    audio = summarize_audio(out/'audio', require_played=True)
    (out/'audio/summary.json').write_text(json.dumps(audio, indent=2)+'\n')
    manifest = json.loads((out/'movie-video-archive/manifest.json').read_text())
    painted = [int(n) for n in re.findall(r'MCIAVI_PaintFrame Painting frame (\d+)',
                                        (out/'wine.log').read_text(errors='replace'))]
    starts = [i for i, value in enumerate(painted) if value == 0]
    require(len(starts) == 2 and starts[0] == 0, 'Actual Intro and Outro paints required')
    begin = starts[1]
    dimensions, compressed = packets(ROOT/'DestructionDerby2/Outro.avi')
    require(dimensions == (320, 192) and painted[begin:] == list(range(len(compressed)-1)),
            'Actual exclusive complete Outro ordinal sequence required')
    require(manifest['pass_'] and manifest['frames'] == len(painted),
            'Actual window readbacks and paint trace differ')
    opened = re.findall(r'MCI_OPEN_ELEMENT L"([^"]+)"',
                        (out/'wine.log').read_text(errors='replace'))
    require(opened == ['INTRO.AVI', 'OUTRO.AVI'], 'Actual original movie filenames differ')
    digest = hashlib.sha256(); empty = []; packet_mismatches = []
    decoded = 0; last_packet = None; last_rgb = None
    for frame, entry in enumerate(manifest['records'][begin:]):
        _, row = unpack_record(out, entry)
        require(row['rectangle'] == [0, 48, 640, 384] and row['private_mci_window_draws'] == 0,
                'Actual game movie window required')
        digest.update(row['window_argb'])
        if compressed[frame]:
            decoded += 1; last_packet = compressed[frame]
        require(row['decode_serial'] == decoded and row['packet'] == last_packet,
                'Actual decode count or retained compressed packet differs')
        if row['packet'] != compressed[frame]: packet_mismatches.append(frame)
        if not compressed[frame]:
            require(row['source_rgb'] == last_rgb, 'Empty packet changed decoded source pixels')
            empty.append(dict(frame=frame, source_rgb_sha256=hashlib.sha256(row['source_rgb']).hexdigest(),
                                source_rgb_zero=not any(row['source_rgb']),
                                window_argb_sha256=hashlib.sha256(row['window_argb']).hexdigest()))
        last_rgb = row['source_rgb']
    require(packet_mismatches == [i for i, packet in enumerate(compressed[:-1]) if not packet],
            'Unexpected decoded-packet observation mismatch')
    report.update(pass_=True, outro_archive_begin=begin, outro_frames=len(compressed)-1,
                  outro_window_sha256=digest.hexdigest(), empty_packet_frames=empty,
                  last_decoded_packet_mismatch_on_empty_frames=packet_mismatches,
                  manifest_sha256=sha(out/'movie-video-archive/manifest.json'),
                  avi_sha256=sha(ROOT/'DestructionDerby2/Outro.avi'),
                  original_port_av_parity='unproven')
    require(report['card_unchanged'], 'Credit route changed the original save card')
    (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print('Actual original Outro captured:', report['outro_frames'], 'window readbacks', flush=True)


if __name__ == '__main__':
    main()
