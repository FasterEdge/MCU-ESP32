// fe_data.h — FasterEdge MCU Data 模块声明（Keil/裸机 C 版）
// Base / Config / Keyring / NetMap
#ifndef FE_DATA_H
#define FE_DATA_H

#include "fe.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// BaseData —— logo / info
// ============================================================
fe_output_t data_base_dispatch(void *inst, const char *act, const char *args);

// ============================================================
// ConfigData —— 扁平点号路径 KV 配置（NVS）：get/set/delete/list/snapshot
// ============================================================
typedef struct {
    char ns[16];    // NVS 命名空间
} config_data_t;
fe_output_t data_config_dispatch(void *inst, const char *act, const char *args);

// ============================================================
// KeyringData —— 密钥与令牌表：status/set_secret/rotate/list_tokens/
//                issue_token/revoke_token/revoke_all
// ============================================================
typedef struct {
    char ns[16];
} keyring_data_t;
fe_output_t data_keyring_dispatch(void *inst, const char *act, const char *args);

// ============================================================
// NetMapData —— 本节点网络信息：info/set_node_name/interfaces/
//               set_default_iface
// ============================================================
typedef struct {
    char node_name[32];
    char default_iface[16];
} netmap_data_t;
fe_output_t data_netmap_dispatch(void *inst, const char *act, const char *args);

// ============================================================
// ChipData —— MCU 专有·芯片信息：info
// ============================================================
fe_output_t data_chip_dispatch(void *inst, const char *act, const char *args);

// ============================================================
// 注册全部 Data（register.c 调用）
// ============================================================
void fe_register_all_data(fe_atom_t *atom);

#ifdef __cplusplus
}
#endif

#endif // FE_DATA_H