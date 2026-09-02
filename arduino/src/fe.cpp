// FasterEdge 开源项目 - Github: https://github.com/FasterEdge - Gitee: https://gitee.com/FasterEdge
// fe.cpp — FasterEdge MCU 核心框架实现（Arduino 版）
#include "fe.h"

namespace fe {

// ============================================================
// Atom 实现
// ============================================================
std::vector<String> Atom::listDataNames() const {
    std::vector<String> names;
    for (const auto &d : dataList_) names.push_back(String(d.name));
    return names;
}

std::vector<String> Atom::listAbilityNames() const {
    std::vector<String> names;
    for (const auto &a : abilityList_) names.push_back(String(a.name));
    return names;
}

CommandOutput Atom::execute(const char *target, const char *act, const String &args) {
    // 路由：data_xxx / ability_xxx
    bool isData = strncmp(target, "data_", 5) == 0;
    const char *base = isData ? target + 5 : (strncmp(target, "ability_", 8) == 0 ? target + 8 : target);

    if (isData) {
        for (const auto &d : dataList_) {
            if (strcmp(d.name, base) == 0) {
                for (size_t i = 0; i < d.cmd_count; i++) {
                    if (strcmp(d.cmds[i].name, act) == 0)
                        return d.dispatch(d.instance, act, args);
                }
                return CommandOutput{String(act), String(), String("unsupported command: ") + act};
            }
        }
        return CommandOutput{String(act), String(), String("unknown data: ") + base};
    }

    for (const auto &a : abilityList_) {
        if (strcmp(a.name, base) == 0) {
            for (size_t i = 0; i < a.cmd_count; i++) {
                if (strcmp(a.cmds[i].name, act) == 0)
                    return a.dispatch(a.instance, act, args);
            }
            return CommandOutput{String(act), String(), String("unsupported command: ") + act};
        }
    }
    return CommandOutput{String(act), String(), String("unknown ability: ") + base};
}

// ============================================================
// 全局单例
// ============================================================
Atom &globalAtom() {
    static Atom atom;
    return atom;
}

// ============================================================
// 工具
// ============================================================
String toJsonString(const char *key, const String &value) {
    return String("{\"") + key + "\":\"" + value + "\"}";
}

String toJsonNumber(const char *key, long value) {
    return String("{\"") + key + "\":" + value + "}";
}

String kvPair(const char *key, const String &value) {
    return String(key) + "=" + value;
}

} // namespace fe
