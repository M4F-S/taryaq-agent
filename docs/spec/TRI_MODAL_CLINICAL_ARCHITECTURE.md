# TARYAQ TRI-MODAL CLINICAL ARCHITECTURE SPECIFICATION
## The General Medical AI Agent for Clinicians, Pharmacists, and Patients
### Dubai Create AI Agents Championship 2026–2027

---

### 1. Architectural Philosophy: The Unified Clinical Engine
Taryaq operates as a single, sovereign, air-gapped clinical intelligence platform that exposes three specialized interfaces adapted to the three key stakeholders in outpatient and community healthcare:

```
                            ┌─────────────────────────────────┐
                            │    TARYAQ SOVEREIGN CORE        │
                            │  - C99 Pharmacovigilance (<0.02ms)│
                            │  - DHA SN 03/2019 Claim Engine  │
                            │  - Zero-Allocation Arabic NLP   │
                            │  - SHA-256 Cryptographic Ledger │
                            └────────────────┬────────────────┘
                                             │
             ┌───────────────────────────────┼───────────────────────────────┐
             ▼                               ▼                               ▼
 ┌───────────────────────┐       ┌───────────────────────┐       ┌───────────────────────┐
 │  CLINICIAN INTERFACE  │       │  PHARMACIST INTERFACE │       │   PATIENT INTERFACE   │
 │ (Doctors / Polyclinics)│      │(Dispensaries / Pharma)│       │  (Digital Companion)  │
 ├───────────────────────┤       ├───────────────────────┤       ├───────────────────────┤
 │ • Ambient Scribe      │       │ • Longitudinal Risk   │       │ • Dialectal Triage    │
 │ • Bilingual Diglossia │       │ • CYP450 / Renal Gate │       │ • Plain-Language Meds │
 │ • Trace-Linked Notes  │       │ • DHA Late Scrubbing  │       │ • Red-Flag Emergency  │
 │ • Pre-Order CDS Hooks │       │ • Override Replay Prot│       │ • Adherence Tracking  │
 └───────────────────────┘       └───────────────────────┘       └───────────────────────┘
```

---

### 2. Interface 1: Clinician Ambient Copilot (Doctors & Clinics)
* **Target Users:** General practitioners, internal medicine specialists, cardiologists, and nephrologists.
* **Core Workflows:**
  1. **Ambient Listening & Code-Switching:** Listens to patient-clinician dialogue, accommodating regional Arabic dialects (Gulf, Egyptian, Levantine) mixed with English clinical jargon.
  2. **Trace-Linked SOAP Note Drafting:** Adopts the "agent drafts, clinician signs" framework. Generates structured encounter notes with trace-linked evidence cards back to conversation timestamps to prevent automation bias and liability risks.
  3. **Real-Time CDS Pre-Order Hooks:** When a clinician orders a drug (e.g. Colchicine), Taryaq queries the local patient history; if concurrent inhibitors or impaired renal clearance ($eGFR < 60\text{ mL/min}$) exist, a deterministic red-flag alert interrupts the draft before submission.

---

### 3. Interface 2: Pharmacist Pharmacovigilance & Claim Firewall (Pharmacies)
* **Target Users:** Community pharmacists, hospital dispensing pharmacists, and pharmacy billing teams.
* **Core Workflows:**
  1. **Longitudinal Medication Intelligence:** Replaces noisy binary pop-up alerts with trajectory-based risk modeling (detecting compounded toxicity across polypharmacy profiles, such as Metformin in CKD Stage 3b or CYP3A4/P-gp inhibitors).
  2. **DHA SN 03/2019 Pre-Submission Scrubber:** Validates outgoing claims against statutory deadlines:
     - Initial claims: by the 15th calendar day of the subsequent month (§3.7.1).
     - Late initial claims: calculates statutory 0.03%/day penalty exposure on Net Claimed amounts (§3.7.2).
     - Resubmission window: within 21 calendar days from Remittance Advice (§3.8.1).
     - Late resubmission claims: calculates statutory 0.03%/day penalty exposure (§3.8.4).
     - Payer tracking: monitors insurer settlement delays eligible for 0.03%/day penalty recovery (§3.9.3).
  3. **Single-Use Clinician Override & Cryptographic Audit:**
     - Blocks can only be committed if a licensed clinician provides an authorized code and documents a clinical rationale.
     - Tokens are strictly single-use; replay attacks are rejected with HTTP 409 Conflict.
     - Every event is sealed in an append-only SHA-256 hash-chained JSONL audit ledger (`taryaq_audit.jsonl`).

---

### 4. Interface 3: Patient Digital Companion & Triage (Patients & Families)
* **Target Users:** Chronic disease patients, polypharmacy elderly individuals, and family caregivers.
* **Core Workflows:**
  1. **Dialectal Conversational Intake:** Allows patients to speak or text in their native Arabic dialect (via WhatsApp or clinic mobile portal) to report symptoms and request refills.
  2. **Plain-Language Medication Counseling:** Translates complex pharmacological instructions into clear, actionable advice (e.g. *"Take this with your morning meal; avoid grapefruits; report any severe muscle ache immediately"*).
  3. **Deterministic Red-Flag Escalation:** Non-diagnostic. If the patient mentions red-flag symptoms (acute chest pressure, radiating arm pain, unilateral facial drooping, or suicidal thoughts), Taryaq instantly interrupts the conversational flow and executes an emergency clinic dispatch / emergency services protocol.
  4. **Adherence & Refill Synchronization:** Tracks medication exhaustion dates and prompts the patient for renewal before critical doses lapse.

---

### 5. Data Sovereignty & Air-Gap Compliance
- Pursuant to **Article 13 of UAE Federal Law No. 2 of 2019**, all three interfaces run completely on-premises on a dedicated edge micro-appliance.
- **Zero cross-border network telemetry:** Patient identifiers (Emirates ID, names, birthdates) are masked in-place using deterministic C99 PII scrubbers before being processed by local language models.
