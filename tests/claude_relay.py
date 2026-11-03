"""Exercise the actual native helper with synthetic data in a temporary folder."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest

RELAY = str(Path(sys.argv.pop(1)).resolve())


class RelayChecks(unittest.TestCase):
    def test_capture_forward_and_cached_timer(self):
        with tempfile.TemporaryDirectory(prefix="claude ' relay ") as folder:
            snapshot = Path(folder) / "private ' snapshot.json"
            stamp = int(time.time())
            payload = {"session_id": "synthetic", "prompt_id": "one",
                       "cost": {"total_api_duration_ms": 123, "total_duration_ms": 500},
                       "rate_limits": {"five_hour": {"used_percentage": 68.5, "resets_at": stamp + 3600}},
                       "transcript_path": "/synthetic/private", "secret": "do-not-store"}
            raw = json.dumps(payload).encode()
            command = [RELAY, "--snapshot", str(snapshot)]
            result = subprocess.run(command + ["--forward", "/bin/cat"], input=raw, capture_output=True, check=True)
            self.assertEqual(result.stdout, raw)
            saved = json.loads(snapshot.read_text())
            self.assertEqual(saved["five_hour"]["used_percentage"], 68.5)
            self.assertNotIn("seven_day", saved)
            self.assertNotIn("secret", snapshot.read_text())
            self.assertNotIn("transcript", snapshot.read_text())
            self.assertEqual(os.stat(snapshot).st_mode & 0o777, 0o600)
            # Preserve an older source timestamp to prove timer-only calls don't freshen it.
            saved["observedAt"] = stamp - 901
            snapshot.write_text(json.dumps(saved))
            payload["cost"]["total_duration_ms"] += 1000
            output = subprocess.run(command, input=json.dumps(payload).encode(), capture_output=True, check=True)
            self.assertIn(b"31 %", output.stdout)
            self.assertEqual(json.loads(snapshot.read_text())["observedAt"], stamp - 901)
            # Unsupported/malformed Code inputs must preserve both existing data and status command.
            before = snapshot.read_bytes()
            result = subprocess.run(command + ["--forward", "/bin/cat"], input=b"not json", capture_output=True, check=True)
            self.assertEqual(result.stdout, b"not json")
            self.assertEqual(snapshot.read_bytes(), before)
            # A fresh server observation updates the timestamp.
            payload["cost"]["total_api_duration_ms"] += 1
            subprocess.run(command, input=json.dumps(payload).encode(), capture_output=True, check=True)
            self.assertGreaterEqual(json.loads(snapshot.read_text())["observedAt"], stamp)


if __name__ == "__main__":
    unittest.main()
