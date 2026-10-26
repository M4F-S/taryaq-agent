/* src/taryaq_agent.c - Sovereign Clinical Safety & DHA Reimbursement Firewall
 * Dubai Create AI Agents Championship 2026-2027 | Best Societal AI Agent
 * Conforms strictly to ISO C99 (-std=c99 -Wall -Wextra -Werror -O2)
 */

#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE 1
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE 1
#endif

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#include <curl/curl.h>

#include "taryaq_adapters.h"
#include "nibras_adapters.h"
#include "grounding_shield.h"
#include "qala_adapters.h"

#define TARYAQ_VERSION "0.1.0-c99-sovereign"
#define DEFAULT_AUDIT_LOG "taryaq_audit.jsonl"
#define TELEMETRY_ENDPOINT "http://127.0.0.1:8787/emit"

/* ========================================================================= */
/* 1. Self-Contained ISO C99 SHA-256 Implementation (Zero External Crypto)   */
/* ========================================================================= */

typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t  buffer[64];
} taryaq_sha256_t;

static uint32_t ror32(uint32_t val, uint32_t bits) {
    return (val >> bits) | (val << (32 - bits));
}

static void sha256_init(taryaq_sha256_t *ctx) {
    ctx->state[0] = 0x6a09e667UL;
    ctx->state[1] = 0xbb67ae85UL;
    ctx->state[2] = 0x3c6ef372UL;
    ctx->state[3] = 0xa54ff53aUL;
    ctx->state[4] = 0x510e527fUL;
    ctx->state[5] = 0x9b05688cUL;
    ctx->state[6] = 0x1f83d9abUL;
    ctx->state[7] = 0x5be0cd19UL;
    ctx->count = 0;
}

static const uint32_t K256[64] = {
    0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL,
    0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
    0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL,
    0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
    0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
    0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
    0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL,
    0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
    0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL,
    0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
    0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL,
    0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
    0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL,
    0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
    0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
    0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
};

static void sha256_transform(uint32_t state[8], const uint8_t data[64]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
    uint32_t w[64];
    for (int i = 0; i < 16; ++i) {
        w[i] = ((uint32_t)data[i * 4] << 24) |
               ((uint32_t)data[i * 4 + 1] << 16) |
               ((uint32_t)data[i * 4 + 2] << 8) |
               ((uint32_t)data[i * 4 + 3]);
    }
    for (int i = 16; i < 64; ++i) {
        uint32_t s0 = ror32(w[i - 15], 7) ^ ror32(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = ror32(w[i - 2], 17) ^ ror32(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    for (int i = 0; i < 64; ++i) {
        uint32_t S1 = ror32(e, 6) ^ ror32(e, 11) ^ ror32(e, 25);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t temp1 = h + S1 + ch + K256[i] + w[i];
        uint32_t S0 = ror32(a, 2) ^ ror32(a, 13) ^ ror32(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = S0 + maj;
        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

static void sha256_update(taryaq_sha256_t *ctx, const uint8_t *data, size_t len) {
    size_t i = 0;
    size_t buf_idx = (size_t)(ctx->count & 0x3F);
    ctx->count += len;
    if (buf_idx > 0) {
        size_t fill = 64 - buf_idx;
        if (len < fill) {
            memcpy(ctx->buffer + buf_idx, data, len);
            return;
        }
        memcpy(ctx->buffer + buf_idx, data, fill);
        sha256_transform(ctx->state, ctx->buffer);
        i += fill;
    }
    for (; i + 64 <= len; i += 64) {
        sha256_transform(ctx->state, data + i);
    }
    if (i < len) {
        memcpy(ctx->buffer, data + i, len - i);
    }
}

static void sha256_final(taryaq_sha256_t *ctx, uint8_t hash[32]) {
    uint64_t total_bits = ctx->count * 8;
    size_t buf_idx = (size_t)(ctx->count & 0x3F);
    ctx->buffer[buf_idx++] = 0x80;
    if (buf_idx > 56) {
        memset(ctx->buffer + buf_idx, 0, 64 - buf_idx);
        sha256_transform(ctx->state, ctx->buffer);
        buf_idx = 0;
    }
    memset(ctx->buffer + buf_idx, 0, 56 - buf_idx);
    for (int i = 0; i < 8; ++i) {
        ctx->buffer[56 + i] = (uint8_t)(total_bits >> ((7 - i) * 8));
    }
    sha256_transform(ctx->state, ctx->buffer);
    for (int i = 0; i < 8; ++i) {
        hash[i * 4]     = (uint8_t)(ctx->state[i] >> 24);
        hash[i * 4 + 1] = (uint8_t)(ctx->state[i] >> 16);
        hash[i * 4 + 2] = (uint8_t)(ctx->state[i] >> 8);
        hash[i * 4 + 3] = (uint8_t)(ctx->state[i]);
    }
}

static void sha256_hex(const char *data, size_t len, char hex_out[65]) {
    taryaq_sha256_t ctx;
    uint8_t hash[32];
    sha256_init(&ctx);
    sha256_update(&ctx, (const uint8_t *)data, len);
    sha256_final(&ctx, hash);
    for (int i = 0; i < 32; ++i) {
        snprintf(hex_out + (i * 2), 3, "%02x", hash[i]);
    }
    hex_out[64] = '\0';
}

static void hmac_sha256(
    const uint8_t *key, size_t key_len,
    const uint8_t *data, size_t data_len,
    uint8_t mac_out[32]
) {
    uint8_t k0[64];
    memset(k0, 0, sizeof(k0));

    if (key_len > 64) {
        taryaq_sha256_t k_ctx;
        sha256_init(&k_ctx);
        sha256_update(&k_ctx, key, key_len);
        sha256_final(&k_ctx, k0);
    } else {
        memcpy(k0, key, key_len);
    }

    uint8_t k_ipad[64];
    uint8_t k_opad[64];
    for (int i = 0; i < 64; ++i) {
        k_ipad[i] = k0[i] ^ 0x36;
        k_opad[i] = k0[i] ^ 0x5c;
    }

    /* Inner hash: H(K_ipad || data) */
    taryaq_sha256_t in_ctx;
    uint8_t inner_hash[32];
    sha256_init(&in_ctx);
    sha256_update(&in_ctx, k_ipad, 64);
    sha256_update(&in_ctx, data, data_len);
    sha256_final(&in_ctx, inner_hash);

    /* Outer hash: H(K_opad || inner_hash) */
    taryaq_sha256_t out_ctx;
    sha256_init(&out_ctx);
    sha256_update(&out_ctx, k_opad, 64);
    sha256_update(&out_ctx, inner_hash, 32);
    sha256_final(&out_ctx, mac_out);
}

static void hmac_sha256_hex(
    const char *key, size_t key_len,
    const char *data, size_t data_len,
    char hex_out[65]
) {
    uint8_t mac[32];
    hmac_sha256((const uint8_t *)key, key_len, (const uint8_t *)data, data_len, mac);
    for (int i = 0; i < 32; ++i) {
        snprintf(hex_out + (i * 2), 3, "%02x", mac[i]);
    }
    hex_out[64] = '\0';
}

static bool taryaq_get_prev_mac(int fd, char prev_mac_out[65]) {
    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size < 0) {
        return false;
    }
    if (file_size == 0) {
        memset(prev_mac_out, '0', 64);
        prev_mac_out[64] = '\0';
        return true;
    }

    size_t to_read = (file_size > 4096) ? 4096 : (size_t)file_size;
    off_t seek_pos = file_size - (off_t)to_read;
    if (lseek(fd, seek_pos, SEEK_SET) < 0) {
        return false;
    }

    char buf[4097];
    ssize_t bytes_read = read(fd, buf, to_read);
    if (bytes_read <= 0) {
        return false;
    }
    buf[bytes_read] = '\0';

    char *p = buf + bytes_read - 1;
    while (p >= buf && (*p == '\n' || *p == '\r' || isspace((unsigned char)*p))) {
        *p = '\0';
        p--;
    }
    if (p < buf) {
        memset(prev_mac_out, '0', 64);
        prev_mac_out[64] = '\0';
        return true;
    }

    char *last_line = p;
    while (last_line > buf && *(last_line - 1) != '\n' && *(last_line - 1) != '\r') {
        last_line--;
    }

    char *mac_pos = strstr(last_line, "\"mac\":\"");
    if (!mac_pos) {
        mac_pos = strstr(last_line, "\"mac\": \"");
        if (mac_pos) mac_pos += 8;
    } else {
        mac_pos += 7;
    }

    if (!mac_pos || strlen(mac_pos) < 64) {
        return false;
    }

    for (int i = 0; i < 64; ++i) {
        if (!isxdigit((unsigned char)mac_pos[i])) {
            return false;
        }
        prev_mac_out[i] = (char)tolower((unsigned char)mac_pos[i]);
    }
    prev_mac_out[64] = '\0';
    return true;
}

/* ========================================================================= */
/* 2. String & JSON Utilities                                                */
/* ========================================================================= */

static void taryaq_json_escape(const char *src, char *dst, size_t dst_cap) {
    if (!dst || dst_cap == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    size_t w = 0;
    for (size_t r = 0; src[r] != '\0' && (w + 2) < dst_cap; ++r) {
        unsigned char c = (unsigned char)src[r];
        if (c == '"' || c == '\\') {
            if (w + 3 >= dst_cap) break;
            dst[w++] = '\\';
            dst[w++] = (char)c;
        } else if (c == '\n') {
            if (w + 3 >= dst_cap) break;
            dst[w++] = '\\';
            dst[w++] = 'n';
        } else if (c == '\r') {
            if (w + 3 >= dst_cap) break;
            dst[w++] = '\\';
            dst[w++] = 'r';
        } else if (c >= 0x20) {
            dst[w++] = (char)c;
        }
    }
    dst[w] = '\0';
}

static uint64_t taryaq_now_utc(void) {
    return (uint64_t)time(NULL);
}

static double taryaq_get_monotonic_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((double)ts.tv_sec * 1000.0) + ((double)ts.tv_nsec / 1000000.0);
}

/* ========================================================================= */
/* 3. Optional Telemetry Bridge Client (libcurl, Graceful Silent Fallback)   */
/* ========================================================================= */

static void taryaq_emit_telemetry(bool enabled, const char *event_type, const char *json_payload) {
    if (!enabled || !json_payload) return;

    CURL *curl = curl_easy_init();
    if (!curl) return;

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, TELEMETRY_ENDPOINT);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_payload);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 150L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 100L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    FILE *devnull = fopen("/dev/null", "w");
    if (devnull) {
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, devnull);
    }

    CURLcode res = curl_easy_perform(curl);
    (void)res;
    (void)event_type;

    if (devnull) fclose(devnull);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
}

/* ========================================================================= */
/* 4. Local Embedded Medication Graph                                        */
/* ========================================================================= */

typedef enum {
    CYP3A4_NONE      = 0,
    CYP3A4_SUBSTRATE = 1,
    CYP3A4_INHIBITOR = 2
} cyp3a4_profile_t;

typedef struct {
    const char       *canonical_name;
    const char       *arabic_name;
    const char       *drug_class;
    double            renal_egfr_cutoff;
    cyp3a4_profile_t  cyp3a4_profile;
    bool              is_controlled;
    const char       *icd10_indication;
    const char       *clinical_notes;
} taryaq_med_entry_t;

static const taryaq_med_entry_t MED_DATABASE[] = {
    {
        .canonical_name = "Colchicine",
        .arabic_name = "كولشيسين",
        .drug_class = "Alkaloid / Anti-gout",
        .renal_egfr_cutoff = 30.0,
        .cyp3a4_profile = CYP3A4_SUBSTRATE,
        .is_controlled = false,
        .icd10_indication = "M10.0",
        .clinical_notes = "Fatal toxicity with CYP3A4 inhibitors in severe renal impairment (eGFR < 30)"
    },
    {
        .canonical_name = "Clarithromycin",
        .arabic_name = "كلاريثروميسين",
        .drug_class = "Macrolide Antibiotic",
        .renal_egfr_cutoff = 30.0,
        .cyp3a4_profile = CYP3A4_INHIBITOR,
        .is_controlled = false,
        .icd10_indication = "J02.9",
        .clinical_notes = "Potent CYP3A4 inhibitor; contra-indicated with colchicine in renal dysfunction"
    },
    {
        .canonical_name = "Morphine",
        .arabic_name = "مورفين",
        .drug_class = "Opioid Analgesic",
        .renal_egfr_cutoff = 20.0,
        .cyp3a4_profile = CYP3A4_NONE,
        .is_controlled = true,
        .icd10_indication = "R52",
        .clinical_notes = "Controlled substance under UAE Law No. 2 of 2019; mandatory Emirates ID verification"
    },
    {
        .canonical_name = "Metformin",
        .arabic_name = "ميتفورمين",
        .drug_class = "Biguanide Antidiabetic",
        .renal_egfr_cutoff = 30.0,
        .cyp3a4_profile = CYP3A4_NONE,
        .is_controlled = false,
        .icd10_indication = "E11.9",
        .clinical_notes = "Risk of lactic acidosis if eGFR < 30 ml/min"
    },
    {
        .canonical_name = "Ketamine",
        .arabic_name = "كيتامين",
        .drug_class = "NMDA Receptor Antagonist / Anesthetic",
        .renal_egfr_cutoff = 15.0,
        .cyp3a4_profile = CYP3A4_SUBSTRATE,
        .is_controlled = true,
        .icd10_indication = "T88.8",
        .clinical_notes = "Controlled schedule medication under UAE Law No. 2 of 2019"
    },
    {
        .canonical_name = "Tramadol",
        .arabic_name = "ترامادول",
        .drug_class = "Synthetic Opioid Analgesic",
        .renal_egfr_cutoff = 30.0,
        .cyp3a4_profile = CYP3A4_SUBSTRATE,
        .is_controlled = true,
        .icd10_indication = "M79.3",
        .clinical_notes = "Controlled substance in UAE; requires verified patient Emirates ID"
    },
    {
        .canonical_name = "Simvastatin",
        .arabic_name = "سيمفاستاتين",
        .drug_class = "HMG-CoA Reductase Inhibitor",
        .renal_egfr_cutoff = 30.0,
        .cyp3a4_profile = CYP3A4_SUBSTRATE,
        .is_controlled = false,
        .icd10_indication = "E78.0",
        .clinical_notes = "CYP3A4 substrate; risk of rhabdomyolysis with strong inhibitors"
    },
    {
        .canonical_name = "Warfarin",
        .arabic_name = "وارفارين",
        .drug_class = "Vitamin K Antagonist Anticoagulant",
        .renal_egfr_cutoff = 15.0,
        .cyp3a4_profile = CYP3A4_NONE,
        .is_controlled = false,
        .icd10_indication = "I48.9",
        .clinical_notes = "Narrow therapeutic window; international normalized ratio monitoring required"
    },
    {
        .canonical_name = "Amiodarone",
        .arabic_name = "أميودارون",
        .drug_class = "Class III Antiarrhythmic",
        .renal_egfr_cutoff = 15.0,
        .cyp3a4_profile = CYP3A4_INHIBITOR,
        .is_controlled = false,
        .icd10_indication = "I49.9",
        .clinical_notes = "Inhibits CYP2C9 and CYP3A4; interacts with Warfarin causing fatal hemorrhage risk"
    },
    {
        .canonical_name = NULL,
        .arabic_name = NULL,
        .drug_class = NULL,
        .renal_egfr_cutoff = 0.0,
        .cyp3a4_profile = CYP3A4_NONE,
        .is_controlled = false,
        .icd10_indication = NULL,
        .clinical_notes = NULL
    }
};

/* ========================================================================= */
/* 5. Tool 1: med_lookup                                                     */
/* ========================================================================= */

static const taryaq_med_entry_t *taryaq_med_lookup(const char *query) {
    if (!query || query[0] == '\0') return NULL;

    while (isspace((unsigned char)*query)) query++;
    if (*query == '\0') return NULL;

    char norm_q[256];
    arabic_normalize_utf8_hardened(query, norm_q, sizeof(norm_q));

    char lower_q[256];
    size_t qlen = strlen(norm_q);
    if (qlen >= sizeof(lower_q)) qlen = sizeof(lower_q) - 1;
    for (size_t i = 0; i < qlen; ++i) {
        lower_q[i] = (char)tolower((unsigned char)norm_q[i]);
    }
    lower_q[qlen] = '\0';

    for (size_t i = 0; MED_DATABASE[i].canonical_name != NULL; ++i) {
        const taryaq_med_entry_t *med = &MED_DATABASE[i];

        char lower_name[128];
        size_t nlen = strlen(med->canonical_name);
        if (nlen >= sizeof(lower_name)) nlen = sizeof(lower_name) - 1;
        for (size_t j = 0; j < nlen; ++j) {
            lower_name[j] = (char)tolower((unsigned char)med->canonical_name[j]);
        }
        lower_name[nlen] = '\0';

        /* Query contains full medication name (e.g., "Colchicine 0.5mg BID") */
        if (strstr(lower_q, lower_name) != NULL) {
            return med;
        }
        /* Or query is an exact/prefix match of at least 4 characters */
        size_t qlen = strlen(lower_q);
        if (qlen >= 4 && strncmp(lower_name, lower_q, qlen) == 0) {
            return med;
        }

        char norm_med_ar[128];
        arabic_normalize_utf8_hardened(med->arabic_name, norm_med_ar, sizeof(norm_med_ar));
        /* Arabic: query contains full drug name (e.g., "حبوب كولشيسين") */
        if (strstr(norm_q, norm_med_ar) != NULL) {
            return med;
        }
        /* Or exact/prefix match of at least 6 bytes (~3 Arabic UTF-8 chars) */
        size_t ar_qlen = strlen(norm_q);
        if (ar_qlen >= 6 && strncmp(norm_med_ar, norm_q, ar_qlen) == 0) {
            return med;
        }
    }

    return NULL;
}

/* ========================================================================= */
/* 6. Tool 2: interaction_check                                              */
/* ========================================================================= */

typedef struct {
    taryaq_verdict_t verdict;
    char             alert_code[64];
    char             clinical_rationale[256];
    bool             has_renal_risk;
    bool             has_cyp3a4_conflict;
    bool             is_controlled_gate;
} taryaq_check_result_t;

static taryaq_check_result_t taryaq_interaction_check(
    const char *primary_name,
    const char *concurrent_name,
    double      egfr,
    bool        id_verified
) {
    taryaq_check_result_t res;
    memset(&res, 0, sizeof(res));

    const taryaq_med_entry_t *p_med = taryaq_med_lookup(primary_name);
    const taryaq_med_entry_t *c_med = concurrent_name ? taryaq_med_lookup(concurrent_name) : NULL;

    const char *p_canon = p_med ? p_med->canonical_name : (primary_name ? primary_name : "Unknown");
    const char *c_canon = c_med ? c_med->canonical_name : (concurrent_name ? concurrent_name : NULL);

    bool is_ctrl = (p_med && p_med->is_controlled) || (c_med && c_med->is_controlled);

    taryaq_rx_context_t rx;
    memset(&rx, 0, sizeof(rx));
    rx.primary_drug = p_canon;
    rx.concurrent_drug = c_canon;
    rx.patient_egfr_ml_min = egfr;
    rx.is_controlled_substance = is_ctrl;
    rx.has_controlled_id_verified = id_verified;
    rx.icd10_code = p_med ? p_med->icd10_indication : "M10.0";
    rx.submission_delay_days = 0;
    rx.has_ejadah_lab_justification = true;

    taryaq_result_t ar = taryaq_verify_prescription_and_claim(&rx);
    res.verdict = ar.verdict;
    snprintf(res.alert_code, sizeof(res.alert_code), "%s", ar.alert_code);
    snprintf(res.clinical_rationale, sizeof(res.clinical_rationale), "%s", ar.clinical_rationale);

    if (p_med && p_med->renal_egfr_cutoff > 0.0 && egfr < p_med->renal_egfr_cutoff) {
        res.has_renal_risk = true;
    }
    if (p_med && c_med && p_med->cyp3a4_profile == CYP3A4_SUBSTRATE && c_med->cyp3a4_profile == CYP3A4_INHIBITOR) {
        res.has_cyp3a4_conflict = true;
    }
    if (is_ctrl && !id_verified) {
        res.is_controlled_gate = true;
    }

    return res;
}

/* ========================================================================= */
/* 7. Session State Model                                                    */
/* ========================================================================= */

typedef enum {
    SESSION_EMPTY       = 0,
    SESSION_DRAFT       = 1,
    SESSION_SAFE        = 2,
    SESSION_BLOCKED     = 3,
    SESSION_OVERRIDDEN  = 4,
    SESSION_COMMITTED   = 5,
    SESSION_ROLLED_BACK = 6
} taryaq_session_state_t;

typedef struct {
    taryaq_session_state_t state;
    char     primary_drug[64];
    char     concurrent_drug[64];
    double   patient_egfr;
    bool     is_controlled;
    bool     has_id_verified;
    char     alert_code[64];
    char     clinical_rationale[256];
    bool     override_approved;
    char     override_token[64];
    char     override_rationale[256];
    double   net_claim_aed;
    uint32_t delay_days;
    double   penalty_aed;
    uint64_t created_utc;
} taryaq_working_session_t;

static taryaq_working_session_t g_session;

/* ========================================================================= */
/* 8. Tool 3: claim_scrub                                                    */
/* ========================================================================= */

typedef struct {
    double           net_claim_aed;
    uint32_t         delay_days;
    double           penalty_rate_pct;
    double           penalty_aed;
    double           adjusted_net_aed;
    char             alert_code[64];
    char             rationale[256];
    char             scrubbed_query[256];
    taryaq_verdict_t verdict;
} taryaq_claim_scrub_result_t;

static taryaq_claim_scrub_result_t taryaq_claim_scrub(
    const char *primary_drug,
    const char *concurrent_drug,
    double      patient_egfr,
    bool        has_override,
    double      net_claim_aed,
    uint32_t    delay_days,
    const char *icd10_code,
    const char *claim_narrative
) {
    taryaq_claim_scrub_result_t res;
    memset(&res, 0, sizeof(res));

    res.net_claim_aed = net_claim_aed;
    res.delay_days = delay_days;

    if (claim_narrative && claim_narrative[0] != '\0') {
        grounding_scrub_search_query(claim_narrative, res.scrubbed_query, sizeof(res.scrubbed_query));
    } else {
        snprintf(res.scrubbed_query, sizeof(res.scrubbed_query), "DHA eClaimLink SN 03/2019 standard claim");
    }

    res.penalty_rate_pct = (double)delay_days * 0.03;
    res.penalty_aed = net_claim_aed * ((double)delay_days * 0.0003);
    if (res.penalty_aed > net_claim_aed) {
        res.penalty_aed = net_claim_aed;
    }
    res.adjusted_net_aed = net_claim_aed - res.penalty_aed;

    taryaq_rx_context_t rx;
    memset(&rx, 0, sizeof(rx));
    rx.primary_drug = (primary_drug && primary_drug[0] != '\0') ? primary_drug :
                      (g_session.primary_drug[0] != '\0' ? g_session.primary_drug : "Metformin");
    rx.concurrent_drug = (concurrent_drug && concurrent_drug[0] != '\0') ? concurrent_drug :
                         (g_session.concurrent_drug[0] != '\0' ? g_session.concurrent_drug : NULL);
    rx.patient_egfr_ml_min = (patient_egfr > 0.0) ? patient_egfr :
                             (g_session.patient_egfr > 0.0 ? g_session.patient_egfr : 90.0);
    rx.has_clinician_override = has_override;
    rx.is_controlled_substance = g_session.is_controlled;
    rx.has_controlled_id_verified = g_session.has_id_verified;
    rx.icd10_code = icd10_code ? icd10_code : "E11.9";
    rx.submission_delay_days = delay_days;
    rx.has_ejadah_lab_justification = true;

    taryaq_result_t ar = taryaq_verify_prescription_and_claim(&rx);
    res.verdict = ar.verdict;
    snprintf(res.alert_code, sizeof(res.alert_code), "%s", ar.alert_code);
    snprintf(res.rationale, sizeof(res.rationale), "%s", ar.clinical_rationale);

    return res;
}

/* ========================================================================= */
/* 8. Tool 4: human_override                                                 */
/* ========================================================================= */

typedef struct {
    bool override_approved;
    char token[64];
    char scope[32];
    char rationale[256];
    char err_message[256];
} taryaq_override_result_t;

static taryaq_override_result_t taryaq_human_override(
    const char           *approval_code,
    const char           *clinical_rationale,
    qala_approval_slot_t *slots,
    size_t                slot_count,
    uint64_t              now_utc
) {
    taryaq_override_result_t res;
    memset(&res, 0, sizeof(res));

    if (!clinical_rationale || strlen(clinical_rationale) < 8) {
        res.override_approved = false;
        snprintf(res.err_message, sizeof(res.err_message),
                 "REJECTED: Mandatory clinical rationale missing or too brief (min 8 chars).");
        return res;
    }

    bool non_space = false;
    for (size_t i = 0; clinical_rationale[i] != '\0'; ++i) {
        if (!isspace((unsigned char)clinical_rationale[i])) {
            non_space = true;
            break;
        }
    }
    if (!non_space) {
        res.override_approved = false;
        snprintf(res.err_message, sizeof(res.err_message),
                 "REJECTED: Clinical rationale cannot be empty or whitespace only.");
        return res;
    }

    qala_tool_call_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.tool_name = "human_override";
    ctx.target_path = "clinical_override_register";
    ctx.raw_args_json = "{\"action\":\"clinical_override\"}";
    ctx.amount_aed = 0.0;
    ctx.is_mutating = true;
    ctx.required_scope = "clinical_override";
    ctx.supplied_code = approval_code;

    qala_policy_result_t pr = policy_verify_call(&ctx, slots, slot_count, now_utc, NULL);

    if (pr.verdict == VERDICT_SAFE) {
        res.override_approved = true;
        snprintf(res.token, sizeof(res.token), "%s", approval_code ? approval_code : "");
        snprintf(res.scope, sizeof(res.scope), "clinical_override");
        snprintf(res.rationale, sizeof(res.rationale), "%s", clinical_rationale ? clinical_rationale : "");
    } else {
        res.override_approved = false;
        snprintf(res.err_message, sizeof(res.err_message), "%s", pr.reason);
    }

    return res;
}

/* ========================================================================= */
/* 10. Tool 5: commit_audit & Rollback Engine                                */
/* ========================================================================= */

static void taryaq_rollback_state(const char *reason, const char *audit_path, bool enable_telemetry) {
    uint64_t now_utc = taryaq_now_utc();
    if (g_session.state != SESSION_EMPTY && g_session.state != SESSION_ROLLED_BACK) {
        if (audit_path) {
            const char *key = getenv("TARYAQ_AUDIT_KEY");
            if (!key || strlen(key) < 32) {
                fprintf(stderr, "[-] Audit key missing: export TARYAQ_AUDIT_KEY=<32+ bytes>\n");
            } else {
                int fd = open(audit_path, O_RDWR | O_CREAT, 0600);
                if (fd >= 0) {
                    if (flock(fd, LOCK_EX) == 0) {
                        char prev_mac[65];
                        if (taryaq_get_prev_mac(fd, prev_mac)) {
                            char canonical[1024];
                            snprintf(canonical, sizeof(canonical),
                                "%llu|Asia/Dubai|Taryaq v0|PRESCRIPTION_ROLLED_BACK|%s|%s|%.1f|%s",
                                (unsigned long long)now_utc,
                                g_session.primary_drug[0] ? g_session.primary_drug : "NONE",
                                g_session.concurrent_drug[0] ? g_session.concurrent_drug : "NONE",
                                g_session.patient_egfr,
                                reason ? reason : "Operator abort or unapproved block");

                            char hmac_msg[2048];
                            snprintf(hmac_msg, sizeof(hmac_msg), "%s\n%s", prev_mac, canonical);
                            char mac[65];
                            hmac_sha256_hex(key, strlen(key), hmac_msg, strlen(hmac_msg), mac);

                            char sha256_sig[65];
                            sha256_hex(canonical, strlen(canonical), sha256_sig);

                            char esc_primary[128], esc_concurrent[128], esc_reason[256], esc_canon[1024];
                            taryaq_json_escape(g_session.primary_drug[0] ? g_session.primary_drug : "NONE", esc_primary, sizeof(esc_primary));
                            taryaq_json_escape(g_session.concurrent_drug[0] ? g_session.concurrent_drug : "NONE", esc_concurrent, sizeof(esc_concurrent));
                            taryaq_json_escape(reason ? reason : "Operator abort or unapproved block", esc_reason, sizeof(esc_reason));
                            taryaq_json_escape(canonical, esc_canon, sizeof(esc_canon));

                            char line[4096];
                            snprintf(line, sizeof(line),
                                "{\"timestamp_utc\":%llu,\"reporting_tz\":\"Asia/Dubai\",\"agent\":\"Taryaq v0\","
                                "\"action\":\"PRESCRIPTION_ROLLED_BACK\",\"primary_drug\":\"%s\",\"concurrent_drug\":\"%s\","
                                "\"patient_egfr\":%.1f,\"reason\":\"%s\","
                                "\"canonical\":\"%s\",\"prev_sha256\":\"%s\",\"mac\":\"%s\","
                                "\"sha256_signature\":\"%s\",\"state\":\"ROLLED_BACK\"}\n",
                                (unsigned long long)now_utc,
                                esc_primary, esc_concurrent, g_session.patient_egfr, esc_reason,
                                esc_canon, prev_mac, mac, sha256_sig);

                            if (lseek(fd, 0, SEEK_END) >= 0) {
                                ssize_t w = write(fd, line, strlen(line));
                                (void)w;
                                fsync(fd);
                            }
                        }
                        flock(fd, LOCK_UN);
                    }
                    close(fd);
                }
            }
        }
    }

    memset(&g_session, 0, sizeof(g_session));
    g_session.state = SESSION_ROLLED_BACK;

    if (enable_telemetry) {
        char tele[256];
        snprintf(tele, sizeof(tele),
            "{\"event_type\":\"state_committed\",\"action\":\"rollback\",\"status\":\"ROLLED_BACK\"}");
        taryaq_emit_telemetry(true, "state_committed", tele);
    }
}

static bool taryaq_commit_audit(const char *audit_path, bool enable_telemetry) {
    if (g_session.state == SESSION_BLOCKED && !g_session.override_approved) {
        fprintf(stderr, "[-] Expected Security Guard: Cannot commit blocked prescription without valid human override.\n");
        taryaq_rollback_state("Blocked prescription without override", audit_path, enable_telemetry);
        return false;
    }

    if (g_session.state != SESSION_SAFE && g_session.state != SESSION_OVERRIDDEN) {
        fprintf(stderr, "[-] Error: No active, validated prescription state to commit.\n");
        return false;
    }

    const char *key = getenv("TARYAQ_AUDIT_KEY");
    if (!key || strlen(key) < 32) {
        fprintf(stderr, "[-] Audit key missing: export TARYAQ_AUDIT_KEY=<32+ bytes>\n");
        return false;
    }

    uint64_t now_utc = taryaq_now_utc();

    char canonical[1024];
    snprintf(canonical, sizeof(canonical),
        "%llu|Asia/Dubai|Taryaq v0|PRESCRIPTION_COMMITTED|%s|%s|%.1f|%s|%s|%s|%.2f|%u|%.2f",
        (unsigned long long)now_utc,
        g_session.primary_drug,
        g_session.concurrent_drug,
        g_session.patient_egfr,
        g_session.alert_code,
        g_session.override_approved ? g_session.override_token : "NONE",
        g_session.override_approved ? g_session.override_rationale : "NONE",
        g_session.net_claim_aed,
        g_session.delay_days,
        g_session.penalty_aed);

    /* Compute legacy SHA-256 for backward-compatibility */
    char legacy_sig[65];
    sha256_hex(canonical, strlen(canonical), legacy_sig);

    const char *path = audit_path ? audit_path : DEFAULT_AUDIT_LOG;
    int fd = open(path, O_RDWR | O_CREAT, 0600);
    if (fd < 0) {
        perror("[-] Error opening audit log file");
        return false;
    }

    if (flock(fd, LOCK_EX) != 0) {
        perror("[-] Error locking audit log file");
        close(fd);
        return false;
    }

    char prev_mac[65];
    if (!taryaq_get_prev_mac(fd, prev_mac)) {
        fprintf(stderr, "[-] Fatal: Audit chain corruption detected in '%s' (malformed or missing previous mac)\n", path);
        flock(fd, LOCK_UN);
        close(fd);
        return false;
    }

    char hmac_msg[2048];
    snprintf(hmac_msg, sizeof(hmac_msg), "%s\n%s", prev_mac, canonical);
    char mac[65];
    hmac_sha256_hex(key, strlen(key), hmac_msg, strlen(hmac_msg), mac);

    char esc_primary[128], esc_concurrent[128], esc_alert[128];
    char esc_token[128], esc_rationale[256], esc_canon[1024];
    taryaq_json_escape(g_session.primary_drug, esc_primary, sizeof(esc_primary));
    taryaq_json_escape(g_session.concurrent_drug, esc_concurrent, sizeof(esc_concurrent));
    taryaq_json_escape(g_session.alert_code, esc_alert, sizeof(esc_alert));
    taryaq_json_escape(g_session.override_token, esc_token, sizeof(esc_token));
    taryaq_json_escape(g_session.override_rationale, esc_rationale, sizeof(esc_rationale));
    taryaq_json_escape(canonical, esc_canon, sizeof(esc_canon));

    char line[4096];
    snprintf(line, sizeof(line),
        "{\"timestamp_utc\":%llu,\"reporting_tz\":\"Asia/Dubai\",\"agent\":\"Taryaq v0\","
        "\"action\":\"PRESCRIPTION_COMMITTED\",\"primary_drug\":\"%s\",\"concurrent_drug\":\"%s\","
        "\"patient_egfr\":%.1f,\"verdict\":\"%s\",\"alert_code\":\"%s\","
        "\"override_status\":\"%s\",\"override_token\":\"%s\",\"override_rationale\":\"%s\","
        "\"claim_net_aed\":%.2f,\"claim_delay_days\":%u,\"penalty_aed\":%.2f,"
        "\"canonical\":\"%s\",\"prev_sha256\":\"%s\",\"mac\":\"%s\","
        "\"sha256_signature\":\"%s\",\"state\":\"COMMITTED\"}\n",
        (unsigned long long)now_utc,
        esc_primary,
        esc_concurrent,
        g_session.patient_egfr,
        (g_session.state == SESSION_OVERRIDDEN) ? "CLINICAL_CONTRAINDICATION_OVERRIDDEN" : "CLINICAL_SAFE",
        esc_alert,
        g_session.override_approved ? "APPROVED" : "NONE",
        esc_token,
        esc_rationale,
        g_session.net_claim_aed,
        g_session.delay_days,
        g_session.penalty_aed,
        esc_canon,
        prev_mac,
        mac,
        legacy_sig);

    if (lseek(fd, 0, SEEK_END) < 0) {
        perror("[-] Error seeking to end of audit log file");
        flock(fd, LOCK_UN);
        close(fd);
        return false;
    }

    ssize_t wlen = write(fd, line, strlen(line));
    if (wlen < 0 || (size_t)wlen != strlen(line)) {
        perror("[-] Error writing audit record");
        flock(fd, LOCK_UN);
        close(fd);
        return false;
    }

    fsync(fd);
    flock(fd, LOCK_UN);
    close(fd);

    g_session.state = SESSION_COMMITTED;

    if (enable_telemetry) {
        char tele[512];
        snprintf(tele, sizeof(tele),
            "{\"event_type\":\"state_committed\",\"agent\":\"Taryaq v0\",\"verdict\":\"%s\","
            "\"primary_drug\":\"%s\",\"sha256\":\"%s\",\"status\":\"COMMITTED\"}",
            (g_session.override_approved ? "OVERRIDDEN" : "SAFE"),
            esc_primary, mac);
        taryaq_emit_telemetry(true, "state_committed", tele);
    }

    return true;
}

/* ========================================================================= */
/* 10. Execution Mode: Deterministic Acceptance Test (--test)                */
/* ========================================================================= */

static int run_test_scenario(const char *audit_path, bool enable_telemetry) {
    double t_start = taryaq_get_monotonic_ms();

    /* Ensure test key is active if not set in environment */
    if (!getenv("TARYAQ_AUDIT_KEY")) {
        setenv("TARYAQ_AUDIT_KEY", "test-key-32-bytes-minimum-000000", 0);
    }

    /* Cryptographic self-test: RFC 4231 HMAC-SHA256 test vectors */
    char hmac_test[65];
    hmac_sha256_hex("Jefe", 4, "what do ya want for nothing?", 28, hmac_test);
    if (strcmp(hmac_test, "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843") != 0) {
        fprintf(stderr, "[-] Fatal: HMAC-SHA256 failed RFC 4231 test vector 2! Got: %s\n", hmac_test);
        return 1;
    }
    hmac_sha256_hex("test-key-32-bytes-minimum-000000", 32, "Taryaq Sovereign Clinical Audit HMAC Verification", 49, hmac_test);
    if (strcmp(hmac_test, "835a20214031ba6b95982d35e1fbd96acf0abcdabda4cbcd3913ce1f9c83979b") != 0) {
        fprintf(stderr, "[-] Fatal: HMAC-SHA256 failed test key vector! Got: %s\n", hmac_test);
        return 1;
    }

    printf("================================================================================\n");
    printf("  TARYAQ v0 CLI DAEMON: DETERMINISTIC ACCEPTANCE SUITE (-Wall -Werror -O2)\n");
    printf("  Target: Best Societal AI Agent | Dubai AI Agents Championship 2026-2027\n");
    printf("  Criteria: Cold start -> Rx -> Block -> Human Override -> Signed Audit Line\n");
    printf("================================================================================\n");

    /* Approval token pool setup */
    qala_approval_slot_t approval_slots[4];
    memset(approval_slots, 0, sizeof(approval_slots));

    snprintf(approval_slots[0].token, sizeof(approval_slots[0].token), "TARYAQ-OVERRIDE-8842-CLIN");
    snprintf(approval_slots[0].scope, sizeof(approval_slots[0].scope), "clinical_override");
    approval_slots[0].expires_utc = taryaq_now_utc() + 86400 * 365;
    approval_slots[0].consumed = false;

    snprintf(approval_slots[1].token, sizeof(approval_slots[1].token), "DUBAI-CLIN-APPROVE-7701");
    snprintf(approval_slots[1].scope, sizeof(approval_slots[1].scope), "clinical_override");
    approval_slots[1].expires_utc = taryaq_now_utc() + 86400 * 365;
    approval_slots[1].consumed = false;

    size_t slot_count = 2;

    /* Milestone 1: Sub-millisecond cold start */
    double t_cold = taryaq_get_monotonic_ms() - t_start;
    printf("[MILESTONE 1/8] Cold Start: %.3f ms (Criteria: < 15.000 ms) -> PASS\n", t_cold);
    if (t_cold >= 15.0) {
        fprintf(stderr, "[-] Fatal: Cold start exceeded 15ms threshold!\n");
        return 1;
    }

    if (enable_telemetry) {
        char tele[256];
        snprintf(tele, sizeof(tele),
            "{\"event_type\":\"task_started\",\"task_id\":\"taryaq-acceptance-01\","
            "\"title\":\"Taryaq v0 Deterministic Verification\",\"ast_time\":\"%.3f ms\"}",
            t_cold);
        taryaq_emit_telemetry(true, "task_started", tele);
    }

    /* Milestone 2: Tool 1 - med_lookup (Arabic normalization & English graph) */
    const taryaq_med_entry_t *med_colch = taryaq_med_lookup("كولشيسين");
    if (!med_colch || strcmp(med_colch->canonical_name, "Colchicine") != 0) {
        fprintf(stderr, "[-] Fatal: Arabic med_lookup failed for 'كولشيسين'!\n");
        return 1;
    }
    const taryaq_med_entry_t *med_clari = taryaq_med_lookup("Clarithromycin");
    if (!med_clari || med_clari->cyp3a4_profile != CYP3A4_INHIBITOR) {
        fprintf(stderr, "[-] Fatal: med_lookup failed for Clarithromycin!\n");
        return 1;
    }
    printf("[MILESTONE 2/8] Tool 1 (med_lookup): 'كولشيسين' -> Colchicine, 'Clarithromycin' -> Inhibitor -> PASS\n");

    /* Milestone 3: Tool 2 - interaction_check (Fatal CYP3A4 renal block) */
    memset(&g_session, 0, sizeof(g_session));
    snprintf(g_session.primary_drug, sizeof(g_session.primary_drug), "%s", med_colch->canonical_name);
    snprintf(g_session.concurrent_drug, sizeof(g_session.concurrent_drug), "%s", med_clari->canonical_name);
    g_session.patient_egfr = 22.0; /* Severe renal failure */
    g_session.created_utc = taryaq_now_utc();
    g_session.state = SESSION_DRAFT;

    taryaq_check_result_t chk = taryaq_interaction_check(
        g_session.primary_drug, g_session.concurrent_drug, g_session.patient_egfr, false);

    if (chk.verdict != CLINICAL_CONTRAINDICATION_BLOCK ||
        strcmp(chk.alert_code, "FATAL_CYP3A4_RENAL_CONTRAINDICATION") != 0) {
        fprintf(stderr, "[-] Fatal: Expected CLINICAL_CONTRAINDICATION_BLOCK, got %d (%s)\n",
                chk.verdict, chk.alert_code);
        return 1;
    }
    g_session.state = SESSION_BLOCKED;
    snprintf(g_session.alert_code, sizeof(g_session.alert_code), "%s", chk.alert_code);
    snprintf(g_session.clinical_rationale, sizeof(g_session.clinical_rationale), "%s", chk.clinical_rationale);

    printf("[MILESTONE 3/8] Tool 2 (interaction_check): Blocked FATAL_CYP3A4_RENAL_CONTRAINDICATION -> PASS\n");

    if (enable_telemetry) {
        char tele[256];
        snprintf(tele, sizeof(tele),
            "{\"event_type\":\"taryaq_clinical\",\"drugs\":\"Colchicine + Clarithromycin\","
            "\"egfr\":22.0,\"verdict\":\"FATAL CONTRAINDICATION BLOCKED\","
            "\"code\":\"FATAL_CYP3A4_RENAL_CONTRAINDICATION\"}");
        taryaq_emit_telemetry(true, "taryaq_clinical", tele);
    }

    /* Milestone 4: Tool 5 Guard - Attempt unapproved commit & verify state rollback */
    bool unauth_commit = taryaq_commit_audit(audit_path, enable_telemetry);
    if (unauth_commit != false || g_session.state != SESSION_ROLLED_BACK) {
        fprintf(stderr, "[-] Fatal: Blocked prescription without override was erroneously committed!\n");
        return 1;
    }
    printf("[MILESTONE 4/8] Tool 5 (commit_audit Guard): Blocked commit rejected & state rolled back -> PASS\n");

    /* Milestone 5: Tool 4 Guard - Re-intake and test mandatory clinical rationale */
    snprintf(g_session.primary_drug, sizeof(g_session.primary_drug), "%s", "Colchicine");
    snprintf(g_session.concurrent_drug, sizeof(g_session.concurrent_drug), "%s", "Clarithromycin");
    g_session.patient_egfr = 22.0;
    g_session.state = SESSION_BLOCKED;
    snprintf(g_session.alert_code, sizeof(g_session.alert_code), "%s", "FATAL_CYP3A4_RENAL_CONTRAINDICATION");

    taryaq_override_result_t bad_ov = taryaq_human_override(
        "TARYAQ-OVERRIDE-8842-CLIN", "   ", approval_slots, slot_count, taryaq_now_utc());
    if (bad_ov.override_approved != false) {
        fprintf(stderr, "[-] Fatal: Human override accepted blank rationale!\n");
        return 1;
    }
    printf("[MILESTONE 5/8] Tool 4 (human_override Guard): Empty clinical rationale rejected -> PASS\n");

    /* Milestone 6: Tool 4 - Apply Human Override with Valid Code and Clinical Rationale */
    const char *approval_code = "TARYAQ-OVERRIDE-8842-CLIN";
    const char *rationale = "ICU continuous hemodialysis initiated; nephrology specialist co-signature verified (Ref: CLIN-REV-9921)";
    taryaq_override_result_t good_ov = taryaq_human_override(
        approval_code, rationale, approval_slots, slot_count, taryaq_now_utc());
    if (!good_ov.override_approved) {
        fprintf(stderr, "[-] Fatal: Human override failed with error: %s\n", good_ov.err_message);
        return 1;
    }
    g_session.override_approved = true;
    snprintf(g_session.override_token, sizeof(g_session.override_token), "%s", good_ov.token);
    snprintf(g_session.override_rationale, sizeof(g_session.override_rationale), "%s", good_ov.rationale);
    g_session.state = SESSION_OVERRIDDEN;

    printf("[MILESTONE 6/8] Tool 4 (human_override): Verified code '%s' and consumed slot -> PASS\n", approval_code);

    if (enable_telemetry) {
        char tele[256];
        snprintf(tele, sizeof(tele),
            "{\"event_type\":\"gate_verdict\",\"verdict\":\"APPROVED & OVERRIDDEN\","
            "\"token\":\"%s\",\"tool\":\"human_override\"}", approval_code);
        taryaq_emit_telemetry(true, "gate_verdict", tele);
    }

    /* Milestone 7: Tool 3 - claim_scrub (DHA SN 03/2019 Claim Cycle & Late Penalty with Dynamic Context) */
    taryaq_claim_scrub_result_t scrub_ok = taryaq_claim_scrub(
        g_session.primary_drug, g_session.concurrent_drug, g_session.patient_egfr,
        g_session.override_approved, 450.00, 0, "M10.0", "Patient prescription claim");
    if (scrub_ok.verdict != CLINICAL_SAFE || scrub_ok.penalty_aed != 0.0) {
        fprintf(stderr, "[-] Fatal: Normal claim failed scrub!\n");
        return 1;
    }
    taryaq_claim_scrub_result_t scrub_late = taryaq_claim_scrub(
        g_session.primary_drug, g_session.concurrent_drug, g_session.patient_egfr,
        g_session.override_approved, 450.00, 12, "M10.0", "Delayed claim submission");
    if (scrub_late.verdict != CLINICAL_CLAIM_CYCLE_WARN || scrub_late.penalty_aed < 1.61 || scrub_late.penalty_aed > 1.63) {
        fprintf(stderr, "[-] Fatal: DHA SN 03/2019 12-day penalty incorrect (expected 1.62 AED, got %.2f AED)!\n",
                scrub_late.penalty_aed);
        return 1;
    }
    g_session.net_claim_aed = scrub_ok.net_claim_aed;
    g_session.delay_days = scrub_ok.delay_days;
    g_session.penalty_aed = scrub_ok.penalty_aed;

    printf("[MILESTONE 7/8] Tool 3 (claim_scrub): DHA SN 03/2019 verified (12 days = 1.62 AED penalty) -> PASS\n");

    /* Milestone 8: Tool 5 - commit_audit (Signed JSONL line & replay protection check) */
    bool commit_ok = taryaq_commit_audit(audit_path, enable_telemetry);
    if (!commit_ok || g_session.state != SESSION_COMMITTED) {
        fprintf(stderr, "[-] Fatal: Commit audit failed for overridden prescription!\n");
        return 1;
    }

    /* Verify audit file on disk */
    FILE *fp = fopen(audit_path ? audit_path : DEFAULT_AUDIT_LOG, "r");
    if (!fp) {
        fprintf(stderr, "[-] Fatal: Could not open audit file '%s' for verification!\n",
                audit_path ? audit_path : DEFAULT_AUDIT_LOG);
        return 1;
    }
    char last_line[2048] = {0};
    char buf[2048];
    while (fgets(buf, sizeof(buf), fp)) {
        if (strlen(buf) > 10) {
            snprintf(last_line, sizeof(last_line), "%s", buf);
        }
    }
    fclose(fp);

    if (strstr(last_line, "PRESCRIPTION_COMMITTED") == NULL ||
        strstr(last_line, "Colchicine") == NULL ||
        strstr(last_line, "TARYAQ-OVERRIDE-8842-CLIN") == NULL ||
        strstr(last_line, "\"canonical\":") == NULL ||
        strstr(last_line, "\"prev_sha256\":") == NULL ||
        strstr(last_line, "\"mac\":") == NULL ||
        strstr(last_line, "sha256_signature") == NULL) {
        fprintf(stderr, "[-] Fatal: Audit line on disk is missing required signature or fields: %s\n", last_line);
        return 1;
    }

    /* Test single-use token replay rejection */
    taryaq_override_result_t replay_ov = taryaq_human_override(
        approval_code, "Replay attempt", approval_slots, slot_count, taryaq_now_utc());
    if (replay_ov.override_approved != false) {
        fprintf(stderr, "[-] Fatal: Replay of consumed approval token was accepted!\n");
        return 1;
    }

    printf("[MILESTONE 8/8] Tool 5 (commit_audit): Signed JSONL on disk & single-use replay blocked -> PASS\n");

    /* Patient Care Companion: Emergency Red-Flag Triage Verification */
    taryaq_triage_result_t tr_ar = taryaq_triage_patient_symptoms("أشعر بألم في الصدر وضيق تنفس");
    if (!tr_ar.is_red_flag || tr_ar.level != TRIAGE_EMERGENCY || strcmp(tr_ar.triage_category, "CARDIOVASCULAR_ACUTE") != 0) {
        fprintf(stderr, "[-] Fatal: Patient Arabic red-flag triage failed to trigger emergency interrupt!\n");
        return 1;
    }
    taryaq_triage_result_t tr_en = taryaq_triage_patient_symptoms("Patient has sudden facial drooping and slurred speech");
    if (!tr_en.is_red_flag || tr_en.level != TRIAGE_EMERGENCY || strcmp(tr_en.triage_category, "STROKE_NEUROLOGICAL") != 0) {
        fprintf(stderr, "[-] Fatal: Patient English stroke red-flag triage failed!\n");
        return 1;
    }
    taryaq_triage_result_t tr_safe = taryaq_triage_patient_symptoms("Routine refill inquiry for Metformin 500mg");
    if (tr_safe.is_red_flag || tr_safe.level != TRIAGE_ROUTINE) {
        fprintf(stderr, "[-] Fatal: Routine inquiry falsely triggered red flag!\n");
        return 1;
    }
    printf("[TRIAGE GATE] Patient Emergency Red-Flag Triage verified (Arabic/English interrupts & routine) -> PASS\n");

    double t_total = taryaq_get_monotonic_ms() - t_start;

    printf("================================================================================\n");
    printf(" [SUCCESS] Taryaq v0 Acceptance Workflow Passed in %.3f ms (< 180,000 ms)\n", t_total);
    printf(" Mode: 100%% Air-Gapped C99 Native Engine | Zero Network Bloat\n");
    printf(" Audit Record: %s\n", audit_path ? audit_path : DEFAULT_AUDIT_LOG);
    printf("================================================================================\n");

    return 0;
}

/* ========================================================================= */
/* 11. Execution Mode: Interactive CLI (--cli)                               */
/* ========================================================================= */

static void print_cli_help(void) {
    printf("--------------------------------------------------------------------------------\n");
    printf(" TARYAQ v0 CLI DAEMON COMMANDS:\n");
    printf("   lookup <drug_name>                  - Query local graph (Arabic or English)\n");
    printf("   rx <primary> [concurrent] [egfr]    - Intake prescription vitals\n");
    printf("   check                               - Run contraindication & interaction engine\n");
    printf("   claim <net_aed> <delay_days> [note] - Run DHA SN 03/2019 claim cycle scrubber\n");
    printf("   override <token> <rationale...>     - Apply single-use cryptographic override\n");
    printf("   triage <patient_symptoms...>        - Emergency red-flag triage (Arabic / English)\n");
    printf("   commit                              - Sign and write append-only audit line\n");
    printf("   rollback                            - Discard uncommitted prescription state\n");
    printf("   status                              - Display current session status\n");
    printf("   slots                               - List available approval token slots\n");
    printf("   help                                - Print this command reference\n");
    printf("   quit / exit                         - Terminate session\n");
    printf("--------------------------------------------------------------------------------\n");
}

static void run_cli_loop(const char *audit_path, bool enable_telemetry) {
    printf("================================================================================\n");
    printf("  TARYAQ v0 CLI DAEMON - Sovereign Clinical Safety & DHA Reimbursement Firewall\n");
    printf("  Dubai Create AI Agents Championship 2026-2027 | Best Societal AI Agent\n");
    printf("  Mode: 100%% Air-Gapped ISO C99 Native Engine | Telemetry: %s\n",
           enable_telemetry ? "ACTIVE (http://127.0.0.1:8787/emit)" : "DISABLED (Air-Gapped)");
    printf("================================================================================\n");
    print_cli_help();

    qala_approval_slot_t approval_slots[4];
    memset(approval_slots, 0, sizeof(approval_slots));
    snprintf(approval_slots[0].token, sizeof(approval_slots[0].token), "TARYAQ-OVERRIDE-8842-CLIN");
    snprintf(approval_slots[0].scope, sizeof(approval_slots[0].scope), "clinical_override");
    approval_slots[0].expires_utc = taryaq_now_utc() + 86400 * 365;
    approval_slots[0].consumed = false;

    snprintf(approval_slots[1].token, sizeof(approval_slots[1].token), "DUBAI-CLIN-APPROVE-7701");
    snprintf(approval_slots[1].scope, sizeof(approval_slots[1].scope), "clinical_override");
    approval_slots[1].expires_utc = taryaq_now_utc() + 86400 * 365;
    approval_slots[1].consumed = false;

    size_t slot_count = 2;

    char line[1024];
    while (true) {
        printf("taryaq> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\nExiting Taryaq CLI.\n");
            break;
        }

        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r' || isspace((unsigned char)line[len - 1]))) {
            line[--len] = '\0';
        }
        if (len == 0) continue;

        char cmd[64] = {0};
        char rest[960] = {0};
        int parsed = sscanf(line, "%63s %959[^\n]", cmd, rest);
        if (parsed <= 0) continue;

        if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
            printf("Exiting Taryaq CLI.\n");
            break;
        } else if (strcmp(cmd, "help") == 0) {
            print_cli_help();
        } else if (strcmp(cmd, "status") == 0) {
            const char *st_str = "EMPTY";
            if (g_session.state == SESSION_DRAFT) st_str = "DRAFT";
            else if (g_session.state == SESSION_SAFE) st_str = "SAFE";
            else if (g_session.state == SESSION_BLOCKED) st_str = "BLOCKED";
            else if (g_session.state == SESSION_OVERRIDDEN) st_str = "OVERRIDDEN";
            else if (g_session.state == SESSION_COMMITTED) st_str = "COMMITTED";
            else if (g_session.state == SESSION_ROLLED_BACK) st_str = "ROLLED_BACK";

            printf("Session State:      %s\n", st_str);
            printf("Primary Drug:       %s\n", g_session.primary_drug[0] ? g_session.primary_drug : "(none)");
            printf("Concurrent Drug:    %s\n", g_session.concurrent_drug[0] ? g_session.concurrent_drug : "(none)");
            printf("Patient eGFR:       %.1f ml/min\n", g_session.patient_egfr);
            printf("Alert Code:         %s\n", g_session.alert_code[0] ? g_session.alert_code : "(none)");
            printf("Override Status:    %s\n", g_session.override_approved ? "APPROVED" : "NOT_APPLIED");
            if (g_session.override_approved) {
                printf("Override Token:     %s\n", g_session.override_token);
                printf("Override Rationale: %s\n", g_session.override_rationale);
            }
            printf("Claim Amount:       AED %.2f (Delay: %u days, Penalty: AED %.2f)\n",
                   g_session.net_claim_aed, g_session.delay_days, g_session.penalty_aed);
        } else if (strcmp(cmd, "lookup") == 0 || strcmp(cmd, "بحث") == 0) {
            if (parsed < 2 || rest[0] == '\0') {
                printf("Usage: lookup <drug_name_ar_or_en>\n");
                continue;
            }
            const taryaq_med_entry_t *med = taryaq_med_lookup(rest);
            if (!med) {
                printf("[-] Medication '%s' not found in embedded sovereign graph.\n", rest);
            } else {
                printf("[+] Found: %s (%s)\n", med->canonical_name, med->arabic_name);
                printf("    Class:            %s\n", med->drug_class);
                printf("    Renal Cutoff:     %.1f eGFR ml/min\n", med->renal_egfr_cutoff);
                printf("    CYP3A4 Profile:   %s\n",
                       med->cyp3a4_profile == CYP3A4_SUBSTRATE ? "Substrate" :
                       med->cyp3a4_profile == CYP3A4_INHIBITOR ? "Potent Inhibitor" : "None");
                printf("    Controlled Drug:  %s\n", med->is_controlled ? "YES (UAE Law No. 2 of 2019)" : "NO");
                printf("    Indication ICD10: %s\n", med->icd10_indication);
                printf("    Clinical Notes:   %s\n", med->clinical_notes);
            }
        } else if (strcmp(cmd, "rx") == 0) {
            char p_drug[64] = {0}, c_drug[64] = {0};
            double egfr = 90.0;
            int n = sscanf(rest, "%63s %63s %lf", p_drug, c_drug, &egfr);
            if (n < 1) {
                printf("Usage: rx <primary_drug> [concurrent_drug] [patient_egfr]\n");
                continue;
            }
            if (n == 1) {
                c_drug[0] = '\0';
                egfr = 90.0;
            } else if (n == 2) {
                egfr = 90.0;
            }

            memset(&g_session, 0, sizeof(g_session));
            snprintf(g_session.primary_drug, sizeof(g_session.primary_drug), "%s", p_drug);
            if (c_drug[0] != '\0' && strcmp(c_drug, "none") != 0 && strcmp(c_drug, "-") != 0) {
                snprintf(g_session.concurrent_drug, sizeof(g_session.concurrent_drug), "%s", c_drug);
            }
            g_session.patient_egfr = egfr;
            g_session.created_utc = taryaq_now_utc();
            g_session.state = SESSION_DRAFT;

            printf("[+] Prescription drafted: %s %s%s (eGFR: %.1f)\n",
                   g_session.primary_drug,
                   g_session.concurrent_drug[0] ? "+ " : "",
                   g_session.concurrent_drug[0] ? g_session.concurrent_drug : "",
                   g_session.patient_egfr);

            taryaq_check_result_t chk = taryaq_interaction_check(
                g_session.primary_drug, g_session.concurrent_drug, g_session.patient_egfr, false);
            snprintf(g_session.alert_code, sizeof(g_session.alert_code), "%s", chk.alert_code);
            snprintf(g_session.clinical_rationale, sizeof(g_session.clinical_rationale), "%s", chk.clinical_rationale);

            if (chk.verdict == CLINICAL_CONTRAINDICATION_BLOCK) {
                g_session.state = SESSION_BLOCKED;
                printf("[!] VERDICT: BLOCKED - %s\n", chk.alert_code);
                printf("    %s\n", chk.clinical_rationale);
                printf("    To override, run: override <token> <clinical_rationale>\n");
            } else if (chk.verdict == CLINICAL_CONTROLLED_DRUG_GATE) {
                g_session.state = SESSION_BLOCKED;
                printf("[!] VERDICT: CONTROLLED MEDICATION GATE - %s\n", chk.alert_code);
                printf("    %s\n", chk.clinical_rationale);
            } else if (chk.verdict == CLINICAL_UNVERIFIED_REVIEW) {
                g_session.state = SESSION_BLOCKED;
                printf("[!] VERDICT: UNVERIFIED DRUG REVIEW REQUIRED - %s\n", chk.alert_code);
                printf("    %s\n", chk.clinical_rationale);
                printf("    To override, run: override <token> <clinical_rationale>\n");
            } else {
                g_session.state = SESSION_SAFE;
                printf("[+] VERDICT: SAFE - %s\n", chk.alert_code);
                printf("    %s\n", chk.clinical_rationale);
            }
        } else if (strcmp(cmd, "check") == 0) {
            if (g_session.primary_drug[0] == '\0') {
                printf("[-] No prescription active. Use 'rx <primary> [concurrent] [egfr]' first.\n");
                continue;
            }
            taryaq_check_result_t chk = taryaq_interaction_check(
                g_session.primary_drug, g_session.concurrent_drug, g_session.patient_egfr, false);
            printf("[*] Interaction Check Result:\n");
            printf("    Verdict:   %d\n", chk.verdict);
            printf("    Code:      %s\n", chk.alert_code);
            printf("    Rationale: %s\n", chk.clinical_rationale);
        } else if (strcmp(cmd, "claim") == 0) {
            double aed = 0.0;
            uint32_t delay = 0;
            char narrative[512] = {0};
            int n = sscanf(rest, "%lf %u %511[^\n]", &aed, &delay, narrative);
            if (n < 2) {
                printf("Usage: claim <net_claim_aed> <delay_days> [claim_narrative]\n");
                continue;
            }
            taryaq_claim_scrub_result_t sc = taryaq_claim_scrub(
                g_session.primary_drug, g_session.concurrent_drug, g_session.patient_egfr,
                g_session.override_approved, aed, delay, "M10.0", narrative);
            g_session.net_claim_aed = sc.net_claim_aed;
            g_session.delay_days = sc.delay_days;
            g_session.penalty_aed = sc.penalty_aed;

            printf("[*] DHA SN 03/2019 Claim Scrub Result:\n");
            printf("    Net Claim:      AED %.2f\n", sc.net_claim_aed);
            printf("    Delay Days:     %u day(s)\n", sc.delay_days);
            printf("    Penalty Rate:   %.2f%% (0.03%%/day)\n", sc.penalty_rate_pct);
            printf("    Penalty Amount: AED %.2f\n", sc.penalty_aed);
            printf("    Adjusted Net:   AED %.2f\n", sc.adjusted_net_aed);
            printf("    Scrubbed Query: %s\n", sc.scrubbed_query);
            printf("    Verdict:        %s (%s)\n",
                   sc.verdict == CLINICAL_SAFE ? "SAFE" : "CYCLE_WARN", sc.alert_code);
        } else if (strcmp(cmd, "override") == 0) {
            char token[64] = {0};
            char rationale[512] = {0};
            int n = sscanf(rest, "%63s %511[^\n]", token, rationale);
            if (n < 2) {
                printf("Usage: override <approval_token> <mandatory_clinical_rationale>\n");
                continue;
            }
            if (g_session.state != SESSION_BLOCKED) {
                printf("[-] Notice: Session is not currently in BLOCKED state.\n");
            }
            taryaq_override_result_t ov = taryaq_human_override(
                token, rationale, approval_slots, slot_count, taryaq_now_utc());
            if (ov.override_approved) {
                g_session.override_approved = true;
                snprintf(g_session.override_token, sizeof(g_session.override_token), "%s", ov.token);
                snprintf(g_session.override_rationale, sizeof(g_session.override_rationale), "%s", ov.rationale);
                g_session.state = SESSION_OVERRIDDEN;
                printf("[+] OVERRIDE SUCCESS: Approval code '%s' accepted & consumed.\n", token);
                printf("    Rationale: %s\n", rationale);
                printf("    Ready to commit. Run: commit\n");
            } else {
                printf("[-] OVERRIDE REJECTED: %s\n", ov.err_message);
            }
        } else if (strcmp(cmd, "triage") == 0 || strcmp(cmd, "فرز") == 0) {
            if (parsed < 2 || rest[0] == '\0') {
                printf("Usage: triage <patient_reported_symptoms_in_arabic_or_english>\n");
                continue;
            }
            taryaq_triage_result_t tr = taryaq_triage_patient_symptoms(rest);
            if (tr.is_red_flag) {
                printf("[!] EMERGENCY RED-FLAG DETECTED: %s\n", tr.triage_category);
                printf("    Trigger Word: %s\n", tr.matched_trigger);
                printf("    Protocol:     %s\n", tr.action_protocol);
                printf("    Guidance:     %s\n", tr.clinical_guidance);
            } else {
                printf("[+] TRIAGE ROUTINE: %s\n", tr.triage_category);
                printf("    Protocol:     %s\n", tr.action_protocol);
                printf("    Guidance:     %s\n", tr.clinical_guidance);
            }
        } else if (strcmp(cmd, "commit") == 0) {
            if (g_session.state == SESSION_BLOCKED && !g_session.override_approved) {
                printf("[-] COMMIT REJECTED: Fatal contraindication blocked without override.\n");
                printf("    To authorize, run: override <token> <clinical_rationale>\n");
                printf("    To discard session, run: rollback\n");
                continue;
            }
            if (g_session.primary_drug[0] == '\0') {
                printf("[-] Nothing to commit. Draft prescription first with 'rx ...'.\n");
                continue;
            }
            bool ok = taryaq_commit_audit(audit_path, enable_telemetry);
            if (ok) {
                printf("[+] COMMIT SUCCESS: Prescription signed and appended to %s\n",
                       audit_path ? audit_path : DEFAULT_AUDIT_LOG);
            } else {
                printf("[-] Commit failed.\n");
            }
        } else if (strcmp(cmd, "rollback") == 0) {
            taryaq_rollback_state("Manual rollback by pharmacist", audit_path, enable_telemetry);
            printf("[+] Session rolled back and reset.\n");
        } else if (strcmp(cmd, "slots") == 0) {
            printf("[*] Available Approval Slots:\n");
            for (size_t i = 0; i < slot_count; ++i) {
                printf("    Slot %zu: Token: %s | Scope: %s | Consumed: %s\n",
                       i + 1, approval_slots[i].token, approval_slots[i].scope,
                       approval_slots[i].consumed ? "YES (USED)" : "NO (AVAILABLE)");
            }
        } else {
            printf("[-] Unknown command '%s'. Type 'help' for command list.\n", cmd);
        }
    }
}

/* ========================================================================= */
/* 12. Main Entry Point                                                      */
/* ========================================================================= */

int main(int argc, char **argv) {
    bool test_mode = false;
    bool cli_mode = false;
    bool enable_telemetry = false;
    const char *audit_path = DEFAULT_AUDIT_LOG;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--test") == 0) {
            test_mode = true;
        } else if (strcmp(argv[i], "--cli") == 0) {
            cli_mode = true;
        } else if (strcmp(argv[i], "--telemetry") == 0) {
            enable_telemetry = true;
        } else if (strcmp(argv[i], "--audit-file") == 0 && (i + 1) < argc) {
            audit_path = argv[++i];
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [--test] [--cli] [--telemetry] [--audit-file <path>] [--help]\n", argv[0]);
            printf("Options:\n");
            printf("  --test         Run automated deterministic acceptance test and exit 0 on success\n");
            printf("  --cli          Launch interactive conversational CLI\n");
            printf("  --telemetry    Enable optional live telemetry emission to http://127.0.0.1:8787/emit\n");
            printf("  --audit-file   Override path to append-only JSONL audit log (default: taryaq_audit.jsonl)\n");
            printf("  --help         Display this help message\n");
            return 0;
        }
    }

    if (enable_telemetry) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    int rc = 0;
    if (test_mode) {
        rc = run_test_scenario(audit_path, enable_telemetry);
    } else {
        (void)cli_mode;
        run_cli_loop(audit_path, enable_telemetry);
    }

    if (enable_telemetry) {
        curl_global_cleanup();
    }

    return rc;
}