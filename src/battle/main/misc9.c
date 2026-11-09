#include "common.h"

extern u8 D_800D2C60[];
extern u8 D_800D2C8B[];
extern u8 *D_800D3278;
extern u8 *D_800D3364;
extern u8 D_800D3410[];
extern u8 D_800D3430[];

INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007D8C0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007DA1C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007DB78);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007DCF8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007DE78);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007DFD4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E154);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E1D0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E234);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E334);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E438);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E554);
void func_8007E674(u8 arg0) {
    ((u32 *)D_800D2C60)[arg0] = 0;
    D_800D2C8B[arg0] = 4;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E6A0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E6F0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E740);
/* best: 95.31%. Identical instruction multiset and operand order; the only
   difference is the register pair: the original keeps the D_800D3410 base in
   v0 and the row address in v1 (mflo then reuses the dead v0), gcc emits the
   mirror image (base v1, row v0, mflo a2). Same allocator tie as func_8007E8AC
   and the misc6 D_800D34x0 family. Tried: s32 vs u32 element, pRow before/after
   pArgs, product written as p[2] * pRow[p[1]] (this variant, operand order
   right), compound assignment.
void func_8007E740(u8 **arg0, u8 arg1) {
    u32 *pRow = (u32 *)(D_800D3410 + arg1 * 0x40);
    u8 *p = *arg0;

    pRow[p[1]] = p[2] * pRow[p[1]];
} */
void func_8007E780(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u32 *pRow = (u32 *)(D_800D3410 + arg1 * 0x40);

    pRow[p[1]] /= p[2];
}
void func_8007E7C0(u8 **arg0) {
    u8 *p = *arg0;

    *(u16 *)(D_800D3278 + p[1] * 2 + 0x394) = p[2];
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E7E4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E8AC);
/* best: 95.38%. Identical instruction multiset and instruction order; only the
   v0/v1 pair is mirrored (original: p[2] in v1, p[1] in v0, sum in v1, the
   D_800D3364 pointer reusing v0). Same allocator tie as func_8007E740. Tried:
   both summand orders, inline index, s32 idx local, pointer local for the base.
void func_8007E8AC(u8 **arg0) {
    u8 *p = *arg0;
    s32 idx = p[2] * 8 + p[1] * 0x40;

    D_800D3364[idx + 0x140] = p[3];
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E8E0);
void func_8007E934(void) {
    u8 buf[0x10];

    func_80078508(buf);
}
s32 func_8007E954(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    return pRow[p[1]] == p[2];
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E98C);
s32 func_8007E9D0(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    return p[2] >= pRow[p[1]];
}
