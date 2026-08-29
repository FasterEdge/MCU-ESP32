// ability_mqtt.cpp — MQTTAbility 实现（Arduino 版）
// set_broker / connect / disconnect / publish / subscribe / unsubscribe /
// is_connected / list_subscriptions / drain
// 使用 PubSubClient 库（platformio.ini 中声明依赖）。
#include "fe_ability.h"
#include <WiFi.h>
#include <PubSubClient.h>

namespace fe {

// 全局 MQTT 客户端单例（由注册模块持有）
static WiFiClient g_wifiClient;
static PubSubClient g_mqtt(g_wifiClient);

// 回调：收到订阅消息后可转发到 CommandOutput 队列（简化：仅打印）
static void onMessage(char *topic, byte *payload, unsigned int length) {
    Serial.printf("[MqttAbility] recv %s: ", topic);
    for (unsigned int i = 0; i < length; i++) Serial.write(payload[i]);
    Serial.println();
}

CommandOutput mqttAbilityDispatch(void *inst, const char *act, const String &args) {
    MQTTAbility *self = static_cast<MQTTAbility *>(inst);

    if (strcmp(act, "set_broker") == 0) {
        // 参数：host[:port]
        if (args.length() == 0)
            return CommandOutput{String(act), String(), String("missing broker")};
        self->broker = args;
        int colon = args.indexOf(':');
        if (colon > 0) {
            g_mqtt.setServer(args.substring(0, colon).c_str(),
                             args.substring(colon + 1).toInt());
        } else {
            g_mqtt.setServer(args.c_str(), 1883);
        }
        return CommandOutput{String(act), String("broker=") + self->broker, String()};
    }
    if (strcmp(act, "connect") == 0) {
        if (self->broker.length() == 0)
            return CommandOutput{String(act), String(), String("broker not set")};
        // 参数可选：clientId（默认 FasterEdge-<chip id>）
        String cid = args.length() ? args : String("FasterEdge-") + String((uint32_t)ESP.getEfuseMac(), HEX);
        g_mqtt.setCallback(onMessage);
        bool ok = g_mqtt.connect(cid.c_str());
        if (ok) { self->connected = true; self->clientId = cid; }
        return CommandOutput{String(act),
            String("{\"connected\":") + (ok ? "true" : "false") + ",\"clientId\":\"" + cid + "\"}",
            ok ? String() : String("mqtt connect failed")};
    }
    if (strcmp(act, "disconnect") == 0) {
        g_mqtt.disconnect();
        self->connected = false;
        return CommandOutput{String(act), String("{\"connected\":false}"), String()};
    }
    if (strcmp(act, "is_connected") == 0) {
        self->connected = g_mqtt.connected();
        return CommandOutput{String(act),
            String("{\"connected\":") + (self->connected ? "true" : "false") + "}", String()};
    }
    if (strcmp(act, "publish") == 0) {
        // 参数：topic,payload（第一个逗号前为 topic）
        int comma = args.indexOf(',');
        if (comma <= 0)
            return CommandOutput{String(act), String(), String("bad format, expect topic,payload")};
        String topic = args.substring(0, comma);
        String payload = args.substring(comma + 1);
        bool ok = g_mqtt.publish(topic.c_str(), payload.c_str());
        return CommandOutput{String(act),
            String("{\"published\":") + (ok ? "true" : "false") + "}",
            ok ? String() : String("publish failed")};
    }
    if (strcmp(act, "subscribe") == 0) {
        if (args.length() == 0)
            return CommandOutput{String(act), String(), String("missing topic")};
        bool ok = g_mqtt.subscribe(args.c_str());
        if (ok) self->subscriptions.push_back(args);
        return CommandOutput{String(act),
            String("{\"subscribed\":") + (ok ? "true" : "false") + "}",
            ok ? String() : String("subscribe failed")};
    }
    if (strcmp(act, "unsubscribe") == 0) {
        if (args.length() == 0)
            return CommandOutput{String(act), String(), String("missing topic")};
        g_mqtt.unsubscribe(args.c_str());
        for (auto it = self->subscriptions.begin(); it != self->subscriptions.end(); ++it)
            if (*it == args) { self->subscriptions.erase(it); break; }
        return CommandOutput{String(act), String("{\"unsubscribed\":true}"), String()};
    }
    if (strcmp(act, "list_subscriptions") == 0) {
        String out = "[";
        for (size_t i = 0; i < self->subscriptions.size(); i++) {
            if (i) out += ",";
            out += "\"" + self->subscriptions[i] + "\"";
        }
        out += "]";
        return CommandOutput{String(act), out, String()};
    }
    if (strcmp(act, "drain") == 0) {
        // 处理一次收包队列
        g_mqtt.loop();
        return CommandOutput{String(act), String("{\"drained\":true}"), String()};
    }
    return CommandOutput{String(act), String(), String("unsupported command: ") + act};
}

} // namespace fe