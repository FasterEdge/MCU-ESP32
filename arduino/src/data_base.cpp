// FasterEdge 开源项目 - Github: https://github.com/FasterEdge - Gitee: https://gitee.com/FasterEdge
// data_base.cpp — BaseData 实现（Arduino 版）
// logo / info
#include "fe_data.h"

namespace fe {

// 简单的 ASCII logo（与 FasterEdge 主仓库 BaseData.logo 呼应）
static const char *LOGO =
    "  __ _        _\n"
    " / _| |_ _  _(_)__ _\n"
    "|  _|  _| || | / _` |\n"
    "|_|  \\__|\\_, |_\\__,_|\n"
    "        |__/\n";

CommandOutput baseDataDispatch(void *inst, const char *act, const String &args) {
    (void)inst; (void)args;
    if (strcmp(act, "logo") == 0) {
        return CommandOutput{String(act), String(LOGO), String()};
    }
    if (strcmp(act, "info") == 0) {
        return CommandOutput{String(act),
            String("{\"name\":\"BaseData\",\"firmware\":\"FasterEdge-MCU 1.0.20260831\","
                   "\"chip\":\"ESP32\",\"sdk\":\"Arduino\",\"cores\":" + String(ESP.getChipCores()) +
                   ",\"freq\":" + String(ESP.getCpuFreqMHz()) + "}"), String()};
    }
    return CommandOutput{String(act), String(), String("unsupported command: ") + act};
}

} // namespace fe
