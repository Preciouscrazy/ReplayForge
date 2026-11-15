"""CLI integration checks, including safe failure behavior (standard library only)."""
import pathlib
import subprocess
import sys
import tempfile
import unittest

if len(sys.argv) != 3:
    raise SystemExit("Usage: test_cli.py <replayforge-executable> <sample-actions>")
EXECUTABLE = pathlib.Path(sys.argv[1]).resolve()
SAMPLE = pathlib.Path(sys.argv[2]).resolve()


class CommandLineTests(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp_dir.cleanup)
        self.root = pathlib.Path(self.temp_dir.name)

    def run_cli(self, command, *paths):
        return subprocess.run(
            [str(EXECUTABLE), command, *(str(path) for path in paths)],
            capture_output=True, text=True, timeout=10, check=False,
        )

    def test_record_and_verify_round_trip(self):
        target = self.root / "recording.rfg"
        result = self.run_cli("record", SAMPLE, target)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.run_cli("verify", target).returncode, 0)
        self.assertFalse((self.root / "recording.rfg.tmp").exists())

    def test_existing_output_is_not_overwritten(self):
        target = self.root / "existing.rfg"
        target.write_bytes(b"IMPORTANT DATA\n")
        result = self.run_cli("record", SAMPLE, target)
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertEqual(target.read_bytes(), b"IMPORTANT DATA\n")
        self.assertFalse((self.root / "existing.rfg.tmp").exists())

    def test_input_as_output_is_rejected(self):
        actions = self.root / "actions.txt"
        actions.write_bytes(SAMPLE.read_bytes())
        before = actions.read_bytes()
        result = self.run_cli("record", actions, actions)
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertEqual(actions.read_bytes(), before)

    def test_existing_staged_file_is_preserved(self):
        target = self.root / "new.rfg"
        staged = self.root / "new.rfg.tmp"
        staged.write_bytes(b"OTHER IN PROGRESS FILE")
        result = self.run_cli("record", SAMPLE, target)
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertFalse(target.exists())
        self.assertEqual(staged.read_bytes(), b"OTHER IN PROGRESS FILE")

    def test_invalid_actions_leave_no_output(self):
        actions = self.root / "invalid.txt"
        actions.write_text("ACTIONS 1\nR X\n", encoding="utf-8")
        target = self.root / "invalid.rfg"
        result = self.run_cli("record", actions, target)
        self.assertEqual(result.returncode, 2, result.stderr)
        self.assertFalse(target.exists())
        self.assertFalse((self.root / "invalid.rfg.tmp").exists())

    def test_tampered_replay_reports_first_tick(self):
        target = self.root / "tampered.rfg"
        self.assertEqual(self.run_cli("record", SAMPLE, target).returncode, 0)
        lines = target.read_text(encoding="utf-8").splitlines()
        parts = lines[1].split()
        parts[3] = "0"
        lines[1] = " ".join(parts)
        target.write_text("\n".join(lines) + "\n", encoding="utf-8")
        result = self.run_cli("verify", target)
        self.assertEqual(result.returncode, 1)
        self.assertIn("DIVERGENCE at tick 0", result.stderr)

    def test_malformed_replay_returns_usage_error(self):
        target = self.root / "invalid.rfg"
        target.write_text("REPLAYFORGE 1\n0 A A nan\n", encoding="utf-8")
        self.assertEqual(self.run_cli("verify", target).returncode, 2)

    def test_missing_input_has_no_output(self):
        target = self.root / "missing.rfg"
        result = self.run_cli("record", self.root / "not-found.txt", target)
        self.assertEqual(result.returncode, 2)
        self.assertFalse(target.exists())


if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]], verbosity=2)
