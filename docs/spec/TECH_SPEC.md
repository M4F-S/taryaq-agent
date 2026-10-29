# TARYAQ (ترياق) TECHNICAL ARCHITECTURE & IMPLEMENTATION SPECIFICATION
## Dubai Create AI Agents Championship 2026–2027 | Best Societal AI Agent
**System Role:** Sovereign General Medical AI Agent (Clinician Copilot, Pharmacist CDS, Patient Companion)  
**Safety Classification:** Inviolable Clinical Safety System & Statutory Claim Engine  
**Version:** 2.0 (Canonical Implementation Contract)  
**Author:** Mohamed Fathy, BPharm (Lead Systems Architect & Founder)

---

### 1. Architectural Philosophy: The Sovereign Clinical Engine
Taryaq enforces an absolute structural separation between **Agentic Medical Reasoning** (LLM inference, ambient SOAP drafting, clinical research synthesis, patient dialogue) and **Deterministic Patient Safety** (C99 renal/CYP contraindication blocker, DHA SN 03/2019 claims scrubber, and SHA-256 HMAC cryptographic audit trail).

```
                            ┌───────────────────────────────────────────────┐
                            │          TARYAQ TRI-MODAL INTERFACE           │
                            ├───────────────────────┬───────────────────────┤
                            │ CLINICIAN AMBIENT     │ PATIENT 24/7          │
                            │ COPILOT               │ HEALTH COMPANION      │
                            │ • Code-switch scribe  │ • Dialectal intake    │
                            │ • DDx generator       │ • Plain-language meds │
                            │ • Posology calculator │ • Red-flag emergency  │
                            └───────────┬───────────┴───────────┬───────────┘
                                        │                       │
                                        ▼                       ▼
                            ┌───────────────────────────────────────────────┐
                            │          TARYAQ AGENT CORE (PYTHON)           │
                            │  - Guideline RAG & Evidence Citation Engine   │
                            │  - Longitudinal Polypharmacy Trajectory       │
                            │  - Pharmacist CDS Verification Console        │
                            └───────────────────────┬───────────────────────┘
                                                    │
                                                    ▼
    ┌───────────────────────────────────────────────────────────────────────────────────────────────┐
    │                                INVIOLABLE C99 SAFETY KERNEL                                   │
    │                                (adapters/libdubai_adapters)                                   │
    ├───────────────────────────────┬───────────────────────────────┬───────────────────────────────┤
    │ 1. Deterministic Pharmacovig  │ 2. DHA SN 03/2019 Claim Engine│ 3. Cryptographic Audit Chain  │
    │    - Renal cutoffs (eGFR)     │    - 15th-day initial deadline│    - Append-only JSONL        │
    │    - CYP3A4 / CYP2D6 / P-gp   │    - 21-day resubmission window│   - HMAC-SHA256 chaining     │
    │    - Narrow therapeutic index │    - 0.03%/day statutory calc │    - Single-use override slot │
    └───────────────────────────────┴───────────────────────────────┴───────────────────────────────┘
                                                    │
                                                    ▼
                            ┌───────────────────────────────────────────────┐
                            │         SOVEREIGN CLINICAL DATABASE           │
                            │         (corpus/taryaq_clinical.db)           │
                            │  - 100+ Outpatient Drugs (ATC/CYP/eGFR)       │
                            │  - UAE Law 30/2021 Controlled Schedules       │
                            │  - Evidence Guidelines (MOHAP/DHA/BNF)        │
                            └───────────────────────────────────────────────┘
```

---

### 2. Sovereign Clinical Database Schema (`corpus/taryaq_clinical.db`)

All clinical entities must be persisted in an air-gapped, zero-cloud SQLite database (`corpus/taryaq_clinical.db`).

#### 2.1 Table: `drugs`
```sql
CREATE TABLE drugs (
    drug_id INTEGER PRIMARY KEY AUTOINCREMENT,
    canonical_name TEXT NOT NULL UNIQUE COLLATE NOCASE,
    arabic_name TEXT NOT NULL COLLATE NOCASE,
    brand_names_json TEXT NOT NULL,           -- JSON array: ["Glucophage", "Cidophage"]
    atc_code TEXT NOT NULL UNIQUE,            -- e.g. "A10BA02"
    drug_class_id INTEGER NOT NULL,
    renal_egfr_cutoff REAL DEFAULT 0.0,       -- Absolute contraindication cutoff in mL/min
    hepatic_metabolism_pct REAL DEFAULT 0.0,  -- Percentage hepatic clearance
    max_daily_dose_mg REAL NOT NULL,          -- Standard maximum daily dose in mg
    is_controlled_uae BOOLEAN DEFAULT 0,      -- 1 if scheduled under UAE Law 30/2021
    schedule_law_30_2021 TEXT,                -- 'Narcotic', 'Controlled_A', 'Controlled_B', 'Semi_Controlled'
    narrow_therapeutic_index BOOLEAN DEFAULT 0,
    cyp3a4_profile TEXT DEFAULT 'NONE',       -- 'SUBSTRATE', 'INHIBITOR', 'INDUCER', 'NONE'
    cyp2d6_profile TEXT DEFAULT 'NONE',
    cyp2c9_profile TEXT DEFAULT 'NONE',
    p_gp_profile TEXT DEFAULT 'NONE',         -- 'SUBSTRATE', 'INHIBITOR', 'INDUCER', 'NONE'
    qt_risk TEXT DEFAULT 'NONE',              -- 'HIGH', 'MODERATE', 'NONE'
    hyperkalemia_risk BOOLEAN DEFAULT 0,
    serotonin_risk BOOLEAN DEFAULT 0,
    indications_icd10_json TEXT NOT NULL,     -- JSON array: ["E11.9", "E11.2"]
    clinical_pearls TEXT,
    FOREIGN KEY(drug_class_id) REFERENCES drug_classes(class_id)
);
CREATE INDEX idx_drugs_name ON drugs(canonical_name);
CREATE INDEX idx_drugs_arabic ON drugs(arabic_name);
CREATE INDEX idx_drugs_atc ON drugs(atc_code);
```

#### 2.2 Table: `drug_classes`
```sql
CREATE TABLE drug_classes (
    class_id INTEGER PRIMARY KEY AUTOINCREMENT,
    class_name TEXT NOT NULL UNIQUE COLLATE NOCASE,
    category TEXT NOT NULL,                   -- e.g. "Cardiovascular", "Antidiabetic", "Antimicrobial"
    default_cyp_profile TEXT DEFAULT 'NONE'
);
```

#### 2.3 Table: `drug_interactions`
```sql
CREATE TABLE drug_interactions (
    interaction_id INTEGER PRIMARY KEY AUTOINCREMENT,
    drug_a_id INTEGER,
    drug_b_id INTEGER,
    class_a_id INTEGER,
    class_b_id INTEGER,
    severity TEXT NOT NULL,                   -- 'CONTRAINDICATED', 'MAJOR', 'MODERATE'
    alert_code TEXT NOT NULL UNIQUE,          -- e.g. "COLCHICINE_CLARITHRO_CYP3A4"
    clinical_rationale TEXT NOT NULL,
    action_directive TEXT NOT NULL,           -- e.g. "Select alternative macrolide (Azithromycin) or hold Colchicine"
    evidence_doc_id TEXT NOT NULL,            -- e.g. "BNF-86-SEC-10.1"
    evidence_section TEXT NOT NULL,
    evidence_sha256 TEXT NOT NULL,
    FOREIGN KEY(drug_a_id) REFERENCES drugs(drug_id),
    FOREIGN KEY(drug_b_id) REFERENCES drugs(drug_id)
);
```

#### 2.4 Table: `renal_dosing_guidelines`
```sql
CREATE TABLE renal_dosing_guidelines (
    rule_id INTEGER PRIMARY KEY AUTOINCREMENT,
    drug_id INTEGER NOT NULL,
    egfr_min REAL NOT NULL,
    egfr_max REAL NOT NULL,
    recommended_dose TEXT NOT NULL,           -- e.g. "500 mg once daily with meal"
    frequency_hours INTEGER NOT NULL,
    contraindicated BOOLEAN DEFAULT 0,
    clinical_rationale TEXT NOT NULL,
    evidence_sha256 TEXT NOT NULL,
    FOREIGN KEY(drug_id) REFERENCES drugs(drug_id)
);
```

#### 2.5 Table: `clinical_encounters`
```sql
CREATE TABLE clinical_encounters (
    encounter_id TEXT PRIMARY KEY,
    patient_id TEXT NOT NULL,
    timestamp_utc INTEGER NOT NULL,
    baseline_egfr REAL NOT NULL,
    weight_kg REAL NOT NULL,
    age_years INTEGER NOT NULL,
    gender TEXT NOT NULL,                     -- 'MALE', 'FEMALE'
    active_medications_json TEXT NOT NULL,    -- JSON list of active drug IDs
    allergies_json TEXT NOT NULL,             -- JSON list of allergens
    icd10_codes_json TEXT NOT NULL,           -- JSON list of diagnostic codes
    soap_note_text TEXT,
    hmac_signature TEXT NOT NULL
);
```

---

### 3. Core Clinical Scope: The 100+ Outpatient Drug Corpus

Implementers must populate `corpus/taryaq_clinical.db` with at least 100 validated outpatient drugs across 15 therapeutic categories:
1. **Cardiovascular / Antihypertensives (14):** Lisinopril, Ramipril, Enalapril, Losartan, Valsartan, Candesartan, Amlodipine, Diltiazem, Verapamil, Metoprolol, Bisoprolol, Carvedilol, Hydrochlorothiazide, Indapamide.
2. **Heart Failure & Diuretics (6):** Furosemide, Torsemide, Spironolactone, Eplerenone, Digoxin, Sacubitril/Valsartan.
3. **Lipid-Lowering (6):** Atorvastatin, Rosuvastatin, Simvastatin, Pravastatin, Ezetimibe, Fenofibrate.
4. **Antidiabetics (12):** Metformin, Gliclazide, Glimepiride, Sitagliptin, Linagliptin, Vildagliptin, Empagliflozin, Dapagliflozin, Semaglutide, Dulaglutide, Insulin Glargine, Insulin Aspart.
5. **Antithrombotics & Anticoagulants (7):** Warfarin, Dabigatran, Rivaroxaban, Apixaban, Clopidogrel, Aspirin, Ticagrelor.
6. **Antiarrhythmics (4):** Amiodarone, Flecainide, Sotalol, Propafenone.
7. **Anti-Gout & Hyperuricemia (4):** Colchicine, Allopurinol, Febuxostat, Probenecid.
8. **Antimicrobials (14):** Amoxicillin, Amoxicillin-Clavulanate, Cefuroxime, Ceftriaxone, Ciprofloxacin, Levofloxacin, Azithromycin, Clarithromycin, Doxycycline, Trimethoprim-Sulfamethoxazole, Nitrofurantoin, Metronidazole, Fluconazole, Itraconazole.
9. **Analgesics & NSAIDs (8):** Paracetamol, Ibuprofen, Naproxen, Celecoxib, Diclofenac, Meloxicam, Ketorolac, Etoricoxib.
10. **Controlled Analgesics & Opioids (Law 30/2021) (7):** Tramadol, Codeine, Morphine, Fentanyl, Oxycodone, Pethidine, Buprenorphine.
11. **Psychotropics & Sedatives (Law 30/2021) (8):** Alprazolam, Diazepam, Lorazepam, Clonazepam, Zolpidem, Pregabalin, Gabapentin, Midazolam.
12. **Antidepressants (8):** Escitalopram, Sertraline, Fluoxetine, Paroxetine, Venlafaxine, Duloxetine, Amitriptyline, Mirtazapine.
13. **Gastroenterology (6):** Omeprazole, Esomeprazole, Pantoprazole, Famotidine, Domperidone, Metoclopramide.
14. **Respiratory (6):** Salbutamol, Fluticasone/Salmeterol, Budesonide/Formoterol, Ipratropium, Tiotropium, Montelukast.
15. **Endocrine & Steroids (6):** Levothyroxine, Carbimazole, Prednisolone, Dexamethasone, Hydrocortisone, Methylprednisolone.

---

### 4. Deterministic C99 Safety Kernel Specification

#### 4.1 Cockcroft-Gault & Physiological eGFR Engine
```c
/* Calculates Creatinine Clearance using Cockcroft-Gault formula */
double taryaq_calculate_crcl(int age, double weight_kg, double scr_mg_dl, bool is_female) {
    if (age <= 0 || weight_kg <= 0.0 || scr_mg_dl <= 0.0) return -1.0;
    double crcl = ((140.0 - (double)age) * weight_kg) / (72.0 * scr_mg_dl);
    if (is_female) crcl *= 0.85;
    return crcl;
}
```

#### 4.2 DHA SN 03/2019 Claim Engine
Implements statutory claims scrubbing pursuant to Dubai Health Authority Service Notification 03/2019:
- **Initial Submission (§3.7.1):** Must be submitted by the 15th calendar day of the subsequent month.
- **Late Initial Penalty (§3.7.2):** Accrues a statutory **0.03% per calendar day** penalty on the Net Claimed Amount starting on day 16 until submission.
- **Resubmission Window (§3.8.1):** Valid only within **21 calendar days** from the receipt of the Remittance Advice.
- **Late Resubmission Penalty (§3.8.4):** Accrues 0.03%/day penalty exposure on resubmitted amounts.

---

### 5. Tri-Modal Agent Specifications

#### 5.1 Clinician Ambient Copilot (Doctors & Clinics)
- **Ambient Scribing:** Ingests raw doctor-patient dialogue, accommodating English clinical terms and Arabic Gulf/Egyptian dialectal phrasing.
- **SOAP Note Generation:** Emits structured SOAP note (Subjective, Objective, Assessment, Plan).
- **Trace-Linked Evidence:** Every recommendation links back to conversation timestamp and verified statutory/clinical guideline hash.
- **Pre-Order CDS Interrupt:** If an ordered medication triggers a C99 contraindication, the agent interrupts drafting with an alternative recommendation.

#### 5.2 Pharmacist CDS Verification Console (Dispensaries & Hospitals)
- **Multi-Drug Polypharmacy Screening:** Checks pairwise and class-level interactions across the patient's entire active profile.
- **Single-Use Clinician Override:** Requires valid, unconsumed authorization token with documented clinical rationale.
- **Cryptographic Audit Log:** Every verification, block, and override is committed to an append-only SHA-256 HMAC ledger (`taryaq_audit.jsonl`).

#### 5.3 Patient Health Companion (24/7 Digital Health)
- **Dialectal Conversational Intake:** Understands symptom descriptions in Gulf, Egyptian, and Levantine Arabic.
- **Deterministic Red-Flag Escalation:** Non-diagnostic. If patient reports acute chest pain, stroke symptoms (FAST), respiratory distress, or severe bleeding, the agent immediately executes an emergency clinical interrupt protocol and directs the patient to call 998 or visit the nearest emergency room.
- **Plain-Language Medication Instructions:** Translates complex posology into clear instructions: meal timing, common manageable side effects, foods to avoid (e.g. grapefruit, dairy with tetracyclines), and adherence reminders.

---

### 6. Conformance & Verification Suite
The implementer must ensure all tests pass:
1. `make test_taryaq` (C99 standalone unit tests).
2. `python3 tests/test_taryaq_acceptance.py` (Full workflow acceptance test).
3. `python3 tests/test_patient_timeline.py` (Longitudinal patient memory test).
4. `python3 tests/test_audit_tamper.py` (Cryptographic audit ledger tamper-resistance test).
