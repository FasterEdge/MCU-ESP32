// ─────────────────────────────────────────────────────────────
// FasterEdge 开源项目
// Github: https://github.com/FasterEdge
// Gitee:  https://gitee.com/FasterEdge
// ─────────────────────────────────────────────────────────────
// ability_base.c — BaseAbility 实现（Keil/裸机 C 版）
// list_data_names / list_ability_names
#include "fe_ability.h"

fe_output_t ability_base_dispatch(void *inst, const char *act, const char *args) {
    (void)inst;
    char names[512];

    if (strcmp(act, "list_data_names") == 0) {
        fe_list_data_names(fe_global_atom(), names, sizeof(names));
        char out[560];
        snprintf(out, sizeof(out), "{\"names\":[%s]}", names);
        return fe_ok(act, out);
    }
    if (strcmp(act, "list_ability_names") == 0) {
        fe_list_ability_names(fe_global_atom(), names, sizeof(names));
        char out[560];
        snprintf(out, sizeof(out), "{\"names\":[%s]}", names);
        return fe_ok(act, out);
    }
    return fe_err(act, "unsupported command");
}