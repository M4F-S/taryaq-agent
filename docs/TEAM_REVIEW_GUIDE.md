# TARYAQ (ترياق) — TEAM REVIEW & EVALUATION GUIDE
**Dubai Create AI Agents Championship 2026–2027**  
**Category:** Best Societal AI Agent (Universal Clinical Decision Support & Pharmacovigilance Platform)  
**Founder & Lead Clinical Architect:** Mohamed Fathy, BPharm (Licensed Clinical Pharmacist & Systems Architect)  
**Active Repository:** `taryaq-agent`  
**Current Status:** **SPRINT-01 through SPRINT-04 COMPLETED & MERGED ON `main`**  
**Test Status:** **88/88 Rust Tests Passing** (`cargo test`) | **27/27 Python Tests Passing** | **36/36 C99 Checks Passing**

---

## 1. Executive Summary & Clinical Rationale

In high-volume ambulatory polypharmacy and community dispensing across the UAE, clinical decisions intersect with stringent statutory and pharmacovigilance mandates:
1. **Lethal Drug-Drug Interactions (DDIs):** Common co-prescriptions (such as *Simvastatin + Clarithromycin*, *Colchicine + Macrolides*, or *Metformin in acute renal impairment*) can lead to fatal rhabdomyolysis, multi-organ failure, or lactic acidosis. General-purpose cloud LLMs frequently hallucinate or fail closed unpredictably.
2. **Statutory Claim Cycle Scrubber (DHA SN 03/2019):** Prescriptions submitted through Dubai's `eClaimLink` post office face statutory delay penalties of **0.03% of Net Claim per calendar day of delay** after a 30-day grace period:
   $$\text{Penalty} = \max(0, \text{days} - 30) \times 0.0003 \times \text{Net Claim}$$
3. **Data Sovereignty (UAE Federal Law No. 2 of 2019 / UAE PDPL Law 45 of 2021):** Patient health data cannot leave sovereign edge hardware without explicit authorization.

**Taryaq (ترياق)** provides a 100% offline, deterministic, multi-modal clinical runtime linking a low-latency ISO C99 safety kernel via Rust FFI (<0.1 ms indexed query latency), an air-gapped clinical database, and an isolated Human-in-the-Loop (HITL) quarantine airlock.

---

## 2. Completed Milestones (Sprints 01–04)

Taryaq has achieved 100% completion across all its assigned standalone milestones:

```
[SPRINT-00] Master Harness & Gomaa Memory Sync        -> COMPLETED (Dubai/docs/CANONICAL_MASTER_SPECIFICATION_2026.md)
[SPRINT-01] Standalone Rust Engine & C99 FFI Bridge   -> COMPLETED (PR #5)
[SPRINT-02] Generalized Clinical Vessel (30 DDI pairs)-> COMPLETED (PR #7)
[SPRINT-03] Tri-Modal Voice Interfaces & Local Whisper -> COMPLETED (PR #8)
[SPRINT-04] HITL Web Scanner & Quarantine Buffer      -> COMPLETED (PR #9)
```

### Detailed Breakdown of Shipped Milestones:

### SPRINT-01: Standalone Rust Engine & C99 Native FFI Bridge (PR #5)
- Standalone Rust crate configured in `Cargo.toml` (`tokio`, `serde`, `rusqlite`, `thiserror`, `sha2`, `hmac`, `hex`).
- `build.rs` compiles C99 deterministic safety kernel (`src/taryaq_agent.c`, `adapters/taryaq_clinical_adapter.c`, `adapters/grounding_shield.c`) via `cc` with `-std=c99 -O2 -Wall -Wextra`.
- Native `extern "C"` FFI bindings (`src/safety/ffi.rs` & `src/safety/mod.rs`) connecting the immutable C99 safety engine to modern async Rust.

### SPRINT-02: Generalized Clinical Vessel & 30-Interaction Matrix (PR #7)
- **Database Engine (`corpus/taryaq_clinical.db`):** Generalized relational schema replacing toy hardcoded lists with an unlimited clinical vessel:
  - `formulary_drugs`: 140 active substances with canonical/Arabic names, ATC codes, CYP/P-gp profiles, renal eGFR cutoffs, hepatic metabolism %, and UAE Law 30/2021 schedules.
  - `drug_interactions`: 46 validated clinical interaction pairs including 30 high-yield contraindication/major pairs with mechanisms, clinical actions, and citations.
  - `medical_conditions`: 20 ICD-10/11 mapped clinical conditions with emergency red-flag priorities.
  - `lab_diagnostic_panels`: 16 LOINC-coded laboratory panels with physiological reference and critical panic limits.
- **Fail-Closed Gate:** Unmodeled medications or unmodeled drug pairs fail closed to `UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW`.
- **Query Performance:** Sub-millisecond indexed lookup latency (<0.1 ms per interaction check).

### SPRINT-03: Tri-Modal Clinical Interfaces & Local Voice Pipeline (PR #8)
- **1. Clinician Ambient Copilot (`src/trimodal/copilot.rs`):**
  - Ambient bilingual dialogue intake (spoken Arabic dialects + medical English code-switching).
  - Trace-linked SOAP note generation ("agent drafts, clinician signs").
  - Ranked Differential Diagnosis (DDx) generator with evidence weights.
  - Pre-order Clinical Decision Support (CDS) interrupt blocking contraindicated regimens before signature.
- **2. Pharmacist Verification Console (`src/trimodal/pharmacist.rs`):**
  - Comprehensive polypharmacy screening across active patient profiles.
  - DHA Standards Notice SN 03/2019 statutory claim delay penalty calculator.
  - Single-use clinician override protocol with HTTP 409 replay attack prevention.
  - Append-only cryptographic HMAC-SHA256 audit ledger (`taryaq_audit.jsonl`).
- **3. Patient Health Companion (`src/trimodal/companion.rs`):**
  - Dialectal conversational intake supporting Gulf (خليجي), Egyptian (مصري), and Levantine (شامي) Arabic.
  - Emergency red-flag intercept (acute chest pain, stroke FAST, severe dyspnea, hemorrhage) triggering immediate 998/999 ambulance dispatch.
  - Plain-language posology counseling (administration timing, food/drink precautions, missed doses).
- **4. Local Voice Audio Pipeline (`src/voice/mod.rs`):**
  - Ingestion of local `.wav`, `.ogg`, and `.mp3` audio recordings.
  - Offline transcription with container format header validation and automated routing to Clinician Copilot, Pharmacist Console, or Patient Companion.

### SPRINT-04: HITL Web Ingestion & Quarantine Airlock (PR #9)
- **Background Pharmacovigilance Scanner (`src/quarantine/scanner.rs`):**
  - Ingests public drug safety circulars from OpenFDA, PubMed, WHO, DHA, and UAE MOHAP.
  - Computes trace-linked SHA-256 evidence provenance hashes (`compute_evidence_sha256()`).
  - Strict air-gapped isolation: candidate findings stage exclusively into `hitl_clinical_quarantine` with status `PENDING` and zero live table egress.
- **The Pharmacist Gate (`src/quarantine/reviewer.rs`):**
  - Licensed reviewer authentication (`Mohamed Fathy, BPharm (Licensed Clinical Pharmacist & Systems Architect)`).
  - Strict credentialing boundaries: rejects empty or unauthenticated credentials (`HitlError::UnauthorizedReviewer`).
  - State machine transition guards: rejects re-approval or re-rejection of processed records (`HitlError::InvalidTransition`).
  - Atomic promotion: upon approval, ensures drug entities exist in `formulary_drugs` and atomically inserts the rule into production `drug_interactions` inside an atomic SQLite transaction.
  - Pharmacist rejection workflow: records marked `REJECTED` with statutory reason notes; live clinical tables remain 100% untouched.

---

## 3. Repository Directory Structure

```
taryaq-agent/
├── Cargo.toml                 # Standalone Rust crate configuration (v0.1.0)
├── Cargo.lock                 # Pinned deterministic dependency lockfile
├── build.rs                   # Native C99 compiler script linking libdubai_adapters
├── Makefile                   # C99 build harness and verification targets
├── README.md                  # High-level architecture and project overview
├── PLAN.md                    # Detailed milestone roadmap and sprint tracking
├── CANONICAL_PRD_POINTER.md   # Pointer to the immutable championship PRD
├── adapters/                  # ISO C99 clinical safety adapters and bridges
│   ├── taryaq_clinical_adapter.c
│   ├── taryaq_rag_engine.c
│   └── grounding_shield.c
├── corpus/                    # Clinical databases and reference knowledge
│   ├── taryaq_clinical.db     # Production SQLite Clinical Vessel (140 drugs, 30 DDI)
│   ├── taryaq_corpus.db       # FTS5 statutory knowledge base (DHA SN 03/2019, Law 30)
│   ├── manifest.json          # Cryptographic SHA-256 manifest of all corpus files
│   └── manifest.sha256
├── docs/                      # Canonical specifications, PRD, and review documentation
│   ├── TEAM_REVIEW_GUIDE.md   # This comprehensive evaluation guide
│   ├── CANONICAL_MASTER_SPECIFICATION_2026.md # Full master specification for championship
│   ├── PRD_CANONICAL_SPEC_2026.md             # Canonical PRD document
│   ├── NUMBER_TRACEABILITY_MATRIX.md          # Statutory numbers and formula traceability
│   ├── clinical/              # Clinical rules and evidence provenance
│   └── spec/                  # Technical specifications
├── src/                       # Rust source code
│   ├── lib.rs                 # Library entrypoint and module exports
│   ├── main.rs                # CLI suite and persistent daemon runtime
│   ├── clinical/              # Clinical database engine and dynamic screening
│   │   ├── db.rs              # SQLite vessel operations, atomic promotion, pair checks
│   │   └── mod.rs
│   ├── quarantine/            # HITL web scanner and The Pharmacist Gate
│   │   ├── scanner.rs         # Alert ingestion (OpenFDA, PubMed, MOHAP, DHA, WHO)
│   │   ├── reviewer.rs        # Licensed pharmacist review and atomic promotion
│   │   └── mod.rs
│   ├── safety/                # C99 FFI safety kernel bindings
│   │   ├── ffi.rs             # Native extern "C" ABI signatures
│   │   └── mod.rs
│   ├── trimodal/              # Tri-modal clinical interfaces
│   │   ├── copilot.rs         # Clinician Ambient Copilot (SOAP + DDx)
│   │   ├── pharmacist.rs      # Pharmacist Verification Console (DHA claims + DDI)
│   │   ├── companion.rs       # Patient Health Companion (Dialectal Arabic + 998 triage)
│   │   ├── audit.rs           # HMAC-SHA256 append-only audit chaining
│   │   └── mod.rs
│   ├── voice/                 # Offline local voice transcription & routing
│   │   └── mod.rs
│   └── taryaq_agent.c         # ISO C99 deterministic safety core
├── tests/                     # Automated unit and integration test batteries
│   ├── c99_bridge_test.rs     # 21 Rust unit tests verifying C99 FFI bindings
│   ├── clinical_interactions_test.rs # 40 tests verifying DDI matrix & fail-closed gates
│   ├── trimodal_test.rs       # 14 tests verifying Copilot, Pharmacist, Companion & Voice
│   ├── quarantine_test.rs     # 12 tests verifying Web Scanner & Pharmacist Gate
│   ├── test_patient_timeline.py # Longitudinal timeline trajectory tests
│   ├── test_clinical_rag.py   # DHA SN 03/2019 statutory RAG tests
│   ├── test_audit_tamper.py   # HMAC audit ledger anti-tampering verification
│   └── fixtures/              # Audio test fixtures (.wav)
└── tools/                     # Verification and audit utilities
    └── verify_audit.py        # Python HMAC-SHA256 cryptographic ledger verifier
```

---

## 4. Verification & Testing Quickstart

### Running the Rust Test Suite (88 Tests Passing)
```bash
cargo test
```
**Expected Output:**
- `safety::ffi::tests`: 1 passed
- `tests/c99_bridge_test.rs`: 21 passed
- `tests/clinical_interactions_test.rs`: 40 passed
- `tests/quarantine_test.rs`: 12 passed
- `tests/trimodal_test.rs`: 14 passed
- **Total: 88 passed, 0 failed.**

### Running Python Integration & Cryptographic Audit Tests (27 Tests Passing)
```bash
# Verify HMAC-SHA256 audit ledger anti-tampering
python3 tests/test_audit_tamper.py

# Verify longitudinal patient timeline & CKD progression
python3 tests/test_patient_timeline.py

# Verify DHA statutory clinical RAG
python3 tests/test_clinical_rag.py
```

### Running C99 Baseline Acceptance Checks (36 Tests Passing)
```bash
make test
make audit-verify
```

---

## 5. CLI Command Reference

The `taryaq_agent_daemon` (aliased as `taryaq`) exposes interactive commands for all clinical workflows:

### 1. Clinician Ambient Copilot
```bash
cargo run --bin taryaq_agent_daemon -- copilot \
  --dialogue "Patient: عندي عوار بالصدر وضيق تنفس. Doctor: BP 160/95" \
  --egfr 25.0 \
  --drugs "Metformin, Lisinopril"
```

### 2. Pharmacist Verification Console
```bash
cargo run --bin taryaq_agent_daemon -- pharmacist \
  --meds "Colchicine, Clarithromycin" \
  --egfr 45.0 \
  --claim 2500.0 \
  --delay 45
```

### 3. Patient Health Companion (Emergency Triage)
```bash
cargo run --bin taryaq_agent_daemon -- companion \
  --message "حاسس بسكاكين في صدري ومش قادر أتنفس خالص"
```

### 4. Local Voice Transcription & Routing
```bash
cargo run --bin taryaq_agent_daemon -- voice-transcribe \
  tests/fixtures/test_ambient_dialogue.wav \
  --route copilot
```

### 5. HITL Web Quarantine & Pharmacist Gate
```bash
# Ingest new circulars from PubMed, OpenFDA, MOHAP, DHA, or WHO
cargo run --bin taryaq_agent_daemon -- quarantine scan --source mohap

# List pending quarantine findings
cargo run --bin taryaq_agent_daemon -- quarantine list --status PENDING

# Inspect candidate finding and parsed interaction payload
cargo run --bin taryaq_agent_daemon -- quarantine inspect <id>

# Licensed pharmacist approval and atomic promotion to production
cargo run --bin taryaq_agent_daemon -- quarantine approve <id> \
  --reviewer "Mohamed Fathy, BPharm"

# Pharmacist rejection with clinical justification
cargo run --bin taryaq_agent_daemon -- quarantine reject <id> \
  --reason "Insufficient randomized trial validation"
```

---

## 6. Statutory Numbers & Traceability Reference

All calculations across Taryaq are codified strictly against official UAE and Dubai statutory frameworks:

| Metric / Regulation | Statutory Source | Mathematical Codification in Taryaq |
| :--- | :--- | :--- |
| **Claim Delay Penalty** | DHA Standards Notice SN 03/2019 (§3.7.2, §3.8.4) | $\text{Penalty} = \max(0, \text{days} - 30) \times 0.03\% \times \text{Net Claim}$ |
| **Maximum Claim Submission Lifecycle** | DHA SN 03/2019 (§3.9.3) | Strict 141 calendar days ceiling from date of service |
| **Metformin Renal Cutoff** | KDIGO 2024 / ADA Standards | Absolute contraindication if $\text{eGFR} < 30 \text{ mL/min/1.73m}^2$; 50% dose reduction for $30 \le \text{eGFR} < 45$ |
| **Simvastatin Macrolide Contraindication** | FDA / MOHAP Drug Safety Advisory | Fatal CYP3A4 inhibition; severe rhabdomyolysis and acute renal failure |
| **Colchicine Macrolide Contraindication** | UAE MOHAP Circular No. 18/2026 | Dual CYP3A4 + P-gp inhibition; multi-organ failure and bone marrow aplasia |
| **Controlled Substances Gate** | UAE Federal Decree-Law No. 30 of 2021 | Schedule I–IV controlled medications require verified physician ID + single-use override |
| **Single-Use Override Replay Prevention** | Dubai eClaimLink Security Architecture | Consumed override tokens are invalidated immediately; replay attacks return HTTP 409 Conflict |
| **Cryptographic Audit Ledger** | DESC ISR v3.1 Domain 11 | Append-only HMAC-SHA256 ledger chaining $\text{MAC}_i = \text{HMAC}(K, \text{MAC}_{i-1} \parallel \text{canonical}_i)$ |
| **Data Sovereignty & Air-Gap** | UAE Federal Law No. 2 of 2019 / UAE PDPL Law 45 | 100% on-device local execution; zero patient health telemetry egress |

---

## 7. Next Steps in Championship Execution

With TARYAQ's core standalone engine, clinical vessel, tri-modal voice interfaces, and HITL quarantine airlock fully complete and verified on `main`:
- **QALA (Agent 2 - Enterprise OS):** SPRINT-05 and SPRINT-06 are complete and merged.
- **NIBRAS (Agent 3 - Personal OS):** SPRINT-07 (Civic Navigator) is currently in execution.
- **Full Integration (SPRINT-09):** All three sovereign agents will be audited on the VPS Hermes Review Fleet (`187.124.2.26`) prior to final submission.
