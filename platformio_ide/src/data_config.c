// data_config.c — ConfigData 实现（Keil/裸机 C 版）
// 扁平点号路径 KV 配置：get / set / delete / list / snapshot
// 使用保留键 __keys 维护可移植索引，不依赖特定 SDK 的 NVS 枚举 API。
#include "fe_data.h"
#include "fe_port.h"

#define FE_CONFIG_INDEX "__keys"

static void norm_key(const char *in, char *out, size_t outlen) {
    size_t n = 0;
    for (const char *p = in; *p && n + 1 < outlen; p++) {
        char c = *p;
        if (c == '.' || c == '/') c = '_';
        out[n++] = c;
    }
    out[n] = 0;
}

static bool valid_key(const char *key) {
    return key && key[0] && strlen(key) < 32 && strcmp(key, FE_CONFIG_INDEX) != 0 &&
           strchr(key, '\n') == NULL && strchr(key, '\r') == NULL;
}

static bool index_has(const char *index, const char *key) {
    size_t klen = strlen(key);
    const char *p = index;
    while (*p) {
        const char *end = strchr(p, '\n');
        size_t len = end ? (size_t)(end - p) : strlen(p);
        if (len == klen && memcmp(p, key, len) == 0) return true;
        if (!end) break;
        p = end + 1;
    }
    return false;
}

static bool index_add(const char *ns, const char *key) {
    char index[192] = {0};
    (void)fe_port_nvs_get_str(ns, FE_CONFIG_INDEX, index, sizeof(index));
    if (index_has(index, key)) return true;
    size_t used = strlen(index), need = strlen(key) + (used ? 1u : 0u);
    if (used + need >= sizeof(index)) return false;
    if (used) index[used++] = '\n';
    snprintf(index + used, sizeof(index) - used, "%s", key);
    return fe_port_nvs_set_str(ns, FE_CONFIG_INDEX, index);
}

static void index_remove(const char *ns, const char *key) {
    char index[192] = {0}, next[192] = {0};
    if (!fe_port_nvs_get_str(ns, FE_CONFIG_INDEX, index, sizeof(index))) return;
    const char *p = index;
    size_t used = 0, klen = strlen(key);
    while (*p) {
        const char *end = strchr(p, '\n');
        size_t len = end ? (size_t)(end - p) : strlen(p);
        if (!(len == klen && memcmp(p, key, len) == 0) && used + len + 1 < sizeof(next)) {
            if (used) next[used++] = '\n';
            memcpy(next + used, p, len); used += len; next[used] = 0;
        }
        if (!end) break;
        p = end + 1;
    }
    (void)fe_port_nvs_set_str(ns, FE_CONFIG_INDEX, next);
}

static bool append_json(char *out, size_t cap, size_t *used, const char *s) {
    while (*s) {
        unsigned char c = (unsigned char)*s++;
        const char *esc = NULL;
        if (c == '"') esc = "\\\"";
        else if (c == '\\') esc = "\\\\";
        else if (c == '\n') esc = "\\n";
        else if (c == '\r') esc = "\\r";
        else if (c == '\t') esc = "\\t";
        if (esc) {
            size_t n = strlen(esc); if (*used + n >= cap) return false;
            memcpy(out + *used, esc, n); *used += n;
        } else {
            if (c < 0x20 || *used + 1 >= cap) return false;
            out[(*used)++] = (char)c;
        }
    }
    out[*used] = 0;
    return true;
}

fe_output_t data_config_dispatch(void *inst, const char *act, const char *args) {
    config_data_t *self = (config_data_t *)inst;
    if (strcmp(act, "get") == 0) {
        if (!valid_key(args)) return fe_err(act, "invalid or missing key");
        char key[32], val[128]; norm_key(args, key, sizeof(key));
        bool found = fe_port_nvs_get_str(self->ns, key, val, sizeof(val));
        char out[192]; snprintf(out, sizeof(out), "{\"%s\":\"%s\"}", args, found ? val : "");
        return fe_ok(act, out);
    }
    if (strcmp(act, "set") == 0) {
        if (!args || !args[0]) return fe_err(act, "bad format, expect key=value");
        char buf[160]; snprintf(buf, sizeof(buf), "%s", args);
        char *eq = strchr(buf, '='); if (!eq) return fe_err(act, "bad format, expect key=value");
        *eq = 0; if (!valid_key(buf)) return fe_err(act, "invalid key");
        char key[32]; norm_key(buf, key, sizeof(key));
        if (!fe_port_nvs_set_str(self->ns, key, eq + 1)) return fe_err(act, "store failed");
        if (!index_add(self->ns, buf)) { (void)fe_port_nvs_remove(self->ns, key); return fe_err(act, "key index full"); }
        return fe_ok(act, "saved");
    }
    if (strcmp(act, "delete") == 0) {
        if (!valid_key(args)) return fe_err(act, "invalid or missing key");
        char key[32]; norm_key(args, key, sizeof(key));
        if (!fe_port_nvs_remove(self->ns, key)) return fe_err(act, "delete failed");
        index_remove(self->ns, args); return fe_ok(act, "deleted");
    }
    if (strcmp(act, "list") == 0 || strcmp(act, "snapshot") == 0) {
        char index[192] = {0}; (void)fe_port_nvs_get_str(self->ns, FE_CONFIG_INDEX, index, sizeof(index));
        char out[256] = {0}; size_t used = 0;
        const bool snapshot = strcmp(act, "snapshot") == 0;
        const char *prefix = snapshot ? "{\"snapshot\":{" : "{\"keys\":[";
        snprintf(out, sizeof(out), "%s", prefix); used = strlen(out);
        const char *p = index; bool first = true;
        while (*p) {
            const char *end = strchr(p, '\n'); size_t len = end ? (size_t)(end - p) : strlen(p);
            char logical[32]; if (len >= sizeof(logical)) return fe_err(act, "corrupt key index");
            memcpy(logical, p, len); logical[len] = 0;
            char key[32], val[128] = {0}; norm_key(logical, key, sizeof(key));
            bool found = fe_port_nvs_get_str(self->ns, key, val, sizeof(val));
            if (found) {
                if (used + (first ? 0u : 1u) + 3 >= sizeof(out)) return fe_err(act, "snapshot too large");
                if (!first) out[used++] = ','; out[used++] = '"'; out[used] = 0;
                if (!append_json(out, sizeof(out), &used, logical)) return fe_err(act, "snapshot too large");
                out[used++] = '"';
                if (snapshot) {
                    out[used++] = ':'; out[used++] = '"'; out[used] = 0;
                    if (!append_json(out, sizeof(out), &used, val)) return fe_err(act, "snapshot too large");
                    out[used++] = '"';
                }
                out[used] = 0; first = false;
            }
            if (!end) break; p = end + 1;
        }
        const char *suffix = snapshot ? "}}" : "]}";
        if (used + 3 > sizeof(out)) return fe_err(act, "snapshot too large");
        snprintf(out + used, sizeof(out) - used, "%s", suffix);
        return fe_ok(act, out);
    }
    return fe_err(act, "unsupported command");
}
