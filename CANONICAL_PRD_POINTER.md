# CANONICAL SPECIFICATION POINTER: TARYAQ (ترياق)
**Dubai Create AI Agents Championship 2026–2027**  
**Category:** Best Societal AI Agent  
**Founder & Clinical Lead:** Mohamed Fathy, BPharm  
**Status:** **IMMUTABLE SPECIFICATION (V2.0-FROZEN)**  

---

## Notice to Implementer Agents in `taryaq-agent`
You are the dedicated implementation agent for **Taryaq (ترياق)**. 
All architecture and clinical requirements are frozen and governed by the master document:
👉 **[`/Users/mohamedfathy/Desktop/Dubai/docs/PRD_CANONICAL_SPEC_2026.md`](file:///Users/mohamedfathy/Desktop/Dubai/docs/PRD_CANONICAL_SPEC_2026.md)**

### Your Scope & Invariants:
1. **End-to-End Clinical Lifecycle:**
   - **Stage 1 (Diagnosis):** Ingest CC, HPI, vitals, labs $\rightarrow$ generate ranked Differential Diagnosis (DDx) and recommended diagnostic workups.
   - **Stage 2 (Clinical Research):** Medical RAG over authoritative guidelines (KDIGO 2024, ADA, ACC/AHA, GOLD, GINA).
   - **Stage 3 (Writing Medicine):** Precision posology matching eGFR (CKD-EPI formula), hepatic reserve, age, pregnancy. Dynamic class-based interaction shielding.
   - **Stage 4 (Comprehensive Care Plan):** Prescriptions, scheduled diagnostic re-testing (e.g. repeat Cr/K+ in 7–14 days), lifestyle/dietary guidance, clinical target milestones.
   - **Stage 5 (Pharmacy & Claims):** Dubai `eClaimLink` DHA SN 03/2019 delay penalty calculation with 30-day grace rule ($\max(0, \text{days}-30) \times 0.03\% \times \text{Net Claim}$) and UAE Law 30 of 2021 Controlled Substance checks.
   - **Stage 6 (24/7 Patient Health Companion):** Lay Arabic/English translation, chronobiology-aware daily dosing schedules, missed-dose protocols, food/beverage interaction safeguards, and continuous symptom tracking with emergency red-flag intercepts.
2. **Real Knowledge Base (Ban on 8-Drug Toy Lists):**
   - You must NOT rely on hardcoded 8-drug if-statements (`if (has_colchicine && has_clarithro)`).
   - Ingest a real structured SQLite clinical formulary database (100+ active ingredients categorized by pharmacological classes: CYP3A4 inhibitors/substrates, P-gp, RAAS, Statins, Anticoagulants, NSAIDs).
3. **Execution Safety:**
   - ISO C99 safety engine (`-std=c99 -Wall -Wextra -Werror -O2`).
   - HMAC-SHA256 tamper-evident backward-chained audit ledger (`taryaq_audit.jsonl`).
4. **Cross-Platform:**
   - Target deployment on iOS, Android, Web, and Desktop via WebAssembly and responsive Next.js PWA.
