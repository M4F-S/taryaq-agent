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
import sqlite3
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
        for fname in ["qala-tokens.jsonl", "taryaq_tokens.jsonl", "qala_test_tokens.jsonl", "taryaq_encounters.jsonl"]:
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

    def test_04_clinical_database_100_drugs_formulary(self):
        """Verifies sovereign clinical database with 100+ medications across 15 therapeutic categories."""
        db_path = os.path.join(ROOT_DIR, "corpus", "taryaq_clinical.db")
        self.assertTrue(os.path.isfile(db_path), f"Database not found at {db_path}")

        conn = sqlite3.connect(db_path)
        cur = conn.cursor()

        # 1. Verify drug count >= 100
        cur.execute("SELECT COUNT(*) FROM drugs;")
        drug_count = cur.fetchone()[0]
        self.assertGreaterEqual(drug_count, 100, f"Expected >= 100 drugs, found {drug_count}")

        # 2. Verify therapeutic categories >= 15
        cur.execute("SELECT COUNT(DISTINCT category) FROM drug_classes;")
        cat_count = cur.fetchone()[0]
        self.assertGreaterEqual(cat_count, 15, f"Expected >= 15 therapeutic categories, found {cat_count}")

        # 3. Verify UAE Law 30/2021 controlled medications
        cur.execute("SELECT COUNT(*) FROM drugs WHERE is_controlled_uae = 1;")
        controlled_count = cur.fetchone()[0]
        self.assertGreaterEqual(controlled_count, 5, "Expected controlled drugs under UAE Law 30/2021")

        # 4. Verify interactions table
        cur.execute("SELECT COUNT(*) FROM drug_interactions;")
        inter_count = cur.fetchone()[0]
        self.assertGreaterEqual(inter_count, 10, "Expected >= 10 modeled interactions")

        # 5. Verify renal dosing guidelines table
        cur.execute("SELECT COUNT(*) FROM renal_dosing_guidelines;")
        renal_count = cur.fetchone()[0]
        self.assertGreaterEqual(renal_count, 10, "Expected >= 10 renal dosing guidelines")

        conn.close()

        # 6. Verify dynamic querying in CLI for English and Arabic names
        cli_queries = [
            ("lookup Lisinopril\nquit\n", "Found: Lisinopril"),
            ("lookup أملوديبين\nquit\n", "Found: Amlodipine"),
            ("lookup Atorvastatin\nquit\n", "Found: Atorvastatin"),
            ("lookup بريجابالين\nquit\n", "Found: Pregabalin")
        ]
        for query_in, expected_out in cli_queries:
            cmd = [AGENT_BIN, "--cli"]
            res = subprocess.run(cmd, input=query_in, capture_output=True, text=True)
            self.assertEqual(res.returncode, 0)
            self.assertIn(expected_out, res.stdout, f"Query '{query_in.strip()}' did not output '{expected_out}'")

    def test_05_trimodal_interfaces_cli_and_demo(self):
        """Verifies Tri-Modal Interfaces demonstration mode and interactive CLI commands."""
        # 1. Run --trimodal-demo flag
        cmd = [AGENT_BIN, "--trimodal-demo"]
        res = subprocess.run(cmd, capture_output=True, text=True, errors="replace")
        self.assertEqual(res.returncode, 0, f"Trimodal demo failed with code {res.returncode}: {res.stderr}")
        self.assertIn("INTERFACE 1: CLINICIAN AMBIENT COPILOT", res.stdout)
        self.assertIn("GENERATED SOAP NOTE", res.stdout)
        self.assertIn("RANKED DIFFERENTIAL DIAGNOSES", res.stdout)
        self.assertIn("PRE-ORDER CDS INTERRUPT ACTIVATED", res.stdout)
        self.assertIn("DUAL_RAAS_BLOCKADE_CONTRAINDICATION", res.stdout)
        self.assertIn("INTERFACE 2: PHARMACIST VERIFICATION CONSOLE", res.stdout)
        self.assertIn("Polypharmacy Interaction Matrix", res.stdout)
        self.assertIn("DHA SN 03/2019 CLAIMS SCRUBBING AUDIT", res.stdout)
        self.assertIn("12 overdue day(s)", res.stdout)
        self.assertIn("Statutory Penalty:   AED 2.70", res.stdout)
        self.assertIn("INTERFACE 3: PATIENT HEALTH COMPANION", res.stdout)
        self.assertIn("CARDIOVASCULAR_ACUTE", res.stdout)
        self.assertIn("DISPATCH 998/999", res.stdout)

        # 2. Interactive CLI commands for Tri-Modal
        cli_input = (
            "copilot Patient presents with elevated BP 160/100 and dizziness. History of hypertension.\n"
            "pharmacist Ramipril,Valsartan 28.0 650.00 45\n"
            "companion عندي ألم شديد بصدري وضيق تنفس\n"
            "companion أنا باخد ميتفورمين للسكر، هل آخده مع الأكل؟\n"
            "quit\n"
        )
        cmd = [AGENT_BIN, "--cli"]
        res_cli = subprocess.run(cmd, input=cli_input, capture_output=True, text=True, errors="replace")
        self.assertEqual(res_cli.returncode, 0)
        self.assertIn("CLINICIAN AMBIENT COPILOT SOAP NOTE", res_cli.stdout)
        self.assertIn("PHARMACIST VERIFICATION CONSOLE", res_cli.stdout)
        self.assertIn("DHA SN 03/2019 CLAIM DELAY: 45 days (15 overdue)", res_cli.stdout)
        self.assertIn("Penalty Amount: AED 2.92", res_cli.stdout)
        self.assertIn("PATIENT HEALTH COMPANION", res_cli.stdout)
        self.assertIn("EMERGENCY RED-FLAG PROTOCOL ACTIVATED: CARDIOVASCULAR_ACUTE", res_cli.stdout)
        self.assertIn("CATEGORY: ROUTINE_CARE", res_cli.stdout)

    def test_06_c99_native_library_and_python_bridge(self):
        """Verifies direct Python CFFI/ctypes bindings to libtaryaq shared library."""
        sys.path.insert(0, ROOT_DIR)
        from adapters.taryaq_gomaa_bridge import TaryaqC99ClinicalGate

        gate = TaryaqC99ClinicalGate()

        # Formulary count
        drug_count = gate.count_clinical_drugs()
        self.assertGreaterEqual(drug_count, 100)

        # Ambient copilot
        copilot = gate.ambient_copilot(
            dialogue_transcript="Doctor discusses adding Valsartan to existing Lisinopril for blood pressure.",
            egfr=60.0,
            proposed_drugs_csv="Lisinopril, Valsartan"
        )
        self.assertTrue(copilot["success"])
        self.assertIn("CHIEF COMPLAINT", copilot["subjective"])
        self.assertTrue(copilot["cds_interrupt_triggered"])
        self.assertEqual(copilot["cds_interrupt_code"], "DUAL_RAAS_BLOCKADE_CONTRAINDICATION")

        # Pharmacist verification
        pharm = gate.pharmacist_verification(
            medications_csv="Ramipril, Valsartan",
            egfr=28.0,
            claim_net_aed=650.00,
            submission_delay_days=45,
            icd10_code="I10",
            id_verified=True
        )
        self.assertTrue(pharm["success"])
        self.assertEqual(pharm["total_drugs_screened"], 2)
        self.assertTrue(pharm["has_contraindication_block"])
        self.assertEqual(pharm["overdue_days"], 15)
        self.assertAlmostEqual(pharm["penalty_aed"], 2.925, places=2)

        # Patient companion emergency triage
        comp_emg = gate.patient_companion_consult(
            patient_message="عندي وجع شديد وضغط في صدري وضيق تنفس وعرق بارد",
            active_meds_csv="Metformin",
            egfr=75.0
        )
        self.assertTrue(comp_emg["success"])
        self.assertTrue(comp_emg["requires_immediate_998"])
        self.assertEqual(comp_emg["triage_category"], "CARDIOVASCULAR_ACUTE")

        # Patient companion routine consult
        comp_rtn = gate.patient_companion_consult(
            patient_message="أنا باخد ميتفورمين للسكر، متى آخده؟",
            active_meds_csv="Metformin",
            egfr=75.0
        )
        self.assertTrue(comp_rtn["success"])
        self.assertFalse(comp_rtn["requires_immediate_998"])
        self.assertEqual(comp_rtn["triage_category"], "ROUTINE_CARE")


if __name__ == "__main__":
    unittest.main()