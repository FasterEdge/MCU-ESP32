// FasterEdge 开源项目 - Github: https://github.com/FasterEdge - Gitee: https://gitee.com/FasterEdge
// ability_time.cpp — TimeAbility 实现（Arduino 版）
// sync_net / sync_manual / sync_system / sync_ntp / get_time / configure_run
#include "fe_ability.h"
#include <Preferences.h>
#include <errno.h>
#include <limits.h>

namespace fe {

static bool isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

// Howard Hinnant's civil-date conversion: days since 1970-01-01.
static int64_t daysFromCivil(int year, unsigned month, unsigned day) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yoe = (unsigned)(year - era * 400);
    const unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (int64_t)era * 146097 + (int64_t)doe - 719468;
}

// Parse an epoch or exactly "YYYY-MM-DD HH:MM:SS" as UTC.
static bool parseEpoch(const String &input, time_t &epoch) {
    String s = input;
    s.trim();
    if (s.length() == 0) return false;

    bool numeric = true;
    size_t start = (s[0] == '+' || s[0] == '-') ? 1 : 0;
    if (start == s.length()) numeric = false;
    for (size_t i = start; numeric && i < s.length(); ++i)
        if (!isdigit((unsigned char)s[i])) numeric = false;

    int64_t value = 0;
    if (numeric) {
        errno = 0;
        char *end = nullptr;
        long long parsed = strtoll(s.c_str(), &end, 10);
        if (errno == ERANGE || !end || *end != '\0' || parsed <= 0) return false;
        value = (int64_t)parsed;
    } else {
        if (s.length() != 19 || s[4] != '-' || s[7] != '-' || s[10] != ' ' ||
            s[13] != ':' || s[16] != ':') return false;
        const uint8_t digitPositions[] = {0,1,2,3,5,6,8,9,11,12,14,15,17,18};
        for (uint8_t pos : digitPositions)
            if (!isdigit((unsigned char)s[pos])) return false;
        int year = s.substring(0, 4).toInt();
        int month = s.substring(5, 7).toInt();
        int day = s.substring(8, 10).toInt();
        int hour = s.substring(11, 13).toInt();
        int minute = s.substring(14, 16).toInt();
        int second = s.substring(17, 19).toInt();
        static const uint8_t monthDays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
        if (year < 1970 || year > 9999 || month < 1 || month > 12 || hour < 0 ||
            hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 59) return false;
        int maxDay = monthDays[month - 1] + ((month == 2 && isLeapYear(year)) ? 1 : 0);
        if (day < 1 || day > maxDay) return false;
        value = daysFromCivil(year, (unsigned)month, (unsigned)day) * 86400LL +
                hour * 3600LL + minute * 60LL + second;
    }

    time_t converted = (time_t)value;
    if ((int64_t)converted != value) return false;
    epoch = converted;
    return true;
}

static bool validServer(const String &server) {
    if (server.length() == 0 || server.length() > 120) return false;
    for (size_t i = 0; i < server.length(); ++i) {
        char c = server[i];
        if (!(isalnum((unsigned char)c) || c == '.' || c == '-' || c == ':' || c == '_')) return false;
    }
    return true;
}

static void loadRunState(TimeAbility &self) {
    if (self.runStateLoaded) return;
    Preferences prefs;
    if (prefs.begin("fe_time", true)) {
        self.runEnabled = prefs.getBool("enabled", false);
        self.runIntervalSec = prefs.getUInt("interval", 3600);
        self.ntpServer = prefs.getString("server", "pool.ntp.org");
        prefs.end();
    }
    if (self.runIntervalSec < 15) self.runIntervalSec = 3600;
    if (!validServer(self.ntpServer)) self.ntpServer = "pool.ntp.org";
    self.nextRunMs = 0;
    self.runStateLoaded = true;
}

static bool persistRunState(const TimeAbility &self) {
    Preferences prefs;
    if (!prefs.begin("fe_time", false)) return false;
    bool ok = prefs.putBool("enabled", self.runEnabled) == 1;
    ok = prefs.putUInt("interval", self.runIntervalSec) == sizeof(uint32_t) && ok;
    ok = prefs.putString("server", self.ntpServer) == self.ntpServer.length() && ok;
    prefs.end();
    return ok;
}

static String runStateJson(const TimeAbility &self) {
    return String("{\"enabled\":") + (self.runEnabled ? "true" : "false") +
           ",\"interval_sec\":" + self.runIntervalSec +
           ",\"server\":\"" + self.ntpServer + "\"}";
}

static String epochText(time_t epoch) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", (long long)epoch);
    return String(buf);
}

void timeAbilityTick(TimeAbility &self, uint32_t nowMs) {
    loadRunState(self);
    if (!self.runEnabled) return;
    if (self.nextRunMs == 0 || (int32_t)(nowMs - self.nextRunMs) >= 0) {
        configTime(0, 0, self.ntpServer.c_str());
        uint64_t intervalMs = (uint64_t)self.runIntervalSec * 1000ULL;
        if (intervalMs > UINT32_MAX) intervalMs = UINT32_MAX;
        self.nextRunMs = nowMs + (uint32_t)intervalMs;
    }
}

CommandOutput timeAbilityDispatch(void *inst, const char *act, const String &args) {
    TimeAbility *self = static_cast<TimeAbility *>(inst);
    loadRunState(*self);

    if (strcmp(act, "get_time") == 0) {
        time_t now = time(nullptr);
        struct tm tmv;
        gmtime_r(&now, &tmv);
        char buf[40];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d UTC",
                 tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
                 tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
        return CommandOutput{String(act), String(buf), String()};
    }
    if (strcmp(act, "sync_manual") == 0) {
        time_t ep;
        if (!parseEpoch(args, ep))
            return CommandOutput{String(act), String(), String("invalid time; expect positive epoch or YYYY-MM-DD HH:MM:SS UTC")};
        struct timeval tv = { .tv_sec = ep, .tv_usec = 0 };
        if (settimeofday(&tv, nullptr) != 0)
            return CommandOutput{String(act), String(), String("settimeofday failed")};
        self->manualEpoch = ep;
        return CommandOutput{String(act), String("epoch=") + epochText(ep), String()};
    }
    if (strcmp(act, "sync_system") == 0) {
        time_t now = time(nullptr);
        return CommandOutput{String(act), String("epoch=") + epochText(now), String()};
    }
    if (strcmp(act, "sync_ntp") == 0 || strcmp(act, "sync_net") == 0) {
        String server = args.length() ? args : self->ntpServer;
        if (!validServer(server))
            return CommandOutput{String(act), String(), String("invalid NTP server")};
        configTime(0, 0, server.c_str());
        return CommandOutput{String(act), String("{\"started\":true,\"server\":\"") + server + "\"}", String()};
    }
    if (strcmp(act, "configure_run") == 0) {
        String cfg = args;
        cfg.trim();
        if (cfg.length() == 0 || cfg == "status")
            return CommandOutput{String(act), runStateJson(*self), String()};
        if (cfg == "off" || cfg == "disable" || cfg == "0") {
            self->runEnabled = false;
            self->nextRunMs = 0;
        } else {
            int space = cfg.indexOf(' ');
            String intervalText = space < 0 ? cfg : cfg.substring(0, space);
            String server = space < 0 ? self->ntpServer : cfg.substring(space + 1);
            server.trim();
            char *end = nullptr;
            unsigned long interval = strtoul(intervalText.c_str(), &end, 10);
            if (!end || *end != '\0' || interval < 15 || interval > 4294967UL)
                return CommandOutput{String(act), String(), String("expect: <interval_sec>=15..4294967 [server], or off/status")};
            if (!validServer(server))
                return CommandOutput{String(act), String(), String("invalid NTP server")};
            self->runEnabled = true;
            self->runIntervalSec = (uint32_t)interval;
            self->ntpServer = server;
            self->nextRunMs = millis(); // run on the next scheduler tick
        }
        if (!persistRunState(*self))
            return CommandOutput{String(act), String(), String("failed to persist run configuration")};
        return CommandOutput{String(act), runStateJson(*self), String()};
    }
    return CommandOutput{String(act), String(), String("unsupported command: ") + act};
}

} // namespace fe
