// ability_gpio.cpp — GpioAbility 实现（Arduino 版，MCU 专有）
// MCU 专有能力：GPIO 引脚控制。
//   mode <pin>,<input|output|input_pullup>  设置引脚模式
//   write <pin>,<0|1>                       输出电平
//   read <pin>                              读输入电平
//   info                                    说明
#include "fe_ability.h"

namespace fe {

CommandOutput gpioAbilityDispatch(void *inst, const char *act, const String &args) {
    (void)inst;
    char tmp[64];
    strncpy(tmp, args.c_str(), sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = 0;
    char *end = NULL;

    if (strcmp(act, "mode") == 0) {
        char *comma = strchr(tmp, ',');
        if (!comma)
            return CommandOutput{String(act), String(), String("bad format, expect pin,mode")};
        *comma = 0;
        long pin = strtol(tmp, &end, 0);
        if (end == tmp || pin < 0 || pin > 39)
            return CommandOutput{String(act), String(), String("invalid pin")};
        String mode = String(comma + 1);
        mode.trim();
        if (mode == "input") {
            pinMode((uint8_t)pin, INPUT);
        } else if (mode == "output") {
            pinMode((uint8_t)pin, OUTPUT);
        } else if (mode == "input_pullup") {
            pinMode((uint8_t)pin, INPUT_PULLUP);
        } else {
            return CommandOutput{String(act), String(), String("mode must be input/output/input_pullup")};
        }
        char out[64];
        snprintf(out, sizeof(out), "{\"pin\":%ld,\"mode\":\"%s\"}", pin, mode.c_str());
        return CommandOutput{String(act), String(out), String()};
    }
    if (strcmp(act, "write") == 0) {
        char *comma = strchr(tmp, ',');
        if (!comma)
            return CommandOutput{String(act), String(), String("bad format, expect pin,level")};
        *comma = 0;
        long pin = strtol(tmp, &end, 0);
        if (end == tmp || pin < 0 || pin > 39)
            return CommandOutput{String(act), String(), String("invalid pin")};
        long level = strtol(comma + 1, &end, 0);
        if (level != 0 && level != 1)
            return CommandOutput{String(act), String(), String("level must be 0 or 1")};
        digitalWrite((uint8_t)pin, (uint8_t)level);
        char out[64];
        snprintf(out, sizeof(out), "{\"pin\":%ld,\"level\":%ld}", pin, level);
        return CommandOutput{String(act), String(out), String()};
    }
    if (strcmp(act, "read") == 0) {
        long pin = strtol(tmp, &end, 0);
        if (end == tmp || pin < 0 || pin > 39)
            return CommandOutput{String(act), String(), String("invalid pin")};
        int level = digitalRead((uint8_t)pin);
        char out[64];
        snprintf(out, sizeof(out), "{\"pin\":%ld,\"level\":%d}", pin, level);
        return CommandOutput{String(act), String(out), String()};
    }
    if (strcmp(act, "info") == 0) {
        return CommandOutput{String(act),
            String("{\"ability\":\"GpioAbility\",\"desc\":\"MCU GPIO 控制\","
                   "\"pins\":\"0..39\",\"modes\":[\"input\",\"output\",\"input_pullup\"]}"),
            String()};
    }
    return CommandOutput{String(act), String(), String("unsupported command: ") + act};
}

} // namespace fe