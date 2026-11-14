#include "common.h"
#include "psyq/libgpu.h"
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
extern void WorldMapSetSpriteSetParent(int, int);
extern void HeapFree(void *pMem);
extern void SoundStopAllElements(void);
extern void SoundRemoveSedsEntry(void *p);
extern void WorldMapFreeSpriteSets(void);
extern void func_80086568(void);
extern void func_800866C8(void);
extern void WorldMapFreeTileAnimEntries(void);
extern void WorldMapFreeFrameAnimEntries(void);
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
extern MATRIX D_8009BE4C;
extern s32 D_8009CCA4;
extern s32 D_8009D3CC;
extern s32 D_8009D804;
extern void (*D_8009CD40)(void);
extern void func_80086700(void);
extern void WorldMapInitGfxEnv(void);
extern void func_80072DB4(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
extern s32 ArchiveDataSync(void);
extern void WorldMapDecompressMapData(void);
extern void func_8009766C(void);
extern void func_80098044(void);
extern s32 ArchiveCdDataSync(s32 arg0);
extern void WorldMapLoadSedsBank(void);
extern void func_80084580(void);
extern void func_8008440C(void);
extern void func_800979C8(void);
extern void func_800736DC(void);
extern void func_800863E0(void);
extern void WorldMapInitTileAnimEntries(void);
extern void WorldMapInitFrameAnimEntries(void);
extern void WorldMapSetupMapPrims(void);
extern void func_80088F64(void);
extern void SoundAddSedsEntry(void *pSeds);
extern s32 ArchiveSetIndex(s32 arg0, s32 arg1);
extern void func_80097BC0(void *pPos);
extern s32 WorldMapGetQueuedLoadCount(void);
extern void func_800967E4(void);
extern s32 Vsync(s32 arg0);
extern void func_80097718(void *p0, void *p1);
extern void func_800978FC(void);
extern void func_8008901C(void);
extern void func_800865A0(void);
extern void func_80075228(void);
extern void func_800923A8(void);
extern void func_800925A0(void);
extern int func_8007C36C(int idx);
extern int func_8007C3B8(int idx);
extern int func_8007C724(int idx);
extern int func_8007C7D8(int idx);
extern int func_8007CC6C(int idx);
extern int func_8007CD20(int idx);
extern int func_8007CE84(int idx);
extern int func_8007CF18(int idx);
extern int func_8007D078(int idx);
extern int func_8007D110(int idx);
extern int func_8007D228(int idx);
extern int func_8007D2B8(int idx);
extern void WorldMapInitGroup12Actor(void);
extern void WorldMapUpdateGroup12Actor(void);
extern void WorldMapInitGroup13Actor(void);
extern void WorldMapUpdateGroup13Actor(void);
extern void WorldMapInitGroup16Actor(void);
extern void WorldMapUpdateGroup16Actor(void);
extern void WorldMapEnterNop(void);
extern void WorldMapUpdateFrameWithParticles(void);
extern void func_8007A06C(void *pBlock, s32 arg, s32 arg2);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007A9F8);
int WorldMapInitActorScrollScale(int idx) {
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
    // volatile store pins the sw after the six copies, as retail has it
    *(volatile s32 *)&pPlayer->unk50 = 0x1000;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007ADD4);
int func_8007B200(int idx) {
    u8 *pPlayer = (u8 *)D_8009BE24 + (idx << 7);
    u8 *pWork = (u8 *)D_8009C620;
    u8 *pBlock = pWork + 0x150;
    u8 *pSrc = pWork + 0x198;
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
/* best 99.68%: 624 bytes, identical instruction multiset. Only the registers of the
   two D_8009C5AC values differ: retail keeps the >>12 result in v0 and the -0x40
   constant in v1, gcc gives the constant v0 (a global-alloc priority tie; 8 shapes
   tried, including a separate local per value, swapping the store order, and
   single-statement vs split shift). The vector block (0x1F800000/0x1F800010) needs
   its own locals x/z/w or gcc reloads the 0x54 field twice (the $at-addressed
   scratchpad store forces a reload) and the head allocation moves too.
int func_8007B394(int idx) {
    u8 *pPlayer = (u8 *)D_8009BE24 + (idx << 7);
    u8 *pWork = (u8 *)D_8009C620;
    u32 *pScratch = (u32 *)0x1F800000;
    s32 x;
    s32 y;
    s32 z;
    s32 w;

    if (*(s16 *)(pPlayer + 4) != 0) {
        *(s16 *)(pPlayer + 4) = 0;
        *(s16 *)(pWork + 0x1A4) = 0;
        *(s16 *)(pWork + 0x150) = 0;
    }
    x = D_8009C5AC >> 12;
    *(s32 *)(pWork + 0x1AC) = x;
    *(s32 *)(pWork + 0x158) = x;
    y = -0x40;
    *(s32 *)(pWork + 0x1B0) = y;
    *(s32 *)(pWork + 0x15C) = y;
    z = D_8009C5B4 >> 12;
    *(s32 *)(pWork + 0x1B4) = z;
    *(s32 *)(pWork + 0x160) = z;
    *(s32 *)(pPlayer + 0x50) += 0x180;
    if (*(s32 *)(pPlayer + 0x50) > 0x800) {
        *(s32 *)(pPlayer + 0x54) += 0x180;
    }
    if (*(s32 *)(pPlayer + 0x50) > 0x7F00) {
        *(s32 *)(pPlayer + 0x50) = 0x7F00;
    }
    if (*(s32 *)(pPlayer + 0x54) > 0x7F00) {
        *(s32 *)(pPlayer + 0x54) = 0x7F00;
    }
    *(MATRIX *)&pScratch[0x3C] = D_8009A180;
    *(MATRIX *)&pScratch[0x44] = *(MATRIX *)&pScratch[0x3C];
    w = *(s32 *)(pPlayer + 0x50);
    *(u32 *)0x1F800008 = w;
    *(u32 *)0x1F800000 = w;
    x = *(s32 *)(pPlayer + 0x54);
    *(u32 *)0x1F800014 = 0x1000;
    *(u32 *)0x1F800004 = 0x1000;
    *(u32 *)0x1F800018 = x;
    *(u32 *)0x1F800010 = x;
    ScaleMatrix((MATRIX *)0x1F8000F0, (VECTOR *)0x1F800000);
    ScaleMatrix((MATRIX *)0x1F800110, (VECTOR *)0x1F800010);
    *(MATRIX *)(pWork + 0x170) = *(MATRIX *)0x1F8000F0;
    *(MATRIX *)(pWork + 0x1C4) = *(MATRIX *)0x1F800110;
    return 1;
}
*/
int func_8007B604(int idx) {
    u8 *pPlayer = (u8 *)D_8009BE24 + (idx << 7);
    u8 *pWork = (u8 *)D_8009C620;
    u8 *pBlock = pWork + 0x1F8;
    u8 *pSrc = pWork + 0x240;
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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007B798);
/* best 99.68%: 624 bytes, identical instruction multiset. This is the +0xA8
   sibling of func_8007B394 (blocks at 0x1F8/0x24C instead of 0x150/0x1A4); the
   parked body below is that one with the offsets shifted. Same allocator tie:
   retail keeps the D_8009C5AC>>12 result in v0 and the -0x40 constant in v1,
   gcc always gives the constant v0. Everything else, including the two
   0x1F8000F0/0x1F800110 matrix copies and both ScaleMatrix calls, is exact.
int func_8007B798(int idx) {
    u8 *pPlayer = (u8 *)D_8009BE24 + (idx << 7);
    u8 *pWork = (u8 *)D_8009C620;
    u32 *pScratch = (u32 *)0x1F800000;
    s32 x;
    s32 y;
    s32 z;
    s32 w;

    if (*(s16 *)(pPlayer + 4) != 0) {
        *(s16 *)(pPlayer + 4) = 0;
        *(s16 *)(pWork + 0x24C) = 0;
        *(s16 *)(pWork + 0x1F8) = 0;
    }
    x = D_8009C5AC >> 12;
    *(s32 *)(pWork + 0x254) = x;
    *(s32 *)(pWork + 0x200) = x;
    y = -0x40;
    *(s32 *)(pWork + 0x258) = y;
    *(s32 *)(pWork + 0x204) = y;
    z = D_8009C5B4 >> 12;
    *(s32 *)(pWork + 0x25C) = z;
    *(s32 *)(pWork + 0x208) = z;
    *(s32 *)(pPlayer + 0x50) += 0x180;
    if (*(s32 *)(pPlayer + 0x50) > 0x800) {
        *(s32 *)(pPlayer + 0x54) += 0x180;
    }
    if (*(s32 *)(pPlayer + 0x50) > 0x7F00) {
        *(s32 *)(pPlayer + 0x50) = 0x7F00;
    }
    if (*(s32 *)(pPlayer + 0x54) > 0x7F00) {
        *(s32 *)(pPlayer + 0x54) = 0x7F00;
    }
    *(MATRIX *)&pScratch[0x3C] = D_8009A180;
    *(MATRIX *)&pScratch[0x44] = *(MATRIX *)&pScratch[0x3C];
    w = *(s32 *)(pPlayer + 0x50);
    *(u32 *)0x1F800008 = w;
    *(u32 *)0x1F800000 = w;
    x = *(s32 *)(pPlayer + 0x54);
    *(u32 *)0x1F800014 = 0x1000;
    *(u32 *)0x1F800004 = 0x1000;
    *(u32 *)0x1F800018 = x;
    *(u32 *)0x1F800010 = x;
    ScaleMatrix((MATRIX *)0x1F8000F0, (VECTOR *)0x1F800000);
    ScaleMatrix((MATRIX *)0x1F800110, (VECTOR *)0x1F800010);
    *(MATRIX *)(pWork + 0x218) = *(MATRIX *)0x1F8000F0;
    *(MATRIX *)(pWork + 0x26C) = *(MATRIX *)0x1F800110;
    return 1;
}
*/

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

    WorldMapSetSpriteSetParent(0, 1);
    WorldMapSetSpriteSetParent(0, 2);
    WorldMapSetSpriteSetParent(0, 3);
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
/* best 97.91% (784 bytes, size exact): every instruction of this long init
   sequence is right -- the two loops, the 32-byte D_8009A180 -> D_8009BE4C
   copy, all twenty func_80097718 pairs -- except the two-insn address setup for
   D_8009C5AC (lui/addiu into $s0). Retail materialises it just before the store
   at 0x783C and then reuses $s0 for the func_80097BC0 argument at 0x78C0; gcc
   only allocates the register when the pointer local is initialised at the top
   of the function, and then it emits the lui/addiu there instead.
   With the assignment at retail's position gcc folds both uses to %hi/%lo
   (97.22%): `lui v0,0xD00000; lui at,%hi; sw v0,0(at)` plus a fresh
   `lui a0/addiu a0` for the call. Tried, all still folding: a pPos local used
   for both the store and the call, (s32*)((u32)&D_8009C5AC), a volatile store.
   The matched WorldMapInitializeMode in misc3 gets $s0 at the assignment point
   only because it declares `extern u8 D_8009C5AC[]` and casts the array to a
   pointer -- that spelling is not available here, the matched functions of this
   unit read D_8009C5AC as an s32. gcc 2.7.2-cdk scores 78.67%, so no preset
   lead.
void func_8007BF50(void) {
    RECT rect;
    s32 *pPos = &D_8009C5AC;

    WorldMapInitGfxEnv();
    rect.x = 0;
    rect.y = 0;
    rect.w = 0x140;
    rect.h = 0xD8;
    MoveImage(&rect, 0x2C0, 0x100);
    DrawSync(0);
    func_80072DB4(0x40, 0, 4, 2);
    while (ArchiveDataSync() >= 3) {
    }
    WorldMapDecompressMapData();
    func_8009766C();
    D_8009BE4C = D_8009A180;
    D_8009CCA4 = 2;
    D_8009D3CC = 0x10;
    D_8009D804 = 0;
    D_8009D144 = 0;
    D_8009CD40 = func_80086700;
    func_80098044();
    ArchiveCdDataSync(0);
    WorldMapLoadSedsBank();
    *pPos = 0xD00000;
    D_8009C5B0 = 0xFFF60000;
    D_8009C5B4 = 0x400000;
    func_80084580();
    func_8008440C();
    func_800979C8();
    func_800736DC();
    func_800863E0();
    WorldMapInitTileAnimEntries();
    WorldMapInitFrameAnimEntries();
    WorldMapSetupMapPrims();
    func_80088F64();
    ArchiveCdDataSync(0);
    SoundAddSedsEntry(D_8006259C);
    ArchiveSetIndex(0x24, 0);
    func_80097BC0(pPos);
    do {
        func_800967E4();
        Vsync(0);
    } while (WorldMapGetQueuedLoadCount() > 0);
    func_80097718((void *)func_800923A8, (void *)func_800925A0);
    func_80097718((void *)func_8007C36C, (void *)func_8007C3B8);
    func_80097718((void *)func_8007C724, (void *)func_8007C7D8);
    func_80097718((void *)func_8007CC6C, (void *)func_8007CD20);
    func_80097718((void *)func_8007CE84, (void *)func_8007CF18);
    func_80097718((void *)func_8007D078, (void *)func_8007D110);
    func_80097718((void *)func_8007D228, (void *)func_8007D2B8);
    func_80097718((void *)WorldMapInitGroup12Actor, (void *)WorldMapUpdateGroup12Actor);
    func_80097718((void *)WorldMapInitGroup13Actor, (void *)WorldMapUpdateGroup13Actor);
    func_80097718((void *)WorldMapInitGroup16Actor, (void *)WorldMapUpdateGroup16Actor);
    func_80097718((void *)WorldMapEnterNop, (void *)WorldMapUpdateFrameWithParticles);
    func_800978FC();
    func_8008901C();
    func_800865A0();
    func_80075228();
}
*/
void func_8007C260(void) {
    u16 v;

    SoundStopAllElements();
    SoundRemoveSedsEntry(D_8006259C);
    HeapFree(D_8006259C);
    WorldMapFreeSpriteSets();
    func_80086568();
    func_800866C8();
    WorldMapFreeTileAnimEntries();
    WorldMapFreeFrameAnimEntries();
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

    WorldMapSetSpriteSetParent(4, 0);
    WorldMapSetSpriteSetParent(4, 2);
    WorldMapSetSpriteSetParent(4, 1);
    WorldMapSetSpriteSetParent(4, 3);
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

    WorldMapSetSpriteSetParent(5, 6);
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

    WorldMapSetSpriteSetParent(9, 7);
    WorldMapSetSpriteSetParent(9, 8);
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

    WorldMapSetSpriteSetParent(0xA, 0xB);
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
