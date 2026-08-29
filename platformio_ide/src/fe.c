// fe.c — FasterEdge MCU 核心框架实现（Keil/裸机 C 版）
#include "fe.h"

// ============================================================
// 注册
// ============================================================
void fe_register_data(fe_atom_t *atom, const fe_module_t *mod) {
    if (atom->data_count >= FE_MAX_MODULES) return;
    atom->data[atom->data_count++] = *mod;
}

void fe_register_ability(fe_atom_t *atom, const fe_module_t *mod) {
    if (atom->ability_count >= FE_MAX_MODULES) return;
    atom->ability[atom->ability_count++] = *mod;
}

// ============================================================
// 名称列表
// ============================================================
void fe_list_data_names(const fe_atom_t *atom, char *out, size_t outlen) {
    size_t n = 0;
    out[0] = 0;
    for (size_t i = 0; i < atom->data_count && n < outlen; i++) {
        int w = snprintf(out + n, outlen - n, "%s%s", i ? "," : "", atom->data[i].name);
        if (w < 0) break;
        n += (size_t)w;
    }
}

void fe_list_ability_names(const fe_atom_t *atom, char *out, size_t outlen) {
    size_t n = 0;
    out[0] = 0;
    for (size_t i = 0; i < atom->ability_count && n < outlen; i++) {
        int w = snprintf(out + n, outlen - n, "%s%s", i ? "," : "", atom->ability[i].name);
        if (w < 0) break;
        n += (size_t)w;
    }
}

// ============================================================
// 命令路由
// ============================================================
fe_output_t fe_execute(fe_atom_t *atom, const char *target, const char *act, const char *args) {
    bool isData = strncmp(target, "data_", 5) == 0;
    const char *base = target;
    if (isData) base = target + 5;
    else if (strncmp(target, "ability_", 8) == 0) base = target + 8;

    size_t count = isData ? atom->data_count : atom->ability_count;
    const fe_module_t *mods = isData ? atom->data : atom->ability;

    for (size_t m = 0; m < count; m++) {
        if (strcmp(mods[m].name, base) == 0) {
            for (size_t c = 0; c < mods[m].cmd_count; c++) {
                if (strcmp(mods[m].cmds[c].name, act) == 0) {
                    return mods[m].dispatch(mods[m].instance, act, args);
                }
            }
            return fe_err(act, "unsupported command");
        }
    }
    return fe_err(act, isData ? "unknown data" : "unknown ability");
}

// ============================================================
// 输出工具
// ============================================================
fe_output_t fe_ok(const char *name, const char *value) {
    fe_output_t o;
    memset(&o, 0, sizeof(o));
    snprintf(o.name, sizeof(o.name), "%s", name ? name : "");
    snprintf(o.value, sizeof(o.value), "%s", value ? value : "");
    o.ok = true;
    return o;
}

fe_output_t fe_err(const char *name, const char *err) {
    fe_output_t o;
    memset(&o, 0, sizeof(o));
    snprintf(o.name, sizeof(o.name), "%s", name ? name : "");
    snprintf(o.err, sizeof(o.err), "%s", err ? err : "error");
    o.ok = false;
    return o;
}

// ============================================================
// 全局 Atom（在 register.c 中填充）
// ============================================================
static fe_atom_t g_atom;

fe_atom_t *fe_global_atom(void) { return &g_atom; }

void fe_init_all(void);  // 由 register.c 实现