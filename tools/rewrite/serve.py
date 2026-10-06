#!/usr/bin/env python3
"""Serve only the rewrite browser build on localhost with worker isolation."""
import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

BUILD = Path('/tmp/wasm-dd2/rewrite-wasm')
FILES = {'/', '/index.html', '/viewer.js', '/dd2_app.js', '/dd2_app.wasm', '/dd2_app.worker.js'}


class Handler(SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path.split('?', 1)[0] not in FILES:
            self.send_error(404)
            return
        super().do_GET()

    def do_HEAD(self):
        if self.path.split('?', 1)[0] not in FILES:
            self.send_error(404)
            return
        super().do_HEAD()

    def end_headers(self):
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        self.send_header('Cache-Control', 'no-store')
        super().end_headers()

    def log_message(self, *args):
        pass


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=8080)
    args = parser.parse_args()
    if not (BUILD / 'dd2_app.js').is_file():
        parser.error('Build first with make rewrite-wasm')
    server = ThreadingHTTPServer(('127.0.0.1', args.port), partial(Handler, directory=str(BUILD)))
    print(f'http://127.0.0.1:{server.server_port}/', flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == '__main__':
    main()
