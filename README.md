# Taryaq (ترياق) — Sovereign Clinical Decision Support & Pharmacovigilance Engine
## Best Societal AI Agent | Dubai Create AI Agents Championship 2026–2027

---

### 1. Executive Summary & Problem Statement
In outpatient ambulatory clinics and community pharmacies across Dubai, clinical safety and insurance administration intersect under severe constraints:
1. **Severe Adverse Drug Events in Chronic Polypharmacy:** High-risk combinations (such as Colchicine + Clarithromycin in patients with mild-to-moderate CKD, eGFR < 60 mL/min) produce acute neurotoxicity and fatal bone marrow suppression. Probabilistic cloud LLM assistants often hallucinate or fail to catch these deterministically.
2. **Statutory Claim Cycle Penalties (DHA SN 03/2019):** Healthcare providers operating on Dubai's `eClaimLink` post office face statutory delay penalties of **0.03% of Net Claimed amount per calendar day of delay** (§3.7.2, §3.8.4).
3. **Data Sovereignty Restrictions:** **Article 13 of UAE Federal Law No. 2 of 2019** prohibits cross-border transfer or foreign cloud storage of patient health information without explicit ministerial exemption.

**Taryaq** solves this via a bare-metal, air-gapped ISO C99 deterministic clinical gate (~20 to 50 µs per in-memory check, sub-3 ms full acceptance suite on Apple Silicon / modern x86_64 hosts) and an HMAC-SHA256 chained audit ledger verifiable with `make audit-verify`.

---

### 2. Architecture & Shipped Core Differentiators
The core engine is built in ISO C99 with zero external network dependencies:
- **Fail-Closed Clinical Safety Core (`adapters/taryaq_clinical_adapter.c`):**
  Any drug combination not explicitly verified in the clinical interaction matrix returns `UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW` rather than `CLINICAL_SAFE`.
  Shipped interaction rules enforce explicit contraindications:
  - *Colchicine + Clarithromycin under renal impairment (eGFR < 60 mL/min):* Fatal CYP3A4/P-gp inhibition contraindication.
  - *Simvastatin + Amlodipine:* Enforces maximum 20 mg ceiling to prevent rhabdomyolysis via CYP3A4 inhibition.
  - *Metformin in renal impairment (eGFR < 30 mL/min):* Contraindicated due to fatal lactic acidosis risk.
  - *Lisinopril + Losartan (ACEi + ARB):* Flagged for hyperkalemia and acute kidney injury from dual renin-angiotensin-aldosterone system (RAAS) blockade.
- **Statutory DHA SN 03/2019 Claims Cycle Scrubber:**
  Calculates statutory delay penalties on Dubai `eClaimLink` (0.03% of net claim per day, §§3.7.1, 3.7.2, 3.8.1, 3.8.4, 3.9.3) before submission.
- **Emergency Red-Flag Patient Triage:**
  Air-gapped triage gate classifying emergency symptoms (Arabic and English: chest pain, dyspnea, acute neurological deficits) for immediate human transfer.
- **Longitudinal Patient Timeline & Polypharmacy Trajectory (`src/taryaq_agent.c`):**
  Zero-dependency longitudinal encounter tracking (`taryaq_encounters.jsonl`) recording patient IDs, consultation dates, prescribed drugs, doses, eGFR values, clinical verdicts, and override tokens:
  - Chronological eGFR progression and CKD staging with ASCII trajectory visualization (`timeline <patient_id>`).
  - History-aware repeat override escalation (`is_repeat_override` flag) that flags recurrent overrides for identical drug-drug interactions.
  - Interactive demonstration with 6 realistic clinical encounters (`make timeline-demo`).
- **Deterministic Clinical RAG & DHA Statutory Grounding Engine (`adapters/taryaq_rag_engine.c`, `corpus/`):**
  Air-gapped SQLite FTS5 BM25 retrieval engine over DHA Standards Notice SN 03/2019, sovereign drug monographs, and UAE Federal Law No. 30 of 2021 schedules:
  - Corrects claim cycle penalty calculation to enforce the statutory 30-day window: $\max(0, \text{days} - 30) \times 0.03\% \times \text{Net Claim}$.
  - Cites `doc_id`, `section`, and cryptographic `passage_sha256` on every verdict.
  - Hard-blocks any prescription or claim referencing a corrupted or tampered passage hash.
  - Interactive statutory query demonstration via `make rag-demo`.
- **HMAC-SHA256 Cryptographic Audit Chaining (`src/taryaq_agent.c`, `tools/verify_audit.py`):**
  Every audit record is cryptographically linked to the previous record's MAC:
  $$\text{MAC}_i = \text{HMAC-SHA256}(K, \text{MAC}_{i-1} \parallel \text{"\\n"} \parallel \text{canonical}_i)$$
  Tampering with any clinical record, eGFR, or payload breaks the chain, detected by `make audit-verify`.

---

### 3. Built On
- **New Components:** Sovereign clinical decision support rules (`adapters/taryaq_clinical_adapter.c`), DHA SN 03/2019 claim scrubber, emergency red-flag triage gate, and cryptographic HMAC-SHA256 audit chaining (`src/taryaq_agent.c`, `tools/verify_audit.py`).
- **Reused Components:** Belya C99 autonomous safety core (deterministic execution engine, sandboxing, and memory safety invariants).
- **Optional Integration:** gomaa Memory OS (`adapters/taryaq_gomaa_bridge.py`) for longitudinal patient memory graphs.

---

### 4. Optional Integration: gomaa
Taryaq integrates optionally with **gomaa** (`https://github.com/M4F-S/gomaa`), an autonomous memory OS with associative retrieval and Ebbinghaus decay:
- Path resolved via `GOMAA_HOME` environment variable (defaults to `~/gomaa`).
- If `gomaa` is not installed or available, the test suite skips gomaa tests cleanly (`OK (skipped=5)`), and the core C99 clinical safety engine operates 100% self-contained.

---

### 5. Production Shipped Components (Standalone Rust + C99 FFI)
All core clinical interfaces and statutory engines are fully implemented and verified in the standalone Rust runtime (`v0.1.0`):
1. **Clinician Ambient Copilot (`src/trimodal/copilot.rs`):**
   - Ambient bilingual consultation listening (code-switching from spoken Arabic dialects to medical English).
   - Trace-linked SOAP note generation ("agent drafts, clinician signs").
   - Ranked Differential Diagnosis (DDx) generator with evidence weights.
   - Pre-order CDS interrupt blocking contraindicated regimens before signature.
2. **Pharmacist Verification Console (`src/trimodal/pharmacist.rs`):**
   - Polypharmacy interaction screening across full active patient regimens.
   - Statutory DHA SN 03/2019 claims delay penalty calculation ($\max(0, \text{days}-30) \times 0.03\% \times \text{Net Claim}$).
   - Single-use clinician override governance with HTTP 409 replay attack prevention.
   - Append-only HMAC-SHA256 cryptographic audit ledger (`taryaq_audit.jsonl`).
3. **Patient Health Companion (`src/trimodal/companion.rs`):**
   - Conversational intake in Gulf, Egyptian, and Levantine Arabic.
   - Emergency red-flag intercept (acute chest pain, stroke FAST, severe dyspnea, hemorrhage) triggering immediate 998/999 ambulance dispatch.
   - Plain-language posology and lifestyle counseling.
4. **Local Voice Audio Pipeline (`src/voice/mod.rs`):**
   - Local `.wav`, `.ogg`, `.mp3` ingestion and offline transcription with automated routing to Copilot, Pharmacist, or Companion.
5. **Human-in-the-Loop Quarantine Airlock (`src/quarantine/`):**
   - Background pharmacovigilance scanner ingesting circulars from OpenFDA, PubMed, WHO, DHA, and UAE MOHAP.
   - Cryptographic SHA-256 evidence provenance hashing.
   - Strict air-gap: candidate findings stage in `hitl_clinical_quarantine` with status `PENDING`.
   - The Pharmacist Gate: Licensed pharmacist verification (`Mohamed Fathy, BPharm`) with atomic promotion into live `drug_interactions` table.

---

### 6. Build & Test Instructions

#### Rust Engine & Full Test Battery (88/88 Tests Passing)
```bash
# Build standalone binary and library
cargo build --release

# Run complete Rust test suite (88 tests passing)
cargo test

# Run interactive CLI commands
cargo run --bin taryaq_agent_daemon -- copilot --dialogue "عندي عوار بالصدر" --egfr 25.0
cargo run --bin taryaq_agent_daemon -- pharmacist --meds "Colchicine, Clarithromycin" --claim 2500.0 --delay 45
cargo run --bin taryaq_agent_daemon -- quarantine list --status PENDING
```

#### C99 Safety Kernel & Python Suites (27/27 Tests Passing)
```bash
# Build standalone C binary and run deterministic verification
make all
make test

# Verify cryptographic HMAC audit chain
make audit-verify

# Run realistic 6-encounter longitudinal timeline demo with CKD progression
make timeline-demo

# Run Clinical RAG & DHA statutory grounding demo
make rag-demo
```

---

### 7. Documentation & Reference Links
- [Team Review Guide](file:///Users/mohamedfathy/Desktop/taryaq-agent/docs/TEAM_REVIEW_GUIDE.md) — Comprehensive guide for evaluators and teammates.
- [Canonical Master Specification 2026](file:///Users/mohamedfathy/Desktop/taryaq-agent/docs/CANONICAL_MASTER_SPECIFICATION_2026.md) — Unified statutory specification.
- [Canonical PRD](file:///Users/mohamedfathy/Desktop/taryaq-agent/docs/PRD_CANONICAL_SPEC_2026.md) — Official product requirements.
- [Sprint Roadmap & Execution Plan](file:///Users/mohamedfathy/Desktop/taryaq-agent/PLAN.md) — SPRINT-01 through SPRINT-04 execution record.
