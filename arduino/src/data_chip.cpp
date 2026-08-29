// data_chip.cpp — ChipData 实现（Arduino 版，MCU 专有）
// MCU 专有 Data：芯片信息。
//   info   返回型号 / 芯片 ID / 内核数 / 频率 / 闪存 / MAC
#include "fe_data.h"

namespace fe {

CommandOutput chipDataDispatch(void *inst, const char *act, const String &args) {
    (void)inst; (void)args;
    if (strcmp(act, "info") == 0) {
        char mac[24];
        snprintf(mac, sizeof(mac), "%012llX", (unsigned long long)ESP.getEfuseMac());
        String out = String("{\"name\":\"ChipData\",\"chip\":\"ESP32\","
                            "\"chipId\":" + String((unsigned long)ESP.getEfuseMac()) + ",");
        out += "\"cores\":" + String(ESP.getChipCores()) + ",";
        out += "\"freqMhz\":" + String(ESP.getCpuFreqMHz()) + ",";
        out += "\"flashBytes\":" + String((unsigned long)ESP.getFlashChipSize()) + ",";
        out += "\"mac\":\"" + String(mac) + "\"}";
        return CommandOutput{String(act), out, String()};
    }
    return CommandOutput{String(act), String(), String("unsupported command: ") + act};
}

} // namespace fe