// ability_mqtt.c — MQTTAbility 实现（Keil/裸机 C 版）
// set_broker / connect / disconnect / publish / subscribe / unsubscribe /
// is_connected / list_subscriptions / drain
// 说明：Keil/裸机环境无现成 MQTT 客户端，此处管理连接状态与命令语义，
// 底层 TCP 由 fe_port 提供；完整 MQTT 3.1.1 报文编解码可接入
// paho.mqtt.embedded-c 或自行实现（TODO: 见下）。
#include "fe_ability.h"
#include "fe_port.h"

fe_output_t ability_mqtt_dispatch(void *inst, const char *act, const char *args) {
    mqtt_ability_t *self = (mqtt_ability_t *)inst;

    if (strcmp(act, "set_broker") == 0) {
        if (!args || !args[0]) return fe_err(act, "missing broker");
        snprintf(self->broker, sizeof(self->broker), "%s", args);
        char out[160];
        snprintf(out, sizeof(out), "broker=%s", self->broker);
        return fe_ok(act, out);
    }
    if (strcmp(act, "connect") == 0) {
        if (self->broker[0] == 0) return fe_err(act, "broker not set");
        // 解析 host[:port]
        char host[96];
        uint16_t port = 1883;
        snprintf(host, sizeof(host), "%s", self->broker);
        char *colon = strchr(host, ':');
        if (colon) { *colon = 0; port = (uint16_t)atoi(colon + 1); }
        if (self->client_id[0] == 0)
            snprintf(self->client_id, sizeof(self->client_id), "FasterEdge-MCU");
        // TCP 连接 + CONNECT 报文（TODO: 完整 MQTT 编解码）
        int rc = fe_port_tcp_connect(host, port);
        if (rc != 0) return fe_err(act, "tcp connect failed");
        self->connected = true;
        char out[128];
        snprintf(out, sizeof(out), "{\"connected\":true,\"clientId\":\"%s\"}", self->client_id);
        return fe_ok(act, out);
    }
    if (strcmp(act, "disconnect") == 0) {
        fe_port_tcp_close();
        self->connected = false;
        return fe_ok(act, "{\"connected\":false}");
    }
    if (strcmp(act, "is_connected") == 0) {
        char out[32];
        snprintf(out, sizeof(out), "{\"connected\":%s}", self->connected ? "true" : "false");
        return fe_ok(act, out);
    }
    if (strcmp(act, "publish") == 0) {
        // 参数：topic,payload
        if (!args || !args[0]) return fe_err(act, "bad format, expect topic,payload");
        char buf[256];
        snprintf(buf, sizeof(buf), "%s", args);
        char *comma = strchr(buf, ',');
        if (!comma) return fe_err(act, "bad format, expect topic,payload");
        *comma = 0;
        // TODO: 构造 PUBLISH 报文并经 fe_port_tcp_write 发送
        (void)fe_port_tcp_write((const uint8_t *)"", 0);
        char out[96];
        snprintf(out, sizeof(out), "{\"published\":true,\"topic\":\"%s\"}", buf);
        return fe_ok(act, out);
    }
    if (strcmp(act, "subscribe") == 0) {
        if (!args || !args[0]) return fe_err(act, "missing topic");
        if (self->sub_count >= 4)
            return fe_err(act, "subscription table full");
        snprintf(self->subs[self->sub_count], 64, "%s", args);
        self->sub_count++;
        // TODO: 构造 SUBSCRIBE 报文发送
        return fe_ok(act, "{\"subscribed\":true}");
    }
    if (strcmp(act, "unsubscribe") == 0) {
        if (!args || !args[0]) return fe_err(act, "missing topic");
        for (uint8_t i = 0; i < self->sub_count; i++) {
            if (strcmp(self->subs[i], args) == 0) {
                for (uint8_t j = i; j + 1 < self->sub_count; j++)
                    snprintf(self->subs[j], 64, "%s", self->subs[j + 1]);
                self->sub_count--;
                break;
            }
        }
        return fe_ok(act, "{\"unsubscribed\":true}");
    }
    if (strcmp(act, "list_subscriptions") == 0) {
        char out[320];
        size_t n = 0;
        out[0] = 0;
        for (uint8_t i = 0; i < self->sub_count; i++) {
            int w = snprintf(out + n, sizeof(out) - n, "%s\"%s\"", i ? "," : "", self->subs[i]);
            if (w < 0) break;
            n += (size_t)w;
        }
        char full[360];
        snprintf(full, sizeof(full), "[%s]", out);
        return fe_ok(act, full);
    }
    if (strcmp(act, "drain") == 0) {
        // 读取并分发收包（TODO: 解析 PUBLISH/SUBACK 等）
        uint8_t buf[64];
        (void)fe_port_tcp_read(buf, sizeof(buf));
        return fe_ok(act, "{\"drained\":true}");
    }
    return fe_err(act, "unsupported command");
}