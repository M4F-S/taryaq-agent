/* nibras_arabic_fademem_adapter.c
 * Hardened C99 Implementation of:
 *   1. Zero-Allocation UTF-8 Arabic & Gulf Dialect Normalizer
 *   2. Time-Gated FadeMem Episodic Memory Consolidation & Context Token Pruning
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

#include "nibras_adapters.h"

/* Helper to get UTF-8 sequence length from leading byte */
static size_t utf8_codepoint_len(unsigned char lead) {
    if ((lead & 0x80) == 0) return 1;
    if ((lead & 0xE0) == 0xC0) return 2;
    if ((lead & 0xF0) == 0xE0) return 3;
    if ((lead & 0xF8) == 0xF0) return 4;
    return 1; /* Invalid lead byte: treat as 1 byte */
}

/* F-11 Fix: Zero-allocation UTF-8 Arabic & Gulf Dialect Normalizer with atomic codepoint handling */
size_t arabic_normalize_utf8_hardened(const char *src, char *dst, size_t dst_cap) {
    if (!src || !dst || dst_cap == 0) return 0;

    const unsigned char *s = (const unsigned char *)src;
    unsigned char *d = (unsigned char *)dst;
    size_t out_idx = 0;

    while (*s != '\0' && (out_idx + 1) < dst_cap) {
        /* Check for Zero-Width Non-Joiner (U+200C: E2 80 8C) or Joiner (U+200D: E2 80 8D) */
        if (s[0] == 0xE2 && s[1] == 0x80 && (s[2] == 0x8C || s[2] == 0x8D)) {
            s += 3;
            continue;
        }

        /* Check for 2-byte UTF-8 Arabic & Extended Arabic blocks (0xD8, 0xD9, 0xDB) */
        if ((s[0] == 0xD8 || s[0] == 0xD9 || s[0] == 0xDB) && s[1] != '\0') {
            unsigned char b0 = s[0];
            unsigned char b1 = s[1];

            if (b0 == 0xD8) {
                /* Unify Alef variants:
                 * U+0622 (آ 0xD8 0xA2), U+0623 (أ 0xD8 0xA3), U+0625 (إ 0xD8 0xA5)
                 * -> U+0627 (ا 0xD8 0xA7)
                 */
                if (b1 == 0xA2 || b1 == 0xA3 || b1 == 0xA5) {
                    if (out_idx + 2 < dst_cap) {
                        d[out_idx++] = 0xD8;
                        d[out_idx++] = 0xA7;
                    }
                    s += 2;
                    continue;
                }

                /* AF-09 Fix: Normalize Hamza-on-Waw U+0624 (ؤ 0xD8 0xA4) -> Waw U+0648 (و 0xD9 0x88) */
                if (b1 == 0xA4) {
                    if (out_idx + 2 < dst_cap) {
                        d[out_idx++] = 0xD9;
                        d[out_idx++] = 0x88;
                    }
                    s += 2;
                    continue;
                }

                /* AF-09 Fix: Normalize Hamza-on-Ya U+0626 (ئ 0xD8 0xA6) -> Ya U+064A (ي 0xD9 0x8A) */
                if (b1 == 0xA6) {
                    if (out_idx + 2 < dst_cap) {
                        d[out_idx++] = 0xD9;
                        d[out_idx++] = 0x8A;
                    }
                    s += 2;
                    continue;
                }

                /* Fold Ta Marbuta U+0629 (ة 0xD8 0xA9) -> Heh U+0647 (ه 0xD9 0x87) */
                if (b1 == 0xA9) {
                    if (out_idx + 2 < dst_cap) {
                        d[out_idx++] = 0xD9;
                        d[out_idx++] = 0x87;
                    }
                    s += 2;
                    continue;
                }
            } else if (b0 == 0xD9) {
                /* Strip Tatweel / Kashida U+0640 (ـ 0xD9 0x80) */
                if (b1 == 0x80) {
                    s += 2;
                    continue;
                }

                /* Strip Tashkeel diacritics U+064B..U+065F (0xD9 0x8B .. 0xD9 0x9F) */
                if (b1 >= 0x8B && b1 <= 0x9F) {
                    s += 2;
                    continue;
                }

                /* Unify Alef Maksura U+0649 (ى 0xD9 0x89) -> Ya U+064A (ي 0xD9 0x8A) */
                if (b1 == 0x89) {
                    if (out_idx + 2 < dst_cap) {
                        d[out_idx++] = 0xD9;
                        d[out_idx++] = 0x8A;
                    }
                    s += 2;
                    continue;
                }

                /* Unify Alef Wasla U+0671 (ٱ 0xD9 0xB1) -> Alef U+0627 (ا 0xD8 0xA7) */
                if (b1 == 0xB1) {
                    if (out_idx + 2 < dst_cap) {
                        d[out_idx++] = 0xD8;
                        d[out_idx++] = 0xA7;
                    }
                    s += 2;
                    continue;
                }

                /* Fold Eastern Arabic digits U+0660..U+0669 (٠-٩: 0xD9 0xA0..0xA9) -> ASCII '0'..'9' */
                if (b1 >= 0xA0 && b1 <= 0xA9) {
                    if (out_idx + 1 < dst_cap) {
                        d[out_idx++] = (unsigned char)('0' + (b1 - 0xA0));
                    }
                    s += 2;
                    continue;
                }
            } else if (b0 == 0xDB) {
                /* Fold Extended/Persian Arabic digits U+06F0..U+06F9 (۰-۹: 0xDB 0xB0..0xB9) -> ASCII '0'..'9' */
                if (b1 >= 0xB0 && b1 <= 0xB9) {
                    if (out_idx + 1 < dst_cap) {
                        d[out_idx++] = (unsigned char)('0' + (b1 - 0xB0));
                    }
                    s += 2;
                    continue;
                }
            }
        }

        /* F-11 Fix: Copy complete multi-byte UTF-8 codepoints atomically */
        size_t cp_len = utf8_codepoint_len(*s);
        if (out_idx + cp_len < dst_cap) {
            for (size_t k = 0; k < cp_len && s[k] != '\0'; ++k) {
                d[out_idx++] = s[k];
            }
        }
        s += cp_len;
    }

    d[out_idx] = '\0';
    return out_idx;
}

/* F-12 Fix: Time-gated FadeMem step function with 64-bit precision and NaN safety */
void nibras_fademem_step_hardened(
    nibras_memory_node_t        *nodes,
    size_t                       count,
    uint64_t                     now_utc,
    double                       half_life_sec,
    double                       sim_time_scale,
    double                       consolidation_window_sec,
    nibras_telemetry_snapshot_t *out_snap
) {
    if (!nodes || !out_snap) return;
    if (!isfinite(half_life_sec) || half_life_sec <= 0.0) return;
    if (!isfinite(sim_time_scale) || sim_time_scale <= 0.0) sim_time_scale = 1.0;
    if (!isfinite(consolidation_window_sec) || consolidation_window_sec <= 0.0) consolidation_window_sec = 259200.0;

    memset(out_snap, 0, sizeof(*out_snap));

    double friction_sum = 0.0;
    size_t friction_count = 0;

    for (size_t i = 0; i < count; ++i) {
        out_snap->total_raw_tokens += nodes[i].token_cost;

        /* AF-09 Fix: Preserve previously distilled permanent rules */
        if (nodes[i].state == TRAJ_DISTILLED_RULE) {
            nodes[i].current_intensity = 1.0;
            out_snap->distilled_rules_count++;
            out_snap->active_tokens_after_prune += (nodes[i].token_cost > 0) ? ((nodes[i].token_cost + 5U) / 6U) : 0;
            continue;
        }

        double raw_dt = (now_utc > nodes[i].created_utc)
                      ? (double)(now_utc - nodes[i].created_utc)
                      : 0.0;
        double effective_dt = raw_dt * sim_time_scale;
        double exponent = -(effective_dt / half_life_sec);
        nodes[i].current_intensity = nodes[i].initial_intensity * pow(2.0, exponent);

        /* NaN intensity safety guard */
        if (!isfinite(nodes[i].current_intensity)) {
            nodes[i].state = TRAJ_NOISE_PRUNED;
            nodes[i].current_intensity = 0.0;
            continue;
        }

        /* Time-Gated Rule Promotion: high salience nodes promote only after consolidation window */
        if (effective_dt >= consolidation_window_sec &&
            nodes[i].initial_intensity >= 0.80 &&
            nodes[i].valence != 0) {
            nodes[i].state = TRAJ_DISTILLED_RULE;
            nodes[i].current_intensity = 1.0; /* Permanent procedural rule */
            out_snap->distilled_rules_count++;
            out_snap->active_tokens_after_prune += (nodes[i].token_cost > 0) ? ((nodes[i].token_cost + 5U) / 6U) : 0;
        }
        /* Prune noise below 0.25 threshold unless valence inverted */
        else if (nodes[i].current_intensity < 0.25 && !nodes[i].valence_inverted) {
            nodes[i].state = TRAJ_NOISE_PRUNED;
        } else {
            nodes[i].state = nodes[i].valence_inverted ? TRAJ_RECOVERY_VECTOR : TRAJ_ACTIVE_EPISODIC;
            out_snap->active_tokens_after_prune += nodes[i].token_cost;
        }

        if (nodes[i].valence < 0 && nodes[i].state != TRAJ_NOISE_PRUNED) {
            friction_sum += nodes[i].current_intensity;
            friction_count++;
        }
    }

    /* F-12 Fix: Cast to 64-bit to prevent 32-bit overflow when saved > 42,949,672 */
    if (out_snap->total_raw_tokens > 0 &&
        out_snap->active_tokens_after_prune <= out_snap->total_raw_tokens) {
        uint64_t saved = (uint64_t)out_snap->total_raw_tokens - (uint64_t)out_snap->active_tokens_after_prune;
        out_snap->tokens_saved_pct = (uint32_t)((saved * 100ULL) / (uint64_t)out_snap->total_raw_tokens);
    }

    out_snap->cognitive_friction_index = (friction_count > 0)
                                       ? (friction_sum / (double)friction_count)
                                       : 0.0;
}

#ifdef NIBRAS_STANDALONE_TEST
#include <assert.h>

int main(void) {
    /* Test 1: Arabic UTF-8 Normalization with both 0xD9 and 0xDB digits */
    const char *raw_arabic = "إدارة الأزمات في دبي ٢٠٢٧ و ۰۹۸۷۶ - مدرسةٌ ذكية";
    char norm_buf[256];
    arabic_normalize_utf8_hardened(raw_arabic, norm_buf, sizeof(norm_buf));

    /* Check that both Eastern digits (2027) and Extended digits (09876) folded to ASCII */
    assert(strstr(norm_buf, "2027") != NULL);
    assert(strstr(norm_buf, "09876") != NULL);

    /* Test 2: Multi-day memory consolidation */
    nibras_memory_node_t nodes[2] = {
        {
            .id = 101,
            .summary = "Routine CI build status check",
            .valence = 0,
            .initial_intensity = 0.45,
            .current_intensity = 0.45,
            .created_utc = 1792600000ULL,
            .token_cost = 450,
            .valence_inverted = false,
            .state = TRAJ_ACTIVE_EPISODIC
        },
        {
            .id = 102,
            .summary = "Critical DIFC statutory registration mandate",
            .valence = 1,
            .initial_intensity = 0.95,
            .current_intensity = 0.95,
            .created_utc = 1792600000ULL,
            .token_cost = 850,
            .valence_inverted = false,
            .state = TRAJ_ACTIVE_EPISODIC
        }
    };

    nibras_telemetry_snapshot_t snap;
    /* Simulate 7 days forward with 10080x scale */
    nibras_fademem_step_hardened(
        nodes, 2, 1792600000ULL + (7 * 86400ULL), 86400.0, 1.0, 3 * 86400.0, &snap
    );

    /* Node 101 decayed below 0.25 -> PRUNED */
    assert(nodes[0].state == TRAJ_NOISE_PRUNED);
    /* Node 102 passed consolidation window and initial_intensity >= 0.80 -> DISTILLED_RULE */
    assert(nodes[1].state == TRAJ_DISTILLED_RULE);
    assert(snap.distilled_rules_count == 1);
    assert(snap.tokens_saved_pct >= 85);

    printf("[PASS] All Nibras C99 Hardened Arabic Normalizer & FadeMem tests passed! (Tokens saved: %u%%)\n", snap.tokens_saved_pct);
    return 0;
}
#endif
