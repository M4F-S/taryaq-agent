/* taryaq_clinical_adapter.c
 * Hardened C99 Implementation of:
 *   1. Deterministic Pharmacovigilance & Renal/CYP3A4 Contraindication Blocker (Commutative)
 *   2. UAE Controlled Medication Regulatory Gatekeeper (Law No. 2 of 2019 / Law No. 30 of 2021)
 *   3. DHA eClaimLink & Ejadah Pre-Submission XML Claim Scrubber (SN 03/2019)
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <ctype.h>

#include "taryaq_adapters.h"

/* Case-insensitive substring search for C99 portability */
static bool strcasestr_c99(const char *haystack, const char *needle) {
    if (!haystack || !needle || needle[0] == '\0') return false;
    size_t n_len = strlen(needle);
    for (size_t i = 0; haystack[i] != '\0'; ++i) {
        size_t j = 0;
        while (needle[j] != '\0' && haystack[i + j] != '\0' &&
               tolower((unsigned char)haystack[i + j]) == tolower((unsigned char)needle[j])) {
            j++;
        }
        if (j == n_len) return true;
    }
    return false;
}

/* Helper to check drug presence across primary and concurrent slots */
static bool drug_matches(const char *primary, const char *concurrent, const char *target) {
    if (primary && strcasestr_c99(primary, target)) return true;
    if (concurrent && strcasestr_c99(concurrent, target)) return true;
    return false;
}

/* Auto-detect controlled / narcotic substances under UAE Law No. 30 of 2021 */
static bool is_controlled_drug_name(const char *name) {
    if (!name) return false;
    static const char *CONTROLLED_SCHEDULE[] = {
        "morphine", "tramadol", "fentanyl", "oxycodone", "pethidine",
        "methadone", "buprenorphine", "diazepam", "lorazepam", "alprazolam",
        "مورفين", "ترامادول", "فنتانيل", "اوكسيكودون", NULL
    };
    for (int i = 0; CONTROLLED_SCHEDULE[i] != NULL; ++i) {
        if (strcasestr_c99(name, CONTROLLED_SCHEDULE[i])) return true;
    }
    return false;
}

taryaq_result_t taryaq_verify_prescription_and_claim(const taryaq_rx_context_t *rx) {
    taryaq_result_t res;
    memset(&res, 0, sizeof(res));

    /* F-01 Fix: Fail-closed initialization before input verification */
    res.verdict = CLINICAL_CONTRAINDICATION_BLOCK;
    snprintf(res.alert_code, sizeof(res.alert_code), "%s", "INVALID_RX_CONTEXT");
    snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s", "Blocked: Prescription context is NULL or primary drug is missing.");

    if (!rx || !rx->primary_drug || rx->primary_drug[0] == '\0') {
        return res;
    }

    /* F-02 Fix: IEEE-754 NaN and physiological eGFR bounds validation */
    if (!isfinite(rx->patient_egfr_ml_min) || rx->patient_egfr_ml_min <= 0.0 || rx->patient_egfr_ml_min > 250.0) {
        res.verdict = CLINICAL_CONTRAINDICATION_BLOCK;
        snprintf(res.alert_code, sizeof(res.alert_code), "%s", "INVALID_RENAL_LAB_VALUE");
        snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s", "Blocked: Patient eGFR must be a finite physiological value between 0.1 and 250.0 mL/min.");
        return res;
    }

    /* F-03 Fix: Commutative, case-insensitive, brand-aware drug detection */
    bool has_colchicine = drug_matches(rx->primary_drug, rx->concurrent_drug, "colchicine") ||
                          drug_matches(rx->primary_drug, rx->concurrent_drug, "كولشيسين");

    bool has_clarithro  = drug_matches(rx->primary_drug, rx->concurrent_drug, "clarithromycin") ||
                          drug_matches(rx->primary_drug, rx->concurrent_drug, "كلاريثروميسين") ||
                          drug_matches(rx->primary_drug, rx->concurrent_drug, "klacid") ||
                          drug_matches(rx->primary_drug, rx->concurrent_drug, "biaxin");

    bool has_metformin  = drug_matches(rx->primary_drug, rx->concurrent_drug, "metformin") ||
                          drug_matches(rx->primary_drug, rx->concurrent_drug, "ميتفورمين") ||
                          drug_matches(rx->primary_drug, rx->concurrent_drug, "glucophage");

    /* 1a. Fatal CYP3A4 + Renal Contraindication Block (Colchicine + Clarithromycin in eGFR < 60) */
    if (has_colchicine && has_clarithro && rx->patient_egfr_ml_min < 60.0) {
        res.verdict = CLINICAL_CONTRAINDICATION_BLOCK;
        snprintf(res.alert_code, sizeof(res.alert_code), "%s", "FATAL_CYP3A4_RENAL_CONTRAINDICATION");
        snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s", "Blocked: Concurrent Colchicine + Clarithromycin in renal impairment (eGFR < 60) causes fatal toxicity due to CYP3A4/P-gp inhibition.");
        return res;
    }

    /* 1b. Metformin Severe Renal Cutoff (eGFR < 30 mL/min -> Lactic Acidosis Block) */
    if (has_metformin && rx->patient_egfr_ml_min < 30.0) {
        res.verdict = CLINICAL_CONTRAINDICATION_BLOCK;
        snprintf(res.alert_code, sizeof(res.alert_code), "%s", "METFORMIN_SEVERE_RENAL_CUTOFF_EGFR_LT_30");
        snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s", "Blocked: Metformin is contraindicated in severe renal impairment (eGFR < 30 mL/min) due to fatal lactic acidosis risk.");
        return res;
    }

    /* 1c. Metformin CKD Stage 3b Review Gate (30.0 <= eGFR < 45.0 mL/min) */
    if (has_metformin && rx->patient_egfr_ml_min >= 30.0 && rx->patient_egfr_ml_min < 45.0) {
        res.verdict = CLINICAL_CLAIM_CYCLE_WARN;
        snprintf(res.alert_code, sizeof(res.alert_code), "%s", "METFORMIN_CKD_STAGE_3B_DOSE_REDUCTION_REQUIRED");
        snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s", "Clinical Review: Metformin in CKD Stage 3b requires dose reduction (max 1000 mg/day) and renal function monitoring every 3 months.");
        return res;
    }

    /* 2. Controlled Medication Gate (UAE Health Law No. 2 of 2019 / Law No. 30 of 2021) */
    bool is_controlled = rx->is_controlled_substance ||
                         is_controlled_drug_name(rx->primary_drug) ||
                         is_controlled_drug_name(rx->concurrent_drug);

    if (is_controlled && !rx->has_controlled_id_verified) {
        res.verdict = CLINICAL_CONTROLLED_DRUG_GATE;
        snprintf(res.alert_code, sizeof(res.alert_code), "%s", "UAE_CONTROLLED_RX_VERIFICATION_REQUIRED");
        snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s",
                 "Hold: Controlled medication requires verified Emirates ID, dosage ceiling verification, and pharmacist co-signature.");
        return res;
    }

    /* 3. DHA eClaimLink & Ejadah Scrubber (SN 03/2019 Statutory Late Penalty) */
    if (rx->submission_delay_days > 0) {
        res.verdict = CLINICAL_CLAIM_CYCLE_WARN;
        snprintf(res.alert_code, sizeof(res.alert_code), "%s", "DHA_SN03_2019_DELAY_PENALTY_EXPOSURE");
        snprintf(res.clinical_rationale, sizeof(res.clinical_rationale),
                 "Claim Cycle Warning: Submission is %u day(s) overdue past statutory 30-day window under DHA SN 03/2019; subject to 0.03%%/day delay penalty (%.4f%%).",
                 rx->submission_delay_days, (double)rx->submission_delay_days * 0.03);
        return res;
    }

    /* F-05 Fix: Ejadah Lab Justification Check for Chronic Diseases */
    if (rx->icd10_code && (strcasestr_c99(rx->icd10_code, "E11") || strcasestr_c99(rx->icd10_code, "E10"))) {
        if (!rx->has_ejadah_lab_justification) {
            res.verdict = CLINICAL_CLAIM_CYCLE_WARN;
            snprintf(res.alert_code, sizeof(res.alert_code), "%s", "EJADAH_LAB_JUSTIFICATION_MISSING");
            snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s", "Ejadah Protocol Warning: Diabetes claim requires attached HbA1c lab result justification under DHA KPI benchmarks.");
            return res;
        }
    }

    /* All checks passed */
    res.verdict = CLINICAL_SAFE;
    snprintf(res.alert_code, sizeof(res.alert_code), "%s", "DHA_NABIDH_SAFE");
    snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s", "Pharmacovigilance and DHA Ejadah claim checks passed.");
    return res;
}

taryaq_verdict_t taryaq_verify_prescription_and_claim_ptr(
    const taryaq_rx_context_t *rx,
    taryaq_result_t           *out_result
) {
    if (!out_result) return CLINICAL_CONTRAINDICATION_BLOCK;
    *out_result = taryaq_verify_prescription_and_claim(rx);
    return out_result->verdict;
}

#ifdef TARYAQ_STANDALONE_TEST
#include <assert.h>

int main(void) {
    /* Test 1: Fatal CYP3A4 interaction in renal failure -> CONTRAINDICATION_BLOCK */
    taryaq_rx_context_t fatal_case = {
        .primary_drug = "Colchicine",
        .concurrent_drug = "Clarithromycin",
        .patient_egfr_ml_min = 22.0,
        .is_controlled_substance = false,
        .has_controlled_id_verified = false,
        .icd10_code = "M10.0",
        .submission_delay_days = 0,
        .has_ejadah_lab_justification = true
    };
    taryaq_result_t r1 = taryaq_verify_prescription_and_claim(&fatal_case);
    assert(r1.verdict == CLINICAL_CONTRAINDICATION_BLOCK);
    assert(strcmp(r1.alert_code, "FATAL_CYP3A4_RENAL_CONTRAINDICATION") == 0);

    /* Test 2: Controlled substance without verified ID -> CONTROLLED_DRUG_GATE */
    taryaq_rx_context_t narcotic_case = {
        .primary_drug = "Morphine",
        .concurrent_drug = NULL,
        .patient_egfr_ml_min = 90.0,
        .is_controlled_substance = true,
        .has_controlled_id_verified = false,
        .icd10_code = "R52",
        .submission_delay_days = 0,
        .has_ejadah_lab_justification = true
    };
    taryaq_result_t r2 = taryaq_verify_prescription_and_claim(&narcotic_case);
    assert(r2.verdict == CLINICAL_CONTROLLED_DRUG_GATE);
    assert(strcmp(r2.alert_code, "UAE_CONTROLLED_RX_VERIFICATION_REQUIRED") == 0);

    /* Test 3: DHA SN 03/2019 Claim Submission delayed past deadline -> CLAIM_CYCLE_WARN */
    taryaq_rx_context_t delayed_claim = {
        .primary_drug = "Metformin",
        .concurrent_drug = NULL,
        .patient_egfr_ml_min = 75.0,
        .is_controlled_substance = false,
        .has_controlled_id_verified = false,
        .icd10_code = "E11.9",
        .submission_delay_days = 12,
        .has_ejadah_lab_justification = true
    };
    taryaq_result_t r3 = taryaq_verify_prescription_and_claim(&delayed_claim);
    assert(r3.verdict == CLINICAL_CLAIM_CYCLE_WARN);
    assert(strcmp(r3.alert_code, "DHA_SN03_2019_DELAY_PENALTY_EXPOSURE") == 0);

    /* Test 4: Fully compliant claim -> SAFE */
    delayed_claim.submission_delay_days = 0;
    taryaq_result_t r4 = taryaq_verify_prescription_and_claim(&delayed_claim);
    assert(r4.verdict == CLINICAL_SAFE);
    assert(strcmp(r4.alert_code, "DHA_NABIDH_SAFE") == 0);

    /* Test 5: Metformin in severe renal impairment (eGFR 26 < 30) -> CONTRAINDICATION_BLOCK */
    taryaq_rx_context_t metformin_renal = {
        .primary_drug = "Metformin",
        .concurrent_drug = NULL,
        .patient_egfr_ml_min = 26.0,
        .is_controlled_substance = false,
        .has_controlled_id_verified = false,
        .icd10_code = "E11.22",
        .submission_delay_days = 0,
        .has_ejadah_lab_justification = true
    };
    taryaq_result_t r5 = taryaq_verify_prescription_and_claim(&metformin_renal);
    assert(r5.verdict == CLINICAL_CONTRAINDICATION_BLOCK);
    assert(strcmp(r5.alert_code, "METFORMIN_SEVERE_RENAL_CUTOFF_EGFR_LT_30") == 0);

    /* Test 6: Commutative Check (Inverted primary/concurrent drug order + lowercase) */
    taryaq_rx_context_t inverted_order_case = {
        .primary_drug = "clarithromycin",
        .concurrent_drug = "colchicine 0.5mg",
        .patient_egfr_ml_min = 22.0,
        .is_controlled_substance = false,
        .has_controlled_id_verified = false,
        .icd10_code = "M10.0",
        .submission_delay_days = 0,
        .has_ejadah_lab_justification = true
    };
    taryaq_result_t r6 = taryaq_verify_prescription_and_claim(&inverted_order_case);
    assert(r6.verdict == CLINICAL_CONTRAINDICATION_BLOCK);
    assert(strcmp(r6.alert_code, "FATAL_CYP3A4_RENAL_CONTRAINDICATION") == 0);

    /* Test 7 (F-01 check): NULL Context Fails Closed */
    taryaq_result_t r7 = taryaq_verify_prescription_and_claim(NULL);
    assert(r7.verdict == CLINICAL_CONTRAINDICATION_BLOCK);
    assert(strcmp(r7.alert_code, "INVALID_RX_CONTEXT") == 0);

    /* Test 8 (F-02 check): NaN eGFR Fails Closed */
    taryaq_rx_context_t nan_case = {
        .primary_drug = "Paracetamol",
        .patient_egfr_ml_min = NAN,
    };
    taryaq_result_t r8 = taryaq_verify_prescription_and_claim(&nan_case);
    assert(r8.verdict == CLINICAL_CONTRAINDICATION_BLOCK);
    assert(strcmp(r8.alert_code, "INVALID_RENAL_LAB_VALUE") == 0);

    /* Test 9 (F-04 check): Auto-detect narcotic Tramadol without caller flag */
    taryaq_rx_context_t auto_narcotic = {
        .primary_drug = "tramadol 50mg",
        .patient_egfr_ml_min = 90.0,
        .is_controlled_substance = false, /* Caller forgot flag! */
        .has_controlled_id_verified = false
    };
    taryaq_result_t r9 = taryaq_verify_prescription_and_claim(&auto_narcotic);
    assert(r9.verdict == CLINICAL_CONTROLLED_DRUG_GATE);
    assert(strcmp(r9.alert_code, "UAE_CONTROLLED_RX_VERIFICATION_REQUIRED") == 0);

    printf("[PASS] All Taryaq C99 Pharmacovigilance & DHA SN 03/2019 Claim Scrubber tests passed successfully!\n");
    return 0;
}
#endif
