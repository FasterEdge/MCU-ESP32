// ability_serial.cpp — SerialAbility 实现（Arduino 版）
// open / close / write / read / is_open / set_config / get_config / list_ports
// ESP32 Arduino 的 Serial 由 HardwareSerial 提供；此处用 HWCDC/USB 或 UART 说明。
#include "fe_ability.h"

namespace fe {

// 端口编号：0 默认（USB CDC / 控制台串口），1..N 为 UARTx
static HardwareSerial *getPort(int n) {
    switch (n) {
        case 0:  return &Serial;      // 默认控制台
#if SOC_UART_NUM > 1
        case 1:  return &Serial1;     // UART1
#endif
#if SOC_UART_NUM > 2
        case 2:  return &Serial2;     // UART2
#endif
        default: return nullptr;
    }
}

CommandOutput serialAbilityDispatch(void *inst, const char *act, const String &args) {
    SerialAbility *self = static_cast<SerialAbility *>(inst);

    if (strcmp(act, "list_ports") == 0) {
        return CommandOutput{String(act), String("{\"ports\":[0,1,2]}"), String()};
    }
    if (strcmp(act, "set_config") == 0) {
        // 参数：port,baud,data,parity,stop  (逗号分隔，可省略)
        int p = args.length() ? args.substring(0, args.indexOf(',')).toInt() : 0;
        int baud = 115200;
        int i1 = args.indexOf(',');
        if (i1 >= 0) {
            String rest = args.substring(i1 + 1);
            int i2 = rest.indexOf(',');
            baud = (i2 >= 0 ? rest.substring(0, i2) : rest).toInt();
        }
        self->baud = baud;
        return CommandOutput{String(act), String("port=") + p + " baud=" + baud, String()};
    }
    if (strcmp(act, "get_config") == 0) {
        return CommandOutput{String(act),
            String("{\"open\":") + (self->open ? "true" : "false") + ",\"baud\":" + self->baud + "}", String()};
    }
    if (strcmp(act, "open") == 0) {
        int p = args.length() ? args.toInt() : 0;
        HardwareSerial *s = getPort(p);
        if (!s) return CommandOutput{String(act), String(), String("invalid port: ") + p};
        s->begin(self->baud);
        self->open = true;
        return CommandOutput{String(act), String("port=") + p + " opened", String()};
    }
    if (strcmp(act, "close") == 0) {
        int p = args.length() ? args.toInt() : 0;
        HardwareSerial *s = getPort(p);
        if (s) s->end();
        self->open = false;
        return CommandOutput{String(act), String("closed"), String()};
    }
    if (strcmp(act, "is_open") == 0) {
        return CommandOutput{String(act),
            String("{\"open\":") + (self->open ? "true" : "false") + "}", String()};
    }
    if (strcmp(act, "write") == 0) {
        // args 即要发送的原始字节/文本
        int p = args.length() ? args.substring(0, args.indexOf(',')).toInt() : 0;
        HardwareSerial *s = getPort(p);
        if (!s || !self->open) return CommandOutput{String(act), String(), String("port not open")};
        size_t n = s->write((const uint8_t *)args.c_str(), args.length());
        return CommandOutput{String(act), String("bytes=") + (long)n, String()};
    }
    if (strcmp(act, "read") == 0) {
        // 读取全部可用字节（hex 输出）
        int p = args.length() ? args.toInt() : 0;
        HardwareSerial *s = getPort(p);
        if (!s || !self->open) return CommandOutput{String(act), String(), String("port not open")};
        String hex;
        while (s->available()) {
            uint8_t b = s->read();
            if (b < 16) hex += "0";
            hex += String(b, HEX);
        }
        return CommandOutput{String(act), hex, String()};
    }
    return CommandOutput{String(act), String(), String("unsupported command: ") + act};
}

} // namespace fe