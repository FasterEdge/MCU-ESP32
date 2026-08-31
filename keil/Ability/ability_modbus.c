// ability_modbus.c — ModbusAbility 实现（Keil/裸机 C 版）
// set_unit_id / get_unit_id / read_holding / read_input / read_coils /
// read_discrete / write_holding / write_coil
// ESP32 作为 Modbus RTU 从站（寄存器表存于 RAM），帧收发经 fe_port UART。
#include "fe_ability.h"
#include "fe_port.h"

// CRC16 (Modbus)
static uint16_t modbus_crc(const uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++)
            crc = (crc & 1) ? ((crc >> 1) ^ 0xA001) : (crc >> 1);
    }
    return crc;
}

fe_output_t ability_modbus_dispatch(void *inst, const char *act, const char *args) {
    modbus_ability_t *self = (modbus_ability_t *)inst;

    if (strcmp(act, "set_unit_id") == 0) {
        int id = args ? atoi(args) : 0;
        if (id <= 0 || id > 247) return fe_err(act, "invalid unit id");
        self->unit_id = (uint8_t)id;
        char out[32];
        snprintf(out, sizeof(out), "unit_id=%d", id);
        return fe_ok(act, out);
    }
    if (strcmp(act, "get_unit_id") == 0) {
        char out[32];
        snprintf(out, sizeof(out), "unit_id=%u", self->unit_id);
        return fe_ok(act, out);
    }

    // 通用参数解析：addr[,count]
    int addr = 0, count = 1;
    if (args && args[0]) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%s", args);
        char *comma = strchr(buf, ',');
        addr = atoi(buf);
        if (comma) count = atoi(comma + 1);
    }
    if (addr < 0 || count < 0) return fe_err(act, "bad args");
    if (count > 42) return fe_err(act, "count too large");

    if (strcmp(act, "read_holding") == 0) {
        if (addr + count > 64) return fe_err(act, "addr out of range");
        char out[256];
        size_t n = 0;
        out[n++] = '[';
        for (int i = 0; i < count; i++) {
            if (i) out[n++] = ',';
            n += (size_t)snprintf(out + n, sizeof(out) - n, "%u", self->holding_regs[addr + i]);
        }
        out[n++] = ']'; out[n] = 0;
        return fe_ok(act, out);
    }
    if (strcmp(act, "read_input") == 0) {
        if (addr + count > 64) return fe_err(act, "addr out of range");
        char out[256];
        size_t n = 0;
        out[n++] = '[';
        for (int i = 0; i < count; i++) {
            if (i) out[n++] = ',';
            n += (size_t)snprintf(out + n, sizeof(out) - n, "%u", self->input_regs[addr + i]);
        }
        out[n++] = ']'; out[n] = 0;
        return fe_ok(act, out);
    }
    if (strcmp(act, "read_coils") == 0) {
        if (addr + count > 64) return fe_err(act, "addr out of range");
        char out[192];
        size_t n = 0;
        out[n++] = '[';
        for (int i = 0; i < count; i++) {
            if (i) out[n++] = ',';
            out[n++] = self->coils[addr + i] ? '1' : '0';
        }
        out[n++] = ']'; out[n] = 0;
        return fe_ok(act, out);
    }
    if (strcmp(act, "read_discrete") == 0) {
        if (addr + count > 64) return fe_err(act, "addr out of range");
        char out[192];
        size_t n = 0;
        out[n++] = '[';
        for (int i = 0; i < count; i++) {
            if (i) out[n++] = ',';
            out[n++] = self->discrete_inputs[addr + i] ? '1' : '0';
        }
        out[n++] = ']'; out[n] = 0;
        return fe_ok(act, out);
    }
    if (strcmp(act, "write_holding") == 0) {
        // 参数：addr,value
        if (!args || !args[0]) return fe_err(act, "expect addr,value");
        char buf[32];
        snprintf(buf, sizeof(buf), "%s", args);
        char *comma = strchr(buf, ',');
        if (!comma) return fe_err(act, "expect addr,value");
        int a = atoi(buf);
        uint16_t v = (uint16_t)atoi(comma + 1);
        if (a < 0 || a >= 64) return fe_err(act, "addr out of range");
        self->holding_regs[a] = v;
        return fe_ok(act, "{\"written\":true}");
    }
    if (strcmp(act, "write_coil") == 0) {
        if (!args || !args[0]) return fe_err(act, "expect addr,value");
        char buf[32];
        snprintf(buf, sizeof(buf), "%s", args);
        char *comma = strchr(buf, ',');
        if (!comma) return fe_err(act, "expect addr,value");
        int a = atoi(buf);
        bool v = atoi(comma + 1) != 0;
        if (a < 0 || a >= 64) return fe_err(act, "addr out of range");
        self->coils[a] = v;
        return fe_ok(act, "{\"written\":true}");
    }
    return fe_err(act, "unsupported command");
}

// 供移植层使用的 RTU 从站服务入口：收到完整请求帧后构造响应帧。
// 完整实现见 TODO（可在 main 轮询中调用）。
void modbus_slave_service(modbus_ability_t *self, const uint8_t *req, size_t len) {
    // TODO: 解析 RTU 帧（unit id + 功能码 0x03/0x04/0x01/0x02/0x06/0x05），
    //       构造响应并经 fe_port_uart_write 返回。
    (void)self; (void)req; (void)len;
    (void)modbus_crc;
}