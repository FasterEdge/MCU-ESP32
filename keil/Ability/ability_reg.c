// FasterEdge 开源项目 - Github: https://github.com/FasterEdge - Gitee: https://gitee.com/FasterEdge
// ability_reg.c — RegAbility 实现（Keil/裸机 C 版，MCU 专有）
// MCU 专有能力：直接读写内存映射外设寄存器。
//   read <addr>           读 32 位寄存器，addr 为十六进制地址（如 0x3FF44004）
//   write <addr>,<value>  写 32 位
//   bit_set <addr>,<bit>  置位
//   bit_clear <addr>,<bit> 清位
//   info                  说明
// 注意：请谨慎使用，误写寄存器可能导致系统异常。
#include "fe_ability.h"

static uint32_t reg_read(uint32_t addr) {
    return *(volatile uint32_t *)(uintptr_t)addr;
}

static void reg_write(uint32_t addr, uint32_t value) {
    *(volatile uint32_t *)(uintptr_t)addr = value;
}

fe_output_t ability_reg_dispatch(void *inst, const char *act, const char *args) {
    (void)inst;
    char tmp[64];
    char *end = NULL;

    snprintf(tmp, sizeof(tmp), "%s", args ? args : "");

    if (strcmp(act, "read") == 0) {
        uint32_t addr = (uint32_t)strtoul(tmp, &end, 0);
        if (end == tmp) return fe_err(act, "bad address");
        uint32_t v = reg_read(addr);
        char out[80];
        snprintf(out, sizeof(out), "{\"addr\":\"0x%08X\",\"value\":%lu,\"hex\":\"0x%08X\"}",
                 (unsigned)addr, (unsigned long)v, (unsigned)v);
        return fe_ok(act, out);
    }
    if (strcmp(act, "write") == 0) {
        char *comma = strchr(tmp, ',');
        if (!comma) return fe_err(act, "bad format, expect addr,value");
        *comma = 0;
        uint32_t addr = (uint32_t)strtoul(tmp, &end, 0);
        if (end == tmp) return fe_err(act, "bad address");
        uint32_t value = (uint32_t)strtoul(comma + 1, &end, 0);
        reg_write(addr, value);
        char out[64];
        snprintf(out, sizeof(out), "{\"addr\":\"0x%08X\",\"value\":%lu,\"hex\":\"0x%08X\"}",
                 (unsigned)addr, (unsigned long)value, (unsigned)value);
        return fe_ok(act, out);
    }
    if (strcmp(act, "bit_set") == 0 || strcmp(act, "bit_clear") == 0) {
        char *comma = strchr(tmp, ',');
        if (!comma) return fe_err(act, "bad format, expect addr,bit");
        *comma = 0;
        uint32_t addr = (uint32_t)strtoul(tmp, &end, 0);
        if (end == tmp) return fe_err(act, "bad address");
        unsigned long bit = strtoul(comma + 1, &end, 0);
        if (bit > 31) return fe_err(act, "bit must be 0..31");
        uint32_t v = reg_read(addr);
        if (strcmp(act, "bit_set") == 0) v |= (1UL << bit);
        else                             v &= ~(1UL << bit);
        reg_write(addr, v);
        char out[96];
        snprintf(out, sizeof(out),
                 "{\"addr\":\"0x%08X\",\"bit\":%lu,\"value\":%lu,\"hex\":\"0x%08X\"}",
                 (unsigned)addr, bit, (unsigned long)v, (unsigned)v);
        return fe_ok(act, out);
    }
    if (strcmp(act, "info") == 0) {
        return fe_ok(act,
            "{\"ability\":\"RegAbility\",\"desc\":\"MCU 寄存器读写\","
            "\"width\":32,\"type\":\"memory-mapped\"}");
    }
    return fe_err(act, "unsupported command");
}
