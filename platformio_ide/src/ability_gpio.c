// ability_gpio.c — GpioAbility 实现（Keil/裸机 C 版，MCU 专有）
// MCU 专有能力：GPIO 引脚控制（平台差异由 fe_port 提供）。
//   mode <pin>,<input|output|input_pullup>  设置引脚模式
//   write <pin>,<0|1>                       输出电平
//   read <pin>                              读输入电平
//   info                                    说明
#include "fe_ability.h"
#include "fe_port.h"

fe_output_t ability_gpio_dispatch(void *inst, const char *act, const char *args) {
    (void)inst;
    char tmp[64];
    char *end = NULL;

    snprintf(tmp, sizeof(tmp), "%s", args ? args : "");

    if (strcmp(act, "mode") == 0) {
        char *comma = strchr(tmp, ',');
        if (!comma) return fe_err(act, "bad format, expect pin,mode");
        *comma = 0;
        long pin = strtol(tmp, &end, 0);
        if (end == tmp || pin < 0 || pin > 63) return fe_err(act, "invalid pin");
        const char *mode = comma + 1;
        if (strcmp(mode, "input") != 0 && strcmp(mode, "output") != 0 &&
            strcmp(mode, "input_pullup") != 0)
            return fe_err(act, "mode must be input/output/input_pullup");
        if (fe_port_gpio_set_mode((uint8_t)pin, mode) != 0)
            return fe_err(act, "set mode failed");
        char out[64];
        snprintf(out, sizeof(out), "{\"pin\":%ld,\"mode\":\"%s\"}", pin, mode);
        return fe_ok(act, out);
    }
    if (strcmp(act, "write") == 0) {
        char *comma = strchr(tmp, ',');
        if (!comma) return fe_err(act, "bad format, expect pin,level");
        *comma = 0;
        long pin = strtol(tmp, &end, 0);
        if (end == tmp || pin < 0 || pin > 63) return fe_err(act, "invalid pin");
        long level = strtol(comma + 1, &end, 0);
        if (level != 0 && level != 1) return fe_err(act, "level must be 0 or 1");
        if (fe_port_gpio_write((uint8_t)pin, (uint8_t)level) != 0)
            return fe_err(act, "write failed");
        char out[64];
        snprintf(out, sizeof(out), "{\"pin\":%ld,\"level\":%ld}", pin, level);
        return fe_ok(act, out);
    }
    if (strcmp(act, "read") == 0) {
        long pin = strtol(tmp, &end, 0);
        if (end == tmp || pin < 0 || pin > 63) return fe_err(act, "invalid pin");
        int level = fe_port_gpio_read((uint8_t)pin);
        if (level < 0) return fe_err(act, "read failed");
        char out[64];
        snprintf(out, sizeof(out), "{\"pin\":%ld,\"level\":%d}", pin, level);
        return fe_ok(act, out);
    }
    if (strcmp(act, "info") == 0) {
        return fe_ok(act,
            "{\"ability\":\"GpioAbility\",\"desc\":\"MCU GPIO 控制\","
            "\"modes\":[\"input\",\"output\",\"input_pullup\"]}");
    }
    return fe_err(act, "unsupported command");
}