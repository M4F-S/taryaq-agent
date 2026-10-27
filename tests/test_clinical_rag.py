#!/usr/bin/env python3
"""tests/test_clinical_rag.py - Deterministic Verification Suite for Clinical RAG & DHA Grounding
Target: Round 3 Clinical RAG | Dubai Create AI Agents Championship 2026-2027
Pure Python 3 Standard Library (Zero pip dependencies).
"""

import sys
import os
import json
import sqlite3
import hashlib
import tempfile
import subprocess
import unittest

ROOT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CORPUS_DIR = os.path.join(ROOT_DIR, "corpus")
MANIFEST_PATH = os.path.join(CORPUS_DIR, "manifest.json")
DB_PATH = os.path.join(CORPUS_DIR, "taryaq_corpus.db")
AGENT_BIN = os.path.join(ROOT_DIR, "build", "taryaq_agent")
VERIFY_SCRIPT = os.path.join(ROOT_DIR, "tools", "verify_audit.py")
TEST_KEY = "test-key-32-bytes-minimum-000000"

os.environ.setdefault("TARYAQ_AUDIT_KEY", TEST_KEY)


def sha256_file(path: str) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while chunk := f.read(65536):
            h.update(chunk)
    return h.hexdigest()


class TestClinicalRAG(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Ensure corpus index is built
        subprocess.run([sys.executable, os.path.join(ROOT_DIR, "tools", "build_corpus_index.py")],
                       cwd=ROOT_DIR, check=True)
        if not os.path.isfile(AGENT_BIN):
            subprocess.run(["make", "all"], cwd=ROOT_DIR, check=True)

    def test_01_corpus_manifest_and_db_integrity(self):
        """Verifies corpus files, manifest.json, and SQLite FTS5 database exist and hashes match."""
        self.assertTrue(os.path.isfile(MANIFEST_PATH), "manifest.json missing")
        self.assertTrue(os.path.isfile(DB_PATH), "taryaq_corpus.db missing")

        with open(MANIFEST_PATH, "r", encoding="utf-8") as f:
            manifest = json.load(f)

        self.assertIn("documents", manifest)
        self.assertIn("chunks", manifest)

        # Check DHA SN 03/2019 document
        self.assertIn("DHA_SN_03_2019", manifest["documents"])
        dha_file = os.path.join(CORPUS_DIR, manifest["documents"]["DHA_SN_03_2019"]["file"])
        self.assertEqual(sha256_file(dha_file), manifest["documents"]["DHA_SN_03_2019"]["sha256"])

        # Check §3.7.2 chunk exists
        self.assertIn("DHA_SN_03_2019_3_7_2", manifest["chunks"])
        chunk_372 = manifest["chunks"]["DHA_SN_03_2019_3_7_2"]
        self.assertEqual(chunk_372["doc_id"], "DHA_SN_03_2019")
        self.assertEqual(chunk_372["section"], "§3.7.2")
        self.assertEqual(len(chunk_372["passage_sha256"]), 64)

    def test_02_sqlite_fts5_bm25_retrieval(self):
        """Tests SQLite FTS5 top-k BM25 retrieval for DHA late-submission query."""
        conn = sqlite3.connect(DB_PATH)
        cur = conn.cursor()

        query = "late OR submission OR penalty"
        cur.execute("""
            SELECT c.doc_id, c.section, c.title, c.passage_sha256, bm25(corpus_fts)
            FROM corpus_fts
            JOIN corpus_chunks c ON c.doc_id = corpus_fts.doc_id AND c.section = corpus_fts.section
            WHERE corpus_fts MATCH ?
            ORDER BY bm25(corpus_fts) ASC
            LIMIT 1;
        """, (query,))
        row = cur.fetchone()
        conn.close()

        self.assertIsNotNone(row, "FTS5 query returned no results")
        doc_id, section, title, passage_hash, score = row
        self.assertEqual(doc_id, "DHA_SN_03_2019")
        self.assertEqual(section, "§3.7.2")
        self.assertIn("Penalty", title)

    def test_03_rag_demo_execution(self):
        """Executes --rag-demo and verifies quoted statutory clause, citation, and math."""
        cmd = [AGENT_BIN, "--rag-demo"]
        res = subprocess.run(cmd, capture_output=True, text=True)
        self.assertEqual(res.returncode, 0, f"--rag-demo failed: {res.stderr}")

        # Check key components of demo output
        self.assertIn("how is the late-submission penalty calculated?", res.stdout)
        self.assertIn("DHA_SN_03_2019", res.stdout)
        self.assertIn("§3.7.2", res.stdout)
        self.assertIn("VERIFIED (Cryptographic SHA-256 Match)", res.stdout)
        self.assertIn("GROUNDING_VERIFIED", res.stdout)
        self.assertIn("max(0, 42 - 30) = 12 overdue day(s)", res.stdout)
        self.assertIn("450.00 AED * 0.0036 = 1.62 AED", res.stdout)

    def test_04_statutory_citation_in_audit_record(self):
        """Verifies that acceptance test generates audit records with attached statutory citations."""
        with tempfile.NamedTemporaryFile(suffix=".jsonl", delete=False) as f:
            temp_audit = f.name

        try:
            cmd = [AGENT_BIN, "--test", "--audit-file", temp_audit]
            res = subprocess.run(cmd, capture_output=True, text=True)
            self.assertEqual(res.returncode, 0, f"--test failed: {res.stderr}")

            # Verify audit file contains statutory citation
            with open(temp_audit, "r", encoding="utf-8") as f:
                lines = [json.loads(line) for line in f if line.strip()]

            self.assertGreaterEqual(len(lines), 1)
            committed = None
            for rec in lines:
                if rec.get("action") == "PRESCRIPTION_COMMITTED":
                    committed = rec
                    break

            self.assertIsNotNone(committed, "No committed prescription found")
            self.assertIn("statutory_doc_id", committed)
            self.assertIn("statutory_section", committed)
            self.assertIn("statutory_passage_sha256", committed)
            self.assertEqual(len(committed["statutory_passage_sha256"]), 64)

            # Verify canonical format contains 16 fields
            canonical = committed.get("canonical", "")
            parts = canonical.split("|")
            self.assertEqual(len(parts), 16, f"Expected 16 canonical fields, got {len(parts)}: {canonical}")
            self.assertEqual(parts[13], committed["statutory_doc_id"])
            self.assertEqual(parts[14], committed["statutory_section"])
            self.assertEqual(parts[15], committed["statutory_passage_sha256"])

            # Verify tools/verify_audit.py passes cleanly
            v_res = subprocess.run([sys.executable, VERIFY_SCRIPT, "--ledger", temp_audit],
                                   capture_output=True, text=True)
            self.assertEqual(v_res.returncode, 0, f"verify_audit.py failed: {v_res.stderr}")
            self.assertIn("records verified", v_res.stdout)
        finally:
            if os.path.exists(temp_audit):
                os.remove(temp_audit)

    def test_05_tampered_passage_hash_fails_closed(self):
        """Mutating statutory_passage_sha256 in audit ledger triggers fail-closed tampering detection."""
        with tempfile.NamedTemporaryFile(suffix=".jsonl", delete=False) as f:
            temp_audit = f.name

        try:
            cmd = [AGENT_BIN, "--test", "--audit-file", temp_audit]
            res = subprocess.run(cmd, capture_output=True, text=True)
            self.assertEqual(res.returncode, 0)

            with open(temp_audit, "r", encoding="utf-8") as f:
                lines = [json.loads(line) for line in f if line.strip()]

            # Find committed record and corrupt passage hash
            for rec in lines:
                if rec.get("action") == "PRESCRIPTION_COMMITTED":
                    rec["statutory_passage_sha256"] = "deadbeef" * 8
                    break

            with open(temp_audit, "w", encoding="utf-8") as f:
                for rec in lines:
                    f.write(json.dumps(rec) + "\n")

            v_res = subprocess.run([sys.executable, VERIFY_SCRIPT, "--ledger", temp_audit],
                                   capture_output=True, text=True)
            self.assertEqual(v_res.returncode, 1, "Verifier allowed corrupted passage hash!")
            self.assertIn("tampering", v_res.stderr.lower())
        finally:
            if os.path.exists(temp_audit):
                os.remove(temp_audit)


if __name__ == "__main__":
    unittest.main()
