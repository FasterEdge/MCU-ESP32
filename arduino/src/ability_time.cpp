// ability_time.cpp — TimeAbility 实现（Arduino 版）
// sync_net / sync_manual / sync_system / sync_ntp / get_time / configure_run
// ESP32 使用 SNTP（configTime）进行网络校时；系统时间来自内部 RTC。
#include "fe_ability.h"

namespace fe {

// 将 'YYYY-MM-DD HH:MM:SS' 或 epoch 秒解析为 epoch。简化实现：
// 若参数为纯数字则直接当作 epoch；否则 TODO: 完整日期解析（可移植 mktime）
static long parseEpoch(const String &s) {
    if (s.length() == 0) return 0;
    bool numeric = true;
    for (size_t i = 0; i < s.length(); i++)
        if (!isdigit(s[i])) { numeric = false; break; }
    if (numeric) return s.toInt();
    // TODO: 完整日期解析 'YYYY-MM-DD HH:MM:SS' -> epoch（timegm）
    return 0;
}

CommandOutput timeAbilityDispatch(void *inst, const char *act, const String &args) {
    TimeAbility *self = static_cast<TimeAbility *>(inst);

    if (strcmp(act, "get_time") == 0) {
        time_t now = time(nullptr);
        struct tm tmv;
        localtime_r(&now, &tmv);
        char buf[40];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                 tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
                 tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
        return CommandOutput{String(act), String(buf), String()};
    }
    if (strcmp(act, "sync_manual") == 0) {
        long ep = parseEpoch(args);
        if (ep <= 0)
            return CommandOutput{String(act), String(), String("invalid epoch arg")};
        struct timeval tv = { .tv_sec = ep, .tv_usec = 0 };
        settimeofday(&tv, nullptr);
        self->manualEpoch = ep;
        return CommandOutput{String(act), String("epoch=") + ep, String()};
    }
    if (strcmp(act, "sync_system") == 0) {
        // 系统时间（内部 RTC / 上次 SNTP 值）
        time_t now = time(nullptr);
        return CommandOutput{String(act), String("epoch=") + (long)now, String()};
    }
    if (strcmp(act, "sync_ntp") == 0) {
        // 从 NTP 服务器同步。参数可选：NTP 服务器地址（默认 pool.ntp.org）
        const char *server = args.length() ? args.c_str() : "pool.ntp.org";
        configTime(0, 0, server);          // 无时区偏移
        // 等待首个 NTP 响应（超时 5s）
        time_t now = 0;
        int tries = 0;
        while (time(&now) < 8 * 3600 * 2 && tries < 50) { // 直到得到合理时间
            delay(100); tries++;
        }
        return CommandOutput{String(act), String("server=") + server + " epoch=" + (long)now, String()};
    }
    if (strcmp(act, "configure_run") == 0) {
        // TODO: 配置周期校时运行（如每 3600s 调用一次 sync_ntp）
        return CommandOutput{String(act), String("configured"), String()};
    }
    return CommandOutput{String(act), String(), String("unsupported command: ") + act};
}

} // namespace fe