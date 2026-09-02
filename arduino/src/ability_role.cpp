// FasterEdge 开源项目 - Github: https://github.com/FasterEdge - Gitee: https://gitee.com/FasterEdge
// ability_role.cpp — RoleAbility 实现（Arduino 版）
// describe / set_role / get_role
#include "fe_ability.h"

namespace fe {

CommandOutput roleAbilityDispatch(void *inst, const char *act, const String &args) {
    RoleAbility *self = static_cast<RoleAbility *>(inst);

    if (strcmp(act, "describe") == 0) {
        return CommandOutput{String(act),
            String("{\"name\":\"RoleAbility\",\"role\":\"") + self->role + "\"}", String()};
    }
    if (strcmp(act, "set_role") == 0) {
        if (args.length() == 0)
            return CommandOutput{String(act), String(), String("missing role arg")};
        // 合法角色：edge / cloud / standalone
        String r = args;
        if (r != "edge" && r != "cloud" && r != "standalone")
            return CommandOutput{String(act), String(), String("invalid role: ") + r};
        self->role = r;
        return CommandOutput{String(act), String("role=") + r, String()};
    }
    if (strcmp(act, "get_role") == 0) {
        return CommandOutput{String(act), String("role=") + self->role, String()};
    }
    return CommandOutput{String(act), String(), String("unsupported command: ") + act};
}

} // namespace fe
