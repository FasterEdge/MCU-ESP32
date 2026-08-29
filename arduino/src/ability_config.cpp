// ability_config.cpp — ConfigFileAbility 实现（Arduino 版）
// load / save / set_path / get_path / exists
// 基于 NVS（Preferences）的扁平配置存取。
#include "fe_ability.h"
#include <Preferences.h>

namespace fe {

// 将 path 作为 NVS key（受限长度，简化截断）
static String normalizeKey(const String &path) {
    String k = path;
    k.replace('/', '_');
    if (k.length() > 15) k = k.substring(0, 15); // NVS key 最大 15 字符
    return k;
}

CommandOutput configFileAbilityDispatch(void *inst, const char *act, const String &args) {
    ConfigFileAbility *self = static_cast<ConfigFileAbility *>(inst);

    if (strcmp(act, "set_path") == 0) {
        if (args.length() == 0)
            return CommandOutput{String(act), String(), String("missing path")};
        self->path = args;
        return CommandOutput{String(act), String("path=") + self->path, String()};
    }
    if (strcmp(act, "get_path") == 0) {
        return CommandOutput{String(act), String("path=") + self->path, String()};
    }
    if (strcmp(act, "exists") == 0) {
        Preferences prefs;
        prefs.begin(self->path.c_str(), true);
        bool found = prefs.isKey(normalizeKey(args).c_str());
        prefs.end();
        return CommandOutput{String(act), String("{\"exists\":") + (found ? "true" : "false") + "}", String()};
    }
    if (strcmp(act, "load") == 0) {
        // 加载整段配置到内存快照。参数：key（可选，默认全量）
        Preferences prefs;
        prefs.begin(self->path.c_str(), true);
        String val;
        if (args.length()) {
            val = prefs.getString(normalizeKey(args).c_str(), "");
        } else {
            // TODO: 遍历 NVS 全部键（ESP-IDF nvs_iterator）
            val = "<snapshot>";  // 占位：完整实现见 Keil/裸机版
        }
        prefs.end();
        return CommandOutput{String(act), val, String()};
    }
    if (strcmp(act, "save") == 0) {
        // 参数格式：key=value
        int eq = args.indexOf('=');
        if (eq <= 0)
            return CommandOutput{String(act), String(), String("bad format, expect key=value")};
        String key = normalizeKey(args.substring(0, eq));
        String value = args.substring(eq + 1);
        Preferences prefs;
        prefs.begin(self->path.c_str(), false);
        prefs.putString(key.c_str(), value);
        prefs.end();
        return CommandOutput{String(act), String("saved"), String()};
    }
    return CommandOutput{String(act), String(), String("unsupported command: ") + act};
}

} // namespace fe