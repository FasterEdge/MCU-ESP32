<div align="center">
<img src="https://avatars.githubusercontent.com/u/245985800?s=200&v=4" style="width:100px;" width="100"/>
<h2>FasterEdge MCU - ESP32</h2>
<h3>FasterEdge 框架的 ESP32 平台实现（Arduino / Keil 双版本）</h3>
</div>

### 一、简介

本项目是 **[FasterEdge](https://github.com/FasterEdge/FasterEdge)** 框架在 **ESP32** 单片机平台上的实现，将主仓库的 **Ability / Data / Command** 模型移植到资源受限的 MCU 环境。从主仓库约 20 个 Ability 与 6 个 Data 中，筛选出 ESP32 上**真正合理**的子集进行实现，其余因依赖 shell 进程 / 容器 / 大型数据库 / 硬件 TSN 等而不适用 MCU。

- ✅ **双版本**：`arduino/`（PlatformIO/Arduino C++）与 `keil/`（Keil MDK 裸机 C）
- ✅ 与主仓库**同名同命令**，方便云边协同对等编程
- ✅ 关键能力零依赖实现（HMAC-SHA256 纯 C 移植，不依赖 mbedTLS）
- ✅ 平台差异全部收敛到移植层（`fe_port.h`），核心逻辑与硬件解耦

### 二、已实现能力（ESP32 合理子集）

**Ability（9 个）**

| 名称 | 类别 | 命令 |
|------|------|------|
| `BaseAbility` | 基础 | `list_data_names` / `list_ability_names` |
| `RoleAbility` | 角色 | `describe` / `set_role` / `get_role` |
| `TimeAbility` | 时间 | `sync_net` / `sync_manual` / `sync_system` / `sync_ntp` / `get_time` / `configure_run` |
| `OneKeyAbility` | 令牌 | `issue_token` / `verify_token` / `revoke_token` / `revoke_all` / `list_tokens` / `status` / `rotate`（HMAC-SHA256）|
| `ConfigFileAbility` | 配置 | `load` / `save` / `set_path` / `get_path` / `exists` |
| `SerialAbility` | 串口 | `open` / `close` / `write` / `read` / `is_open` / `set_config` / `get_config` / `list_ports` |
| `MQTTAbility` | MQTT | `set_broker` / `connect` / `disconnect` / `publish` / `subscribe` / `unsubscribe` / `is_connected` / `list_subscriptions` / `drain` |
| `ModbusAbility` | Modbus | `set_unit_id` / `get_unit_id` / `read_holding` / `read_input` / `read_coils` / `read_discrete` / `write_holding` / `write_coil` |
| `EdgeRoleAbility` | 边缘角色 | `describe` / `set_zone` / `get_zone` / `set_status` / `get_status` / `set_online` / `record_latency` / `get_metrics` |

**Data（4 个）**

| 名称 | 功能 | 命令 |
|------|------|------|
| `BaseData` | 框架元信息 | `logo` / `info` |
| `ConfigData` | KV 配置（NVS 持久化）| `get` / `set` / `delete` / `list` / `snapshot` |
| `KeyringData` | 密钥与令牌表（NVS）| `status` / `set_secret` / `rotate` / `list_tokens` / `issue_token` / `revoke_token` / `revoke_all` |
| `NetMapData` | 本节点网络信息 | `info` / `set_node_name` / `interfaces` / `set_default_iface` |

### 三、排除项与理由

| 主仓库能力 | 排除原因 |
|-----------|---------|
| CmdAbility / ShAbility / BashAbility | 依赖 shell 进程与作业调度，MCU 无操作系统进程概念 |
| DockerAbility / KubernetesAbility | 依赖容器运行时，MCU 资源不足以承载 |
| EKuiperAbility / InfluxDBAbility | 依赖大型流式计算/数据库后端 |
| FileTransferAbility / AlgorithmDistributionAbility | 依赖完整文件系统与网络协议栈 |
| TSNAbility | ESP32 无硬件 TSN（802.1Qbv）支持 |
| 完整 NetMapAbility 拓扑管理 | 简化为本节点信息（NetMapData），多节点拓扑由云端聚合 |

### 四、目录结构

```
MCU-ESP32/
├── arduino/                    # PlatformIO / Arduino C++ 版
│   ├── platformio.ini          # ESP32 构建配置（PubSubClient 依赖）
│   ├── include/
│   │   ├── fe.h                # 核心框架（Atom/Ability/Data/Command）
│   │   ├── fe_ability.h        # Ability 声明
│   │   ├── fe_data.h           # Data 声明
│   │   └── fe_hmac_sha256.h    # HMAC-SHA256（纯 C）
│   └── src/
│       ├── main.cpp            # 串口命令解释器
│       ├── fe.cpp              # 框架实现
│       ├── register.cpp        # 注册全部模块
│       ├── fe_hmac_sha256.c
│       ├── ability_*.cpp       # 9 个 Ability
│       └── data_*.cpp          # 4 个 Data
└── keil/                       # Keil MDK 裸机 C 版
    ├── MDK-ARM/
    │   └── FasterEdge-MCU-ESP32.uvprojx   # Keil 工程
    ├── Core/                   # fe.h / fe.c / fe_hmac_sha256.c
    ├── Inc/                    # fe_ability.h / fe_data.h / fe_port.h
    ├── Ability/                # ability_*.c（9 个）
    ├── Data/                   # data_*.c（4 个）
    └── User/                   # main.c / register.c / fe_port.c
```

### 五、Arduino 版使用

```bash
cd arduino
# 安装 PlatformIO（若未安装）：pip install platformio
pio run            # 编译
pio upload         # 烧录到 ESP32
pio device monitor # 打开串口监视器 (115200)
```

**串口命令示例：**

```
help
ability_BaseAbility list_ability_names
ability_RoleAbility set_role edge
ability_TimeAbility sync_ntp
ability_OneKeyAbility issue_token sensor01
ability_SerialAbility open 0
ability_SerialAbility write hello
ability_ModbusAbility write_holding 0,42
ability_MQTTAbility set_broker 192.168.1.10:1883
ability_MQTTAbility publish sensors/temp,25.5
data_ConfigData set wifi.ssid=MyNet
data_KeyringData issue_token sensor01
data_NetMapData info
```

> MQTT 使用 [PubSubClient](https://github.com/knolleary/pubsubclient)（已在 platformio.ini 声明）。WiFi 连接需在 `main.cpp` 的 `setup()` 中取消注释并填入 SSID/密码。

### 六、Keil 版使用

1. 用 Keil MDK 打开 `keil/MDK-ARM/FasterEdge-MCU-ESP32.uvprojx`
2. 在 `User/fe_port.c` 中按注释完成平台移植（UART / NVS / 时间 / 随机数 / TCP）
3. 编译烧录，通过串口（115200）输入同样格式的命令

> 说明：ESP32 为 Xtensa 内核，Keil MDK 主要面向 ARM Cortex-M。若在 Keil 中编译，可将 `fe_port.c` 的 TODO 替换为 ESP-IDF API（文件末尾附完整参考片段），或在其他 Cortex-M MCU 上直接复用本框架。

### 七、与 FasterEdge 主仓库的对应关系

- 命令名与主仓库 **完全一致**（如 `issue_token`、`sync_ntp`、`write_holding`）
- `Atom` 模型简化：主仓库的多实例 Atom → MCU 单例全局 Atom
- 严格类型校验保留：非法参数返回 `invalid arguments` 类错误
- 令牌/密钥默认持久化到 NVS，重启不丢失
