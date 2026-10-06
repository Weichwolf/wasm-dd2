#!/usr/bin/env python3
"""Observe bounded actual Wine Pulse ring-overflow branches in a live original.

Requires the supported installed 32-bit Wine driver and an unchanged original
running in the supplied owned prefix. A hardware breakpoint observes only the
taken overflow path; it never replaces game/backend state, code or returns.
GDB suspends threads and changes timing. This diagnostic neither measures every
lost sample nor proves uninstrumented playback, downstream causality or parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path[:0] = [str(ROOT / 'tools'), str(ROOT / 'tools/reference')]
from artifacts import WORK, check_space, prepare_output
from reference.capture import EXE_SHA256, original_pid

DRIVER = Path('/usr/lib/i386-linux-gnu/wine/i386-unix/winepulse.so')
DRIVER_SHA256 = 'fd992d8e48fcd206c613ff114c247570f7935d25e5ca5503943726eb312e8f7a'
BRANCH = 0x6325
SIGNATURE = bytes.fromhex('03 86 68 01 00 00 31 d2 89 ae 7c 01 00 00 29 e8 f7 f5 89 96 68 01 00 00')


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def sha(path):
    with Path(path).open('rb') as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()


def target(prefix, deadline):
    while time.monotonic() < deadline:
        pid = original_pid(prefix)
        if pid:
            maps = Path(f'/proc/{pid}/maps').read_text()
            entries = [line.split(maxsplit=5) for line in maps.splitlines()]
            driver = [row for row in entries if len(row) == 6 and row[5] == str(DRIVER)]
            game = [row for row in entries if len(row) == 6 and Path(row[5]).name.lower() == 'dd2h.exe']
            if driver and game:
                require(all(sha(Path(row[5])) == EXE_SHA256 for row in game), 'mapped original EXE differs')
                with Path(f'/proc/{pid}/mem').open('rb', buffering=0) as memory:
                    memory.seek(0x462cd4)
                    movie = struct.unpack('<I', memory.read(4))[0]
                    base = min(int(row[0].split('-')[0], 16) - int(row[2], 16) for row in driver)
                    memory.seek(base + BRANCH)
                    signature = memory.read(len(SIGNATURE))
                require(signature == SIGNATURE, 'mapped Wine overflow path bytes differ')
                if movie == 1:
                    token = Path(f'/proc/{pid}/stat').read_text().split(') ', 1)[1].split()[19]
                    return pid, token, base, maps
        time.sleep(.05)
    raise TimeoutError('supported live original Intro/Pulse mapping did not appear')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prefix', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--wait-seconds', type=float, default=60)
    parser.add_argument('--seconds', type=float, default=15)
    parser.add_argument('--max-events', type=int, default=12)
    args = parser.parse_args()
    prefix = args.prefix.resolve()
    require(WORK in prefix.parents and 0 < args.wait_seconds <= 120 and 0 < args.seconds <= 30 and
            1 <= args.max_events <= 32, 'owned prefix and bounded wait/observations required')
    require(sha(DRIVER) == DRIVER_SHA256, 'unsupported Wine Pulse backend; no guessed offsets allowed')
    output = prepare_output(args.output)
    require(WORK in output.parents and not output.exists(), 'fresh output under /tmp/wasm-dd2/ required')
    output.mkdir(parents=True)
    pid, token, base, maps = target(prefix, time.monotonic() + args.wait_seconds)
    (output / 'maps.txt').write_text(maps)
    metadata = dict(scope=__doc__, original_port_parity='unproven', engine_state_writes=False,
                    hardware_breakpoint=True, debugger_changes_timing=True, pid=pid, start_token=token,
                    exe_sha256=EXE_SHA256, backend_path=str(DRIVER), backend_sha256=DRIVER_SHA256,
                    load_bias=base, overflow_path_relative_address=BRANCH, expected_path_bytes=SIGNATURE.hex(),
                    verifier_sha256=sha(Path(__file__)), seconds=args.seconds, maximum_events=args.max_events)
    script = output / 'overflow.gdb'
    script.write_text('set pagination off\nset confirm off\nset auto-solib-add off\n'
        'handle SIGSEGV nostop noprint pass\nhandle SIGUSR1 nostop noprint pass\n'
        'handle SIG32 nostop noprint pass\nhandle SIG33 nostop noprint pass\n'
        'handle SIGINT stop noprint nopass\n' + f'attach {pid}\npython\n' +
        f'output={str(output)!r}\nmetadata={metadata!r}\n' + r'''
import gdb,json,os,signal,struct,threading,time
from pathlib import Path
inferior=gdb.selected_inferior()
events=[]
pending_event=None
def u32(address):
    return struct.unpack('<I',inferior.read_memory(address,4).tobytes())[0]
class Advanced(gdb.Breakpoint):
    def stop(self):
        global pending_event
        try:
            assert pending_event is not None
            row=pending_event
            assert row['stream']==int(gdb.parse_and_eval('$esi')) & 0xffffffff
            assert row['lwp']==gdb.selected_thread().ptid[1]
            row['read_offset_after']=u32(row['stream']+0x168)
            row['pending_bytes_after']=u32(row['stream']+0x17c)
            assert row['read_offset_after']==row['predicted_read_offset_after']
            assert row['pending_bytes_after']==row['capacity_bytes']
            row['advance_observed_monotonic_ns']=time.monotonic_ns()
            events.append(row);pending_event=None;self.enabled=False
            return len(events)>=metadata['maximum_events']
        except BaseException as error:
            metadata['error']=repr(error);return True
advanced=Advanced('*'+hex(metadata['load_bias']+0x633d),type=gdb.BP_HARDWARE_BREAKPOINT,internal=True)
advanced.enabled=False
class Overflow(gdb.Breakpoint):
    def stop(self):
        global pending_event
        try:
            pointer=int(gdb.parse_and_eval('$esi')) & 0xffffffff
            pending=int(gdb.parse_and_eval('$eax')) & 0xffffffff
            capacity=int(gdb.parse_and_eval('$ebp')) & 0xffffffff
            assert u32(pointer+0x17c)==pending and u32(pointer+0x154)==capacity and 0<capacity<pending
            format_=u32(pointer+8);rate=u32(pointer+12)
            channels=bytes(inferior.read_memory(pointer+16,1))[0]
            if format_!=3 or rate!=22050 or channels!=2:return False
            before=u32(pointer+0x168)
            assert u32(pointer)==0 and before<capacity and pending%4==capacity%4==before%4==0
            row=dict(index=len(events),observed_monotonic_ns=time.monotonic_ns(),
                     lwp=gdb.selected_thread().ptid[1],stream=pointer,
                     pulse_stream=u32(pointer+4),format='s16le',rate=rate,channels=channels,
                     pending_bytes=pending,capacity_bytes=capacity,read_offset_before=before,
                     discarded_bytes_at_this_branch=pending-capacity,
                     predicted_read_offset_after=(before+pending-capacity)%capacity,
                     period_bytes=u32(pointer+0x158),local_held_bytes=u32(pointer+0x170))
            assert pending_event is None
            pending_event=row;advanced.enabled=True
            return False
        except BaseException as error:
            metadata['error']=repr(error);return True
breakpoint=Overflow('*'+hex(metadata['load_bias']+metadata['overflow_path_relative_address']),
                    type=gdb.BP_HARDWARE_BREAKPOINT,internal=True)
timer=threading.Timer(metadata['seconds'],lambda:os.kill(os.getpid(),signal.SIGINT))
timer.daemon=True;timer.start()
try:
    gdb.execute('continue')
finally:
    timer.cancel()
    breakpoint.delete()
    advanced.delete()
    if gdb.selected_inferior().pid:gdb.execute('detach')
metadata.update(observations_valid='error' not in metadata and pending_event is None,events=events,
                runtime_overflow_observed=bool(events),target_detached=True,
                observed_branch_discard_bytes=sum(row['discarded_bytes_at_this_branch'] for row in events),
                complete_movie_loss_count=False)
(Path(output)/'report.json').write_text(json.dumps(metadata,indent=2)+chr(10))
''' + '\nend\nquit\n')
    with (output / 'gdb.log').open('w') as log:
        subprocess.run(['gdb', '--nx', '-q', '-batch', '-x', str(script)], stdout=log,
                       stderr=subprocess.STDOUT, check=True, timeout=args.seconds + 20)
    report = json.loads((output / 'report.json').read_text())
    require(report['observations_valid'] and report['target_detached'] and
            sha(Path(__file__)) == metadata['verifier_sha256'], 'invalid or changing overflow observation')
    check_space(output)
    print('Original Pulse overflow diagnostic:', len(report['events']), 'actual taken S16 branches;',
          'whole loss count and parity unproven')


if __name__ == '__main__':
    main()
