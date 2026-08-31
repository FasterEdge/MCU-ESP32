// data_config.cpp — ConfigData 实现（Arduino 版）
// 扁平点号路径 KV 配置：get / set / delete / list / snapshot
#include "fe_data.h"
#include <Preferences.h>

namespace fe {

static const char *REGISTRY_KEY = "__keys";
static const size_t MAX_KEYS = 64;

static bool validLogicalKey(const String &key) {
    if (key.length() == 0 || key.length() > 96) return false;
    for (size_t i = 0; i < key.length(); ++i) {
        unsigned char c = (unsigned char)key[i];
        if (c < 0x20 || c == 0x7f || c == '\n' || c == '\r') return false;
    }
    return true;
}

static uint32_t keyHash(const String &key) {
    uint32_t hash = 2166136261UL;
    for (size_t i = 0; i < key.length(); ++i) {
        hash ^= (uint8_t)key[i];
        hash *= 16777619UL;
    }
    return hash;
}

static String storageKey(const String &key) {
    char out[12];
    snprintf(out, sizeof(out), "k%08lx", (unsigned long)keyHash(key));
    return String(out);
}

static std::vector<String> decodeRegistry(const String &encoded) {
    std::vector<String> keys;
    int start = 0;
    while (start < (int)encoded.length()) {
        int end = encoded.indexOf('\n', start);
        if (end < 0) end = encoded.length();
        String key = encoded.substring(start, end);
        if (key.length()) keys.push_back(key);
        start = end + 1;
    }
    return keys;
}

static String encodeRegistry(const std::vector<String> &keys) {
    String out;
    for (const String &key : keys) {
        if (out.length()) out += '\n';
        out += key;
    }
    return out;
}

static int findKey(const std::vector<String> &keys, const String &key) {
    for (size_t i = 0; i < keys.size(); ++i)
        if (keys[i] == key) return (int)i;
    return -1;
}

static bool hasHashCollision(const std::vector<String> &keys, const String &key) {
    String physical = storageKey(key);
    for (const String &existing : keys)
        if (existing != key && storageKey(existing) == physical) return true;
    return false;
}

static String jsonEscape(const String &input) {
    String out;
    out.reserve(input.length() + 8);
    const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < input.length(); ++i) {
        uint8_t c = (uint8_t)input[i];
        if (c == '"' || c == '\\') { out += '\\'; out += (char)c; }
        else if (c == '\b') out += "\\b";
        else if (c == '\f') out += "\\f";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c < 0x20) {
            out += "\\u00"; out += hex[c >> 4]; out += hex[c & 15];
        } else out += (char)c;
    }
    return out;
}

CommandOutput configDataDispatch(void *inst, const char *act, const String &args) {
    ConfigData *self = static_cast<ConfigData *>(inst);

    if (strcmp(act, "get") == 0) {
        if (!validLogicalKey(args))
            return CommandOutput{String(act), String(), String("invalid or missing key")};
        Preferences prefs;
        if (!prefs.begin(self->ns.c_str(), true))
            return CommandOutput{String(act), String(), String("failed to open config")};
        std::vector<String> keys = decodeRegistry(prefs.getString(REGISTRY_KEY, ""));
        int index = findKey(keys, args);
        String value = index >= 0 ? prefs.getString(storageKey(args).c_str(), "") : String();
        prefs.end();
        if (index < 0) return CommandOutput{String(act), String(), String("key not found")};
        return CommandOutput{String(act), kvPair(args.c_str(), value), String()};
    }
    if (strcmp(act, "set") == 0) {
        int eq = args.indexOf('=');
        if (eq <= 0)
            return CommandOutput{String(act), String(), String("bad format, expect key=value")};
        String key = args.substring(0, eq);
        String value = args.substring(eq + 1);
        if (!validLogicalKey(key))
            return CommandOutput{String(act), String(), String("invalid key")};
        Preferences prefs;
        if (!prefs.begin(self->ns.c_str(), false))
            return CommandOutput{String(act), String(), String("failed to open config")};
        std::vector<String> keys = decodeRegistry(prefs.getString(REGISTRY_KEY, ""));
        int index = findKey(keys, key);
        if (index < 0 && keys.size() >= MAX_KEYS) {
            prefs.end();
            return CommandOutput{String(act), String(), String("key registry full")};
        }
        if (hasHashCollision(keys, key)) {
            prefs.end();
            return CommandOutput{String(act), String(), String("key hash collision")};
        }
        size_t written = prefs.putString(storageKey(key).c_str(), value);
        if (written != value.length()) {
            prefs.end();
            return CommandOutput{String(act), String(), String("failed to save value")};
        }
        if (index < 0) {
            keys.push_back(key);
            String registry = encodeRegistry(keys);
            if (prefs.putString(REGISTRY_KEY, registry) != registry.length()) {
                prefs.remove(storageKey(key).c_str());
                prefs.end();
                return CommandOutput{String(act), String(), String("failed to update key registry")};
            }
        }
        prefs.end();
        return CommandOutput{String(act), String("saved"), String()};
    }
    if (strcmp(act, "delete") == 0) {
        if (!validLogicalKey(args))
            return CommandOutput{String(act), String(), String("invalid or missing key")};
        Preferences prefs;
        if (!prefs.begin(self->ns.c_str(), false))
            return CommandOutput{String(act), String(), String("failed to open config")};
        std::vector<String> keys = decodeRegistry(prefs.getString(REGISTRY_KEY, ""));
        int index = findKey(keys, args);
        if (index < 0) {
            prefs.end();
            return CommandOutput{String(act), String(), String("key not found")};
        }
        if (!prefs.remove(storageKey(args).c_str())) {
            prefs.end();
            return CommandOutput{String(act), String(), String("failed to delete value")};
        }
        keys.erase(keys.begin() + index);
        String registry = encodeRegistry(keys);
        bool registryOk = keys.empty() ? prefs.remove(REGISTRY_KEY) || !prefs.isKey(REGISTRY_KEY)
                                       : prefs.putString(REGISTRY_KEY, registry) == registry.length();
        prefs.end();
        if (!registryOk) return CommandOutput{String(act), String(), String("deleted value but failed to update registry")};
        return CommandOutput{String(act), String("deleted"), String()};
    }
    if (strcmp(act, "list") == 0 || strcmp(act, "snapshot") == 0) {
        Preferences prefs;
        if (!prefs.begin(self->ns.c_str(), true))
            return CommandOutput{String(act), String(), String("failed to open config")};
        std::vector<String> keys = decodeRegistry(prefs.getString(REGISTRY_KEY, ""));
        String out = strcmp(act, "list") == 0 ? "{\"keys\":[" : "{\"snapshot\":{";
        for (size_t i = 0; i < keys.size(); ++i) {
            if (i) out += ',';
            out += '"'; out += jsonEscape(keys[i]); out += '"';
            if (strcmp(act, "snapshot") == 0) {
                out += ":\"";
                out += jsonEscape(prefs.getString(storageKey(keys[i]).c_str(), ""));
                out += '"';
            }
        }
        prefs.end();
        out += strcmp(act, "list") == 0 ? "]}" : "}}";
        return CommandOutput{String(act), out, String()};
    }
    return CommandOutput{String(act), String(), String("unsupported command: ") + act};
}

} // namespace fe
