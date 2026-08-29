// register.c — 注册全部 Data / Ability 到全局 Atom（Keil/裸机 C 版）
#include "fe.h"
#include "fe_ability.h"
#include "fe_data.h"

// ============================================================
// 模块实例
// ============================================================
static role_ability_t       g_role;
static time_ability_t       g_time;
static onekey_ability_t     g_onekey;
static configfile_ability_t g_configfile;
static serial_ability_t     g_serial = { .open = false, .baud = 115200, .port = 0 };
static mqtt_ability_t       g_mqtt;
static modbus_ability_t     g_modbus;
static edgerole_ability_t   g_edgerole;

static config_data_t  g_config_data  = { .ns = "fe_cfg" };
static keyring_data_t g_keyring_data = { .ns = "fe_key" };
static netmap_data_t  g_netmap_data  = { .node_name = "esp32", .default_iface = "wlan0" };

// ============================================================
// 注册 Data
// ============================================================
void fe_register_all_data(fe_atom_t *atom) {
    static const fe_cmd_t baseCmds[] = {
        {"logo", data_base_dispatch},
        {"info", data_base_dispatch},
    };
    static const fe_cmd_t configCmds[] = {
        {"get", data_config_dispatch},
        {"set", data_config_dispatch},
        {"delete", data_config_dispatch},
        {"list", data_config_dispatch},
        {"snapshot", data_config_dispatch},
    };
    static const fe_cmd_t keyringCmds[] = {
        {"status", data_keyring_dispatch},
        {"set_secret", data_keyring_dispatch},
        {"rotate", data_keyring_dispatch},
        {"list_tokens", data_keyring_dispatch},
        {"issue_token", data_keyring_dispatch},
        {"revoke_token", data_keyring_dispatch},
        {"revoke_all", data_keyring_dispatch},
    };
    static const fe_cmd_t netmapCmds[] = {
        {"info", data_netmap_dispatch},
        {"set_node_name", data_netmap_dispatch},
        {"interfaces", data_netmap_dispatch},
        {"set_default_iface", data_netmap_dispatch},
    };

    fe_register_data(atom, &(fe_module_t){"BaseData",   "框架元信息", baseCmds,   sizeof(baseCmds)/sizeof(baseCmds[0]),   NULL,               data_base_dispatch});
    fe_register_data(atom, &(fe_module_t){"ConfigData", "KV 配置(NVS)", configCmds, sizeof(configCmds)/sizeof(configCmds[0]), &g_config_data,  data_config_dispatch});
    fe_register_data(atom, &(fe_module_t){"KeyringData","密钥令牌表(NVS)", keyringCmds, sizeof(keyringCmds)/sizeof(keyringCmds[0]), &g_keyring_data, data_keyring_dispatch});
    fe_register_data(atom, &(fe_module_t){"NetMapData", "本节点网络信息", netmapCmds, sizeof(netmapCmds)/sizeof(netmapCmds[0]), &g_netmap_data,  data_netmap_dispatch});
}

// ============================================================
// 注册 Ability
// ============================================================
void fe_register_all_abilities(fe_atom_t *atom) {
    static const fe_cmd_t baseCmds[] = {
        {"list_data_names", ability_base_dispatch},
        {"list_ability_names", ability_base_dispatch},
    };
    static const fe_cmd_t roleCmds[] = {
        {"describe", ability_role_dispatch},
        {"set_role", ability_role_dispatch},
        {"get_role", ability_role_dispatch},
    };
    static const fe_cmd_t timeCmds[] = {
        {"sync_net", ability_time_dispatch},
        {"sync_manual", ability_time_dispatch},
        {"sync_system", ability_time_dispatch},
        {"sync_ntp", ability_time_dispatch},
        {"get_time", ability_time_dispatch},
        {"configure_run", ability_time_dispatch},
    };
    static const fe_cmd_t onekeyCmds[] = {
        {"issue_token", ability_onekey_dispatch},
        {"verify_token", ability_onekey_dispatch},
        {"revoke_token", ability_onekey_dispatch},
        {"revoke_all", ability_onekey_dispatch},
        {"list_tokens", ability_onekey_dispatch},
        {"status", ability_onekey_dispatch},
        {"rotate", ability_onekey_dispatch},
    };
    static const fe_cmd_t configfileCmds[] = {
        {"load", ability_configfile_dispatch},
        {"save", ability_configfile_dispatch},
        {"set_path", ability_configfile_dispatch},
        {"get_path", ability_configfile_dispatch},
        {"exists", ability_configfile_dispatch},
    };
    static const fe_cmd_t serialCmds[] = {
        {"open", ability_serial_dispatch},
        {"close", ability_serial_dispatch},
        {"write", ability_serial_dispatch},
        {"read", ability_serial_dispatch},
        {"is_open", ability_serial_dispatch},
        {"set_config", ability_serial_dispatch},
        {"get_config", ability_serial_dispatch},
        {"list_ports", ability_serial_dispatch},
    };
    static const fe_cmd_t mqttCmds[] = {
        {"set_broker", ability_mqtt_dispatch},
        {"connect", ability_mqtt_dispatch},
        {"disconnect", ability_mqtt_dispatch},
        {"publish", ability_mqtt_dispatch},
        {"subscribe", ability_mqtt_dispatch},
        {"unsubscribe", ability_mqtt_dispatch},
        {"is_connected", ability_mqtt_dispatch},
        {"list_subscriptions", ability_mqtt_dispatch},
        {"drain", ability_mqtt_dispatch},
    };
    static const fe_cmd_t modbusCmds[] = {
        {"set_unit_id", ability_modbus_dispatch},
        {"get_unit_id", ability_modbus_dispatch},
        {"read_holding", ability_modbus_dispatch},
        {"read_input", ability_modbus_dispatch},
        {"read_coils", ability_modbus_dispatch},
        {"read_discrete", ability_modbus_dispatch},
        {"write_holding", ability_modbus_dispatch},
        {"write_coil", ability_modbus_dispatch},
    };
    static const fe_cmd_t edgeroleCmds[] = {
        {"describe", ability_edgerole_dispatch},
        {"set_zone", ability_edgerole_dispatch},
        {"get_zone", ability_edgerole_dispatch},
        {"set_status", ability_edgerole_dispatch},
        {"get_status", ability_edgerole_dispatch},
        {"set_online", ability_edgerole_dispatch},
        {"record_latency", ability_edgerole_dispatch},
        {"get_metrics", ability_edgerole_dispatch},
    };

    fe_register_ability(atom, &(fe_module_t){"BaseAbility",       "基础",     baseCmds,       sizeof(baseCmds)/sizeof(baseCmds[0]),       NULL,            ability_base_dispatch});
    fe_register_ability(atom, &(fe_module_t){"RoleAbility",       "角色",     roleCmds,       sizeof(roleCmds)/sizeof(roleCmds[0]),       &g_role,         ability_role_dispatch});
    fe_register_ability(atom, &(fe_module_t){"TimeAbility",       "时间",     timeCmds,       sizeof(timeCmds)/sizeof(timeCmds[0]),       &g_time,         ability_time_dispatch});
    fe_register_ability(atom, &(fe_module_t){"OneKeyAbility",     "一键令牌", onekeyCmds,     sizeof(onekeyCmds)/sizeof(onekeyCmds[0]),     &g_onekey,       ability_onekey_dispatch});
    fe_register_ability(atom, &(fe_module_t){"ConfigFileAbility", "配置文件", configfileCmds, sizeof(configfileCmds)/sizeof(configfileCmds[0]), &g_configfile,  ability_configfile_dispatch});
    fe_register_ability(atom, &(fe_module_t){"SerialAbility",     "串口",     serialCmds,     sizeof(serialCmds)/sizeof(serialCmds[0]),     &g_serial,       ability_serial_dispatch});
    fe_register_ability(atom, &(fe_module_t){"MQTTAbility",       "MQTT",     mqttCmds,       sizeof(mqttCmds)/sizeof(mqttCmds[0]),       &g_mqtt,         ability_mqtt_dispatch});
    fe_register_ability(atom, &(fe_module_t){"ModbusAbility",     "Modbus",   modbusCmds,     sizeof(modbusCmds)/sizeof(modbusCmds[0]),     &g_modbus,       ability_modbus_dispatch});
    fe_register_ability(atom, &(fe_module_t){"EdgeRoleAbility",   "边缘角色", edgeroleCmds,   sizeof(edgeroleCmds)/sizeof(edgeroleCmds[0]), &g_edgerole,     ability_edgerole_dispatch});
}

// ============================================================
// 初始化全部
// ============================================================
void fe_init_all(void) {
    fe_atom_t *atom = fe_global_atom();
    fe_register_all_data(atom);
    fe_register_all_abilities(atom);
}