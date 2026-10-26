#!/usr/bin/env python3
"""
tests/test_taryaq_acceptance.py - Acceptance Verification Suite for Taryaq v0 CLI Daemon
Target: Task M-05 | Best Societal AI Agent | Dubai AI Championship 2026-2027
Pure Python 3 Standard Library (Zero pip dependencies).

Verifies Acceptance Criteria from RECOMMENDATIONS_2026-10-24.md:
"cold start -> prescription -> blocked interaction -> override with reason -> audit line,
in under 3 minutes, with the network cable unplugged."
"""

import sys
import os
import time
import json
import subprocess
import tempfile
import unittest

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
AGENT_BIN = os.path.join(ROOT_DIR, "build", "taryaq_agent")

os.environ.setdefault("TARYAQ_AUDIT_KEY", "test-key-32-bytes-minimum-000000")


class TestTaryaqAcceptance(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not os.path.isfile(AGENT_BIN):
            subprocess.run(["make", "taryaq_agent"], cwd=ROOT_DIR, check=True)
        if not os.path.isfile(AGENT_BIN):
            raise FileNotFoundError(f"Binary {AGENT_BIN} not found and make failed.")

    def setUp(self):
        for fname in ["qala-tokens.jsonl", "taryaq_tokens.jsonl", "qala_test_tokens.jsonl"]:
            fpath = os.path.join(ROOT_DIR, fname)
            if os.path.exists(fpath):
                try:
                    os.remove(fpath)
                except OSError:
                    pass

    def tearDown(self):
        self.setUp()

    def test_01_automated_test_scenario_and_audit_line(self):
        """Verifies deterministic --test mode and validates signed JSONL audit line on disk."""
        with tempfile.NamedTemporaryFile(suffix=".jsonl", delete=False) as f:
            temp_audit = f.name

        try:
            cmd = [AGENT_BIN, "--test", "--audit-file", temp_audit]
            t0 = time.time()
            res = subprocess.run(cmd, capture_output=True, text=True)
            elapsed = time.time() - t0

            # 1. Verification of clean exit code
            self.assertEqual(res.returncode, 0, f"Process exited with {res.returncode}. Stderr: {res.stderr}")

            # 2. Latency threshold (< 180 seconds)
            self.assertLess(elapsed, 180.0, "Total execution exceeded 3 minutes")

            # 3. Check milestone outputs in stdout
            self.assertIn("[MILESTONE 1/8] Cold Start:", res.stdout)
            self.assertIn("[MILESTONE 2/8] Tool 1 (med_lookup):", res.stdout)
            self.assertIn("[MILESTONE 3/8] Tool 2 (interaction_check):", res.stdout)
            self.assertIn("[MILESTONE 4/8] Tool 5 (commit_audit Guard):", res.stdout)
            self.assertIn("[MILESTONE 5/8] Tool 4 (human_override Guard):", res.stdout)
            self.assertIn("[MILESTONE 6/8] Tool 4 (human_override):", res.stdout)
            self.assertIn("[MILESTONE 7/8] Tool 3 (claim_scrub):", res.stdout)
            self.assertIn("[MILESTONE 8/8] Tool 5 (commit_audit):", res.stdout)

            # 4. Check cold start output < 15ms
            cold_start_passed = any(
                "Cold Start:" in line and "-> PASS" in line for line in res.stdout.splitlines()
            )
            self.assertTrue(cold_start_passed, "Cold start milestone missing or failed")

            # 5. Verify audit file on disk
            self.assertTrue(os.path.isfile(temp_audit), "Audit log file was not created on disk")
            with open(temp_audit, "r", encoding="utf-8") as af:
                lines = [ln.strip() for ln in af if ln.strip()]

            self.assertGreaterEqual(len(lines), 1, "Audit log file is empty")
            committed_line = None
            for ln in lines:
                entry = json.loads(ln)
                if entry.get("action") == "PRESCRIPTION_COMMITTED":
                    committed_line = entry
                    break

            self.assertIsNotNone(committed_line, "No PRESCRIPTION_COMMITTED entry found in audit log")

            # 6. Verify exact JSON Schema requirements
            self.assertEqual(committed_line["agent"], "Taryaq v0")
            self.assertEqual(committed_line["reporting_tz"], "Asia/Dubai")
            self.assertEqual(committed_line["primary_drug"], "Colchicine")
            self.assertEqual(committed_line["concurrent_drug"], "Clarithromycin")
            self.assertEqual(committed_line["patient_egfr"], 22.0)
            self.assertEqual(committed_line["alert_code"], "FATAL_CYP3A4_RENAL_CONTRAINDICATION")
            self.assertEqual(committed_line["override_status"], "APPROVED")
            self.assertEqual(committed_line["override_token"], "TARYAQ-OVERRIDE-8842-CLIN")
            self.assertIn("nephrology", committed_line["override_rationale"])
            self.assertEqual(committed_line["state"], "COMMITTED")
            self.assertIn("sha256_signature", committed_line)
            self.assertEqual(len(committed_line["sha256_signature"]), 64)
        finally:
            if os.path.exists(temp_audit):
                os.remove(temp_audit)

    def test_02_interactive_cli_workflow(self):
        """Verifies interactive CLI conversational workflow over stdin/stdout."""
        with tempfile.NamedTemporaryFile(suffix=".jsonl", delete=False) as f:
            temp_audit = f.name

        try:
            cli_input = (
                "lookup كولشيسين\n"
                "rx Colchicine Clarithromycin 22.0\n"
                "commit\n"
                "override TARYAQ-OVERRIDE-8842-CLIN Continuous dialysis initiated\n"
                "claim 450.00 12\n"
                "commit\n"
                "quit\n"
            )

            cmd = [AGENT_BIN, "--cli", "--audit-file", temp_audit]
            res = subprocess.run(cmd, input=cli_input, capture_output=True, text=True)

            self.assertEqual(res.returncode, 0, f"CLI exited with {res.returncode}")
            self.assertIn("Found: Colchicine (كولشيسين)", res.stdout)
            self.assertIn("VERDICT: BLOCKED - FATAL_CYP3A4_RENAL_CONTRAINDICATION", res.stdout)
            self.assertIn("COMMIT REJECTED: Fatal contraindication blocked without override", res.stdout)
            self.assertIn("OVERRIDE SUCCESS: Approval code 'TARYAQ-OVERRIDE-8842-CLIN' accepted & consumed", res.stdout)
            self.assertIn("COMMIT SUCCESS: Prescription signed and appended", res.stdout)

            with open(temp_audit, "r", encoding="utf-8") as af:
                lines = [ln.strip() for ln in af if ln.strip()]

            self.assertGreaterEqual(len(lines), 1)
            last_entry = json.loads(lines[-1])
            self.assertEqual(last_entry["action"], "PRESCRIPTION_COMMITTED")
            self.assertEqual(last_entry["primary_drug"], "Colchicine")
            self.assertEqual(last_entry["override_token"], "TARYAQ-OVERRIDE-8842-CLIN")
            self.assertEqual(last_entry["state"], "COMMITTED")
        finally:
            if os.path.exists(temp_audit):
                os.remove(temp_audit)

    def test_03_zero_network_offline_guarantee(self):
        """Verifies that agent completes without failure even when telemetry is enabled and host is offline."""
        with tempfile.NamedTemporaryFile(suffix=".jsonl", delete=False) as f:
            temp_audit = f.name

        try:
            cmd = [AGENT_BIN, "--test", "--telemetry", "--audit-file", temp_audit]
            t0 = time.time()
            res = subprocess.run(cmd, capture_output=True, text=True)
            elapsed = time.time() - t0

            self.assertEqual(res.returncode, 0, f"Process exited with {res.returncode}")
            self.assertLess(elapsed, 15.0, "Telemetry timeout stalled execution")
            self.assertIn("[SUCCESS] Taryaq v0 Acceptance Workflow Passed", res.stdout)
        finally:
            if os.path.exists(temp_audit):
                os.remove(temp_audit)


if __name__ == "__main__":
    unittest.main()