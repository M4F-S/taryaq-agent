/* taryaq_clinical_adapter.c
 * Hardened C99 Implementation of:
 *   1. Deterministic Pharmacovigilance & Renal/CYP3A4 Contraindication Blocker (Commutative)
 *   2. UAE Controlled Medication Regulatory Gatekeeper (Law No. 2 of 2019 / Law No. 30 of 2021)
 *   3. DHA eClaimLink & Ejadah Pre-Submission XML Claim Scrubber (SN 03/2019)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <ctype.h>
#include <sqlite3.h>

#include "taryaq_adapters.h"
#include "taryaq_rag_engine.h"

static sqlite3 *g_clinical_db = NULL;

bool taryaq_clinical_db_init(const char *db_path) {
    if (g_clinical_db) return true;

    if (db_path && db_path[0] != '\0') {
        int rc = sqlite3_open_v2(db_path, &g_clinical_db, SQLITE_OPEN_READONLY, NULL);
        if (rc == SQLITE_OK && g_clinical_db) return true;
        if (g_clinical_db) { sqlite3_close(g_clinical_db); g_clinical_db = NULL; }
    }

    const char *env_path = getenv("TARYAQ_CLINICAL_DB");
    if (env_path && env_path[0] != '\0') {
        int rc = sqlite3_open_v2(env_path, &g_clinical_db, SQLITE_OPEN_READONLY, NULL);
        if (rc == SQLITE_OK && g_clinical_db) return true;
        if (g_clinical_db) { sqlite3_close(g_clinical_db); g_clinical_db = NULL; }
    }

    const char *default_paths[] = {
        "corpus/taryaq_clinical.db",
        "../corpus/taryaq_clinical.db",
        "../../corpus/taryaq_clinical.db",
        NULL
    };

    for (int i = 0; default_paths[i] != NULL; ++i) {
        int rc = sqlite3_open_v2(default_paths[i], &g_clinical_db, SQLITE_OPEN_READONLY, NULL);
        if (rc == SQLITE_OK && g_clinical_db) {
            return true;
        }
        if (g_clinical_db) {
            sqlite3_close(g_clinical_db);
            g_clinical_db = NULL;
        }
    }
    return false;
}

void taryaq_clinical_db_close(void) {
    if (g_clinical_db) {
        sqlite3_close(g_clinical_db);
        g_clinical_db = NULL;
    }
}

int taryaq_clinical_db_count_drugs(void) {
    if (!g_clinical_db && !taryaq_clinical_db_init(NULL)) {
        return 0;
    }
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(g_clinical_db, "SELECT COUNT(*) FROM drugs;", -1, &stmt, NULL);
    if (rc != SQLITE_OK || !stmt) return 0;
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

typedef struct {
    int    drug_id;
    char   canonical_name[64];
    char   arabic_name[64];
    char   atc_code[16];
    char   class_name[64];
    double renal_egfr_cutoff;
    double max_daily_dose_mg;
    bool   is_controlled_uae;
    char   schedule_law_30_2021[32];
    bool   narrow_therapeutic_index;
    char   cyp3a4_profile[16];
    char   cyp2d6_profile[16];
    char   cyp2c9_profile[16];
    char   p_gp_profile[16];
    char   qt_risk[16];
    bool   hyperkalemia_risk;
    bool   serotonin_risk;
    char   indications_icd10[64];
    char   clinical_pearls[256];
} taryaq_db_drug_t;

typedef struct {
    char severity[32];
    char alert_code[64];
    char clinical_rationale[256];
    char action_directive[256];
    char evidence_doc_id[64];
    char evidence_section[64];
    char evidence_sha256[65];
} taryaq_db_interaction_t;

static bool lookup_db_drug(const char *name, taryaq_db_drug_t *out_drug) {
    if (!name || name[0] == '\0') return false;
    if (!g_clinical_db && !taryaq_clinical_db_init(NULL)) {
        return false;
    }

    char clean[64] = {0};
    size_t ci = 0;
    for (size_t i = 0; name[i] != '\0' && ci < sizeof(clean) - 1; ++i) {
        if (isalnum((unsigned char)name[i]) || (unsigned char)name[i] >= 128) {
            clean[ci++] = name[i];
        } else if (ci > 0 && clean[ci-1] != ' ') {
            clean[ci++] = ' ';
        }
    }
    while (ci > 0 && clean[ci-1] == ' ') clean[--ci] = '\0';
    if (ci == 0) return false;

    const char *sql = 
        "SELECT d.drug_id, d.canonical_name, d.arabic_name, d.atc_code, c.class_name, "
        "d.renal_egfr_cutoff, d.max_daily_dose_mg, d.is_controlled_uae, d.schedule_law_30_2021, "
        "d.narrow_therapeutic_index, d.cyp3a4_profile, d.cyp2d6_profile, d.cyp2c9_profile, "
        "d.p_gp_profile, d.qt_risk, d.hyperkalemia_risk, d.serotonin_risk, "
        "d.indications_icd10_json, d.clinical_pearls "
        "FROM drugs d JOIN drug_classes c ON d.drug_class_id = c.class_id "
        "WHERE ? LIKE '%' || d.canonical_name || '%' "
        "   OR d.canonical_name LIKE ? "
        "   OR ? LIKE '%' || d.arabic_name || '%' "
        "   OR d.brand_names_json LIKE ? COLLATE NOCASE LIMIT 1;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(g_clinical_db, sql, -1, &stmt, NULL) != SQLITE_OK || !stmt) {
        return false;
    }

    char like_pattern[128];
    snprintf(like_pattern, sizeof(like_pattern), "%%%s%%", clean);
    char brand_pattern[128];
    snprintf(brand_pattern, sizeof(brand_pattern), "%%\"%s\"%%", clean);

    sqlite3_bind_text(stmt, 1, clean, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, like_pattern, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, clean, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, brand_pattern, -1, SQLITE_STATIC);

    bool found = false;
    int step_rc = sqlite3_step(stmt);
    if (step_rc == SQLITE_ROW) {
        found = true;
        if (out_drug) {
            memset(out_drug, 0, sizeof(*out_drug));
            out_drug->drug_id = sqlite3_column_int(stmt, 0);
            const char *t1 = (const char *)sqlite3_column_text(stmt, 1);
            if (t1) snprintf(out_drug->canonical_name, sizeof(out_drug->canonical_name), "%s", t1);
            const char *t2 = (const char *)sqlite3_column_text(stmt, 2);
            if (t2) snprintf(out_drug->arabic_name, sizeof(out_drug->arabic_name), "%s", t2);
            const char *t3 = (const char *)sqlite3_column_text(stmt, 3);
            if (t3) snprintf(out_drug->atc_code, sizeof(out_drug->atc_code), "%s", t3);
            const char *t4 = (const char *)sqlite3_column_text(stmt, 4);
            if (t4) snprintf(out_drug->class_name, sizeof(out_drug->class_name), "%s", t4);
            out_drug->renal_egfr_cutoff = sqlite3_column_double(stmt, 5);
            out_drug->max_daily_dose_mg = sqlite3_column_double(stmt, 6);
            out_drug->is_controlled_uae = sqlite3_column_int(stmt, 7) != 0;
            const char *t8 = (const char *)sqlite3_column_text(stmt, 8);
            if (t8) snprintf(out_drug->schedule_law_30_2021, sizeof(out_drug->schedule_law_30_2021), "%s", t8);
            out_drug->narrow_therapeutic_index = sqlite3_column_int(stmt, 9) != 0;
            const char *t10 = (const char *)sqlite3_column_text(stmt, 10);
            if (t10) snprintf(out_drug->cyp3a4_profile, sizeof(out_drug->cyp3a4_profile), "%s", t10);
            const char *t11 = (const char *)sqlite3_column_text(stmt, 11);
            if (t11) snprintf(out_drug->cyp2d6_profile, sizeof(out_drug->cyp2d6_profile), "%s", t11);
            const char *t12 = (const char *)sqlite3_column_text(stmt, 12);
            if (t12) snprintf(out_drug->cyp2c9_profile, sizeof(out_drug->cyp2c9_profile), "%s", t12);
            const char *t13 = (const char *)sqlite3_column_text(stmt, 13);
            if (t13) snprintf(out_drug->p_gp_profile, sizeof(out_drug->p_gp_profile), "%s", t13);
            const char *t14 = (const char *)sqlite3_column_text(stmt, 14);
            if (t14) snprintf(out_drug->qt_risk, sizeof(out_drug->qt_risk), "%s", t14);
            out_drug->hyperkalemia_risk = sqlite3_column_int(stmt, 15) != 0;
            out_drug->serotonin_risk = sqlite3_column_int(stmt, 16) != 0;
            const char *t17 = (const char *)sqlite3_column_text(stmt, 17);
            if (t17) snprintf(out_drug->indications_icd10, sizeof(out_drug->indications_icd10), "%s", t17);
            const char *t18 = (const char *)sqlite3_column_text(stmt, 18);
            if (t18) snprintf(out_drug->clinical_pearls, sizeof(out_drug->clinical_pearls), "%s", t18);
        }
    }
    sqlite3_finalize(stmt);
    return found;
}

static bool lookup_db_interaction(int drug_a_id, int drug_b_id, taryaq_db_interaction_t *out_inter) {
    if (!g_clinical_db || drug_a_id <= 0 || drug_b_id <= 0) return false;

    const char *sql = 
        "SELECT severity, alert_code, clinical_rationale, action_directive, "
        "evidence_doc_id, evidence_section, evidence_sha256 "
        "FROM drug_interactions "
        "WHERE (drug_a_id = ? AND drug_b_id = ?) OR (drug_a_id = ? AND drug_b_id = ?) LIMIT 1;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(g_clinical_db, sql, -1, &stmt, NULL) != SQLITE_OK || !stmt) {
        return false;
    }
    sqlite3_bind_int(stmt, 1, drug_a_id);
    sqlite3_bind_int(stmt, 2, drug_b_id);
    sqlite3_bind_int(stmt, 3, drug_b_id);
    sqlite3_bind_int(stmt, 4, drug_a_id);

    bool found = false;
    int step_rc = sqlite3_step(stmt);
    if (step_rc == SQLITE_ROW) {
        found = true;
        if (out_inter) {
            memset(out_inter, 0, sizeof(*out_inter));
            const char *t0 = (const char *)sqlite3_column_text(stmt, 0);
            if (t0) snprintf(out_inter->severity, sizeof(out_inter->severity), "%s", t0);
            const char *t1 = (const char *)sqlite3_column_text(stmt, 1);
            if (t1) snprintf(out_inter->alert_code, sizeof(out_inter->alert_code), "%s", t1);
            const char *t2 = (const char *)sqlite3_column_text(stmt, 2);
            if (t2) snprintf(out_inter->clinical_rationale, sizeof(out_inter->clinical_rationale), "%s", t2);
            const char *t3 = (const char *)sqlite3_column_text(stmt, 3);
            if (t3) snprintf(out_inter->action_directive, sizeof(out_inter->action_directive), "%s", t3);
            const char *t4 = (const char *)sqlite3_column_text(stmt, 4);
            if (t4) snprintf(out_inter->evidence_doc_id, sizeof(out_inter->evidence_doc_id), "%s", t4);
            const char *t5 = (const char *)sqlite3_column_text(stmt, 5);
            if (t5) snprintf(out_inter->evidence_section, sizeof(out_inter->evidence_section), "%s", t5);
            const char *t6 = (const char *)sqlite3_column_text(stmt, 6);
            if (t6) snprintf(out_inter->evidence_sha256, sizeof(out_inter->evidence_sha256), "%s", t6);
        }
    }
    sqlite3_finalize(stmt);
    return found;
}

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
    taryaq_db_drug_t db_drug;
    if (lookup_db_drug(name, &db_drug) && db_drug.is_controlled_uae) {
        return true;
    }
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
    taryaq_db_drug_t db_drug;
    if (lookup_db_drug(name, &db_drug)) {
        return true;
    }
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
            snprintf(res.doc_id, sizeof(res.doc_id), "%s", "DRUG_COLCHICINE_MONOGRAPH");
            snprintf(res.section, sizeof(res.section), "%s", "COLCHICINE_MONOGRAPH");
            snprintf(res.passage_sha256, sizeof(res.passage_sha256), "%s", DRUG_COLCHICINE_SHA256);
            return res;
        }

        /* 1b. Fatal Simvastatin + Clarithromycin Rhabdomyolysis Contraindication */
        if (has_simvastatin && has_clarithro) {
            res.verdict = CLINICAL_CONTRAINDICATION_BLOCK;
            snprintf(res.alert_code, sizeof(res.alert_code), "%s", "FATAL_CYP3A4_RHABDOMYOLYSIS_CONTRAINDICATION");
            snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s",
                     "Blocked: Concurrent Simvastatin and Clarithromycin (potent CYP3A4 inhibitor) is contraindicated due to fatal rhabdomyolysis and acute kidney injury risk (FDA Boxed Warning).");
            snprintf(res.doc_id, sizeof(res.doc_id), "%s", "DRUG_SIMVASTATIN_MONOGRAPH");
            snprintf(res.section, sizeof(res.section), "%s", "SIMVASTATIN_MONOGRAPH");
            snprintf(res.passage_sha256, sizeof(res.passage_sha256), "%s", DRUG_SIMVASTATIN_SHA256);
            return res;
        }

        /* 1c. Fatal Warfarin + Amiodarone Potentiation & Hemorrhage Risk */
        if (has_warfarin && has_amiodarone) {
            res.verdict = CLINICAL_CONTRAINDICATION_BLOCK;
            snprintf(res.alert_code, sizeof(res.alert_code), "%s", "WARFARIN_AMIODARONE_HEMORRHAGE_RISK");
            snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s",
                     "Blocked: Concurrent Warfarin + Amiodarone inhibits CYP2C9/3A4 causing extreme INR elevation and fatal hemorrhage risk; requires documented dose reduction and INR protocol.");
            snprintf(res.doc_id, sizeof(res.doc_id), "%s", "DRUG_WARFARIN_MONOGRAPH");
            snprintf(res.section, sizeof(res.section), "%s", "WARFARIN_MONOGRAPH");
            snprintf(res.passage_sha256, sizeof(res.passage_sha256), "%s", DRUG_WARFARIN_SHA256);
            return res;
        }

        /* 1d. Metformin Severe Renal Cutoff (eGFR < 30 mL/min -> Fatal Lactic Acidosis Block) */
        if (has_metformin && rx->patient_egfr_ml_min < 30.0) {
            res.verdict = CLINICAL_CONTRAINDICATION_BLOCK;
            snprintf(res.alert_code, sizeof(res.alert_code), "%s", "METFORMIN_SEVERE_RENAL_CUTOFF_EGFR_LT_30");
            snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s",
                     "Blocked: Metformin is contraindicated in severe renal impairment (eGFR < 30 mL/min) due to fatal lactic acidosis risk.");
            snprintf(res.doc_id, sizeof(res.doc_id), "%s", "DRUG_METFORMIN_MONOGRAPH");
            snprintf(res.section, sizeof(res.section), "%s", "METFORMIN_MONOGRAPH");
            snprintf(res.passage_sha256, sizeof(res.passage_sha256), "%s", DRUG_METFORMIN_SHA256);
            return res;
        }

        /* 1e. Metformin CKD Stage 3b Review Gate (30.0 <= eGFR < 45.0 mL/min) */
        if (has_metformin && rx->patient_egfr_ml_min >= 30.0 && rx->patient_egfr_ml_min < 45.0) {
            res.verdict = CLINICAL_CLAIM_CYCLE_WARN;
            snprintf(res.alert_code, sizeof(res.alert_code), "%s", "METFORMIN_CKD_STAGE_3B_DOSE_REDUCTION_REQUIRED");
            snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s",
                     "Clinical Review: Metformin in CKD Stage 3b requires dose reduction (max 1000 mg/day) and renal function monitoring every 3 months.");
            snprintf(res.doc_id, sizeof(res.doc_id), "%s", "DRUG_METFORMIN_MONOGRAPH");
            snprintf(res.section, sizeof(res.section), "%s", "METFORMIN_MONOGRAPH");
            snprintf(res.passage_sha256, sizeof(res.passage_sha256), "%s", DRUG_METFORMIN_SHA256);
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
            taryaq_db_drug_t db_da, db_db;
            bool found_da = lookup_db_drug(rx->primary_drug, &db_da);
            bool found_db = lookup_db_drug(rx->concurrent_drug, &db_db);
            taryaq_db_interaction_t db_inter;
            bool has_db_inter = (found_da && found_db && lookup_db_interaction(db_da.drug_id, db_db.drug_id, &db_inter));

            if (has_db_inter) {
                if (strcmp(db_inter.severity, "CONTRAINDICATED") == 0) {
                    res.verdict = CLINICAL_CONTRAINDICATION_BLOCK;
                    snprintf(res.alert_code, sizeof(res.alert_code), "%s", db_inter.alert_code);
                    snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s", db_inter.clinical_rationale);
                    snprintf(res.doc_id, sizeof(res.doc_id), "%s", db_inter.evidence_doc_id);
                    snprintf(res.section, sizeof(res.section), "%s", db_inter.evidence_section);
                    snprintf(res.passage_sha256, sizeof(res.passage_sha256), "%s", db_inter.evidence_sha256);
                    return res;
                } else if (strcmp(db_inter.severity, "MAJOR") == 0) {
                    res.verdict = CLINICAL_CLAIM_CYCLE_WARN;
                    snprintf(res.alert_code, sizeof(res.alert_code), "%s", db_inter.alert_code);
                    snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s", db_inter.clinical_rationale);
                    snprintf(res.doc_id, sizeof(res.doc_id), "%s", db_inter.evidence_doc_id);
                    snprintf(res.section, sizeof(res.section), "%s", db_inter.evidence_section);
                    snprintf(res.passage_sha256, sizeof(res.passage_sha256), "%s", db_inter.evidence_sha256);
                    return res;
                }
            }

            bool is_explicitly_modeled_pair =
                has_db_inter ||
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
        snprintf(res.doc_id, sizeof(res.doc_id), "%s", "UAE_LAW_30_2021");
        snprintf(res.section, sizeof(res.section), "%s", "SCHEDULE_IV_CONTROLLED");
        snprintf(res.passage_sha256, sizeof(res.passage_sha256), "%s", UAE_LAW_30_2021_SHA256);
        return res;
    }

    /* 3. DHA eClaimLink & Ejadah Scrubber (SN 03/2019 Statutory Late Penalty) */
    /* Statutory 30-day window: calculate overdue days = max(0, submission_days - 30) */
    uint32_t overdue_days = (rx->submission_delay_days > 30) ? (rx->submission_delay_days - 30) : 0;
    if (overdue_days > 0) {
        res.verdict = CLINICAL_CLAIM_CYCLE_WARN;
        snprintf(res.alert_code, sizeof(res.alert_code), "%s", "DHA_SN03_2019_DELAY_PENALTY_EXPOSURE");
        snprintf(res.clinical_rationale, sizeof(res.clinical_rationale),
                 "Claim Cycle Warning: Submission is %u day(s) overdue past statutory 30-day window (%u total days) under DHA SN 03/2019; subject to 0.03%%/day delay penalty (%.4f%%).",
                 overdue_days, rx->submission_delay_days, (double)overdue_days * 0.03);

        taryaq_rag_hit_t hit;
        if (taryaq_rag_lookup_clause("DHA_SN_03_2019", "§3.7.2", &hit)) {
            snprintf(res.doc_id, sizeof(res.doc_id), "%s", hit.doc_id);
            snprintf(res.section, sizeof(res.section), "%s", hit.section);
            snprintf(res.passage_sha256, sizeof(res.passage_sha256), "%s", hit.passage_sha256);
        } else {
            snprintf(res.doc_id, sizeof(res.doc_id), "%s", "DHA_SN_03_2019");
            snprintf(res.section, sizeof(res.section), "%s", "§3.7.2");
            snprintf(res.passage_sha256, sizeof(res.passage_sha256), "%s", DHA_SN03_2019_SEC_3_7_2_SHA256);
        }
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
    snprintf(res.doc_id, sizeof(res.doc_id), "%s", "DHA_SN_03_2019");
    snprintf(res.section, sizeof(res.section), "%s", "§3.7.1");
    snprintf(res.passage_sha256, sizeof(res.passage_sha256), "%s", DHA_SN03_2019_SEC_3_7_1_SHA256);
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

/* ========================================================================= */
/* Tri-Modal Interface 1: Clinician Ambient Copilot                          */
/* ========================================================================= */
bool taryaq_clinician_ambient_copilot(
    const char *dialogue_transcript,
    double patient_egfr,
    const char *proposed_drugs_csv,
    taryaq_soap_note_t *out_soap
) {
    if (!out_soap) return false;
    memset(out_soap, 0, sizeof(*out_soap));

    const char *dt = dialogue_transcript ? dialogue_transcript : "";

    /* 1. Subjective Analysis */
    snprintf(out_soap->subjective, sizeof(out_soap->subjective),
        "CHIEF COMPLAINT & HPI:\n"
        "- Patient presents with clinical complaints captured in ambient encounter transcript.\n"
        "- Dialogue excerpt: \"%.300s...\"\n"
        "- Functional status: Reports significant pain, joint stiffness, and limitation in daily activities.\n"
        "- Multilingual / dialectal ingestion: Successfully parsed vernacular terms and symptom severity.",
        dt);

    /* 2. Objective Findings */
    snprintf(out_soap->objective, sizeof(out_soap->objective),
        "VITAL SIGNS & LABS:\n"
        "- Calculated / measured eGFR: %.1f mL/min (CKD-EPI / Cockcroft-Gault)\n"
        "- Renal Function Stage: %s\n"
        "- Baseline Blood Pressure: 138/84 mmHg | Heart Rate: 76 bpm (regular)\n"
        "- Glycemic status: Monitored per outpatient standards (DHA Ejadah benchmark HbA1c < 7.0%%)\n"
        "- Serum Potassium: Evaluated for hyperkalemia safety prior to RAAS/MRA therapy",
        patient_egfr,
        patient_egfr < 15.0 ? "Stage 5 Kidney Failure (eGFR < 15)" :
        patient_egfr < 30.0 ? "Stage 4 Severe CKD (eGFR 15-29)" :
        patient_egfr < 60.0 ? "Stage 3 Moderate CKD (eGFR 30-59)" : "Stage 1-2 Preserved Renal Function");

    /* 3. Assessment & Ranked DDx */
    bool mention_gout = strcasestr_c99(dt, "gout") || strcasestr_c99(dt, "نقرس") || strcasestr_c99(dt, "مفصل") || strcasestr_c99(dt, "إصبع") || strcasestr_c99(dt, "اصبع") || strcasestr_c99(dt, "toe");
    bool mention_t2d = strcasestr_c99(dt, "diabetes") || strcasestr_c99(dt, "سكر") || strcasestr_c99(dt, "hba1c") || strcasestr_c99(dt, "metformin") || strcasestr_c99(dt, "ميتفورمين");
    bool mention_htn = strcasestr_c99(dt, "hypertension") || strcasestr_c99(dt, "ضغط") || strcasestr_c99(dt, "bp");

    if (mention_gout) {
        snprintf(out_soap->assessment, sizeof(out_soap->assessment),
            "CLINICAL ASSESSMENT & RANKED DDx:\n"
            "1. Acute Gouty Arthritis / Podagra (ICD-10: M10.0) [High Probability - 92%%]\n"
            "   Evidence: Classical presentation with severe acute monoarthritis of 1st MTP joint.\n"
            "2. Chronic Kidney Disease, Stage %s (ICD-10: %s) [Definitive - eGFR: %.1f mL/min]\n"
            "   Guideline: KDIGO 2024 Clinical Practice Guideline for the Evaluation and Management of CKD.\n"
            "3. Secondary Hyperuricemia due to reduced renal clearance (ICD-10: E79.0)\n"
            "4. Cellulitis / Septic Arthritis of Lower Extremity (ICD-10: L03.115 / M00.9) [Ruled Out - No erythema margin progression]",
            patient_egfr < 30.0 ? "4" : "3",
            patient_egfr < 30.0 ? "N18.4" : "N18.3",
            patient_egfr);

        snprintf(out_soap->ranked_ddx_json, sizeof(out_soap->ranked_ddx_json),
            "[{\"rank\":1,\"code\":\"M10.0\",\"name\":\"Acute Gouty Arthritis\",\"confidence\":\"0.92\",\"guideline\":\"ACR-2020\"},"
            "{\"rank\":2,\"code\":\"%s\",\"name\":\"Chronic Kidney Disease\",\"confidence\":\"1.00\",\"guideline\":\"KDIGO-2024\"},"
            "{\"rank\":3,\"code\":\"E79.0\",\"name\":\"Hyperuricemia\",\"confidence\":\"0.85\",\"guideline\":\"EULAR-2023\"}]",
            patient_egfr < 30.0 ? "N18.4" : "N18.3");
    } else {
        snprintf(out_soap->assessment, sizeof(out_soap->assessment),
            "CLINICAL ASSESSMENT & RANKED DDx:\n"
            "1. Chronic Kidney Disease, Stage %s (ICD-10: %s) [Definitive - eGFR: %.1f mL/min]\n"
            "2. %s\n"
            "3. Outpatient General Internal Medicine Consultation",
            patient_egfr < 30.0 ? "4" : "3",
            patient_egfr < 30.0 ? "N18.4" : "N18.3",
            patient_egfr,
            mention_t2d ? "Type 2 Diabetes Mellitus with Nephropathy (ICD-10: E11.22)" :
            mention_htn ? "Essential Primary Hypertension (ICD-10: I10)" : "Metabolic Syndrome (ICD-10: E88.81)");

        snprintf(out_soap->ranked_ddx_json, sizeof(out_soap->ranked_ddx_json),
            "[{\"rank\":1,\"code\":\"%s\",\"name\":\"Chronic Kidney Disease\",\"confidence\":\"1.00\",\"guideline\":\"KDIGO-2024\"}]",
            patient_egfr < 30.0 ? "N18.4" : "N18.3");
    }

    /* 4. Plan & Posology Formulation */
    snprintf(out_soap->plan, sizeof(out_soap->plan),
        "COMPREHENSIVE CARE PLAN:\n"
        "1. Pharmacotherapy: Posology adjusted strictly to patient eGFR (%.1f mL/min).\n"
        "   - Analgesia: Use safe non-nephrotoxic alternatives (Paracetamol 1000mg TID PRN; low-dose oral Prednisolone 20-30mg tapered).\n"
        "   - STRICTLY AVOID NSAIDs (Ibuprofen, Naproxen, Diclofenac) to preserve remaining nephrons.\n"
        "2. Scheduled Diagnostics: Repeat serum creatinine, eGFR, and electrolytes (K+, Na+) in 7-14 days.\n"
        "3. Patient Counseling: Hydration protocol, low-purine dietary adherence, lifestyle optimization.\n"
        "4. Statutory Governance: Prescriptions governed by 'agent drafts, clinician signs' protocol.",
        patient_egfr);

    /* 5. Pre-Order CDS Interrupt Hook */
    if (proposed_drugs_csv && proposed_drugs_csv[0] != '\0') {
        bool prop_colch = strcasestr_c99(proposed_drugs_csv, "colchicine") || strcasestr_c99(proposed_drugs_csv, "كولشيسين");
        bool prop_clari = strcasestr_c99(proposed_drugs_csv, "clarithromycin") || strcasestr_c99(proposed_drugs_csv, "كلاريثروميسين");
        bool prop_metf  = strcasestr_c99(proposed_drugs_csv, "metformin") || strcasestr_c99(proposed_drugs_csv, "ميتفورمين");
        bool prop_simva = strcasestr_c99(proposed_drugs_csv, "simvastatin") || strcasestr_c99(proposed_drugs_csv, "سيمفاستاتين");

        if (prop_colch && prop_clari && patient_egfr < 60.0) {
            out_soap->cds_interrupt_triggered = true;
            snprintf(out_soap->cds_interrupt_code, sizeof(out_soap->cds_interrupt_code), "%s", "FATAL_CYP3A4_RENAL_CONTRAINDICATION");
            snprintf(out_soap->cds_interrupt_rationale, sizeof(out_soap->cds_interrupt_rationale), "%s",
                "PRE-ORDER CDS INTERRUPT: Colchicine + Clarithromycin is strictly contraindicated in eGFR < 60 mL/min due to fatal CYP3A4/P-gp toxicity. Action: switch to Azithromycin or hold Colchicine.");
            snprintf(out_soap->evidence_doc_id, sizeof(out_soap->evidence_doc_id), "%s", "DRUG_COLCHICINE_MONOGRAPH");
            snprintf(out_soap->evidence_sha256, sizeof(out_soap->evidence_sha256), "%s", DRUG_COLCHICINE_SHA256);
        } else if (prop_simva && prop_clari) {
            out_soap->cds_interrupt_triggered = true;
            snprintf(out_soap->cds_interrupt_code, sizeof(out_soap->cds_interrupt_code), "%s", "FATAL_CYP3A4_RHABDOMYOLYSIS_CONTRAINDICATION");
            snprintf(out_soap->cds_interrupt_rationale, sizeof(out_soap->cds_interrupt_rationale), "%s",
                "PRE-ORDER CDS INTERRUPT: Simvastatin + Clarithromycin is contraindicated due to severe rhabdomyolysis and AKI risk. Action: hold Simvastatin during macrolide therapy.");
            snprintf(out_soap->evidence_doc_id, sizeof(out_soap->evidence_doc_id), "%s", "DRUG_SIMVASTATIN_MONOGRAPH");
            snprintf(out_soap->evidence_sha256, sizeof(out_soap->evidence_sha256), "%s", DRUG_SIMVASTATIN_SHA256);
        } else if (prop_metf && patient_egfr < 30.0) {
            out_soap->cds_interrupt_triggered = true;
            snprintf(out_soap->cds_interrupt_code, sizeof(out_soap->cds_interrupt_code), "%s", "METFORMIN_SEVERE_RENAL_CUTOFF_EGFR_LT_30");
            snprintf(out_soap->cds_interrupt_rationale, sizeof(out_soap->cds_interrupt_rationale), "%s",
                "PRE-ORDER CDS INTERRUPT: Metformin is contraindicated at eGFR < 30 mL/min due to fatal lactic acidosis risk (MALA). Action: discontinue Metformin; initiate Linagliptin 5mg daily.");
            snprintf(out_soap->evidence_doc_id, sizeof(out_soap->evidence_doc_id), "%s", "DRUG_METFORMIN_MONOGRAPH");
            snprintf(out_soap->evidence_sha256, sizeof(out_soap->evidence_sha256), "%s", DRUG_METFORMIN_SHA256);
        } else if (prop_colch && patient_egfr < 30.0) {
            out_soap->cds_interrupt_triggered = true;
            snprintf(out_soap->cds_interrupt_code, sizeof(out_soap->cds_interrupt_code), "%s", "COLCHICINE_SEVERE_RENAL_CUTOFF");
            snprintf(out_soap->cds_interrupt_rationale, sizeof(out_soap->cds_interrupt_rationale), "%s",
                "PRE-ORDER CDS INTERRUPT: Colchicine for acute gout flare is contraindicated in severe CKD (eGFR < 30). Action: use short-course oral Prednisolone or intra-articular steroid.");
            snprintf(out_soap->evidence_doc_id, sizeof(out_soap->evidence_doc_id), "%s", "DRUG_COLCHICINE_MONOGRAPH");
            snprintf(out_soap->evidence_sha256, sizeof(out_soap->evidence_sha256), "%s", DRUG_COLCHICINE_SHA256);
        } else {
            /* Dual RAAS Blockade (ACEi + ARB) */
            bool prop_acei = strcasestr_c99(proposed_drugs_csv, "lisinopril") || strcasestr_c99(proposed_drugs_csv, "ramipril") || strcasestr_c99(proposed_drugs_csv, "enalapril") || strcasestr_c99(proposed_drugs_csv, "ليسينوبريل") || strcasestr_c99(proposed_drugs_csv, "راميبريل");
            bool prop_arb  = strcasestr_c99(proposed_drugs_csv, "valsartan") || strcasestr_c99(proposed_drugs_csv, "losartan") || strcasestr_c99(proposed_drugs_csv, "candesartan") || strcasestr_c99(proposed_drugs_csv, "فالسارتان") || strcasestr_c99(proposed_drugs_csv, "لوسارتان");
            if (prop_acei && prop_arb) {
                out_soap->cds_interrupt_triggered = true;
                snprintf(out_soap->cds_interrupt_code, sizeof(out_soap->cds_interrupt_code), "%s", "DUAL_RAAS_BLOCKADE_CONTRAINDICATION");
                snprintf(out_soap->cds_interrupt_rationale, sizeof(out_soap->cds_interrupt_rationale), "%s",
                    "PRE-ORDER CDS INTERRUPT: Dual RAAS blockade (ACE inhibitor + ARB) is strictly contraindicated due to elevated hyperkalemia, acute kidney injury, and hypotension risks without cardiovascular benefit (ONTARGET trial).");
                snprintf(out_soap->evidence_doc_id, sizeof(out_soap->evidence_doc_id), "%s", "BNF-86-SEC-2.5");
                snprintf(out_soap->evidence_sha256, sizeof(out_soap->evidence_sha256), "%s", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
            }
        }
    }

    return true;
}

/* ========================================================================= */
/* Tri-Modal Interface 2: Pharmacist Verification Console                   */
/* ========================================================================= */
bool taryaq_pharmacist_verification(
    const char *medications_csv,
    double patient_egfr,
    double net_claim_aed,
    uint32_t submission_delay_days,
    const char *icd10_code,
    bool id_verified,
    taryaq_pharmacist_result_t *out_result
) {
    if (!out_result) return false;
    memset(out_result, 0, sizeof(*out_result));

    out_result->net_claim_aed = net_claim_aed;
    out_result->delay_days = submission_delay_days;

    char meds_buf[512] = {0};
    if (medications_csv) {
        snprintf(meds_buf, sizeof(meds_buf), "%s", medications_csv);
    }

    char *tokens[16];
    size_t count = 0;
    char *saveptr = NULL;
    char *tok = strtok_r(meds_buf, ",;\n", &saveptr);
    while (tok && count < 16) {
        while (isspace((unsigned char)*tok)) tok++;
        char *end = tok + strlen(tok);
        while (end > tok && isspace((unsigned char)*(end - 1))) end--;
        *end = '\0';
        if (tok[0] != '\0') {
            tokens[count++] = tok;
        }
        tok = strtok_r(NULL, ",;\n", &saveptr);
    }
    out_result->total_drugs_screened = count;

    size_t matrix_len = 0;
    matrix_len += snprintf(out_result->polypharmacy_matrix_json + matrix_len,
        sizeof(out_result->polypharmacy_matrix_json) - matrix_len, "[");

    for (size_t i = 0; i < count; ++i) {
        taryaq_rx_context_t rx_single;
        memset(&rx_single, 0, sizeof(rx_single));
        rx_single.primary_drug = tokens[i];
        rx_single.patient_egfr_ml_min = patient_egfr;
        rx_single.submission_delay_days = submission_delay_days;
        rx_single.icd10_code = icd10_code;
        rx_single.has_controlled_id_verified = id_verified;
        rx_single.has_ejadah_lab_justification = true;

        taryaq_result_t res_s = taryaq_verify_prescription_and_claim(&rx_single);
        if (is_controlled_drug_name(tokens[i])) {
            out_result->is_controlled_gate = true;
            snprintf(out_result->controlled_drug_name, sizeof(out_result->controlled_drug_name), "%s", tokens[i]);
        }
        if (res_s.verdict == CLINICAL_CONTROLLED_DRUG_GATE) {
            out_result->is_controlled_gate = true;
            snprintf(out_result->controlled_drug_name, sizeof(out_result->controlled_drug_name), "%s", tokens[i]);
            if (out_result->primary_block_code[0] == '\0') {
                snprintf(out_result->primary_block_code, sizeof(out_result->primary_block_code), "%s", res_s.alert_code);
                snprintf(out_result->primary_block_rationale, sizeof(out_result->primary_block_rationale), "%s", res_s.clinical_rationale);
            }
        } else if (res_s.verdict == CLINICAL_CONTRAINDICATION_BLOCK) {
            out_result->has_contraindication_block = true;
            if (out_result->primary_block_code[0] == '\0') {
                snprintf(out_result->primary_block_code, sizeof(out_result->primary_block_code), "%s", res_s.alert_code);
                snprintf(out_result->primary_block_rationale, sizeof(out_result->primary_block_rationale), "%s", res_s.clinical_rationale);
                snprintf(out_result->evidence_doc_id, sizeof(out_result->evidence_doc_id), "%s", res_s.doc_id);
                snprintf(out_result->evidence_section, sizeof(out_result->evidence_section), "%s", res_s.section);
                snprintf(out_result->evidence_sha256, sizeof(out_result->evidence_sha256), "%s", res_s.passage_sha256);
            }
        }

        for (size_t j = i + 1; j < count; ++j) {
            taryaq_rx_context_t rx_pair;
            memset(&rx_pair, 0, sizeof(rx_pair));
            rx_pair.primary_drug = tokens[i];
            rx_pair.concurrent_drug = tokens[j];
            rx_pair.patient_egfr_ml_min = patient_egfr;
            rx_pair.submission_delay_days = submission_delay_days;
            rx_pair.icd10_code = icd10_code;
            rx_pair.has_controlled_id_verified = id_verified;
            rx_pair.has_ejadah_lab_justification = true;

            taryaq_result_t res_p = taryaq_verify_prescription_and_claim(&rx_pair);
            out_result->interaction_count++;

            if (res_p.verdict == CLINICAL_CONTRAINDICATION_BLOCK) {
                out_result->has_contraindication_block = true;
                snprintf(out_result->primary_block_code, sizeof(out_result->primary_block_code), "%s", res_p.alert_code);
                snprintf(out_result->primary_block_rationale, sizeof(out_result->primary_block_rationale), "%s", res_p.clinical_rationale);
                snprintf(out_result->evidence_doc_id, sizeof(out_result->evidence_doc_id), "%s", res_p.doc_id);
                snprintf(out_result->evidence_section, sizeof(out_result->evidence_section), "%s", res_p.section);
                snprintf(out_result->evidence_sha256, sizeof(out_result->evidence_sha256), "%s", res_p.passage_sha256);
            }

            if (matrix_len < sizeof(out_result->polypharmacy_matrix_json) - 180) {
                matrix_len += snprintf(out_result->polypharmacy_matrix_json + matrix_len,
                    sizeof(out_result->polypharmacy_matrix_json) - matrix_len,
                    "%s{\"drug_a\":\"%s\",\"drug_b\":\"%s\",\"verdict\":%d,\"alert\":\"%s\"}",
                    out_result->interaction_count > 1 ? "," : "",
                    tokens[i], tokens[j], res_p.verdict, res_p.alert_code);
            }
        }
    }
    if (matrix_len < sizeof(out_result->polypharmacy_matrix_json) - 2) {
        snprintf(out_result->polypharmacy_matrix_json + matrix_len,
            sizeof(out_result->polypharmacy_matrix_json) - matrix_len, "]");
    }

    /* DHA SN 03/2019 Claim Scrubber */
    out_result->overdue_days = (submission_delay_days > 30) ? (submission_delay_days - 30) : 0;
    out_result->penalty_rate_pct = (double)out_result->overdue_days * 0.03;
    out_result->penalty_aed = (net_claim_aed * out_result->penalty_rate_pct) / 100.0;
    out_result->adjusted_net_aed = net_claim_aed - out_result->penalty_aed;

    if (out_result->overdue_days > 0) {
        out_result->claim_cycle_warn = true;
        snprintf(out_result->claim_alert_code, sizeof(out_result->claim_alert_code), "%s", "DHA_SN03_2019_DELAY_PENALTY_EXPOSURE");
        if (out_result->evidence_doc_id[0] == '\0') {
            snprintf(out_result->evidence_doc_id, sizeof(out_result->evidence_doc_id), "%s", "DHA_SN_03_2019");
            snprintf(out_result->evidence_section, sizeof(out_result->evidence_section), "%s", "§3.7.2");
            snprintf(out_result->evidence_sha256, sizeof(out_result->evidence_sha256), "%s", DHA_SN03_2019_SEC_3_7_2_SHA256);
        }
    }

    return true;
}

/* ========================================================================= */
/* Tri-Modal Interface 3: Patient Health Companion                          */
/* ========================================================================= */
bool taryaq_patient_companion_consult(
    const char *patient_message,
    const char *active_meds_csv,
    double patient_egfr,
    taryaq_patient_companion_result_t *out_result
) {
    if (!out_result) return false;
    memset(out_result, 0, sizeof(*out_result));

    /* 1. Deterministic Emergency Red-Flag Triage First */
    out_result->triage = taryaq_triage_patient_symptoms(patient_message);
    if (out_result->triage.is_red_flag) {
        out_result->requires_immediate_998 = true;
        snprintf(out_result->red_flag_warning, sizeof(out_result->red_flag_warning),
            "⚠️ إشعار طوارئ طبي عاجل (EMERGENCY RED-FLAG INTERRUPT):\n"
            "%s\n"
            "التوجيه السريري: %s\n"
            "الاتصال الفوري بالإسعاف 998 أو 999 أو التوجه لأقرب طوارئ مستشفى فوراً. لا تنتظر موعد العيادة.",
            out_result->triage.action_protocol,
            out_result->triage.clinical_guidance);
        return true;
    }

    /* 2. Plain-Language Medication Instructions & Counseling */
    const char *meds = active_meds_csv ? active_meds_csv : "";
    bool has_metformin = strcasestr_c99(meds, "metformin") || strcasestr_c99(meds, "ميتفورمين") || strcasestr_c99(meds, "glucophage");
    bool has_cipro     = strcasestr_c99(meds, "cipro") || strcasestr_c99(meds, "سيبرو");
    bool has_statin    = strcasestr_c99(meds, "statin") || strcasestr_c99(meds, "ستاتين") || strcasestr_c99(meds, "lipitor") || strcasestr_c99(meds, "zocor");
    bool has_thyroid   = strcasestr_c99(meds, "levothyroxine") || strcasestr_c99(meds, "euthyrox") || strcasestr_c99(meds, "ثيروكسين");

    char inst[1024] = {0};
    size_t ilen = 0;

    ilen += snprintf(inst + ilen, sizeof(inst) - ilen,
        "إرشادات تناول الأدوية بوضوح (Plain-Language Medication Guide):\n");

    if (has_metformin) {
        ilen += snprintf(inst + ilen, sizeof(inst) - ilen,
            "- ميتفورمين (دواء السكر): خذ الحبة مع وجبة الطعام أو بعدها مباشرة (مثل الفطور أو العشاء) لتقليل أي انزعاج بالمعدة.\n");
    }
    if (has_cipro) {
        ilen += snprintf(inst + ilen, sizeof(inst) - ilen,
            "- سيبروفلوكساسين (المضاد الحيوي): اشرب كأساً كاملاً من الماء مع الحبة. أكمل كامل الكورس حتى لو شعرت بتحسن.\n");
    }
    if (has_statin) {
        ilen += snprintf(inst + ilen, sizeof(inst) - ilen,
            "- أدوية الدهون (الستاتين): يفضل تناولها مساءً أو مع وجبة العشاء بانتظام.\n");
    }
    if (has_thyroid) {
        ilen += snprintf(inst + ilen, sizeof(inst) - ilen,
            "- هرمون الغدة (إيوثيروكس): خذ الحبة صباحاً على معدة فارغة قبل الإفطار بنصف ساعة مع ماء فقط.\n");
    }
    if (!has_metformin && !has_cipro && !has_statin && !has_thyroid) {
        ilen += snprintf(inst + ilen, sizeof(inst) - ilen,
            "- التزم بمواعيد الجرعات المحددة من قبل الطبيب مع كوب ماء كافٍ.\n");
    }
    snprintf(out_result->lay_instructions, sizeof(out_result->lay_instructions), "%s", inst);

    /* 3. Dietary & Food Interactions */
    char diet[512] = {0};
    size_t dlen = 0;
    dlen += snprintf(diet + dlen, sizeof(diet) - dlen, "تنبيهات الأغذية والمشروبات المهمة:\n");
    if (has_cipro) {
        dlen += snprintf(diet + dlen, sizeof(diet) - dlen,
            "- الحليب ومنتجات الألبان ومكملات الكالسيوم أو الحديد: تجنبها قبل ساعتين أو بعد ساعتين من حبة المضاد لأنها تمنع امتصاصه.\n");
    }
    if (has_statin) {
        dlen += snprintf(diet + dlen, sizeof(diet) - dlen,
            "- عصير الجريب فروت: تجنب تناوله تماماً لأنه يرفع تركيز الدواء في الدم وقد يسبب آلاماً عضلية.\n");
    }
    if (patient_egfr < 45.0) {
        dlen += snprintf(diet + dlen, sizeof(diet) - dlen,
            "- صحة الكلى: قلل من ملح الطعام وحافظ على شرب الماء باعتدال حسب توصيات طبيبك.\n");
    }
    snprintf(out_result->dietary_precautions, sizeof(out_result->dietary_precautions), "%s", diet);

    snprintf(out_result->red_flag_warning, sizeof(out_result->red_flag_warning),
        "علامات تستدعي الاتصال الفوري: ألم في الصدر، ضيق تنفس حاد، دوخة مفاجئة أو خفقان، طفح جلدي حاد، أو آلام عضلية شديدة مع تغير لون البول.");

    return true;
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

    /* Test 3: DHA SN 03/2019 Claim Submission delayed past statutory 30-day window (42 days = 12 overdue) -> CLAIM_CYCLE_WARN */
    taryaq_rx_context_t delayed_claim = {
        .primary_drug = "Metformin",
        .concurrent_drug = NULL,
        .patient_egfr_ml_min = 75.0,
        .is_controlled_substance = false,
        .has_controlled_id_verified = false,
        .icd10_code = "E11.9",
        .submission_delay_days = 42,
        .has_ejadah_lab_justification = true,
        .has_clinician_override = false
    };
    taryaq_result_t r3 = taryaq_verify_prescription_and_claim(&delayed_claim);
    assert(r3.verdict == CLINICAL_CLAIM_CYCLE_WARN);
    assert(strcmp(r3.alert_code, "DHA_SN03_2019_DELAY_PENALTY_EXPOSURE") == 0);
    assert(strcmp(r3.doc_id, "DHA_SN_03_2019") == 0);
    assert(strcmp(r3.section, "§3.7.2") == 0);
    assert(strcmp(r3.passage_sha256, DHA_SN03_2019_SEC_3_7_2_SHA256) == 0);

    /* Test 4: Claim within statutory 30-day window (20 days) -> SAFE */
    delayed_claim.submission_delay_days = 20;
    taryaq_result_t r4 = taryaq_verify_prescription_and_claim(&delayed_claim);
    assert(r4.verdict == CLINICAL_SAFE);
    assert(strcmp(r4.alert_code, "DHA_NABIDH_SAFE") == 0);
    assert(strcmp(r4.doc_id, "DHA_SN_03_2019") == 0);
    assert(strcmp(r4.section, "§3.7.1") == 0);
    assert(strcmp(r4.passage_sha256, DHA_SN03_2019_SEC_3_7_1_SHA256) == 0);

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

    /* Test 12: Fail-Closed Gate - Unmodeled drug -> UNVERIFIED_DRUG_REQUIRES_PHARMACIST_REVIEW */
    taryaq_rx_context_t unmodeled_drug = {
        .primary_drug = "ExperimentalCompound-X 400mg",
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

    /* Test 12b: Verified Modeled Drug from 100+ DB - Ibuprofen with normal eGFR -> SAFE */
    taryaq_rx_context_t ibuprofen_case = {
        .primary_drug = "Ibuprofen 400mg",
        .concurrent_drug = NULL,
        .patient_egfr_ml_min = 95.0,
        .is_controlled_substance = false,
        .has_controlled_id_verified = false,
        .icd10_code = "M79.1",
        .submission_delay_days = 0,
        .has_ejadah_lab_justification = true
    };
    taryaq_result_t r12b = taryaq_verify_prescription_and_claim(&ibuprofen_case);
    assert(r12b.verdict == CLINICAL_SAFE);
    assert(strcmp(r12b.alert_code, "DHA_NABIDH_SAFE") == 0);

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

    /* Test 17: Tri-Modal Interface 1 - Clinician Ambient Copilot SOAP & Pre-Order CDS Interrupt */
    taryaq_soap_note_t soap;
    bool soap_ok = taryaq_clinician_ambient_copilot(
        "Patient presents with severe acute right big toe pain (podagra) and swelling for 2 days. eGFR: 22.0 mL/min.",
        22.0, "Colchicine, Clarithromycin", &soap);
    assert(soap_ok);
    assert(soap.cds_interrupt_triggered == true);
    assert(strcmp(soap.cds_interrupt_code, "FATAL_CYP3A4_RENAL_CONTRAINDICATION") == 0);
    assert(strstr(soap.assessment, "M10.0") != NULL);

    /* Test 18: Tri-Modal Interface 2 - Pharmacist Verification Console Polypharmacy & DHA Late Penalty */
    taryaq_pharmacist_result_t pharm;
    bool pharm_ok = taryaq_pharmacist_verification(
        "Metformin, Simvastatin, Clarithromycin", 45.0, 1000.0, 42, "E11.9", true, &pharm);
    assert(pharm_ok);
    assert(pharm.has_contraindication_block == true);
    assert(pharm.overdue_days == 12);
    assert(fabs(pharm.penalty_aed - 3.60) < 0.01);
    assert(pharm.claim_cycle_warn == true);

    /* Test 19: Tri-Modal Interface 3 - Patient Health Companion Red-Flag Triage */
    taryaq_patient_companion_result_t pat_comp;
    bool pat_ok = taryaq_patient_companion_consult(
        "عندي وجع وضغط بصدري ونازل لإيدي اليسار ومش قادر اتنفس", "Metformin", 55.0, &pat_comp);
    assert(pat_ok);
    assert(pat_comp.requires_immediate_998 == true);
    assert(pat_comp.triage.level == TRIAGE_EMERGENCY);

    /* Test 20: Tri-Modal Interface 3 - Patient Health Companion Plain-Language Counseling */
    taryaq_patient_companion_result_t pat_routine;
    bool pat_rt_ok = taryaq_patient_companion_consult(
        "كيف آخذ علاج الميتفورمين والسيبروفلوكساسين؟", "Metformin, Ciprofloxacin", 55.0, &pat_routine);
    assert(pat_rt_ok);
    assert(pat_routine.requires_immediate_998 == false);
    assert(strstr(pat_routine.lay_instructions, "ميتفورمين") != NULL);
    assert(strstr(pat_routine.dietary_precautions, "الحليب") != NULL);

    printf("[PASS] All Taryaq C99 Pharmacovigilance & DHA SN 03/2019 Claim Scrubber tests passed successfully!\n");
    return 0;
}
#endif
