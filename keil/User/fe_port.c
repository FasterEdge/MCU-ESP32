// fe_port.c — FasterEdge MCU 平台移植层参考实现（Keil/裸机 C 版）
// 本文件是跨 SDK 移植模板，目标 MCU/SDK 未指定；所有 REQUIRED_PORT_HOOK 必须由目标适配层实现，禁止把占位返回值视为可用硬件实现。
// ESP32 参考实现片段见文件末尾注释（基于 ESP-IDF API）。
#include "fe_port.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================
// 串口（UART）
// ============================================================
static fe_port_uart_rx_cb_t g_rx_cb = NULL;
static void *g_rx_user = NULL;

void fe_port_uart_init(uint8_t port, uint32_t baud, fe_port_uart_rx_cb_t rx_cb, void *user) {
    g_rx_cb = rx_cb;
    g_rx_user = user;
    // REQUIRED_PORT_HOOK: 初始化 UART(port, baud)；配置 RX 中断并调用 g_rx_cb 回传字节
    (void)port; (void)baud;
}

size_t fe_port_uart_write(uint8_t port, const uint8_t *data, size_t len) {
    // REQUIRED_PORT_HOOK: 发送 len 字节到 UART(port)
    (void)port; (void)data;
    return len;   // 返回实际发送字节数
}

bool fe_port_uart_available(uint8_t port) {
    (void)port;
    // REQUIRED_PORT_HOOK: 返回 RX FIFO 是否有数据
    return false;
}

int fe_port_uart_read(uint8_t port) {
    (void)port;
    // REQUIRED_PORT_HOOK: 读取一个字节，无数据返回 -1
    return -1;
}

void fe_port_uart_close(uint8_t port) {
    (void)port;
    // REQUIRED_PORT_HOOK: 关闭 UART
}

// ============================================================
// 非易失存储（NVS）
// ============================================================
bool fe_port_nvs_get_str(const char *ns, const char *key, char *out, size_t outlen) {
    // REQUIRED_PORT_HOOK: 从 NVS(ns) 读 key -> out
    (void)ns; (void)key; (void)out; (void)outlen;
    return false;
}

bool fe_port_nvs_set_str(const char *ns, const char *key, const char *value) {
    // REQUIRED_PORT_HOOK: 写 key -> NVS(ns)
    (void)ns; (void)key; (void)value;
    return true;
}

bool fe_port_nvs_remove(const char *ns, const char *key) {
    (void)ns; (void)key;
    // REQUIRED_PORT_HOOK: 删除键
    return true;
}

bool fe_port_nvs_get_u32(const char *ns, const char *key, uint32_t *out) {
    (void)ns; (void)key; (void)out;
    // REQUIRED_PORT_HOOK: 读 32 位整数
    return false;
}

bool fe_port_nvs_set_u32(const char *ns, const char *key, uint32_t value) {
    (void)ns; (void)key; (void)value;
    // REQUIRED_PORT_HOOK: 写 32 位整数
    return true;
}

// ============================================================
// 系统时间
// ============================================================
uint64_t fe_port_time_now(void) {
    // REQUIRED_PORT_HOOK: 返回当前 epoch 秒
    return 0;
}

void fe_port_time_set(uint64_t epoch) {
    // REQUIRED_PORT_HOOK: 设置 RTC
    (void)epoch;
}

int fe_port_time_sync_ntp(const char *server) {
    // REQUIRED_PORT_HOOK: 通过 SNTP 客户端同步时间（server 可为 NULL 用默认）
    (void)server;
    return -1;
}

// ============================================================
// 随机数
// ============================================================
void fe_port_random_fill(uint8_t *buf, size_t len) {
    // REQUIRED_PORT_HOOK: 填充硬件随机数
    for (size_t i = 0; i < len; i++) buf[i] = (uint8_t)(i * 31 + 7);
}

// ============================================================
// 网络
// ============================================================
bool fe_port_wifi_connected(void) {
    // REQUIRED_PORT_HOOK: 返回 WiFi 连接状态
    return false;
}

void fe_port_wifi_ip(char *out, size_t outlen) {
    // REQUIRED_PORT_HOOK: 写入本机 IP
    snprintf(out, outlen, "0.0.0.0");
}

int fe_port_tcp_connect(const char *host, uint16_t port) {
    (void)host; (void)port;
    // REQUIRED_PORT_HOOK: 建立 TCP 连接
    return -1;
}

size_t fe_port_tcp_write(const uint8_t *data, size_t len) {
    (void)data;
    // REQUIRED_PORT_HOOK: TCP 发送
    return len;
}

int fe_port_tcp_read(uint8_t *buf, size_t len) {
    (void)buf; (void)len;
    // REQUIRED_PORT_HOOK: TCP 读取，返回字节数（0 = 无数据，-1 = 断开）
    return 0;
}

void fe_port_tcp_close(void) {
    // REQUIRED_PORT_HOOK: 关闭 TCP
}

void fe_port_delay_ms(uint32_t ms) {
    // REQUIRED_PORT_HOOK: 阻塞延时
    (void)ms;
}

// ============================================================
// GPIO —— 见文件末尾 ESP-IDF 参考（driver/gpio.h）
// ============================================================
int fe_port_gpio_set_mode(uint8_t pin, const char *mode) {
    // REQUIRED_PORT_HOOK: 设置引脚模式（input / output / input_pullup）
    (void)pin; (void)mode;
    return 0;
}

int fe_port_gpio_write(uint8_t pin, uint8_t level) {
    // REQUIRED_PORT_HOOK: 输出电平 0/1
    (void)pin; (void)level;
    return 0;
}

int fe_port_gpio_read(uint8_t pin) {
    // REQUIRED_PORT_HOOK: 读输入电平
    (void)pin;
    return 0;
}

// ============================================================
// 芯片信息 —— 见文件末尾 ESP-IDF 参考（esp_chip_info.h / esp_efuse）
// ============================================================
void fe_port_chip_info(char *out, size_t outlen) {
    // REQUIRED_PORT_HOOK: 生成芯片信息 JSON，如 {"chip":"ESP32","cores":2,...}
    snprintf(out, outlen, "{\"chip\":\"ESP32\"}");
}

/*
 * ============================================================
 * ESP32 (ESP-IDF) 参考实现片段
 * ============================================================
 * 在 ESP-IDF 工程中，上述 REQUIRED_PORT_HOOK 可替换为：
 *
 *   // 串口
 *   #include "driver/uart.h"
 *   #include "driver/gpio.h"
 *   #define FE_UART_NUM UART_NUM_0
 *   void fe_port_uart_init(uint8_t p, uint32_t b, fe_port_uart_rx_cb_t cb, void *u) {
 *       uart_config_t c = {.baud_rate=b,.data_bits=UART_DATA_8_BITS,
 *                          .parity=UART_PARITY_DISABLE,.stop_bits=UART_STOP_BITS_1,
 *                          .flow_ctrl=UART_HW_FLOWCTRL_DISABLE};
 *       uart_param_config(FE_UART_NUM, &c);
 *       uart_driver_install(FE_UART_NUM, 1024, 0, 0, NULL, 0);
 *       uart_set_pin(FE_UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE,
 *                    UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
 *       // RX 轮询在 main 循环或 RTOS 任务中调用 fe_port_uart_available/read
 *   }
 *   size_t fe_port_uart_write(uint8_t p, const uint8_t *d, size_t l) {
 *       return uart_write_bytes(FE_UART_NUM, d, l);
 *   }
 *   int fe_port_uart_read(uint8_t p) {
 *       uint8_t b; int n = uart_read_bytes(FE_UART_NUM, &b, 1, 0);
 *       return n > 0 ? b : -1;
 *   }
 *
 *   // NVS
 *   #include "nvs_flash.h"
 *   #include "nvs.h"
 *   bool fe_port_nvs_get_str(const char *ns, const char *k, char *o, size_t l) {
 *       nvs_handle_t h; if (nvs_open(ns, NVS_READONLY, &h) != ESP_OK) return false;
 *       size_t sz = l; bool ok = nvs_get_str(h, k, o, &sz) == ESP_OK;
 *       nvs_close(h); return ok;
 *   }
 *
 *   // 时间
 *   #include "esp_sntp.h"
 *   int fe_port_time_sync_ntp(const char *s) {
 *       esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
 *       esp_sntp_setservername(0, s ? s : "pool.ntp.org");
 *       esp_sntp_init();
 *       return 0;
 *   }
 *
 *   // 随机数
 *   #include "esp_random.h"
 *   void fe_port_random_fill(uint8_t *b, size_t l) {
 *       for (size_t i = 0; i < l; i += 4) {
 *           uint32_t r = esp_random();
 *           for (size_t j = 0; j < 4 && i + j < l; j++) b[i+j] = (uint8_t)(r >> (j*8));
 *       }
 *   }
 *
 *   // 延时
 *   #include "freertos/FreeRTOS.h"
 *   #include "freertos/task.h"
 *   void fe_port_delay_ms(uint32_t ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }
 *
 *   // GPIO
 *   #include "driver/gpio.h"
 *   int fe_port_gpio_set_mode(uint8_t pin, const char *mode) {
 *       gpio_config_t cfg = {0};
 *       if (strcmp(mode, "input") == 0)       cfg.mode = GPIO_MODE_INPUT;
 *       else if (strcmp(mode, "input_pullup") == 0) { cfg.mode = GPIO_MODE_INPUT; cfg.pull_up_en = GPIO_PULLUP_ENABLE; }
 *       else cfg.mode = GPIO_MODE_OUTPUT;
 *       cfg.pin_bit_mask = 1ULL << pin;
 *       return gpio_config(&cfg) == ESP_OK ? 0 : -1;
 *   }
 *   int fe_port_gpio_write(uint8_t pin, uint8_t level) {
 *       return gpio_set_level(pin, level) == ESP_OK ? 0 : -1;
 *   }
 *   int fe_port_gpio_read(uint8_t pin) {
 *       int v = gpio_get_level(pin); return v >= 0 ? v : -1;
 *   }
 *
 *   // 芯片信息
 *   #include "esp_chip_info.h"
 *   #include "esp_flash.h"
 *   void fe_port_chip_info(char *out, size_t l) {
 *       esp_chip_info_t ci; esp_chip_info(&ci);
 *       uint32_t flash = 0; esp_flash_get_size(NULL, &flash);
 *       snprintf(out, l, "{\"chip\":\"%s\",\"cores\":%d,\"features\":\"0x%08x\",\"flashBytes\":%lu}",
 *                CONFIG_IDF_TARGET, ci.cores, ci.features, (unsigned long)flash);
 *   }
 *
 * ============================================================
 */