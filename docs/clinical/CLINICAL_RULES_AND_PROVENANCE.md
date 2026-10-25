# TARYAQ CLINICAL RULES, PHARMACOKINETICS & STATUTORY PROVENANCE
## Sovereign Pharmacovigilance & DHA Claim Standards
### Lead Clinical Pharmacist & System Architect Reference

---

### 1. Clinical Pharmacological Interaction Rules

#### Rule 1: Colchicine + Potent CYP3A4 / P-gp Inhibitors in Renal Impairment
* **Primary Compound:** Colchicine (`كولشيسين`).
* **Interacting Compounds:** Clarithromycin (`كلاريثروميسين`, Klacid, Biaxin), Ketoconazole, Itraconazole.
* **Mechanism of Toxicity:** 
  Colchicine is heavily cleared via hepatic CYP3A4 metabolism and intestinal/renal P-glycoprotein (P-gp) efflux. Concomitant administration of potent dual CYP3A4 and P-gp inhibitors (especially Clarithromycin) dramatically increases Colchicine plasma concentrations (up to 300–400% AUC increase). In patients with baseline renal impairment ($eGFR < 60\text{ mL/min}$ or CKD Stage 3/4), this combination triggers rapid, fatal multiorgan failure, severe neuromyopathy, acute rhabdomyolysis, and life-threatening bone marrow aplasia.
* **Clinical Threshold:**
  $$\text{If } (Colchicine + Clarithromycin) \land (eGFR < 60.0\text{ mL/min}) \implies \mathbf{CLINICAL\_CONTRAINDICATION\_BLOCK}$$
* **Regulatory/Clinical Citations:** FDA Boxed Warning; BNF 84 Colchicine Monograph; EMA SmPC Colchicine.

---

#### Rule 2: Metformin Renal Contraindication & CKD Stage 3b Review
* **Primary Compound:** Metformin (`ميتفورمين`, Glucophage).
* **Mechanism of Toxicity:**
  Metformin is eliminated unchanged by renal tubular secretion. In severe renal impairment ($eGFR < 30\text{ mL/min}$), significant accumulation occurs, triggering life-threatening Metformin-Associated Lactic Acidosis (MALA), which carries a mortality rate exceeding 40%.
* **Clinical Thresholds:**
  1. **Severe Renal Contraindication:**
     $$\text{If } Metformin \land (eGFR < 30.0\text{ mL/min}) \implies \mathbf{CLINICAL\_CONTRAINDICATION\_BLOCK}$$
     *Alert Code:* `METFORMIN_SEVERE_RENAL_CUTOFF_EGFR_LT_30`
  2. **CKD Stage 3b Dose-Adjustment Warning:**
     $$\text{If } Metformin \land (30.0 \le eGFR < 45.0\text{ mL/min}) \implies \mathbf{CLINICAL\_CLAIM\_CYCLE\_WARN}$$
     *Alert Code:* `METFORMIN_CKD_STAGE_3B_DOSE_REDUCTION_REQUIRED`
     *Action:* Flag for dose reduction (max 1000 mg/day) and quarterly renal monitoring.
* **Regulatory/Clinical Citations:** KDIGO 2024 Clinical Practice Guideline for Diabetes Management in CKD; FDA Drug Safety Communication (Metformin renal safety).

---

#### Rule 3: Controlled Medication Verification (UAE Law No. 30 of 2021)
* **Target Compounds:** Controlled and semi-controlled drugs (e.g. Pregabalin / Lyrica, Tramadol, Benzodiazepines).
* **Clinical Rule:**
  $$\text{If } is\_controlled \land (\neg has\_controlled\_id\_verified) \implies \mathbf{CLINICAL\_CONTROLLED\_DRUG\_GATE}$$
* **Action:** Hard hold until Emirates ID is verified, daily dose ceilings are confirmed, and the dispensing pharmacist co-signs.

---

### 2. DHA Standards Notice SN 03/2019 Claim Cycle Scrubber

All outpatient claims routed to the Dubai Health Post Office (`eClaimLink` / DHPO) are governed by statutory submission timelines:

| Section | Statutory Provision | Operational Requirement | Taryaq Engine Action |
| :--- | :--- | :--- | :--- |
| **§3.7.1** | Initial Submission Deadline | Claims must be submitted by the 15th calendar day of the month following the service date. | Flags claims generated after the 15th. |
| **§3.7.2** | Initial Delay Penalty | **0.03% of Net Claimed amount per calendar day of delay** for late initial submissions. | Calculates exact AED exposure deducted from provider payout. |
| **§3.8.1** | Resubmission Window | Resubmissions must be uploaded within **21 calendar days** from Remittance Advice receipt. | Computes calendar days elapsed since remittance. |
| **§3.8.2** | Maximum Resubmissions | Maximum of **two (2) resubmissions** permitted per claim. | Flags 3rd submission attempts for administrative rejection. |
| **§3.8.4** | Resubmission Delay Penalty | **0.03% of Net Claimed amount per calendar day of delay** for late resubmissions. | Calculates exact AED penalty deducted from net claim. |
| **§3.9.3** | Payer Settlement Delay Fee | **0.03% per calendar day delay fee owed by Insurers/Payers** to Providers for late payment past the statutory window. | **Revenue Recovery:** Audits overdue remittances and calculates interest penalties owed to the clinic. |
