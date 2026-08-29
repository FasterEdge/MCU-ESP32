// fe.h — FasterEdge MCU 核心框架（Keil/裸机 C 版）
// 纯 C 实现，不依赖 Arduino/RTOS；平台相关操作通过 fe_port.h 抽象，
// 可在任意 Cortex-M / Xtensa 工具链（Keil MDK / GCC）下编译。
// 对应 FasterEdge 主仓库的 Atom / Ability / Data / Command 模型
#ifndef FE_H
#define FE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================
// 基础类型
// ============================================================

// 命令输出（对应 FasterEdge 的 CommandOutput）
typedef struct {
    char name[32];      // 命令名
    char value[256];    // 返回值（文本 / JSON）
    char err[128];      // 错误信息（空 = 成功）
    bool ok;
} fe_output_t;

// 命令执行回调：inst 为各模块实例，act 命令名，args 参数字符串
typedef fe_output_t (*fe_cmd_handler_t)(void *inst, const char *act, const char *args);

// 命令表项
typedef struct {
    const char *name;
    fe_cmd_handler_t handler;
} fe_cmd_t;

// 模块（Data / Ability 通用描述）
typedef struct {
    const char *name;       // 如 "BaseData" / "BaseAbility"
    const char *desc;
    const fe_cmd_t *cmds;
    size_t cmd_count;
    void *instance;
    fe_cmd_handler_t dispatch;
} fe_module_t;

// ============================================================
// Atom：注册表 + 命令路由
// ============================================================
#define FE_MAX_MODULES 16

typedef struct {
    fe_module_t data[FE_MAX_MODULES];
    size_t data_count;
    fe_module_t ability[FE_MAX_MODULES];
    size_t ability_count;
} fe_atom_t;

// 注册
void fe_register_data(fe_atom_t *atom, const fe_module_t *mod);
void fe_register_ability(fe_atom_t *atom, const fe_module_t *mod);

// 查询名称列表（写入逗号分隔字符串）
void fe_list_data_names(const fe_atom_t *atom, char *out, size_t outlen);
void fe_list_ability_names(const fe_atom_t *atom, char *out, size_t outlen);

// 执行命令：target 形如 "data_BaseData" / "ability_BaseAbility"
fe_output_t fe_execute(fe_atom_t *atom, const char *target, const char *act, const char *args);

// 构建输出工具
fe_output_t fe_ok(const char *name, const char *value);
fe_output_t fe_err(const char *name, const char *err);

// ============================================================
// 全局 Atom + 初始化（由 register.c 实现）
// ============================================================
fe_atom_t *fe_global_atom(void);
void fe_init_all(void);

#ifdef __cplusplus
}
#endif

#endif // FE_H