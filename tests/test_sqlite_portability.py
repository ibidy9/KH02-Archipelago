"""Optional native checks against rebuilt transports; temporary databases only."""

import ctypes as c
from contextlib import closing
import os
from pathlib import Path
import sqlite3
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "payload"))
from client.kh02.multi_record_journal_candidate import initialize


@unittest.skipUnless(
    os.environ.get("KH02_TEST_RUNTIME"), "Set KH02_TEST_RUNTIME to rebuilt runtime directory"
)
class SQLitePortability(unittest.TestCase):
    def test_all_runtime_variants_accept_and_validate_journals(self):
        root = Path(os.environ["KH02_TEST_RUNTIME"])
        for index in range(1, 9):
            with self.subTest(variant=index), tempfile.TemporaryDirectory() as temp:
                lib = c.CDLL(str((root / f"v277{index:02}" / "Kh02FreshTransport.dll").resolve()))
                lib.Kh02NativeJournalOpen.argtypes = [c.c_char_p]
                lib.Kh02NativeJournalOpen.restype = c.c_void_p
                lib.Kh02NativeJournalClose.argtypes = [c.POINTER(c.c_void_p)]
                lib.Kh02NativeJournalClose.restype = c.c_uint32
                folder = Path(temp) / "save profile 雪"
                folder.mkdir()
                path = folder / "isolated.sqlite"
                initialize(path)
                before = path.read_bytes()
                ptr = c.c_void_p(lib.Kh02NativeJournalOpen(str(path).encode("utf-8")))
                self.assertTrue(ptr.value)
                self.assertEqual(lib.Kh02NativeJournalClose(c.byref(ptr)), 1)
                self.assertIsNone(ptr.value)
                self.assertEqual(path.read_bytes(), before)
                missing = folder / "missing.sqlite"
                self.assertFalse(lib.Kh02NativeJournalOpen(str(missing).encode("utf-8")))
                self.assertFalse(missing.exists())
                with closing(sqlite3.connect(path)) as db:
                    db.execute("UPDATE metadata SET version=99")
                    db.commit()
                self.assertFalse(lib.Kh02NativeJournalOpen(str(path).encode("utf-8")))


if __name__ == "__main__":
    unittest.main()
