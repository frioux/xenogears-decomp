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

extern u8* D_8009BE24;
extern s32 D_8009D144;
extern s32 D_8009BD40;
extern s16 D_8009D558;
extern s32 D_8009BBB4;
extern s32 D_8009BE28;
extern u8* D_8009BE3C;
extern s32 D_8009C5BC;
extern s32 D_8009BC38;
extern s32 D_8009BCB0;
extern s32 D_8009BC3C;
extern s32 D_8009BCB4;
extern s32 D_8009C180;
extern s16 D_8009BD3A;
extern s16 D_8006F94E;
extern s16 D_8006F950;
extern s16 D_8006F954;
extern s32 D_8009BBC4;

extern void func_80097440(s32* p);
extern void func_80097244(s32* p);
extern void func_80089748(void);
extern void func_80089C78(void);
extern void func_8008615C(void);
extern void func_800848F4(void);
extern void func_800980D4(s32* p);
extern void func_800981C8(s32* p);
extern void func_80096130(void);
extern void func_80098CC0(void);
extern void func_800983A0(s32* p);
extern void func_8009932C(s32 arg0, s32 arg1, s32* p);
extern void func_80073B04(void);
extern void func_800737EC(void);
extern void func_80086798(void);
extern void func_80084818(void);
extern void func_80086124(void);
extern void func_80086568(void);
extern void func_800866C8(void);
extern void func_80074F04(void);
extern void func_800750DC(void);
extern void func_80088FF4(void);
extern void func_80089128(void);
extern void func_80097D64(void);
extern void func_800976A0(void);
extern void HeapFree(void* pMem);

typedef s32 (*WorldMapScriptHandler)(u8* pSlot, s32 arg1, s16 arg2, s16 arg3);
extern WorldMapScriptHandler D_8009A3C0[];

s32 func_80076A1C(void) {
    u8* p;
    s32* pState;

    if (D_8009D144 == 0) {
        func_80097440(&D_8009BD40);
    } else {
        func_80097244(&D_8009BD40);
    }
    func_80089748();
    func_80089C78();
    func_8008615C();
    func_800848F4();
    func_800980D4(&D_8009BBB4);
    if (D_8009D558 != 0) {
        func_800981C8(&D_8009BE28);
        func_80096130();
        func_80098CC0();
    }
    pState = &D_8009BE28;
    func_800983A0(pState);
    p = D_8009BE3C;
    func_8009932C(*(s32*)(p + 0x70), *(s32*)(p + 0x74), pState);
    D_8009C5BC = D_8009C5BC + 0x40;
    func_80073B04();
    func_800737EC();
    func_80086798();
    return 1;
}

s32 func_80076B34(s32 arg0) {
    u8* pSlot = (u8*)D_8009BE24 + (arg0 << 7);
    u8* pCmd;
    s32 cmd;
    s32 n = 0;
    s32 pad[2];

    do {
        pCmd = *(u8**)(pSlot + 0x50) + (n << 1);
        *(u8**)(pSlot + 0x50) = pCmd;
        cmd = *(s32*)pCmd;
        n = D_8009A3C0[cmd & 0xFFFF](pSlot, cmd >> 16, *(s16*)(pCmd + 4), *(s16*)(pCmd + 6));
    } while (n != 0);
    return 1;
}

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
/* Best attempt: 99.67%. Only difference: the original negates the local
   (`negu v1,v1`) where gcc's cse substitutes the parameter (`negu v1,a2`),
   although the `move v1,a2` copy itself is emitted in the right place. Also
   tried: the ternary form `(arg2 < 0) ? -arg2 : arg2` (same code), the
   `limit = arg2` assign moved before the first abs (move lands in the wrong
   delay slot, 91.67%), `0 - limit`, an arg2 test instead of a limit test.
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
s32 func_80077480(void) {
    u16 v;

    func_80084818();
    func_80086124();
    func_80086568();
    func_800866C8();
    func_80074F04();
    func_800750DC();
    func_80088FF4();
    func_80089128();
    func_80097D64();
    HeapFree(D_8009BC38);
    HeapFree(D_8009BCB0);
    HeapFree(D_8009BC3C);
    HeapFree(D_8009BCB4);
    HeapFree(D_8009C180);
    func_800976A0();
    v = *(u16*)&D_8009BD3A;
    D_8006F94E = 0x11;
    D_8006F954 = 7;
    D_8009BBC4 = 1;
    D_8006F950 = v;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_8007756C);
/* Best attempt: 68.25%. Structure decoded and right: the 16-byte copy
   pSlot+0x28 -> pSlot+0x38 is one struct assignment (lw x4 then sw x4) and the
   three 8-byte copies around 0x1F8000A0 swap D_8009BD40 with D_8009BD48 through
   the scratchpad (needs a 1-byte-aligned struct, which is what gives lwl/lwr).
   Remaining diffs: the original keeps &D_8009BD40 in $s0 for the whole
   function (the call's a0 and two of the copies) and puts the address of
   D_8009BD38 in $a3 for its single store, where gcc rematerialises both
   with lui/$at. Next step: pointer locals for those two addresses. D_8009BD3A
   must be declared s16 (the original does lh for the << 12).
s32 func_8007756C(s32 arg0) {
    u8* pSlot = (u8*)D_8009BE24 + (arg0 << 7);

    D_8009BD38 = -0x80;
    D_8009BD3A = 0x200;
    D_8009D3F0 = 0x400000;
    D_8009BD3C = 0;
    *(s32*)(pSlot + 0x28) = 0xFFF80000;
    D_8009BE0C = 0x78;
    *(s32*)(pSlot + 0x2C) = D_8009BD3A << 12;
    *(WorldMapPos4*)(pSlot + 0x38) = *(WorldMapPos4*)(pSlot + 0x28);
    D_8009D144 = 0;
    D_8009BE28 = D_8009C5AC;
    D_8009BE2C = D_8009C5B0;
    D_8009BE30 = D_8009C5B4;
    func_80096F18(&D_8009BD40, &D_8009BE28, D_8009D3F0);
    *(WorldMapName8*)0x1F8000A0 = *(WorldMapName8*)&D_8009BD40;
    *(WorldMapName8*)&D_8009BD40 = *(WorldMapName8*)&D_8009BD48;
    *(WorldMapName8*)&D_8009BD48 = *(WorldMapName8*)0x1F8000A0;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_800776E0);

s32 func_80077954(void) {
    return 1;
}

s32 func_8007795C(void) {
    u8* p;
    s32* pState;

    if (D_8009D144 == 0) {
        func_80097440(&D_8009BD40);
    } else {
        func_80097244(&D_8009BD40);
    }
    func_8008615C();
    func_800848F4();
    func_800980D4(&D_8009BBB4);
    if (D_8009D558 != 0) {
        func_800981C8(&D_8009BE28);
        func_80096130();
        func_80098CC0();
    }
    pState = &D_8009BE28;
    func_800983A0(pState);
    p = D_8009BE3C;
    func_8009932C(*(s32*)(p + 0x70), *(s32*)(p + 0x74), pState);
    D_8009C5BC = D_8009C5BC + 0x40;
    func_80073B04();
    func_800737EC();
    func_80086798();
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_80077A64);
