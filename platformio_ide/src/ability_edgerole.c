// ability_edgerole.c — EdgeRoleAbility 实现（Keil/裸机 C 版，简化）
// describe / set_zone / get_zone / set_status / get_status /
// set_online / record_latency / get_metrics
#include "fe_ability.h"

fe_output_t ability_edgerole_dispatch(void *inst, const char *act, const char *args) {
    edgerole_ability_t *self = (edgerole_ability_t *)inst;

    if (strcmp(act, "describe") == 0) {
        char out[128];
        snprintf(out, sizeof(out), "{\"name\":\"EdgeRoleAbility\",\"zone\":\"%s\",\"online\":%s}",
                 self->zone, self->online ? "true" : "false");
        return fe_ok(act, out);
    }
    if (strcmp(act, "set_zone") == 0) {
        if (!args || !args[0]) return fe_err(act, "missing zone");
        snprintf(self->zone, sizeof(self->zone), "%s", args);
        char out[64];
        snprintf(out, sizeof(out), "zone=%s", self->zone);
        return fe_ok(act, out);
    }
    if (strcmp(act, "get_zone") == 0) {
        char out[64];
        snprintf(out, sizeof(out), "zone=%s", self->zone);
        return fe_ok(act, out);
    }
    if (strcmp(act, "set_status") == 0) {
        if (!args || (strcmp(args, "healthy") && strcmp(args, "degraded") && strcmp(args, "offline")))
            return fe_err(act, "invalid status");
        snprintf(self->status, sizeof(self->status), "%s", args);
        char out[48];
        snprintf(out, sizeof(out), "status=%s", self->status);
        return fe_ok(act, out);
    }
    if (strcmp(act, "get_status") == 0) {
        char out[48];
        snprintf(out, sizeof(out), "status=%s", self->status);
        return fe_ok(act, out);
    }
    if (strcmp(act, "set_online") == 0) {
        self->online = (args && (strcmp(args, "1") == 0 || strcmp(args, "true") == 0));
        char out[32];
        snprintf(out, sizeof(out), "{\"online\":%s}", self->online ? "true" : "false");
        return fe_ok(act, out);
    }
    if (strcmp(act, "record_latency") == 0) {
        self->latency_ms = args ? atoi(args) : 0;
        char out[48];
        snprintf(out, sizeof(out), "latency_ms=%ld", (long)self->latency_ms);
        return fe_ok(act, out);
    }
    if (strcmp(act, "get_metrics") == 0) {
        char out[96];
        snprintf(out, sizeof(out), "{\"latency_ms\":%ld,\"online\":%s}",
                 (long)self->latency_ms, self->online ? "true" : "false");
        return fe_ok(act, out);
    }
    return fe_err(act, "unsupported command");
}