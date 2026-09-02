// FasterEdge 开源项目 - Github: https://github.com/FasterEdge - Gitee: https://gitee.com/FasterEdge
// ability_reg.cpp — RegAbility 实现（Arduino 版，MCU 专有）
// MCU 专有能力：直接读写内存映射外设寄存器。
//   read <addr>           读 32 位寄存器，addr 为十六进制地址（如 0x3FF44004）
//   write <addr>,<value>  写 32 位
//   bit_set <addr>,<bit>  置位
//   bit_clear <addr>,<bit> 清位
//   info                  说明
// 注意：请谨慎使用，误写寄存器可能导致系统异常。
#include "fe_ability.h"

namespace fe {

// ESP32 内存映射寄存器读写（volatile 直读直写，跨架构安全）
static uint32_t regRead(uint32_t addr) {
    return *(volatile uint32_t *)(uintptr_t)addr;
}

static void regWrite(uint32_t addr, uint32_t value) {
    *(volatile uint32_t *)(uintptr_t)addr = value;
}

CommandOutput regAbilityDispatch(void *inst, const char *act, const String &args) {
    (void)inst;
    char *end = NULL;
    char tmp[64];
    strncpy(tmp, args.c_str(), sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = 0;

    if (strcmp(act, "read") == 0) {
        uint32_t addr = (uint32_t)strtoul(tmp, &end, 0);
        if (end == tmp)
            return CommandOutput{String(act), String(), String("bad address")};
        uint32_t v = regRead(addr);
        char out[80];
        snprintf(out, sizeof(out), "{\"addr\":\"0x%08X\",\"value\":%lu,\"hex\":\"0x%08X\"}",
                 (unsigned)addr, (unsigned long)v, (unsigned)v);
        return CommandOutput{String(act), String(out), String()};
    }
    if (strcmp(act, "write") == 0) {
        char *comma = strchr(tmp, ',');
        if (!comma)
            return CommandOutput{String(act), String(), String("bad format, expect addr,value")};
        *comma = 0;
        uint32_t addr = (uint32_t)strtoul(tmp, &end, 0);
        if (end == tmp)
            return CommandOutput{String(act), String(), String("bad address")};
        uint32_t value = (uint32_t)strtoul(comma + 1, &end, 0);
        regWrite(addr, value);
        char out[64];
        snprintf(out, sizeof(out), "{\"addr\":\"0x%08X\",\"value\":%lu,\"hex\":\"0x%08X\"}",
                 (unsigned)addr, (unsigned long)value, (unsigned)value);
        return CommandOutput{String(act), String(out), String()};
    }
    if (strcmp(act, "bit_set") == 0 || strcmp(act, "bit_clear") == 0) {
        char *comma = strchr(tmp, ',');
        if (!comma)
            return CommandOutput{String(act), String(), String("bad format, expect addr,bit")};
        *comma = 0;
        uint32_t addr = (uint32_t)strtoul(tmp, &end, 0);
        if (end == tmp)
            return CommandOutput{String(act), String(), String("bad address")};
        unsigned long bit = strtoul(comma + 1, &end, 0);
        if (bit > 31)
            return CommandOutput{String(act), String(), String("bit must be 0..31")};
        uint32_t v = regRead(addr);
        if (strcmp(act, "bit_set") == 0) v |= (1UL << bit);
        else                             v &= ~(1UL << bit);
        regWrite(addr, v);
        char out[96];
        snprintf(out, sizeof(out),
                 "{\"addr\":\"0x%08X\",\"bit\":%lu,\"value\":%lu,\"hex\":\"0x%08X\"}",
                 (unsigned)addr, bit, (unsigned long)v, (unsigned)v);
        return CommandOutput{String(act), String(out), String()};
    }
    if (strcmp(act, "info") == 0) {
        return CommandOutput{String(act),
            String("{\"ability\":\"RegAbility\",\"desc\":\"MCU 寄存器读写\","
                   "\"width\":32,\"type\":\"memory-mapped\"}"), String()};
    }
    return CommandOutput{String(act), String(), String("unsupported command: ") + act};
}

} // namespace fe
