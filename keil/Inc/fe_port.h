// fe_port.h — FasterEdge MCU 平台移植层（Keil/裸机 C 版）
// 平台相关能力在此抽象：串口收发、NVS 存储、系统时间、随机数。
// 移植到具体 MCU 时只需实现本文件，Ability/Data 逻辑无需改动。
#ifndef FE_PORT_H
#define FE_PORT_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// 串口（UART）
// ============================================================
typedef void (*fe_port_uart_rx_cb_t)(uint8_t byte, void *user);

// 初始化串口：port 编号，baud 波特率，rx 回调（可为 NULL）
void fe_port_uart_init(uint8_t port, uint32_t baud, fe_port_uart_rx_cb_t rx_cb, void *user);
// 发送 len 字节，返回实际发送字节数
size_t fe_port_uart_write(uint8_t port, const uint8_t *data, size_t len);
// 是否有数据可读
bool fe_port_uart_available(uint8_t port);
// 读一个字节
int fe_port_uart_read(uint8_t port);
// 关闭串口
void fe_port_uart_close(uint8_t port);

// ============================================================
// 非易失存储（NVS / Flash）
// ============================================================
// 读字符串：ns 命名空间，key 键，out 输出缓冲，outlen 缓冲长度。
// 存在返回 true。
bool fe_port_nvs_get_str(const char *ns, const char *key, char *out, size_t outlen);
// 写字符串：成功返回 true
bool fe_port_nvs_set_str(const char *ns, const char *key, const char *value);
// 删除键：成功返回 true
bool fe_port_nvs_remove(const char *ns, const char *key);
// 读无符号整数
bool fe_port_nvs_get_u32(const char *ns, const char *key, uint32_t *out);
// 写无符号整数
bool fe_port_nvs_set_u32(const char *ns, const char *key, uint32_t value);

// ============================================================
// 系统时间（epoch 秒）
// ============================================================
// 读取当前 epoch 秒
uint64_t fe_port_time_now(void);
// 设置 epoch 秒
void fe_port_time_set(uint64_t epoch);
// 从 NTP 同步（server 可为 NULL 用默认）。成功返回 0，失败返回负错误码
int fe_port_time_sync_ntp(const char *server);

// ============================================================
// 随机数
// ============================================================
// 填充 len 字节随机数
void fe_port_random_fill(uint8_t *buf, size_t len);

// ============================================================
// 网络（WiFi / TCP）——MQTT 等需要
// ============================================================
// WiFi 是否已连接
bool fe_port_wifi_connected(void);
// 获取本机 IP 字符串（写入 out）
void fe_port_wifi_ip(char *out, size_t outlen);
// 建立 TCP 连接：host, port。返回 0 成功
int fe_port_tcp_connect(const char *host, uint16_t port);
// 发送数据，返回实际发送
size_t fe_port_tcp_write(const uint8_t *data, size_t len);
// 读取数据，返回字节数（0 = 无数据，-1 = 断开）
int fe_port_tcp_read(uint8_t *buf, size_t len);
// 断开 TCP
void fe_port_tcp_close(void);

// ============================================================
// 延时（毫秒）
// ============================================================
void fe_port_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif // FE_PORT_H