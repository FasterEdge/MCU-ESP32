// ability_role.c — RoleAbility 实现（Keil/裸机 C 版）
// describe / set_role / get_role
#include "fe_ability.h"

fe_output_t ability_role_dispatch(void *inst, const char *act, const char *args) {
    role_ability_t *self = (role_ability_t *)inst;

    if (strcmp(act, "describe") == 0) {
        char out[128];
        snprintf(out, sizeof(out), "{\"name\":\"RoleAbility\",\"role\":\"%s\"}", self->role);
        return fe_ok(act, out);
    }
    if (strcmp(act, "set_role") == 0) {
        if (!args || args[0] == 0)
            return fe_err(act, "missing role");
        if (strcmp(args, "edge") != 0 && strcmp(args, "cloud") != 0 &&
            strcmp(args, "standalone") != 0)
            return fe_err(act, "invalid role");
        snprintf(self->role, sizeof(self->role), "%s", args);
        char out[64];
        snprintf(out, sizeof(out), "role=%s", self->role);
        return fe_ok(act, out);
    }
    if (strcmp(act, "get_role") == 0) {
        char out[64];
        snprintf(out, sizeof(out), "role=%s", self->role);
        return fe_ok(act, out);
    }
    return fe_err(act, "unsupported command");
}