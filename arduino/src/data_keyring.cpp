// FasterEdge 开源项目 - Github: https://github.com/FasterEdge - Gitee: https://gitee.com/FasterEdge
// data_keyring.cpp — KeyringData 实现（Arduino 版）
// 共享密钥与令牌表：status / set_secret / rotate / list_tokens /
// issue_token / revoke_token / revoke_all
// NVS（Preferences）持久化。令牌逻辑复用 HMAC-SHA256（见 fe_hmac_sha256.h）。
#include "fe_data.h"
#include "fe_hmac_sha256.h"
#include <Preferences.h>

namespace fe {

static String b64url(const uint8_t *data, size_t len) {
    static const char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    String out;
    size_t i = 0;
    while (i + 2 < len) {
        uint32_t n = (data[i] << 16) | (data[i+1] << 8) | data[i+2];
        out += tbl[(n>>18)&63]; out += tbl[(n>>12)&63]; out += tbl[(n>>6)&63]; out += tbl[n&63];
        i += 3;
    }
    if (i + 1 == len) { uint32_t n = data[i]<<16; out += tbl[(n>>18)&63]; out += tbl[(n>>12)&63]; }
    else if (i + 2 == len) { uint32_t n = (data[i]<<16)|(data[i+1]<<8);
        out += tbl[(n>>18)&63]; out += tbl[(n>>12)&63]; out += tbl[(n>>6)&63]; }
    return out;
}

static String loadSecret(const char *ns, bool create) {
    Preferences prefs;
    prefs.begin(ns, !create);
    String s = prefs.getString("secret", "");
    if (create && s.length() == 0) {
        char buf[33];
        for (int i = 0; i < 32; i++) buf[i] = (char)(esp_random() & 0xff);
        buf[32] = 0;
        s = String(buf);
        prefs.end();
        prefs.begin(ns, false);
        prefs.putString("secret", s);
    }
    prefs.end();
    return s;
}

CommandOutput keyringDataDispatch(void *inst, const char *act, const String &args) {
    KeyringData *self = static_cast<KeyringData *>(inst);

    if (strcmp(act, "status") == 0) {
        Preferences prefs;
        prefs.begin(self->ns.c_str(), true);
        bool hasSecret = prefs.isKey("secret");
        uint32_t tokens = prefs.getUInt("seq", 0);
        prefs.end();
        return CommandOutput{String(act),
            String("{\"secret\":" + String(hasSecret ? "true" : "false") + ",\"tokens\":") + tokens + "}", String()};
    }
    if (strcmp(act, "set_secret") == 0) {
        if (args.length() < 8)
            return CommandOutput{String(act), String(), String("secret too short (>=8)")};
        Preferences prefs;
        prefs.begin(self->ns.c_str(), false);
        prefs.putString("secret", args);
        prefs.end();
        return CommandOutput{String(act), String("{\"set\":true}"), String()};
    }
    if (strcmp(act, "rotate") == 0) {
        String s = loadSecret(self->ns.c_str(), true);   // 若存在会复用；此处强制重新生成：
        Preferences prefs;
        prefs.begin(self->ns.c_str(), false);
        prefs.remove("secret");
        prefs.end();
        s = loadSecret(self->ns.c_str(), true);
        return CommandOutput{String(act), String("{\"rotated\":true}"), String()};
    }
    if (strcmp(act, "issue_token") == 0) {
        String secret = loadSecret(self->ns.c_str(), true);
        Preferences prefs;
        prefs.begin(self->ns.c_str(), false);
        uint32_t seq = prefs.getUInt("seq", 0);
        prefs.putUInt("seq", seq + 1);
        prefs.end();
        String subject = args.length() ? args : "default";
        char payload[64];
        snprintf(payload, sizeof(payload), "%lu:%s", (unsigned long)seq, subject.c_str());
        uint8_t mac[32];
        fe_hmac_sha256((const uint8_t *)secret.c_str(), secret.length(),
                       (const uint8_t *)payload, strlen(payload), mac);
        return CommandOutput{String(act),
            String("{\"token\":\"") + b64url(mac, 32) + "\",\"seq\":" + seq + "}", String()};
    }
    if (strcmp(act, "list_tokens") == 0) {
        Preferences registry;
        if (!registry.begin("fe_onekey", true))
            return CommandOutput{String(act), String(), String("failed to open token registry")};
        String encoded = registry.getString("tokens", "");
        registry.end();
        String out = "{\"tokens\":[";
        int start = 0; bool first = true;
        while (start < (int)encoded.length()) {
            int end = encoded.indexOf('\n', start); if (end < 0) end = encoded.length();
            String line = encoded.substring(start, end);
            int p1 = line.indexOf('|'), p2 = line.indexOf('|', p1 + 1);
            if (p1 > 0 && p2 > p1) {
                if (!first) out += ','; first = false;
                out += "{\"seq\":" + line.substring(0, p1) + ",\"subject\":\"";
                String subject = line.substring(p1 + 1, p2); subject.replace("\\", "\\\\"); subject.replace("\"", "\\\"");
                out += subject + "\",\"revoked\":" + (line.substring(p2 + 1) == "1" ? "true" : "false") + "}";
            }
            start = end + 1;
        }
        out += "]}";
        return CommandOutput{String(act), out, String()};
    }
    if (strcmp(act, "revoke_token") == 0 || strcmp(act, "revoke_all") == 0) {
        Preferences prefs;
        prefs.begin(self->ns.c_str(), false);
        prefs.putUInt("seq", 0);   // 重置序列使旧令牌失效
        prefs.end();
        return CommandOutput{String(act), String("{\"revoked\":true}"), String()};
    }
    return CommandOutput{String(act), String(), String("unsupported command: ") + act};
}

} // namespace fe
