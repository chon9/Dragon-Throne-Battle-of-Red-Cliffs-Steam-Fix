#include "LatencyStats.h"
#include <cassert>
#include <cstdio>
int main() {
    dt::DurationStats s;
    s.Add(-1); assert(s.count==0);
    s.Add(1000); s.Add(3000);
    assert(s.count==2 && s.MeanMs()==2 && s.MaxMs()==3);
    dt::LatencyStats d;
    d.Poll(0xfffffff0); d.Poll(0x10);
    assert(d.pollGap.MaxMs()==32);
    assert(!d.ReportDue(0xfffffff0));
    assert(!d.ReportDue(0x10));
    assert(d.ReportDue(uint32_t(0xfffffff0u+10000u)));
    d.sent=10; d.delivered=5; d.ResetWindow();
    assert(d.sent==0 && d.delivered==0 && d.pollGap.count==0);
    assert(d.haveReport && d.havePoll);
    puts("PASS: latency samples, invalid durations, tick rollover, bounded reporting and window reset");
}
