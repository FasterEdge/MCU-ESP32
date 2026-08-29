// fe_data.h — FasterEdge MCU Data 模块声明（Arduino 版）
// 对应 FasterEdge 主仓库 data/ 下在 ESP32 上合理的子集
#pragma once

#include "fe.h"

namespace fe {

// ============================================================
// BaseData —— 框架元信息：logo / info
// ============================================================
struct BaseData {
    CommandOutput dispatch(const char *act, const String &args);
};
extern CommandOutput baseDataDispatch(void *inst, const char *act, const String &args);

// ============================================================
// ConfigData —— 扁平点号路径 KV 配置（NVS 持久化）：
//               get / set / delete / list / snapshot
// ============================================================
struct ConfigData {
    String ns;  // NVS 命名空间
    ConfigData() : ns("fe_cfg") {}
    CommandOutput dispatch(const char *act, const String &args);
};
extern CommandOutput configDataDispatch(void *inst, const char *act, const String &args);

// ============================================================
// KeyringData —— 共享密钥与令牌表（NVS 持久化）：
//                status / set_secret / rotate / list_tokens /
//                issue_token / revoke_token / revoke_all
// ============================================================
struct KeyringData {
    String ns;  // NVS 命名空间
    KeyringData() : ns("fe_key") {}
    CommandOutput dispatch(const char *act, const String &args);
};
extern CommandOutput keyringDataDispatch(void *inst, const char *act, const String &args);

// ============================================================
// NetMapData —— 本节点网络信息：
//               info / set_node_name / interfaces / set_default_iface
// ============================================================
struct NetMapData {
    String nodeName;
    String defaultIface;
    NetMapData() : nodeName("esp32"), defaultIface("wlan0") {}
    CommandOutput dispatch(const char *act, const String &args);
};
extern CommandOutput netMapDataDispatch(void *inst, const char *act, const String &args);

// ============================================================
// 注册全部 Data
// ============================================================
void registerAllData(Atom &atom);

} // namespace fe