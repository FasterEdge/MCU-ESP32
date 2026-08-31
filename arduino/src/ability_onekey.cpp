// ability_onekey.cpp — OneKeyAbility 实现（Arduino 版）
// issue_token / verify_token / revoke_token / revoke_all / list_tokens / status / rotate
#include "fe_ability.h"
#include "fe_hmac_sha256.h"
#include <Preferences.h>
#include <errno.h>

namespace fe {

static const char *ONEKEY_NS = "fe_onekey";
static const char *TOKEN_REGISTRY = "tokens";
static const size_t MAX_TOKENS = 32;
static const size_t MAX_SUBJECT = 40;

struct TokenRecord {
    uint32_t id;
    String subject;
    bool revoked;
};

static String base64urlEncode(const uint8_t *data, size_t len) {
    static const char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    String out;
    out.reserve((len * 4 + 2) / 3);
    size_t i = 0;
    while (i + 2 < len) {
        uint32_t n = ((uint32_t)data[i] << 16) | ((uint32_t)data[i+1] << 8) | data[i+2];
        out += tbl[(n >> 18) & 63]; out += tbl[(n >> 12) & 63];
        out += tbl[(n >> 6) & 63];  out += tbl[n & 63];
        i += 3;
    }
    if (i + 1 == len) {
        uint32_t n = (uint32_t)data[i] << 16;
        out += tbl[(n >> 18) & 63]; out += tbl[(n >> 12) & 63];
    } else if (i + 2 == len) {
        uint32_t n = ((uint32_t)data[i] << 16) | ((uint32_t)data[i+1] << 8);
        out += tbl[(n >> 18) & 63]; out += tbl[(n >> 12) & 63]; out += tbl[(n >> 6) & 63];
    }
    return out;
}

static String jsonEscape(const String &input) {
    String out;
    out.reserve(input.length() + 8);
    for (size_t i = 0; i < input.length(); ++i) {
        char c = input[i];
        if (c == '"' || c == '\\') { out += '\\'; out += c; }
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else out += c;
    }
    return out;
}

static bool validSubject(const String &subject) {
    if (subject.length() == 0 || subject.length() > MAX_SUBJECT) return false;
    for (size_t i = 0; i < subject.length(); ++i) {
        uint8_t c = (uint8_t)subject[i];
        if (c < 0x20 || c == 0x7f || c == '|' || c == '\n' || c == '\r') return false;
    }
    return true;
}

static String generateSecret() {
    static const char hex[] = "0123456789abcdef";
    String secret;
    secret.reserve(64);
    for (int i = 0; i < 32; ++i) {
        uint8_t b = (uint8_t)(esp_random() & 0xff);
        secret += hex[b >> 4];
        secret += hex[b & 15];
    }
    return secret;
}

static String loadOrCreateSecret(Preferences &prefs) {
    String secret = prefs.getString("secret", "");
    if (secret.length() != 64) {
        secret = generateSecret();
        if (prefs.putString("secret", secret) != secret.length()) return String();
    }
    return secret;
}

static std::vector<TokenRecord> decodeRecords(const String &encoded) {
    std::vector<TokenRecord> records;
    int start = 0;
    while (start < (int)encoded.length()) {
        int end = encoded.indexOf('\n', start);
        if (end < 0) end = encoded.length();
        String line = encoded.substring(start, end);
        int p1 = line.indexOf('|');
        int p2 = line.indexOf('|', p1 + 1);
        if (p1 > 0 && p2 > p1) {
            String idText = line.substring(0, p1);
            char *parseEnd = nullptr;
            unsigned long id = strtoul(idText.c_str(), &parseEnd, 10);
            String subject = line.substring(p1 + 1, p2);
            if (parseEnd && *parseEnd == '\0' && id <= UINT32_MAX && validSubject(subject))
                records.push_back(TokenRecord{(uint32_t)id, subject, line.substring(p2 + 1) == "1"});
        }
        start = end + 1;
    }
    return records;
}

static String encodeRecords(const std::vector<TokenRecord> &records) {
    String out;
    for (const TokenRecord &record : records) {
        if (out.length()) out += '\n';
        out += String(record.id); out += '|'; out += record.subject; out += '|'; out += (record.revoked ? '1' : '0');
    }
    return out;
}

static int findRecord(const std::vector<TokenRecord> &records, uint32_t id) {
    for (size_t i = 0; i < records.size(); ++i)
        if (records[i].id == id) return (int)i;
    return -1;
}

static String makeToken(const String &secret, uint32_t id, const String &subject) {
    String payload = String(id) + ":" + subject;
    uint8_t mac[32];
    fe_hmac_sha256((const uint8_t *)secret.c_str(), secret.length(),
                   (const uint8_t *)payload.c_str(), payload.length(), mac);
    return base64urlEncode(mac, sizeof(mac));
}

static bool constantTimeEqual(const String &a, const String &b) {
    size_t maxLen = a.length() > b.length() ? a.length() : b.length();
    uint8_t diff = (uint8_t)(a.length() ^ b.length());
    for (size_t i = 0; i < maxLen; ++i) {
        uint8_t ac = i < a.length() ? (uint8_t)a[i] : 0;
        uint8_t bc = i < b.length() ? (uint8_t)b[i] : 0;
        diff |= ac ^ bc;
    }
    return diff == 0;
}

static bool parseId(const String &text, uint32_t &id) {
    if (text.length() == 0) return false;
    for (size_t i = 0; i < text.length(); ++i)
        if (!isdigit((unsigned char)text[i])) return false;
    char *end = nullptr;
    errno = 0;
    unsigned long value = strtoul(text.c_str(), &end, 10);
    if (errno == ERANGE || !end || *end != '\0' || value > UINT32_MAX) return false;
    id = (uint32_t)value;
    return true;
}

CommandOutput oneKeyAbilityDispatch(void *inst, const char *act, const String &args) {
    OneKeyAbility *self = static_cast<OneKeyAbility *>(inst);
    Preferences prefs;
    if (!prefs.begin(ONEKEY_NS, false))
        return CommandOutput{String(act), String(), String("failed to open token store")};
    self->secret = loadOrCreateSecret(prefs);
    if (self->secret.length() != 64) {
        prefs.end();
        return CommandOutput{String(act), String(), String("failed to initialize token secret")};
    }
    self->tokenSeq = prefs.getUInt("next", 0);
    std::vector<TokenRecord> records = decodeRecords(prefs.getString(TOKEN_REGISTRY, ""));

    if (strcmp(act, "status") == 0) {
        size_t active = 0;
        for (const TokenRecord &record : records) if (!record.revoked) ++active;
        prefs.end();
        return CommandOutput{String(act), String("{\"active\":") + (unsigned long)active +
            ",\"registered\":" + (unsigned long)records.size() + ",\"next_seq\":" + (unsigned long)self->tokenSeq + "}", String()};
    }
    if (strcmp(act, "issue_token") == 0) {
        String subject = args.length() ? args : "default";
        if (!validSubject(subject)) {
            prefs.end();
            return CommandOutput{String(act), String(), String("invalid subject (1..40 printable characters; no '|')")};
        }
        if (records.size() >= MAX_TOKENS || self->tokenSeq == UINT32_MAX) {
            prefs.end();
            return CommandOutput{String(act), String(), String("token registry full or sequence exhausted")};
        }
        uint32_t id = (uint32_t)self->tokenSeq;
        if (prefs.putUInt("next", id + 1) != sizeof(uint32_t)) {
            prefs.end();
            return CommandOutput{String(act), String(), String("failed to persist token sequence")};
        }
        records.push_back(TokenRecord{id, subject, false});
        String encoded = encodeRecords(records);
        if (prefs.putString(TOKEN_REGISTRY, encoded) != encoded.length()) {
            prefs.end();
            return CommandOutput{String(act), String(), String("failed to register token")};
        }
        self->tokenSeq = id + 1;
        String token = makeToken(self->secret, id, subject);
        prefs.end();
        return CommandOutput{String(act), String("{\"token\":\"") + token +
            "\",\"seq\":" + (unsigned long)id + ",\"subject\":\"" + jsonEscape(subject) + "\"}", String()};
    }
    if (strcmp(act, "verify_token") == 0) {
        int p1 = args.indexOf(':');
        int p2 = args.indexOf(':', p1 + 1);
        uint32_t id;
        if (p1 <= 0 || p2 <= p1 + 1 || !parseId(args.substring(0, p1), id)) {
            prefs.end();
            return CommandOutput{String(act), String(), String("bad format, expect seq:subject:token")};
        }
        String subject = args.substring(p1 + 1, p2);
        String token = args.substring(p2 + 1);
        int index = findRecord(records, id);
        bool valid = index >= 0 && !records[index].revoked && records[index].subject == subject &&
                     constantTimeEqual(makeToken(self->secret, id, subject), token);
        prefs.end();
        return CommandOutput{String(act), String("{\"valid\":") + (valid ? "true" : "false") + "}",
                             valid ? String() : String("token invalid or revoked")};
    }
    if (strcmp(act, "revoke_token") == 0) {
        uint32_t id;
        if (!parseId(args, id)) {
            prefs.end();
            return CommandOutput{String(act), String(), String("expect token sequence id")};
        }
        int index = findRecord(records, id);
        if (index < 0) {
            prefs.end();
            return CommandOutput{String(act), String(), String("token not found")};
        }
        records[index].revoked = true;
        String encoded = encodeRecords(records);
        bool ok = prefs.putString(TOKEN_REGISTRY, encoded) == encoded.length();
        prefs.end();
        if (!ok) return CommandOutput{String(act), String(), String("failed to persist revocation")};
        return CommandOutput{String(act), String("{\"revoked\":true,\"seq\":") + (unsigned long)id + "}", String()};
    }
    if (strcmp(act, "revoke_all") == 0) {
        bool ok = !prefs.isKey(TOKEN_REGISTRY) || prefs.remove(TOKEN_REGISTRY);
        prefs.end();
        if (!ok) return CommandOutput{String(act), String(), String("failed to revoke tokens")};
        return CommandOutput{String(act), String("{\"revoked\":true}"), String()};
    }
    if (strcmp(act, "list_tokens") == 0) {
        String out = "{\"tokens\":[";
        for (size_t i = 0; i < records.size(); ++i) {
            if (i) out += ',';
            out += "{\"seq\":"; out += (unsigned long)records[i].id;
            out += ",\"subject\":\""; out += jsonEscape(records[i].subject);
            out += "\",\"revoked\":"; out += records[i].revoked ? "true" : "false"; out += '}';
        }
        prefs.end();
        out += "]}";
        return CommandOutput{String(act), out, String()};
    }
    if (strcmp(act, "rotate") == 0) {
        String secret = generateSecret();
        bool ok = prefs.putString("secret", secret) == secret.length();
        ok = (!prefs.isKey(TOKEN_REGISTRY) || prefs.remove(TOKEN_REGISTRY)) && ok;
        self->secret = secret;
        prefs.end();
        if (!ok) return CommandOutput{String(act), String(), String("failed to rotate token secret")};
        return CommandOutput{String(act), String("{\"rotated\":true}"), String()};
    }
    prefs.end();
    return CommandOutput{String(act), String(), String("unsupported command: ") + act};
}

} // namespace fe
