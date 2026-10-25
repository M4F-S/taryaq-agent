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

#ifdef __cplusplus
}
#endif

#endif /* TARYAQ_ADAPTERS_H */
