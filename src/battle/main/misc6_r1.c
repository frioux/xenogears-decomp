#include "common.h"

extern u8 D_800D3420[];
extern u8 D_800D3430[];

// 100.0% under gcc 2.7.2-cdk, 97.x under this unit's 2.7.2: the whole run of
// three functions (AD6C, ADB0, ADF4) wants a cdk TU. Splitting it needs
// tools/scripts/split_unit.py, whose chunking still cuts park blocks - finish that
// first (see reports/TRACKER.md, session 7).
void func_8007AD6C(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[1]] &= pArgs[2] + (pArgs[3] << 8);
}
// 100.0% under gcc 2.7.2-cdk, 97.x under this unit's 2.7.2: the whole run of
// three functions (AD6C, ADB0, ADF4) wants a cdk TU. Splitting it needs
// tools/scripts/split_unit.py, whose chunking still cuts park blocks - finish that
// first (see reports/TRACKER.md, session 7).
void func_8007ADB0(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[1]] |= pArgs[2] + (pArgs[3] << 8);
}
// 100.0% under gcc 2.7.2-cdk, 97.x under this unit's 2.7.2: the whole run of
// three functions (AD6C, ADB0, ADF4) wants a cdk TU. Splitting it needs
// tools/scripts/split_unit.py, whose chunking still cuts park blocks - finish that
// first (see reports/TRACKER.md, session 7).
void func_8007ADF4(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[1]] ^= pArgs[2] + (pArgs[3] << 8);
}
