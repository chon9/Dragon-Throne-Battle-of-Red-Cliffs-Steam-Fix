#pragma once
#include <cstdint>
namespace dt {
// Local durations only. Never subtract clocks from different machines.
struct DurationStats {
    uint64_t count = 0, totalUs = 0, maxUs = 0;
    void Add(int64_t us) {
        if (us < 0) return;
        ++count;
        totalUs += static_cast<uint64_t>(us);
        if (static_cast<uint64_t>(us) > maxUs) maxUs = static_cast<uint64_t>(us);
    }
    double MeanMs() const { return count ? double(totalUs) / double(count) / 1000.0 : 0.0; }
    double MaxMs() const { return double(maxUs) / 1000.0; }
};
struct LatencyStats {
    DurationStats steamInbox, gameInbox, pollGap;
    uint64_t sent = 0, delivered = 0;
    uint32_t lastPoll = 0, lastReport = 0;
    bool havePoll = false, haveReport = false;
    void Poll(uint32_t now) {
        if (havePoll) pollGap.Add(uint64_t(uint32_t(now-lastPoll))*1000);
        havePoll = true; lastPoll = now;
    }
    bool ReportDue(uint32_t now) {
        if (!haveReport) { haveReport = true; lastReport = now; return false; }
        if (uint32_t(now-lastReport) < 10000) return false;
        lastReport = now; return true;
    }
    void ResetWindow() {
        steamInbox = {}; gameInbox = {}; pollGap = {};
        sent = delivered = 0;
    }
};
}
