/* Probe 2: the two-axis distance block of battle/main/misc12 func_80084854.
 * Question: does gcc 2.6.0 reload `*p` for the second axis (retail does: it
 * emits three `lbu v1,0(s1)`), or CSE it with the first axis' load?
 * Scratch, not part of any build. */
#include "common.h"

typedef struct {
    /* 0x00 */ u16 x;
    /* 0x02 */ u16 y;
    /* 0x04 */ u8 unk4[0x18];
} ProbeEntry; // Size: 0x1C

extern ProbeEntry ProbeTable[];
extern u8 ProbeList[];

s32 ProbeTwoAxes(u8 arg0, u8 arg1) {
    u8 *p;
    s32 d;
    s32 dist;
    s32 best;

    best = 0xFFFFFF;
    p = ProbeList;
    do {
        if (*p != 0xFF && *p != arg0) {
            d = ProbeTable[*p].y - ProbeTable[arg0].y;
            if (d < 0) {
                d = -d;
            }
            dist = d * d;
            d = ProbeTable[*p].x - ProbeTable[arg0].x;
            if (d < 0) {
                d = -d;
            }
            dist += d * d;
            if (dist < best) {
                best = dist;
            }
        }
    } while (++p < ProbeList + 11);
    (void)arg1;
    return best;
}
