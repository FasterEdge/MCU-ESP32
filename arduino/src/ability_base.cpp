// ability_base.cpp — BaseAbility 实现（Arduino 版）
// list_data_names / list_ability_names
#include "fe_ability.h"

namespace fe {

CommandOutput baseAbilityDispatch(void *inst, const char *act, const String &args) {
    (void)inst; (void)args;
    BaseAbility *self = static_cast<BaseAbility *>(inst);

    if (strcmp(act, "list_data_names") == 0) {
        auto names = globalAtom().listDataNames();
        String out = "[";
        for (size_t i = 0; i < names.size(); i++) {
            if (i) out += ",";
            out += "\"" + names[i] + "\"";
        }
        out += "]";
        return CommandOutput{String(act), out, String()};
    }
    if (strcmp(act, "list_ability_names") == 0) {
        auto names = globalAtom().listAbilityNames();
        String out = "[";
        for (size_t i = 0; i < names.size(); i++) {
            if (i) out += ",";
            out += "\"" + names[i] + "\"";
        }
        out += "]";
        return CommandOutput{String(act), out, String()};
    }
    return CommandOutput{String(act), String(), String("unsupported command: ") + act};
}

} // namespace fe