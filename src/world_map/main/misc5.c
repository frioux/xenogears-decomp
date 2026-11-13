#include "common.h"
#include "psyq/libgte.h"

typedef struct {
    /* 0x00 */ u8 unk0[0x20];
    /* 0x20 */ u16 unk20;
    /* 0x22 */ u16 unk22;
    /* 0x24 */ u8 unk24[0x4];
    /* 0x28 */ u32 unk28;
    /* 0x2C */ u32 unk2C;
    /* 0x30 */ u32 unk30;
    /* 0x34 */ u8 unk34[0x4];
    /* 0x38 */ u32 unk38;
    /* 0x3C */ u32 unk3C;
    /* 0x40 */ u32 unk40;
    /* 0x44 */ u8 unk44[0xC];
    /* 0x50 */ u32 unk50;
    /* 0x54 */ u32 unk54;
    /* 0x58 */ u32 unk58;
    /* 0x5C */ u8 unk5C[0x24];
} WorldMapPlayer; // Size: 0x80

typedef struct {
    /* 0x00 */ s32 x;
    /* 0x04 */ s32 y;
    /* 0x08 */ s32 z;
    /* 0x0C */ s32 w;
} WorldMapPos; // Size: 0x10

typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ u8 unk2[0x16];
    /* 0x18 */ SVECTOR rot;
    /* 0x20 */ MATRIX mat;
    /* 0x40 */ u8 unk40[0x14];
} WorldMapRotBlock; // Size: 0x54

typedef struct {
    /* 0x000 */ u8 unk0[0x18];
    /* 0x018 */ SVECTOR unk18;
    /* 0x020 */ MATRIX unk20;
    /* 0x040 */ u8 unk40[0x308];
    /* 0x348 */ s16 unk348;
    /* 0x34A */ u8 unk34A[0x16];
    /* 0x360 */ SVECTOR unk360;
    /* 0x368 */ MATRIX unk368;
} WorldMapWork;

extern WorldMapPlayer *D_8009BE24;
extern WorldMapWork *D_8009C620;
extern u16 D_8009A4D8;
extern u16 D_8009A4E8[];
extern u16 D_8009BD38;
extern u16 D_8009BD3A;
extern u16 D_8009BD3C;
extern s32 D_8009BE0C;
extern s32 D_8009BE28;
extern s32 D_8009BE2C;
extern s32 D_8009BE30;
extern s32 D_8009C5AC;
extern s32 D_8009C5B0;
extern s32 D_8009C5B4;
extern s32 D_8009D3F0;
extern s32 D_8009D55C;
extern s32 D_8009D560;
extern s32 D_8009D564;
extern s32 D_8009D144;
extern u8 *D_8006259C;
extern s32 D_8009BC38;
extern s32 D_8009BCB0;
extern s32 D_8009BC3C;
extern s32 D_8009BCB4;
extern s32 D_8009C180;
extern s16 D_8006F94E;
extern s16 D_8006F950;
extern s16 D_8006F954;
extern s32 D_8009BBC4;
extern void func_800848B4(int, int);
extern void HeapFree(void *pMem);
extern void func_80039FF8(void);
extern void func_8003852C(void *p);
extern void func_80084818(void);
extern void func_80086568(void);
extern void func_800866C8(void);
extern void func_80074F04(void);
extern void func_800750DC(void);
extern void func_80088FF4(void);
extern void func_80089128(void);
extern void func_80097D64(void);
extern void func_800976A0(void);
extern void func_80093354(void *p);
extern s32 func_80093A5C(s32, s32);
extern void func_800894C8(s32);
extern void func_80089160(s32, void *, s32);
extern s16 D_8009A488[];
extern MATRIX D_8009A180;
extern void func_8007A06C(void *pBlock, s32 arg, s32 arg2);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007A9F8);
/* best 94.72%: everything matches except the position of the final
   sw to D_8009BE24[idx].unk50 -- retail leaves it last, gcc hoists it
   next to the addiu that materialises 0x1000. Tried: literal, pointer
   local, s32 locals, plain globals (89.35%), address computed mid-block.

int func_8007AD34(int idx) {
    WorldMapPlayer *pPlayer;
    s32 a;
    s32 b;
    s32 c;

    a = D_8009C5AC;
    b = D_8009C5B0;
    c = D_8009C5B4;
    D_8009BE0C = 0x78;
    D_8009D3F0 = 0x1C0000;
    D_8009BD38 = 0x40;
    D_8009BD3A = 0x480;
    D_8009BD3C = 0;
    pPlayer = D_8009BE24 + idx;
    D_8009BE28 = a;
    D_8009D55C = a;
    D_8009BE2C = b;
    D_8009D560 = b;
    D_8009BE30 = c;
    D_8009D564 = c;
    pPlayer->unk50 = 0x1000;
    return 1;
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007AD34);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007ADD4);
int func_8007B200(int idx) {
    u8 *pWork = (u8 *)D_8009C620;
    u8 *pBlock = pWork + 0x150;
    u8 *pSrc = pWork + 0x198;
    u8 *pPlayer = (u8 *)D_8009BE24 + (idx << 7);
    u32 *pScratch = (u32 *)0x1F800000;
    WorldMapRotBlock *pLast;
    s32 scale;
    s32 i;

    for (i = 0; i < 2; i++) {
        u32 arg1 = *(u32 *)pSrc;
        u16 arg2 = *(u16 *)(*(u32 *)(pSrc - 8) + 4);

        func_8007A06C(pBlock, arg1, arg2);
        pSrc += 0x54;
        pBlock += 0x54;
    }
    *(s32 *)(pPlayer + 0x54) = 0;
    *(s32 *)(pPlayer + 0x50) = 0;
    *(MATRIX *)&pScratch[0x3C] = D_8009A180;
    scale = *(s32 *)(pPlayer + 0x50);
    pScratch[1] = 0x1000;
    pScratch[2] = scale;
    pScratch[0] = scale;
    ScaleMatrix((MATRIX *)&pScratch[0x3C], (VECTOR *)pScratch);
    pLast = (WorldMapRotBlock *)pBlock;
    pLast->mat = *(MATRIX *)&pScratch[0x3C];
    pLast[1].mat = *(MATRIX *)&pScratch[0x3C];
    pLast[1].unk0 = 1;
    pLast->unk0 = 1;
    return 3;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007B394);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007B604);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007B798);

int func_8007BA08(void) {
    return 3;
}

int func_8007BA10(int idx) {
    u8 *pPlayer = (u8 *)D_8009BE24 + (idx << 7);
    s16 timer;
    int nResult = 1;

    if (*(s16 *)(pPlayer + 4) == 1) {
        *(s16 *)(pPlayer + 4) = 0;
        *(s16 *)(pPlayer + 0x22) = 0x3C;
        *(s32 *)(pPlayer + 0x28) = D_8009C5AC;
        *(s32 *)(pPlayer + 0x2C) = D_8009C5B0;
        *(s32 *)(pPlayer + 0x30) = D_8009C5B4;
    }
    timer = *(u16 *)(pPlayer + 0x22);
    timer--;
    *(s16 *)(pPlayer + 0x22) = timer;
    if (timer > 0) {
        *(s32 *)(pPlayer + 0x28) += -0x95D0;
        *(s32 *)(pPlayer + 0x30) += 0x2F130;
        *(u16 *)0x1F8000A0 = *(s32 *)(pPlayer + 0x28) >> 12;
        *(u16 *)0x1F8000A2 = *(s32 *)(pPlayer + 0x2C) >> 12;
        *(u16 *)0x1F8000A4 = *(s32 *)(pPlayer + 0x30) >> 12;
        func_80089160(9, (void *)0x1F8000A0, (s32)D_8009A488);
    } else {
        *(s16 *)(pPlayer + 0x22) = 0x3C;
        *(WorldMapPos *)(pPlayer + 0x28) = *(WorldMapPos *)&D_8009C5AC;
        func_800894C8(9);
        nResult = 3;
    }
    return nResult;
}

int func_8007BB60(int idx) {
    WorldMapWork *pWork;
    WorldMapPlayer *pPlayer;

    func_800848B4(0, 1);
    func_800848B4(0, 2);
    func_800848B4(0, 3);
    pWork = D_8009C620;
    pPlayer = D_8009BE24 + idx;
    pPlayer->unk50 = 0;
    pPlayer->unk54 = 0x100;
    pPlayer->unk58 = 0x100;
    pWork->unk18.vx = 0;
    pWork->unk18.vy = 0;
    pWork->unk18.vz = 0;
    RotMatrixYXZ(&pWork->unk18, &pWork->unk20);
    return 3;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007BBEC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007BF50);
void func_8007C260(void) {
    u16 v;

    func_80039FF8();
    func_8003852C(D_8006259C);
    HeapFree(D_8006259C);
    func_80084818();
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
    v = D_8009BD3A;
    D_8006F94E = 0x111;
    D_8006F954 = 2;
    D_8009BBC4 = 1;
    D_8006F950 = v;
}

/* best 95.26%: instruction multiset and registers identical to the original,
   only the emission order of the independent loads differs (gcc hoists
   lhu D_8009A4D8 and defers sll $4,$4,1). A pointer local instead gives
   65.26% with the same multiset, and a pointer local plus an s32 index
   local (i = pPlayer->unk50 before the D_8009A4D8 read) gives 49.68%.
*/
int func_8007C36C(int idx) {
    WorldMapPlayer *pPlayer = D_8009BE24;
    u32 *pCount;

    pPlayer += idx;
    pCount = &pPlayer->unk50;
    *pCount = 0;
    pPlayer->unk20 = D_8009A4D8;
    pPlayer->unk22 = D_8009A4E8[*pCount];
    *pCount = *pCount + 1;
    return 1;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007C3B8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007C724);
/* 44.62%: instruction multiset and size identical (180 bytes), only gcc sched2
   order differs. Retail puts the 0x7C store right after the addu and emits the
   three D_8009C5xx loads after it, and keeps li 0xffc0 late; gcc hoists the
   D_8009C5B0/B4 loads, sinks the 0x7C store next to the 0x58 store and hoists
   li 0xffc0 to the top. Tried: unk5C indexed vs a named unk7C field, s32
   locals for the three loads, plain globals (all three give the same 180-byte
   schedule), -O3 (41.22%). Same class as misc4 func_80077DC8 (70%) and misc6
   func_80080578 (34.97%).
int func_8007C724(int idx) {
    WorldMapPlayer *pPlayer;
    s32 a;
    s32 b;
    s32 c;

    D_8009BE0C = 0x78;
    D_8009D3F0 = 0x400000;
    pPlayer = D_8009BE24 + idx;
    D_8009D144 = 1;
    ((u32 *)pPlayer->unk5C)[8] = 0x1000;
    a = D_8009C5AC;
    b = D_8009C5B0;
    c = D_8009C5B4;
    D_8009BD38 = -0x40;
    D_8009BD3A = 0;
    D_8009BD3C = 0;
    D_8009BE28 = a;
    D_8009D55C = a;
    D_8009BE2C = b;
    D_8009D560 = b;
    D_8009BE30 = c;
    D_8009D564 = c;
    pPlayer->unk58 = 0x40;
    pPlayer->unk50 = 0;
    return 1;
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007C7D8);
int func_8007CC6C(int idx) {
    WorldMapWork *pWork;
    WorldMapPlayer *pPlayer;
    WorldMapRotBlock *pBlock;

    func_800848B4(4, 0);
    func_800848B4(4, 2);
    func_800848B4(4, 1);
    func_800848B4(4, 3);
    pWork = D_8009C620;
    pBlock = (WorldMapRotBlock *)&((u8 *)pWork)[0x150];
    pBlock->unk0 = 0;
    pBlock->rot.vx = pBlock->rot.vy = pBlock->rot.vz = 0;
    RotMatrixYXZ(&pBlock->rot, &pBlock->mat);
    pPlayer = D_8009BE24 + idx;
    pPlayer->unk40 = -0x4000;
    pPlayer->unk28 = 0xD00000;
    pPlayer->unk3C = 0;
    pPlayer->unk38 = 0;
    pPlayer->unk2C = 0;
    pPlayer->unk30 = 0x400000;
    return 1;
}
int func_8007CD20(int idx) {
    u8 *pPlayer = (u8 *)D_8009BE24 + (idx << 7);
    u8 *pBlock = (u8 *)D_8009C620 + 0x150;
    u16 *pScratch = (u16 *)0x1F800000;
    s16 mode;

    mode = *(s16 *)(pPlayer + 4);
    if (mode == 1) {
        *(s16 *)(pPlayer + 4) = 0;
        *(s16 *)(pPlayer + 0x20) = 1;
    } else if (mode == 2) {
        *(s16 *)(pPlayer + 4) = 0;
        *(s16 *)(pPlayer + 0x20) = 2;
    }
    *(s32 *)(pPlayer + 0x30) += *(s32 *)(pPlayer + 0x40);
    func_80093354(pPlayer + 0x28);
    *(s32 *)(pBlock + 8) = *(s32 *)(pPlayer + 0x28) >> 12;
    *(s32 *)(pBlock + 0xC) = *(s32 *)(pPlayer + 0x2C) >> 12;
    *(s32 *)(pBlock + 0x10) = *(s32 *)(pPlayer + 0x30) >> 12;
    pScratch[0x50] = *(s32 *)(pPlayer + 0x28) >> 12;
    pScratch[0x51] = *(s32 *)(pPlayer + 0x2C) >> 12;
    pScratch[0x52] = *(s32 *)(pPlayer + 0x30) >> 12;
    func_80089160(0x13, (void *)&pScratch[0x50], 0);
    switch (*(s16 *)(pPlayer + 0x20)) {
    case 1:
        func_80089160(0x1F, (void *)&pScratch[0x50], 0);
        /* fallthrough */
    case 0:
        D_8009D564 = *(s32 *)(pPlayer + 0x30);
        break;
    case 2:
        func_80089160(0x1F, (void *)&pScratch[0x50], 0);
        break;
    }
    return 1;
}
int func_8007CE84(int idx) {
    WorldMapWork *pWork;
    WorldMapPlayer *pPlayer;
    WorldMapRotBlock *pBlock;

    func_800848B4(5, 6);
    pWork = D_8009C620;
    pBlock = (WorldMapRotBlock *)&((u8 *)pWork)[0x1A4];
    pBlock->unk0 = 0;
    pBlock->rot.vx = pBlock->rot.vy = pBlock->rot.vz = 0;
    RotMatrixYXZ(&pBlock->rot, &pBlock->mat);
    pPlayer = D_8009BE24 + idx;
    pPlayer->unk40 = -0x4000;
    pPlayer->unk28 = 0xB00000;
    pPlayer->unk20 = 0;
    pPlayer->unk3C = 0;
    pPlayer->unk38 = 0;
    pPlayer->unk2C = 0;
    pPlayer->unk30 = 0x200000;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007CF18);
/* best 75.19% (356 bytes, size exact): the two `if` blocks and every store
   are right, but gcc CSEs the three scratchpad addresses (0x1F8000A0/A2/A4,
   each used once per block) into s3/s4/s2 across both calls -- retail
   re-materialises them as absolute `lui $at / sh` for all but one, so our
   frame is 0x30 not 0x28 and s5 replaces retail's single s3-held constant 1.
   Tried: volatile on the six stores (76.84%, 364 bytes), keeping the
   literal 1 in each comparison. The same six stores in func_8007D2B8 (this
   file, matched) do compile to the $at form, so this is an allocator tie
   triggered by the extra `0x20` test, not a source-shape error.
int func_8007CF18(int idx) {
    u8 *pPlayer = (u8 *)D_8009BE24 + (idx << 7);
    u8 *pWork = (u8 *)D_8009C620;

    if (*(s16 *)(pPlayer + 4) == 1) {
        *(s16 *)(pPlayer + 4) = 0;
        *(s16 *)(pPlayer + 0x20) = 1;
    }
    *(s32 *)(pPlayer + 0x30) += *(s32 *)(pPlayer + 0x40);
    func_80093354(pPlayer + 0x28);
    *(s32 *)(pPlayer + 0x2C) = func_80093A5C(*(s32 *)(pPlayer + 0x28), *(s32 *)(pPlayer + 0x30)) - 0x4000;
    *(s32 *)(pWork + 0x1AC) = *(s32 *)(pPlayer + 0x28) >> 12;
    *(s32 *)(pWork + 0x1B0) = *(s32 *)(pPlayer + 0x2C) >> 12;
    *(s32 *)(pWork + 0x1B4) = *(s32 *)(pPlayer + 0x30) >> 12;
    *(u16 *)0x1F8000A0 = *(s32 *)(pPlayer + 0x28) >> 12;
    *(u16 *)0x1F8000A2 = *(s32 *)(pPlayer + 0x2C) >> 12;
    *(u16 *)0x1F8000A4 = *(s32 *)(pPlayer + 0x30) >> 12;
    func_80089160(0x15, (void *)0x1F8000A0, 0);
    if (*(s16 *)(pPlayer + 0x20) == 1) {
        *(u16 *)0x1F8000A0 = *(s32 *)(pPlayer + 0x28) >> 12;
        *(u16 *)0x1F8000A2 = *(s32 *)(pPlayer + 0x2C) >> 12;
        *(u16 *)0x1F8000A4 = *(s32 *)(pPlayer + 0x30) >> 12;
        func_80089160(0x1E, (void *)0x1F8000A0, 0);
    }
    return 1;
}
*/
int func_8007D078(int idx) {
    WorldMapWork *pWork;
    WorldMapPlayer *pPlayer;
    WorldMapRotBlock *pBlock;

    func_800848B4(9, 7);
    func_800848B4(9, 8);
    pWork = D_8009C620;
    pBlock = (WorldMapRotBlock *)&((u8 *)pWork)[0x2F4];
    pBlock->unk0 = 0;
    pBlock->rot.vx = pBlock->rot.vy = pBlock->rot.vz = 0;
    RotMatrixYXZ(&pBlock->rot, &pBlock->mat);
    pPlayer = D_8009BE24 + idx;
    pPlayer->unk40 = -0x4000;
    pPlayer->unk3C = 0;
    pPlayer->unk38 = 0;
    pPlayer->unk28 = 0xC00000;
    pPlayer->unk2C = 0;
    pPlayer->unk30 = 0;
    return 1;
}
int func_8007D110(int idx) {
    u8 *pPlayer = (u8 *)D_8009BE24 + (idx << 7);
    u8 *pWork = (u8 *)D_8009C620;

    if (*(s16 *)(pPlayer + 4) == 1) {
        *(s16 *)(pPlayer + 4) = 0;
        *(s16 *)(pWork + 0x2F4) = 1;
        *(s16 *)(pWork + 0x24C) = 1;
        *(s16 *)(pWork + 0x2A0) = 1;
        func_800894C8(0x16);
        return 3;
    }
    *(s32 *)(pPlayer + 0x30) += *(s32 *)(pPlayer + 0x40);
    func_80093354(pPlayer + 0x28);
    *(s32 *)(pPlayer + 0x2C) = func_80093A5C(*(s32 *)(pPlayer + 0x28), *(s32 *)(pPlayer + 0x30)) - 0x4000;
    *(s32 *)(pWork + 0x2FC) = *(s32 *)(pPlayer + 0x28) >> 12;
    *(s32 *)(pWork + 0x300) = *(s32 *)(pPlayer + 0x2C) >> 12;
    *(s32 *)(pWork + 0x304) = *(s32 *)(pPlayer + 0x30) >> 12;
    *(u16 *)0x1F8000A0 = *(s32 *)(pPlayer + 0x28) >> 12;
    *(u16 *)0x1F8000A2 = *(s32 *)(pPlayer + 0x2C) >> 12;
    *(u16 *)0x1F8000A4 = *(s32 *)(pPlayer + 0x30) >> 12;
    func_80089160(0x16, (void *)0x1F8000A0, 0);
    return 1;
}

int func_8007D228(int idx) {
    WorldMapWork *pWork;
    WorldMapPlayer *pPlayer;

    func_800848B4(0xA, 0xB);
    pWork = D_8009C620;
    pWork->unk348 = 0;
    pWork->unk360.vx = pWork->unk360.vy = pWork->unk360.vz = 0;
    RotMatrixYXZ(&pWork->unk360, &pWork->unk368);
    pPlayer = D_8009BE24 + idx;
    pPlayer->unk40 = -0x4000;
    pPlayer->unk28 = 0xE00000;
    pPlayer->unk3C = 0;
    pPlayer->unk38 = 0;
    pPlayer->unk2C = 0;
    pPlayer->unk30 = 0x100000;
    return 1;
}

int func_8007D2B8(int idx) {
    u8 *pPlayer = (u8 *)D_8009BE24 + (idx << 7);
    u8 *pWork = (u8 *)D_8009C620;

    if (*(s16 *)(pPlayer + 4) == 1) {
        *(s16 *)(pPlayer + 4) = 0;
        *(s16 *)(pWork + 0x348) = 1;
        *(s16 *)(pWork + 0x39C) = 1;
        func_800894C8(0x17);
        *(u16 *)0x1F8000A0 = *(s32 *)(pPlayer + 0x28) >> 12;
        *(u16 *)0x1F8000A2 = *(s32 *)(pPlayer + 0x2C) >> 12;
        *(u16 *)0x1F8000A4 = *(s32 *)(pPlayer + 0x30) >> 12;
        func_80089160(0x1C, (void *)0x1F8000A0, 0);
        return 3;
    }
    *(s32 *)(pPlayer + 0x30) += *(s32 *)(pPlayer + 0x40);
    func_80093354(pPlayer + 0x28);
    *(s32 *)(pPlayer + 0x2C) = func_80093A5C(*(s32 *)(pPlayer + 0x28), *(s32 *)(pPlayer + 0x30)) - 0x4000;
    *(s32 *)(pWork + 0x350) = *(s32 *)(pPlayer + 0x28) >> 12;
    *(s32 *)(pWork + 0x354) = *(s32 *)(pPlayer + 0x2C) >> 12;
    *(s32 *)(pWork + 0x358) = *(s32 *)(pPlayer + 0x30) >> 12;
    *(u16 *)0x1F8000A0 = *(s32 *)(pPlayer + 0x28) >> 12;
    *(u16 *)0x1F8000A2 = *(s32 *)(pPlayer + 0x2C) >> 12;
    *(u16 *)0x1F8000A4 = *(s32 *)(pPlayer + 0x30) >> 12;
    func_80089160(0x17, (void *)0x1F8000A0, 0);
    return 1;
}
