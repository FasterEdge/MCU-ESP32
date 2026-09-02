// FasterEdge 开源项目 - Github: https://github.com/FasterEdge - Gitee: https://gitee.com/FasterEdge
// ability_config.cpp — ConfigFileAbility 实现（Arduino 版）
// load / save / set_path / get_path / exists; explicit registry supports full snapshots.
#include "fe_ability.h"
#include <Preferences.h>

namespace fe {

static const char *REGISTRY_KEY = "__keys";
static const size_t MAX_KEYS = 64;

static bool validKey(const String &key) {
    if (key.length() == 0 || key.length() > 96) return false;
    for (size_t i = 0; i < key.length(); ++i)
        if ((uint8_t)key[i] < 0x20 || key[i] == 0x7f || key[i] == '\n' || key[i] == '\r') return false;
    return true;
}

static uint32_t keyHash(const String &key) {
    uint32_t hash = 2166136261UL;
    for (size_t i = 0; i < key.length(); ++i) { hash ^= (uint8_t)key[i]; hash *= 16777619UL; }
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
    for (const String &key : keys) { if (out.length()) out += '\n'; out += key; }
    return out;
}

static int findKey(const std::vector<String> &keys, const String &key) {
    for (size_t i = 0; i < keys.size(); ++i) if (keys[i] == key) return (int)i;
    return -1;
}

static bool collision(const std::vector<String> &keys, const String &key) {
    String physical = storageKey(key);
    for (const String &existing : keys)
        if (existing != key && storageKey(existing) == physical) return true;
    return false;
}

static String jsonEscape(const String &input) {
    String out;
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

CommandOutput configFileAbilityDispatch(void *inst, const char *act, const String &args) {
    ConfigFileAbility *self = static_cast<ConfigFileAbility *>(inst);

    if (strcmp(act, "set_path") == 0) {
        if (args.length() == 0 || args.length() > 15)
            return CommandOutput{String(act), String(), String("namespace path must be 1..15 characters")};
        self->path = args;
        return CommandOutput{String(act), String("path=") + self->path, String()};
    }
    if (strcmp(act, "get_path") == 0)
        return CommandOutput{String(act), String("path=") + self->path, String()};

    Preferences prefs;
    bool readOnly = strcmp(act, "save") != 0;
    if (!prefs.begin(self->path.c_str(), readOnly))
        return CommandOutput{String(act), String(), String("failed to open config namespace")};
    std::vector<String> keys = decodeRegistry(prefs.getString(REGISTRY_KEY, ""));

    if (strcmp(act, "exists") == 0) {
        bool found = validKey(args) && findKey(keys, args) >= 0;
        prefs.end();
        return CommandOutput{String(act), String("{\"exists\":") + (found ? "true" : "false") + "}", String()};
    }
    if (strcmp(act, "load") == 0) {
        if (args.length()) {
            int index = validKey(args) ? findKey(keys, args) : -1;
            String value = index >= 0 ? prefs.getString(storageKey(args).c_str(), "") : String();
            prefs.end();
            if (index < 0) return CommandOutput{String(act), String(), String("key not found")};
            return CommandOutput{String(act), value, String()};
        }
        String out = "{\"snapshot\":{";
        for (size_t i = 0; i < keys.size(); ++i) {
            if (i) out += ',';
            out += '"'; out += jsonEscape(keys[i]); out += "\":\"";
            out += jsonEscape(prefs.getString(storageKey(keys[i]).c_str(), "")); out += '"';
        }
        prefs.end();
        out += "}}";
        return CommandOutput{String(act), out, String()};
    }
    if (strcmp(act, "save") == 0) {
        int eq = args.indexOf('=');
        if (eq <= 0) { prefs.end(); return CommandOutput{String(act), String(), String("bad format, expect key=value")}; }
        String key = args.substring(0, eq);
        String value = args.substring(eq + 1);
        int index = validKey(key) ? findKey(keys, key) : -2;
        if (index == -2 || (index < 0 && keys.size() >= MAX_KEYS) || collision(keys, key)) {
            prefs.end();
            return CommandOutput{String(act), String(), String("invalid key, full registry, or hash collision")};
        }
        if (prefs.putString(storageKey(key).c_str(), value) != value.length()) {
            prefs.end(); return CommandOutput{String(act), String(), String("failed to save value")};
        }
        if (index < 0) {
            keys.push_back(key);
            String registry = encodeRegistry(keys);
            if (prefs.putString(REGISTRY_KEY, registry) != registry.length()) {
                prefs.remove(storageKey(key).c_str()); prefs.end();
                return CommandOutput{String(act), String(), String("failed to update key registry")};
            }
        }
        prefs.end();
        return CommandOutput{String(act), String("saved"), String()};
    }
    prefs.end();
    return CommandOutput{String(act), String(), String("unsupported command: ") + act};
}

} // namespace fe
