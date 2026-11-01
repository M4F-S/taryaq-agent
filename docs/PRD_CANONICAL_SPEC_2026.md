# CANONICAL PRODUCT REQUIREMENTS DOCUMENT (PRD)
## Dubai Create AI Agents Championship 2026–2027
**Document Tag:** `PRD-CANONICAL-V2.0-FROZEN`  
**Status:** **NON-EDITABLE MASTER SPECIFICATION & ARCHITECTURAL BASELINE**  
**Lead Systems Architect & Founder:** Mohamed Fathy, BPharm  
**Role:** Master System Orchestrator  
**Date of Ratification:** 2026-10-29  

---

## 1. Executive Mandate & Strategic Vision

### 1.1 The Championship Mission
The **Dubai Create AI Agents Championship 2026–2027** (organized by Dubai Chamber of Digital Economy / Dubai Future Foundation under the patronage of H.H. Sheikh Hamdan bin Mohammed bin Rashid Al Maktoum) awards **AED 2,500,000+** to the world's most capable autonomous AI agents. Our competition portfolio consists of three breakthrough, sovereign entries:
1. **Best Societal AI Agent:** **Taryaq (ترياق)** — Sovereign Clinical Decision Support, Diagnostic & Therapeutic Care Planning, Pharmacovigilance & 24/7 Patient Health Companion.
2. **Best Business AI Agent:** **Qala (القلعة)** — Autonomous Enterprise Operating System & Corporate Agency Mesh (Head Executive Agent + 6 Departmental Specialists).
3. **Best Personal AI Agent:** **Nibras (نبراس)** — Sovereign Life Operating System, Dubai Civic Navigator, Family Office Wealth Vault & Executive Cognitive Exocortex.

### 1.2 The Core Invariants
- **No Mock Models / No Compliance Theater:** All keyword-hash mocks (e.g. fake Laya MD5 scripts) are eliminated. Every agent is powered by a real local LLM (via GGUF / `llama.cpp`) or sovereign private endpoint, strictly governed by an unbypassable C99 safety substrate.
- **Universal Cross-Platform Delivery:** Every agent must run natively across **iOS, Android, Web, and Desktop** via WebAssembly (WASM) and a responsive, touch-friendly application.
- **Strict Separation of Concerns:**
  - **Master Orchestrator (in `Dubai`):** Owns global architecture, canonical PRDs, cross-repo integration tests, and Tri-Mirror Law auditing.
  - **Dedicated Implementer Agents (in `taryaq-agent`, `qala-agent`, `nibras-agent`):** Execute repository-local implementations, database builds, and specialized UI views without cross-repo entanglement.
- **The Neuro-Symbolic "Two-Brain" Law:**
  - **Brain 1 (Generative Neural Agent):** Fluent Arabic/English dialogue, reasoning, chart analysis, and domain synthesis.
  - **Brain 2 (Sovereign Deterministic Governor):** Bare-metal ISO C99 safety engine enforcing statutory rules, sub-millisecond mathematical boundaries, and HMAC-SHA256 tamper-evident audit ledgers.

---

## 2. Global System Architecture

```
+───────────────────────────────────────────────────────────────────────────────────────────────────+
|                                  UNIFIED USER-FACING PLATFORMS                                    |
|                       iOS (iPhone/iPad) • Android • Web Browser • Desktop (macOS/Win)             |
+───────────────────────────────────────────────────────────────────────────────────────────────────+
                                                  │
                                                  ▼
+───────────────────────────────────────────────────────────────────────────────────────────────────+
|                           TIER 1: GENERATIVE NEURAL AGENT (BRAIN 1)                               |
|        • Models: Local GGUF (Llama-3.2-3B / Qwen-2.5-7B) or Sovereign Private Endpoint             |
|        • Multi-Turn Conversational Reasoning • Bilingual Arabic/English Code-Switching            |
|        • Role Personas: Doctors, Pharmacists, Patients, Corporate Executives, Citizens            |
+───────────────────────────────────────────────────────────────────────────────────────────────────+
                                                  │ (Proposed Actions, Prescriptions, Tool Calls)
                                                  ▼
+───────────────────────────────────────────────────────────────────────────────────────────────────+
|                        TIER 2: SOVEREIGN DETERMINISTIC GOVERNOR (BRAIN 2)                         |
|        • ISO C99 Core (-std=c99 -Wall -Wextra -Werror -O2) • Sub-Millisecond Latency             |
|        • Unbypassable Boundary Gates:                                                             |
|          - Taryaq: Class-Based Pharmacology Matrix & DHA SN 03/2019 Claim Arithmetic              |
|          - Qala: DESC ISR v3.1 Path Sandbox & Tau-3 FinOps AED 50,000 Approval Gate              |
|          - Nibras: FadeMem Ebbinghaus Biological Memory Decay & Token Pruning (>85%)              |
|        • Grounding Shield: Verification of numerical/factual claims against statutory manifests   |
+───────────────────────────────────────────────────────────────────────────────────────────────────+
                                                  │ (Signed Execution & Ledger Logging)
                                                  ▼
+───────────────────────────────────────────────────────────────────────────────────────────────────+
|                         TIER 3: CRYPTOGRAPHIC AUDIT & STATUTORY STORAGE                           |
|        • HMAC-SHA256 Hash-Chained Audit Ledger (Tamper-Evident Backward Linking)                  |
|        • Single-Use Cryptographic Override Codes (Replay Attack Prevention)                       |
|        • Full Data Residency: UAE Health Data Law No. 2 of 2019 & UAE PDPL Law No. 45 of 2021     |
+───────────────────────────────────────────────────────────────────────────────────────────────────+
```

---

## 3. Agent 1 Specification: TARYAQ (ترياق)
**Track:** Best Societal AI Agent  
**Domain:** End-to-End Clinical Intelligence, Diagnostic Support, Pharmacotherapy, Claim Firewall & Patient Care  
**Founder & Clinical Lead:** Mohamed Fathy, BPharm  

### 3.1 The Complete End-to-End Clinical Lifecycle

```
[1. DIAGNOSIS ASSISTANCE]  Chief Complaint & Labs ──> Ranked Differential Diagnosis (DDx)
            │
            ▼
[2. CLINICAL RESEARCH]     Authoritative Medical RAG ──> Guidelines & Clinical Trials (KDIGO, ADA, ACC/AHA)
            │
            ▼
[3. WRITING MEDICINE]      Phenotype-Matched Rx ──> Class-Based Pharmacology, Posology & Renal Titration
            │
            ▼
[4. COMPLETE CARE PLAN]    Diagnostic Follow-up, Lifestyle Interventions, Monitoring & Clinical Targets
            │
            ▼
[5. PHARMACY & CLAIMS]     Formulary Verification, Law 30 Controlled Checks & DHA SN 03/2019 Claim Scrub
            │
            ▼
[6. PATIENT COMPANION]     24/7 Dosing Schedules, Lay-Translation, Symptom Tracking & Red-Flag Triage
```

#### Step 1: Diagnostic Assistance & Differential Diagnosis (DDx)
- **Clinical Intake & Synthesis:** Ingests Chief Complaint (CC), History of Present Illness (HPI), Review of Systems (ROS), Vitals, Physical Exam, and Lab panels. Formats structured clinical SOAP notes (Subjective, Objective, Assessment, Plan).
- **Ranked Differential Diagnosis (DDx):** Synthesizes symptoms, signs, and biomarkers to generate a prioritized differential diagnosis:
  - *Primary Suspected Etiology* with probability rationale.
  - *Secondary Differentials* to consider in parallel.
  - *"Must-Not-Miss" Red Flags* (e.g., Atypical Acute Coronary Syndrome, Sepsis, Pulmonary Embolism, Acute Mesenteric Ischemia).
- **Diagnostic Workup Recommendations:** Suggests high-yield, cost-effective confirmatory lab tests, imaging modalities, and biomarker panels (e.g. Troponin-I, BNP, eGFR, LFTs, D-dimer, HbA1c, urine albumin-to-creatinine ratio).

#### Step 2: Clinical Research & Evidence Retrieval (Deep Medical RAG)
- **Authoritative Practice Guidelines:** Direct, section-chunked indexing of gold-standard clinical guidelines:
  - *Nephrology:* KDIGO (Kidney Disease: Improving Global Outcomes) 2024 Clinical Practice Guidelines.
  - *Endocrinology:* ADA (American Diabetes Association) Standards of Care in Diabetes.
  - *Cardiology:* ACC/AHA Heart Failure and Hypertension Guidelines.
  - *Pulmonology:* GOLD (COPD) and GINA (Asthma) Management Strategies.
  - *Statutory Dubai/UAE Pathways:* DHA Clinical Practice Guidelines and MoHAP Therapeutic Protocols.
- **Evidence-Based Rationale:** Every clinical diagnostic and therapeutic recommendation provides graded evidence (Class I/II/III, Level of Evidence A/B/C) with exact document citations (`doc_id`, `section`, `passage_sha256`).

#### Step 3: Precision Pharmacotherapy & Posology ("Writing Medicine")
- **Phenotype-Matched Drug Selection:** Tailors first-line, second-line, and add-on medications based on patient-specific parameters:
  - Renal function: Exact eGFR calculation (CKD-EPI formula) and stage-dependent dosing.
  - Hepatic reserve: Child-Pugh and MELD classification.
  - Age, weight/BMI, pregnancy/lactation safety categories.
  - Documented allergies and pharmacogenomic/CYP metabolic phenotype.
- **Precise Posology Formulation:**
  - Initial therapeutic dose, titration schedule, and maximum safe ceiling.
  - Administration directions in medical English and culturally fluent Arabic.
  - Dynamic renal/hepatic dose reductions (e.g. 50% dose reduction, extending dosing interval from q12h to q24h).
- **Class-Based Interaction Shield:** Deterministically intercepts:
  - Drug-Drug Interactions (CYP3A4, CYP2D6, CYP2C9, P-glycoprotein, QT prolongation).
  - Drug-Disease Contraindications (e.g., NSAIDs in Stage 4 CKD, Beta-blockers in severe active bronchospasm).
  - Drug-Food & Timing Interactions (grapefruit CYP3A4 inhibition, calcium/iron chelation with tetracyclines/quinolones).

#### Step 4: Comprehensive Diagnostic & Therapeutic Care Plan
Taryaq generates a structured, holistic patient management plan encompassing both medical and operational tracks:
1. **Pharmacological Prescriptions:** Complete electronic prescription orders with active ingredient, brand name, strength, dosage form, route, frequency, and duration.
2. **Diagnostic Monitoring & Re-Testing Schedule:**
   - Pre-scheduled laboratory follow-up (e.g., repeat serum creatinine and potassium 7–14 days after initiating ACEi/ARB or spironolactone; regular INR tracking for warfarin; therapeutic drug monitoring for vancomycin/tacrolimus).
   - Surveillance imaging and specialist referrals.
3. **Non-Pharmacological & Lifestyle Interventions:**
   - Specific dietary recommendations (e.g., sodium restriction < 2 g/day, potassium management, renal protein goals).
   - Hydration rules, physical activity guidelines, and smoking cessation support.
4. **Explicit Clinical Targets & Milestones:**
   - Target blood pressure (e.g. < 130/80 mmHg in proteinuric CKD).
   - Target HbA1c (< 7.0% or individualized for frail elderly).
   - Target LDL-C (< 55 mg/dL in very high cardiovascular risk).
5. **Patient Safety & Escalation Rules:**
   - Warning symptoms requiring immediate emergency hospital presentation.
   - Self-monitoring instructions (home blood glucose logs, daily morning weights for heart failure).

#### Step 5: Pharmacy Operations, Controlled Drugs & DHA Claims
1. **DHA SN 03/2019 Claim Cycle Scrubber:**
   - Evaluates Dubai `eClaimLink` initial submission and remittance advice resubmission windows.
   - Enforces statutory 30-day grace rule and computes exact daily delay penalties:  
     $$\text{Penalty (AED)} = \max(0, \text{days} - 30) \times 0.03\% \times \text{Net Claim Amount}$$
2. **UAE Controlled Substance Verification (Federal Law No. 30 of 2021):**
   - Classifies medications against MoHAP Schedule 1 (Narcotics), Schedule 2 (Psychotropics), and Semi-Controlled Lists.
   - Validates prescription duration limits (maximum 30 days for narcotics, 3-day emergency prescriptions) and mandatory Emirates ID logging.
3. **Therapeutic Alternative Recommender:** Suggests non-interacting therapeutic alternatives within the same pharmacological class when a severe contraindication is detected.

#### Step 6: The 24/7 Patient Health & Medication Companion
Completing the patient-facing loop so healthcare delivery does not stop when the patient leaves the clinic:
1. **Prescription Lay-Translation & Health Literacy:**
   - Decodes Latin abbreviations, complex dosages, and clinical jargon into clear, empathetic Arabic and English.
   - Explains the purpose of each medication (e.g. *"This tablet protects your kidneys and gently lowers your blood pressure"*) to maximize patient buy-in.
2. **Chronobiology-Aware Dosing Schedule:**
   - Builds an individualized daily medication timeline matching the patient's lifestyle and prayer times (e.g., statins at bedtime, diuretics in the morning, levothyroxine 30 minutes before breakfast on an empty stomach, separating calcium supplements from antibiotics by 4 hours).
3. **Intelligent Adherence & Missed-Dose Protocols:**
   - Provides safe, clinically verified rules when a dose is missed (e.g. *"Take it now if within 4 hours; if near your next dose, skip it. Never double the dose"*).
4. **Food, Drink & Lifestyle Safeguard:**
   - Warns against real-world hazards (avoiding grapefruit juice with amlodipine/simvastatin, avoiding high-potassium salt substitutes with ACE inhibitors, limiting caffeine/dairy during antibiotic courses).
5. **Continuous Symptom Monitoring & Emergency Red-Flag Intercept:**
   - Patients can describe how they feel in plain spoken or written Arabic/English.
   - Distinguishes expected mild side effects (mild dry mouth) from dangerous adverse drug events:
     * Dry irritating cough or facial swelling on Lisinopril $\rightarrow$ Halts medication, alerts physician for angioedema risk.
     * Severe unexplained muscle soreness on Statins $\rightarrow$ Instructs immediate blood test for rhabdomyolysis.
     * Black tarry stools or unusual bruising on Warfarin/NSAIDs $\rightarrow$ Triggers immediate emergency hospital presentation.
6. **Lab Results Literacy:**
   - Translates confusing laboratory printouts (eGFR, serum creatinine, HbA1c, lipid panel, AST/ALT) into plain-language status cards with longitudinal visual trends.

### 3.2 Real Class-Based Pharmacological Knowledge Base
Taryaq strictly rejects hardcoded 8-drug lists. It is powered by a structured SQLite formulary database:
- **Comprehensive Drug Ontology:** Active ingredients, brand names (Arabic & English), ATC codes.
- **Pharmacological Classes:** Strong/Moderate CYP3A4 inhibitors & inducers, CYP2D6, CYP2C9, P-glycoprotein inhibitors/substrates, Vitamin K Antagonists, DOACs, ACEi, ARBs, Biguanides, Macrolides, Azole Antifungals, Statins, NSAIDs.
- **Class-Level Interaction Engine:** A single rule dynamically protects **hundreds of drug pairs**:
  $$\text{Substrate}_{\text{CYP3A4/NTI}} + \text{Inhibitor}_{\text{Potent CYP3A4}} + (\text{eGFR} < 60) \implies \text{FATAL\_CONTRAINDICATION\_BLOCK}$$

---

## 4. Agent 2 Specification: NIBRAS (نبراس)
**Track:** Best Personal AI Agent  
**Domain:** Sovereign Life Operating System, Dubai Civic Navigator, Family Office Vault & Cognitive Exocortex  

### 4.1 Why Nibras Wins: Beyond Generic Chatbots
Generic personal chatbots fail because they are just ungrounded wrappers around OpenAI or Siri. Nibras wins by acting as the **Sovereign Personal Operating System & AI Twin for Dubai's Leaders, Founders, and Citizens**.

### 4.2 Core Breakthrough Capabilities

#### 1. The Dubai Life Navigator (Deep Civic & Municipal Integration)
Nibras is natively grounded in Dubai’s government and municipal ecosystem:
- **UAE Pass & Identity Lifecycle:** Tracks Emirates ID and passport renewals, family visa statuses, and Golden Visa compliance under GDRFA regulations.
- **Municipal & Utility Management:** Proactively manages DEWA utility thresholds, RTA vehicle registrations, Salik balance depletion warnings, and parking permits.
- **Real Estate & Tenancy Engine:** Tracks Ejari registration renewals, rental index increases according to Dubai Land Department (RERA) rental calculators, and property service charge milestones.
- **Education & Family Governance:** Synchronizes KHDA school academic calendars, fee payment milestones, and family health immunizations under DHA schedules.
- *Execution:* Instead of just telling you a link, Nibras drafts the exact paperwork, calculates statutory fees, and prepares the filing package.

#### 2. The Sovereign Family Office & Private Wealth Vault
Built for high-net-worth executives, investors, and family business principals:
- **Private Deal & Investment Memory:** Ingests term sheets, SAFE agreements, shareholder resolutions, and venture fund capital calls.
- **Corporate Governance Tracker:** Tracks board directorship commitments, fiduciary duties, voting rights, and conflict-of-interest declarations across multiple entities (DIFC, ADGM, Onshore).
- **Confidential Decision Vault:** Immutable record of private agreements, personal loans, family trust allocations, and strategic business commitments with line-range citations and SHA-256 integrity proofs.

#### 3. Biologically Grounded Cognitive Shield (FadeMem + Ebbinghaus + Valence)
Dubai executives face extreme information overload across WhatsApp, Telegram, email, and board packs. Nibras acts as an active **Cognitive Firewall**:
- **Biological Context Pruning:** Employs Ebbinghaus forgetting curves with importance scoring ($I \in [0.0, 1.0]$). Ephemeral chatter ($I < 0.25$) decays automatically; core commitments and statutory rules ($I = 1.0$) are permanently pinned.
- **Token Efficiency:** Consistently delivers **>85% context token reduction** (condensing 1,500+ raw conversational tokens into <150 active working memory tokens).
- **Cognitive Friction Index (CFI):** Computes a live mental bandwidth and cognitive valence score, proactively shielding the executive from meeting fatigue and alert exhaustion.

#### 4. Arabic-First Cultural & Dialect Resonance
- Bare-metal C99 zero-allocation normalizer handling Eastern Arabic digits (`٠-٩`), diacritics (tashkeel), and character variations.
- Fluent code-switching across **Emirati and Gulf Arabic dialects**, Modern Standard Arabic, and corporate English, honoring local etiquette and official protocol.

#### 5. 100% Sovereign On-Device Privacy (UAE PDPL Law No. 45 of 2021)
- 100% on-device sovereign execution.
- Zero private data, family records, wealth details, or voice audio ever leaves the physical device.

---

## 5. Agent 3 Specification: QALA (القلعة)
**Track:** Best Business AI Agent  
**Domain:** Autonomous Enterprise Operating System & Corporate Agency Mesh  

### 5.1 The Enterprise Multi-Agent Agency Mesh
Rather than a single static tool, Qala operates as an **Autonomous Agency of Specialist Agents** deployed inside any corporation to empower management and employees across all corporate domains:

```
                          ┌────────────────────────────┐
                          │    COMPANY ADMIN / CEO     │
                          │   (Executive Leadership)   │
                          └─────────────┬──────────────┘
                                        │ (Directives & Global Inquiries)
                                        ▼
                  ┌───────────────────────────────────────────┐
                  │          HEAD EXECUTIVE AGENT             │
                  │   (Corporate Chief of Staff / Director)   │
                  │ • Cross-Departmental Orchestration        │
                  │ • Shared Enterprise Memory & Routing      │
                  │ • Daily Executive Briefings & Governance  │
                  └──────┬──────────────┬──────────────┬──────┘
                         │              │              │
         ┌───────────────┼──────────────┼──────────────┼───────────────┐
         ▼               ▼              ▼              ▼               ▼
   ┌───────────┐   ┌───────────┐  ┌───────────┐  ┌───────────┐   ┌───────────┐
   │  FINANCE  │   │   LEGAL   │  │   TRADE   │  │    HR     │   │PROCURE/OPS│
   │ & FINOPS  │   │& COMPLIANCE│ │& CUSTOMS  │  │& PEOPLE   │   │& LOGISTICS│
   └─────┬─────┘   └─────┬─────┘  └─────┬─────┘  └─────┬─────┘   └─────┬─────┘
         │               │              │              │               │
         └───────────────┴──────────────┼──────────────┴───────────────┘
                                        ▼
                  ┌───────────────────────────────────────────┐
                  │       SOVEREIGN C99 SECURITY SUBSTRATE    │
                  │ • DESC ISR v3.1 Path Sandbox (Domain 14)  │
                  │ • Tau-3 FinOps AED 50,000 Approval Gate   │
                  │ • HMAC-SHA256 Chained Corporate Ledger    │
                  └───────────────────────────────────────────┘
```

#### 1. The Head Executive Agent (Managing Director & Chief of Staff)
- **Central Orchestration:** Decomposes complex executive mandates (e.g. *"Open a new logistics branch in JAFZA by Q3"*) into sub-tasks for Finance, Legal, Trade, HR, and Operations.
- **Enterprise Memory & Silo Breaking:** Maintains global organizational awareness, ensuring HR headcount updates instantly sync with Finance payroll forecasts and Legal visa quotas.
- **Consolidated Executive Briefings:** Delivers daily status summaries, bottleneck alerts, and financial exposure reports directly to company administrators.

#### 2. Specialized Departmental Sub-Agents
1. **Finance & FinOps Agent (`Qala-Finance`):**
   - Accounts payable/receivable automation and three-way invoice matching.
   - UAE Corporate Tax (9%) and FTA VAT (5%) reconciliation.
   - Cash flow forecasting and payroll auditing.
   - **Tau-3 FinOps Governance:** Autonomous approvals for spending $< \text{AED } 50,000.00$; strictly locks spending $\ge \text{AED } 50,000.00$ behind single-use admin approval codes.
2. **Legal, Regulatory & Compliance Agent (`Qala-Legal`):**
   - Enforces **DESC ISR v3.1** cybersecurity domains and data classification.
   - Commercial contract analysis, NDA drafting, and vendor SLA redlining.
   - UAE Labor Law (Federal Decree-Law No. 33 of 2021) compliance.
   - Free zone governance (DIFC, ADGM, DAFZA, DMCC regulations).
3. **Trade, Logistics & Customs Agent (`Qala-Trade`):**
   - Integrates with Dubai Customs (e-Mirsal II) and DP World container tracking.
   - Automated Harmonized System (HS) code classification and 5% customs tariff calculation based on CIF valuation.
   - Bill of lading validation, demurrage prevention, and import/export compliance.
4. **Human Resources & People Ops Agent (`Qala-HR`):**
   - Employee onboarding/offboarding workflows and automated offer letters.
   - UAE MOHRE quota tracking, work permit approvals, and Golden Visa processing.
   - Wages Protection System (WPS) compliance and leave balance auditing.
5. **Procurement, Vendor & Sales Agent (`Qala-Procure`):**
   - Supplier background vetting, RFP scoring, and competitive bid analysis.
   - Purchase order generation and contract milestone verification.
   - Client proposal generation and CRM deal tracking.
6. **Operations & Facilities Agent (`Qala-Ops`):**
   - Company Standard Operating Procedure (SOP) search and retrieval.
   - IT asset management, internal service desk automation, and facility maintenance scheduling.

#### 3. Sovereign C99 Security Substrate
All inter-agent communication, file access, and transaction executions route through the deterministic C99 core:
- **Path Sandboxing:** Strict workspace containment via `realpath()`; departments cannot snoop on unauthorized directories.
- **Tamper-Evident Ledger:** Every cross-departmental action and financial approval is chained using HMAC-SHA256 backward hashes.

---

## 6. Multi-Platform Delivery Matrix (iOS, Android, Web, Desktop)

Every agent must be available to end-users across all computing form factors:

| Platform | Deployment Technology | Execution Architecture | Target Audience Profile |
| :--- | :--- | :--- | :--- |
| **iOS (iPhone & iPad)** | Responsive Progressive Web App (PWA) / Safari Standalone | Client-Side WebAssembly (WASM) C99 Engine + Sovereign Bridge | Clinicians on hospital rounds, mobile executives, patients at home. |
| **Android** | Progressive Web App (PWA) / WebAPK | Client-Side WASM C99 Engine + Sovereign Bridge | Widespread community patient adoption, field technicians, retail pharmacists. |
| **Web Browser** | Next.js / React Application | WASM Core + Live WebSocket Telemetry HUD | Hospital administrative cockpits, corporate trade desks, enterprise administrators. |
| **Desktop (macOS / Windows)** | Tauri / PWA Desktop Shell | Native C99 dynamic library (`.dylib` / `.dll`) or local WASM | Heavy batch claims processing, continuous customs filing, executive command center. |

---

## 7. Multi-Agent Implementation Governance (Rules of the Project)

To ensure smooth collaboration between human architects and autonomous subagents:

### 7.1 Separation of Workspaces
- **Master Control Plane (`/Users/mohamedfathy/Desktop/Dubai`):**
  - Managed by the **Master System Orchestrator**.
  - Owns master integration tests (`make test`), canonical PRDs, shared corporate corpora, and Tri-Mirror Law verification.
  - **Does not write internal agent code** inside individual workspaces.
- **Dedicated Agent Workspaces:**
  - `taryaq-agent`: Clinical team builds the SQLite formulary database, clinical RAG corpus, and Doctor/Pharmacist/Patient views.
  - `qala-agent`: Enterprise team builds the Corporate Agency mesh, departmental knowledge bases, and ERP connectors.
  - `nibras-agent`: Personal team builds the Dubai Life Navigator, Family Office Vault, and FadeMem cognitive HUD.

### 7.2 The Immutable Pointer Invariant
Every repository contains an immutable pointer (`CANONICAL_PRD_POINTER.md`) referencing this document:
`Dubai/docs/PRD_CANONICAL_SPEC_2026.md`.
No implementer agent may deviate, invent mock stubs, or alter the architectural boundaries defined herein.

### 7.3 The Tri-Mirror Law
$$\text{Local Desktop Workspace} \equiv \text{Google Drive Mirror} \equiv \text{GitHub Remote Repository}$$
Every merged commit must pass `make clean && make test` and be synchronized via `./sync_mirror.sh`.

---

## 8. Phased Implementation Roadmap (Execution Phase)

### Phase 1: Real Knowledge Bases & Ontologies (Current Sprint)
- **Taryaq:** Ingest structured pharmacological ontology (100+ active ingredients, classes, renal cutoffs) into SQLite; update C99 engine to class evaluation; expand RAG corpus with SmPCs and BNF.
- **Qala:** Define the Corporate Agency schema and department-specific tool manifests.
- **Nibras:** Build the Dubai Civic Navigator ontology (UAE Pass, DEWA, RTA, GDRFA, DLD rules).

### Phase 2: Live AI Model & WebAssembly Portability
- Connect real local LLM runner (via GGUF / `llama.cpp` or sovereign endpoint).
- Compile C99 engines (`libtaryaq`, `libqala`, `libnibras`) to WebAssembly (`.wasm`).

### Phase 3: Unified Multi-Platform User Interfaces
- Build responsive Next.js PWA interfaces for Taryaq (Doctor/Pharmacist/Patient), Qala (Corporate Agency Mesh), and Nibras (Executive Life OS).

### Phase 4: Stage Video, Air-Gap Testing & Championship Submission
- Record 2.5-minute physical hardware demonstration.
- Final number traceability audit and submission to DCDE.
