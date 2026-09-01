// ─────────────────────────────────────────────────────────────
// FasterEdge 开源项目
// Github: https://github.com/FasterEdge
// Gitee:  https://gitee.com/FasterEdge
// ─────────────────────────────────────────────────────────────
// ability_configfile.c — ConfigFileAbility 实现（Keil/裸机 C 版）
// load / save / set_path / get_path / exists
#include "fe_ability.h"
#include "fe_port.h"

// NVS key 规范化（替换 '/' 为 '_'，限长）
static void norm_key(const char *in, char *out, size_t outlen) {
    size_t n = 0;
    for (const char *p = in; *p && n + 1 < outlen; p++) {
        out[n++] = (*p == '/') ? '_' : *p;
    }
    out[n] = 0;
}

fe_output_t ability_configfile_dispatch(void *inst, const char *act, const char *args) {
    configfile_ability_t *self = (configfile_ability_t *)inst;

    if (strcmp(act, "set_path") == 0) {
        if (!args || !args[0]) return fe_err(act, "missing path");
        snprintf(self->path, sizeof(self->path), "%s", args);
        char out[64];
        snprintf(out, sizeof(out), "path=%s", self->path);
        return fe_ok(act, out);
    }
    if (strcmp(act, "get_path") == 0) {
        char out[64];
        snprintf(out, sizeof(out), "path=%s", self->path);
        return fe_ok(act, out);
    }
    if (strcmp(act, "exists") == 0) {
        char key[32];
        norm_key(args ? args : "", key, sizeof(key));
        char buf[64];
        bool found = fe_port_nvs_get_str(self->path, key, buf, sizeof(buf));
        char out[64];
        snprintf(out, sizeof(out), "{\"exists\":%s}", found ? "true" : "false");
        return fe_ok(act, out);
    }
    if (strcmp(act, "load") == 0) {
        char key[32];
        norm_key(args ? args : "", key, sizeof(key));
        char val[128];
        bool found = fe_port_nvs_get_str(self->path, key, val, sizeof(val));
        if (!found) return fe_ok(act, "{}");
        char out[192];
        snprintf(out, sizeof(out), "{\"%s\":\"%s\"}", key, val);
        return fe_ok(act, out);
    }
    if (strcmp(act, "save") == 0) {
        if (!args || !args[0]) return fe_err(act, "bad format, expect key=value");
        char buf[160];
        snprintf(buf, sizeof(buf), "%s", args);
        char *eq = strchr(buf, '=');
        if (!eq) return fe_err(act, "bad format, expect key=value");
        *eq = 0;
        char key[32];
        norm_key(buf, key, sizeof(key));
        fe_port_nvs_set_str(self->path, key, eq + 1);
        return fe_ok(act, "saved");
    }
    return fe_err(act, "unsupported command");
}