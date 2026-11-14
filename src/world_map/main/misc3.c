#include "common.h"
#include "psyq/libetc.h"
#include "psyq/libgpu.h"

extern s32 D_8009D554;
extern s32 D_8009D7CC;
extern s32 D_8009CCA4;
extern s32 D_8009D3CC;

extern void func_800894C8(s32 arg0);
extern void func_80089514(s32 arg0);
extern void func_80097770(s32 arg0, s32 arg1);
extern s32* D_80062528;

extern void SoundSetAudioManagerVolumeWithFade(s32* pManager);
extern void func_80089160(s32 arg0, void* arg1, s32 arg2);
extern void SoundPlaySoundEffectOnFreePair(s32 arg0);
extern void func_8003A3B8(s32 arg0, s32 arg1, s32 arg2);

extern u8 D_8009C5AC[];
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

extern void WorldMapBuildRotationMatrix(s32* p);
extern void func_80097244(s32* p);
extern void func_80089748(void);
extern void func_80089C78(void);
extern void func_8008615C(void);
extern void func_800848F4(void);
extern void func_800980D4(s32* p);
extern void func_800981C8(s32* p);
extern void WorldMapWaitForLoadSlotFree(void);
extern void func_80098CC0(void);
extern void func_800983A0(s32* p);
extern void func_8009932C(s32 arg0, s32 arg1, s32* p);
extern void func_80073B04(void);
extern void func_800737EC(void);
extern void func_80086798(void);
extern void WorldMapFreeSpriteSets(void);
extern void func_80086124(void);
extern void func_80086568(void);
extern void func_800866C8(void);
extern void WorldMapFreeTileAnimEntries(void);
extern void WorldMapFreeFrameAnimEntries(void);
extern void func_80088FF4(void);
extern void func_80089128(void);
extern void func_80097D64(void);
extern void func_800976A0(void);
extern void HeapFree(void* pMem);

typedef s32 (*WorldMapScriptHandler)(u8* pSlot, s32 arg1, s16 arg2, s16 arg3);
extern WorldMapScriptHandler D_8009A3C0[];

// World map player (stride 0x80), the scroll fields func_80076DA4 and
// func_80076FA8 touch. Same layout as misc12.c's local WorldMapPlayer.
typedef struct {
    /* 0x00 */ u8 unk0[0x28];
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ s32 unk34;
    /* 0x38 */ s32 unk38;
    /* 0x3C */ s32 unk3C;
    /* 0x40 */ s32 unk40;
    /* 0x44 */ u8 unk44[0xC];
    /* 0x50 */ s32 unk50;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ s32 unk5C;
} WorldMapPlayer; // Size: 0x60

// Four-word position record; misc12.c copies the same bytes through a Quad.
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 w;
} WorldMapPos4; // Size: 0x10

// Scroll delta buffer: the wrapped xyz offset at 0x00 and its 1/8 step at 0x10.
typedef struct {
    /* 0x00 */ s32 x;
    /* 0x04 */ s32 y;
    /* 0x08 */ s32 z;
    /* 0x0C */ s32 pad0C;
    /* 0x10 */ s32 stepX;
    /* 0x14 */ s32 stepY;
    /* 0x18 */ s32 stepZ;
} WorldMapScrollDelta; // Size: 0x1C

// Eight name bytes swapped through the scratchpad by func_8007756C; the type
// is byte-aligned so gcc copies it with lwl/lwr.
typedef struct {
    /* 0x0 */ u8 name[8];
} WorldMapName8; // Size: 0x8

extern void WorldMapWrapDelta(WorldMapScrollDelta* pDelta);
extern void func_80096F18(void* pOut, s32* pSrc, s32 nHeight);
extern s32 D_8009BE0C;
extern s32 D_8009BD48;
extern s32 D_8009BE2C;
extern s32 D_8009BE30;
extern s32 D_8009D55C;
extern s32 D_8009D560;
extern s32 D_8009D564;
extern s32 D_8009BBBC;
extern s16 D_8009BD38;
extern s16 D_8009BD3A;
extern s16 D_8009BD3C;

s32 WorldMapUpdateFrame(void) {
    u8* p;
    s32* pState;

    if (D_8009D144 == 0) {
        WorldMapBuildRotationMatrix(&D_8009BD40);
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
        WorldMapWaitForLoadSlotFree();
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

s32 WorldMapRunSlotScript(s32 arg0) {
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

s32 WorldMapScriptOp00End(void) {
    D_8009D554 = 0;
    D_8009D7CC = 0;
    return 0;
}

s32 WorldMapScriptOp01WaitFrames(u8* p, s32 arg1) {
    s16 timer = *(s16*)(p + 0x22);

    if (timer == 0) {
        *(s16*)(p + 0x22) = arg1;
        return 0;
    }
    timer--;
    *(s16*)(p + 0x22) = timer;
    return (timer < 1) << 1;
}

s32 WorldMapScriptOp02StartSlot(s32 arg0, s32 arg1, s32 arg2) {
    func_80097770(arg1, arg2);
    return 4;
}

s32 WorldMapScriptOp03SetPosition(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    *(s32*)D_8009C5AC = arg1 << 12;
    D_8009C5B0 = arg2 << 12;
    D_8009C5B4 = arg3 << 12;
    return 4;
}

s32 WorldMapScriptOp04SetScratchPosition(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    *(u16*)0x1F8000A0 = arg1;
    *(u16*)0x1F8000A2 = arg2;
    *(u16*)0x1F8000A4 = arg3;
    return 4;
}

s32 WorldMapScriptOp05ActivateSpriteGroup(s32 arg0, s32 arg1) {
    func_80089160(arg1, (void*)0x1F8000A0, 0);
    return 2;
}

s32 WorldMapScriptOp06DeactivateSpriteGroup(s32 arg0, s32 arg1) {
    func_800894C8(arg1);
    return 2;
}

s32 WorldMapScriptOp07ClearGroupEntries(s32 arg0, s32 arg1) {
    func_80089514(arg1);
    return 2;
}

s32 WorldMapScriptOp08FadeMasterVolume(void) {
    SoundSetAudioManagerVolumeWithFade(D_80062528);
    return 4;
}

s32 WorldMapScriptOp09PlaySound(s32 arg0, s32 arg1) {
    SoundPlaySoundEffectOnFreePair((*(u16*)((u8*)D_8006259C + 0x14) << 16) | arg1);
    return 2;
}

s32 WorldMapScriptOp10FadeSound(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    func_8003A3B8((*(u16*)((u8*)D_8006259C + 0x14) << 16) | arg1, arg2, arg3);
    return 4;
}

s32 WorldMapScriptOp11SetDrawMode(s32 arg0, s32 arg1, s32 arg2) {
    D_8009CCA4 = arg1;
    D_8009D3CC = arg2;
    return 4;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_80076DA4);
/* Best attempt: 96.52% (420 bytes, same size). The compare block, the three
   abs-step blocks and the frame are byte-identical. Only the last three
   halfword stores differ: the original emits them fully sequentially (lw 0x38,
   nop, sra, sh D_8009BD38; lw 0x3C, nop, sra, sh D_8009BD3A; lw 0x40, nop, sra,
   sh D_8009BD3C) while gcc hoists one load one slot early. gcc emits the three
   stores in source order 1,3,2, so writing them 0x38/0x40/0x3C (below) is what
   reproduces the original's 38/3A/3C order. Also tried: natural order (stores
   come out 38/3C/3A), s32 locals, s16 locals, the interleaved per-value form,
   and volatile globals (94.68%: fixes the store order, the hoist stays).
   Shortcut: one scheduler slot; do not retry these shapes.
void func_80076DA4(WorldMapPlayer* pPlayer, WorldMapScrollDelta* pDelta) {
    if ((pPlayer->unk38 != pPlayer->unk50) | (pPlayer->unk3C != pPlayer->unk54) |
        (pPlayer->unk40 != pPlayer->unk58)) {
        pDelta->x = pPlayer->unk50 - pPlayer->unk38;
        pDelta->y = pPlayer->unk54 - pPlayer->unk3C;
        pDelta->z = pPlayer->unk58 - pPlayer->unk40;
        WorldMapWrapDelta(pDelta);
        pDelta->stepX = pDelta->x >> 3;
        pDelta->stepY = pDelta->y >> 3;
        pDelta->stepZ = pDelta->z >> 3;
        if ((pDelta->stepX < 0 ? -pDelta->stepX : pDelta->stepX) < 0x40) {
            pPlayer->unk38 = pPlayer->unk50;
        } else {
            pPlayer->unk38 += pDelta->stepX;
        }
        if ((pDelta->stepY < 0 ? -pDelta->stepY : pDelta->stepY) < 0x40) {
            pPlayer->unk3C = pPlayer->unk54;
        } else {
            pPlayer->unk3C += pDelta->stepY;
        }
        if ((pDelta->stepZ < 0 ? -pDelta->stepZ : pDelta->stepZ) < 0x40) {
            pPlayer->unk40 = pPlayer->unk58;
        } else {
            pPlayer->unk40 += pDelta->stepZ;
        }
    }
    D_8009BD38 = pPlayer->unk38 >> 12;
    D_8009BD3C = pPlayer->unk40 >> 12;
    D_8009BD3A = pPlayer->unk3C >> 12;
}
*/

void WorldMapApproachHeight(u8* p) {
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
/* Best attempt: 75.16%. Same skeleton as func_80076DA4 (compare the three
   player fields against the D_8009D55C/560/564 goal, wrap the delta through
   WorldMapWrapDelta, 1/8 step, |step| < 0x200 snaps to the goal). Remaining diff:
   inside the if-block gcc cse reuses the D_8009D55C/560/564 values it loaded
   for the compare (in a2/a3/a1), while the original reloads D_8009D560 and
   D_8009D564 plus the player fields, so everything after shifts by two
   instructions. The four-word copy at the end is WorldMapPos4.
void func_80076FA8(WorldMapPlayer* pPlayer, WorldMapScrollDelta* pDelta) {
    if ((pPlayer->unk28 != D_8009D55C) | (pPlayer->unk2C != D_8009D560) |
        (pPlayer->unk30 != D_8009D564)) {
        pDelta->x = D_8009D55C - pPlayer->unk28;
        pDelta->y = D_8009D560 - pPlayer->unk2C;
        pDelta->z = D_8009D564 - pPlayer->unk30;
        WorldMapWrapDelta(pDelta);
        pDelta->stepX = pDelta->x >> 3;
        pDelta->stepY = pDelta->y >> 3;
        pDelta->stepZ = pDelta->z >> 3;
        if ((pDelta->stepX < 0 ? -pDelta->stepX : pDelta->stepX) < 0x200) {
            D_8009BBB4 += pDelta->x;
            pPlayer->unk28 = D_8009D55C;
        } else {
            D_8009BBB4 += pDelta->stepX;
            pPlayer->unk28 += pDelta->stepX;
        }
        if ((pDelta->stepY < 0 ? -pDelta->stepY : pDelta->stepY) < 0x200) {
            pPlayer->unk2C = D_8009D560;
        } else {
            pPlayer->unk2C += pDelta->stepY;
        }
        if ((pDelta->stepZ < 0 ? -pDelta->stepZ : pDelta->stepZ) < 0x200) {
            D_8009BBBC += pDelta->z;
            pPlayer->unk30 = D_8009D564;
        } else {
            D_8009BBBC += pDelta->stepZ;
            pPlayer->unk30 += pDelta->stepZ;
        }
    }
    *(WorldMapPos4*)&D_8009BE28 = *(WorldMapPos4*)&pPlayer->unk28;
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_800771D8);
/* Best attempt: 99.67%. Only difference: the original negates the local
   (negu v1,v1) where gcc's cse substitutes the parameter (negu v1,a2),
   although the move v1,a2 copy itself is emitted in the right place. Also
   tried: the ternary form (arg2 < 0) ? -arg2 : arg2 (same code), the
   limit = arg2 assign moved before the first abs (move lands in the wrong
   delay slot, 91.67%), 0 - limit, an arg2 test instead of a limit test.
   Preset screen: cdk 99.67, gcc-2.6.0 fails to compile. This is not a
   compiler-version or register-allocation tie: fold_rtx substitutes the copy
   into the negate under both 2.7.2 presets, so the source shape that keeps
   negu v1,v1 has not been found.
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

extern MATRIX D_8009A180;
extern MATRIX D_8009BE4C;
extern s32 D_8009D804;
extern void (*D_8009CD40)(void);
extern void func_80086700(void);
extern void WorldMapInitGfxEnv(void);
extern void func_80072DB4(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
extern s32 ArchiveDataSync(void);
extern void WorldMapDecompressMapData(void);
extern void func_80073530(void);
extern void func_8009766C(void);
extern void func_80098044(void);
extern s32 ArchiveCdDataSync(s32 arg0);
extern void WorldMapLoadSedsBank(void);
extern void func_8008440C(void);
extern void func_80084580(void);
extern void func_800979C8(void);
extern void func_800736DC(void);
extern void func_80085F58(void);
extern void func_80085FE0(void);
extern void func_800863E0(void);
extern void WorldMapInitTileAnimEntries(void);
extern void WorldMapInitFrameAnimEntries(void);
extern void WorldMapSetupMapPrims(void);
extern void func_80088F64(void);
extern void SoundAddSedsEntry(void* pSeds);
extern s32 ArchiveSetIndex(s32 arg0, s32 arg1);
extern void func_80097718(void* p0, void* p1);
extern void func_800978FC(void);
extern void func_8008901C(void);
extern void func_800865A0(void);
extern void func_80075228(void);
extern void func_800923A8(void);
extern void func_800925A0(void);
extern void func_80087710(void);
extern void func_80087734(void);
extern void WorldMapModeInitNop(void);
extern void WorldMapUpdateFrameWithSlotProjection(void);
extern void WorldMapEnterNop(void);
extern void WorldMapUpdateFrameWithParticles(void);
extern void WorldMapInitActorAtScrollPosition(void);
extern void func_800783E8(void);
extern void func_80077DC8(void);
extern void func_80077E68(void);
extern void func_80097BC0(void* p);
extern void func_800967E4(void);
extern s32 WorldMapGetQueuedLoadCount(void);
extern s32 func_8007756C(s32 arg0);
extern s32 func_800776E0(s32 arg0);

void WorldMapInitializeMode(void) {
    RECT rect;
    s32* pPos;

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
    func_80073530();
    func_8009766C();
    D_8009BE4C = D_8009A180;
    D_8009CCA4 = 2;
    D_8009D3CC = 4;
    D_8009D804 = 0;
    D_8009D144 = 0;
    D_8009CD40 = func_80086700;
    func_80098044();
    ArchiveCdDataSync(0);
    pPos = (s32*)D_8009C5AC;
    *pPos = 0x7702000;
    D_8009C5B0 = 0xFFD00000;
    D_8009C5B4 = 0x27C0000;
    func_80084580();
    func_8008440C();
    func_800979C8();
    func_800736DC();
    func_80085F58();
    func_800863E0();
    WorldMapInitTileAnimEntries();
    WorldMapInitFrameAnimEntries();
    WorldMapSetupMapPrims();
    func_80088F64();
    ArchiveSetIndex(0x24, 0);
    func_80097BC0(pPos);
    do {
        func_800967E4();
        Vsync(0);
    } while (WorldMapGetQueuedLoadCount() > 0);
    func_80097718((void*)func_800923A8, (void*)func_800925A0);
    func_80097718((void*)func_8007756C, (void*)func_800776E0);
    func_80097718((void*)func_80087710, (void*)func_80087734);
    func_80097718((void*)WorldMapModeInitNop, (void*)WorldMapUpdateFrameWithSlotProjection);
    func_800978FC();
    func_8008901C();
    func_800865A0();
    func_80085FE0();
    func_80075228();
    func_80089160(0xE, NULL, 0);
}

s32 WorldMapExitMode(void) {
    u16 v;

    WorldMapFreeSpriteSets();
    func_80086124();
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
    v = *(u16*)&D_8009BD3A;
    D_8006F94E = 0x11;
    D_8006F954 = 7;
    D_8009BBC4 = 1;
    D_8006F950 = v;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_8007756C);
/* Best attempt: 74.59% (380 bytes vs 368). Structure decoded and right: the
   16-byte copy pSlot+0x28 -> pSlot+0x38 is one struct assignment (lw x4 then
   sw x4) and the three 8-byte copies around 0x1F8000A0 swap D_8009BD40 with
   D_8009BD48 through the scratchpad (needs a 1-byte-aligned struct, which is
   what gives lwl/lwr). D_8009BD3A must be s16 (the original does lh). Four
   remaining diffs: (1) the original keeps &D_8009BD40 in $s0 across the call
   and both copies, gcc rematerialises lui/addiu, and a WorldMapName8 pointer
   local is folded straight back to the symbol; (2) retail reloads
   D_8009BD3A after storing 0x200 to it, gcc forwards the constant (a
   volatile cast forces the load but then emits lhu plus sll/sra for the
   shift and costs 12 bytes); (3) the single D_8009BD38 store goes through
   $a3 (lui+addiu) in the original and through $at here; (4) retail stores
   D_8009BE28 through the $a1 that is also the call argument, then BE30,
   BE2C, where gcc uses $at for all three and orders BE30 before BE28.
   Preset screen: gcc-2.7.2 -G0 scores 85.30 and leaves all 18 already
   matched functions in this unit at 100.00 (nothing in this unit is
   gp-relative: every symbol is an absolute from undefined_syms_auto), so a
   -G0 switch would not break the unit but still does not close it.
s32 func_8007756C(s32 arg0) {
    u8* pSlot = (u8*)D_8009BE24 + (arg0 << 7);
    WorldMapName8* pName = (WorldMapName8*)&D_8009BD40;

    D_8009BD38 = -0x80;
    D_8009BD3A = 0x200;
    D_8009D3F0 = 0x400000;
    D_8009BD3C = 0;
    *(s32*)(pSlot + 0x28) = 0xFFF80000;
    D_8009BE0C = 0x78;
    *(s32*)(pSlot + 0x2C) = *(volatile s16*)&D_8009BD3A << 12;
    *(WorldMapPos4*)(pSlot + 0x38) = *(WorldMapPos4*)(pSlot + 0x28);
    D_8009D144 = 0;
    D_8009BE28 = D_8009C5AC;
    D_8009BE2C = D_8009C5B0;
    D_8009BE30 = D_8009C5B4;
    func_80096F18(pName, &D_8009BE28, D_8009D3F0);
    *(WorldMapName8*)0x1F8000A0 = *pName;
    *pName = *(WorldMapName8*)&D_8009BD48;
    *(WorldMapName8*)&D_8009BD48 = *(WorldMapName8*)0x1F8000A0;
    return 1;
}
*/

extern u16 D_8009BD10;
extern u16 D_8009CD4C;
extern s32 D_8009BCDC;
extern s32 D_8009BD48;

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc3", func_800776E0);
/* Best attempt: 92.93% (632 bytes; retail is 628). Everything is decoded and
   every relocation is right. Retail keeps TWO callee-saved roles: s1 =
   (u8*)0x1F800000 (set by the first insn of the prologue, used for the two
   scratch deltas AND the two 0x1F8000A0 name words after the call) and s0 =
   &D_8009BD40 (loaded mid-function just before func_80096F18, then used for
   both lwl/lwr+swl/swr name swaps). Notes from the attempts below:
   - gcc only keeps 0x1F800000 in a saved register when the 0x1F8000A0 name
     access is spelled (pScratch + 0xA0) - a literal (WorldMapName8*)0x1F8000A0
     makes gcc rematerialise lui/ori and the base lands in t0.
   - the clamp is `if (nPos < -0x180000) ... else if (nPos > 0x18000) ...`,
     both stores through one register (retail reuses v1), and nPos must be a
     local (a0) - it also reproduces the branch-delay subu.
   - the four key tests reload D_8009CD4C each time (u16; 0x1000 add 0x8000,
     0x4000 sub 0x8000 on +0x28; 0x8004 add 0xFFFF0000 and 0x2008 add 0x10000
     on +0x2C). 0x8000 must stay a literal so gcc materialises it with
     ori v1,zero,0x8000 + addu.
   - the compare is `(unk38 != unk28) | (unk3C != unk2C)` (xor/sltu/or) and the
     body reloads 0x1F800000/0x1F800004 for the >>3 steps.
   - retail keeps &D_8009BD38 in a3 for the single `sh v0,0(a3)` while the
     D_8009BD3A store goes through %lo($at); not reproduced yet.
   - preset screen: 2 other functions in this unit match at 100 under the
     unit's XenoWorldMap preset, so the compiler is right.
   Open problem: with pName assigned right before the call gcc drops s1 (only
   s0 is saved, sp-24 frame); with pName initialised at the top it saves s1 but
   assigns the scratch base s0 and pName s1, the opposite of retail, and puts
   the BD40 la in the prologue. Nothing has produced s1 = scratch with s0 =
   pName.
*/

s32 WorldMapSlotEnterNop(void) {
    return 1;
}

s32 WorldMapUpdateFrameWithoutEffects(void) {
    u8* p;
    s32* pState;

    if (D_8009D144 == 0) {
        WorldMapBuildRotationMatrix(&D_8009BD40);
    } else {
        func_80097244(&D_8009BD40);
    }
    func_8008615C();
    func_800848F4();
    func_800980D4(&D_8009BBB4);
    if (D_8009D558 != 0) {
        func_800981C8(&D_8009BE28);
        WorldMapWaitForLoadSlotFree();
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
void WorldMapInitializeModeWithSound(void) {
    RECT rect;
    s32* pPos;

    WorldMapInitGfxEnv();
    rect.x = 0;
    rect.y = 0;
    rect.w = 0x140;
    rect.h = 0xD8;
    MoveImage(&rect, 0x2C0, 0x100);
    DrawSync(0);
    func_80072DB4(0x40, 0, 4, 1);
    while (ArchiveDataSync() >= 3) {
    }
    WorldMapDecompressMapData();
    func_8009766C();
    D_8009BE4C = D_8009A180;
    D_8009CCA4 = 1;
    D_8009D3CC = 4;
    D_8009D804 = 0;
    D_8009D144 = 0;
    D_8009CD40 = func_80086700;
    func_80098044();
    ArchiveCdDataSync(0);
    WorldMapLoadSedsBank();
    pPos = (s32*)D_8009C5AC;
    *pPos = 0x2000000;
    D_8009C5B0 = 0xFFE00000;
    D_8009C5B4 = 0x2000000;
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
    func_80097718((void*)func_800923A8, (void*)func_800925A0);
    func_80097718((void*)func_80077DC8, (void*)func_80077E68);
    func_80097718((void*)WorldMapInitActorAtScrollPosition, (void*)func_800783E8);
    func_80097718((void*)WorldMapEnterNop, (void*)WorldMapUpdateFrameWithParticles);
    func_800978FC();
    func_8008901C();
    func_800865A0();
}

/*
s32 func_800776E0(s32 arg0) {
    u8* pScratch = (u8*)0x1F800000;
    u8* pSlot = (u8*)D_8009BE24 + (arg0 << 7);
    WorldMapName8* pName;
    s32 nPos;

    if (D_8009BD10 & 0x40) {
        D_8009D554 = 0;
        D_8009D7CC = 0;
    }
    if (D_8009CD4C & 0x1000) {
        *(s32*)(pSlot + 0x28) += 0x8000;
    }
    if (D_8009CD4C & 0x4000) {
        *(s32*)(pSlot + 0x28) -= 0x8000;
    }
    nPos = *(s32*)(pSlot + 0x28);
    if (nPos < -0x180000) {
        *(s32*)(pSlot + 0x28) = -0x180000;
    } else if (nPos > 0x18000) {
        *(s32*)(pSlot + 0x28) = 0x18000;
    }
    if (D_8009CD4C & 0x8004) {
        *(s32*)(pSlot + 0x2C) += 0xFFFF0000;
    }
    if (D_8009CD4C & 0x2008) {
        *(s32*)(pSlot + 0x2C) += 0x10000;
    }
    if ((*(s32*)(pSlot + 0x38) != *(s32*)(pSlot + 0x28)) |
        (*(s32*)(pSlot + 0x3C) != *(s32*)(pSlot + 0x2C))) {
        *(s32*)pScratch = *(s32*)(pSlot + 0x28) - *(s32*)(pSlot + 0x38);
        *(s32*)(pScratch + 4) = *(s32*)(pSlot + 0x2C) - *(s32*)(pSlot + 0x3C);
        *(s32*)(pSlot + 0x38) += *(s32*)pScratch >> 3;
        *(s32*)(pSlot + 0x3C) += *(s32*)(pScratch + 4) >> 3;
    }
    D_8009BD38 = *(s32*)(pSlot + 0x38) >> 12;
    D_8009BD3A = *(s32*)(pSlot + 0x3C) >> 12;
    pName = (WorldMapName8*)&D_8009BD40;
    func_80096F18(pName, &D_8009BE28, D_8009D3F0);
    *(WorldMapName8*)(pScratch + 0xA0) = *pName;
    *pName = *(WorldMapName8*)&D_8009BD48;
    *(WorldMapName8*)&D_8009BD48 = *(WorldMapName8*)(pScratch + 0xA0);
    D_8009BD3A = (D_8009BD3A + 0x800) & 0xFFF;
    SetGeomScreen(D_8009BCDC);
    return 1;
}
*/
