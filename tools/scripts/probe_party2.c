#include "common.h"
extern s32 g_ProbeQ[3];
extern s32 g_ProbeQ1q __asm__("\"g_ProbeQ+4\"");
extern s32 g_ProbeQ2q __asm__("\"g_ProbeQ+8\"");
s32 ProbeQuoted(s32 v) {
    s32 m = g_ProbeQ1q;
    s32 s = g_ProbeQ2q;
    g_ProbeQ[1] = 7;
    g_ProbeQ1q = m + v;
    g_ProbeQ2q = s + v;
    return m + s;
}
