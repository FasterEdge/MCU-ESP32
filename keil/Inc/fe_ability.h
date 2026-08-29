// fe_ability.h — FasterEdge MCU Ability 模块声明（Keil/裸机 C 版）
// 与 Arduino 版能力子集一致：Base / Role / Time / OneKey /
// ConfigFile / Serial / MQTT / Modbus / EdgeRole
#ifndef FE_ABILITY_H
#define FE_ABILITY_H

#include "fe.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// BaseAbility —— list_data_names / list_ability_names
// ============================================================
fe_output_t ability_base_dispatch(void *inst, const char *act, const char *args);

// ============================================================
// RoleAbility —— describe / set_role / get_role
// ============================================================
typedef struct {
    char role[16];      // edge / cloud / standalone
} role_ability_t;
fe_output_t ability_role_dispatch(void *inst, const char *act, const char *args);

// ============================================================
// TimeAbility —— sync_net / sync_manual / sync_system /
//                sync_ntp / get_time / configure_run
// ============================================================
typedef struct {
    uint64_t manual_epoch;
} time_ability_t;
fe_output_t ability_time_dispatch(void *inst, const char *act, const char *args);

// ============================================================
// OneKeyAbility —— issue_token / verify_token / revoke_token /
//                  revoke_all / list_tokens / status / rotate
// （HMAC-SHA256，密钥存于 fe_port NVS）
// ============================================================
typedef struct {
    char secret[33];    // HMAC 密钥（NVS 持久化）
    uint32_t seq;       // 令牌序列
} onekey_ability_t;
fe_output_t ability_onekey_dispatch(void *inst, const char *act, const char *args);

// ============================================================
// ConfigFileAbility —— load / save / set_path / get_path / exists
// ============================================================
typedef struct {
    char path[32];      // 配置命名空间
} configfile_ability_t;
fe_output_t ability_configfile_dispatch(void *inst, const char *act, const char *args);

// ============================================================
// SerialAbility —— open / close / write / read / is_open /
//                  set_config / get_config / list_ports
// ============================================================
typedef struct {
    bool open;
    uint32_t baud;
    uint8_t port;
} serial_ability_t;
fe_output_t ability_serial_dispatch(void *inst, const char *act, const char *args);

// ============================================================
// MQTTAbility —— set_broker / connect / disconnect / publish /
//                subscribe / unsubscribe / is_connected /
//                list_subscriptions / drain
// （TCP 层由 fe_port 提供，协议可自行接入 paho/自定义客户端）
// ============================================================
typedef struct {
    char broker[128];
    char client_id[48];
    char subs[4][64];   // 订阅主题表（简化固定 4 条）
    uint8_t sub_count;
    bool connected;
} mqtt_ability_t;
fe_output_t ability_mqtt_dispatch(void *inst, const char *act, const char *args);

// ============================================================
// ModbusAbility —— set_unit_id / read_holding / read_input /
//                  read_coils / read_discrete / write_holding / write_coil
// ============================================================
typedef struct {
    uint8_t unit_id;
    uint16_t holding_regs[64];
    uint16_t input_regs[64];
    bool     coils[64];
    bool     discrete_inputs[64];
} modbus_ability_t;
fe_output_t ability_modbus_dispatch(void *inst, const char *act, const char *args);

// ============================================================
// EdgeRoleAbility —— describe / set_zone / set_status / set_online /
//                    record_latency / get_metrics
// ============================================================
typedef struct {
    char zone[32];
    char status[16];    // healthy / degraded / offline
    bool online;
    int32_t latency_ms;
} edgerole_ability_t;
fe_output_t ability_edgerole_dispatch(void *inst, const char *act, const char *args);

// ============================================================
// RegAbility —— MCU 专有·寄存器操作：read / write / bit_set /
//               bit_clear / info
// ============================================================
fe_output_t ability_reg_dispatch(void *inst, const char *act, const char *args);

// ============================================================
// GpioAbility —— MCU 专有·GPIO 控制：mode / write / read / info
// ============================================================
fe_output_t ability_gpio_dispatch(void *inst, const char *act, const char *args);

// ============================================================
// 注册全部 Ability（register.c 调用）
// ============================================================
void fe_register_all_abilities(fe_atom_t *atom);

#ifdef __cplusplus
}
#endif

#endif // FE_ABILITY_H