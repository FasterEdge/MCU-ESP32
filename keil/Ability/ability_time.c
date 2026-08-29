// ability_time.c — TimeAbility 实现（Keil/裸机 C 版）
// sync_net / sync_manual / sync_system / sync_ntp / get_time / configure_run
// 时间读写通过 fe_port 抽象（移植层实现 RTC / 系统节拍）。
#include "fe_ability.h"
#include "fe_port.h"

fe_output_t ability_time_dispatch(void *inst, const char *act, const char *args) {
    time_ability_t *self = (time_ability_t *)inst;

    if (strcmp(act, "get_time") == 0) {
        uint64_t now = fe_port_time_now();
        char out[64];
        snprintf(out, sizeof(out), "{\"epoch\":%llu}", (unsigned long long)now);
        return fe_ok(act, out);
    }
    if (strcmp(act, "sync_manual") == 0) {
        if (!args || args[0] == 0)
            return fe_err(act, "missing epoch");
        unsigned long long ep = strtoull(args, NULL, 10);
        if (ep == 0) return fe_err(act, "invalid epoch");
        fe_port_time_set(ep);
        self->manual_epoch = ep;
        char out[64];
        snprintf(out, sizeof(out), "epoch=%llu", ep);
        return fe_ok(act, out);
    }
    if (strcmp(act, "sync_system") == 0) {
        uint64_t now = fe_port_time_now();
        char out[64];
        snprintf(out, sizeof(out), "epoch=%llu", (unsigned long long)now);
        return fe_ok(act, out);
    }
    if (strcmp(act, "sync_ntp") == 0) {
        const char *server = (args && args[0]) ? args : NULL;
        int rc = fe_port_time_sync_ntp(server);
        if (rc != 0) return fe_err(act, "ntp sync failed");
        return fe_ok(act, "ntp synced");
    }
    if (strcmp(act, "configure_run") == 0) {
        // TODO: 配置周期校时（如每 3600s 调 sync_ntp），在 main 调度中实现
        return fe_ok(act, "configured");
    }
    return fe_err(act, "unsupported command");
}