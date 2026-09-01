// ─────────────────────────────────────────────────────────────
// FasterEdge 开源项目
// Github: https://github.com/FasterEdge
// Gitee:  https://gitee.com/FasterEdge
// ─────────────────────────────────────────────────────────────
// ability_serial.c — SerialAbility 实现（Keil/裸机 C 版）
// open / close / write / read / is_open / set_config / get_config / list_ports
#include "fe_ability.h"
#include "fe_port.h"

fe_output_t ability_serial_dispatch(void *inst, const char *act, const char *args) {
    serial_ability_t *self = (serial_ability_t *)inst;

    if (strcmp(act, "list_ports") == 0) {
        return fe_ok(act, "{\"ports\":[0,1,2]}");
    }
    if (strcmp(act, "set_config") == 0) {
        // 参数：port,baud（逗号分隔，可省略）
        uint8_t port = self->port;
        uint32_t baud = self->baud;
        if (args && args[0]) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%s", args);
            char *comma = strchr(buf, ',');
            port = (uint8_t)atoi(buf);
            if (comma) baud = (uint32_t)atoi(comma + 1);
        }
        self->port = port;
        self->baud = baud;
        char out[64];
        snprintf(out, sizeof(out), "port=%u baud=%lu", port, (unsigned long)baud);
        return fe_ok(act, out);
    }
    if (strcmp(act, "get_config") == 0) {
        char out[96];
        snprintf(out, sizeof(out), "{\"open\":%s,\"baud\":%lu,\"port\":%u}",
                 self->open ? "true" : "false", (unsigned long)self->baud, self->port);
        return fe_ok(act, out);
    }
    if (strcmp(act, "open") == 0) {
        uint8_t port = self->port;
        if (args && args[0]) port = (uint8_t)atoi(args);
        fe_port_uart_init(port, self->baud, NULL, NULL);
        self->port = port;
        self->open = true;
        char out[48];
        snprintf(out, sizeof(out), "port=%u opened", port);
        return fe_ok(act, out);
    }
    if (strcmp(act, "close") == 0) {
        fe_port_uart_close(self->port);
        self->open = false;
        return fe_ok(act, "closed");
    }
    if (strcmp(act, "is_open") == 0) {
        char out[32];
        snprintf(out, sizeof(out), "{\"open\":%s}", self->open ? "true" : "false");
        return fe_ok(act, out);
    }
    if (strcmp(act, "write") == 0) {
        if (!self->open) return fe_err(act, "port not open");
        size_t n = fe_port_uart_write(self->port, (const uint8_t *)(args ? args : ""),
                                      args ? strlen(args) : 0);
        char out[32];
        snprintf(out, sizeof(out), "bytes=%u", (unsigned)n);
        return fe_ok(act, out);
    }
    if (strcmp(act, "read") == 0) {
        if (!self->open) return fe_err(act, "port not open");
        char hex[128];
        size_t n = 0;
        while (fe_port_uart_available(self->port) && n + 2 < sizeof(hex)) {
            int b = fe_port_uart_read(self->port);
            if (b < 0) break;
            snprintf(hex + n, sizeof(hex) - n, "%02X", b & 0xff);
            n += 2;
        }
        if (n == 0) return fe_ok(act, "");
        return fe_ok(act, hex);
    }
    return fe_err(act, "unsupported command");
}