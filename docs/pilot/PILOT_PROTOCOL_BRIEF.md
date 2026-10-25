# CLINICAL PILOT PROTOCOL BRIEF
## Protocol Reference: PROTOCOL-TYQ-2027-01
### Prospective Shadow Evaluation of Taryaq v0 Sovereign Clinical Decision Support & DHA Reimbursement Firewall

**Principal Investigator:** Mohamed Fathy, BPharm (Founder & Lead Clinical AI Architect, Taryaq)  
**Co-Principal Investigator:** [Prospective Clinical Site Lead Pharmacist / Pharmacy Manager]  
**Institutional Sponsor:** [Prospective Clinical Governance Chair / Medical Director]  
**Clinical Site:** [Prospective Ambulatory Clinic & Community Pharmacy — Dubai, UAE]  
**Pilot Horizon:** Proposed Q1 2027 (February 1 – March 15, 2027)  
**Regulatory Framework:** UAE Federal Law No. 2 of 2019 (Health ICT Law) & DHA SN 03/2019  
**Submission Category:** Best Societal AI Agent | Dubai AI Agents Championship 2026–2027  

---

## 1. Executive Summary & Study Synopsis

| Parameter | Clinical Protocol Specification |
| :--- | :--- |
| **Study Title** | Prospective Shadow Evaluation of Sovereign Deterministic Pharmacovigilance & Pre-Submission DHA SN 03/2019 Claim Cycle Scrubbing in Dubai Outpatient Pharmacy Practice |
| **Study Design** | 4-week prospective shadow observational evaluation preceded by a 2-week baseline IT calibration buffer. |
| **Sample Size** | $N = 500$ consecutive outpatient electronic prescriptions; $N = 500$ paired electronic claim XML payloads. |
| **Intervention Mode** | Strict **Shadow Advisory Mode (`SHADOW_ADVISORY_MODE`)**. Non-interventional; zero autonomous modification of EHR orders. |
| **Target Setting** | High-volume ambulatory polypharmacy setting: Cardiology, Nephrology, Internal Medicine, and Community Pharmacy. |
| **Primary Endpoints** | 1. Interception sensitivity for fatal/severe Adverse Drug Events (ADEs) and renal contraindications.<br>2. Real-time deterministic execution latency (< 1.0 ms for C99 rules, < 60 ms hybrid model inference). |
| **Secondary Endpoints** | 1. DHA SN 03/2019 late submission penalty exposure avoided (AED value calculated at statutory 0.03%/day).<br>2. Reduction in first-pass claim cycle resubmission rejections.<br>3. Pharmacist verification time per complex polypharmacy prescription.<br>4. Cryptographic ledger integrity and override fidelity (100% SHA-256 verifiable audit records). |

---

## 2. Clinical Background & The Dubai Healthcare Challenge

In outpatient ambulatory clinics and community pharmacies across Dubai, clinical safety and insurance administration intersect under two acute constraints:
1. **Severe Adverse Drug Events in Chronic Polypharmacy:** Patients with mild-to-moderate chronic kidney disease (CKD, eGFR < 60 mL/min) are frequently co-prescribed Colchicine and potent CYP3A4 inhibitors (e.g., Clarithromycin), producing acute neurotoxicity and fatal bone marrow suppression. Probabilistic cloud LLM assistants often fail to catch these deterministically.
2. **Statutory Claim Cycle Penalties (DHA SN 03/2019):** Healthcare providers operating on Dubai's `eClaimLink` post office are subject to strict administrative windows under Standards Notice SN 03/2019:
   * Initial claim submission: by the 15th calendar day of the subsequent month (§3.7.1).
   * Resubmission window: within 21 calendar days from Remittance Advice receipt (§3.8.1).
   * Statutory late penalty: **0.03% of Net Claimed amount per calendar day of delay** (§3.7.2, §3.8.4), plus monitoring payer reimbursement delay fees (§3.9.3).
3. **Data Sovereignty Restrictions:** **Article 13 of UAE Federal Law No. 2 of 2019** prohibits cross-border transfer or foreign cloud storage of patient health information without explicit ministerial exemption, making cloud-based foreign clinical assistants legally non-compliant for Dubai healthcare providers.

---

## 3. Trial Design & Shadow Advisory Workflow

```
       [Physician Orders Rx in Clinic EMR / PMS]
                          │
       ┌──────────────────┴──────────────────┐
       ▼                                     ▼
[Standard Dispensing Flow]         [Mirror Tap via Local AF_UNIX]
Licensed Pharmacist Dispenses                │
Standard Care Uninterrupted                  ▼
       │                          [Taryaq v0 Sovereign Engine]
       │                          - Laya 322M Intent Triage
       │                          - C99 Deterministic Safety Shield
       │                          - DHA SN 03/2019 Claim Scrubber
       │                                     │
       │                                     ▼
       │                          [Shadow Clinical Audit Record]
       │                          - Intercept Logged in taryaq_audit.jsonl
       │                          - Cryptographic SHA-256 Signature
       │                                     │
       └──────────────────┬──────────────────┘
                          ▼
        [Weekly Joint Clinical Review Committee]
        - Partner Medical Director & Clinical Pharmacist Panel
        - Adjudication of Concordance, Sensitivity, & Penalty Delta
```

1. **Mirror Ingestion:** Electronic prescriptions and billing payloads generated in the partner clinic EHR are mirrored via an isolated local Unix Domain Socket (`AF_UNIX`) to the Taryaq appliance.
2. **Shadow Evaluation:** Taryaq executes the 5-tool safety suite (`med_lookup`, `interaction_check`, `claim_scrub`, `human_override`, `commit_audit`) in under 2.5 milliseconds.
3. **Non-Disruption:** The primary pharmacy dispensing workflow proceeds without any delay or software locks.
4. **Committee Adjudication:** A weekly multidisciplinary panel (Medical Director, Lead Clinical Pharmacist, and Taryaq Lead Investigator) reviews all flagged shadow intercepts against patient outcomes to calculate true positive, false positive, and avoided penalty metrics.

---

## 4. Hardware & Zero-Leak Sovereignty Architecture

To ensure 100% compliance with UAE Federal Law No. 2 of 2019 and DESC Information Security Regulations:

* **Hardware Appliance:** Minisforum UM790 Pro Mini-PC.
  * Processor: AMD Ryzen 9 7940HS (8 cores / 16 threads, up to 5.2 GHz, 16 MB L3 cache).
  * Memory: 64 GB dual-channel DDR5-5600 MT/s (89.6 GB/s memory bandwidth).
  * Graphics: Integrated AMD Radeon 780M (RDNA3, 12 CUs).
  * Storage: 2x 1TB Samsung 990 Pro NVMe PCIe 4.0 in hardware RAID-1.
* **Network Isolation:**
  * Physical air-gap: No external WAN connection; physical Ethernet loopback to the clinic switch with no default gateway.
  * Zero telemetry: All outbound sockets blocked via Linux `iptables` and local kernel namespaces.
* **On-Device Model Stack:**
  * **Laya Multilingual (322M):** On-CPU calibrated triage engine running temperature-scaled intent routing and safety classification in under 40 ms CPU SLA (<1 ms decision head).
  * **Jais 3 (7B Q4_K_M):** Sovereign Arabic-bilingual LLM operating under strict context-free GBNF grammar constraints (`taryaq_tool_call.gbnf`), guaranteeing zero hallucinated tool calls.
  * **C99 Deterministic Safety Core:** Zero-allocation, compiled bare-metal safety rules (`taryaq_clinical_adapter.c`, `grounding_shield.c`) executing in 0.015 ms.

---

## 5. Endpoints & Statistical Analysis Plan

### 5.1 Primary Endpoints

1. **Fatal Drug-Drug Interaction Interception Sensitivity:**
   $$Sensitivity = \frac{True\ Intercepted\ ADEs}{True\ Intercepted\ ADEs + Missed\ ADEs} \times 100\%$$
   *Target:* $\ge 99.5\%$ sensitivity across renal contraindications and CYP3A4 inhibitors.
2. **Deterministic Response Latency:**
   * C99 safety shield execution latency: Target $p99 < 0.5\text{ ms}$.
   * End-to-end hybrid agent workflow: Target $p99 < 150\text{ ms}$.

### 5.2 Secondary Endpoints

1. **DHA SN 03/2019 Claim Cycle Penalty Mitigation:**
   $$Avoided\ Late\ Penalties\ (AED) = \sum_{i=1}^{N} (Net\ Claimed\ Amount_i \times 0.0003 \times \Delta t_{i,overdue\ days})$$
   Quantifies total AED financial exposure prevented by pre-submission flagging.
2. **First-Pass Resubmission Rate Reduction:**
   Target $\ge 35\%$ reduction in initial claim denials by identifying missing prior authorizations and ICD-10/CPT coding mismatches.
3. **Pharmacist Workflow Acceleration:**
   Time saved per polypharmacy encounter (minutes per order).
4. **Cryptographic Audit Integrity:**
   $100\%$ validation of SHA-256 state hashes in `taryaq_audit.jsonl` with zero undetected tampering attempts.

---

## 6. Ethical Oversight & Data Privacy Governance

* **IRB Exemption / Quality Improvement Determination:** Because this evaluation operates exclusively in a non-interventional shadow advisory mode on mirrored data without altering clinical treatment, it is classified as a Clinical Quality Improvement (CQI) and Health ICT Validation Initiative under DHA Clinical Governance guidelines.
* **Data De-Identification:** Patient names, Emirates IDs, and mobile numbers are hashed at the ingress socket using salted SHA-256 before analysis.
* **Anonymised Dissemination:** All study results prepared for the Dubai AI Agents Championship 2026–2027 will report solely aggregated statistical summaries with zero individually identifiable patient data.

---

## 7. Protocol Governance & Proposed Investigation Team

```text
Study Team Roster (Unsigned Evaluation Protocol):
1. [Name of Medical Director]
   Medical Director & Clinical Governance Chair [Prospective Partner Healthcare Group]
2. [Name of Head of Pharmacy]
   Head of Pharmacy & Clinical Pharmacy Specialist [Prospective Partner Community Pharmacy]
3. Mohamed Fathy, BPharm
   Founder & Lead Clinical AI Architect, Taryaq Sovereign Clinical AI Systems
```
