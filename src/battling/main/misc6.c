#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"
#include "system/memory.h"

// .sdata/.bss of the overlay (supplied by splat's asm segments)
extern u8* D_800926C8;
extern u8* D_800926BC;
extern u8 D_800928A0;
extern u8 D_80092708;
extern u8 D_800926F0;
extern u8 D_800926F4;
extern u8 D_800926F8;
extern u8 D_8009273C;
extern u8 D_80092740;
extern s32 D_80092744;
extern s32 D_800912DC;
extern s32 D_80059488;
extern s16 D_800926E8;
extern s16 D_800926EC;
extern u8 D_8009791B;
extern u8 D_80099037;
extern u8 D_800954E0[];
extern s32 D_800928E8;

void func_8007E964(u8 c);
u8* func_8007E8AC(u8 c);
s32 func_8007EB6C(u8* pText);
extern s32 D_800926DC;

void func_8008EB4C(s32 arg);
void func_8008895C(void);

// 0x20-byte entry of the D_80092874 table; func_8007EEE8 reads only the s16 at 0x4.
typedef struct {
    /* 0x00 */ u8 unk0[4];
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 unk6[0x1A];
} BattleFilterEntry; // Size: 0x20

// 0xC-byte record whose address func_8007EEE8 queues into D_800928EC.
typedef struct {
    /* 0x00 */ u8 unk0[0xC];
} BattleQueueRecord; // Size: 0xC

extern BattleFilterEntry* D_80092874;
extern s32 D_80091964[];
extern u8** D_800928EC;
extern s32 D_80092888;

typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u16 unk4;
    /* 0x6 */ u16 unk6;
    /* 0x8 */ u16 unk8;
    /* 0xA */ u16 unkA;
} BattleSlot; // Size: 0xC

typedef struct {
    /* 0x00 */ u16 unk0;
    /* 0x02 */ u8 unk2;
    /* 0x03 */ u8 unk3;
    /* 0x04 */ u8 r;
    /* 0x05 */ u8 g;
    /* 0x06 */ u8 b;
    /* 0x07 */ u8 unk7;
    /* 0x08 */ s32 x;
    /* 0x0C */ s32 y;
    /* 0x10 */ SVECTOR v0;
    /* 0x18 */ SVECTOR v1;
} BattleQuad; // Size: 0x20

extern BattleQuad D_80094818[100];
extern s32 D_800926B0;
extern s32 D_800926B4;

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007E020);

void func_8007E24C(void) {
    s32 i;

    for (i = 0; i < 0x21C; i++) {
        ((BattleSlot*)D_800926C8)[i].unk6 = 0;
        ((BattleSlot*)D_800926C8)[i].unk4 = 0;
        ((BattleSlot*)D_800926C8)[i].unk2 = 0;
        ((BattleSlot*)D_800926C8)[i].unk0 = 0;
    }
    D_8009791B = 0;
    D_80099037 = 0;
    for (i = 0; i < 0xFF; i++) {
        *(D_800926BC + i * 0xA + 6) = 0;
        *(u16*)(D_800926BC + i * 0xA + 4) = 0;
        *(u16*)(D_800926BC + i * 0xA + 2) = 0;
        *(u16*)(D_800926BC + i * 0xA + 0) = 0;
    }
}

void func_8007E2D8(void) {
    BattleQuad* pQuad = D_80094818;
    s32 i;

    for (i = 0; i < 100; i++) {
        pQuad[i].unk3 = 3;
        pQuad[i].unk7 = 0x40;
    }
    D_800926B0 = 0;
    D_800926B4 = 0;
}

void func_8007E31C(s32* pPos, s32* pPos2, u8* pColor) {
    BattleQuad* pQuad;

    if (D_800926B0 < 100) {
        pQuad = &D_80094818[D_800926B0];
        pQuad->v0.vx = pPos[0];
        pQuad->v0.vy = pPos[1];
        pQuad->v0.vz = pPos[2];
        pQuad->v1.vx = pPos2[0];
        pQuad->v1.vy = pPos2[1];
        pQuad->v1.vz = pPos2[2];
        pQuad->r = pColor[0];
        pQuad->g = pColor[1];
        pQuad->b = pColor[2];
        D_800926B0++;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007E3CC);

void func_8007E528(s32 mode) {
    if (D_80092708 == 0 && mode == 0xA) {
        func_8008EB4C(0x24);
    }
    D_80092708 = mode;
}

void func_8007E574(void* pOt) {
    if (D_80092708 != 0) {
        if (D_800928E8 & 2) {
            AddPrim(pOt, &D_800954E0[D_800928A0 * 0x1C]);
            AddPrim(pOt, &D_800954E0[D_800928A0 * 0x1C - 8]);
        }
        D_80092708--;
    }
}

s32 func_8007E624(void) {
    return D_800926DC;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007E634);

void func_8007E894(s16 a, s16 b) {
    D_800926E8 = a;
    D_800926EC = b;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007E8AC);

void func_8007E954(s32 value) {
    D_800912DC = value;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007E964);

s32 func_8007EB6C(u8* pText) {
    s32 width = 0;

    while (*pText != 0) {
        u8* pGlyph = func_8007E8AC(*pText);

        pText++;
        width += 2 + ((pGlyph[2] * D_800912DC) >> 8);
    }
    return width;
}

void func_8007EBE0(u8* pText) {
    s32 y = D_800926E8;

    while (*pText != 0) {
        func_8007E964(*pText);
        pText++;
    }
    D_800926E8 = y;
    D_800926EC += 0x14;
}

void func_8007EC54(u8* pText) {
    s32 y = D_800926E8;

    D_800926E8 -= func_8007EB6C(pText) / 2;
    while (*pText != 0) {
        func_8007E964(*pText);
        pText++;
    }
    D_800926E8 = y;
    D_800926EC += 0x14;
}

void func_8007ECF0(u8* pText) {
    s32 y = D_800926E8;

    D_800926E8 -= func_8007EB6C(pText);
    while (*pText != 0) {
        func_8007E964(*pText);
        pText++;
    }
    D_800926E8 = y;
    D_800926EC += 0x14;
}

void func_8007ED84(u8* pText, s32 dy) {
    s32 tmp[2]; // unused: reproduces the original's 8-byte frame slot
    s32 y = D_800926E8;

    D_800926E8 = y - dy;
    while (*pText != 0) {
        func_8007E964(*pText);
        pText++;
    }
    D_800926E8 = y;
    D_800926EC += 0x14;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007EE08);
/* best 95.83%, one instruction short: the original materialises 0xFF twice (its
   own li at the head of each arm, the else arm copy stolen into the beqz delay
   slot by reorg); gcc 2.7.2 merges the two identical constants into the delay
   slot and both arms read it from there. Everything else (load of D_80059488
   first, sb order F4/F8/F0, x*20 as sll/addu/sll) is already exact.
void func_8007EE08(s32 arg) {
    if (arg != 0) {
        D_800926F4 = 0xFF;
        D_800926F8 = 0;
        D_800926F0 = D_80059488 * 20;
    } else {
        D_800926F0 = 0xFF;
        D_800926F4 = 0xFF;
        D_800926F8 = 0xFF;
    }
}
 */

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007EE68);
/* best 72.66%: the original loads D_80059488 TWICE into two registers (v1 for
   0xF0, a0 for 0xF4) and subtracts with li 0xFF + subu; gcc CSEs the two
   identical loads of the one extern and then strength-reduces 0xFF - (x * 20)
   to nor, since only the stored byte matters. The original subu means its two
   MEMs were two different symbols that both resolve to 0x80059488, and splat
   can only name one of them.
void func_8007EE68(s32 arg) {
    if (arg != 0) {
        D_800926F8 = 0xFF;
        D_800926F0 = 0xFF - D_80059488 * 20;
        D_800926F4 = 0xFF - D_80059488 * 20;
    } else {
        D_800926F0 = 0xFF;
        D_800926F4 = 0xFF;
        D_800926F8 = 0xFF;
    }
}
 */

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007EEE8);
/* best 90.45%: every instruction and relocation matches except which of the two
   loop induction pointers gets a0 and which gets a1 (the original keeps the
   0x20-byte table pointer in a1 and the 0xC-byte record pointer in a0) and, as
   a consequence, where the HeapAlloc result is copied into a3 in the loop
   preheader. Frame (0x20, ra at 0x18), the s0/s1 split, the
   g_GameState+0x1930 lhu, the filter compare and the D_80092888
   read-modify-write are exact. Tried: index and pointer forms for both
   induction variables and every mixture, i*3 over an s32 array, a nested if
   with continue, split increment, six declaration orders, and -G0 / -O3 /
   gcc 2.6.0 / gcc 2.7.2-cdk (90.45 or worse under all). What is left is a
   global.c allocator tie-break between two allocnos with identical ref counts
   and live ranges.
void func_8007EEE8(s32 arg) {
    s32 i;
    s32 limit;
    u8** pQueue;

    limit = g_GameState.unk1930[0];
    pQueue = HeapAlloc(0xC4, 1);
    D_800928EC = pQueue;
    D_80092888 = 0;
    for (i = 0; i < 0x31; i++) {
        if (arg == 0 || D_80092874[i].unk4 <= limit) {
            pQueue[D_80092888++] = (u8*)&D_80091964[i];
        }
    }
    if (arg != 0) {
        func_8008895C();
    }
}
 */

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007EFB4);
/* best 71.17%: structure, store order and the 7x7 iteration are right.
   The original keeps count in a2 and the six hoisted constants in t1-t6, saves
   no registers, and has a 0x48 frame with ra at 0x40 (8-byte-aligned frame, so
   0x30 of unreferenced local space); gcc spills count to s0, uses a 0x18 frame.
   Tried: count declared first/last, u16 count (worse, 69.33%).
typedef struct {
    0x00  u16 unk0;
    0x02  u16 unk2;
    0x04  u16 unk4;
    0x06  u16 unk6;
    0x08  u16 unk8;
    0x0A  u16 unkA;
    0x0C  u16 unkC;
    0x0E  u16 unkE;
    0x10  u16 unk10;
    0x12  u16 unk12;
} BattleSprite; // Size: 0x14

void func_8007EFB4(void) {
    s32 count = 0x1FF;
    BattleSprite* pSprite;
    s32 row;
    s32 col;

    pSprite = HeapAlloc(0x3D4, 0);
    D_8009270C = pSprite;
    for (row = 0; row < 7; row++) {
        for (col = 0; col < 7; col++) {
            pSprite->unk0 = 0x200;
            pSprite->unk2 = count--;
            pSprite->unk4 = 0x80;
            pSprite->unk6 = 1;
            pSprite->unk8 = 0x1A0 + row * 0x20;
            pSprite->unkA = col * 0x40;
            pSprite->unkC = 0x1E;
            pSprite->unkE = 0x40;
            pSprite++;
        }
    }
}
 */

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007F05C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007F258);

void func_8007F834(void) {
    D_80092740 = 0;
    D_8009273C = 0;
    D_80092744 = 0;
}
