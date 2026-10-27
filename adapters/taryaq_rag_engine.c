/* taryaq_rag_engine.c - Deterministic C99 Clinical RAG & Grounding Engine
 * Target: Round 3 Clinical RAG | Dubai Create AI Agents Championship 2026-2027
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sqlite3.h>
#include <strings.h>

#include "taryaq_rag_engine.h"

static sqlite3 *g_rag_db = NULL;

/* Static fallback table of authoritative statutory clauses for fail-closed verification */
static const struct {
    const char *doc_id;
    const char *section;
    const char *title;
    const char *passage;
    const char *passage_sha256;
} S_STATIC_CLAUSES[] = {
    {
        "DHA_SN_03_2019",
        "§3.7.1",
        "DHA SN 03/2019 Initial Claim Submission Deadline",
        "Healthcare providers licensed in the Emirate of Dubai must submit all initial outpatient claims to the Dubai Health Post Office (eClaimLink / DHA) within the statutory 30-day window or by the 15th calendar day of the month following the date of service. Initial claims submitted within this window are processed without statutory penalty.",
        DHA_SN03_2019_SEC_3_7_1_SHA256
    },
    {
        "DHA_SN_03_2019",
        "§3.7.2",
        "DHA SN 03/2019 Initial Submission Delay Penalty Exposure",
        "Healthcare providers must submit all outpatient claims to the Dubai Health Post Office (eClaimLink / DHA) within the statutory 30-day window. Claims submitted past the statutory 30-day window from the date of service are subject to a statutory delay penalty of 0.03% of the Net Claimed amount per calendar day of delay calculated as max(0, submission_days - 30). Late submission penalties are deducted from payer remittances.",
        DHA_SN03_2019_SEC_3_7_2_SHA256
    },
    {
        "DHA_SN_03_2019",
        "§3.8.1",
        "Resubmission Timelines",
        "Healthcare providers may resubmit rejected or partially paid claims within a statutory window of 21 calendar days from the date of Remittance Advice issuance. Resubmissions must include supporting clinical documentation and corrective coding justifications.",
        DHA_SN03_2019_SEC_3_8_1_SHA256
    },
    {
        "DHA_SN_03_2019",
        "§3.8.4",
        "Resubmission Delay Penalty Exposure",
        "Resubmissions uploaded past the statutory 21-calendar-day window are subject to a statutory delay penalty of 0.03% of the Net Claimed amount per calendar day of delay beyond day 21, deducted from the provider payout.",
        DHA_SN03_2019_SEC_3_8_4_SHA256
    },
    {
        "DHA_SN_03_2019",
        "§3.9.3",
        "Payer Remittance Settlement Delay Fee",
        "Insurers and Third-Party Administrators (TPAs) operating in the Emirate of Dubai must settle clean claims within statutory timelines. Unjustified delay in remittance settlement past statutory deadlines incurs a mandatory delay fee of 0.03% per calendar day payable to the healthcare provider.",
        DHA_SN03_2019_SEC_3_9_3_SHA256
    },
    {
        "DRUG_COLCHICINE_MONOGRAPH",
        "COLCHICINE_MONOGRAPH",
        "Colchicine Pharmacokinetics and CYP3A4/P-gp Interactions",
        "Colchicine is an alkaloid used for acute gout flares and familial Mediterranean fever. It is heavily metabolized by hepatic CYP3A4 and excreted via renal P-glycoprotein (P-gp) transport. Co-administration of potent dual CYP3A4 and P-gp inhibitors such as Clarithromycin in patients with renal impairment (eGFR < 60 mL/min) is strictly contraindicated due to rapid accumulation leading to fatal multiorgan failure, severe neuromyopathy, and bone marrow aplasia (FDA Boxed Warning).",
        DRUG_COLCHICINE_SHA256
    },
    {
        "DRUG_CLARITHROMYCIN_MONOGRAPH",
        "CLARITHROMYCIN_MONOGRAPH",
        "Clarithromycin Macrolide Safety and Interaction Profile",
        "Clarithromycin is a macrolide antibiotic and potent mechanism-based inhibitor of CYP3A4 and P-glycoprotein. Concomitant administration with Colchicine under renal impairment is contraindicated. Concomitant administration with Simvastatin is strictly contraindicated due to profound elevation of statin AUC (>10-fold) precipitating acute rhabdomyolysis and myoglobinuric acute kidney injury.",
        DRUG_CLARITHROMYCIN_SHA256
    },
    {
        "DRUG_METFORMIN_MONOGRAPH",
        "METFORMIN_MONOGRAPH",
        "Metformin Renal Elimination and Lactic Acidosis Thresholds",
        "Metformin is a biguanide antihyperglycemic agent eliminated unchanged via renal tubular secretion. In severe renal impairment (eGFR < 30 mL/min), metformin accumulation triggers life-threatening Metformin-Associated Lactic Acidosis (MALA), carrying a mortality rate exceeding 40%. Metformin is contraindicated at eGFR < 30 mL/min. In moderate CKD Stage 3b (eGFR 30 to 44 mL/min), dosage must be reduced to a maximum ceiling of 1000 mg daily with quarterly renal monitoring (KDIGO 2024).",
        DRUG_METFORMIN_SHA256
    },
    {
        "DRUG_SIMVASTATIN_MONOGRAPH",
        "SIMVASTATIN_MONOGRAPH",
        "Simvastatin Hepatic Metabolism and CYP3A4 Ceiling Rules",
        "Simvastatin is an HMG-CoA reductase inhibitor substrate of CYP3A4. Strong CYP3A4 inhibitors like clarithromycin, ketoconazole, and itraconazole are contraindicated due to severe rhabdomyolysis risk. When co-prescribed with moderate CYP3A4 inhibitors or calcium channel blockers like amlodipine, the maximum permissible daily dose of simvastatin is capped at 20 mg daily.",
        DRUG_SIMVASTATIN_SHA256
    },
    {
        "DRUG_WARFARIN_MONOGRAPH",
        "WARFARIN_MONOGRAPH",
        "Warfarin Anticoagulation and CYP2C9/3A4 Potentiation",
        "Warfarin is a narrow therapeutic index oral anticoagulant existing as a racemic mixture. S-warfarin is metabolized by CYP2C9 while R-warfarin is cleared by CYP3A4 and CYP1A2. Concomitant administration with Amiodarone severely inhibits warfarin clearance, precipitating supra-therapeutic INR and fatal hemorrhagic complications; dose reduction and strict INR monitoring protocols are mandatory.",
        DRUG_WARFARIN_SHA256
    },
    {
        "DRUG_AMIODARONE_MONOGRAPH",
        "AMIODARONE_MONOGRAPH",
        "Amiodarone Class III Antiarrhythmic Multitarget Inhibition",
        "Amiodarone is a multi-channel antiarrhythmic agent and potent inhibitor of CYP2C9, CYP2D6, CYP3A4, and P-glycoprotein. Due to its exceptionally long elimination half-life (up to 60 days), potentiation of warfarin anticoagulant effect persists for months following discontinuation.",
        DRUG_AMIODARONE_SHA256
    },
    {
        "DRUG_PARACETAMOL_MONOGRAPH",
        "PARACETAMOL_MONOGRAPH",
        "Paracetamol Analgesic Safety and Dosing Guidelines",
        "Paracetamol (acetaminophen) is a non-opioid analgesic and antipyretic indicated for mild-to-moderate pain. Standard oral adult dosing is 500 to 1000 mg every 4 to 6 hours up to a maximum of 4000 mg in 24 hours. Concomitant administration with standard aminopenicillins like Amoxicillin is clinically verified safe with zero pharmacokinetic contraindication.",
        DRUG_PARACETAMOL_SHA256
    },
    {
        "DRUG_AMOXICILLIN_MONOGRAPH",
        "AMOXICILLIN_MONOGRAPH",
        "Amoxicillin Aminopenicillin Spectrum and Renal Safety",
        "Amoxicillin is an extended-spectrum aminopenicillin indicated for respiratory, otolaryngologic, and dermatologic bacterial infections. It is cleared primarily through renal glomerular filtration and tubular secretion. Concomitant administration with Paracetamol represents a verified, safe standard outpatient therapeutic regimen.",
        DRUG_AMOXICILLIN_SHA256
    },
    {
        "DRUG_TRAMADOL_MONOGRAPH",
        "TRAMADOL_MONOGRAPH",
        "Tramadol Synthetic Opioid and Controlled Classification",
        "Tramadol is a centrally acting synthetic opioid analgesic and weak serotonin-norepinephrine reuptake inhibitor. Due to dependency, abuse liability, and respiratory depression risks, tramadol is scheduled as a controlled substance under UAE Federal Law No. 30 of 2021. Dispensing requires verified Emirates ID, daily dosage ceiling checks, and pharmacist co-signature.",
        DRUG_TRAMADOL_SHA256
    },
    {
        "UAE_LAW_30_2021",
        "SCHEDULE_IV_CONTROLLED",
        "Controlled and Semi-Controlled Substance Dispensing Protocol",
        "All narcotics, psychotropics, and controlled medications (including Morphine, Tramadol, Fentanyl, Oxycodone, Ketamine, Buprenorphine, Methadone, Diazepam, Lorazepam, and Alprazolam) are governed by strict dispensing protocols. Outpatient dispensing requires verified patient Emirates ID or passport, electronic registration in the sovereign prescription monitoring system, adherence to authorized maximum posology ceilings, and physical or biometric co-signature of the licensed dispensing clinical pharmacist. Prescriptions for Schedule IV substances expire within three (3) calendar days of issuance.",
        UAE_LAW_30_2021_SHA256
    },
    { NULL, NULL, NULL, NULL, NULL }
};

bool taryaq_rag_init(const char *db_path) {
    if (g_rag_db) {
        return true;
    }

    const char *paths[] = {
        db_path,
        DEFAULT_CORPUS_DB_PATH,
        "corpus/taryaq_corpus.db",
        "../corpus/taryaq_corpus.db",
        NULL
    };

    for (int i = 0; paths[i] != NULL; ++i) {
        if (!paths[i] || paths[i][0] == '\0') continue;
        int rc = sqlite3_open_v2(paths[i], &g_rag_db, SQLITE_OPEN_READONLY, NULL);
        if (rc == SQLITE_OK && g_rag_db) {
            return true;
        }
        if (g_rag_db) {
            sqlite3_close(g_rag_db);
            g_rag_db = NULL;
        }
    }

    /* Fallback to in-memory populated database if external file is inaccessible */
    int rc = sqlite3_open(":memory:", &g_rag_db);
    if (rc != SQLITE_OK || !g_rag_db) {
        return false;
    }

    sqlite3_exec(g_rag_db,
        "CREATE TABLE corpus_chunks (chunk_id TEXT PRIMARY KEY, doc_id TEXT, doc_title TEXT, section TEXT, title TEXT, passage TEXT, passage_sha256 TEXT);"
        "CREATE VIRTUAL TABLE corpus_fts USING fts5(doc_id UNINDEXED, section UNINDEXED, title, passage, tokenize='porter');",
        NULL, NULL, NULL);

    sqlite3_stmt *stmt_chunk, *stmt_fts;
    sqlite3_prepare_v2(g_rag_db, "INSERT INTO corpus_chunks VALUES (?, ?, ?, ?, ?, ?, ?);", -1, &stmt_chunk, NULL);
    sqlite3_prepare_v2(g_rag_db, "INSERT INTO corpus_fts VALUES (?, ?, ?, ?);", -1, &stmt_fts, NULL);

    for (int i = 0; S_STATIC_CLAUSES[i].doc_id != NULL; ++i) {
        char cid[128];
        snprintf(cid, sizeof(cid), "%s_%s", S_STATIC_CLAUSES[i].doc_id, S_STATIC_CLAUSES[i].section);

        sqlite3_bind_text(stmt_chunk, 1, cid, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt_chunk, 2, S_STATIC_CLAUSES[i].doc_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt_chunk, 3, S_STATIC_CLAUSES[i].doc_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt_chunk, 4, S_STATIC_CLAUSES[i].section, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt_chunk, 5, S_STATIC_CLAUSES[i].title, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt_chunk, 6, S_STATIC_CLAUSES[i].passage, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt_chunk, 7, S_STATIC_CLAUSES[i].passage_sha256, -1, SQLITE_STATIC);
        sqlite3_step(stmt_chunk);
        sqlite3_reset(stmt_chunk);

        sqlite3_bind_text(stmt_fts, 1, S_STATIC_CLAUSES[i].doc_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt_fts, 2, S_STATIC_CLAUSES[i].section, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt_fts, 3, S_STATIC_CLAUSES[i].title, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt_fts, 4, S_STATIC_CLAUSES[i].passage, -1, SQLITE_STATIC);
        sqlite3_step(stmt_fts);
        sqlite3_reset(stmt_fts);
    }

    sqlite3_finalize(stmt_chunk);
    sqlite3_finalize(stmt_fts);

    return true;
}

static void sanitize_fts_query(const char *raw, char *out, size_t out_cap) {
    if (!raw || !out || out_cap == 0) return;
    out[0] = '\0';

    char buf[512];
    size_t bi = 0;
    for (size_t i = 0; raw[i] != '\0' && bi < sizeof(buf) - 1; ++i) {
        if (isalnum((unsigned char)raw[i]) || raw[i] == ' ') {
            buf[bi++] = raw[i];
        } else {
            buf[bi++] = ' ';
        }
    }
    buf[bi] = '\0';

    size_t written = 0;
    char *saveptr = NULL;
    char *token = strtok_r(buf, " \t\r\n", &saveptr);
    while (token != NULL) {
        /* Skip very short stop words */
        if (strlen(token) >= 3) {
            int n = snprintf(out + written, out_cap - written,
                             "%s\"%s\"", (written > 0 ? " OR " : ""), token);
            if (n > 0 && (size_t)n < out_cap - written) {
                written += (size_t)n;
            } else {
                break;
            }
        }
        token = strtok_r(NULL, " \t\r\n", &saveptr);
    }

    if (written == 0) {
        snprintf(out, out_cap, "\"DHA\"");
    }
}

size_t taryaq_rag_search(const char *query, taryaq_rag_hit_t *hits, size_t max_hits) {
    if (!query || !hits || max_hits == 0) return 0;
    if (!g_rag_db && !taryaq_rag_init(NULL)) return 0;

    char fts_query[512];
    sanitize_fts_query(query, fts_query, sizeof(fts_query));

    const char *sql =
        "SELECT c.chunk_id, c.doc_id, c.section, c.title, c.passage, c.passage_sha256, bm25(corpus_fts) "
        "FROM corpus_fts f "
        "JOIN corpus_chunks c ON c.doc_id = f.doc_id AND c.section = f.section "
        "WHERE corpus_fts MATCH ? "
        "ORDER BY bm25(corpus_fts) ASC "
        "LIMIT ?;";

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(g_rag_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        return 0;
    }

    sqlite3_bind_text(stmt, 1, fts_query, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, (int)max_hits);

    size_t count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_hits) {
        const char *cid   = (const char *)sqlite3_column_text(stmt, 0);
        const char *did   = (const char *)sqlite3_column_text(stmt, 1);
        const char *sec   = (const char *)sqlite3_column_text(stmt, 2);
        const char *tit   = (const char *)sqlite3_column_text(stmt, 3);
        const char *pas   = (const char *)sqlite3_column_text(stmt, 4);
        const char *hsh   = (const char *)sqlite3_column_text(stmt, 5);
        double      score = sqlite3_column_double(stmt, 6);

        snprintf(hits[count].chunk_id, sizeof(hits[count].chunk_id), "%s", cid ? cid : "");
        snprintf(hits[count].doc_id, sizeof(hits[count].doc_id), "%s", did ? did : "");
        snprintf(hits[count].section, sizeof(hits[count].section), "%s", sec ? sec : "");
        snprintf(hits[count].title, sizeof(hits[count].title), "%s", tit ? tit : "");
        snprintf(hits[count].passage, sizeof(hits[count].passage), "%s", pas ? pas : "");
        snprintf(hits[count].passage_sha256, sizeof(hits[count].passage_sha256), "%s", hsh ? hsh : "");
        hits[count].score = score;

        count++;
    }

    sqlite3_finalize(stmt);
    return count;
}

bool taryaq_rag_lookup_clause(const char *doc_id, const char *section, taryaq_rag_hit_t *out_hit) {
    if (!doc_id || !section || !out_hit) return false;
    memset(out_hit, 0, sizeof(*out_hit));

    if (g_rag_db || taryaq_rag_init(NULL)) {
        const char *sql =
            "SELECT chunk_id, doc_id, section, title, passage, passage_sha256 "
            "FROM corpus_chunks WHERE doc_id = ? AND section = ? LIMIT 1;";
        sqlite3_stmt *stmt = NULL;
        if (sqlite3_prepare_v2(g_rag_db, sql, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, doc_id, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, section, -1, SQLITE_STATIC);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const char *cid = (const char *)sqlite3_column_text(stmt, 0);
                const char *did = (const char *)sqlite3_column_text(stmt, 1);
                const char *sec = (const char *)sqlite3_column_text(stmt, 2);
                const char *tit = (const char *)sqlite3_column_text(stmt, 3);
                const char *pas = (const char *)sqlite3_column_text(stmt, 4);
                const char *hsh = (const char *)sqlite3_column_text(stmt, 5);

                snprintf(out_hit->chunk_id, sizeof(out_hit->chunk_id), "%s", cid ? cid : "");
                snprintf(out_hit->doc_id, sizeof(out_hit->doc_id), "%s", did ? did : "");
                snprintf(out_hit->section, sizeof(out_hit->section), "%s", sec ? sec : "");
                snprintf(out_hit->title, sizeof(out_hit->title), "%s", tit ? tit : "");
                snprintf(out_hit->passage, sizeof(out_hit->passage), "%s", pas ? pas : "");
                snprintf(out_hit->passage_sha256, sizeof(out_hit->passage_sha256), "%s", hsh ? hsh : "");
                sqlite3_finalize(stmt);
                return true;
            }
            sqlite3_finalize(stmt);
        }
    }

    /* Fallback lookup in static table */
    for (int i = 0; S_STATIC_CLAUSES[i].doc_id != NULL; ++i) {
        if (strcasecmp(S_STATIC_CLAUSES[i].doc_id, doc_id) == 0 &&
            strcasecmp(S_STATIC_CLAUSES[i].section, section) == 0) {
            snprintf(out_hit->doc_id, sizeof(out_hit->doc_id), "%s", S_STATIC_CLAUSES[i].doc_id);
            snprintf(out_hit->section, sizeof(out_hit->section), "%s", S_STATIC_CLAUSES[i].section);
            snprintf(out_hit->title, sizeof(out_hit->title), "%s", S_STATIC_CLAUSES[i].title);
            snprintf(out_hit->passage, sizeof(out_hit->passage), "%s", S_STATIC_CLAUSES[i].passage);
            snprintf(out_hit->passage_sha256, sizeof(out_hit->passage_sha256), "%s", S_STATIC_CLAUSES[i].passage_sha256);
            return true;
        }
    }

    return false;
}

bool taryaq_rag_verify_passage_hash(const char *doc_id, const char *section, const char *passage_sha256) {
    if (!doc_id || !section || !passage_sha256) return false;

    taryaq_rag_hit_t hit;
    if (taryaq_rag_lookup_clause(doc_id, section, &hit)) {
        return (strcasecmp(hit.passage_sha256, passage_sha256) == 0);
    }

    return false;
}

void taryaq_rag_close(void) {
    if (g_rag_db) {
        sqlite3_close(g_rag_db);
        g_rag_db = NULL;
    }
}
