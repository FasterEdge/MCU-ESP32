// data_config.cpp — ConfigData 实现（Arduino 版）
// 扁平点号路径 KV 配置：get / set / delete / list / snapshot
// NVS（Preferences）持久化。
#include "fe_data.h"
#include <Preferences.h>

namespace fe {

static String normalizeKey(const String &k) {
    String s = k;
    s.replace('.', '_');
    s.replace('/', '_');
    if (s.length() > 15) s = s.substring(0, 15);
    return s;
}

CommandOutput configDataDispatch(void *inst, const char *act, const String &args) {
    ConfigData *self = static_cast<ConfigData *>(inst);

    if (strcmp(act, "get") == 0) {
        if (args.length() == 0)
            return CommandOutput{String(act), String(), String("missing key")};
        Preferences prefs;
        prefs.begin(self->ns.c_str(), true);
        String v = prefs.getString(normalizeKey(args).c_str(), "");
        prefs.end();
        return CommandOutput{String(act), kvPair(args.c_str(), v), String()};
    }
    if (strcmp(act, "set") == 0) {
        int eq = args.indexOf('=');
        if (eq <= 0)
            return CommandOutput{String(act), String(), String("bad format, expect key=value")};
        String key = args.substring(0, eq);
        String value = args.substring(eq + 1);
        Preferences prefs;
        prefs.begin(self->ns.c_str(), false);
        prefs.putString(normalizeKey(key).c_str(), value);
        prefs.end();
        return CommandOutput{String(act), String("saved"), String()};
    }
    if (strcmp(act, "delete") == 0) {
        if (args.length() == 0)
            return CommandOutput{String(act), String(), String("missing key")};
        Preferences prefs;
        prefs.begin(self->ns.c_str(), false);
        prefs.remove(normalizeKey(args).c_str());
        prefs.end();
        return CommandOutput{String(act), String("deleted"), String()};
    }
    if (strcmp(act, "list") == 0) {
        // TODO: 完整遍历 NVS（ESP-IDF nvs_iterator / Preferences 无原生枚举）
        return CommandOutput{String(act), String("{\"keys\":[]}"), String()};
    }
    if (strcmp(act, "snapshot") == 0) {
        // TODO: 导出全部键值快照（同上遍历限制）
        return CommandOutput{String(act), String("{\"snapshot\":{}}"), String()};
    }
    return CommandOutput{String(act), String(), String("unsupported command: ") + act};
}

} // namespace fe