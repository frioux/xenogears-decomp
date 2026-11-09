#include "common.h"

extern s32 D_8009D554;
extern s32 D_8009D7CC;
extern s32 D_8009CCA4;
extern s32 D_8009D3CC;

extern void func_800894C8(s32 arg0);
extern void func_80089514(s32 arg0);
extern void func_80097770(s32 arg0, s32 arg1);
extern s32* D_80062528;

extern void func_8003A89C(s32* pManager);
extern void func_80089160(s32 arg0, void* arg1, s32 arg2);
extern void func_80039E60(s32 arg0);
extern void func_8003A3B8(s32 arg0, s32 arg1, s32 arg2);

extern s32 D_8009C5AC;
extern s32 D_8009C5B0;
extern s32 D_8009C5B4;
extern s32 D_8009D3F0;
extern void* D_8006259C;

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_80076A1C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_80076B34);

s32 func_80076BC4(void) {
    D_8009D554 = 0;
    D_8009D7CC = 0;
    return 0;
}

s32 func_80076BDC(u8* p, s32 arg1) {
    s16 timer = *(s16*)(p + 0x22);

    if (timer == 0) {
        *(s16*)(p + 0x22) = arg1;
        return 0;
    }
    timer--;
    *(s16*)(p + 0x22) = timer;
    return (timer < 1) << 1;
}

s32 func_80076C18(s32 arg0, s32 arg1, s32 arg2) {
    func_80097770(arg1, arg2);
    return 4;
}

s32 func_80076C3C(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    D_8009C5AC = arg1 << 12;
    D_8009C5B0 = arg2 << 12;
    D_8009C5B4 = arg3 << 12;
    return 4;
}

s32 func_80076C68(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    *(u16*)0x1F8000A0 = arg1;
    *(u16*)0x1F8000A2 = arg2;
    *(u16*)0x1F8000A4 = arg3;
    return 4;
}

s32 func_80076C88(s32 arg0, s32 arg1) {
    func_80089160(arg1, (void*)0x1F8000A0, 0);
    return 2;
}

s32 func_80076CB4(s32 arg0, s32 arg1) {
    func_800894C8(arg1);
    return 2;
}

s32 func_80076CD4(s32 arg0, s32 arg1) {
    func_80089514(arg1);
    return 2;
}

s32 func_80076CF4(void) {
    func_8003A89C(D_80062528);
    return 4;
}

s32 func_80076D1C(s32 arg0, s32 arg1) {
    func_80039E60((*(u16*)((u8*)D_8006259C + 0x14) << 16) | arg1);
    return 2;
}

s32 func_80076D50(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    func_8003A3B8((*(u16*)((u8*)D_8006259C + 0x14) << 16) | arg1, arg2, arg3);
    return 4;
}

s32 func_80076D8C(s32 arg0, s32 arg1, s32 arg2) {
    D_8009CCA4 = arg1;
    D_8009D3CC = arg2;
    return 4;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_80076DA4);
void func_80076F54(u8* p) {
    s32 cur = *(s32*)(p + 0x5C);
    s32 idx;

    if (cur != D_8009D3F0) {
        idx = (cur - D_8009D3F0) >> 3;
        if ((idx < 0 ? -idx : idx) < 0x40) {
            D_8009D3F0 = cur;
        } else {
            D_8009D3F0 = D_8009D3F0 + idx;
        }
    }
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_80076FA8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_800771D8);
/* Best attempt: 99.67% - the two abs() locals are right, but gcc copy-propagates
   `limit` back to the parameter and emits `negu v1,a2` where the original has
   `negu v1,v1`. Tried: 0 - limit, arg2 test, move the decl before the early
   return; the negu operand always folds to a2.
s32 func_800771D8(s32 arg0, s32 arg1, s32 arg2) {
    s32 delta;
    s32 limit;

    if (arg0 == arg1) {
        return arg0;
    }
    delta = arg1 - arg0;
    if (delta < 0) {
        delta = -delta;
    }
    limit = arg2;
    if (limit < 0) {
        limit = -limit;
    }
    if (delta < limit) {
        arg0 = arg1;
    } else {
        arg0 += arg2;
    }
    return arg0;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_80077214);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_80077480);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_8007756C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_800776E0);

s32 func_80077954(void) {
    return 1;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_8007795C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_80077A64);
