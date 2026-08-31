// main.cpp — FasterEdge MCU Arduino 入口（ESP32）
// 提供串口命令解释器：输入 "data_xxx act args" / "ability_xxx act args"
// 示例：
//   ability_BaseAbility list_ability_names
//   data_ConfigData set wifi.ssid=MyNet
//   ability_TimeAbility sync_ntp
#include "fe.h"
#include "fe_ability.h"
#include "fe_data.h"

using namespace fe;

String inputBuffer;

void setup() {
    Serial.begin(115200);
    delay(200);
    Serial.println();
    Serial.println("FasterEdge-MCU (ESP32 / Arduino)");
    Serial.println("input: <data|ability>_<Name> <act> [args]  |  'help'");
    initAll();

    // 可选：连接 WiFi（MQTT/SNTP 需要）。取消注释并填好凭据
    // WiFi.mode(WIFI_STA);
    // WiFi.begin("SSID", "PASSWORD");
}

void printHelp() {
    Serial.println("Usage: <data|ability>_<Name> <act> [args]");
    auto &atom = globalAtom();
    auto names = atom.listAbilityNames();
    Serial.print("abilities: ");
    for (auto &n : names) { Serial.print(n); Serial.print(" "); }
    Serial.println();
    auto dnames = atom.listDataNames();
    Serial.print("data: ");
    for (auto &n : dnames) { Serial.print(n); Serial.print(" "); }
    Serial.println();
    Serial.println("examples:");
    Serial.println("  ability_BaseAbility list_ability_names");
    Serial.println("  ability_RoleAbility set_role edge");
    Serial.println("  ability_TimeAbility sync_ntp");
    Serial.println("  ability_OneKeyAbility issue_token sensor01");
    Serial.println("  ability_SerialAbility open 0");
    Serial.println("  ability_ModbusAbility write_holding 0,42");
    Serial.println("  data_ConfigData set wifi.ssid=MyNet");
    Serial.println("  data_KeyringData issue_token sensor01");
}

void loop() {
    runTimeAbilityTick(millis());
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n') {
            inputBuffer.trim();
            if (inputBuffer.length() == 0) { inputBuffer = ""; continue; }
            if (inputBuffer == "help") { printHelp(); inputBuffer = ""; continue; }

            // 解析：target act [args]
            int sp1 = inputBuffer.indexOf(' ');
            if (sp1 <= 0) {
                Serial.println("bad command");
                inputBuffer = "";
                continue;
            }
            String target = inputBuffer.substring(0, sp1);
            String rest = inputBuffer.substring(sp1 + 1);
            int sp2 = rest.indexOf(' ');
            String act, args;
            if (sp2 > 0) { act = rest.substring(0, sp2); args = rest.substring(sp2 + 1); }
            else         { act = rest; }

            CommandOutput out = globalAtom().execute(target.c_str(), act.c_str(), args);
            if (out.ok()) {
                Serial.print("OK ");
                Serial.print(out.name);
                if (out.value.length()) { Serial.print(" -> "); Serial.print(out.value); }
                Serial.println();
            } else {
                Serial.print("ERR ");
                Serial.print(out.name);
                Serial.print(": ");
                Serial.println(out.err);
            }
            inputBuffer = "";
        } else if (c == '\r') {
            // ignore
        } else {
            inputBuffer += c;
        }
    }
    delay(10);
}