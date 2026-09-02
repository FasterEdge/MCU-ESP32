// FasterEdge 开源项目 - Github: https://github.com/FasterEdge - Gitee: https://gitee.com/FasterEdge
#include "fe_ability.h"
#include "fe_port.h"

#define MQTT_MAX_PACKET 768

static int put_u16(uint8_t *b, size_t cap, size_t *n, uint16_t v) {
    if (*n + 2 > cap) return -1;
    b[(*n)++] = (uint8_t)(v >> 8); b[(*n)++] = (uint8_t)v; return 0;
}
static int put_bytes(uint8_t *b, size_t cap, size_t *n, const uint8_t *p, size_t len) {
    if (*n + len > cap) return -1;
    if (len) memcpy(b + *n, p, len); *n += len; return 0;
}
static int put_str(uint8_t *b, size_t cap, size_t *n, const char *s) {
    size_t len = s ? strlen(s) : 0;
    if (len > 65535 || put_u16(b, cap, n, (uint16_t)len) != 0) return -1;
    return put_bytes(b, cap, n, (const uint8_t *)(s ? s : ""), len);
}
static int put_rl(uint8_t *b, size_t cap, size_t *n, size_t v) {
    do { uint8_t d = (uint8_t)(v % 128); v /= 128; if (v) d |= 0x80;
         if (*n >= cap) return -1; b[(*n)++] = d; } while (v);
    return 0;
}
static int packet(uint8_t type, const uint8_t *body, size_t blen, uint8_t *out, size_t cap) {
    size_t n = 0;
    if (blen > 268435455u || cap < 2 || put_bytes(out, cap, &n, &type, 1) != 0) return -1;
    if (put_rl(out, cap, &n, blen) != 0 || put_bytes(out, cap, &n, body, blen) != 0) return -1;
    return (int)n;
}
static int send_packet(uint8_t type, const uint8_t *body, size_t blen) {
    static uint8_t out[MQTT_MAX_PACKET]; int n = packet(type, body, blen, out, sizeof(out));
    if (n < 0 || fe_port_tcp_write(out, (size_t)n) != (size_t)n) return -1;
    return 0;
}
static int parse_host(const char *broker, char *host, size_t cap, uint16_t *port) {
    const char *c; unsigned long p = 1883;
    if (!broker || !host || cap == 0) return -1;
    if (strlen(broker) >= cap) return -1; strcpy(host, broker);
    c = strrchr(host, ':');
    if (c && c != host) { char *end; p = strtoul(c + 1, &end, 10); if (*end || p < 1 || p > 65535) return -1; *((char *)c) = 0; }
    if (!host[0]) return -1; *port = (uint16_t)p; return 0;
}
static int parse_packet(mqtt_ability_t *s, const uint8_t *p, size_t len) {
    size_t i = 1, rl = 0, mult = 1, body;
    uint8_t d, type;
    if (len < 2) return 0;
    type = (uint8_t)(p[0] >> 4);
    do { if (i >= len || i > 5) return -1; d = p[i++]; rl += (size_t)(d & 127) * mult; mult *= 128; } while (d & 128);
    if (rl > MQTT_MAX_PACKET || i + rl > len) return 0;
    body = i;
    if (type == 2) { if (rl < 2 || p[body] != 0 || p[body + 1] != 0) return -1; s->connected = true; s->last_event = 2; }
    else if (type == 3) {
        size_t q = body, end = body + rl; uint16_t tl; int qos = (p[0] >> 1) & 3;
        if (q + 2 > end) return -1; tl = ((uint16_t)p[q] << 8) | p[q + 1]; q += 2;
        if (!tl || tl >= sizeof(s->last_topic) || q + tl > end) return -1;
        memcpy(s->last_topic, p + q, tl); s->last_topic[tl] = 0; q += tl;
        if (qos) { if (q + 2 > end) return -1; q += 2; }
        if (end - q >= sizeof(s->last_payload)) return -1;
        memcpy(s->last_payload, p + q, end - q); s->last_payload[end - q] = 0; s->last_payload_len = end - q; s->last_event = 3;
    } else if (type == 4 || type == 9 || type == 11) { if (rl < 2) return -1; s->last_packet_id = ((uint16_t)p[body] << 8) | p[body + 1]; s->last_event = type; }
    else if (type == 13) s->last_event = 13;
    else if (type == 14) s->connected = false;
    return (int)(i + rl);
}
static int read_parse(mqtt_ability_t *s) {
    if (s->rx_len >= sizeof(s->rx_buf)) { s->connected = false; s->rx_len = 0; return -1; }
    int n = fe_port_tcp_read(s->rx_buf + s->rx_len, sizeof(s->rx_buf) - s->rx_len);
    if (n < 0) { s->connected = false; return -1; }
    if (n == 0) return 0;
    s->rx_len += (size_t)n;
    for (;;) { int used = parse_packet(s, s->rx_buf, s->rx_len); if (used <= 0) { if (used < 0) { s->connected = false; s->rx_len = 0; return -1; } break; } if ((size_t)used < s->rx_len) memmove(s->rx_buf, s->rx_buf + used, s->rx_len - (size_t)used); s->rx_len -= (size_t)used; }
    return n;
}

fe_output_t ability_mqtt_dispatch(void *inst, const char *act, const char *args) {
    mqtt_ability_t *s = (mqtt_ability_t *)inst; static uint8_t body[MQTT_MAX_PACKET], pkt[MQTT_MAX_PACKET]; size_t n; int rc;
    if (strcmp(act, "set_broker") == 0) { if (!args || !args[0] || strlen(args) >= sizeof(s->broker)) return fe_err(act, "invalid broker"); strcpy(s->broker, args); return fe_ok(act, s->broker); }
    if (strcmp(act, "connect") == 0) {
        char host[96]; uint16_t port; if (parse_host(s->broker, host, sizeof(host), &port) != 0) return fe_err(act, "invalid broker");
        if (!s->client_id[0]) strcpy(s->client_id, "FasterEdge-MCU"); if (strlen(s->client_id) > 65535) return fe_err(act, "client id too long");
        if (fe_port_tcp_connect(host, port) != 0) return fe_err(act, "tcp connect failed");
        n = 0; if (put_str(body, sizeof(body), &n, "MQTT") || put_bytes(body, sizeof(body), &n, (const uint8_t *)"\x04\x02\x00\x3c", 4) || put_str(body, sizeof(body), &n, s->client_id) || send_packet(0x10, body, n) != 0) { fe_port_tcp_close(); return fe_err(act, "connect packet failed"); }
        s->connected = false; s->rx_len = 0;
        for (uint8_t tries = 0; tries < 50 && !s->connected; ++tries) {
            rc = read_parse(s);
            if (rc < 0) break;
            if (!s->connected) fe_port_delay_ms(10);
        }
        if (!s->connected) { fe_port_tcp_close(); return fe_err(act, "connack timeout or rejected"); }
        return fe_ok(act, "{\"connected\":true}");
    }
    if (strcmp(act, "disconnect") == 0) { if (s->connected) (void)send_packet(0xe0, NULL, 0); fe_port_tcp_close(); s->connected = false; return fe_ok(act, "{\"connected\":false}"); }
    if (strcmp(act, "is_connected") == 0) return fe_ok(act, s->connected ? "{\"connected\":true}" : "{\"connected\":false}");
    if (strcmp(act, "publish") == 0) { char b[512], *c; if (!s->connected || !args || strlen(args) >= sizeof(b)) return fe_err(act, "not connected or bad format"); strcpy(b, args); c = strchr(b, ','); if (!c || c == b) return fe_err(act, "expect topic,payload"); *c++ = 0; n = 0; if (put_str(body, sizeof(body), &n, b) || put_bytes(body, sizeof(body), &n, (const uint8_t *)c, strlen(c)) || send_packet(0x30, body, n) != 0) return fe_err(act, "publish failed"); snprintf((char *)pkt, sizeof(pkt), "{\"published\":true,\"topic\":\"%s\"}", b); return fe_ok(act, (const char *)pkt); }
    if (strcmp(act, "subscribe") == 0 || strcmp(act, "unsubscribe") == 0) { uint8_t type = strcmp(act, "subscribe") == 0 ? 0x82 : 0xa2; if (!s->connected || !args || !args[0] || strlen(args) >= 64 || s->sub_count >= 4) return fe_err(act, "bad subscription"); s->next_packet_id = (uint16_t)(s->next_packet_id + 1); if (!s->next_packet_id) s->next_packet_id = 1; n = 0; if (put_u16(body, sizeof(body), &n, s->next_packet_id) || put_str(body, sizeof(body), &n, args) || (type == 0x82 && put_bytes(body, sizeof(body), &n, (const uint8_t *)"\x00", 1)) || send_packet(type, body, n) != 0) return fe_err(act, "subscription send failed"); if (type == 0x82) { strcpy(s->subs[s->sub_count++], args); return fe_ok(act, "{\"subscribed\":true}"); } { uint8_t i; for (i=0;i<s->sub_count;i++) if (!strcmp(s->subs[i],args)) { for (;i+1<s->sub_count;i++) strcpy(s->subs[i],s->subs[i+1]); s->sub_count--; break; } } return fe_ok(act, "{\"unsubscribed\":true}"); }
    if (strcmp(act, "list_subscriptions") == 0) { n = 0; pkt[n++]='['; for (uint8_t i=0;i<s->sub_count;i++) { int w=snprintf((char *)pkt+n,sizeof(pkt)-n,"%s\"%s\"",i?",":"",s->subs[i]); if(w<0 || (size_t)w>=sizeof(pkt)-n) return fe_err(act,"output too large"); n+=(size_t)w; } pkt[n++]=']'; pkt[n]=0; return fe_ok(act,(char *)pkt); }
    if (strcmp(act, "drain") == 0) { rc = read_parse(s); if (rc < 0) return fe_err(act,"connection closed"); if (s->last_event == 3) { snprintf((char *)pkt,sizeof(pkt),"{\"topic\":\"%s\",\"payload\":\"%s\"}",s->last_topic,s->last_payload); return fe_ok(act,(char *)pkt); } snprintf((char *)pkt,sizeof(pkt),"{\"drained\":true,\"bytes\":%d,\"event\":%u}",rc,s->last_event); return fe_ok(act,(char *)pkt); }
    return fe_err(act, "unsupported command");
}
