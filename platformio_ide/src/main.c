// main.c — FasterEdge MCU ESP32 / ESP-IDF 入口（platformio_ide 版）
// 串口命令解释器：输入 "data_xxx act args" / "ability_xxx act args"
// 示例：
//   ability_BaseAbility list_ability_names
//   data_ConfigData set wifi.ssid=MyNet
//   ability_TimeAbility sync_ntp
//   ability_ModbusAbility write_holding 0,42
#include "fe.h"
#include "fe_ability.h"
#include "fe_data.h"
#include "fe_port.h"
#include <string.h>

// 平台初始化（fe_port.c 提供）
void fe_port_init_platform(void);

// 命令行缓冲
static char line[256];
static size_t line_len = 0;

// 从串口读取一行（轮询）
static bool read_line(char *buf, size_t buflen, size_t *len) {
    while (fe_port_uart_available(0)) {
        int c = fe_port_uart_read(0);
        if (c < 0) break;
        if (c == '\n' || c == '\r') {
            if (line_len == 0) continue;
            memcpy(buf, line, line_len);
            buf[line_len] = 0;
            *len = line_len;
            line_len = 0;
            return true;
        }
        if (line_len + 1 < sizeof(line)) line[line_len++] = (char)c;
    }
    return false;
}

static void print_help(void) {
    char names[512];
    fe_port_uart_write(0, (const uint8_t *)"Usage: <data|ability>_<Name> <act> [args]\n", 46);
    fe_list_ability_names(fe_global_atom(), names, sizeof(names));
    fe_port_uart_write(0, (const uint8_t *)"abilities: ", 11);
    fe_port_uart_write(0, (const uint8_t *)names, strlen(names));
    fe_port_uart_write(0, (const uint8_t *)"\n", 1);
    fe_list_data_names(fe_global_atom(), names, sizeof(names));
    fe_port_uart_write(0, (const uint8_t *)"data: ", 6);
    fe_port_uart_write(0, (const uint8_t *)names, strlen(names));
    fe_port_uart_write(0, (const uint8_t *)"\nexamples:\n", 11);
    fe_port_uart_write(0, (const uint8_t *)"  ability_BaseAbility list_ability_names\n", 42);
    fe_port_uart_write(0, (const uint8_t *)"  ability_TimeAbility sync_ntp\n", 32);
    fe_port_uart_write(0, (const uint8_t *)"  ability_ModbusAbility write_holding 0,42\n", 45);
    fe_port_uart_write(0, (const uint8_t *)"  data_ConfigData set wifi.ssid=MyNet\n", 38);
}

void app_main(void) {
    // 平台初始化（NVS / 网络栈）
    fe_port_init_platform();
    fe_port_uart_init(0, 115200, NULL, NULL);

    const char *banner = "\nFasterEdge-MCU (ESP32 / PlatformIO ESP-IDF)\ninput: <data|ability>_<Name> <act> [args]  |  'help'\n";
    fe_port_uart_write(0, (const uint8_t *)banner, strlen(banner));

    fe_init_all();

    for (;;) {
        char buf[256];
        size_t len;
        if (read_line(buf, sizeof(buf), &len)) {
            char *sp1, *target, *rest, *sp2, *act, *args;
            fe_output_t out;
            char oline[512];
            while (len && (buf[len-1] == '\n' || buf[len-1] == '\r')) buf[--len] = 0;

            if (strcmp(buf, "help") == 0) { print_help(); continue; }

            sp1 = strchr(buf, ' ');
            if (!sp1) {
                fe_port_uart_write(0, (const uint8_t *)"bad command\n", 12);
                continue;
            }
            *sp1 = 0;
            target = buf;
            rest = sp1 + 1;
            sp2 = strchr(rest, ' ');
            if (sp2) { *sp2 = 0; act = rest; args = sp2 + 1; }
            else     { act = rest; args = NULL; }

            out = fe_execute(fe_global_atom(), target, act, args ? args : "");
            if (out.ok)
                snprintf(oline, sizeof(oline), "OK %s -> %s\n", out.name, out.value);
            else
                snprintf(oline, sizeof(oline), "ERR %s: %s\n", out.name, out.err);
            fe_port_uart_write(0, (const uint8_t *)oline, strlen(oline));
        }
        fe_port_delay_ms(10);
    }
}