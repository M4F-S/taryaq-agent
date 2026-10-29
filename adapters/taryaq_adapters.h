/* taryaq_adapters.h - Deterministic Pharmacovigilance & DHA Claim Scrubber for Taryaq
 * Target: Best Societal AI Agent | Dubai Create AI Agents Championship 2026-2027
 */
#ifndef TARYAQ_ADAPTERS_H
#define TARYAQ_ADAPTERS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CLINICAL_SAFE                   = 0, /* Prescription and billing claim pass checks */
    CLINICAL_CLAIM_CYCLE_WARN       = 1, /* DHA SN 03/2019 submission deadline exceeded (0.03%/day) */
    CLINICAL_CONTROLLED_DRUG_GATE   = 2, /* Controlled substance requires pharmacist sign-off */
    CLINICAL_CONTRAINDICATION_BLOCK = 3, /* Fatal drug-drug or renal contraindication blocked */
    CLINICAL_UNVERIFIED_REVIEW      = 4  /* Unverified drug or combination requires pharmacist review */
} taryaq_verdict_t;

/* Backward compatibility & statutory aliases */
#define CLINICAL_EJADAH_SCRUB_WARN CLINICAL_CLAIM_CYCLE_WARN
#define UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW CLINICAL_UNVERIFIED_REVIEW

typedef struct {
    const char *primary_drug;
    const char *concurrent_drug;
    double      patient_egfr_ml_min;
    bool        is_controlled_substance;
    bool        has_controlled_id_verified;
    const char *icd10_code;
    uint32_t    submission_delay_days;       /* Days elapsed past DHA SN 03/2019 window */
    bool        has_ejadah_lab_justification; /* Retained for ABI compatibility */
    bool        has_clinician_override;       /* Single-use human override verified */
} taryaq_rx_context_t;

typedef struct {
    taryaq_verdict_t verdict;
    char             alert_code[64];
    char             clinical_rationale[256];
    char             doc_id[64];             /* Statutory or clinical provenance document */
    char             section[64];            /* Specific section/clause (e.g. §3.7.2) */
    char             passage_sha256[65];     /* Cryptographic SHA-256 hash of authoritative clause */
} taryaq_result_t;

/* Patient Care Companion: Emergency Red-Flag Triage Engine */
typedef enum {
    TRIAGE_ROUTINE   = 0, /* Non-urgent inquiry or refill */
    TRIAGE_URGENT    = 1, /* Urgent evaluation recommended within 24-48 hours */
    TRIAGE_EMERGENCY = 2  /* IMMEDIATE RED-FLAG INTERRUPT: Emergency transfer (998/999) */
} taryaq_triage_level_t;

typedef struct {
    taryaq_triage_level_t level;
    bool                  is_red_flag;
    char                  triage_category[64];   /* e.g., CARDIOVASCULAR_ACUTE, STROKE_NEUROLOGICAL */
    char                  matched_trigger[64];   /* Trigger phrase matched */
    char                  action_protocol[256];  /* Emergency protocol instructions */
    char                  clinical_guidance[256];/* Clinical rationale & handover notes */
} taryaq_triage_result_t;

/* Function Prototypes */
taryaq_result_t taryaq_verify_prescription_and_claim(const taryaq_rx_context_t *rx);

/* F-15 Fix: Pointer-based ABI export to prevent large struct return-by-value across compiler/FFI boundaries */
taryaq_verdict_t taryaq_verify_prescription_and_claim_ptr(const taryaq_rx_context_t *rx, taryaq_result_t *out_result);

/* Patient Emergency Red-Flag Triage Function */
taryaq_triage_result_t taryaq_triage_patient_symptoms(const char *symptom_narrative);

/* ========================================================================= */
/* Sovereign Clinical Formulary Database Management                          */
/* ========================================================================= */
bool taryaq_clinical_db_init(const char *db_path);
void taryaq_clinical_db_close(void);
int  taryaq_clinical_db_count_drugs(void);

/* ========================================================================= */
/* Tri-Modal Interface 1: Clinician Ambient Copilot                          */
/* ========================================================================= */
typedef struct {
    char subjective[1024];
    char objective[1024];
    char assessment[1024];
    char plan[1024];
    char ranked_ddx_json[512]; /* JSON array of ranked DDx objects with ICD-10 */
    bool cds_interrupt_triggered;
    char cds_interrupt_code[64];
    char cds_interrupt_rationale[256];
    char evidence_doc_id[64];
    char evidence_sha256[65];
} taryaq_soap_note_t;

bool taryaq_clinician_ambient_copilot(
    const char *dialogue_transcript,
    double patient_egfr,
    const char *proposed_drugs_csv,
    taryaq_soap_note_t *out_soap
);

/* ========================================================================= */
/* Tri-Modal Interface 2: Pharmacist Verification Console                   */
/* ========================================================================= */
typedef struct {
    size_t total_drugs_screened;
    size_t interaction_count;
    bool has_contraindication_block;
    char primary_block_code[64];
    char primary_block_rationale[256];
    bool is_controlled_gate;
    char controlled_drug_name[64];
    double net_claim_aed;
    uint32_t delay_days;
    uint32_t overdue_days;
    double penalty_rate_pct;
    double penalty_aed;
    double adjusted_net_aed;
    bool claim_cycle_warn;
    char claim_alert_code[64];
    char evidence_doc_id[64];
    char evidence_section[64];
    char evidence_sha256[65];
    char polypharmacy_matrix_json[2048];
} taryaq_pharmacist_result_t;

bool taryaq_pharmacist_verification(
    const char *medications_csv,
    double patient_egfr,
    double net_claim_aed,
    uint32_t submission_delay_days,
    const char *icd10_code,
    bool id_verified,
    taryaq_pharmacist_result_t *out_result
);

/* ========================================================================= */
/* Tri-Modal Interface 3: Patient Health Companion                          */
/* ========================================================================= */
typedef struct {
    taryaq_triage_result_t triage;
    char lay_instructions[1024];
    char dietary_precautions[512];
    char red_flag_warning[1024];
    bool requires_immediate_998;
} taryaq_patient_companion_result_t;

bool taryaq_patient_companion_consult(
    const char *patient_message,
    const char *active_meds_csv,
    double patient_egfr,
    taryaq_patient_companion_result_t *out_result
);

#ifdef __cplusplus
}
#endif

#endif /* TARYAQ_ADAPTERS_H */
