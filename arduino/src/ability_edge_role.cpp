// ability_edge_role.cpp — EdgeRoleAbility 实现（Arduino 版，简化）
// describe / set_zone / get_zone / set_status / get_status /
// set_online / record_latency / get_metrics
// 说明：MCU 上做完整云边角色（注册服务/心跳上报）过重，
// 此处保留最核心的本地角色状态与简单指标，可配合 MQTTAbility 上报。
#include "fe_ability.h"

namespace fe {

CommandOutput edgeRoleAbilityDispatch(void *inst, const char *act, const String &args) {
    EdgeRoleAbility *self = static_cast<EdgeRoleAbility *>(inst);

    if (strcmp(act, "describe") == 0) {
        return CommandOutput{String(act),
            String("{\"name\":\"EdgeRoleAbility\",\"zone\":\"") + self->zone +
            "\",\"online\":" + (self->online ? "true" : "false") + "}", String()};
    }
    if (strcmp(act, "set_zone") == 0) {
        if (args.length() == 0)
            return CommandOutput{String(act), String(), String("missing zone")};
        self->zone = args;
        return CommandOutput{String(act), String("zone=") + self->zone, String()};
    }
    if (strcmp(act, "get_zone") == 0) {
        return CommandOutput{String(act), String("zone=") + self->zone, String()};
    }
    if (strcmp(act, "set_status") == 0) {
        // healthy / degraded / offline
        if (args != "healthy" && args != "degraded" && args != "offline")
            return CommandOutput{String(act), String(), String("invalid status")};
        self->status = args;
        return CommandOutput{String(act), String("status=") + args, String()};
    }
    if (strcmp(act, "get_status") == 0) {
        return CommandOutput{String(act), String("status=") + self->status, String()};
    }
    if (strcmp(act, "set_online") == 0) {
        // 参数 1/0 或 true/false
        self->online = (args == "1" || args == "true");
        return CommandOutput{String(act),
            String("{\"online\":") + (self->online ? "true" : "false") + "}", String()};
    }
    if (strcmp(act, "record_latency") == 0) {
        self->lastLatencyMs = args.toInt();
        return CommandOutput{String(act), String("latency_ms=") + self->lastLatencyMs, String()};
    }
    if (strcmp(act, "get_metrics") == 0) {
        return CommandOutput{String(act),
            String("{\"latency_ms\":") + self->lastLatencyMs +
            ",\"online\":" + (self->online ? "true" : "false") + "}", String()};
    }
    return CommandOutput{String(act), String(), String("unsupported command: ") + act};
}

} // namespace fe