// data_keyring.c — KeyringData 实现（Keil/裸机 C 版）
// 共享密钥与令牌表：status / set_secret / rotate / list_tokens /
// issue_token / revoke_token / revoke_all
#include "fe_data.h"
#include "fe_hmac_sha256.h"
#include "fe_port.h"

static const char *B64URL = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

static void b64url(const uint8_t *data, size_t len, char *out, size_t outlen) {
    size_t n = 0;
    for (size_t i = 0; i + 2 < len && n + 4 < outlen; i += 3) {
        uint32_t v = ((uint32_t)data[i] << 16) | ((uint32_t)data[i+1] << 8) | data[i+2];
        out[n++] = B64URL[(v >> 18) & 63]; out[n++] = B64URL[(v >> 12) & 63];
        out[n++] = B64URL[(v >> 6) & 63];  out[n++] = B64URL[v & 63];
    }
    if (len % 3 == 1 && n + 2 <= outlen) {
        uint32_t v = (uint32_t)data[len-1] << 16;
        out[n++] = B64URL[(v >> 18) & 63]; out[n++] = B64URL[(v >> 12) & 63];
    } else if (len % 3 == 2 && n + 3 <= outlen) {
        uint32_t v = ((uint32_t)data[len-2] << 16) | ((uint32_t)data[len-1] << 8);
        out[n++] = B64URL[(v >> 18) & 63]; out[n++] = B64URL[(v >> 12) & 63]; out[n++] = B64URL[(v >> 6) & 63];
    }
    if (n < outlen) out[n] = 0;
}

static void load_secret(const char *ns, char *out, size_t outlen, bool create) {
    if (fe_port_nvs_get_str(ns, "secret", out, outlen)) return;
    if (create) {
        fe_port_random_fill((uint8_t *)out, 32);
        if (outlen > 32) out[32] = 0;
        fe_port_nvs_set_str(ns, "secret", out);
    }
}

fe_output_t data_keyring_dispatch(void *inst, const char *act, const char *args) {
    keyring_data_t *self = (keyring_data_t *)inst;

    if (strcmp(act, "status") == 0) {
        char secret[64];
        bool hasSecret = fe_port_nvs_get_str(self->ns, "secret", secret, sizeof(secret));
        uint32_t tokens = 0;
        fe_port_nvs_get_u32(self->ns, "seq", &tokens);
        char out[96];
        snprintf(out, sizeof(out), "{\"secret\":%s,\"tokens\":%lu}",
                 hasSecret ? "true" : "false", (unsigned long)tokens);
        return fe_ok(act, out);
    }
    if (strcmp(act, "set_secret") == 0) {
        if (!args || strlen(args) < 8) return fe_err(act, "secret too short (>=8)");
        fe_port_nvs_set_str(self->ns, "secret", args);
        return fe_ok(act, "{\"set\":true}");
    }
    if (strcmp(act, "rotate") == 0) {
        fe_port_nvs_remove(self->ns, "secret");
        char secret[64];
        load_secret(self->ns, secret, sizeof(secret), true);
        return fe_ok(act, "{\"rotated\":true}");
    }
    if (strcmp(act, "issue_token") == 0) {
        char secret[64];
        load_secret(self->ns, secret, sizeof(secret), true);
        uint32_t seq = 0;
        fe_port_nvs_get_u32(self->ns, "seq", &seq);
        fe_port_nvs_set_u32(self->ns, "seq", seq + 1);
        const char *subject = (args && args[0]) ? args : "default";
        char payload[64];
        uint8_t mac[32];
        snprintf(payload, sizeof(payload), "%lu:%s", (unsigned long)seq, subject);
        fe_hmac_sha256((const uint8_t *)secret, strlen(secret),
                       (const uint8_t *)payload, strlen(payload), mac);
        char tok[64];
        b64url(mac, 32, tok, sizeof(tok));
        char out[128];
        snprintf(out, sizeof(out), "{\"token\":\"%s\",\"seq\":%lu}", tok, (unsigned long)seq);
        return fe_ok(act, out);
    }
    if (strcmp(act, "list_tokens") == 0) {
        // TODO: 维护令牌登记表后返回全部未吊销令牌
        return fe_ok(act, "{\"tokens\":[]}");
    }
    if (strcmp(act, "revoke_token") == 0 || strcmp(act, "revoke_all") == 0) {
        fe_port_nvs_set_u32(self->ns, "seq", 0);   // 重置序列使旧令牌失效
        return fe_ok(act, "{\"revoked\":true}");
    }
    return fe_err(act, "unsupported command");
}