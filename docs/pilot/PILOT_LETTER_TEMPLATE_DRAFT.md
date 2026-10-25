# CLINICAL EVALUATION LETTER OF INTENT (LOI) — DRAFT TEMPLATE
## Sovereign Clinical Decision Support & DHA Reimbursement Pre-Submission Scrubber

> [!CAUTION]
> **DRAFT — UNSIGNED — NOT AN AGREEMENT**  
> This document is a standardized clinical pilot engagement template prepared for prospective partner discussions in Dubai, UAE.  
> It does **NOT** represent an executed, binding, or signed legal agreement.  
> No institutional partnership is in effect until formally reviewed, approved, and countersigned by authorized healthcare officers.

---

### PROPOSED INSTITUTIONAL PARTNERS

* **Prospective Healthcare Partner:** **[PARTNER HEALTHCARE ENTITY NAME]**  
  * Facility Name: **[CLINICAL FACILITY / COMMUNITY PHARMACY NAME]**  
  * DHA Facility License Number: **[DHA FACILITY LICENSE NUMBER]**  
  * MOHAP Establishment ID: **[MOHAP ESTABLISHMENT ID]**  
  * Facility Location: **[FACILITY PHYSICAL ADDRESS, DUBAI, UAE]**  
* **Technology Developer:** **Taryaq Sovereign Clinical AI Systems**  
  * Lead Clinical Pharmacist & Founder: **Mohamed Fathy, BPharm** (Licensed Clinical Pharmacist)  
  * Jurisdiction: Dubai Internet City / Dubai Silicon Oasis, Dubai, UAE  

---

### PROPOSED UNDERTAKING & SCOPE (EVALUATION DRAFT)

The purpose of this non-binding Letter of Intent (LOI) is to establish the prospective terms under which **[PARTNER HEALTHCARE ENTITY NAME]** and **Taryaq Sovereign Clinical AI Systems** may collaborate to evaluate the **Taryaq v0** software suite during Q1 2027:

```text
"Subject to formal execution of a definitive clinical evaluation agreement, [PARTNER HEALTHCARE ENTITY NAME] intends to collaborate with Taryaq Sovereign Clinical AI Systems to conduct an on-premises prospective shadow evaluation of the Taryaq v0 clinical safety decision support engine during Q1 2027 (target window: February 1 – March 15, 2027), and to evaluate anonymised safety and financial metrics for the Dubai AI Agents Championship 2026–2027."
```

---

### PROPOSED CLINICAL & REGULATORY TERMS

1. **Non-Interventional Advisory Mode (Shadow Evaluation):**  
   The evaluation shall operate in strict `SHADOW_ADVISORY_MODE`. Taryaq v0 shall have **zero autonomous dispensing, prescribing, or medical record altering authority**. Licensed facility clinical pharmacists and attending physicians retain sole and exclusive clinical decision-making and legal responsibility.

2. **Data Residency & Zero-Egress Mandate (UAE Federal Law No. 2 of 2019):**  
   Pursuant to Article 13 of UAE Federal Law No. 2 of 2019 on the Use of Information and Communication Technology (ICT) in Health Fields, all computational processing shall occur strictly on a dedicated, on-premises physical appliance deployed inside the facility's secure server enclave. No patient health information (PHI), identifiable data, or prescription telemetry shall be transmitted outside the facility premises or to public cloud infrastructure.

3. **Statutory Claim Cycle Scrubber Evaluation (DHA Standards Notice SN 03/2019):**  
   The software shall shadow-audit outpatient electronic billing claims prior to submission to the Dubai Health Post Office (`eClaimLink` / DHPO), validating compliance against 15th-of-month (§3.7.1) and 21-day Remittance Advice (§3.8.1) timelines, and calculating potential exposure to the statutory **0.03% per calendar day late penalty** on Net Claimed amounts (§3.7.2, §3.8.4), while monitoring statutory payer delay liabilities (§3.9.3).

4. **Cryptographic Accountability & Auditability:**  
   Every software check, clinical alert, and pharmacist override shall be logged to an append-only, tamper-evident JSONL audit record sealed with SHA-256 cryptographic signatures. Overrides require verified authorized tokens and documented clinical rationales.

---

### PROPOSED SIGNATURE BLOCKS (UNSIGNED TEMPLATE)

```text
========================================================================================
1. FOR PROSPECTIVE HEALTHCARE PARTNER (CLINICAL GOVERNANCE & MEDICAL DIRECTION)
   Name:             [NAME OF AUTHORIZED MEDICAL DIRECTOR]
   Title:            Medical Director / Head of Clinical Governance
   Entity:           [PARTNER HEALTHCARE ENTITY NAME]
   DHA License:      [DHA PROFESSIONAL LICENSE NUMBER]
   Signature Status: [UNSIGNED — PENDING FORMAL INSTITUTIONAL REVIEW]
   Date:             _______________________

2. FOR PROSPECTIVE PHARMACY OPERATIONS (DISPENSING & PHARMACY SERVICES)
   Name:             [NAME OF HEAD OF PHARMACY / PHARMACY MANAGER]
   Title:            Head of Pharmacy & Lead Clinical Pharmacist
   Facility:         [COMMUNITY PHARMACY / DISPENSARY NAME]
   DHA License:      [DHA PROFESSIONAL LICENSE NUMBER]
   Signature Status: [UNSIGNED — PENDING FORMAL INSTITUTIONAL REVIEW]
   Date:             _______________________

3. FOR TECHNOLOGY DEVELOPER (TARYAQ SOVEREIGN CLINICAL AI SYSTEMS)
   Name:             Mohamed Fathy, BPharm
   Title:            Founder & Lead Clinical AI Architect, Taryaq
   Qualification:    Licensed Clinical Pharmacist
   Signature Status: [UNSIGNED — PENDING COUNTERSIGNATURE]
   Date:             _______________________
========================================================================================
```
