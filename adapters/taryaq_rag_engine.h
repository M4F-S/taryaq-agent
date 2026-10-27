/* taryaq_rag_engine.h - Deterministic C99 Clinical RAG & Grounding Engine
 * Target: Round 3 Clinical RAG | Dubai Create AI Agents Championship 2026-2027
 */
#ifndef TARYAQ_RAG_ENGINE_H
#define TARYAQ_RAG_ENGINE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_CORPUS_DB_PATH "corpus/taryaq_corpus.db"

/* Known Statutory SHA-256 Hashes for Verification */
#define DHA_SN03_2019_SEC_3_7_1_SHA256 "b0a422ad0e2edc66290007caaa5508d4ec4ac4229453b170881fb482449218f3"
#define DHA_SN03_2019_SEC_3_7_2_SHA256 "35bd39f8756297aaf921e2e2720f30c7e9ceac5f524646d40fd0601298089a5a"
#define DHA_SN03_2019_SEC_3_8_1_SHA256 "4c01fbba6a5b0d4e49f878501cf6606466e18fe6ea1671d455b728bfa8cd0331"
#define DHA_SN03_2019_SEC_3_8_4_SHA256 "f3116079c47127f44dd87694a901a495ad0b0e4423989fa55f836b638750a4b8"
#define DHA_SN03_2019_SEC_3_9_3_SHA256 "dedc5aa6493c4fa56a40e596ebf0d0a3024ca6f4269f512dd7192557167dd32f"

#define DRUG_COLCHICINE_SHA256          "8bab2c0a6b1b286e3b4979cd1afe2734fa376c889234ca85c314db1becbeb3f3"
#define DRUG_CLARITHROMYCIN_SHA256      "a98515f6744d9120881b57ae7228b6cfe21b7d5349570feaebaa06c1ae5174c0"
#define DRUG_METFORMIN_SHA256           "9fda4e8fee4a0b07e56318c2e7713cdb8998b75cac2d2993e201500ae67f295d"
#define DRUG_SIMVASTATIN_SHA256         "02994de0d776fcf365fbce8f8b190e037a5a4bc8bbe6a961e0af5dd72a3eb329"
#define DRUG_WARFARIN_SHA256            "5be86800f5e901b3ab6cdcc99c164636b3cb78ae35ac13ac5bc65c0c1bd88994"
#define DRUG_AMIODARONE_SHA256          "2862a1d09beb9677c182db4aa211cbf4edc60fc1b06e93884b3fc28fe92ee0b4"
#define DRUG_PARACETAMOL_SHA256         "40b9f1b7e3ba11ac9e14fede9388d55d2c2fe8fc811b4388db4e75f3604ac68d"
#define DRUG_AMOXICILLIN_SHA256         "a9317dcc21bf69ddbbc2bbf7c3499cf8e59a4380d0f9d79a12a6ae9f8695bca7"
#define DRUG_TRAMADOL_SHA256            "a15f29a4ff2e493d715e7cade52d623b9679f1ade40c4d7359efc2a72bd2e4f6"
#define UAE_LAW_30_2021_SHA256          "e0d15014d33abf8dfd4e142648f7f470a515444d7c237fee4c198b434c0978a0"

typedef struct {
    char   chunk_id[64];
    char   doc_id[64];
    char   section[64];
    char   title[128];
    char   passage[1024];
    char   passage_sha256[65];
    double score; /* BM25 rank score */
} taryaq_rag_hit_t;

/* Initialize or open the SQLite FTS5 corpus database */
bool taryaq_rag_init(const char *db_path);

/* Query corpus for top-k matching passages using SQLite FTS5 (BM25) */
size_t taryaq_rag_search(
    const char       *query,
    taryaq_rag_hit_t *hits,
    size_t            max_hits
);

/* Retrieve exact statutory clause by doc_id and section */
bool taryaq_rag_lookup_clause(
    const char       *doc_id,
    const char       *section,
    taryaq_rag_hit_t *out_hit
);

/* Verify that a passage SHA-256 checksum matches the authoritative corpus */
bool taryaq_rag_verify_passage_hash(
    const char *doc_id,
    const char *section,
    const char *passage_sha256
);

/* Close the RAG engine database handle */
void taryaq_rag_close(void);

#ifdef __cplusplus
}
#endif

#endif /* TARYAQ_RAG_ENGINE_H */
