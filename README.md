# Taryaq (ترياق) — Sovereign Clinical Decision Support & Pharmacovigilance Engine
## Best Societal AI Agent | Dubai Create AI Agents Championship 2026–2027

---

### 1. Executive Summary & Problem Statement
In outpatient ambulatory clinics and community pharmacies across Dubai, clinical safety and insurance administration intersect under severe constraints:
1. **Severe Adverse Drug Events in Chronic Polypharmacy:** High-risk combinations (such as Colchicine + Clarithromycin in patients with mild-to-moderate CKD, eGFR < 60 mL/min) produce acute neurotoxicity and fatal bone marrow suppression. Probabilistic cloud LLM assistants often hallucinate or fail to catch these deterministically.
2. **Statutory Claim Cycle Penalties (DHA SN 03/2019):** Healthcare providers operating on Dubai's `eClaimLink` post office face statutory delay penalties of **0.03% of Net Claimed amount per calendar day of delay** (§3.7.2, §3.8.4).
3. **Data Sovereignty Restrictions:** **Article 13 of UAE Federal Law No. 2 of 2019** prohibits cross-border transfer or foreign cloud storage of patient health information without explicit ministerial exemption.

**Taryaq** solves this via a bare-metal, air-gapped ISO C99 engine executing deterministic pharmacovigilance checks in **< 0.02 ms** with tamper-evident SHA-256 cryptographic audit chaining.

---

### 2. Tri-Modal User Interfaces
Taryaq operates across three distinct healthcare personas:
1. **Clinician Interface (Doctors & Ambulatory Clinics):**
   - Ambient bilingual consultation listening (code-switching from spoken Arabic dialects to medical English).
   - Trace-linked SOAP note generation ("agent drafts, clinician signs" framework).
   - Pre-order Clinical Decision Support (CDS) hooks flagging renal and drug-drug contraindications.
2. **Pharmacist Interface (Community & Hospital Pharmacies):**
   - Longitudinal polypharmacy trajectory tracking (avoiding binary alert fatigue).
   - Pre-submission DHA SN 03/2019 claims cycle scrubber (§3.7.1, §3.7.2, §3.8.1, §3.8.4, §3.9.3).
   - Single-use clinician override verification with tamper-evident audit logging.
3. **Patient Interface (Companion & Intake):**
   - Culturally aware Arabic dialect triage.
   - Plain-language medication counseling and adherence tracking.
   - Hard red-flag emergency transfer gate (chest pressure, acute neurological symptoms).

---

### 3. Build & Test Instructions
```bash
# Build standalone CLI and shared library
make all

# Run complete deterministic verification suite
make test

# Clean build artifacts and runtime logs
make clean
```
