// FasterEdge 开源项目 - Github: https://github.com/FasterEdge - Gitee: https://gitee.com/FasterEdge
// data_netmap.cpp — NetMapData 实现（Arduino 版）
// 本节点网络信息：info / set_node_name / interfaces / set_default_iface
#include "fe_data.h"
#include <WiFi.h>

namespace fe {

// 获取本节点 IP（简化：返回 STA IP 或 AP IP）
static String currentIp() {
    if (WiFi.isConnected()) return WiFi.localIP().toString();
    if (WiFi.getMode() & WIFI_AP) return WiFi.softAPIP().toString();
    return "0.0.0.0";
}

CommandOutput netMapDataDispatch(void *inst, const char *act, const String &args) {
    NetMapData *self = static_cast<NetMapData *>(inst);

    if (strcmp(act, "info") == 0) {
        return CommandOutput{String(act),
            String("{\"node\":\"") + self->nodeName + "\",\"ip\":\"" + currentIp() +
            "\",\"mac\":\"" + WiFi.macAddress() + "\",\"rssi\":" + WiFi.RSSI() + "}", String()};
    }
    if (strcmp(act, "set_node_name") == 0) {
        if (args.length() == 0)
            return CommandOutput{String(act), String(), String("missing name")};
        self->nodeName = args;
        return CommandOutput{String(act), String("node=") + self->nodeName, String()};
    }
    if (strcmp(act, "interfaces") == 0) {
        String out = "[";
        out += "{\"name\":\"wlan0\",\"mac\":\"" + WiFi.macAddress() +
               "\",\"ip\":\"" + currentIp() + "\"}";
        out += "]";
        return CommandOutput{String(act), out, String()};
    }
    if (strcmp(act, "set_default_iface") == 0) {
        self->defaultIface = args.length() ? args : "wlan0";
        return CommandOutput{String(act), String("iface=") + self->defaultIface, String()};
    }
    return CommandOutput{String(act), String(), String("unsupported command: ") + act};
}

} // namespace fe
