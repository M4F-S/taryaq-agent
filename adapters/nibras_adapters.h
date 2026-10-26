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
    bool                is_pinned;       /* Permanent node: immune to decay */
    nibras_traj_state_t state;
    char                category[32];    /* "appointment", "action", "decision", "chatter", etc. */
    char                raw_arabic[256]; /* Original unnormalized or raw user input */
} nibras_memory_node_t;

/* Live HUD Telemetry Snapshot */
typedef struct {
    uint32_t total_raw_tokens;
    uint32_t active_tokens_after_prune;
    uint32_t tokens_saved_pct;
    uint32_t distilled_rules_count;
    double   cognitive_friction_index;
} nibras_telemetry_snapshot_t;

/* Entity types detected in normalized Arabic text */
typedef enum {
    NIBRAS_ENTITY_NONE        = 0,
    NIBRAS_ENTITY_ACTION      = 1, /* Action items (review, send, sign, follow up, etc.) */
    NIBRAS_ENTITY_APPOINTMENT = 2, /* Meetings, calls, appointments */
    NIBRAS_ENTITY_DATETIME    = 3, /* Dates, times, days of week */
    NIBRAS_ENTITY_DECISION    = 4  /* Executive decisions, policies, permanent rules */
} nibras_entity_type_t;

typedef struct {
    nibras_entity_type_t type;
    char                 label[32];   /* e.g., "ACTION", "APPOINTMENT", "DATETIME", "DECISION" */
    char                 text[96];    /* Extracted string */
    size_t               offset;      /* Byte offset in normalized text */
} nibras_entity_t;

typedef struct {
    size_t          entity_count;
    nibras_entity_t entities[16];
    bool            is_action_item;
    bool            is_appointment;
    bool            is_executive_decision;
    char            action_verb[64];
    char            appointment_title[64];
    char            datetime_str[64];
    double          suggested_salience;
    bool            suggest_pin;
    int32_t         suggested_valence;
} nibras_extracted_info_t;

/* Zero-allocation UTF-8 Arabic and Gulf Dialect Normalizer */
size_t arabic_normalize_utf8_hardened(const char *src, char *dst, size_t dst_cap);

/* Deterministic Arabic entity, appointment, and action item extractor */
size_t nibras_extract_arabic_entities(
    const char              *normalized_text,
    nibras_extracted_info_t *out_info
);

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
