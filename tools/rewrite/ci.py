#!/usr/bin/env python3
"""Run bounded rewrite CI checks and expose failure diagnostics as annotations."""
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, run_bounded


def main():
    if len(sys.argv) != 2 or sys.argv[1] not in ('native', 'wasm'):
        raise SystemExit('Usage: ci.py native|wasm')
    profile = sys.argv[1]
    directory = WORK / ('rewrite-ci-' + profile)
    directory.mkdir(parents=True, exist_ok=True)
    commands = [['make', 'rewrite-check']] if profile == 'native' else [
        ['make', 'rewrite-wasm'], ['ctest', '--preset', 'rewrite-wasm']]
    for index, command in enumerate(commands):
        log = directory / (str(index) + '.log')
        with log.open('w') as stream:
            result = run_bounded(command, directory=directory, timeout=600,
                                 stdout=stream, stderr=subprocess.STDOUT)
        content = log.read_text(errors='replace')
        print(content, flush=True)
        if result.returncode:
            tail = content[-24000:]
            escaped = tail.replace('%', '%25').replace('\r', '%0D').replace('\n', '%0A')
            print('::error title=Rewrite ' + profile + ' diagnostics::' + escaped, flush=True)
            return result.returncode
    return 0


if __name__ == '__main__':
    sys.exit(main())
