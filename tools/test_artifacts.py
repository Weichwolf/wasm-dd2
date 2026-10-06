"""Regression checks for destructive cleanup and capture budget boundaries."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest
from unittest.mock import patch

import artifacts


class ArtifactTests(unittest.TestCase):
    def test_cleanup_preserves_open_recent_structured_and_symlinked_files(self):
        with tempfile.TemporaryDirectory(prefix='dd2-cleanup-test-', dir=artifacts.WORK) as work:
            root = Path(work)
            for name in ('old.log', 'old.log.1', 'old.log.zst', 'open.log', 'open.log.zst', 'recent.log', 'recent.log.zst', 'report.log.json', 'report.json', 'frame.bin'):
                (root / name).write_text('evidence')
                os.utime(root / name, (time.time()-7200, time.time()-7200))
            (root / 'recent.log').touch()
            (root / 'recent.log.zst').touch()
            (root / 'link.log').symlink_to(root / 'frame.bin')
            (root / 'link.log.zst').symlink_to(root / 'frame.bin')
            with (root / 'open.log').open() as opened, (root / 'open.log.zst').open() as compressed:
                result = artifacts.cleanup_logs((root,), age=3600)
                self.assertEqual(result['removed_logs'], 3)
                self.assertEqual(opened.read(), 'evidence')
                self.assertEqual(compressed.read(), 'evidence')
            self.assertEqual({path.name for path in root.iterdir()},
                             {'open.log', 'open.log.zst', 'recent.log', 'recent.log.zst', 'report.log.json', 'report.json', 'frame.bin', 'link.log', 'link.log.zst'})

    def test_output_rejects_repo_and_symlink_escape(self):
        with tempfile.TemporaryDirectory(prefix='dd2-path-test-', dir=artifacts.WORK) as work:
            root = Path(work)
            (root / 'escape').symlink_to(artifacts.ROOT, target_is_directory=True)
            for path in (artifacts.ROOT / 'third_party/verification-artifacts', root / 'escape/run', Path('/tmp')):
                with self.assertRaises(ValueError):
                    artifacts.temporary_output(path)
            self.assertEqual(artifacts.temporary_output(root / 'run'), root / 'run')

    def test_budget_terminates_only_owned_writer_and_rejects_success(self):
        with tempfile.TemporaryDirectory(prefix='dd2-budget-test-', dir=artifacts.WORK) as work:
            root = Path(work)
            code = "import os,time; from pathlib import Path; Path('pid.json').write_text(str(os.getpid())); Path('large.bin').write_bytes(b'x'*4096); time.sleep(30)"
            with patch.object(artifacts, 'MAX_BYTES', 1024), patch.object(artifacts, 'MIN_FREE', 0):
                with self.assertRaisesRegex(RuntimeError, 'exceeded'):
                    artifacts.run_bounded([sys.executable, '-c', code], directory=root, cwd=root, timeout=10)
            pid = int((root / 'pid.json').read_text())
            self.assertFalse(Path(f'/proc/{pid}').exists())

    def test_deadline_and_successful_binary_cleanup(self):
        with tempfile.TemporaryDirectory(prefix='dd2-timeout-test-', dir=artifacts.WORK) as work:
            root = Path(work)
            with self.assertRaises(subprocess.TimeoutExpired):
                artifacts.run_bounded([sys.executable, '-c', 'import time; time.sleep(30)'], directory=root, timeout=0.1)
            (root / 'frame.bin').write_bytes(b'pixels')
            (root / 'frame.pal').write_bytes(b'palette')
            (root / 'report.json').write_text(json.dumps({'pass': True}))
            (root / 'run.log').write_text('passed')
            artifacts.discard_frames(root)
            self.assertEqual({path.name for path in root.iterdir()}, {'report.json', 'run.log'})

    def test_low_free_space_is_rejected_before_subprocess(self):
        with tempfile.TemporaryDirectory(prefix='dd2-space-test-', dir=artifacts.WORK) as work:
            with patch.object(artifacts.shutil, 'disk_usage', return_value=type('Usage', (), {'free': 0})()):
                with self.assertRaisesRegex(RuntimeError, '1 GiB free'):
                    artifacts.run_bounded([sys.executable, '-c', "raise Exception('must not start')"], directory=Path(work), timeout=1)


if __name__ == '__main__':
    unittest.main()
