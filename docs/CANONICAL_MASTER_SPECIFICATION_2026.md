# CANONICAL MASTER SPECIFICATION & PRODUCT REQUIREMENTS DOCUMENT (PRD)
## The Sovereign Rust Multi-Agent Architecture for Dubai Create AI Agents Championship 2026–2027
**Target Platforms:** Taryaq (Societal), Qala (Business), Nibras (Personal)  
**Lead Systems Architect & Founder:** Mohamed Fathy, BPharm  
**Version:** 3.0 (Canonical Implementation Contract)  
**Status:** RATIFIED & LOCKED — PRE-IMPLEMENTATION SPECIFICATION

---

## 1. FOUNDATIONAL ARCHITECTURE & ENGINEERING LAWS

### 1.1 The Standalone Rust Agent Runtime + C99 Inviolable Kernel Bridge
Each agent repository (`taryaq-agent`, `qala-agent`, `nibras-agent`) operates as a **100% standalone Rust project** (`Cargo.toml`) with zero shared repository dependencies.
- **The Cognitive & Orchestration Tier (Rust):**
  - High-concurrency async runtime powered by `tokio`.
  - Compile-time guaranteed memory safety with zero garbage collection overhead.
  - Rock-solid serialization and schema validation via `serde` and `serde_json`.
  - Local embedded relational engines via `rusqlite`.
  - HTTP / WebSocket networking via `axum` and `reqwest`.
- **The Inviolable Safety Kernel (C99 via Rust FFI):**
  - Sub-0.05ms deterministic execution latency.
  - Linked directly into Rust via `extern "C"` FFI bindings from `adapters/libdubai_adapters`.
  - Enforces non-bypassable statutory checks: DESC ISR v3.1 path sandboxing, Tau-3 FinOps AED 50,000.00 gates, DHA SN 03/2019 claims penalty calculations, and HMAC-SHA256 audit chaining.
  - **Inviolable Rule:** The Rust agent runtime can propose any action, but **zero state mutation or external execution can occur without passing through the C99 Governor**.

### 1.2 The Multi-Session Dev Team Law
To prevent context saturation, drift, and model hallucinations:
1. **One Session = One Feature:** Every session is launched to complete exactly one scoped task.
2. **Gomaa Check-In:** At session start, the agent queries `~/.gomaa/gomaa.db` for the current sprint status.
3. **Execution & Verification:** The agent implements the feature, passes all unit and integration tests, and generates verification proofs.
4. **Gomaa Check-Out:** At session end, the agent writes the completed milestone and artifacts back to `sprint_tasks` and closes cleanly.

### 1.3 The Gomaa Memory System Schema (`~/.gomaa/gomaa.db`)
Gomaa acts as the persistent corporate whiteboard across all sessions. A dedicated `sprint_tasks` table tracks the exact feature checklist:

```sql
CREATE TABLE IF NOT EXISTS sprint_tasks (
    task_id TEXT PRIMARY KEY,
    agent_target TEXT NOT NULL,         -- 'TARYAQ', 'QALA', 'NIBRAS', 'ORCHESTRATION'
    milestone_code TEXT NOT NULL,       -- e.g. 'M-01', 'M-02'
    title TEXT NOT NULL,
    description TEXT NOT NULL,
    status TEXT NOT NULL,               -- 'PENDING', 'IN_PROGRESS', 'COMPLETED', 'BLOCKED'
    assigned_session_id TEXT,
    acceptance_criteria_json TEXT NOT NULL,
    verification_output_json TEXT,
    completed_at DATETIME
);
```

### 1.4 Hermes Review Fleet & VPS Infrastructure (`187.124.2.26`)
- **Role:** Independent adversarial review, stress testing, and staging verification.
- **Access Protocol:** SSH password authentication via `sshpass` (`root@187.124.2.26`).
- **Fleet Sync:** Major milestones are synced from local Gomaa to `/root/.hermes/home/.mnemosyne/mnemosyne.db` on the VPS to allow Hermes agents to audit pull requests and run automated security reviews.

---

## 2. AGENT 1: TARYAQ (BEST SOCIETAL AI AGENT)

### 2.1 The Empty Clinical Engine & Generalized Relational Schema
Taryaq rejects toy hardcoded lists. It is built as an **Empty Clinical Vessel** capable of holding thousands of active substances, international disease taxonomies, lab panels, and diagnostic modalities.

```sql
-- Generalized Drug Formulary
CREATE TABLE formulary_drugs (
    atc_code VARCHAR(10) PRIMARY KEY,
    canonical_name VARCHAR(128) NOT NULL COLLATE NOCASE,
    arabic_name VARCHAR(128) NOT NULL COLLATE NOCASE,
    brand_names_json TEXT NOT NULL,           -- JSON array: ["Glucophage", "Cidophage"]
    drug_class_id INTEGER NOT NULL,
    renal_egfr_cutoff REAL DEFAULT 0.0,       -- Absolute contraindication cutoff in mL/min
    hepatic_metabolism_pct REAL DEFAULT 0.0,
    max_daily_dose_mg REAL NOT NULL,
    is_controlled_uae BOOLEAN DEFAULT 0,
    schedule_law_30_2021 TEXT,                -- 'Narcotic', 'Controlled_A', 'Controlled_B', 'Semi_Controlled'
    narrow_therapeutic_index BOOLEAN DEFAULT 0,
    cyp3a4_profile TEXT DEFAULT 'NONE',       -- 'SUBSTRATE', 'INHIBITOR', 'INDUCER', 'NONE'
    cyp2d6_profile TEXT DEFAULT 'NONE',
    cyp2c9_profile TEXT DEFAULT 'NONE',
    cyp2c19_profile TEXT DEFAULT 'NONE',
    p_gp_profile TEXT DEFAULT 'NONE',
    qt_risk TEXT DEFAULT 'NONE',              -- 'HIGH', 'MODERATE', 'NONE'
    hyperkalemia_risk BOOLEAN DEFAULT 0,
    serotonin_risk BOOLEAN DEFAULT 0,
    indications_icd10_json TEXT NOT NULL,
    clinical_pearls TEXT
);

-- Pairwise & Class Clinical Interactions
CREATE TABLE drug_interactions (
    interaction_id INTEGER PRIMARY KEY AUTOINCREMENT,
    drug_a_atc VARCHAR(10) NOT NULL,
    drug_b_atc VARCHAR(10) NOT NULL,
    alert_code VARCHAR(64) UNIQUE NOT NULL,
    severity_level VARCHAR(16) CHECK(severity_level IN ('CONTRAINDICATED', 'MAJOR', 'MODERATE')),
    mechanism_text TEXT NOT NULL,
    clinical_action TEXT NOT NULL,
    evidence_citation TEXT NOT NULL,
    FOREIGN KEY(drug_a_atc) REFERENCES formulary_drugs(atc_code),
    FOREIGN KEY(drug_b_atc) REFERENCES formulary_drugs(atc_code)
);

-- Generalized Disease / Indication Taxonomy
CREATE TABLE medical_conditions (
    icd10_code VARCHAR(10) PRIMARY KEY,
    icd11_code VARCHAR(10),
    condition_name_en VARCHAR(256) NOT NULL,
    condition_name_ar VARCHAR(256) NOT NULL,
    category VARCHAR(64) NOT NULL,
    red_flag_priority BOOLEAN DEFAULT 0
);

-- Diagnostic Laboratory Panels (LOINC)
CREATE TABLE lab_diagnostic_panels (
    loinc_code VARCHAR(16) PRIMARY KEY,
    test_name VARCHAR(128) NOT NULL,
    specimen VARCHAR(32) NOT NULL,
    standard_unit VARCHAR(32) NOT NULL,
    reference_low REAL,
    reference_high REAL,
    critical_panic_low REAL,
    critical_panic_high REAL
);

-- Staging Buffer for Human-in-the-Loop Web Updates
CREATE TABLE hitl_clinical_quarantine (
    staging_id INTEGER PRIMARY KEY AUTOINCREMENT,
    source_url TEXT NOT NULL,
    extracted_finding TEXT NOT NULL,
    target_atc_code VARCHAR(10),
    proposed_interaction_json TEXT,
    evidence_pmid VARCHAR(16),
    discovered_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    review_status TEXT DEFAULT 'PENDING',     -- 'PENDING', 'APPROVED', 'REJECTED'
    reviewer_notes TEXT,
    reviewed_at DATETIME
);
```

### 2.2 Complete Pharmacovigilance Matrix: 30 High-Yield Clinical Interactions
The table below codifies the 30 critical outpatient interactions populated in `taryaq_clinical.db`:

| ID | Drug A (ATC) | Drug B (ATC) | Mechanism Description | Severity Tier | Standard Alert Code | Clinical Rationale & Management Action | Evidence Citation |
|:---|:---|:---|:---|:---|:---|:---|:---|
| 1 | Warfarin (B01AA03) | Amiodarone (C01BD01) | CYP2C9/CYP3A4 inhibition by amiodarone impairs S-warfarin clearance | MAJOR | `WARFARIN_AMIODARONE_CYP2C9_INHIB` | Marked INR elevation causes life-threatening hemorrhage. Reduce warfarin dose 33–50%; monitor INR weekly. | CHEST 2018 (PMID: 30122176); DailyMed Warfarin Sec 7.3 |
| 2 | Simvastatin (C10AA01) | Clarithromycin (J01FA09) | Potent CYP3A4 inhibition increases simvastatin AUC up to 10-fold | CONTRAINDICATED | `STATIN_MACROLIDE_CYP3A4_RHABDO` | Massive statin exposure induces severe rhabdomyolysis and acute kidney injury. Concomitant use contraindicated; suspend statin or switch to azithromycin. | FDA Safety Comm. (2012); DailyMed Simvastatin Sec 4; PMID: 22619236 |
| 3 | Lisinopril (C09AA03) | Losartan (C09CA01) | Dual RAAS blockade produces additive efferent arteriolar vasodilation | CONTRAINDICATED | `ACEI_ARB_DUAL_RAAS_BLOCKADE` | Severe hypotension, acute renal failure, and hyperkalemia occur without clinical benefit. Avoid concomitant use. | ONTARGET Trial, N Engl J Med 2008 (PMID: 18378520); DailyMed Lisinopril Sec 5.1 |
| 4 | Spironolactone (C03DA01) | Enalapril (C09AA02) | Additive mineralocorticoid antagonism and suppressed distal potassium secretion | MAJOR | `MRA_ACEI_ADDITIVE_HYPERKALEMIA` | Causes lethal hyperkalemia and ventricular arrhythmias. Contraindicated if K+ > 5.0 mEq/L or eGFR < 30 mL/min; monitor serum K+. | RALES Trial, N Engl J Med 1999 (PMID: 10471456); AHA HF Guidelines 2022 (PMID: 35363499) |
| 5 | Clopidogrel (B01AC04) | Omeprazole (A02BC01) | Competitive CYP2C19 inhibition prevents bioactivation of clopidogrel prodrug | MAJOR | `CLOPIDOGREL_OMEPRAZOLE_CYP2C19` | Attenuated platelet inhibition increases risk of stent thrombosis and recurrent MI. Substitute with pantoprazole or rabeprazole. | COGENT Trial, N Engl J Med 2010 (PMID: 20925534); DailyMed Plavix Sec 7.1 |
| 6 | Methotrexate (L01BA01) | Ibuprofen (M01AE01) | NSAIDs inhibit renal OAT1/OAT3 transporters and reduce medullary perfusion | MAJOR | `MTX_NSAID_RENAL_CLEARANCE_TOX` | Markedly reduced renal methotrexate clearance precipitates pancytopenia and severe hepatotoxicity. Avoid concurrent NSAIDs. | Ann Rheum Dis 2017 (PMID: 28546162); DailyMed Methotrexate Sec 7 |
| 7 | Fluoxetine (N06AB03) | Tramadol (N02AX02) | Additive synaptic serotonin reuptake inhibition plus CYP2D6 blockade | MAJOR | `SSRI_TRAMADOL_SEROTONIN_SYN` | Triggers Serotonin Syndrome manifested by clonus, autonomic instability, hyperthermia, and seizures. Avoid combination. | Boyer & Shannon, N Engl J Med 2005 (PMID: 15784664); DailyMed Tramadol Sec 5.6 |
| 8 | Digoxin (C01AA05) | Clarithromycin (J01FA09) | Inhibition of intestinal and renal P-gp efflux pumps and gut flora eradication | MAJOR | `DIGOXIN_CLARITHRO_PGP_TOXICITY` | Serum digoxin levels increase >2-fold, precipitating lethal AV block and dysrhythmias. Halve digoxin dose and monitor ECG. | Circulation 2002 (PMID: 11980678); DailyMed Lanoxin Sec 7.1 |
| 9 | Ciprofloxacin (J01MA02) | Amiodarone (C01BD01) | Synergistic blockade of myocardial rapid delayed rectifier potassium channels (IKr) | CONTRAINDICATED | `QT_PROLONGATION_SUMMATION_TDP` | Significant QTc prolongation precipitates Torsades de Pointes. Combination strictly contraindicated; use non-quinolone antibacterial. | CredibleMeds Category 'Known Risk'; DailyMed Ciprofloxacin Sec 5.3; PMID: 21676878 |
| 10 | Metformin (A10BA02) | Iodinated Contrast (V08AB02) | Radiocontrast-induced acute renal tubular injury impairs metformin elimination | MAJOR | `METFORMIN_CONTRAST_LACTIC_ACIDOSIS` | Biguanide accumulation triggers lactic acidosis with high mortality. Discontinue metformin prior to contrast if eGFR < 60 mL/min. | ACR Manual on Contrast Media (2023); DailyMed Glucophage Sec 5.1; PMID: 28864703 |
| 11 | Sildenafil (G04BE03) | Nitroglycerin (C01DA02) | Synergistic cGMP accumulation via PDE5 inhibition and nitrate NO donation | CONTRAINDICATED | `PDE5I_NITRATE_FATAL_HYPOTENSION` | Produces profound refractory systemic vasodilation, hemodynamic collapse, and coronary hypoperfusion. Strictly contraindicated (24h window). | ACC/AHA Practice Guidelines, Circulation 1999 (PMID: 10587354); DailyMed Revatio Sec 4 |
| 12 | Allopurinol (M04AA01) | Azathioprine (L04AX01) | Xanthine oxidase inhibition blocks primary catabolic clearance of 6-MP | CONTRAINDICATED | `ALLOPURINOL_AZATHIOPRINE_MYELOSUPP` | Dramatic shunting into active cytotoxic thioguanine metabolites induces pancytopenia and lethal bone marrow failure. Reduce azathioprine dose by 75%. | J Rheumatol 2018 (PMID: 29545437); DailyMed Imuran Sec 7.1 |
| 13 | Linezolid (J01XX08) | Sertraline (N06AB06) | Non-selective, reversible MAO-A inhibition by linezolid combined with SSRI | CONTRAINDICATED | `LINEZOLID_SSRI_SEROTONIN_TOXICITY` | Rapid accumulation of synaptic 5-HT triggers malignant hyperthermia, delirium, and neuromuscular rigidity. Discontinue SSRI 2 weeks prior. | FDA Drug Safety Comm. (2011); DailyMed Zyvox Sec 4.2; PMID: 23143825 |
| 14 | Potassium Chloride (A12BA01) | Spironolactone (C03DA01) | Exogenous potassium supplementation combined with mineralocorticoid antagonism | CONTRAINDICATED | `POTASSIUM_MRA_SEVERE_HYPERKALEMIA` | Impaired distal tubular excretion leads to rapid hyperkalemia, cardiac arrest, and ventricular fibrillation. Prohibit routine concurrent supplementation. | Kidney Int 2020 (PMID: 32679173); DailyMed Klor-Con Sec 5.1 |
| 15 | Lithium (N05AN01) | Hydrochlorothiazide (C03AA03) | Distal tubular sodium depletion increases compensatory proximal reabsorption of lithium | MAJOR | `LITHIUM_THIAZIDE_TOXICITY_ACCUM` | Renal lithium clearance decreases by 25–40%, inducing neurotoxicity, tremor, ataxia, and coma. Reduce lithium dosage by 50%. | Am J Psychiatry 2016 (PMID: 27181414); DailyMed Lithobid Sec 7 |
| 16 | Warfarin (B01AA03) | Ciprofloxacin (J01MA02) | CYP1A2 metabolic inhibition by ciprofloxacin and disruption of vitamin K gut flora | MAJOR | `WARFARIN_FLUOROQUINOLONE_BLEED` | Increases free warfarin plasma concentration, causing sudden prolongation of PT/INR and bleeding. Preemptively reduce warfarin by 25–33%. | Chest 2012 (PMID: 22315268); OpenFDA Label Ciprofloxacin Sec 7 |
| 17 | Theophylline (R03DA04) | Ciprofloxacin (J01MA02) | Potent inhibition of hepatic CYP1A2 clearance pathways by ciprofloxacin | MAJOR | `THEOPHYLLINE_CIPRO_CYP1A2_SEIZURE` | Elevates serum theophylline levels 2- to 3-fold, triggering tachyarrhythmias and status epilepticus. Reduce theophylline dose by 50%. | Clin Pharmacokinet 2004 (PMID: 15535758); DailyMed Theo-24 Sec 7.2 |
| 18 | Rivaroxaban (B01AF01) | Ketoconazole (J02AB02) | Dual potent inhibition of hepatic CYP3A4 degradation and P-gp efflux transport | CONTRAINDICATED | `DOAC_AZOLE_DUAL_CLEARANCE_INHIB` | Systemic rivaroxaban AUC increases >150%, escalating fatal hemorrhagic stroke risks. Combination contraindicated. | DailyMed Xarelto Sec 4; Br J Clin Pharmacol 2013 (PMID: 23594271) |
| 19 | Carbamazepine (N03AF01) | Ethinylestradiol (G03AA07) | Potent hepatic CYP3A4 and UGT enzyme induction accelerates hormone clearance | MAJOR | `CARBAMAZEPINE_OCP_CONTRACEPTIVE_FAIL` | Markedly lower hormone concentrations result in breakthrough ovulation and unplanned pregnancy. Advise non-hormonal contraception. | CDC Contraceptive Guidance, MMWR 2016 (PMID: 27467196); DailyMed Tegretol Sec 7 |
| 20 | Cyclosporine (L04AD01) | St. John's Wort (A13A) | Hyperforin potently induces intestinal and hepatic CYP3A4 and P-gp expression | CONTRAINDICATED | `CYCLOSPORINE_STJOHNS_ORGAN_REJECT` | Precipitous drops in cyclosporine trough levels below therapeutic ranges trigger acute allograft rejection. Concomitant administration contraindicated. | Lancet 2000 (PMID: 10683003); DailyMed Sandimmune Sec 7.4 |
| 21 | Colchicine (M04AC01) | Clarithromycin (J01FA09) | Dual inhibition of CYP3A4 metabolism and P-gp-mediated renal excretion | CONTRAINDICATED | `COLCHICINE_MACROLIDE_FATAL_TOXICITY` | Colchicine accumulation causes mitotic arrest, neuromuscular toxicity, multiorgan failure, and death. Strictly contraindicated in renal/hepatic impairment. | FDA Safety Comm. (2009); DailyMed Colcrys Sec 4; PMID: 20516676 |
| 22 | Levothyroxine (H03AA01) | Ferrous Sulfate (B03AA07) | Intraluminal chelation: polyvalent iron ions bind levothyroxine into insoluble chelate | MODERATE | `LEVOTHYROXINE_IRON_CHELATION_HYPO` | Drastically impaired thyroxine absorption leads to clinical hypothyroidism. Separate oral administration by at least 4 hours. | Thyroid 2014 (PMID: 25110870); DailyMed Synthroid Sec 7.1 |
| 23 | Atorvastatin (C10AA05) | Gemfibrozil (C10AB04) | Gemfibrozil inhibits statin glucuronidation (UGT1A1/3) and blocks OATP1B1 hepatic uptake | MAJOR | `STATIN_FIBRATE_OATP1B1_RHABDOMYO` | Increases circulating atorvastatin levels, escalating rhabdomyolysis and acute renal failure risks. If fibrate needed, fenofibrate preferred. | Am J Cardiol 2007 (PMID: 17448743); DailyMed Lipitor Sec 7.1 |
| 24 | Verapamil (C08DB01) | Atenolol (C07AB03) | Synergistic negative chronotropic, dromotropic, and inotropic actions on SA/AV nodes | MAJOR | `NONDIHYDRO_CCB_BB_AV_BLOCK` | Severe symptomatic sinus bradycardia, complete heart block, and acute heart failure. Avoid combination in left ventricular dysfunction. | ACC/AHA Stable Ischemic Heart Disease Guidelines 2012 (PMID: 23166211); DailyMed Calan Sec 7 |
| 25 | Citalopram (N06AB04) | Ondansetron (A04AA01) | Additive pharmacodynamic blockade of myocardial hERG potassium channels | MAJOR | `SSRI_5HT3_QT_SUMMATION_ARRHYTHMIA` | Synergistic QTc prolongation increases incidence of ventricular tachydysrhythmias. Limit citalopram to 20 mg/day in adults >60. | FDA Drug Safety Announce. (2012); DailyMed Celexa Sec 5.2; PMID: 24352720 |
| 26 | Tacrolimus (L04AD02) | Voriconazole (J02AC03) | Potent inhibition of hepatic and intestinal mucosal CYP3A4 by voriconazole | MAJOR | `TACROLIMUS_VORICONAZOLE_NEPHROTOX` | Increases tacrolimus AUC by 300%, causing severe nephrotoxicity and neurotoxicity. Preemptively reduce tacrolimus dose by 66%; monitor trough. | Am J Transplant 2007 (PMID: 17391136); DailyMed Vfend Sec 7.1 |
| 27 | Tamoxifen (L02BA01) | Paroxetine (N06AB05) | Irreversible mechanism-based inhibition of hepatic CYP2D6 by paroxetine | MAJOR | `TAMOXIFEN_PAROXETINE_CYP2D6_FAILURE` | Prevents metabolic conversion of tamoxifen into active endoxifen, causing therapeutic failure and breast cancer relapse. Select venlafaxine. | BMJ 2010 (PMID: 20142325); DailyMed Nolvadex Sec 7.1 |
| 28 | Phenytoin (N03AB02) | Valproic Acid (N03AG01) | Valproate displaces phenytoin from albumin binding sites and inhibits CYP2C9 | MAJOR | `PHENYTOIN_VALPROATE_PROTEIN_DISPLACE` | Unbound (free active) phenytoin fractions increase significantly, precipitating ataxia, nystagmus, and delirium. Monitor free serum phenytoin. | Epilepsia 2017 (PMID: 28267202); DailyMed Dilantin Sec 7.2 |
| 29 | Ramipril (C09AA05) | Sacubitril/Valsartan (C09DX04) | Simultaneous inhibition of neprilysin and angiotensin-converting enzyme pathways | CONTRAINDICATED | `NEPRILYSIN_ACEI_FATAL_ANGIOEDEMA` | Impairs metabolic degradation of bradykinin and substance P, triggering severe, potentially fatal airway angioedema. Enforce 36h washout. | PARADIGM-HF, N Engl J Med 2014 (PMID: 25176015); DailyMed Entresto Sec 4 |
| 30 | Morphine (N02AA01) | Diazepam (N05BA01) | Synergistic depressive actions on respiratory center neurons in the brainstem | MAJOR | `OPIOID_BENZO_FATAL_RESP_DEPRESSION` | Produces profound sedation, hypercapnic respiratory depression, coma, and fatal respiratory arrest. Restrict to absolute minimum duration; co-prescribe naloxone. | FDA Black Box Warning (2016); DailyMed MS Contin Sec 5.1; PMID: 28292881 |

### 2.3 DHA Regulatory Claims Scrubbing & Ejadah Value-Based Metrics

#### DHA SN 03/2019 Claim Submission Lifecycle (141 Days Maximum):
1. **Initial Submission Window:** Must be transmitted to eClaimLink by the 15th calendar day of the subsequent month.
2. **Late Initial Submission Penalty:** Payer is statutorily entitled to levy a **0.03% daily penalty** on the Net Claimed Amount for each calendar day overdue (§3.7.2).
3. **Adjudication & Remittance Advice (RA):** Payers must upload an electronic RA within **45 calendar days** (expedited **7 calendar days** for emergency medical services).
4. **Late Payer Settlement Penalty:** If the payer fails to settle within 45 days (or 7 days for emergency), provider levies **0.03% per calendar day** against the Net Claimed Amount.
5. **Provider Resubmission Lifecycle:** Denied claim lines must be corrected and resubmitted within **21 calendar days** from receipt of RA with explicit denial reasons. Maximum of 2 resubmission cycles allowed. Overdue resubmissions face a 0.03% daily penalty.
6. **Bi-Annual Reconciliation:** Mandatory twice-yearly formal reconciliation arbitrating contested claims.
7. **Statutory Data Archival:** Maximum operational retrieval period is **24 months** from initial submission date.

#### DHA eClaimLink Statutory Denial Code Mappings:
- `ELIG-001`: Patient not covered under active policy on date of service. Validate active Emirates ID before resubmission.
- `AUTH-001`: Prior authorization mandatory but omitted prior to service delivery. Attach retrospective approval or route to self-pay.
- `AUTH-003`: `PriorAuthorizationNumber` string invalid or malformed. Align with electronic payer authorization token.
- `CODE-010`: Activity/diagnosis inconsistent with ordering clinician's licensed DHA medical specialty.
- `CODE-014`: Billed CPT/ICD-10 activity inconsistent with patient demographic age or biological gender.
- `MNEC-004`: Service not clinically justified due to missing supporting secondary diagnoses or diagnostic lab results.
- `MNEC-005`: Service frequency exceeds protocol limits.
- `TIME-001`: Statutory initial submission deadline expired under SN 03/2019; technical rejection unless force majeure documented.
- `SURC-001`: Severe drug-drug interaction or absolute clinical contraindication; requires licensed clinician override documentation.

#### Ejadah Value-Based Healthcare KPIs:
- **Type 2 Diabetes Mellitus (DHA/DHIC/HEIPD/CG-03):** Prediabetes screening at age $\ge 30$ for BMI $\ge 25\text{ kg/m}^2$ (or $\ge 23$ for Asian descent). HbA1c testing at least twice per measurement year; quarterly if $\text{HbA1c} > 7.0\%$. Target blood pressure $< 130/80\text{ mmHg}$.
- **Hypertension & Renal Risk (DHA/DHIC/HEIPD/CG-20):** Blood pressure recorded at every encounter. Readings $\ge 130/80\text{ mmHg}$ require clinical management. Patients with HTN + diabetes require eGFR and urine Albumin-to-Creatinine Ratio (ACR) every 6–12 months.
- **Plasma Lipid Disorders (DHA/DHIC/HEIPD/CG-09):** Regional ASCVD risk stratification tracking LDL-C, non-HDL-C, and ApoB. Severe hypertriglyceridemia ($\text{TG} \ge 5.0\text{ to }10.0\text{ mmol/L}$ [$440\text{ to }880\text{ mg/dL}$]) requires immediate dietary fat restriction and fenofibrate combined with prescription omega-3 fatty acids ($2\text{ to }4\text{ g/day}$) to prevent acute pancreatitis.

### 2.4 Tri-Modal Clinical Interfaces

#### 1. Clinician Ambient Copilot (DeepCura Pattern)
- **Ambient Listening:** Processes doctor-patient conversations in mixed English clinical jargon and Arabic regional dialects (Gulf, Egyptian, Levantine).
- **Automated SOAP Scribing:** Emits structured Subjective, Objective, Assessment, and Plan notes with trace-linked evidence hashes back to conversation timestamps.
- **Ranked Differential Diagnoses (DDx):** Auto-codes ICD-10 diagnostic entries linked to clinical guideline citations.
- **Pre-Order CDS Interrupt:** Halts drafting if ordered posology breaches eGFR cutoffs or introduces absolute drug contraindications.

#### 2. Pharmacist Verification Console
- **Polypharmacy Trajectory Engine:** Trajectory-based compounded toxicity analysis across complete patient medication histories.
- **Single-Use Clinician Override:** Requires valid, unconsumed authorization token with clinical rationale. Replay attacks rejected with HTTP 409 Conflict.
- **HMAC-SHA256 Audit Trail:** Every verification, block, and override is committed to an append-only cryptographic ledger (`taryaq_audit.jsonl`).

#### 3. Patient Health Companion (Onlim Pattern)
- **Dialectal Conversational Intake:** Understands colloquial symptom reporting in Gulf, Egyptian, and Levantine Arabic.
- **Emergency Red-Flag Escalation:** Non-diagnostic. Recognizes acute cardiovascular distress (crushing chest pain, radiating jaw/arm pain, "عوار / نغز بالصدر"), acute neurological stroke deficits (facial drooping, slurred speech, unilateral weakness - FAST), acute dyspnea / anaphylaxis ("اختناق"), severe hemorrhage ("استفراغ دم"), or loss of consciousness. **Immediately triggers emergency protocol and directs patient to call 998 or 999.**
- **Plain-Language Posology Counseling:** Translates complex regimens into accessible advice: administration timing, meal relations, manageable side effects, and refill schedules.

### 2.5 Offline RAG & Human-in-the-Loop (HITL) Web Ingestion
- **100% Offline Vault:** Clinical search executes locally in sub-millisecond cycles against SQLite FTS5 and quantized embeddings. Zero data leaves the edge hardware.
- **Web Quarantine Buffer:** When connected, background workers scan PubMed, WHO, OpenFDA, and DHA portals for new safety alerts.
- **The Pharmacist Gate:** New findings are staged in `hitl_clinical_quarantine`. Mohamed Fathy, BPharm reviews the evidence and clicks **Approve** before findings are committed to the offline database.

---

## 3. AGENT 2: QALA (BEST BUSINESS AI AGENT)

### 3.1 Corporate Agency Mesh Architecture
Qala operates as an autonomous enterprise operating system featuring a **Managing Director (Chief of Staff)** agent orchestrating **6 Departmental Specialist Agents**:

```
                           ┌────────────────────────────────────────┐
                           │      MANAGING DIRECTOR / COS AGENT     │
                           │  - Executive intent decomposition      │
                           │  - Inter-departmental coordination    │
                           │  - CEO human escalation & approvals    │
                           └───────────────────┬────────────────────┘
                                               │
             ┌───────────────────┬─────────────┴─────┬───────────────────┬───────────────────┐
             ▼                   ▼                   ▼                   ▼                   ▼
     ┌──────────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
     │   FINANCE    │    │    LEGAL     │    │TRADE/CUSTOMS │    │  HR & PEOPLE │    │ PROCUREMENT  │
     │  & FINOPS    │    │ & COMPLIANCE │    │ (DP WORLD)   │    │  (UAE LABOR) │    │ & FACILITIES │
     └──────┬───────┘    └──────┬───────┘    └──────┬───────┘    └──────┬───────┘    └──────┬───────┘
            │                   │                   │                   │                   │
            └───────────────────┴─────────────┬─────┴───────────────────┴───────────────────┘
                                              ▼
                             ┌───────────────────────────────────┐
                             │    QALA C99/RUST POLICY GOVERNOR  │
                             ├───────────────────────────────────┤
                             │ • Tau-3 Gate (> AED 50k ceiling)  │
                             │ • DESC ISR v3.1 Path Sandbox      │
                             │ • Scope-Bound Single-Use Tokens   │
                             │ • Append-Only SHA-256 HMAC Ledger │
                             └─────────────────┬─────────────────┘
                                               ▼
                             ┌───────────────────────────────────┐
                             │     ENTERPRISE INTEGRATIONS       │
                             │   (SAP, Dubai Customs, DEWA)      │
                             └───────────────────────────────────┘
```

### 3.2 Departmental Specialists & Tool Schemas

#### 1. Finance & FinOps Specialist (`qala_finance`)
- `finops_reconcile_vat(invoice_no, trn, net_aed, vat_aed)`: Verifies UAE FTA 5% VAT.
- `finops_request_wire(beneficiary, iban, amount_aed, memo)`: Intercepted by Tau-3 gate if $> \text{AED 50,000.00}$.
- `finops_audit_payroll(month, total_wps_aed, employee_count)`: Audits Wages Protection System (WPS) compliance under Ministerial Resolution No. 43 of 2022.

#### 2. Legal & Compliance Specialist (`qala_legal`)
- `legal_review_contract(contract_text, governing_law)`: Validates jurisdiction clauses (DIFC, ADGM, Dubai Civil Courts).
- `legal_check_desc_compliance(system_id, data_classification)`: Enforces DESC ISR v3.1 classification (`Public`, `Restricted`, `Confidential`, `Secret`).
- `legal_audit_nda(nda_text, confidentiality_period_years)`: Audits compliance with UAE trade secrets protection laws.

#### 3. Trade & Customs Specialist (`qala_trade` — DP World & Mirsal II)
- **Tariff Formula:** 
  $$\text{Customs Duty} = 0.05 \times \text{CIF}$$
  $$\text{VAT Base} = \text{CIF} + \text{Customs Duty} = 1.05 \times \text{CIF}$$
  $$\text{VAT Amount} = 0.05 \times (1.05 \times \text{CIF}) = 0.0525 \times \text{CIF}$$
  $$\text{Total Landed Duty \& Tax} = 0.1025 \times \text{CIF}\quad (10.25\%)$$
- **Zero-Rated Categories:** Human pharmaceuticals (Chapter 30, HS 3004.90) and diagnostic medical equipment (Chapter 90, HS 9018.90) with MoHAP/EDE permit. Basic foodstuffs zero-rated under GCC food security protocols.
- **Free Zone to Mainland Exit:** Bonded storage to domestic consumption transition requires *Import to Local Market from Free Zone* declaration via Mirsal II. Temporary admission requires a 5% refundable cash deposit or bank guarantee released upon verified re-entry within 180 days.
- `customs_clear_shipment(bill_of_lading, container_id, importer_code, duty_amount_aed)`: Gated if duty $> \text{AED 50,000.00}$.

#### 4. HR & People Specialist (`qala_hr` — UAE Labor Law No. 33 of 2021)
- **Article 51 End-of-Service Gratuity (EOSG) Formulas:**
  * Tenure $< 1$ Year: Zero (0) payout.
  * Tenure $1 \le Y \le 5$ Years: 21 days' basic salary per completed year:
    $$\text{Gratuity} = Y \times 21 \times \left(\frac{\text{Basic}}{30}\right)$$
  * Tenure $Y > 5$ Years: 21 days/year for first 5 years, plus 30 days/year for each additional year:
    $$\text{Gratuity} = \left[ 5 \times 21 \times \left(\frac{\text{Basic}}{30}\right) \right] + \left[ (Y - 5) \times 30 \times \left(\frac{\text{Basic}}{30}\right) \right]$$
  * Maximum Cap: Total payout capped at 2 years' basic salary ($24 \times \text{Basic}$).
  * Part-Time Proration (Cabinet Resolution No. 1/2022 Art. 30):
    $$\text{Part-Time Gratuity} = \left( \frac{\text{Actual Contracted Hours}}{2,080} \right) \times \text{Full-Time Gratuity}$$
  * Settlement Deadline: Article 53 mandates disbursement within **14 calendar days** from termination date. Employer deductions capped at maximum 25% of terminal package.
- **MoHRE Nafis Emiratisation Engine:**
  * Mandate applies to private mainland entities employing 50+ skilled workers.
  * 2% annual increment (1% semi-annual compliance evaluated June 30 and December 31), reaching 10% skilled Emiratisation by end of 2026.
  * Statutory Non-Compliance Penalties: Fixed at **AED 8,000 per month** (AED 96,000/year) in 2026 for every unfilled national position.

#### 5. Procurement & Facilities Specialists (`qala_procurement`, `qala_ops`)
- `procurement_three_way_match(po_number, invoice_number, grn_number)`: Matches Purchase Order, Invoice, and Goods Received Note.
- `procurement_sanctions_check(vendor_name, uae_tax_id)`: UAE AML/CFT screening.
- `ops_dewa_audit_commercial_bill(account_no, kwh_consumed, billed_aed)`: Audits commercial slab electricity bills.

### 3.3 DESC ISR v3.1 Enterprise Security Controls
- **Data Classification:** 4 mandatory tiers: `Public`, `Restricted`, `Confidential`, `Secret`.
- **Data Residency Safeguards:** All digital records classified as `Confidential` or `Secret` must reside exclusively within the UAE. Transmission to foreign public cloud systems or extraterritorial model APIs is prohibited under Domains 1 and 11.
- **Edge Deployment:** Edge appliances (Minisforum UM790 Pro) must implement AES-256 storage encryption, TLS 1.3 encrypted pipelines, and role-based access isolation.

---

## 4. AGENT 3: NIBRAS (BEST PERSONAL AI AGENT)

### 4.1 Dubai Civic Navigator (6 Statutory Engines)

#### 1. GDRFA 10-Year Golden Visa Eligibility Engine
- **Real Estate Investor Category:**
  * Minimum Valuation: Aggregate purchase value $\ge \text{AED 2,000,000}$ certified on DLD Title Deed.
  * Mortgaged Properties: Bank letter confirming applicant's paid equity $\ge \text{AED 2,000,000}$.
  * Off-Plan Assets: Valid under approved DLD Oqood contract showing net payments $\ge \text{AED 2,000,000}$.
- **Skilled Professional Category:**
  * Active employment contract under MoHRE Occupational Classification Level 1 (Managers/Directors) or Level 2 (Professionals).
  * Minimum monthly basic salary: **AED 30,000.00** (excluding allowances).
  * Attested Bachelor's degree authenticated by MoFA and equivalency granted by MoE.
- **Family Sponsorship:** Uncapped sponsorship of spouse, children (unmarried daughters no age limit, sons up to 25), and parents (1-year renewable).

#### 2. DLD Ejari & Decree No. 43 of 2013 Rental Increase Bracket
Permissible rent increases are calculated using the gap percentage between current contractual rent ($R_{\text{current}}$) and the RERA Smart Rental Index benchmark ($R_{\text{market}}$):
$$\Delta_{\text{gap}} = \left( \frac{R_{\text{market}} - R_{\text{current}}}{R_{\text{market}}} \right) \times 100\%$$

| Tier | Gap from RERA Market Index ($\Delta_{\text{gap}}$) | Maximum Allowable Rent Increase | Statutorily Permitted New Rent ($R_{\text{new}}$) | Governing Legal Reference |
|:---|:---|:---|:---|:---|
| **1** | $\Delta_{\text{gap}} \le 10.0\%$ | **0%** (No increase permitted) | $R_{\text{current}}$ | Decree No. 43 of 2013, Art. 1(a) |
| **2** | $11.0\% \le \Delta_{\text{gap}} \le 20.0\%$ | **5% maximum** | $R_{\text{current}} \times 1.05$ | Decree No. 43 of 2013, Art. 1(b) |
| **3** | $21.0\% \le \Delta_{\text{gap}} \le 30.0\%$ | **10% maximum** | $R_{\text{current}} \times 1.10$ | Decree No. 43 of 2013, Art. 1(c) |
| **4** | $31.0\% \le \Delta_{\text{gap}} \le 40.0\%$ | **15% maximum** | $R_{\text{current}} \times 1.15$ | Decree No. 43 of 2013, Art. 1(d) |
| **5** | $\Delta_{\text{gap}} > 40.0\%$ | **20% maximum** | $R_{\text{current}} \times 1.20$ | Decree No. 43 of 2013, Art. 1(e) |

- **Statutory Notice Period:** Article 14 of Law No. 33 of 2008 mandates formal written notice served at least **90 calendar days** prior to lease expiry. Failure to serve notice renders any increase null and void.

#### 3. DEWA Residential Utility Slab and Tariff Calculation Models
- **Refundable Security Deposits:** Residential Apartment: AED 2,000.00; Residential Villa: AED 4,000.00.
- **Expatriate Residential Electricity Slab Tariffs:**
  * Green Slab ($1\text{ to }2,000\text{ kWh}$): 23 fils / kWh
  * Yellow Slab ($2,001\text{ to }4,000\text{ kWh}$): 28 fils / kWh
  * Orange Slab ($4,001\text{ to }6,000\text{ kWh}$): 32 fils / kWh
  * Red Slab ($> 6,000\text{ kWh}$): 38 fils / kWh
- **Expatriate Residential Water Slab Tariffs:**
  * Green Slab ($1\text{ to }6,000\text{ IG}$): 3.5 fils / IG
  * Yellow Slab ($6,001\text{ to }12,000\text{ IG}$): 4.0 fils / IG
  * Red Slab ($> 12,000\text{ IG}$): 4.6 fils / IG
- **Fuel Surcharges:** Electricity: 5.0 fils / kWh; Water: 0.6 fils / IG.
- **Housing Fee:** 5% of annual Ejari contract value divided across 12 monthly bills.

#### 4. RTA Smart Mobility Engine
- **Salik Variable Toll Pricing:** 10 electronic gates (including Business Bay Crossing and Al Safa South). Dynamic toll: AED 6.00 (peak hours), AED 4.00 (off-peak), Free (10:00 PM to 6:00 AM weekdays).
- **Vehicle Registration Renewal Checklist:** Technical inspection fee (AED 170, valid 30 days), 13-month insurance certificate, traffic fines clearance, renewal fee of AED 350 + AED 20 Knowledge/Innovation fee.
- **Parking Zones:** Zone A (AED 4/hr), Zone B (AED 3/hr), Zone C (AED 2/hr), Zone D (AED 2/hr).

#### 5. KHDA Private School Fees Framework
Allowable tuition fee adjustments are tied to the annual Education Cost Index (ECI) and Dubai Schools Inspection Bureau (DSIB) ratings:
- Maintained Rating: $1.0 \times \text{ECI}$
- Improved (Very Weak $\rightarrow$ Weak): $2.0 \times \text{ECI}$
- Improved (Weak $\rightarrow$ Acceptable): $2.0 \times \text{ECI}$
- Improved (Acceptable $\rightarrow$ Good): $2.0 \times \text{ECI}$
- Improved (Good $\rightarrow$ Very Good): $1.75 \times \text{ECI}$
- Improved (Very Good $\rightarrow$ Outstanding): $1.5 \times \text{ECI}$
- Dropped in Rating: **0% increase** (Strict fee freeze).

#### 6. UAE Pass PKI & Digital Signature Engine
Local cryptographic verification of digital identity and document signing without cloud telemetry.

### 4.2 FadeMem Biological Memory & Context Pruning
- **Decay Formula (Ebbinghaus curve):**
  $$R = e^{-\frac{t}{S}}$$
  Where $t$ is elapsed operational time, and $S$ is memory stability:
  $$S = S_{\text{base}} \cdot (1 + \alpha \cdot \ln(1 + n_{\text{access}})) \cdot w_{\text{statutory}}$$
- **Pruning Policy:** Nodes falling below retention threshold ($\theta_{\text{decay}} = 0.25$) are pruned from the LLM prompt and migrated to local archive. Executive commitments and statutory rules pinned with $I = 1.0$ (immune to decay).
- **Efficiency:** Achieves sustained $>85\%$ token footprint reduction.
- **Cognitive Friction Index (CFI):**
  $$\text{CFI} = 0.3 \cdot \Delta_{\text{complexity}} + 0.3 \cdot (1 - \text{Confidence}_{\text{RAG}}) + 0.4 \cdot \text{Risk}_{\text{Regulatory}}$$
  When $\text{CFI} > 0.70$, the agent suppresses casual conversation and forces structured declarative steps.
- **Sovereign Privacy:** 100% on-device air-gap compliant with UAE Federal Decree-Law No. 45 of 2021 (PDPL).

---

## 5. REPOSITORY SPRINT ROADMAP & MASTER TODO LIST

| Sprint ID | Repository | Milestone Name | Scope & Core Deliverables | Target Exit Criteria |
|:---|:---|:---|:---|:---|
| **SPRINT-00** | `Dubai` | Master Harness & Gomaa Sync | Extend `~/.gomaa/gomaa.db` with `sprint_tasks` table and seed sprint backlog. Commit Master Specification to `Dubai/docs/`. | All 4 repos synced under Tri-Mirror Law. |
| **SPRINT-01** | `taryaq-agent` | Rust Engine Skeleton & FFI Bridge | Initialize standalone Rust crate (`Cargo.toml`). Link `libdubai_adapters` C99 safety kernel via FFI. Pass existing C99 test suite from Rust. | `cargo test` executes and passes all 20 C99 unit checks. |
| **SPRINT-02** | `taryaq-agent` | Empty Clinical Vessel & 30 Interactions | Implement generalized SQLite schema (`formulary_drugs`, `drug_interactions`, `hitl_clinical_quarantine`). Insert 30 high-yield interaction rules. | SQLite queries resolve in $<0.1\text{ ms}$; unmodeled drugs fail closed. |
| **SPRINT-03** | `taryaq-agent` | Tri-Modal Interfaces & Voice Pipeline | Implement Clinician Ambient Copilot, Pharmacist Verification Console, and Patient Health Companion with dialectal Arabic red-flag triage. Connect local `faster-whisper`. | Dialectal emergency triggers 998 dispatch; SOAP notes drafted. |
| **SPRINT-04** | `taryaq-agent` | HITL Web Ingestion & Quarantine | Build background web scanner for PubMed/FDA alerts. Implement human-in-the-loop review workflow for Mohamed, BPharm. | Unapproved web alerts remain in quarantine; approved alerts merge to DB. |
| **SPRINT-05** | `qala-agent` | Standalone Rust Engine & Agency Mesh | Initialize standalone Rust crate. Link DESC ISR v3.1 and Tau-3 gates via FFI. Implement Managing Director + 6 Departmental Specialists. | Cross-departmental container clearance scenario executes. |
| **SPRINT-06** | `qala-agent` | Trade & Labor Regulatory Engines | Codify Dubai Customs Mirsal II (10.25% landed tax) and UAE Labor Law Art. 51 gratuity calculation with Nafis quotas. | 100% accurate mathematical calculations against statutory formulas. |
| **SPRINT-07** | `nibras-agent` | Standalone Rust Engine & Civic Navigator | Initialize standalone Rust crate. Codify GDRFA Golden Visa, DLD Ejari Decree 43/2013 5-tier bracket, DEWA slab tariffs, and KHDA framework. | All 6 civic calculators pass unit test battery. |
| **SPRINT-08** | `nibras-agent` | FadeMem Memory & Sovereign Vault | Implement biological decay ($R = e^{-t/S}$), CFI HUD, and local SQLite Family Office Vault under UAE PDPL Law 45. | $>85\%$ token reduction verified; zero cloud data egress. |
| **SPRINT-09** | `Dubai` | Full Integration & VPS Hermes Review | Run `scripts/verify_all.sh` across all repos. Trigger Hermes on VPS `187.124.2.26` for adversarial audit and PR certification. | All 3 standalone Rust agents compile, verify, and pass with zero defects. |
