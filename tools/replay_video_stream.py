"""Compare completed native presentations literally before discarding raw files.

The renderer closes both files before flushing its presentation journal. Each
successful block is recorded before closed raw files are removed. A failure
terminates only this test's native writer, preserving the offending capture.
Full game functionality and physical timing are separate requirements.
"""
import hashlib
import json
from pathlib import Path
import threading

from artifacts import check_space, open_files
from reference.video_archive import Records

FIELDS = ('level','cf','poly_list','restart_cd_audio')


def identity(stat):
    # Reading a file may change atime; it does not change its captured bytes.
    return (stat.st_dev,stat.st_ino,stat.st_size,stat.st_mtime_ns,stat.st_ctime_ns)


class Comparison:
    def __init__(self, directory, original, source, end, stop_writer):
        self.directory = Path(directory)
        self.source = source
        self.end = end
        if not 0 < end <= source['frame_count']:
            raise ValueError('Invalid streaming video endpoint')
        self.records = Records(original)
        if len(self.records) != source['frame_count']:
            self.records.__exit__()
            raise ValueError('Original streaming video extent differs')
        self.stop_writer = stop_writer
        self.rows = []
        self.pending = []
        self.journal = None
        self.proof = (self.directory/'literal-comparison.jsonl').open('x')
        self.error = None
        self.stop = threading.Event()
        self.thread = threading.Thread(target=self.run,name='native-literal-video',daemon=True)

    def start(self):
        self.thread.start()
        return self

    def check(self):
        if self.error is not None:
            raise RuntimeError('Native streaming video comparison failed') from self.error

    def discard(self):
        self.proof.flush()
        # The proof journal has been written before any successful raw removal.
        opened = open_files()
        for path, before in self.pending:
            now = path.stat()
            if identity(now) != identity(before) or (now.st_dev,now.st_ino) in opened:
                raise ValueError('Verified native frame reopened or changed')
        if self.pending:
            first=len(self.rows)-len(self.pending)//2
            report=dict(scope='Literal comparison of a completed native presentation block; full run acceptance is pending.',
                pass_=True,original_video_sha256=self.source['video_sha256'],
                original_trace_sha256=self.source['trace_sha256'],first=first,frames=self.rows[first:])
            (self.directory/f'literal-block-{first:05d}.json').write_text(json.dumps(report,indent=2)+'\n')
        for path, _ in self.pending:
            path.unlink()
        self.pending.clear()
        check_space(self.directory.parent)

    def drain(self):
        if self.journal is None:
            path = self.directory/'presentations.jsonl'
            if not path.exists():return
            self.journal = path.open()
        while len(self.rows) < self.end:
            position = self.journal.tell()
            line = self.journal.readline()
            if not line:return
            if not line.endswith('\n'):
                self.journal.seek(position)
                return
            row = json.loads(line)
            index = len(self.rows)
            expected = self.source['frames'][index]
            if row['index'] != index or any(row[k] != expected[k] for k in FIELDS):
                raise ValueError('Native presentation order/state differs at '+str(index))
            raw = self.records[index]
            values = []
            for suffix, begin, size in [('bin',128,307200),('pal',128+307200,1024)]:
                path = self.directory/f'f{index:05d}.{suffix}'
                before = path.stat()
                data = path.read_bytes()
                if identity(path.stat()) != identity(before) or len(data) != size or data != raw[begin:begin+size]:
                    raise ValueError('Native '+suffix+' bytes differ at '+str(index))
                values.append((path,before,hashlib.sha256(data).hexdigest()))
            proof = dict(row,original_bytes_compared=True,framebuffer_sha256=values[0][2],
                palette_sha256=values[1][2])
            self.proof.write(json.dumps(proof)+'\n')
            self.rows.append(proof)
            self.pending += [(p,s) for p,s,_ in values]
            if len(self.pending) >= 256:self.discard()

    def run(self):
        try:
            while not self.stop.wait(.01):
                self.drain()
        except BaseException as error:
            self.error = error
            self.stop_writer()

    def finish(self):
        self.stop.set()
        self.thread.join(timeout=60)
        report = dict(scope=__doc__.strip(),pass_=False,expected_frames=self.end,
            original_video_sha256=self.source['video_sha256'],
            original_trace_sha256=self.source['trace_sha256'])
        try:
            if self.thread.is_alive():raise RuntimeError('Native video worker did not stop')
            self.check();self.drain()
            if len(self.rows) != self.end:raise ValueError('Native video comparison incomplete')
            self.discard()
            report.update(pass_=True,frames=self.rows)
        except BaseException as error:
            report.update(error=str(error),compared_frames=len(self.rows))
            raise
        finally:
            self.proof.close()
            if self.journal:self.journal.close()
            self.records.__exit__()
            (self.directory/'comparison.json').write_text(json.dumps(report,indent=2)+'\n')
        return report
