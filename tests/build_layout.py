"""Exercise cache relocation handling in temporary project folders."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

CMAKE = sys.argv.pop(1)
HELPER = Path(__file__).resolve().parents[1] / 'tools/prepare-build.cmake'


class BuildLayout(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name) / 'project with spaces'
        (self.root / 'tools').mkdir(parents=True)
        self.script = self.root / 'tools/prepare-build.cmake'
        shutil.copyfile(HELPER, self.script)
        self.build = self.root / 'build'

    def cache(self, source=None, build=None):
        self.build.mkdir(exist_ok=True)
        (self.build / 'CMakeCache.txt').write_text(
            f'CMAKE_HOME_DIRECTORY:INTERNAL={source or self.root.as_posix()}\n'
            f'CMAKE_CACHEFILE_DIR:INTERNAL={build or self.build.as_posix()}\n')
        (self.build / 'keep.txt').write_text('preserve this file')

    def prepare(self):
        result = subprocess.run([CMAKE, '-P', str(self.script)],
                                cwd=self.temporary.name, text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return list(self.root.glob('build-backup-*'))

    def test_new_build_and_matching_cache_are_untouched(self):
        self.assertEqual(self.prepare(), [])
        self.assertFalse(self.build.exists())
        self.cache()
        self.assertEqual(self.prepare(), [])
        self.assertEqual((self.build / 'keep.txt').read_text(), 'preserve this file')

    def test_copied_source_or_build_directory_is_preserved(self):
        for source, build in [('C:/old/project', None), (None, 'C:/old/build')]:
            with self.subTest(source=source, build=build):
                previous = set(self.root.glob('build-backup-*'))
                self.cache(source, build)
                backups = set(self.prepare())
                self.assertEqual(len(backups - previous), 1)
                self.assertTrue(previous.issubset(backups))
                self.assertFalse(self.build.exists())
                backup = (backups - previous).pop()
                self.assertEqual((backup / 'keep.txt').read_text(), 'preserve this file')
                self.assertTrue((backup / 'CMakeCache.txt').is_file())

    @unittest.skipUnless(sys.platform == 'win32', 'Windows path comparison')
    def test_windows_case_and_slash_differences_do_not_trigger_backup(self):
        self.cache(str(self.root).upper(), str(self.build).upper())
        self.assertEqual(self.prepare(), [])
        self.assertTrue(self.build.is_dir())


if __name__ == '__main__':
    unittest.main()
