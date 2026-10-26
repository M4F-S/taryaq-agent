#!/usr/bin/env python3
"""tests/test_patient_timeline.py - Verification Suite for Taryaq Patient Timeline
Dubai Create AI Agents Championship 2026-2027 | Best Societal AI Agent
Round 2: Patient Timeline & Polypharmacy Trajectory Tracking
"""

import sys
import os
import json
import subprocess
import tempfile
import unittest

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
AGENT_BIN = os.path.join(ROOT_DIR, "build", "taryaq_agent")
VERIFY_SCRIPT = os.path.join(ROOT_DIR, "tools", "verify_audit.py")
TEST_KEY = "test-key-32-bytes-minimum-000000"


class TestPatientTimeline(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not os.path.isfile(AGENT_BIN):
            subprocess.run(["make", "build/taryaq_agent"], cwd=ROOT_DIR, check=True)
        if not os.path.isfile(AGENT_BIN):
            raise FileNotFoundError(f"Binary {AGENT_BIN} not found and build failed.")

    def test_01_timeline_demo_execution(self):
        """Verify that make timeline-demo / --timeline-demo runs all 6 encounters and exits 0."""
        with tempfile.NamedTemporaryFile(suffix=".jsonl", delete=False) as f_audit, \
             tempfile.NamedTemporaryFile(suffix=".jsonl", delete=False) as f_timeline:
            audit_file = f_audit.name
            timeline_file = f_timeline.name

        try:
            env = dict(os.environ, TARYAQ_AUDIT_KEY=TEST_KEY)
            cmd = [
                AGENT_BIN,
                "--timeline-demo",
                "--audit-file", audit_file,
                "--timeline-file", timeline_file,
            ]
            res = subprocess.run(cmd, env=env, capture_output=True, text=True)
            self.assertEqual(res.returncode, 0, f"Demo exited with {res.returncode}. Stderr: {res.stderr}")

            # Check encounter milestones in stdout
            self.assertIn("[ENCOUNTER 1/6]", res.stdout)
            self.assertIn("Baseline: Metformin 1000mg BID (eGFR: 92.0)", res.stdout)
            self.assertIn("[ENCOUNTER 2/6]", res.stdout)
            self.assertIn("Simvastatin 20mg + Amlodipine 5mg", res.stdout)
            self.assertIn("[ENCOUNTER 3/6]", res.stdout)
            self.assertIn("1st OVERRIDE VERIFIED", res.stdout)
            self.assertIn("[ENCOUNTER 4/6]", res.stdout)
            self.assertIn("Paracetamol 1000mg + Amoxicillin 500mg", res.stdout)
            self.assertIn("[ENCOUNTER 5/6]", res.stdout)
            self.assertIn("ROLLED BACK", res.stdout)
            self.assertIn("[ENCOUNTER 6/6]", res.stdout)
            self.assertIn("REPEAT OVERRIDE FLAGGED", res.stdout)

            # Check ASCII timeline trajectory chart rendered
            self.assertIn("TARYAQ LONGITUDINAL PATIENT TRAJECTORY: DXB-PAT-7819", res.stdout)
            self.assertIn("RAPID CKD PROGRESSION", res.stdout)
            self.assertIn("Stage 4", res.stdout)
            self.assertIn("RENAL FUNCTION TRAJECTORY (eGFR ml/min):", res.stdout)
        finally:
            if os.path.exists(audit_file):
                os.remove(audit_file)
            if os.path.exists(timeline_file):
                os.remove(timeline_file)

    def test_02_chronological_egfr_progression_in_ledger(self):
        """Verify that encounter records reflect the expected chronological eGFR decline."""
        with tempfile.NamedTemporaryFile(suffix=".jsonl", delete=False) as f_audit, \
             tempfile.NamedTemporaryFile(suffix=".jsonl", delete=False) as f_timeline:
            audit_file = f_audit.name
            timeline_file = f_timeline.name

        try:
            env = dict(os.environ, TARYAQ_AUDIT_KEY=TEST_KEY)
            cmd = [
                AGENT_BIN,
                "--timeline-demo",
                "--audit-file", audit_file,
                "--timeline-file", timeline_file,
            ]
            subprocess.run(cmd, env=env, check=True, capture_output=True)

            with open(timeline_file, "r", encoding="utf-8") as f:
                records = [json.loads(line) for line in f if line.strip()]

            self.assertEqual(len(records), 6)
            egfr_values = [r["patient_egfr"] for r in records]
            expected_egfr = [92.0, 74.0, 45.0, 35.0, 26.0, 22.0]
            self.assertEqual(egfr_values, expected_egfr)

            # Check patient ID consistency
            for r in records:
                self.assertEqual(r["patient_id"], "DXB-PAT-7819")
        finally:
            if os.path.exists(audit_file):
                os.remove(audit_file)
            if os.path.exists(timeline_file):
                os.remove(timeline_file)

    def test_03_repeat_override_flagged_not_fresh(self):
        """Assert that Encounter 3 is fresh (1st override) and Encounter 6 is flagged as repeat (not fresh)."""
        with tempfile.NamedTemporaryFile(suffix=".jsonl", delete=False) as f_audit, \
             tempfile.NamedTemporaryFile(suffix=".jsonl", delete=False) as f_timeline:
            audit_file = f_audit.name
            timeline_file = f_timeline.name

        try:
            env = dict(os.environ, TARYAQ_AUDIT_KEY=TEST_KEY)
            cmd = [
                AGENT_BIN,
                "--timeline-demo",
                "--audit-file", audit_file,
                "--timeline-file", timeline_file,
            ]
            subprocess.run(cmd, env=env, check=True, capture_output=True)

            with open(timeline_file, "r", encoding="utf-8") as f:
                records = [json.loads(line) for line in f if line.strip()]

            # Encounter 3 (1st override): not repeat
            enc3 = records[2]
            self.assertEqual(enc3["is_repeat_override"], False)
            self.assertIn("OVERRIDDEN", enc3["verdict"])
            self.assertEqual(enc3["override_token"], "TARYAQ-OVERRIDE-8842-CLIN")

            # Encounter 6 (2nd override for same interaction): must be flagged as repeat override
            enc6 = records[5]
            self.assertEqual(enc6["is_repeat_override"], True)
            self.assertEqual(enc6["override_token"], "DUBAI-CLIN-APPROVE-7701")
            self.assertIn("REPEAT", enc6["verdict"])
        finally:
            if os.path.exists(audit_file):
                os.remove(audit_file)
            if os.path.exists(timeline_file):
                os.remove(timeline_file)

    def test_04_cli_timeline_command(self):
        """Test interactive CLI 'timeline DXB-PAT-7819' command."""
        with tempfile.NamedTemporaryFile(suffix=".jsonl", delete=False) as f_audit, \
             tempfile.NamedTemporaryFile(suffix=".jsonl", delete=False) as f_timeline:
            audit_file = f_audit.name
            timeline_file = f_timeline.name

        try:
            env = dict(os.environ, TARYAQ_AUDIT_KEY=TEST_KEY)
            # 1. Seed demo encounters
            subprocess.run([
                AGENT_BIN,
                "--timeline-demo",
                "--audit-file", audit_file,
                "--timeline-file", timeline_file,
            ], env=env, check=True, capture_output=True)

            # 2. Query timeline in CLI
            cli_input = "timeline DXB-PAT-7819\nquit\n"
            cmd = [
                AGENT_BIN,
                "--cli",
                "--audit-file", audit_file,
                "--timeline-file", timeline_file,
            ]
            res = subprocess.run(cmd, input=cli_input, env=env, capture_output=True, text=True)
            self.assertEqual(res.returncode, 0)
            self.assertIn("TARYAQ LONGITUDINAL PATIENT TRAJECTORY: DXB-PAT-7819", res.stdout)
            self.assertIn("RENAL FUNCTION TRAJECTORY", res.stdout)
            self.assertIn("Baseline eGFR:     92.0 ml/min", res.stdout)
            self.assertIn("Current eGFR:      22.0 ml/min", res.stdout)
            self.assertIn("Trajectory Delta:  -70.0 ml/min", res.stdout)
        finally:
            if os.path.exists(audit_file):
                os.remove(audit_file)
            if os.path.exists(timeline_file):
                os.remove(timeline_file)

    def test_05_hmac_audit_chain_intact_post_timeline_demo(self):
        """Verify that the cryptographic HMAC-SHA256 audit chain is valid across all 6 clinical encounters."""
        with tempfile.NamedTemporaryFile(suffix=".jsonl", delete=False) as f_audit, \
             tempfile.NamedTemporaryFile(suffix=".jsonl", delete=False) as f_timeline:
            audit_file = f_audit.name
            timeline_file = f_timeline.name

        try:
            env = dict(os.environ, TARYAQ_AUDIT_KEY=TEST_KEY)
            subprocess.run([
                AGENT_BIN,
                "--timeline-demo",
                "--audit-file", audit_file,
                "--timeline-file", timeline_file,
            ], env=env, check=True, capture_output=True)

            # Run verify_audit.py on the audit ledger
            verify_cmd = [sys.executable, VERIFY_SCRIPT, "--ledger", audit_file]
            res = subprocess.run(verify_cmd, env=env, capture_output=True, text=True)
            self.assertEqual(res.returncode, 0, f"HMAC audit chain verification failed: {res.stderr}")
            self.assertIn("OK 6 records verified", res.stdout)
        finally:
            if os.path.exists(audit_file):
                os.remove(audit_file)
            if os.path.exists(timeline_file):
                os.remove(timeline_file)


if __name__ == "__main__":
    unittest.main()
