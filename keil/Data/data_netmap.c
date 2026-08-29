// data_netmap.c — NetMapData 实现（Keil/裸机 C 版）
// info / set_node_name / interfaces / set_default_iface
#include "fe_data.h"
#include "fe_port.h"

fe_output_t data_netmap_dispatch(void *inst, const char *act, const char *args) {
    netmap_data_t *self = (netmap_data_t *)inst;

    if (strcmp(act, "info") == 0) {
        char ip[32];
        fe_port_wifi_ip(ip, sizeof(ip));
        char out[160];
        snprintf(out, sizeof(out),
                 "{\"node\":\"%s\",\"ip\":\"%s\",\"online\":%s}",
                 self->node_name, ip,
                 fe_port_wifi_connected() ? "true" : "false");
        return fe_ok(act, out);
    }
    if (strcmp(act, "set_node_name") == 0) {
        if (!args || !args[0]) return fe_err(act, "missing name");
        snprintf(self->node_name, sizeof(self->node_name), "%s", args);
        char out[64];
        snprintf(out, sizeof(out), "node=%s", self->node_name);
        return fe_ok(act, out);
    }
    if (strcmp(act, "interfaces") == 0) {
        char ip[32];
        fe_port_wifi_ip(ip, sizeof(ip));
        char out[160];
        snprintf(out, sizeof(out),
                 "[{\"name\":\"wlan0\",\"ip\":\"%s\",\"mac\":\"%s\"}]",
                 ip, self->node_name);
        return fe_ok(act, out);
    }
    if (strcmp(act, "set_default_iface") == 0) {
        snprintf(self->default_iface, sizeof(self->default_iface),
                 "%s", (args && args[0]) ? args : "wlan0");
        char out[48];
        snprintf(out, sizeof(out), "iface=%s", self->default_iface);
        return fe_ok(act, out);
    }
    return fe_err(act, "unsupported command");
}