// FasterEdge 开源项目 - Github: https://github.com/FasterEdge - Gitee: https://gitee.com/FasterEdge
// ability_modbus.cpp — ModbusAbility 实现（Arduino 版）
// set_unit_id / read_holding / read_input / read_coils / read_discrete /
// write_holding / write_coil
// 本能力将 ESP32 作为 Modbus RTU 从站（寄存器表存于 RAM），
// 底层帧收发通过 Serial（RS485 透传需外接 MAX485）。
#include "fe_ability.h"

namespace fe {

ModbusAbility::ModbusAbility() : unitId(1) {
    holdingRegs.assign(64, 0);
    inputRegs.assign(64, 0);
    coils.assign(64, false);
    discreteInputs.assign(64, false);
}

// CRC16 (Modbus)
static uint16_t modbusCrc(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xA001;
            else crc >>= 1;
        }
    }
    return crc;
}

CommandOutput modbusAbilityDispatch(void *inst, const char *act, const String &args) {
    ModbusAbility *self = static_cast<ModbusAbility *>(inst);

    if (strcmp(act, "set_unit_id") == 0) {
        uint8_t id = (uint8_t)args.toInt();
        if (id == 0 || id > 247)
            return CommandOutput{String(act), String(), String("invalid unit id")};
        self->unitId = id;
        return CommandOutput{String(act), String("unit_id=") + id, String()};
    }
    if (strcmp(act, "get_unit_id") == 0) {
        return CommandOutput{String(act), String("unit_id=") + (int)self->unitId, String()};
    }

    // 参数统一格式：addr[,count]（从 0 开始）
    int comma = args.indexOf(',');
    int addr = (comma > 0 ? args.substring(0, comma) : args).toInt();
    int count = (comma > 0 ? args.substring(comma + 1) : "1").toInt();
    if (addr < 0 || count < 0) return CommandOutput{String(act), String(), String("bad args")};

    if (strcmp(act, "read_holding") == 0) {
        if (addr + count > (int)self->holdingRegs.size())
            return CommandOutput{String(act), String(), String("addr out of range")};
        String out = "[";
        for (int i = 0; i < count; i++) { if (i) out += ","; out += self->holdingRegs[addr + i]; }
        out += "]";
        return CommandOutput{String(act), out, String()};
    }
    if (strcmp(act, "read_input") == 0) {
        if (addr + count > (int)self->inputRegs.size())
            return CommandOutput{String(act), String(), String("addr out of range")};
        String out = "[";
        for (int i = 0; i < count; i++) { if (i) out += ","; out += self->inputRegs[addr + i]; }
        out += "]";
        return CommandOutput{String(act), out, String()};
    }
    if (strcmp(act, "read_coils") == 0) {
        if (addr + count > (int)self->coils.size())
            return CommandOutput{String(act), String(), String("addr out of range")};
        String out = "[";
        for (int i = 0; i < count; i++) { if (i) out += ","; out += self->coils[addr + i] ? "1" : "0"; }
        out += "]";
        return CommandOutput{String(act), out, String()};
    }
    if (strcmp(act, "read_discrete") == 0) {
        if (addr + count > (int)self->discreteInputs.size())
            return CommandOutput{String(act), String(), String("addr out of range")};
        String out = "[";
        for (int i = 0; i < count; i++) { if (i) out += ","; out += self->discreteInputs[addr + i] ? "1" : "0"; }
        out += "]";
        return CommandOutput{String(act), out, String()};
    }
    if (strcmp(act, "write_holding") == 0) {
        // 参数：addr,value
        if (comma <= 0) return CommandOutput{String(act), String(), String("expect addr,value")};
        uint16_t value = (uint16_t)args.substring(comma + 1).toInt();
        if (addr >= (int)self->holdingRegs.size())
            return CommandOutput{String(act), String(), String("addr out of range")};
        self->holdingRegs[addr] = value;
        return CommandOutput{String(act), String("{\"written\":true}"), String()};
    }
    if (strcmp(act, "write_coil") == 0) {
        if (comma <= 0) return CommandOutput{String(act), String(), String("expect addr,value")};
        bool value = args.substring(comma + 1).toInt() != 0;
        if (addr >= (int)self->coils.size())
            return CommandOutput{String(act), String(), String("addr out of range")};
        self->coils[addr] = value;
        return CommandOutput{String(act), String("{\"written\":true}"), String()};
    }

    return CommandOutput{String(act), String(), String("unsupported command: ") + act};
}

} // namespace fe
