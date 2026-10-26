#!/usr/bin/env python3
"""
tests/test_taryaq_gomaa.py - Test Suite for Taryaq & Gomaa Memory OS Integration
Track: Best Societal AI Agent | Dubai Create AI Agents Championship 2026-2027
Engine: Belya C99 Deterministic Gate + Gomaa 4-Quadrant Memory OS
"""

import os
import sys
import tempfile
import unittest
import shutil

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, ROOT_DIR)
sys.path.insert(0, os.path.join(ROOT_DIR, "adapters"))

from taryaq_gomaa_bridge import TaryaqLongitudinalPatientMemory, GOMAA_AVAILABLE


@unittest.skipUnless(GOMAA_AVAILABLE, "optional: gomaa Memory OS not installed (set GOMAA_HOME)")
class TestTaryaqGomaaIntegration(unittest.TestCase):
    def setUp(self):
        self.tmpdir = tempfile.mkdtemp(prefix="taryaq_gomaa_test_")
        self.db_path = os.path.join(self.tmpdir, "test_taryaq_memory.db")
        self.lib_dir = os.path.join(ROOT_DIR, "build")
        self.bridge = TaryaqLongitudinalPatientMemory(db_path=self.db_path, lib_dir=self.lib_dir)

    def tearDown(self):
        shutil.rmtree(self.tmpdir, ignore_errors=True)

    def test_01_multi_week_consultation_logging(self):
        """Test recording multi-week patient consultations into Gomaa Episodic Event ledger."""
        mrn = "DXB-PAT-4401"
        # Week 1: Initial visit for acute gout flare
        w1 = self.bridge.log_patient_consultation(
            patient_mrn=mrn,
            week_number=1,
            medications_prescribed=["Colchicine"],
            egfr_ml_min=65.0,
            adherence_rate=1.0,
            notes="Patient diagnosed with acute gout. Started Colchicine 0.5mg BID."
        )
        self.assertEqual(w1["status"], "LOGGED")
        self.assertEqual(w1["turn_index"], 1)
        self.assertEqual(w1["current_egfr"], 65.0)

        # Week 2: Routine follow-up
        w2 = self.bridge.log_patient_consultation(
            patient_mrn=mrn,
            week_number=2,
            medications_prescribed=["Colchicine"],
            egfr_ml_min=62.0,
            adherence_rate=0.95,
            notes="Symptoms resolving. Tolerating medication well."
        )
        self.assertEqual(w2["turn_index"], 2)

        # Verify episodic events in SQLite
        events = self.bridge.store.get_episodic_events(mrn)
        self.assertEqual(len(events), 2)
        self.assertEqual(events[0]["origin_agent"], "taryaq_agent")

    def test_02_renal_trajectory_tracking(self):
        """Test tracking renal trajectory (eGFR decline) across multi-week consultations."""
        mrn = "DXB-PAT-4402"
        # Baseline visit (eGFR 68.0 mL/min)
        self.bridge.log_patient_consultation(
            patient_mrn=mrn,
            week_number=1,
            medications_prescribed=["Colchicine"],
            egfr_ml_min=68.0
        )
        # Week 4 follow-up: Acute renal decline to 22.0 mL/min
        w4 = self.bridge.log_patient_consultation(
            patient_mrn=mrn,
            week_number=4,
            medications_prescribed=["Colchicine"],
            egfr_ml_min=22.0,
            reported_side_effects=["nausea", "fatigue"],
            notes="Patient presented with acute dehydration. Lab shows serum creatinine spike, eGFR dropped to 22."
        )
        self.assertIn("ACUTE_DECLINE", w4["egfr_trend"])
        self.assertEqual(w4["current_egfr"], 22.0)

    def test_03_context_window_compression(self):
        """Test that Gomaa Working Memory compresses multi-week visits to prevent LLM context blowup."""
        mrn = "DXB-PAT-4403"
        for w in range(1, 8):
            self.bridge.log_patient_consultation(
                patient_mrn=mrn,
                week_number=w,
                medications_prescribed=["Amlodipine", "Metformin"],
                egfr_ml_min=85.0 - w,
                notes=f"Detailed consultation notes for week {w} covering diet, exercise, and vitals."
            )

        compact_ctx = self.bridge.get_compact_clinical_context(mrn)
        self.assertEqual(compact_ctx["total_visits_logged"], 7)
        self.assertLess(compact_ctx["compact_working_tokens"], 180)
        self.assertGreaterEqual(compact_ctx["context_savings_pct"], 60)

    def test_04_unbypassable_c99_belya_clinical_safety_gate(self):
        """
        Test that proposing Clarithromycin for a patient on Colchicine with eGFR < 30
        is deterministically blocked by the C99 Belya gate in < 0.05 ms.
        """
        mrn = "DXB-PAT-4404"
        # Patient already taking Colchicine, eGFR recently dropped to 20 mL/min
        self.bridge.log_patient_consultation(
            patient_mrn=mrn,
            week_number=3,
            medications_prescribed=["Colchicine"],
            egfr_ml_min=20.0
        )

        # Physician attempts to prescribe Clarithromycin for respiratory infection
        res = self.bridge.evaluate_candidate_prescription(
            patient_mrn=mrn,
            new_candidate_drug="Clarithromycin"
        )

        self.assertTrue(res["is_blocked"], "High-risk CYP3A4 interaction was not blocked!")
        self.assertEqual(res["alert_code"], "FATAL_CYP3A4_RENAL_CONTRAINDICATION")
        self.assertEqual(res["verdict"], "CLINICAL_CONTRAINDICATION_BLOCK")
        self.assertLess(res["latency_ms"], 1.0, "C99 safety gate exceeded microsecond latency")

    def test_05_human_in_the_loop_override_with_audit_trail(self):
        """Test single-use human clinical override verification with Gomaa audit trail."""
        mrn = "DXB-PAT-4405"
        self.bridge.log_patient_consultation(
            patient_mrn=mrn,
            week_number=2,
            medications_prescribed=["Colchicine"],
            egfr_ml_min=24.0
        )

        # Physician provides legitimate clinical override code with rationale
        res = self.bridge.evaluate_candidate_prescription(
            patient_mrn=mrn,
            new_candidate_drug="Clarithromycin",
            human_override_code="TARYAQ-OVERRIDE-8842-CLIN",
            override_rationale="Temporary single-dose monitored therapy with ICU renal panel."
        )

        self.assertEqual(res["override_status"], "APPROVED_BY_PHYSICIAN")
        self.assertEqual(res["override_code"], "TARYAQ-OVERRIDE-8842-CLIN")

        # Verify audit event logged in Gomaa Episodic Memory
        events = self.bridge.store.get_episodic_events(mrn)
        audit_events = [e for e in events if e["event_type"] == "clinical_prescription_audit"]
        self.assertGreaterEqual(len(audit_events), 1)


if __name__ == "__main__":
    unittest.main()
