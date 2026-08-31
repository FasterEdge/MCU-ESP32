// ability_time.c — TimeAbility 实现（Keil/裸机 C 版）
// configure_run 保存周期校时配置；实际调度器可读取该状态后调用 sync_ntp。
#include "fe_ability.h"
#include "fe_port.h"

#define TIME_NS "fe_time"

fe_output_t ability_time_dispatch(void *inst, const char *act, const char *args) {
    time_ability_t *self = (time_ability_t *)inst;
    if (strcmp(act, "get_time") == 0) {
        char out[64]; snprintf(out, sizeof(out), "{\"epoch\":%llu}", (unsigned long long)fe_port_time_now());
        return fe_ok(act, out);
    }
    if (strcmp(act, "sync_manual") == 0) {
        if (!args || !args[0]) return fe_err(act, "missing epoch");
        char *end = NULL; unsigned long long ep = strtoull(args, &end, 10);
        if (ep == 0 || !end || *end) return fe_err(act, "invalid epoch");
        fe_port_time_set(ep); self->manual_epoch = ep;
        char out[64]; snprintf(out, sizeof(out), "epoch=%llu", ep); return fe_ok(act, out);
    }
    if (strcmp(act, "sync_system") == 0) {
        char out[64]; snprintf(out, sizeof(out), "epoch=%llu", (unsigned long long)fe_port_time_now());
        return fe_ok(act, out);
    }
    if (strcmp(act, "sync_net") == 0 || strcmp(act, "sync_ntp") == 0) {
        const char *server = (args && args[0]) ? args : NULL;
        if (fe_port_time_sync_ntp(server) != 0) return fe_err(act, "ntp sync failed");
        return fe_ok(act, "ntp synced");
    }
    if (strcmp(act, "configure_run") == 0) {
        uint32_t interval = 0; char server[64] = {0};
        (void)fe_port_nvs_get_u32(TIME_NS, "interval", &interval);
        (void)fe_port_nvs_get_str(TIME_NS, "server", server, sizeof(server));
        if (args && args[0]) {
            if (strcmp(args, "off") == 0 || strcmp(args, "0") == 0) {
                interval = 0; server[0] = 0;
            } else {
                char cfg[96]; snprintf(cfg, sizeof(cfg), "%s", args);
                char *comma = strchr(cfg, ','); if (comma) { *comma = 0; snprintf(server, sizeof(server), "%s", comma + 1); }
                char *end = NULL; unsigned long v = strtoul(cfg, &end, 10);
                if (!end || *end || v < 10 || v > 86400UL * 30UL) return fe_err(act, "expect off or interval[10..2592000][,server]");
                interval = (uint32_t)v;
            }
            if (!fe_port_nvs_set_u32(TIME_NS, "interval", interval)) return fe_err(act, "store failed");
            if (server[0]) { if (!fe_port_nvs_set_str(TIME_NS, "server", server)) return fe_err(act, "store failed"); }
            else (void)fe_port_nvs_remove(TIME_NS, "server");
        }
        char out[128];
        snprintf(out, sizeof(out), "{\"enabled\":%s,\"interval\":%lu,\"server\":\"%s\"}",
                 interval ? "true" : "false", (unsigned long)interval, server);
        return fe_ok(act, out);
    }
    return fe_err(act, "unsupported command");
}
