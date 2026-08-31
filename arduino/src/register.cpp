// register.cpp — 注册全部 Data / Ability 到全局 Atom（Arduino 版）
#include "fe.h"
#include "fe_ability.h"
#include "fe_data.h"

namespace fe {

// 各模块单例实例
static BaseAbility       g_baseAbility;
static RoleAbility       g_roleAbility;
static TimeAbility       g_timeAbility;
static OneKeyAbility     g_oneKeyAbility;
static ConfigFileAbility g_configFileAbility;
static SerialAbility     g_serialAbility;
static MQTTAbility       g_mqttAbility;
static ModbusAbility     g_modbusAbility;
static EdgeRoleAbility   g_edgeRoleAbility;
static RegAbility        g_regAbility;
static GpioAbility       g_gpioAbility;

static BaseData    g_baseData;
static ConfigData  g_configData;
static KeyringData g_keyringData;
static NetMapData  g_netMapData;
static ChipData    g_chipData;

void runTimeAbilityTick(uint32_t nowMs) {
    timeAbilityTick(g_timeAbility, nowMs);
}

void registerAllData(Atom &atom) {
    static const CommandEntry baseDataCmds[] = {
        {"logo", baseDataDispatch},
        {"info", baseDataDispatch},
    };
    static const CommandEntry configDataCmds[] = {
        {"get", configDataDispatch},
        {"set", configDataDispatch},
        {"delete", configDataDispatch},
        {"list", configDataDispatch},
        {"snapshot", configDataDispatch},
    };
    static const CommandEntry keyringDataCmds[] = {
        {"status", keyringDataDispatch},
        {"set_secret", keyringDataDispatch},
        {"rotate", keyringDataDispatch},
        {"list_tokens", keyringDataDispatch},
        {"issue_token", keyringDataDispatch},
        {"revoke_token", keyringDataDispatch},
        {"revoke_all", keyringDataDispatch},
    };
    static const CommandEntry netMapDataCmds[] = {
        {"info", netMapDataDispatch},
        {"set_node_name", netMapDataDispatch},
        {"interfaces", netMapDataDispatch},
        {"set_default_iface", netMapDataDispatch},
    };
    static const CommandEntry chipDataCmds[] = {
        {"info", chipDataDispatch},
    };

    atom.registerData({ "BaseData",    "框架元信息", baseDataCmds,    sizeof(baseDataCmds)/sizeof(baseDataCmds[0]),    &g_baseData,    baseDataDispatch });
    atom.registerData({ "ConfigData",  "扁平点号路径 KV 配置（NVS）", configDataCmds, sizeof(configDataCmds)/sizeof(configDataCmds[0]), &g_configData,  configDataDispatch });
    atom.registerData({ "KeyringData", "共享密钥与令牌表（NVS）", keyringDataCmds, sizeof(keyringDataCmds)/sizeof(keyringDataCmds[0]), &g_keyringData, keyringDataDispatch });
    atom.registerData({ "NetMapData",  "本节点网络信息", netMapDataCmds, sizeof(netMapDataCmds)/sizeof(netMapDataCmds[0]), &g_netMapData,  netMapDataDispatch });
    atom.registerData({ "ChipData",    "芯片信息(MCU 专有)", chipDataCmds, sizeof(chipDataCmds)/sizeof(chipDataCmds[0]), &g_chipData,    chipDataDispatch });
}

void registerAllAbilities(Atom &atom) {
    static const CommandEntry baseAbilityCmds[] = {
        {"list_data_names", baseAbilityDispatch},
        {"list_ability_names", baseAbilityDispatch},
    };
    static const CommandEntry roleAbilityCmds[] = {
        {"describe", roleAbilityDispatch},
        {"set_role", roleAbilityDispatch},
        {"get_role", roleAbilityDispatch},
    };
    static const CommandEntry timeAbilityCmds[] = {
        {"sync_net", timeAbilityDispatch},
        {"sync_manual", timeAbilityDispatch},
        {"sync_system", timeAbilityDispatch},
        {"sync_ntp", timeAbilityDispatch},
        {"get_time", timeAbilityDispatch},
        {"configure_run", timeAbilityDispatch},
    };
    static const CommandEntry oneKeyAbilityCmds[] = {
        {"issue_token", oneKeyAbilityDispatch},
        {"verify_token", oneKeyAbilityDispatch},
        {"revoke_token", oneKeyAbilityDispatch},
        {"revoke_all", oneKeyAbilityDispatch},
        {"list_tokens", oneKeyAbilityDispatch},
        {"status", oneKeyAbilityDispatch},
        {"rotate", oneKeyAbilityDispatch},
    };
    static const CommandEntry configFileAbilityCmds[] = {
        {"load", configFileAbilityDispatch},
        {"save", configFileAbilityDispatch},
        {"set_path", configFileAbilityDispatch},
        {"get_path", configFileAbilityDispatch},
        {"exists", configFileAbilityDispatch},
    };
    static const CommandEntry serialAbilityCmds[] = {
        {"open", serialAbilityDispatch},
        {"close", serialAbilityDispatch},
        {"write", serialAbilityDispatch},
        {"read", serialAbilityDispatch},
        {"is_open", serialAbilityDispatch},
        {"set_config", serialAbilityDispatch},
        {"get_config", serialAbilityDispatch},
        {"list_ports", serialAbilityDispatch},
    };
    static const CommandEntry mqttAbilityCmds[] = {
        {"set_broker", mqttAbilityDispatch},
        {"connect", mqttAbilityDispatch},
        {"disconnect", mqttAbilityDispatch},
        {"publish", mqttAbilityDispatch},
        {"subscribe", mqttAbilityDispatch},
        {"unsubscribe", mqttAbilityDispatch},
        {"is_connected", mqttAbilityDispatch},
        {"list_subscriptions", mqttAbilityDispatch},
        {"drain", mqttAbilityDispatch},
    };
    static const CommandEntry modbusAbilityCmds[] = {
        {"set_unit_id", modbusAbilityDispatch},
        {"get_unit_id", modbusAbilityDispatch},
        {"read_holding", modbusAbilityDispatch},
        {"read_input", modbusAbilityDispatch},
        {"read_coils", modbusAbilityDispatch},
        {"read_discrete", modbusAbilityDispatch},
        {"write_holding", modbusAbilityDispatch},
        {"write_coil", modbusAbilityDispatch},
    };
    static const CommandEntry edgeRoleAbilityCmds[] = {
        {"describe", edgeRoleAbilityDispatch},
        {"set_zone", edgeRoleAbilityDispatch},
        {"get_zone", edgeRoleAbilityDispatch},
        {"set_status", edgeRoleAbilityDispatch},
        {"get_status", edgeRoleAbilityDispatch},
        {"set_online", edgeRoleAbilityDispatch},
        {"record_latency", edgeRoleAbilityDispatch},
        {"get_metrics", edgeRoleAbilityDispatch},
    };
    static const CommandEntry regAbilityCmds[] = {
        {"read", regAbilityDispatch},
        {"write", regAbilityDispatch},
        {"bit_set", regAbilityDispatch},
        {"bit_clear", regAbilityDispatch},
        {"info", regAbilityDispatch},
    };
    static const CommandEntry gpioAbilityCmds[] = {
        {"mode", gpioAbilityDispatch},
        {"write", gpioAbilityDispatch},
        {"read", gpioAbilityDispatch},
        {"info", gpioAbilityDispatch},
    };

    atom.registerAbility({ "BaseAbility",       "基础",       baseAbilityCmds,       sizeof(baseAbilityCmds)/sizeof(baseAbilityCmds[0]),       &g_baseAbility,       baseAbilityDispatch });
    atom.registerAbility({ "RoleAbility",       "角色",       roleAbilityCmds,       sizeof(roleAbilityCmds)/sizeof(roleAbilityCmds[0]),       &g_roleAbility,       roleAbilityDispatch });
    atom.registerAbility({ "TimeAbility",       "时间",       timeAbilityCmds,       sizeof(timeAbilityCmds)/sizeof(timeAbilityCmds[0]),       &g_timeAbility,       timeAbilityDispatch });
    atom.registerAbility({ "OneKeyAbility",     "一键令牌",   oneKeyAbilityCmds,     sizeof(oneKeyAbilityCmds)/sizeof(oneKeyAbilityCmds[0]),     &g_oneKeyAbility,     oneKeyAbilityDispatch });
    atom.registerAbility({ "ConfigFileAbility", "配置文件",   configFileAbilityCmds, sizeof(configFileAbilityCmds)/sizeof(configFileAbilityCmds[0]), &g_configFileAbility, configFileAbilityDispatch });
    atom.registerAbility({ "SerialAbility",     "串口",       serialAbilityCmds,     sizeof(serialAbilityCmds)/sizeof(serialAbilityCmds[0]),     &g_serialAbility,     serialAbilityDispatch });
    atom.registerAbility({ "MQTTAbility",       "MQTT",       mqttAbilityCmds,       sizeof(mqttAbilityCmds)/sizeof(mqttAbilityCmds[0]),       &g_mqttAbility,       mqttAbilityDispatch });
    atom.registerAbility({ "ModbusAbility",     "Modbus",     modbusAbilityCmds,     sizeof(modbusAbilityCmds)/sizeof(modbusAbilityCmds[0]),     &g_modbusAbility,     modbusAbilityDispatch });
    atom.registerAbility({ "EdgeRoleAbility",   "边缘角色",   edgeRoleAbilityCmds,   sizeof(edgeRoleAbilityCmds)/sizeof(edgeRoleAbilityCmds[0]),   &g_edgeRoleAbility,   edgeRoleAbilityDispatch });
    atom.registerAbility({ "RegAbility",         "寄存器操作(专有)", regAbilityCmds,   sizeof(regAbilityCmds)/sizeof(regAbilityCmds[0]),           &g_regAbility,        regAbilityDispatch });
    atom.registerAbility({ "GpioAbility",        "GPIO 控制(专有)", gpioAbilityCmds,  sizeof(gpioAbilityCmds)/sizeof(gpioAbilityCmds[0]),         &g_gpioAbility,       gpioAbilityDispatch });
}

void initAll() {
    Atom &atom = globalAtom();
    registerAllData(atom);
    registerAllAbilities(atom);
}

} // namespace fe