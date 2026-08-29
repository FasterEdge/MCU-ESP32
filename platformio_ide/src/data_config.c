// data_config.c — ConfigData 实现（Keil/裸机 C 版）
// 扁平点号路径 KV 配置：get / set / delete / list / snapshot
#include "fe_data.h"
#include "fe_port.h"

static void norm_key(const char *in, char *out, size_t outlen) {
    size_t n = 0;
    for (const char *p = in; *p && n + 1 < outlen; p++) {
        char c = *p;
        if (c == '.' || c == '/') c = '_';
        out[n++] = c;
    }
    out[n] = 0;
}

fe_output_t data_config_dispatch(void *inst, const char *act, const char *args) {
    config_data_t *self = (config_data_t *)inst;

    if (strcmp(act, "get") == 0) {
        if (!args || !args[0]) return fe_err(act, "missing key");
        char key[32];
        norm_key(args, key, sizeof(key));
        char val[128];
        bool found = fe_port_nvs_get_str(self->ns, key, val, sizeof(val));
        char out[192];
        snprintf(out, sizeof(out), "{\"%s\":\"%s\"}", args, found ? val : "");
        return fe_ok(act, out);
    }
    if (strcmp(act, "set") == 0) {
        if (!args || !args[0]) return fe_err(act, "bad format, expect key=value");
        char buf[160];
        snprintf(buf, sizeof(buf), "%s", args);
        char *eq = strchr(buf, '=');
        if (!eq) return fe_err(act, "bad format, expect key=value");
        *eq = 0;
        char key[32];
        norm_key(buf, key, sizeof(key));
        fe_port_nvs_set_str(self->ns, key, eq + 1);
        return fe_ok(act, "saved");
    }
    if (strcmp(act, "delete") == 0) {
        if (!args || !args[0]) return fe_err(act, "missing key");
        char key[32];
        norm_key(args, key, sizeof(key));
        fe_port_nvs_remove(self->ns, key);
        return fe_ok(act, "deleted");
    }
    if (strcmp(act, "list") == 0) {
        // TODO: 完整遍历 NVS（nvs_iterator）；当前返回空表
        return fe_ok(act, "{\"keys\":[]}");
    }
    if (strcmp(act, "snapshot") == 0) {
        // TODO: 导出全部键值快照
        return fe_ok(act, "{\"snapshot\":{}}");
    }
    return fe_err(act, "unsupported command");
}