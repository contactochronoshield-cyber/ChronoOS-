import importlib.util
import tempfile
import unittest
from pathlib import Path

MODULE = Path(__file__).resolve().parents[1] / "chrono_ci_healer.py"
SPEC = importlib.util.spec_from_file_location("chrono_ci_healer", MODULE)
HEALER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(HEALER)


class HealerTests(unittest.TestCase):
    def test_detects_python_bytecode_failure(self):
        with tempfile.TemporaryDirectory() as tmp:
            log = Path(tmp) / "ci.log"
            log.write_text("FAIL: Python bytecode present\n", encoding="utf-8")
            self.assertEqual(HEALER.diagnose(log), 0)

    def test_unknown_error_does_not_modify_log(self):
        with tempfile.TemporaryDirectory() as tmp:
            log = Path(tmp) / "ci.log"
            original = "A new error requiring investigation\n"
            log.write_text(original, encoding="utf-8")
            self.assertEqual(HEALER.diagnose(log), 0)
            self.assertEqual(log.read_text(encoding="utf-8"), original)

    def test_missing_log_returns_error(self):
        with tempfile.TemporaryDirectory() as tmp:
            self.assertEqual(
                HEALER.diagnose(Path(tmp) / "missing.log"), 2
            )


if __name__ == "__main__":
    unittest.main(verbosity=2)
