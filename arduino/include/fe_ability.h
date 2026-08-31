// fe_ability.h — FasterEdge MCU Ability 模块声明（Arduino 版）
// 对应 FasterEdge 主仓库 ability/ 下在 ESP32 上合理的能力子集
#pragma once

#include "fe.h"

namespace fe {

// ============================================================
// BaseAbility —— 基础：list_data_names / list_ability_names
// ============================================================
struct BaseAbility {
    CommandOutput dispatch(const char *act, const String &args);
};
extern CommandOutput baseAbilityDispatch(void *inst, const char *act, const String &args);

// ============================================================
// RoleAbility —— 角色：describe / set_role / get_role
// ============================================================
struct RoleAbility {
    String role;    // 本机角色
    RoleAbility() : role("edge") {}
    CommandOutput dispatch(const char *act, const String &args);
};
extern CommandOutput roleAbilityDispatch(void *inst, const char *act, const String &args);

// ============================================================
// TimeAbility —— 时间：sync_net / sync_manual / sync_system /
//               sync_ntp / get_time / configure_run
// ============================================================
struct TimeAbility {
    time_t manualEpoch;       // 手动设定的时间（epoch 秒）
    bool runEnabled;          // 周期 NTP 是否启用
    bool runStateLoaded;      // 持久化配置是否已加载
    uint32_t runIntervalSec;  // 周期间隔（秒）
    uint32_t nextRunMs;       // millis() 调度时刻
    String ntpServer;
    TimeAbility() : manualEpoch(0), runEnabled(false), runStateLoaded(false),
                    runIntervalSec(3600), nextRunMs(0), ntpServer("pool.ntp.org") {}
    CommandOutput dispatch(const char *act, const String &args);
};
extern CommandOutput timeAbilityDispatch(void *inst, const char *act, const String &args);
extern void timeAbilityTick(TimeAbility &self, uint32_t nowMs);
extern void runTimeAbilityTick(uint32_t nowMs);

// ============================================================
// OneKeyAbility —— 一键令牌：issue_token / verify_token /
//                 revoke_token / revoke_all / list_tokens / status / rotate
// （HMAC-SHA256；密钥保存在 NVS，通过 Preferences 实现）
// ============================================================
struct OneKeyAbility {
    String secret;  // HMAC 密钥（默认从 NVS 加载）
    size_t tokenSeq;
    OneKeyAbility() : tokenSeq(0) {}
    CommandOutput dispatch(const char *act, const String &args);
};
extern CommandOutput oneKeyAbilityDispatch(void *inst, const char *act, const String &args);

// ============================================================
// ConfigFileAbility —— 配置文件(NVS)：load / save / set_path /
//                     get_path / exists
// ============================================================
struct ConfigFileAbility {
    String path;    // 当前配置命名空间
    ConfigFileAbility() : path("fe_config") {}
    CommandOutput dispatch(const char *act, const String &args);
};
extern CommandOutput configFileAbilityDispatch(void *inst, const char *act, const String &args);

// ============================================================
// SerialAbility —— 串口(UART)：open / close / write / read /
//                 is_open / set_config / get_config / list_ports
// ============================================================
struct SerialAbility {
    bool open;
    long baud;
    SerialAbility() : open(false), baud(115200) {}
    CommandOutput dispatch(const char *act, const String &args);
};
extern CommandOutput serialAbilityDispatch(void *inst, const char *act, const String &args);

// ============================================================
// MQTTAbility —— MQTT：set_broker / connect / disconnect /
//               publish / subscribe / unsubscribe / is_connected /
//               list_subscriptions / drain
// ============================================================
struct MQTTAbility {
    String broker;
    String clientId;
    std::vector<String> subscriptions;
    bool connected;
    MQTTAbility() : connected(false) {}
    CommandOutput dispatch(const char *act, const String &args);
};
extern CommandOutput mqttAbilityDispatch(void *inst, const char *act, const String &args);

// ============================================================
// ModbusAbility —— Modbus RTU：set_unit_id / read_holding /
//                 read_input / read_coils / read_discrete /
//                 write_holding / write_coil
// ============================================================
struct ModbusAbility {
    uint8_t unitId;
    std::vector<uint16_t> holdingRegs;   // 保持寄存器区
    std::vector<uint16_t> inputRegs;     // 输入寄存器区
    std::vector<bool>     coils;         // 线圈区
    std::vector<bool>     discreteInputs;// 离散输入区
    ModbusAbility();
    CommandOutput dispatch(const char *act, const String &args);
};
extern CommandOutput modbusAbilityDispatch(void *inst, const char *act, const String &args);

// ============================================================
// EdgeRoleAbility —— 边缘角色(简化)：describe / set_zone /
//                   set_status / set_online / record_latency / get_metrics
// ============================================================
struct EdgeRoleAbility {
    String zone;
    String status;
    bool online;
    long lastLatencyMs;
    EdgeRoleAbility() : zone("default"), status("unknown"), online(false), lastLatencyMs(0) {}
    CommandOutput dispatch(const char *act, const String &args);
};
extern CommandOutput edgeRoleAbilityDispatch(void *inst, const char *act, const String &args);

// ============================================================
// RegAbility —— MCU 专有·寄存器操作：read / write / bit_set /
//              bit_clear / info（直接访问内存映射外设寄存器）
// ============================================================
struct RegAbility {
    CommandOutput dispatch(const char *act, const String &args);
};
extern CommandOutput regAbilityDispatch(void *inst, const char *act, const String &args);

// ============================================================
// GpioAbility —— MCU 专有·GPIO 控制：mode / write / read / info
// ============================================================
struct GpioAbility {
    CommandOutput dispatch(const char *act, const String &args);
};
extern CommandOutput gpioAbilityDispatch(void *inst, const char *act, const String &args);

// ============================================================
// 注册全部 Ability
// ============================================================
void registerAllAbilities(Atom &atom);

} // namespace fe