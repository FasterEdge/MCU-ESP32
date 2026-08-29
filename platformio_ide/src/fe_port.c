// fe_port.c — FasterEdge MCU 平台移植层（ESP32 / ESP-IDF 版）
// 用于 platformio_ide 工程（VS Code + PlatformIO 插件）：
//   platform = espressif32, framework = espidf
// 本文件是 keil 版 TODO 移植层的真实实现：
//   UART  -> driver/uart.h
//   NVS   -> nvs_flash.h / nvs.h（key 规范化，兼容点号路径）
//   时间   -> gettimeofday + esp_sntp
//   随机   -> esp_random
//   WiFi/TCP -> esp_wifi / esp_netif / lwip
#include "fe_port.h"

#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#include "driver/uart.h"
#include "driver/gpio.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_sntp.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_chip_info.h"
#include "esp_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/sockets.h"

#define FE_TAG "fe_port"
#define FE_UART_NUM UART_NUM_0

// ============================================================
// 串口（UART）
// ============================================================
static fe_port_uart_rx_cb_t g_rx_cb = NULL;
static void *g_rx_user = NULL;

void fe_port_uart_init(uint8_t port, uint32_t baud, fe_port_uart_rx_cb_t rx_cb, void *user) {
    uart_config_t cfg = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    (void)port;
    uart_param_config(FE_UART_NUM, &cfg);
    uart_driver_install(FE_UART_NUM, 1024, 0, 0, NULL, 0);
    g_rx_cb = rx_cb;
    g_rx_user = user;
}

size_t fe_port_uart_write(uint8_t port, const uint8_t *data, size_t len) {
    (void)port;
    uart_write_bytes(FE_UART_NUM, data, len);
    uart_wait_tx_done(FE_UART_NUM, 100);
    return len;
}

bool fe_port_uart_available(uint8_t port) {
    (void)port;
    size_t n = 0;
    uart_get_buffered_data_len(FE_UART_NUM, &n);
    return n > 0;
}

int fe_port_uart_read(uint8_t port) {
    (void)port;
    uint8_t b = 0;
    int n = uart_read_bytes(FE_UART_NUM, &b, 1, 0);
    return n > 0 ? (int)b : -1;
}

void fe_port_uart_close(uint8_t port) {
    (void)port;
    uart_driver_delete(FE_UART_NUM);
}

// ============================================================
// 非易失存储（NVS）
// ============================================================
// NVS key 约束：<=15 字符，仅 [a-zA-Z0-9_]。把点号/斜杠路径
// 规范化为下划线并截断，使 data_ConfigData 的扁平点号 key 可用。
static void norm_nvs_key(const char *in, char *out, size_t outlen) {
    size_t n = 0;
    for (const char *p = in; *p && n + 1 < outlen && n < 14; p++) {
        char c = *p;
        if (c == '.' || c == '/') c = '_';
        out[n++] = c;
    }
    out[n] = 0;
}

static bool nvs_open_ns(const char *ns, bool write, nvs_handle_t *h) {
    esp_err_t err = nvs_open(ns, write ? NVS_READWRITE : NVS_READONLY, h);
    return err == ESP_OK;
}

bool fe_port_nvs_get_str(const char *ns, const char *key, char *out, size_t outlen) {
    nvs_handle_t h;
    char k[16];
    size_t sz = outlen;
    esp_err_t err;
    norm_nvs_key(key, k, sizeof(k));
    if (!nvs_open_ns(ns, false, &h)) return false;
    err = nvs_get_str(h, k, out, &sz);
    nvs_close(h);
    return err == ESP_OK;
}

bool fe_port_nvs_set_str(const char *ns, const char *key, const char *value) {
    nvs_handle_t h;
    char k[16];
    bool ok;
    norm_nvs_key(key, k, sizeof(k));
    if (!nvs_open_ns(ns, true, &h)) return false;
    ok = (nvs_set_str(h, k, value) == ESP_OK) && (nvs_commit(h) == ESP_OK);
    nvs_close(h);
    return ok;
}

bool fe_port_nvs_remove(const char *ns, const char *key) {
    nvs_handle_t h;
    char k[16];
    bool ok;
    norm_nvs_key(key, k, sizeof(k));
    if (!nvs_open_ns(ns, true, &h)) return false;
    ok = (nvs_erase_key(h, k) == ESP_OK) && (nvs_commit(h) == ESP_OK);
    nvs_close(h);
    return ok;
}

bool fe_port_nvs_get_u32(const char *ns, const char *key, uint32_t *out) {
    nvs_handle_t h;
    char k[16];
    esp_err_t err;
    norm_nvs_key(key, k, sizeof(k));
    if (!nvs_open_ns(ns, false, &h)) return false;
    err = nvs_get_u32(h, k, out);
    nvs_close(h);
    return err == ESP_OK;
}

bool fe_port_nvs_set_u32(const char *ns, const char *key, uint32_t value) {
    nvs_handle_t h;
    char k[16];
    bool ok;
    norm_nvs_key(key, k, sizeof(k));
    if (!nvs_open_ns(ns, true, &h)) return false;
    ok = (nvs_set_u32(h, k, value) == ESP_OK) && (nvs_commit(h) == ESP_OK);
    nvs_close(h);
    return ok;
}

// ============================================================
// 系统时间
// ============================================================
uint64_t fe_port_time_now(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec;
}

void fe_port_time_set(uint64_t epoch) {
    struct timeval tv;
    tv.tv_sec = (time_t)epoch;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
}

int fe_port_time_sync_ntp(const char *server) {
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, server ? server : "pool.ntp.org");
    esp_sntp_init();
    return 0;
}

// ============================================================
// 随机数
// ============================================================
void fe_port_random_fill(uint8_t *buf, size_t len) {
    size_t i;
    for (i = 0; i + 4 <= len; i += 4) {
        uint32_t r = esp_random();
        memcpy(buf + i, &r, 4);
    }
    if (i < len) {
        uint32_t r = esp_random();
        memcpy(buf + i, &r, len - i);
    }
}

// ============================================================
// 网络（WiFi / TCP）
// ============================================================
// WiFi 由外部代码（sdkconfig wifi SSID/PSK 或自动连接）建立，
// 这里仅查询状态。TCP 使用 lwip BSD socket。
bool fe_port_wifi_connected(void) {
    esp_netif_ip_info_t ip;
    esp_netif_t *nif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (!nif) return false;
    if (esp_netif_get_ip_info(nif, &ip) != ESP_OK) return false;
    return ip.ip.addr != 0;
}

void fe_port_wifi_ip(char *out, size_t outlen) {
    esp_netif_ip_info_t ip;
    uint32_t a;
    esp_netif_t *nif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (nif && esp_netif_get_ip_info(nif, &ip) == ESP_OK && ip.ip.addr != 0) {
        // 不依赖 IPSTR/IP2STR 宏（不同 IDF 版本展开不同），按字节格式化
        a = ip.ip.addr;
        snprintf(out, outlen, "%u.%u.%u.%u",
                 (unsigned)(a & 0xff), (unsigned)((a >> 8) & 0xff),
                 (unsigned)((a >> 16) & 0xff), (unsigned)((a >> 24) & 0xff));
    } else {
        snprintf(out, outlen, "0.0.0.0");
    }
}

static int g_sock = -1;

int fe_port_tcp_connect(const char *host, uint16_t port) {
    struct sockaddr_in sa;
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0) return -1;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = inet_addr(host);
    if (connect(fd, (struct sockaddr *)&sa, sizeof(sa)) != 0) {
        close(fd);
        return -1;
    }
    g_sock = fd;
    return 0;
}

size_t fe_port_tcp_write(const uint8_t *data, size_t len) {
    if (g_sock < 0) return 0;
    int n = send(g_sock, data, len, 0);
    return n > 0 ? (size_t)n : 0;
}

int fe_port_tcp_read(uint8_t *buf, size_t len) {
    if (g_sock < 0) return -1;
    int n = recv(g_sock, buf, len, 0);
    return n;   // 0=对端关闭, -1=错误
}

void fe_port_tcp_close(void) {
    if (g_sock >= 0) { close(g_sock); g_sock = -1; }
}

// ============================================================
// GPIO
// ============================================================
int fe_port_gpio_set_mode(uint8_t pin, const char *mode) {
    gpio_config_t cfg = {0};
    if (strcmp(mode, "input") == 0) {
        cfg.mode = GPIO_MODE_INPUT;
    } else if (strcmp(mode, "input_pullup") == 0) {
        cfg.mode = GPIO_MODE_INPUT;
        cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    } else if (strcmp(mode, "output") == 0) {
        cfg.mode = GPIO_MODE_OUTPUT;
    } else {
        return -1;
    }
    cfg.pin_bit_mask = 1ULL << pin;
    return gpio_config(&cfg) == ESP_OK ? 0 : -1;
}

int fe_port_gpio_write(uint8_t pin, uint8_t level) {
    return gpio_set_level(pin, level) == ESP_OK ? 0 : -1;
}

int fe_port_gpio_read(uint8_t pin) {
    int v = gpio_get_level(pin);
    return v >= 0 ? v : -1;
}

// ============================================================
// 芯片信息
// ============================================================
void fe_port_chip_info(char *out, size_t outlen) {
    esp_chip_info_t ci;
    uint32_t flash = 0;
    esp_chip_info(&ci);
    esp_flash_get_size(NULL, &flash);
    snprintf(out, outlen,
             "{\"chip\":\"%s\",\"cores\":%d,\"features\":\"0x%08x\",\"flashBytes\":%lu}",
             CONFIG_IDF_TARGET, ci.cores, ci.features, (unsigned long)flash);
}

// ============================================================
// 延时
// ============================================================
void fe_port_delay_ms(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

// ============================================================
// 初始化（app_main 调用）
// ============================================================
void fe_port_init_platform(void) {
    // NVS 初始化（失败通常表示空间已满，抹除重建后重试一次）
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    // 网络栈（WiFi 由 sdkconfig 或上层代码配置连接）
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    (void)FE_TAG;
}