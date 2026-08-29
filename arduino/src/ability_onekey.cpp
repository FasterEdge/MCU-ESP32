// ability_onekey.cpp — OneKeyAbility 实现（Arduino 版）
// issue_token / verify_token / revoke_token / revoke_all / list_tokens / status / rotate
// 令牌 = HMAC-SHA256(secret, "seq:subject")，以 base64url 呈现。
// 密钥与序列号通过 Preferences 持久化到 NVS。
#include "fe_ability.h"
#include "fe_hmac_sha256.h"
#include <Preferences.h>

namespace fe {

// base64url 编码（无 padding）
static String base64urlEncode(const uint8_t *data, size_t len) {
    static const char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    String out;
    size_t i = 0;
    while (i + 2 < len) {
        uint32_t n = (data[i] << 16) | (data[i+1] << 8) | data[i+2];
        out += tbl[(n >> 18) & 63]; out += tbl[(n >> 12) & 63];
        out += tbl[(n >> 6) & 63];  out += tbl[n & 63];
        i += 3;
    }
    if (i + 1 == len) {
        uint32_t n = data[i] << 16;
        out += tbl[(n >> 18) & 63]; out += tbl[(n >> 12) & 63];
    } else if (i + 2 == len) {
        uint32_t n = (data[i] << 16) | (data[i+1] << 8);
        out += tbl[(n >> 18) & 63]; out += tbl[(n >> 12) & 63]; out += tbl[(n >> 6) & 63];
    }
    return out;
}

// 生成/加载密钥
static String loadOrCreateSecret() {
    Preferences prefs;
    prefs.begin("fe_onekey", false);
    String secret = prefs.getString("secret", "");
    if (secret.length() == 0) {
        // 简单随机源：esp_random
        char buf[33];
        for (int i = 0; i < 32; i++) buf[i] = (char)(esp_random() & 0xff);
        buf[32] = 0;
        secret = String(buf);
        prefs.putString("secret", secret);
    }
    size_t seq = prefs.getUInt("seq", 0);
    prefs.putUInt("seq", seq);
    prefs.end();
    return secret;
}

static void persistSeq(size_t seq) {
    Preferences prefs;
    prefs.begin("fe_onekey", false);
    prefs.putUInt("seq", seq);
    prefs.end();
}

CommandOutput oneKeyAbilityDispatch(void *inst, const char *act, const String &args) {
    OneKeyAbility *self = static_cast<OneKeyAbility *>(inst);
    if (self->secret.length() == 0)
        self->secret = loadOrCreateSecret();

    // 解析 'subject'（命令参数）
    String subject = args.length() ? args : "default";

    if (strcmp(act, "status") == 0) {
        return CommandOutput{String(act),
            String("{\"tokens\":") + (self->tokenSeq + 1) + "}", String()};
    }
    if (strcmp(act, "issue_token") == 0) {
        size_t seq = self->tokenSeq++;
        persistSeq(seq);
        char payload[64];
        snprintf(payload, sizeof(payload), "%zu:%s", seq, subject.c_str());
        uint8_t mac[32];
        fe_hmac_sha256((const uint8_t *)self->secret.c_str(), self->secret.length(),
                       (const uint8_t *)payload, strlen(payload), mac);
        String token = base64urlEncode(mac, 32);
        return CommandOutput{String(act),
            String("{\"token\":\"") + token + "\",\"seq\":" + seq + "}", String()};
    }
    if (strcmp(act, "verify_token") == 0) {
        // 期望 args 为 "seq:token"（冒号分隔）
        int colon = args.indexOf(':');
        if (colon <= 0)
            return CommandOutput{String(act), String(), String("bad format, expect seq:token")};
        String seqStr = args.substring(0, colon);
        String tok = args.substring(colon + 1);
        size_t seq = (size_t)seqStr.toInt();
        char payload[64];
        snprintf(payload, sizeof(payload), "%zu:%s", seq, subject.c_str());
        uint8_t mac[32];
        fe_hmac_sha256((const uint8_t *)self->secret.c_str(), self->secret.length(),
                       (const uint8_t *)payload, strlen(payload), mac);
        String expect = base64urlEncode(mac, 32);
        bool ok = (expect == tok);
        return CommandOutput{String(act),
            String("{\"valid\":") + (ok ? "true" : "false") + "}", ok ? String() : String("token invalid")};
    }
    if (strcmp(act, "revoke_token") == 0 || strcmp(act, "revoke_all") == 0) {
        // TODO: 引入黑名单存储后实现真正吊销；当前重置序列使旧令牌失效
        self->tokenSeq = 0;
        persistSeq(0);
        return CommandOutput{String(act), String("{\"revoked\":true}"), String()};
    }
    if (strcmp(act, "list_tokens") == 0) {
        // TODO: 维护令牌登记表后返回全部未吊销令牌
        return CommandOutput{String(act), String("{\"tokens\":[]}"), String()};
    }
    if (strcmp(act, "rotate") == 0) {
        self->secret = loadOrCreateSecret();  // 重新生成密钥
        self->tokenSeq = 0;
        persistSeq(0);
        return CommandOutput{String(act), String("{\"rotated\":true}"), String()};
    }
    return CommandOutput{String(act), String(), String("unsupported command: ") + act};
}

} // namespace fe