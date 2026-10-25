# TARYAQ (ترياق) AGENT SESSION INSTRUCTIONS & OPERATOR GUIDELINES
## Best Societal AI Agent | Dubai Create AI Agents Championship 2026–2027
### Target Workspace: `/Users/mohamedfathy/Desktop/taryaq-agent`
### Remote Origin: `https://github.com/M4F-S/taryaq-agent.git`
### Google Drive Mirror: `My Drive/Dubai_AI_Championship_2027/taryaq-agent/`

---

### 1. OPERATOR IDENTITY & COMMUNICATION RULES
1. **Who the Operator is:** Mohamed Fathy, BPharm (Licensed Clinical Pharmacist & Lead Systems Architect).
2. **Clear & Simple Language:** Always discuss problems and plans in **clear, simple, everyday English**. Never overwhelm the operator with dense academic jargon or robotic multiple-choice questions. If a decision is needed, explain the practical real-world trade-off simply.
3. **Research-First Protocol:** **NEVER guess, assume, or invent medical rules, drug interactions, or system architectures.** Always ask Mohamed for his clinical research and reference literature before making architectural or clinical decisions. Rely strictly on the research provided (e.g. `docs/research/HEALTHCARE_AI_AGENTS_DIRECTORY_2026.md`).
4. **Engineering First (Strictly No Hype):** Focus 100% on **building, refactoring, and testing the software**. Do NOT talk about pitch decks, demo videos, or peripheral hardware specs until the agent code is 100% complete, hardened, and verified with passing tests.
5. **Tri-Mirror Synchronization:** The local folder, Google Drive mirror, and GitHub repo **must remain 100% identical mirrors at all times**. Whenever code or documentation is created or modified, run `./sync_mirror.sh` to update both Google Drive and GitHub immediately.

---

### 2. UNDERLYING ENGINE FOUNDATION: BELYA + ALMAZ
- **Primary Foundation Engine:** **`Belya` (`M4F-S/Belya`)**
  - Taryaq's core clinical decision engine is built on **Belya's deterministic C99 architecture**.
  - *Why:* Clinical pharmacovigilance and DHA SN 03/2019 claims scrubbing demand microsecond determinism (< 0.02 ms), zero dynamic heap allocation, and zero-hallucination hard safety gates.
- **Secondary Integration:** **`Almaz` (`M4F-S/Almaz`)**
  - Taryaq's patient adherence and chronic symptom tracking interface leverages **Almaz's longitudinal memory principles** to follow patient recovery over weeks without context window saturation.

---

### 3. ARCHITECTURAL SCOPE: TRI-MODAL MEDICAL AGENT
Taryaq is a generalist medical AI agent specialized for three core healthcare stakeholders:
1. **For Clinicians (Doctors & Polyclinics):**
   - Ambient consultation listening: translates spoken Arabic dialects (Gulf, Levantine, Egyptian) into standardized medical English SOAP notes.
   - Operates strictly under the **"agent drafts, clinician signs"** non-SaMD framework (ensuring zero physician liability).
   - Real-time pre-order Clinical Decision Support (CDS) hooks for acute renal and drug-drug alerts.
2. **For Pharmacists (Community & Hospital Dispensaries):**
   - Longitudinal polypharmacy trajectory tracking (avoiding binary alert fatigue like FeelBetter).
   - Pre-submission DHA SN 03/2019 claims cycle scrubber (§3.7.1, §3.7.2, §3.8.1, §3.8.4, §3.9.3).
   - Fail-closed interaction engine: **never return SAFE for unmodeled drugs**.
   - Single-use clinician override verification with SHA-256 cryptographic audit chaining.
3. **For Patients (Chronic Care & Intake Companion):**
   - Dialectal Arabic conversational intake and symptom assessment.
   - Plain-language medication counselor ("take with food", "avoid grapefruits").
   - Hard red-flag emergency transfer gate (instant escalation for chest pain, acute unilateral weakness).

---

### 4. IMMEDIATE ENGINEERING ROADMAP
When continuing development in this session, focus on these concrete coding tasks:
- [x] **Task 1: Fail-Closed Clinical Gate:** In `adapters/taryaq_clinical_adapter.c`, expand the rule matrix to include high-risk pairs (Simvastatin + Clarithromycin, Warfarin + Amiodarone). If either drug is unrecognized or unmodeled, return `UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW` instead of `CLINICAL_SAFE`.
- [x] **Task 2: Dynamic Patient Context in Claim Scrubber:** In `src/taryaq_agent.c`, pass the actual prescription medication and patient eGFR into `claim_scrub` rather than static placeholders.
- [x] **Task 3: Red-Flag Emergency Triage Module:** Add deterministic C99 triage checking in `adapters/taryaq_clinical_adapter.c` for patient-reported red-flag symptoms.
- [x] **Task 4: Build & Test Verification:** Run `make clean && make test` and ensure all suites pass with zero warnings.

---

### 5. HOW TO VERIFY & TEST
```bash
# Compile and run the complete test suite
make clean && make test

# Synchronize with Google Drive and GitHub
./sync_mirror.sh
```
