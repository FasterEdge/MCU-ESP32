<div align="center">
<img src="https://avatars.githubusercontent.com/u/245985800?s=200&v=4" style="width:100px;" width="100"/>
<h2>FasterEdge MCU - ESP32</h2>
<h3>ESP32 platform implementation of the FasterEdge framework (Arduino / Keil)</h3>
</div>

### 1. Introduction

This project is the **ESP32** MCU implementation of the **[FasterEdge](https://github.com/FasterEdge/FasterEdge)** framework, porting the **Ability / Data / Command** model to resource-constrained microcontrollers. From ~20 Abilities and 6 Data in the main repo, we keep only the subset that is genuinely **reasonable on ESP32**; the rest depend on shell processes / containers / large databases / hardware TSN and are not applicable to MCUs.

- ✅ **Two versions**: `arduino/` (PlatformIO/Arduino C++) and `keil/` (Keil MDK bare-metal C)
- ✅ **Same names and commands** as the main repo, for symmetric cloud-edge programming
- ✅ Key crypto implemented with zero dependencies (pure-C HMAC-SHA256, no mbedTLS)
- ✅ All platform differences isolated in a port layer (`fe_port.h`); core logic is hardware-agnostic

### 2. Implemented Capabilities (ESP32-reasonable subset)

**Ability (9)**

| Name | Category | Commands |
|------|----------|----------|
| `BaseAbility` | Base | `list_data_names` / `list_ability_names` |
| `RoleAbility` | Role | `describe` / `set_role` / `get_role` |
| `TimeAbility` | Time | `sync_net` / `sync_manual` / `sync_system` / `sync_ntp` / `get_time` / `configure_run` |
| `OneKeyAbility` | Token | `issue_token` / `verify_token` / `revoke_token` / `revoke_all` / `list_tokens` / `status` / `rotate` (HMAC-SHA256) |
| `ConfigFileAbility` | Config | `load` / `save` / `set_path` / `get_path` / `exists` |
| `SerialAbility` | UART | `open` / `close` / `write` / `read` / `is_open` / `set_config` / `get_config` / `list_ports` |
| `MQTTAbility` | MQTT | `set_broker` / `connect` / `disconnect` / `publish` / `subscribe` / `unsubscribe` / `is_connected` / `list_subscriptions` / `drain` |
| `ModbusAbility` | Modbus | `set_unit_id` / `get_unit_id` / `read_holding` / `read_input` / `read_coils` / `read_discrete` / `write_holding` / `write_coil` |
| `EdgeRoleAbility` | Edge role | `describe` / `set_zone` / `get_zone` / `set_status` / `get_status` / `set_online` / `record_latency` / `get_metrics` |

**Data (4)**

| Name | Function | Commands |
|------|----------|----------|
| `BaseData` | Framework metadata | `logo` / `info` |
| `ConfigData` | KV config (NVS) | `get` / `set` / `delete` / `list` / `snapshot` |
| `KeyringData` | Secrets & tokens (NVS) | `status` / `set_secret` / `rotate` / `list_tokens` / `issue_token` / `revoke_token` / `revoke_all` |
| `NetMapData` | Local network info | `info` / `set_node_name` / `interfaces` / `set_default_iface` |

### 3. Excluded Capabilities & Rationale

| Main-repo capability | Reason for exclusion |
|----------------------|----------------------|
| CmdAbility / ShAbility / BashAbility | Depend on shell processes and job scheduling; MCUs have no OS processes |
| DockerAbility / KubernetesAbility | Depend on container runtime; not feasible on MCU |
| EKuiperAbility / InfluxDBAbility | Depend on heavy streaming / database backends |
| FileTransferAbility / AlgorithmDistributionAbility | Depend on full filesystem and network stack |
| TSNAbility | ESP32 has no hardware TSN (802.1Qbv) support |
| Full NetMapAbility topology | Simplified to local node info (NetMapData); topology aggregated in cloud |

### 4. Directory Layout

```
MCU-ESP32/
├── arduino/                    # PlatformIO / Arduino C++ version
│   ├── platformio.ini          # ESP32 build config (PubSubClient dep)
│   ├── include/                # fe.h / fe_ability.h / fe_data.h / fe_hmac_sha256.h
│   └── src/                    # main.cpp / fe.cpp / register.cpp / ability_*.cpp / data_*.cpp
└── keil/                       # Keil MDK bare-metal C version
    ├── MDK-ARM/                # FasterEdge-MCU-ESP32.uvprojx
    ├── Core/                   # fe.h / fe.c / fe_hmac_sha256.c
    ├── Inc/                    # fe_ability.h / fe_data.h / fe_port.h
    ├── Ability/                # ability_*.c (9)
    ├── Data/                   # data_*.c (4)
    └── User/                   # main.c / register.c / fe_port.c

platformio_ide/                # VS Code + PlatformIO plugin project (ESP-IDF framework)
    ├── platformio.ini          # espressif32 / espidf / esp32dev
    ├── .vscode/extensions.json # recommends PlatformIO IDE
    ├── include/                # fe.h / fe_ability.h / fe_data.h / fe_port.h / fe_hmac_sha256.h
    └── src/                    # reuses keil bare-metal C + real ESP-IDF fe_port
```

> Three build routes, three toolchains: `arduino/` (Arduino C++), `keil/` (Keil MDK), `platformio_ide/` (VS Code PlatformIO plugin + ESP-IDF); same commands.

### 5. Arduino Version

```bash
cd arduino
pio run            # build
pio upload         # flash ESP32
pio device monitor # open serial monitor (115200)
```

**Serial command examples:**

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

> MQTT uses [PubSubClient](https://github.com/knolleary/pubsubclient) (declared in platformio.ini). To enable WiFi, uncomment and fill credentials in `main.cpp` `setup()`.

### 6. Keil Version

1. Open `keil/MDK-ARM/FasterEdge-MCU-ESP32.uvprojx` with Keil MDK
2. Implement the platform port in `User/fe_port.c` (UART / NVS / time / random / TCP)
3. Build, flash, and use the same command format over serial (115200)

> Note: ESP32 uses the Xtensa core while Keil MDK mainly targets ARM Cortex-M. For Keil builds, replace the TODOs in `fe_port.c` with ESP-IDF APIs (a full reference snippet is included at the end of the file), or reuse this framework directly on other Cortex-M MCUs.

### 6-b. PlatformIO IDE Version (VS Code plugin)

`platformio_ide/` is a **bare-metal C + ESP-IDF framework** project that reuses the keil C code with a real ESP-IDF `fe_port.c` (UART/NVS/SNTP/random/WiFi/TCP). No Keil needed — build and flash right from VS Code.

1. Install the **PlatformIO IDE** extension in VS Code (prompted when opening `platformio_ide/`)
2. Open the `platformio_ide/` directory
3. Click **Build** / **Upload** / **Serial Monitor** (115200) in the status bar

```bash
cd platformio_ide
pio run            # build
pio run -t upload  # flash
pio device monitor # serial monitor
```

> Unlike `arduino/` (Arduino C++ framework), this version is a pure-C implementation on the ESP-IDF framework; serial commands are identical.

### 7. Correspondence with the Main Repo

- Command names are **identical** to the main repo (e.g. `issue_token`, `sync_ntp`, `write_holding`)
- `Atom` model simplified: multi-instance Atom → MCU singleton global Atom
- Strict type checking retained: invalid args return `invalid arguments`-style errors
- Tokens/secrets persist to NVS by default and survive reboot
