# TARYAQ (ترياق) — STANDALONE RUST REPOSITORY SPRINT PLAN & EXECUTION RECORD
**Dubai Create AI Agents Championship 2026–2027**  
**Category:** Best Societal AI Agent  
**Founder & Clinical Lead:** Mohamed Fathy, BPharm (Licensed Clinical Pharmacist & Systems Architect)  
**Status:** **ALL TARYAQ SPRINTS (01–04) COMPLETED & MERGED ON `main`**

---

## 1. Ground Rules & Sovereign Invariants

1. **Standalone Rust Architecture:** Standalone Rust crate `taryaq` with native ISO C99 FFI safety bridge.
2. **Clinical Vessel Principle:** Uses a generalized SQLite relational schema (`corpus/taryaq_clinical.db`) designed for unlimited drugs, ICD-10/11 diseases, LOINC panels, and quarantine staging—never hardcoded toy lists.
3. **Fail-Closed Safety Gate:** Any unmodeled drug or unmodeled drug-drug combination fails closed to `UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW`.
4. **Air-Gapped Sovereign Privacy:** 100% offline-first execution compliant with UAE Federal Law No. 2 of 2019 and UAE PDPL (Federal Decree-Law No. 45 of 2021). Zero patient telemetry egress.
5. **The Tri-Mirror Law:** Local Desktop $\equiv$ Google Drive (`Dubai_AI_Championship_2027/taryaq-agent`) $\equiv$ GitHub Remotes (`M4F-S/taryaq-agent`).
6. **Branch Protection Law:** `enforce_admins=true`. Zero direct pushes to `main`. All changes must pass automated test suites and merge via Pull Request.

---

## 2. Sprint Roadmap & Execution Summary

| Sprint ID | Milestone Code | Scope & Deliverables | PR & Status | Test Evidence |
| :--- | :---: | :--- | :---: | :---: |
| **SPRINT-00** | `M-00` | Master Harness & Gomaa Memory Setup | Merged in `Dubai` | Schema initialized in `~/.gomaa/gomaa.db` |
| **SPRINT-01** | `M-01` | Rust Engine Skeleton & C99 FFI Bridge | **Merged (PR #5)** | 21 C99 unit tests passing via Rust FFI |
| **SPRINT-02** | `M-02` | Empty Clinical Vessel & 30 DDI Matrix | **Merged (PR #7)** | 40 DDI and schema tests passing (<0.1 ms latency) |
| **SPRINT-03** | `M-03` | Tri-Modal Interfaces & Voice Pipeline | **Merged (PR #8)** | 14 Tri-Modal & Whisper tests passing |
| **SPRINT-04** | `M-04` | HITL Web Ingestion & Quarantine Airlock | **Merged (PR #9)** | 12 Quarantine & Pharmacist Gate tests passing |

**Total Test Suite Status:** **88/88 Rust tests passing** (`cargo test`) | **27/27 Python tests passing** | **36/36 C99 checks passing** (`make test`).

---

## 3. Detailed Milestone Logs

### SPRINT-01: Rust Engine Skeleton & C99 FFI Bridge
- [x] Initialized standalone Rust crate `taryaq` in `Cargo.toml`.
- [x] Implemented `build.rs` to compile C99 deterministic safety core (`src/taryaq_agent.c`, `adapters/taryaq_clinical_adapter.c`, `adapters/grounding_shield.c`) into `libdubai_adapters.a`.
- [x] Created `src/safety/ffi.rs` with `extern "C"` bindings for `clinical_db_init`, `verify_prescription_and_claim`, `triage_patient_symptoms`, and struct alignment checks.
- [x] Verified zero memory slicing across ABI boundaries.
- [x] Created `tests/c99_bridge_test.rs` executing 21 automated safety checks.

### SPRINT-02: Empty Clinical Vessel & 30-Interaction Matrix
- [x] Implemented production relational database `corpus/taryaq_clinical.db`:
  - `formulary_drugs`: 140 active ingredients with canonical names, Arabic names, ATC codes, CYP/P-gp profiles, eGFR cutoffs, Law 30/2021 schedules.
  - `drug_interactions`: 46 validated clinical interaction pairs including 30 high-yield contraindications/major warnings.
  - `medical_conditions`: 20 ICD-10/11 mapped conditions with red-flag flags.
  - `lab_diagnostic_panels`: 16 LOINC laboratory panels with physiological and critical panic limits.
  - `hitl_clinical_quarantine`: Staging table for incoming alert communications.
- [x] Built fail-closed screening engine in `src/clinical/db.rs` (`check_interaction_pair`, `screen_patient_profile`).
- [x] Commutative order invariance verified across all pairs: $\text{Check}(A, B) \equiv \text{Check}(B, A)$.
- [x] Created `tests/clinical_interactions_test.rs` with 40 comprehensive tests verifying indexed query latency $<0.1\text{ ms}$.

### SPRINT-03: Tri-Modal Clinical Interfaces & Voice Pipeline
- [x] **Clinician Ambient Copilot (`src/trimodal/copilot.rs`):**
  - Ambient bilingual dialogue intake (spoken Arabic dialects + medical English).
  - Trace-linked SOAP note generation with timestamped citations.
  - Ranked Differential Diagnosis (DDx) generator with evidence weights.
  - Pre-order CDS interrupt blocking contraindicated co-prescriptions.
- [x] **Pharmacist Verification Console (`src/trimodal/pharmacist.rs`):**
  - Polypharmacy interaction screening across full active regimens.
  - DHA Standards Notice SN 03/2019 claims delay penalty calculation: $\max(0, \text{days}-30) \times 0.03\% \times \text{Net Claim}$.
  - Single-use clinician override validation with HTTP 409 replay attack prevention.
  - Append-only HMAC-SHA256 cryptographic audit ledger (`taryaq_audit.jsonl`).
- [x] **Patient Health Companion (`src/trimodal/companion.rs`):**
  - Dialectal conversational intake for Gulf (خليجي), Egyptian (مصري), and Levantine (شامي) Arabic.
  - Emergency red-flag intercept (acute chest pain, stroke FAST, severe dyspnea, hemorrhage) triggering immediate 998/999 ambulance dispatch.
  - Plain-language posology and lifestyle counseling.
- [x] **Local Voice Audio Pipeline (`src/voice/mod.rs`):**
  - Ingestion of `.wav`, `.ogg`, and `.mp3` audio recordings.
  - Offline transcription with container format header validation and automated routing to Copilot, Pharmacist, or Companion.
- [x] Created `tests/trimodal_test.rs` with 14 automated integration tests.

### SPRINT-04: HITL Web Ingestion & Quarantine Airlock
- [x] **Pharmacovigilance Web Alert Scanner (`src/quarantine/scanner.rs`):**
  - Ingests circulars from OpenFDA, PubMed, WHO, DHA, and UAE MOHAP.
  - Computes trace-linked SHA-256 evidence provenance hashes (`compute_evidence_sha256()`).
  - Strict air-gapped isolation: candidate findings stage into `hitl_clinical_quarantine` with status `PENDING` and zero live table egress.
- [x] **The Pharmacist Gate (`src/quarantine/reviewer.rs`):**
  - Licensed reviewer authentication (`Mohamed Fathy, BPharm (Licensed Clinical Pharmacist & Systems Architect)`).
  - Strict credentialing boundaries: rejects empty or unauthenticated credentials (`HitlError::UnauthorizedReviewer`).
  - State machine transition guards: rejects re-approval or re-rejection of processed records (`HitlError::InvalidTransition`).
  - Atomic promotion workflow: validates clinical evidence, admits candidate entities into `formulary_drugs`, and commits the interaction into `drug_interactions` inside an atomic SQLite transaction.
  - Pharmacist rejection workflow: records marked `REJECTED` with statutory reason notes; live clinical tables remain untouched.
- [x] CLI commands added to `src/main.rs`: `taryaq quarantine list/inspect/scan/approve/reject`.
- [x] Created `tests/quarantine_test.rs` with 12 automated unit and integration tests.

---

## 4. Verification Check-Out Record

- **GitHub PRs Merged:** PR #5 (Sprint-01), PR #7 (Sprint-02), PR #8 (Sprint-03), PR #9 (Sprint-04).
- **Gomaa Memory (`~/.gomaa/gomaa.db`):** SPRINT-01, SPRINT-02, SPRINT-03, SPRINT-04 all marked `COMPLETED`.
- **Google Drive Mirror:** Synchronized via `./sync_mirror.sh`.
