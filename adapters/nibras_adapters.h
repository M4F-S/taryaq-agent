/* nibras_adapters.h - UTF-8 Arabic Normalizer & FadeMem Engine for Nibras Executive
 * Target: Best Personal AI Agent | Dubai Create AI Agents Championship 2026-2027
 */
#ifndef NIBRAS_ADAPTERS_H
#define NIBRAS_ADAPTERS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Memory trajectory node state */
typedef enum {
    TRAJ_NOISE_PRUNED      = 0, /* Decayed below I < 0.25 (pruned from prompt) */
    TRAJ_ACTIVE_EPISODIC   = 1, /* Active working context */
    TRAJ_DISTILLED_RULE    = 2, /* Consolidated into permanent semantic rule */
    TRAJ_RECOVERY_VECTOR   = 3  /* Valence inversion recovery vector */
} nibras_traj_state_t;

/* Episodic/Semantic memory node */
typedef struct {
    uint64_t            id;
    char                summary[128];
    int32_t             valence;         /* -1 (Thorn/Friction), 0 (Neutral), +1 (Rose/Success) */
    double              initial_intensity; /* I_0 in [0.0, 1.0] */
    double              current_intensity; /* Decayed intensity */
    uint64_t            created_utc;
    uint32_t            token_cost;
    bool                valence_inverted;
    nibras_traj_state_t state;
} nibras_memory_node_t;

/* Live HUD Telemetry Snapshot */
typedef struct {
    uint32_t total_raw_tokens;
    uint32_t active_tokens_after_prune;
    uint32_t tokens_saved_pct;
    uint32_t distilled_rules_count;
    double   cognitive_friction_index;
} nibras_telemetry_snapshot_t;

/* Zero-allocation UTF-8 Arabic and Gulf Dialect Normalizer */
size_t arabic_normalize_utf8_hardened(const char *src, char *dst, size_t dst_cap);

/* Time-accelerated FadeMem step function */
void nibras_fademem_step_hardened(
    nibras_memory_node_t        *nodes,
    size_t                       count,
    uint64_t                     now_utc,
    double                       half_life_sec,
    double                       sim_time_scale,
    double                       consolidation_window_sec,
    nibras_telemetry_snapshot_t *out_snap
);

#ifdef __cplusplus
}
#endif

#endif /* NIBRAS_ADAPTERS_H */
