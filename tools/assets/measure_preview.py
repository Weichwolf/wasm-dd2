#!/usr/bin/env python3
"""Measure bounded warm authored-preview graphics calls without claiming game FPS."""
import argparse
from datetime import datetime, timezone
from functools import partial
import hashlib
from http.server import ThreadingHTTPServer
import json
import math
import os
from pathlib import Path
import shlex
import statistics
import subprocess
import sys
import threading

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from artifacts import WORK, check_space, prepare_output, run_bounded
from rewrite.quality import ROOT, tool
from rewrite.serve import BUILD, Handler

# Reuse the current implementation, flags and release libraries rather than
# approximating its renderer. Private open is exposed only in this temporary
# diagnostic translation unit by appending a driver to a source copy. The
# normal application API/build is unchanged.
DRIVER = r'''
#include <SDL_timer.h>
#include <dirent.h>

static unsigned dd2_benchmark_threads(void) {
    DIR *directory = opendir("/proc/self/task");
    if (directory == NULL) { return 0; }
    unsigned count = 0;
    const struct dirent *entry = NULL;
    while ((entry = readdir(directory)) != NULL) {
        count += entry->d_name[0] != '.';
    }
    const int closed = closedir(directory);
    return closed == 0 ? count : 0;
}

int main(int argc, char **argv) {
    enum { DD2_BENCHMARK_FRAMES = 30 };
    if (argc != 2 || !dd2_content_open(argv[1], true)) { return EXIT_FAILURE; }
    const uint64_t frequency = SDL_GetPerformanceFrequency();
    if (frequency == 0) { dd2_content_close(); return EXIT_FAILURE; }
    const int details[] = {0, 1, 2, 0};
    const int cockpits[] = {0, 0, 0, 1};
    const char *names[] = {"full", "exterior", "npc", "cockpit"};
    printf("{\"process_threads\":%u,\"results\":[", dd2_benchmark_threads());
    for (size_t view = 0; view < sizeof(details) / sizeof(details[0]); ++view) {
        if (!dd2_content_select(details[view], cockpits[view])) { dd2_content_close(); return EXIT_FAILURE; }
        for (size_t frame = 0; frame < 3; ++frame) {
            if (!dd2_content_present()) { dd2_content_close(); return EXIT_FAILURE; }
        }
        double milliseconds[DD2_BENCHMARK_FRAMES] = {0};
        const double thousand = 1000;
        for (size_t frame = 0; frame < DD2_BENCHMARK_FRAMES; ++frame) {
            const uint64_t start = SDL_GetPerformanceCounter();
            if (!dd2_content_present()) { dd2_content_close(); return EXIT_FAILURE; }
            const uint64_t end = SDL_GetPerformanceCounter();
            milliseconds[frame] = (double)(end - start) * thousand / (double)frequency;
        }
        printf("%s{\"label\":\"%s\",\"milliseconds\":[", view == 0 ? "" : ",", names[view]);
        for (size_t frame = 0; frame < DD2_BENCHMARK_FRAMES; ++frame) {
            printf("%s%.9f", frame == 0 ? "" : ",", milliseconds[frame]);
        }
        printf("]}");
    }
    printf("]}\n");
    dd2_content_close();
    return EXIT_SUCCESS;
}
'''


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=WORK / 'authored-preview-timing')
    args = parser.parse_args()
    output = prepare_output(args.output)
    if WORK not in output.parents:
        parser.error('Output must be beneath /tmp/wasm-dd2/')
    output.mkdir(parents=True, exist_ok=True)
    check_space(output)
    calls = []
    env = os.environ.copy()
    env.pop('XAUTHORITY', None)
    def run(command, label):
        log = output / (label + '.log')
        with log.open('w') as stream:
            result = run_bounded(list(map(str, command)), directory=output, timeout=60,
                                 stdout=stream, stderr=subprocess.STDOUT, cwd=output, env=env)
        data = log.read_bytes()
        calls.append(dict(label=label, exit=result.returncode, sha256=digest(data)))
        if result.returncode != 0:
            raise RuntimeError(label + ': ' + log.read_text())
        return log
    source = output / 'preview_benchmark.c'
    source.write_text((ROOT / 'src/game/content_viewer.c').read_text() + '\n' + DRIVER)
    run([tool('clang-format'), '--style=file:' + str(ROOT / '.clang-format'), '-i', source], 'format')
    flags = ['-std=c11', '-O3', '-DNDEBUG', '-Wall', '-Wextra', '-Wpedantic', '-Wno-unused-parameter',
             '-Wno-unused-function', '-fno-strict-aliasing', '-ffast-math', '-Werror', '-Wshadow', '-Wconversion',
             '-Wstrict-prototypes', '-Wmissing-prototypes', '-Wformat=2', '-I', ROOT / 'src',
             '-I', ROOT / 'deps/softgl/libsoftgl/include']
    sdl = shlex.split(subprocess.check_output(['pkg-config', '--cflags', '--libs', 'sdl2'], text=True))
    run([tool('clang-tidy'), '--config-file=' + str(ROOT / '.clang-tidy'), '--warnings-as-errors=*', source, '--', *flags, *[value for value in sdl if value.startswith('-I')]], 'tidy')
    binary = output / 'preview_benchmark'
    libraries = [WORK / 'rewrite-native' / name for name in ('libdd2_render.a', 'libdd2_platform.a', 'libdd2_assets.a', 'softgl/libsoftgl.a')]
    run([tool('clang'), *flags, source, *libraries, *sdl, '-lz', '-lm', '-pthread', '-o', binary], 'build')
    with (output / 'xvfb.log').open('w') as stream:
        display = subprocess.Popen(['Xvfb', '-displayfd', '1', '-screen', '0', '1280x1024x24', '-ac'], stdout=subprocess.PIPE, stderr=stream)
        try:
            number = display.stdout.readline().decode().strip()
            if not number: raise RuntimeError('Private Xvfb failed')
            env['DISPLAY'] = ':' + number
            native = json.loads(run([binary, ROOT / 'assets/runtime'], 'native').read_text())
        finally:
            display.terminate(); display.wait(timeout=5); display.stdout.close()
    server = ThreadingHTTPServer(('127.0.0.1', 0), partial(Handler, directory=str(BUILD)))
    worker = threading.Thread(target=server.serve_forever, daemon=True); worker.start()
    browser_path = output / 'browser.json'
    try:
        run(['node', ROOT / 'tools/assets/measure_preview_browser.js',
             f'http://127.0.0.1:{server.server_port}/content.html', browser_path], 'browser')
    finally:
        server.shutdown(); worker.join(); server.server_close()
    browser = json.loads(browser_path.read_text())
    results = {}
    for platform, data in (('native', native), ('browser', browser)):
        results[platform] = {}
        for row in data['results']:
            values = sorted(row['milliseconds'])
            results[platform][row['label']] = dict(samples=len(values), median_ms=statistics.median(values),
                                                   p95_ms=values[math.ceil(.95 * len(values)) - 1], max_ms=max(values))
    files = ['.clang-format', '.clang-tidy', 'src/game/content_viewer.c', 'src/render/renderer.c', 'src/render/model_draw.c', 'src/render/model_view.c',
             'src/platform/window.c', 'tools/assets/measure_preview.py', 'tools/assets/measure_preview_browser.js']
    report = dict(pass_=True, measured_at=datetime.now(timezone.utc).isoformat(),
                  scope=browser['scope'], profile=dict(width=640,height=360,samples=4,render_threads='Automatic pinned SoftGL pools; explicit Native four-thread selection remains pending'),
                  native_method='Temporary diagnostic translation unit appends a driver to the current viewer source, same release flags/libraries, actual SDL/Xvfb presentation; normal application simulation/event/compositor costs are not included',
                  native=native, browser=browser, results=results, calls=calls,
                  source_sha256={name:digest((ROOT/name).read_bytes()) for name in files},
                  libraries_sha256={str(path):digest(path.read_bytes()) for path in libraries},
                  browser_build_sha256={path.name:digest(path.read_bytes()) for path in sorted(BUILD.glob('dd2_content_viewer.*'))},
                  driver_sha256=digest(source.read_bytes()), binary_sha256=digest(binary.read_bytes()))
    (output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    source.unlink(); binary.unlink(); browser_path.unlink()
    for path in output.glob('*.log'): path.unlink()
    check_space(output)
    print(json.dumps(dict(pass_=True, results=results, report=str(output/'report.json'))))


if __name__ == '__main__':
    main()
