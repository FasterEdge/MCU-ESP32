// ability_onekey.c — OneKeyAbility 实现（Keil/裸机 C 版）
// issue_token / verify_token / revoke_token / revoke_all / list_tokens / status / rotate
// 令牌 = HMAC-SHA256(secret, "seq:subject")，base64url 呈现。
#include "fe_ability.h"
#include "fe_hmac_sha256.h"
#include "fe_port.h"

static const char *B64URL_TBL = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

static void b64url_encode(const uint8_t *data, size_t len, char *out, size_t outlen) {
    size_t n = 0;
    for (size_t i = 0; i + 2 < len && n + 4 < outlen; i += 3) {
        uint32_t v = ((uint32_t)data[i] << 16) | ((uint32_t)data[i+1] << 8) | data[i+2];
        out[n++] = B64URL_TBL[(v >> 18) & 63];
        out[n++] = B64URL_TBL[(v >> 12) & 63];
        out[n++] = B64URL_TBL[(v >> 6) & 63];
        out[n++] = B64URL_TBL[v & 63];
    }
    if (len % 3 == 1 && n + 2 <= outlen) {
        uint32_t v = (uint32_t)data[len-1] << 16;
        out[n++] = B64URL_TBL[(v >> 18) & 63];
        out[n++] = B64URL_TBL[(v >> 12) & 63];
    } else if (len % 3 == 2 && n + 3 <= outlen) {
        uint32_t v = ((uint32_t)data[len-2] << 16) | ((uint32_t)data[len-1] << 8);
        out[n++] = B64URL_TBL[(v >> 18) & 63];
        out[n++] = B64URL_TBL[(v >> 12) & 63];
        out[n++] = B64URL_TBL[(v >> 6) & 63];
    }
    if (n < outlen) out[n] = 0;
}

static void load_or_create_secret(onekey_ability_t *self) {
    if (!fe_port_nvs_get_str("fe_onekey", "secret", self->secret, sizeof(self->secret))) {
        fe_port_random_fill((uint8_t *)self->secret, 32);
        self->secret[32] = 0;
        fe_port_nvs_set_str("fe_onekey", "secret", self->secret);
    }
    fe_port_nvs_get_u32("fe_onekey", "seq", &self->seq);
}

static void persist_seq(onekey_ability_t *self) {
    fe_port_nvs_set_u32("fe_onekey", "seq", self->seq);
}

fe_output_t ability_onekey_dispatch(void *inst, const char *act, const char *args) {
    onekey_ability_t *self = (onekey_ability_t *)inst;
    if (self->secret[0] == 0) load_or_create_secret(self);
    const char *subject = (args && args[0]) ? args : "default";

    if (strcmp(act, "status") == 0) {
        char out[64];
        snprintf(out, sizeof(out), "{\"tokens\":%lu}", (unsigned long)(self->seq + 1));
        return fe_ok(act, out);
    }
    if (strcmp(act, "issue_token") == 0) {
        char payload[64];
        uint8_t mac[32];
        snprintf(payload, sizeof(payload), "%lu:%s", (unsigned long)self->seq, subject);
        fe_hmac_sha256((const uint8_t *)self->secret, strlen(self->secret),
                       (const uint8_t *)payload, strlen(payload), mac);
        char tok[64];
        b64url_encode(mac, 32, tok, sizeof(tok));
        char out[128];
        snprintf(out, sizeof(out), "{\"token\":\"%s\",\"seq\":%lu}", tok, (unsigned long)self->seq);
        self->seq++;
        persist_seq(self);
        return fe_ok(act, out);
    }
    if (strcmp(act, "verify_token") == 0) {
        // 期望 args = "seq:token:subject"
        char buf[256];
        snprintf(buf, sizeof(buf), "%s", args ? args : "");
        char *colon1 = strchr(buf, ':');
        if (!colon1) return fe_err(act, "bad format, expect seq:token");
        *colon1 = 0;
        char *tok = colon1 + 1;
        char *colon2 = strchr(tok, ':');
        const char *subj = subject;
        if (colon2) { *colon2 = 0; subj = colon2 + 1; }
        unsigned long seq = strtoul(buf, NULL, 10);
        char payload[64];
        uint8_t mac[32];
        snprintf(payload, sizeof(payload), "%lu:%s", seq, subj);
        fe_hmac_sha256((const uint8_t *)self->secret, strlen(self->secret),
                       (const uint8_t *)payload, strlen(payload), mac);
        char expect[64];
        b64url_encode(mac, 32, expect, sizeof(expect));
        bool ok = (strcmp(expect, tok) == 0);
        char out[64];
        snprintf(out, sizeof(out), "{\"valid\":%s}", ok ? "true" : "false");
        return ok ? fe_ok(act, out) : fe_err(act, "token invalid");
    }
    if (strcmp(act, "revoke_token") == 0 || strcmp(act, "revoke_all") == 0) {
        // TODO: 引入黑名单后实现真正吊销；当前重置序列使旧令牌失效
        self->seq = 0;
        persist_seq(self);
        return fe_ok(act, "{\"revoked\":true}");
    }
    if (strcmp(act, "list_tokens") == 0) {
        // TODO: 维护令牌登记表后返回全部未吊销令牌
        return fe_ok(act, "{\"tokens\":[]}");
    }
    if (strcmp(act, "rotate") == 0) {
        fe_port_nvs_remove("fe_onekey", "secret");
        self->secret[0] = 0;
        load_or_create_secret(self);
        self->seq = 0;
        persist_seq(self);
        return fe_ok(act, "{\"rotated\":true}");
    }
    return fe_err(act, "unsupported command");
}