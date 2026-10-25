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
        "ketamine", "مورفين", "ترامادول", "فنتانيل", "اوكسيكودون", "كيتامين", NULL
    };
    for (int i = 0; CONTROLLED_SCHEDULE[i] != NULL; ++i) {
        if (strcasestr_c99(name, CONTROLLED_SCHEDULE[i])) return true;
    }
    return false;
}

/* Fail-Closed Gate: Recognize modeled medications in sovereign clinical graph */
static bool is_known_modeled_drug(const char *name) {
    if (!name || name[0] == '\0') return false;
    static const char *KNOWN_DRUGS[] = {
        "colchicine", "كولشيسين",
        "clarithromycin", "كلاريثروميسين", "klacid", "biaxin",
        "metformin", "ميتفورمين", "glucophage",
        "simvastatin", "سيمفاستاتين", "zocor",
        "warfarin", "وارفارين", "coumadin",
        "amiodarone", "أميودارون", "اميودارون", "cordarone",
        "paracetamol", "باراسيتامول", "panadol", "acetaminophen",
        "amoxicillin", "أموكسيسيلين", "اموكسيسيلين", "augmentin",
        "morphine", "مورفين",
        "tramadol", "ترامادول",
        "ketamine", "كيتامين",
        "fentanyl", "فنتانيل",
        "oxycodone", "اوكسيكودون",
        "pethidine",
        "methadone",
        "buprenorphine",
        "diazepam",
        "lorazepam",
        "alprazolam",
        NULL
    };
    for (int i = 0; KNOWN_DRUGS[i] != NULL; ++i) {
        if (strcasestr_c99(name, KNOWN_DRUGS[i])) return true;
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

    bool has_simvastatin = drug_matches(rx->primary_drug, rx->concurrent_drug, "simvastatin") ||
                           drug_matches(rx->primary_drug, rx->concurrent_drug, "سيمفاستاتين") ||
                           drug_matches(rx->primary_drug, rx->concurrent_drug, "zocor");

    bool has_warfarin    = drug_matches(rx->primary_drug, rx->concurrent_drug, "warfarin") ||
                           drug_matches(rx->primary_drug, rx->concurrent_drug, "وارفارين") ||
                           drug_matches(rx->primary_drug, rx->concurrent_drug, "coumadin");

    bool has_amiodarone  = drug_matches(rx->primary_drug, rx->concurrent_drug, "amiodarone") ||
                           drug_matches(rx->primary_drug, rx->concurrent_drug, "أميودارون") ||
                           drug_matches(rx->primary_drug, rx->concurrent_drug, "اميودارون") ||
                           drug_matches(rx->primary_drug, rx->concurrent_drug, "cordarone");

    bool has_paracetamol = drug_matches(rx->primary_drug, rx->concurrent_drug, "paracetamol") ||
                           drug_matches(rx->primary_drug, rx->concurrent_drug, "باراسيتامول") ||
                           drug_matches(rx->primary_drug, rx->concurrent_drug, "panadol") ||
                           drug_matches(rx->primary_drug, rx->concurrent_drug, "acetaminophen");

    bool has_amoxicillin = drug_matches(rx->primary_drug, rx->concurrent_drug, "amoxicillin") ||
                           drug_matches(rx->primary_drug, rx->concurrent_drug, "أموكسيسيلين") ||
                           drug_matches(rx->primary_drug, rx->concurrent_drug, "اموكسيسيلين") ||
                           drug_matches(rx->primary_drug, rx->concurrent_drug, "augmentin");

    bool has_concurrent = (rx->concurrent_drug != NULL && rx->concurrent_drug[0] != '\0' &&
                           strcmp(rx->concurrent_drug, "none") != 0 && strcmp(rx->concurrent_drug, "-") != 0);

    /* 1. Fatal Drug-Drug & Renal Contraindication Blocks (Bypassed ONLY if single-use human override verified) */
    if (!rx->has_clinician_override) {
        /* 1a. Fatal CYP3A4 + Renal Contraindication Block (Colchicine + Clarithromycin in eGFR < 60) */
        if (has_colchicine && has_clarithro && rx->patient_egfr_ml_min < 60.0) {
            res.verdict = CLINICAL_CONTRAINDICATION_BLOCK;
            snprintf(res.alert_code, sizeof(res.alert_code), "%s", "FATAL_CYP3A4_RENAL_CONTRAINDICATION");
            snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s",
                     "Blocked: Concurrent Colchicine + Clarithromycin in renal impairment (eGFR < 60) causes fatal toxicity due to CYP3A4/P-gp inhibition.");
            return res;
        }

        /* 1b. Fatal Simvastatin + Clarithromycin Rhabdomyolysis Contraindication */
        if (has_simvastatin && has_clarithro) {
            res.verdict = CLINICAL_CONTRAINDICATION_BLOCK;
            snprintf(res.alert_code, sizeof(res.alert_code), "%s", "FATAL_CYP3A4_RHABDOMYOLYSIS_CONTRAINDICATION");
            snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s",
                     "Blocked: Concurrent Simvastatin and Clarithromycin (potent CYP3A4 inhibitor) is contraindicated due to fatal rhabdomyolysis and acute kidney injury risk (FDA Boxed Warning).");
            return res;
        }

        /* 1c. Fatal Warfarin + Amiodarone Potentiation & Hemorrhage Risk */
        if (has_warfarin && has_amiodarone) {
            res.verdict = CLINICAL_CONTRAINDICATION_BLOCK;
            snprintf(res.alert_code, sizeof(res.alert_code), "%s", "WARFARIN_AMIODARONE_HEMORRHAGE_RISK");
            snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s",
                     "Blocked: Concurrent Warfarin + Amiodarone inhibits CYP2C9/3A4 causing extreme INR elevation and fatal hemorrhage risk; requires documented dose reduction and INR protocol.");
            return res;
        }

        /* 1d. Metformin Severe Renal Cutoff (eGFR < 30 mL/min -> Fatal Lactic Acidosis Block) */
        if (has_metformin && rx->patient_egfr_ml_min < 30.0) {
            res.verdict = CLINICAL_CONTRAINDICATION_BLOCK;
            snprintf(res.alert_code, sizeof(res.alert_code), "%s", "METFORMIN_SEVERE_RENAL_CUTOFF_EGFR_LT_30");
            snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s",
                     "Blocked: Metformin is contraindicated in severe renal impairment (eGFR < 30 mL/min) due to fatal lactic acidosis risk.");
            return res;
        }

        /* 1e. Metformin CKD Stage 3b Review Gate (30.0 <= eGFR < 45.0 mL/min) */
        if (has_metformin && rx->patient_egfr_ml_min >= 30.0 && rx->patient_egfr_ml_min < 45.0) {
            res.verdict = CLINICAL_CLAIM_CYCLE_WARN;
            snprintf(res.alert_code, sizeof(res.alert_code), "%s", "METFORMIN_CKD_STAGE_3B_DOSE_REDUCTION_REQUIRED");
            snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s",
                     "Clinical Review: Metformin in CKD Stage 3b requires dose reduction (max 1000 mg/day) and renal function monitoring every 3 months.");
            return res;
        }

        /* 1f. Fail-Closed Safety Gate: Unmodeled drug or unverified multi-drug combination */
        if (!is_known_modeled_drug(rx->primary_drug) || (has_concurrent && !is_known_modeled_drug(rx->concurrent_drug))) {
            res.verdict = CLINICAL_UNVERIFIED_REVIEW;
            snprintf(res.alert_code, sizeof(res.alert_code), "%s", "UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW");
            snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s",
                     "Fail-Closed Gate: Medication is not modeled in the verified sovereign clinical database; requires clinical pharmacist review.");
            return res;
        }

        /* If concurrent drug exists, verify whether this specific combination has explicit clinical modeling */
        if (has_concurrent) {
            bool is_explicitly_modeled_pair =
                (has_colchicine && has_clarithro) ||
                (has_simvastatin && has_clarithro) ||
                (has_warfarin && has_amiodarone) ||
                (has_metformin && has_simvastatin) ||
                (has_paracetamol && has_amoxicillin); /* Verified safe analgesic + antibiotic co-prescription */

            if (!is_explicitly_modeled_pair) {
                res.verdict = CLINICAL_UNVERIFIED_REVIEW;
                snprintf(res.alert_code, sizeof(res.alert_code), "%s", "UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW");
                snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s",
                         "Fail-Closed Gate: Drug combination is not explicitly verified in the sovereign interaction matrix; requires clinical pharmacist review.");
                return res;
            }
        }
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

/* ========================================================================= */
/* Patient Care Companion: C99 Emergency Red-Flag Triage Engine              */
/* ========================================================================= */

taryaq_triage_result_t taryaq_triage_patient_symptoms(const char *symptom_narrative) {
    taryaq_triage_result_t res;
    memset(&res, 0, sizeof(res));

    res.level = TRIAGE_ROUTINE;
    res.is_red_flag = false;
    snprintf(res.triage_category, sizeof(res.triage_category), "%s", "ROUTINE_CARE");
    snprintf(res.action_protocol, sizeof(res.action_protocol), "%s", "Routine outpatient consultation or medication counseling.");
    snprintf(res.clinical_guidance, sizeof(res.clinical_guidance), "%s", "No acute red-flag symptoms detected. Continue standard clinical care.");

    if (!symptom_narrative || symptom_narrative[0] == '\0') {
        return res;
    }

    /* 1. Compound Dialectal Arabic & Clinical Rules */
    bool has_ar_chest = strcasestr_c99(symptom_narrative, "صدر");
    bool has_ar_pain  = strcasestr_c99(symptom_narrative, "ألم") || strcasestr_c99(symptom_narrative, "الم") ||
                        strcasestr_c99(symptom_narrative, "وجع") || strcasestr_c99(symptom_narrative, "ضغط") ||
                        strcasestr_c99(symptom_narrative, "عوار") || strcasestr_c99(symptom_narrative, "نغز") ||
                        strcasestr_c99(symptom_narrative, "حرق");
    if (has_ar_chest && has_ar_pain) {
        res.level = TRIAGE_EMERGENCY;
        res.is_red_flag = true;
        snprintf(res.triage_category, sizeof(res.triage_category), "%s", "CARDIOVASCULAR_ACUTE");
        snprintf(res.matched_trigger, sizeof(res.matched_trigger), "%s", "ألم / ضغط في الصدر");
        snprintf(res.action_protocol, sizeof(res.action_protocol), "%s",
                 "بروتوكول طوارئ فوري: نقل عاجل إلى قسم الطوارئ / الاتصال بالإسعاف 998 أو 999 فوراً. تجنب القيادة بنفسك.");
        snprintf(res.clinical_guidance, sizeof(res.clinical_guidance), "%s",
                 "اشتباه متلازمة الشريان التاجي الحادة أو جلطة قلبية. يتطلب تخطيط قلب فوري وتقييم إسعافي.");
        return res;
    }

    bool has_ar_dyspnea = strcasestr_c99(symptom_narrative, "تنفس") || strcasestr_c99(symptom_narrative, "اتنفس") ||
                          strcasestr_c99(symptom_narrative, "أتنفس");
    bool has_ar_resp_distress = strcasestr_c99(symptom_narrative, "ضيق") || strcasestr_c99(symptom_narrative, "صعوب") ||
                                strcasestr_c99(symptom_narrative, "مش قادر") || strcasestr_c99(symptom_narrative, "خنقة");
    if (has_ar_dyspnea && has_ar_resp_distress) {
        res.level = TRIAGE_EMERGENCY;
        res.is_red_flag = true;
        snprintf(res.triage_category, sizeof(res.triage_category), "%s", "RESPIRATORY_DISTRESS");
        snprintf(res.matched_trigger, sizeof(res.matched_trigger), "%s", "ضيق / صعوبة تنفس حادة");
        snprintf(res.action_protocol, sizeof(res.action_protocol), "%s",
                 "بروتوكول طوارئ فوري: صعوبة تنفس حادة. الاتصال بالإسعاف 998 أو 999 فوراً.");
        snprintf(res.clinical_guidance, sizeof(res.clinical_guidance), "%s",
                 "ضائقة تنفسية حادة مهددة للحياة تستدعي الأكسجة والتدخل الطبي الإسعافي.");
        return res;
    }

    bool has_ar_stroke_sign = strcasestr_c99(symptom_narrative, "شلل") || strcasestr_c99(symptom_narrative, "تنميل") ||
                              strcasestr_c99(symptom_narrative, "اعوجاج") || strcasestr_c99(symptom_narrative, "عوجاج") ||
                              strcasestr_c99(symptom_narrative, "ثقل");
    bool has_ar_stroke_loc  = strcasestr_c99(symptom_narrative, "فم") || strcasestr_c99(symptom_narrative, "وجه") ||
                              strcasestr_c99(symptom_narrative, "لسان") || strcasestr_c99(symptom_narrative, "نصف") ||
                              strcasestr_c99(symptom_narrative, "يد") || strcasestr_c99(symptom_narrative, "ذراع");
    if (has_ar_stroke_sign && has_ar_stroke_loc) {
        res.level = TRIAGE_EMERGENCY;
        res.is_red_flag = true;
        snprintf(res.triage_category, sizeof(res.triage_category), "%s", "STROKE_NEUROLOGICAL");
        snprintf(res.matched_trigger, sizeof(res.matched_trigger), "%s", "أعراض سكتة عصبية حادة (FAST)");
        snprintf(res.action_protocol, sizeof(res.action_protocol), "%s",
                 "بروتوكول طوارئ فوري: اشتباه سكتة دماغية حادة. الاتصال بالإسعاف 998 أو 999 فوراً.");
        snprintf(res.clinical_guidance, sizeof(res.clinical_guidance), "%s",
                 "علامة عصبية حادة للسكتة الدماغية. الالتزام بالنافذة الزمنية الحرجة للإذابة الخثرية.");
        return res;
    }

    /* 2. Structured Red Flag Trigger Table (English and Arabic Keywords) */
    typedef struct {
        const char            *trigger;
        taryaq_triage_level_t  level;
        const char            *category;
        const char            *protocol;
        const char            *guidance;
    } red_flag_entry_t;

    static const red_flag_entry_t RED_FLAGS[] = {
        /* Cardiovascular / Acute Coronary Syndrome */
        { "chest pain", TRIAGE_EMERGENCY, "CARDIOVASCULAR_ACUTE",
          "EMERGENCY PROTOCOL INTERRUPT: Immediate transfer to Emergency Department / Call 998 or 999. Do not drive.",
          "Possible Acute Coronary Syndrome (ACS) or myocardial ischemia. Immediate 12-lead ECG and troponin indicated." },
        { "chest pressure", TRIAGE_EMERGENCY, "CARDIOVASCULAR_ACUTE",
          "EMERGENCY PROTOCOL INTERRUPT: Immediate transfer to Emergency Department / Call 998 or 999. Do not drive.",
          "Possible Acute Coronary Syndrome (ACS) or myocardial ischemia. Immediate 12-lead ECG and troponin indicated." },
        { "heart attack", TRIAGE_EMERGENCY, "CARDIOVASCULAR_ACUTE",
          "EMERGENCY PROTOCOL INTERRUPT: Immediate transfer to Emergency Department / Call 998 or 999.",
          "Suspected myocardial infarction. Emergency medical services dispatch required." },
        { "crushing pain", TRIAGE_EMERGENCY, "CARDIOVASCULAR_ACUTE",
          "EMERGENCY PROTOCOL INTERRUPT: Immediate transfer to Emergency Department / Call 998 or 999.",
          "Severe ischemic cardiac chest pain presentation. Immediate emergency dispatch." },
        { "radiating to arm", TRIAGE_EMERGENCY, "CARDIOVASCULAR_ACUTE",
          "EMERGENCY PROTOCOL INTERRUPT: Immediate transfer to Emergency Department / Call 998 or 999.",
          "Typical radiation of cardiac ischemic pain. Immediate emergency dispatch." },
        { "radiating to jaw", TRIAGE_EMERGENCY, "CARDIOVASCULAR_ACUTE",
          "EMERGENCY PROTOCOL INTERRUPT: Immediate transfer to Emergency Department / Call 998 or 999.",
          "Typical radiation of cardiac ischemic pain. Immediate emergency dispatch." },
        { "جلطة قلبية", TRIAGE_EMERGENCY, "CARDIOVASCULAR_ACUTE",
          "بروتوكول طوارئ فوري: استدعاء الإسعاف 998 أو 999 فوراً.",
          "اشتباه احتشاء عضلة القلب الحاد. نقل فوري لمركز قسطرة قلبية." },
        { "ذبحة", TRIAGE_EMERGENCY, "CARDIOVASCULAR_ACUTE",
          "بروتوكول طوارئ فوري: استدعاء الإسعاف 998 أو 999 فوراً.",
          "اشتباه ذبحة صدرية غير مستقرة تستوجب التقييم الطبي العاجل." },

        /* Neurological / Acute Stroke */
        { "facial drooping", TRIAGE_EMERGENCY, "STROKE_NEUROLOGICAL",
          "EMERGENCY PROTOCOL INTERRUPT: Suspected Acute Stroke (FAST Protocol). Call 998/999 immediately.",
          "Acute neurological deficit indicative of stroke. Critical time window for reperfusion/thrombolysis." },
        { "face drooping", TRIAGE_EMERGENCY, "STROKE_NEUROLOGICAL",
          "EMERGENCY PROTOCOL INTERRUPT: Suspected Acute Stroke (FAST Protocol). Call 998/999 immediately.",
          "Acute neurological deficit indicative of stroke. Critical time window for reperfusion/thrombolysis." },
        { "slurred speech", TRIAGE_EMERGENCY, "STROKE_NEUROLOGICAL",
          "EMERGENCY PROTOCOL INTERRUPT: Suspected Acute Stroke (FAST Protocol). Call 998/999 immediately.",
          "Acute dysarthria or aphasia. Emergency stroke assessment required." },
        { "unilateral weakness", TRIAGE_EMERGENCY, "STROKE_NEUROLOGICAL",
          "EMERGENCY PROTOCOL INTERRUPT: Suspected Acute Stroke (FAST Protocol). Call 998/999 immediately.",
          "Sudden hemiparesis or unilateral limb weakness. Immediate stroke center transfer." },
        { "arm weakness", TRIAGE_EMERGENCY, "STROKE_NEUROLOGICAL",
          "EMERGENCY PROTOCOL INTERRUPT: Suspected Acute Stroke (FAST Protocol). Call 998/999 immediately.",
          "Sudden limb weakness. Emergency stroke pathway activation." },
        { "جلطة دماغية", TRIAGE_EMERGENCY, "STROKE_NEUROLOGICAL",
          "بروتوكول طوارئ فوري: الاتصال بالإسعاف 998 أو 999 فوراً.",
          "اشتباه حادث وعائي دماغي حاد. نقل عاجل لوحدة السكتة الدماغية." },
        { "سكتة دماغية", TRIAGE_EMERGENCY, "STROKE_NEUROLOGICAL",
          "بروتوكول طوارئ فوري: الاتصال بالإسعاف 998 أو 999 فوراً.",
          "حادث وعائي دماغي حاد يستدعي النقل الفوري." },

        /* Respiratory Distress / Severe Anaphylaxis */
        { "cannot breathe", TRIAGE_EMERGENCY, "RESPIRATORY_DISTRESS",
          "EMERGENCY PROTOCOL INTERRUPT: Acute Respiratory Compromise. Call 998/999 immediately.",
          "Imminent respiratory failure or severe acute airway compromise. Emergency stabilization required." },
        { "shortness of breath", TRIAGE_EMERGENCY, "RESPIRATORY_DISTRESS",
          "EMERGENCY PROTOCOL INTERRUPT: Severe Dyspnea. Call 998/999 immediately.",
          "Acute respiratory compromise. High risk of decompensation." },
        { "unable to breathe", TRIAGE_EMERGENCY, "RESPIRATORY_DISTRESS",
          "EMERGENCY PROTOCOL INTERRUPT: Acute Airway / Breathing Compromise. Call 998/999 immediately.",
          "Life-threatening respiratory compromise. Emergency airway management indicated." },
        { "anaphylaxis", TRIAGE_EMERGENCY, "RESPIRATORY_DISTRESS",
          "EMERGENCY PROTOCOL INTERRUPT: Suspected Anaphylaxis. Administer Epipen if available & call 998/999.",
          "Severe systemic IgE-mediated allergic reaction. Immediate IM epinephrine and airway protection required." },
        { "اختناق", TRIAGE_EMERGENCY, "RESPIRATORY_DISTRESS",
          "بروتوكول طوارئ فوري: انسداد مجرى التنفس. الاتصال بالإسعاف 998 أو 999 فوراً.",
          "انسداد ميكانيكي أو تشنج حنجري حاد لمجرى التنفس." },

        /* Severe Hemorrhage */
        { "coughing blood", TRIAGE_EMERGENCY, "SEVERE_HEMORRHAGE",
          "EMERGENCY PROTOCOL INTERRUPT: Severe Hemoptysis. Call 998/999 immediately.",
          "Active pulmonary hemorrhage. High risk of airway compromise and exsanguination." },
        { "vomiting blood", TRIAGE_EMERGENCY, "SEVERE_HEMORRHAGE",
          "EMERGENCY PROTOCOL INTERRUPT: Acute Upper GI Hemorrhage. Call 998/999 immediately.",
          "Severe hematemesis. Hemodynamic stabilization and urgent endoscopy required." },
        { "massive bleeding", TRIAGE_EMERGENCY, "SEVERE_HEMORRHAGE",
          "EMERGENCY PROTOCOL INTERRUPT: Active Hemorrhage. Call 998/999 immediately.",
          "Severe blood loss with impending hemorrhagic shock. Immediate vascular control and fluid resuscitation." },
        { "استفراغ دم", TRIAGE_EMERGENCY, "SEVERE_HEMORRHAGE",
          "بروتوكول طوارئ فوري: نزيف هضمي حاد. الاتصال بالإسعاف 998 أو 999 فوراً.",
          "قيء دموي حاد يستدعي الإنعاش الحجمي الفوري وتنظيراً هضمياً عاجلاً." },
        { "كحة دم", TRIAGE_EMERGENCY, "SEVERE_HEMORRHAGE",
          "بروتوكول طوارئ فوري: نفث دموي رئوي. الاتصال بالإسعاف 998 أو 999 فوراً.",
          "نزيف مجرى تنفسي مهدد للحياة يستدعي التدخل الفوري." },
        { "نزيف حاد", TRIAGE_EMERGENCY, "SEVERE_HEMORRHAGE",
          "بروتوكول طوارئ فوري: نزيف شديد. الاتصال بالإسعاف 998 أو 999 فوراً.",
          "فقدان دم حاد يستوجب السيطرة على النزف والإنعاش الدوري." },

        /* Altered Consciousness / Convulsions */
        { "loss of consciousness", TRIAGE_EMERGENCY, "NEURO_CONSCIOUSNESS",
          "EMERGENCY PROTOCOL INTERRUPT: Unresponsive Patient. Call 998/999 immediately.",
          "Coma or sudden syncope of undetermined etiology. ABCDE emergency resuscitation required." },
        { "passed out", TRIAGE_EMERGENCY, "NEURO_CONSCIOUSNESS",
          "EMERGENCY PROTOCOL INTERRUPT: Syncope / Loss of Consciousness. Call 998/999 immediately.",
          "Unresponsive state. Immediate medical assessment required." },
        { "active seizure", TRIAGE_EMERGENCY, "NEURO_CONSCIOUSNESS",
          "EMERGENCY PROTOCOL INTERRUPT: Active Convulsions / Seizure. Protect head & call 998/999.",
          "Risk of status epilepticus. Emergency benzodiazepines and airway protection indicated." },
        { "فقدان الوعي", TRIAGE_EMERGENCY, "NEURO_CONSCIOUSNESS",
          "بروتوكول طوارئ فوري: غياب عن الوعي. الاتصال بالإسعاف 998 أو 999 فوراً.",
          "فقدان وعي حاد يستدعي حماية مجرى التنفس والتقييم العصبي السريع." },
        { "اغماء", TRIAGE_EMERGENCY, "NEURO_CONSCIOUSNESS",
          "بروتوكول طوارئ فوري: حالة إغماء حادة. الاتصال بالإسعاف 998 أو 999 فوراً.",
          "انقطاع وعي مفاجئ يستوجب فحص سكر الدم والدوران الدماغي." },
        { "إغماء", TRIAGE_EMERGENCY, "NEURO_CONSCIOUSNESS",
          "بروتوكول طوارئ فوري: حالة إغماء حادة. الاتصال بالإسعاف 998 أو 999 فوراً.",
          "انقطاع وعي مفاجئ يستوجب فحص سكر الدم والدوران الدماغي." },
        { "تشنجات", TRIAGE_EMERGENCY, "NEURO_CONSCIOUSNESS",
          "بروتوكول طوارئ فوري: نوبة تشنجات نشطة. حماية الرأس والاتصال بالإسعاف 998 أو 999 فوراً.",
          "نوبة صرعية حادة أو اختلاجية تستدعي إيقاف النوبة وتأمين الأكسجين." },

        /* 4. Severe Hemorrhage */
        { "coughing blood", TRIAGE_EMERGENCY, "SEVERE_HEMORRHAGE",
          "EMERGENCY PROTOCOL INTERRUPT: Severe Hemoptysis. Call 998/999 immediately.",
          "Active pulmonary hemorrhage. High risk of airway compromise and exsanguination." },
        { "vomiting blood", TRIAGE_EMERGENCY, "SEVERE_HEMORRHAGE",
          "EMERGENCY PROTOCOL INTERRUPT: Acute Upper GI Hemorrhage. Call 998/999 immediately.",
          "Severe hematemesis. Hemodynamic stabilization and urgent endoscopy required." },
        { "massive bleeding", TRIAGE_EMERGENCY, "SEVERE_HEMORRHAGE",
          "EMERGENCY PROTOCOL INTERRUPT: Active Hemorrhage. Call 998/999 immediately.",
          "Severe blood loss with impending hemorrhagic shock. Immediate vascular control and fluid resuscitation." },
        { "استفراغ دم", TRIAGE_EMERGENCY, "SEVERE_HEMORRHAGE",
          "بروتوكول طوارئ فوري: نزيف هضمي حاد. الاتصال بالإسعاف 998 أو 999 فوراً.",
          "قيء دموي حاد يستدعي الإنعاش الحجمي الفوري وتنظيراً هضمياً عاجلاً." },
        { "كحة دم", TRIAGE_EMERGENCY, "SEVERE_HEMORRHAGE",
          "بروتوكول طوارئ فوري: نفث دموي رئوي. الاتصال بالإسعاف 998 أو 999 فوراً.",
          "نزيف مجرى تنفسي مهدد للحياة يستدعي التدخل الفوري." },
        { "نزيف حاد", TRIAGE_EMERGENCY, "SEVERE_HEMORRHAGE",
          "بروتوكول طوارئ فوري: نزيف شديد. الاتصال بالإسعاف 998 أو 999 فوراً.",
          "فقدان دم حاد يستوجب السيطرة على النزف والإنعاش الدوري." },

        /* 5. Altered Consciousness / Convulsions */
        { "loss of consciousness", TRIAGE_EMERGENCY, "NEURO_CONSCIOUSNESS",
          "EMERGENCY PROTOCOL INTERRUPT: Unresponsive Patient. Call 998/999 immediately.",
          "Coma or sudden syncope of undetermined etiology. ABCDE emergency resuscitation required." },
        { "passed out", TRIAGE_EMERGENCY, "NEURO_CONSCIOUSNESS",
          "EMERGENCY PROTOCOL INTERRUPT: Syncope / Loss of Consciousness. Call 998/999 immediately.",
          "Unresponsive state. Immediate medical assessment required." },
        { "active seizure", TRIAGE_EMERGENCY, "NEURO_CONSCIOUSNESS",
          "EMERGENCY PROTOCOL INTERRUPT: Active Convulsions / Seizure. Protect head & call 998/999.",
          "Risk of status epilepticus. Emergency benzodiazepines and airway protection indicated." },
        { "فقدان الوعي", TRIAGE_EMERGENCY, "NEURO_CONSCIOUSNESS",
          "بروتوكول طوارئ فوري: غياب عن الوعي. الاتصال بالإسعاف 998 أو 999 فوراً.",
          "فقدان وعي حاد يستدعي حماية مجرى التنفس والتقييم العصبي السريع." },
        { "اغماء", TRIAGE_EMERGENCY, "NEURO_CONSCIOUSNESS",
          "بروتوكول طوارئ فوري: حالة إغماء حادة. الاتصال بالإسعاف 998 أو 999 فوراً.",
          "انقطاع وعي مفاجئ يستوجب فحص سكر الدم والدوران الدماغي." },
        { "إغماء", TRIAGE_EMERGENCY, "NEURO_CONSCIOUSNESS",
          "بروتوكول طوارئ فوري: حالة إغماء حادة. الاتصال بالإسعاف 998 أو 999 فوراً.",
          "انقطاع وعي مفاجئ يستوجب فحص سكر الدم والدوران الدماغي." },
        { "تشنجات", TRIAGE_EMERGENCY, "NEURO_CONSCIOUSNESS",
          "بروتوكول طوارئ فوري: نوبة تشنجات نشطة. حماية الرأس والاتصال بالإسعاف 998 أو 999 فوراً.",
          "نوبة صرعية حادة أو اختلاجية تستدعي إيقاف النوبة وتأمين الأكسجين." },

        /* 6. Crisis / Psychiatric Emergency */
        { "suicidal thoughts", TRIAGE_EMERGENCY, "PSYCHIATRIC_EMERGENCY",
          "CRISIS INTERRUPT: Immediate Crisis Protocol. Do not leave patient alone. Call Crisis Support / 998/999.",
          "Active self-harm or suicidal crisis. Immediate clinical safety escort and emergency psychiatric support." },
        { "kill myself", TRIAGE_EMERGENCY, "PSYCHIATRIC_EMERGENCY",
          "CRISIS INTERRUPT: Immediate Crisis Protocol. Call Crisis Support / 998/999.",
          "Imminent self-harm risk. Urgent psychiatric emergency evaluation required." },
        { "انتحار", TRIAGE_EMERGENCY, "PSYCHIATRIC_EMERGENCY",
          "بروتوكول طوارئ نفسي فوري: عدم ترك المريض بمفرده والاتصال بخط الدعم النفسي والإسعاف 998/999 فوراً.",
          "خطر إيذاء نفس حاد يستدعي الإحالة النفسية الطارئة والمرافقة السريرية المستمرة." },
        { "افكار انتحارية", TRIAGE_EMERGENCY, "PSYCHIATRIC_EMERGENCY",
          "بروتوكول طوارئ نفسي فوري: الاتصال بخط المساندة والإسعاف 998/999 فوراً.",
          "أفكار إيذاء نفس حادة تتطلب تقييماً نفسياً إسعافياً فورياً." },
        { "أفكار انتحارية", TRIAGE_EMERGENCY, "PSYCHIATRIC_EMERGENCY",
          "بروتوكول طوارئ نفسي فوري: الاتصال بخط المساندة والإسعاف 998/999 فوراً.",
          "أفكار إيذاء نفس حادة تتطلب تقييماً نفسياً إسعافياً فورياً." },

        { NULL, TRIAGE_ROUTINE, NULL, NULL, NULL }
    };

    for (int i = 0; RED_FLAGS[i].trigger != NULL; ++i) {
        if (strcasestr_c99(symptom_narrative, RED_FLAGS[i].trigger)) {
            res.level = RED_FLAGS[i].level;
            res.is_red_flag = true;
            snprintf(res.triage_category, sizeof(res.triage_category), "%s", RED_FLAGS[i].category);
            snprintf(res.matched_trigger, sizeof(res.matched_trigger), "%s", RED_FLAGS[i].trigger);
            snprintf(res.action_protocol, sizeof(res.action_protocol), "%s", RED_FLAGS[i].protocol);
            snprintf(res.clinical_guidance, sizeof(res.clinical_guidance), "%s", RED_FLAGS[i].guidance);
            return res;
        }
    }

    return res;
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
        .has_ejadah_lab_justification = true,
        .has_clinician_override = false
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
        .has_ejadah_lab_justification = true,
        .has_clinician_override = false
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
        .has_ejadah_lab_justification = true,
        .has_clinician_override = false
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
        .has_ejadah_lab_justification = true,
        .has_clinician_override = false
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
        .has_ejadah_lab_justification = true,
        .has_clinician_override = false
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

    /* Test 10: Expanded Matrix - Simvastatin + Clarithromycin -> FATAL_CYP3A4_RHABDOMYOLYSIS_CONTRAINDICATION */
    taryaq_rx_context_t simva_clari = {
        .primary_drug = "Simvastatin",
        .concurrent_drug = "Clarithromycin",
        .patient_egfr_ml_min = 90.0, /* Even with normal renal function, absolute contraindication */
        .is_controlled_substance = false,
        .has_controlled_id_verified = false,
        .icd10_code = "E78.0",
        .submission_delay_days = 0,
        .has_ejadah_lab_justification = true
    };
    taryaq_result_t r10 = taryaq_verify_prescription_and_claim(&simva_clari);
    assert(r10.verdict == CLINICAL_CONTRAINDICATION_BLOCK);
    assert(strcmp(r10.alert_code, "FATAL_CYP3A4_RHABDOMYOLYSIS_CONTRAINDICATION") == 0);

    /* Test 11: Expanded Matrix - Warfarin + Amiodarone -> WARFARIN_AMIODARONE_HEMORRHAGE_RISK */
    taryaq_rx_context_t warf_amio = {
        .primary_drug = "Warfarin",
        .concurrent_drug = "Amiodarone",
        .patient_egfr_ml_min = 85.0,
        .is_controlled_substance = false,
        .has_controlled_id_verified = false,
        .icd10_code = "I48.9",
        .submission_delay_days = 0,
        .has_ejadah_lab_justification = true
    };
    taryaq_result_t r11 = taryaq_verify_prescription_and_claim(&warf_amio);
    assert(r11.verdict == CLINICAL_CONTRAINDICATION_BLOCK);
    assert(strcmp(r11.alert_code, "WARFARIN_AMIODARONE_HEMORRHAGE_RISK") == 0);

    /* Test 12: Fail-Closed Gate - Unmodeled drug (Ibuprofen with valid eGFR) -> UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW */
    taryaq_rx_context_t unmodeled_drug = {
        .primary_drug = "Ibuprofen 400mg",
        .concurrent_drug = NULL,
        .patient_egfr_ml_min = 95.0,
        .is_controlled_substance = false,
        .has_controlled_id_verified = false,
        .icd10_code = "R51",
        .submission_delay_days = 0,
        .has_ejadah_lab_justification = true
    };
    taryaq_result_t r12 = taryaq_verify_prescription_and_claim(&unmodeled_drug);
    assert(r12.verdict == CLINICAL_UNVERIFIED_REVIEW);
    assert(strcmp(r12.alert_code, "UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW") == 0);

    /* Test 13: Fail-Closed Gate - Unmodeled combination of known drugs -> UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW */
    taryaq_rx_context_t unmodeled_combo = {
        .primary_drug = "Colchicine",
        .concurrent_drug = "Warfarin",
        .patient_egfr_ml_min = 90.0,
        .is_controlled_substance = false,
        .has_controlled_id_verified = false,
        .icd10_code = "M10.0",
        .submission_delay_days = 0,
        .has_ejadah_lab_justification = true
    };
    taryaq_result_t r13 = taryaq_verify_prescription_and_claim(&unmodeled_combo);
    assert(r13.verdict == CLINICAL_UNVERIFIED_REVIEW);
    assert(strcmp(r13.alert_code, "UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW") == 0);

    /* Test 14: Emergency Red-Flag Triage - Arabic Acute Chest Pain Interrupt */
    taryaq_triage_result_t tr1 = taryaq_triage_patient_symptoms("أشعر بألم في الصدر وضيق تنفس مستمر");
    assert(tr1.is_red_flag == true);
    assert(tr1.level == TRIAGE_EMERGENCY);
    assert(strcmp(tr1.triage_category, "CARDIOVASCULAR_ACUTE") == 0);

    /* Test 15: Emergency Red-Flag Triage - English Acute Stroke Deficit Interrupt */
    taryaq_triage_result_t tr2 = taryaq_triage_patient_symptoms("Patient woke up with facial drooping and slurred speech");
    assert(tr2.is_red_flag == true);
    assert(tr2.level == TRIAGE_EMERGENCY);
    assert(strcmp(tr2.triage_category, "STROKE_NEUROLOGICAL") == 0);

    /* Test 16: Emergency Red-Flag Triage - Routine Care (No Red Flag) */
    taryaq_triage_result_t tr3 = taryaq_triage_patient_symptoms("أريد تجديد وصفة دواء السكر ميتفورمين 500");
    assert(tr3.is_red_flag == false);
    assert(tr3.level == TRIAGE_ROUTINE);
    assert(strcmp(tr3.triage_category, "ROUTINE_CARE") == 0);

    printf("[PASS] All Taryaq C99 Pharmacovigilance & DHA SN 03/2019 Claim Scrubber tests passed successfully!\n");
    return 0;
}
#endif
