"""Exercise the PE verifier against real builds and corrupted copies thereof."""
import contextlib
import importlib.util
import io
from pathlib import Path
import struct
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("campfire_builder", ROOT / "scripts/edk2.py")
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)


class PeVerificationTests(unittest.TestCase):
    def verify(self, path):
        with contextlib.redirect_stdout(io.StringIO()):
            return builder.verify(path)

    def test_real_debug_and_release(self):
        for target in ("DEBUG", "RELEASE"):
            with self.subTest(target=target):
                result = self.verify(builder.artifact(target))
                self.assertEqual(result["machine"], "0x8664")
                self.assertEqual(result["subsystem"], 10)
                self.assertGreater(result["size"], 0)

    def test_corruptions_rejected(self):
        original = builder.artifact("DEBUG").read_bytes()
        pe = struct.unpack_from("<I", original, 0x3c)[0]
        mutations = [("machine", pe + 4, "<H", 0x14c),
                     ("subsystem", pe + 24 + 68, "<H", 3),
                     ("pe32", pe + 24, "<H", 0x10b),
                     ("entry", pe + 24 + 16, "<I", 0),
                     ("dll_import", pe + 24 + 120, "<I", 0x1000),
                     ("pe_signature", pe, "<I", 0),
                     ("header_offset", 0x3c, "<I", 0xfffffff0)]
        with tempfile.TemporaryDirectory(dir=ROOT / "Build/tests") as directory:
            path = Path(directory) / "corrupted.efi"
            for name, offset, fmt, value in mutations:
                with self.subTest(corruption=name):
                    data = bytearray(original)
                    struct.pack_into(fmt, data, offset, value)
                    path.write_bytes(data)
                    with self.assertRaises((RuntimeError, struct.error)):
                        self.verify(path)
            for name, data in (("empty", b""), ("text", b"CampfireHunt"), ("truncated_section", original[:-512])):
                with self.subTest(corruption=name):
                    path.write_bytes(data)
                    with self.assertRaises((RuntimeError, struct.error)):
                        self.verify(path)


if __name__ == "__main__":
    unittest.main(verbosity=2)
