// FasterEdge 开源项目 - Github: https://github.com/FasterEdge - Gitee: https://gitee.com/FasterEdge
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
    // 42 worst-case uint16 values fit in the fixed 256-byte JSON buffer.
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
// req 必须是一个完整的 RTU 帧（地址、功能码、PDU、CRC；CRC 低字节在前）。
// 服务入口不分配内存，响应最大为 135 字节（本实现的 64 项寄存器表）。
static void modbus_send_response(const uint8_t *pdu, size_t pdu_len) {
    uint8_t frame[135];
    uint16_t crc;
    size_t i;
    if (pdu_len + 2 > sizeof(frame)) return;
    for (i = 0; i < pdu_len; i++) frame[i] = pdu[i];
    crc = modbus_crc(frame, pdu_len);
    frame[pdu_len] = (uint8_t)(crc & 0xFF);
    frame[pdu_len + 1] = (uint8_t)(crc >> 8);
    (void)fe_port_uart_write(0, frame, pdu_len + 2);
}

static void modbus_send_exception(uint8_t unit, uint8_t function, uint8_t exception) {
    uint8_t pdu[3];
    pdu[0] = unit;
    pdu[1] = (uint8_t)(function | 0x80);
    pdu[2] = exception;
    modbus_send_response(pdu, 3);
}

static bool modbus_range_ok(uint16_t address, uint16_t count) {
    return count != 0 && address < 64 && count <= 64 - address;
}

void modbus_slave_service(modbus_ability_t *self, const uint8_t *req, size_t len) {
    uint8_t unit, function;
    uint16_t received_crc, calculated_crc;
    uint16_t address, count, value;
    uint8_t pdu[133];
    uint8_t byte_count, i, out_len;
    bool broadcast;

    // 最短请求是 8 字节；先做边界和 CRC 检查，坏帧静默丢弃。
    if (!self || !req || len < 8) return;
    unit = req[0];
    function = req[1];
    if (unit != self->unit_id && unit != 0) return;
    received_crc = (uint16_t)req[len - 2] | ((uint16_t)req[len - 1] << 8);
    calculated_crc = modbus_crc(req, len - 2);
    if (received_crc != calculated_crc) return;
    broadcast = (unit == 0);
    if (broadcast && (function < 5 || function > 6)) return;

    // 0x01/0x02/0x03/0x04：起始地址和数量均为大端序。
    if (function == 1 || function == 2 || function == 3 || function == 4) {
        if (len != 8) return;
        address = (uint16_t)(((uint16_t)req[2] << 8) | req[3]);
        count = (uint16_t)(((uint16_t)req[4] << 8) | req[5]);
        if (!modbus_range_ok(address, count)) {
            if (!broadcast) modbus_send_exception(unit, function, 2);
            return;
        }
        if (function == 1 || function == 2) {
            byte_count = (uint8_t)((count + 7) / 8);
            pdu[0] = unit; pdu[1] = function; pdu[2] = byte_count;
            for (i = 0; i < byte_count; i++) pdu[(uint8_t)(3 + i)] = 0;
            for (i = 0; i < count; i++) {
                uint8_t bit = (uint8_t)(address + i);
                bool state = (function == 1) ? self->coils[bit] : self->discrete_inputs[bit];
                if (state) pdu[(uint8_t)(3 + i / 8)] |= (uint8_t)(1 << (i % 8));
            }
            modbus_send_response(pdu, (size_t)byte_count + 3);
        } else {
            byte_count = (uint8_t)(count * 2);
            pdu[0] = unit; pdu[1] = function; pdu[2] = byte_count;
            out_len = 3;
            for (i = 0; i < count; i++) {
                value = (function == 3) ? self->holding_regs[address + i] : self->input_regs[address + i];
                pdu[out_len++] = (uint8_t)(value >> 8);
                pdu[out_len++] = (uint8_t)value;
            }
            modbus_send_response(pdu, (size_t)out_len);
        }
        return;
    }

    // 0x05：写单线圈，只接受 0xFF00 或 0x0000。
    if (function == 5) {
        if (len != 8) return;
        address = (uint16_t)(((uint16_t)req[2] << 8) | req[3]);
        value = (uint16_t)(((uint16_t)req[4] << 8) | req[5]);
        if (address >= 64) { if (!broadcast) modbus_send_exception(unit, function, 2); return; }
        if (value != 0x0000 && value != 0xFF00) { if (!broadcast) modbus_send_exception(unit, function, 3); return; }
        self->coils[address] = (value == 0xFF00);
        if (!broadcast) modbus_send_response(req, 6);
        return;
    }

    // 0x06：写单个保持寄存器；响应为请求 PDU 的回显。
    if (function == 6) {
        if (len != 8) return;
        address = (uint16_t)(((uint16_t)req[2] << 8) | req[3]);
        if (address >= 64) { if (!broadcast) modbus_send_exception(unit, function, 2); return; }
        value = (uint16_t)(((uint16_t)req[4] << 8) | req[5]);
        self->holding_regs[address] = value;
        if (!broadcast) modbus_send_response(req, 6);
        return;
    }
    if (!broadcast) modbus_send_exception(unit, function, 1);
}
