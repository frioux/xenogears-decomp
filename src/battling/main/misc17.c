#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/inline_c.h"

#define BATTLE_SPU_ADDR_BASE 0x60000
#define BATTLE_SOUND_ID_NONE 0xFF
// The battle sound slots g_BattleSoundSlots holds, one element pair apiece.
#define BATTLE_SOUND_SLOT_COUNT 4
// What a slot's frame timer saturates at: BattleIncrementSoundSlotTimers stops it here.
#define BATTLE_SOUND_SLOT_TIMER_MAX 0xFFFF
// Half turn of the 12-bit angle space the actor's facing offset starts at.
#define BATTLE_ANGLE_HALF_TURN 0x800 // same define as battle/misc3.c and battle/misc4.c; battle actor.h is not includable here
// Denominator of a 0..255 gauge fraction (src/battling/main/misc2.c's BATTLE_GAUGE_FRACTION_SCALE).
#define BATTLE_GAUGE_FRACTION_SCALE 255 // same define as battle/misc2.c; battle actor.h is not includable here
// The battle effect image quad spans the whole screen (0x140 x 0xDA) and its
// texture points run up to (0x6F, 0x2A).
#define BATTLE_EFFECT_QUAD_WIDTH 0x140
#define BATTLE_EFFECT_QUAD_HEIGHT 0xDA
#define BATTLE_EFFECT_TEX_U_MAX 0x6F
#define BATTLE_EFFECT_TEX_V_MAX 0x2A
// The shade the effect is drawn at when it is not tinted: the quad is then drawn
// as a raw texture (bit 0 of the primitive code) instead of a coloured one.
#define BATTLE_EFFECT_SHADE_NEUTRAL 0x80

// gcc 2.7.2 does not merge adjacent byte/halfword stores, so the effect prim
// fields are written as the word stores its object has (the helpers
// src/battling/main/misc10.c uses for its own prims).
#define BattleSetXY0Fast(p, _x0, _y0) *(s32*)&(p)->x0 = (_x0) | ((_y0) << 0x10)
#define BattleSetXY1Fast(p, _x1, _y1) *(s32*)&(p)->x1 = (_x1) | ((_y1) << 0x10)
#define BattleSetXY2Fast(p, _x2, _y2) *(s32*)&(p)->x2 = (_x2) | ((_y2) << 0x10)
#define BattleSetXY3Fast(p, _x3, _y3) *(s32*)&(p)->x3 = (_x3) | ((_y3) << 0x10)
#define BattleSetWHFast(p, _w, _h) *(s32*)&(p)->w = (_w) | ((_h) << 0x10)
#define BattleSetUV0Fast(p, _u0, _v0) *(u16*)&(p)->u0 = (_u0) | ((_v0) << 0x8)
#define BattleSetUV1Fast(p, _u1, _v1) *(u16*)&(p)->u1 = (_u1) | ((_v1) << 0x8)
#define BattleSetUV2Fast(p, _u2, _v2) *(u16*)&(p)->u2 = (_u2) | ((_v2) << 0x8)
#define BattleSetUV3Fast(p, _u3, _v3) *(u16*)&(p)->u3 = (_u3) | ((_v3) << 0x8)
#define BattleSetRGBCFast(p, _r0, _g0, _b0, _code) \
    *(s32*)&(p)->r0 = (_r0) | ((_g0) << 0x8) | ((_b0) << 0x10) | ((_code) << 0x18)

// volatile: retail schedules this load after the argument setup, not before it.
extern volatile s32 g_SpuChannelMask;
extern s32 g_BattleEffectGrid;
extern s32 g_BattleEffectGridNext;
extern u_long* g_BattleEffectImage;
extern s32 g_BattleActorDistance2D;
extern u8 g_BattleAiAppliedBehaviorLevel;
extern u8 g_BattleDrawBufferIndex;
extern POLY_FT4 g_BattleEffectPolys[2];
extern TILE g_BattleEffectTiles[2];
extern DR_MODE g_BattleEffectDrawModes[2];
extern u8 g_BattleAiBehaviorLevel;
extern u8 g_BattlePlayer2ComControlled;

// The four-word sound data block a slot owns (a position, per
// BattleUpdateSoundSlotPositions) and that BattleAcquireSoundSlot copies in.
typedef struct {
    /* 0x0 */ u32 w[4];
} BattleSoundData; // Size: 0x10

typedef struct {
    /* 0x00 */ u8 soundId;
    /* 0x01 */ u8 active;
    /* 0x02 */ u8 elementIndex;
    /* 0x03 */ u8 flags;
    /* 0x04 */ u16 timer;
    /* 0x06 */ u16 pad6;
    /* 0x08 */ u32 spuAddr;
    /* 0x0C */ u32 elementMask;
    /* 0x10 */ BattleSoundData data;
    /* 0x20 */ u32* pData;
} BattleSlot; // Size: 0x24

struct BattleActor;

// The flags word at +0x2C of a BattleWork (see src/battling/main/misc18.c).
typedef union {
    /* 0x0 */ u32 flags;
    /* 0x0 */ struct {
        /* 0x0 */ u32 random : 8;
        /* 0x0 */ u32 unk2C_8 : 1;
        /* 0x0 */ u32 unk2C_9 : 1;
        /* 0x0 */ u32 unk2C_10 : 1;
        /* 0x0 */ u32 unk2C_11 : 1;
        /* 0x0 */ u32 unk2C_12 : 1;
        /* 0x0 */ u32 unk2C_13 : 1;
        /* 0x0 */ u32 pad2C : 18;
    } bits;
    /* 0x0 */ struct {
        /* 0x0 */ u16 pad2C;
        /* 0x2 */ u8 unk2E;
        /* 0x3 */ u8 pad2F;
    } bytes;
} BattleWorkFlags; // Size: 0x4

// The per-slot action state the battle main loop runs on. +0x04 counts down to
// the next state change and is stored as a byte elsewhere, so it is read
// through a u16 here and compared signed. The names are the ones the same
// struct carries in src/battling/main/misc18.c, which writes it.
typedef struct {
    /* 0x00 */ struct BattleActor* pOwner;
    /* 0x04 */ u16 delay;
    /* 0x06 */ u16 pad6;
    /* 0x08 */ u8 state;
    /* 0x09 */ u8 ticks;
    /* 0x0A */ s16 targetAngle;
    /* 0x0C */ s16 maxDistance;
    /* 0x0E */ u8 setFlag8000;
    /* 0x0F */ u8 difficulty;
    /* 0x10 */ s32 actionChance0;
    /* 0x14 */ s32 actionChance1;
    /* 0x18 */ s32 actionChance2;
    /* 0x1C */ s32 actionChance3;
    /* 0x20 */ s32 phase;
    /* 0x24 */ s32 breakThreshold;
    /* 0x28 */ s32 actionDistance;
    /* 0x2C */ BattleWorkFlags flags;
    /* 0x30 */ u16 savedMoveCount;
    /* 0x32 */ u16 pad32;
} BattleWork; // Size: 0x34

// A battle participant as this unit drives it. Same layout as the BattleActor
// in src/battling/main/misc18.c; the fields this unit does not read stay
// opaque.
typedef struct BattleActor {
    /* 0x000 */ u8 unk0[0x48];
    /* 0x048 */ s32 distance;
    /* 0x04C */ u8 unk4C[0xC];
    /* 0x058 */ s32 angle;
    /* 0x05C */ u8 unk5C[0x3C];
    /* 0x098 */ s32 stepForward;
    /* 0x09C */ s32 stepBack;
    /* 0x0A0 */ u8 unkA0[0x14];
    /* 0x0B4 */ s16 gaugeValue;
    /* 0x0B6 */ s16 gaugePoints;
    /* 0x0B8 */ u8 unkB8[0x4];
    /* 0x0BC */ s16 gaugeMax;
    /* 0x0BE */ s16 unkBE;
    /* 0x0C0 */ u8 unkC0[0xC];
    /* 0x0CC */ u16 sideBaseAngle;
    /* 0x0CE */ s16 facingOffset;
    /* 0x0D0 */ u32 flags;
    /* 0x0D4 */ u8 unkD4[0x835];
    /* 0x909 */ u8 typeId;
    /* 0x90A */ u8 unk90A[0xCF2];
    /* 0x15FC */ BattleWork* pWork;
    /* 0x1600 */ u8 unk1600[0x64];
    /* 0x1664 */ u8* pSoundTable;
    /* 0x1668 */ u16 moveCount;
} BattleActor; // Size: 0x166A

extern BattleSlot g_BattleSoundSlots[];

extern u16 D_80096EA4;
extern u16 D_80096EC8;
extern u16 D_80096EEC;
extern u16 D_80096F10;

extern u8 D_80091EE0[];
extern u8 g_BattleSoundTableType1D[];
extern u8 g_BattleSoundTableType24[];
extern u8 g_BattleSoundTableType1B[];
extern u8 g_BattleSoundTableDefault[];
extern u8 g_BattleSoundTableType9[];

extern void BattleQueueMessage(BattleActor*, s32);
extern void BattleTryQueueMessage5(BattleActor*, BattleWork*);
extern void BattleAcquireSoundSlot(u8*, u32, u8*, u32);
extern s32 func_8008B650(s32, s16, s16);
extern void SoundStopAllElements(void);
extern u32 SoundGetActiveElementMask(u32);
extern void SoundStopElementsByIndex(u8);
extern void BattleQueueRandomMessages(BattleActor*, BattleWork*, s32);
extern void BattleDispatchWorkState(BattleActor*);
extern void SoundPlaySoundEffectOnPairWithLevels(s32, s32, s32, s32);
extern void BattleEffectCopyBuffers(void);
extern void SoundSetElementPanByIndex(s32, s32);
extern void SoundSetElementVolumeByIndex(s32, s32);
extern VECTOR g_BattleViewOrigin;

INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E120);
/* best: objdiff 87.18% - 392 vs 320 bytes. Frame (0x30, s5 = mfhi), the
   five 16-bit grid taps, the /5 magic and the outer 3-IV tail all match.
   Still off by: (1) loop 1, retail fills the beq delay slot with a copy of
   the persistent value (`move v0,s2`) and stores through v0 in both rows,
   gcc stores s2 directly (tried: switch vs if/else-if, nested if, temp
   local); (2) loop 2, retail keeps the shared row offset in ELEMENT units
   (t4 = y*0x70, s1 = t4+1, sll by 1) and forms the five tap bases in the
   reverse of declaration order, gcc scales the offset into bytes and drops
   the sll (tried: inline y*0x70+1 per pointer, reversed declarations, plain
   indexed taps - 78-87%, the local `s32 off` + five base pointers is the
   best 87.18%).
void func_8008E120(void) {
    s16* pGridA;
    s16* pGridB;
    s16* pUpBase;
    s16* pMidRBase;
    s16* pMidLBase;
    s16* pDnRBase;
    s16* pDnLBase;
    s16* pRow;
    s32 value;
    s32 sum;
    s32 i;
    s32 x;
    s32 y;

    if (g_BattleEffectImage == 0) {
        return;
    }
    pGridB = (s16*)g_BattleEffectGridNext;
    value = 0;
    pRow = pGridB + 0x1490;
    for (i = 0; i < 0x70; i++) {
        switch (rand() & 3) {
        case 0:
            value = 0x180;
            break;
        case 1:
            value = 0;
            break;
        }
        pRow[0x70] = value;
        pRow[0] = value;
        pRow++;
    }
    pGridA = (s16*)g_BattleEffectGrid;
    pUpBase = pGridA - 0x70;
    pMidRBase = pGridA + 1;
    pMidLBase = pGridA - 1;
    pDnRBase = pGridA + 0x71;
    pDnLBase = pGridA + 0x6F;
    for (y = 0x2F; y >= 2; y--) {
        s32 off = y * 0x70 + 1;
        s16* pUp = pUpBase + off;
        s16* pMidR = pMidRBase + off;
        s16* pMidL = pMidLBase + off;
        s16* pDnR = pDnRBase + off;
        s16* pDnL = pDnLBase + off;
        s16* pOut = &pGridB[(y - 1) * 0x70 + 1];

        for (x = 1; x < 0x70; x++) {
            sum = *pUp++ + *pMidR++ + *pMidL++ + *pDnR++ + *pDnL++;
            value = sum / 5;
            if (value >= 4) {
                value -= 3;
            }
            *pOut++ = value;
        }
    }
    BattleEffectCopyBuffers();
}
*/
void BattleDrawEffectTile(u8* pOt, s32 color, s32 flag) {
    TILE* pTile;

    pTile = &g_BattleEffectTiles[g_BattleDrawBufferIndex];
    BattleSetRGBCFast(pTile, color, color, color, 0x60);
    setlen(pTile, 3);
    BattleSetXY0Fast(pTile, 0, 0);
    BattleSetWHFast(pTile, BATTLE_EFFECT_QUAD_WIDTH, BATTLE_EFFECT_QUAD_HEIGHT);
    setSemiTrans(pTile, 1);
    AddPrim(pOt, pTile);
    if (flag != 0) {
        SetDrawMode(&g_BattleEffectDrawModes[g_BattleDrawBufferIndex], 0, 1, GetTPage(0, 2, 0, 0), NULL);
    } else {
        SetDrawMode(&g_BattleEffectDrawModes[g_BattleDrawBufferIndex], 0, 1, GetTPage(0, 1, 0, 0), NULL);
    }
    AddPrim(pOt, &g_BattleEffectDrawModes[g_BattleDrawBufferIndex]);
}
void BattleDrawEffectImage(u8* pOt, s32 shadeLevel, s32 drawTile) {
    POLY_FT4* pPrim;
    TILE* pTile;
    RECT rect;
    s32 shade;

    shade = shadeLevel * 2 / 3;
    pPrim = &g_BattleEffectPolys[g_BattleDrawBufferIndex];
    setlen(pPrim, 9);
    BattleSetRGBCFast(pPrim, shade, shade, shade, 0x2C);
    setShadeTex(pPrim, shade == BATTLE_EFFECT_SHADE_NEUTRAL);
    BattleSetXY0Fast(pPrim, 0, 0);
    BattleSetXY1Fast(pPrim, BATTLE_EFFECT_QUAD_WIDTH, 0);
    BattleSetXY2Fast(pPrim, 0, BATTLE_EFFECT_QUAD_HEIGHT);
    BattleSetXY3Fast(pPrim, BATTLE_EFFECT_QUAD_WIDTH, BATTLE_EFFECT_QUAD_HEIGHT);
    BattleSetUV0Fast(pPrim, 0, 0);
    BattleSetUV1Fast(pPrim, BATTLE_EFFECT_TEX_U_MAX, 0);
    BattleSetUV2Fast(pPrim, 0, BATTLE_EFFECT_TEX_V_MAX);
    BattleSetUV3Fast(pPrim, BATTLE_EFFECT_TEX_U_MAX, BATTLE_EFFECT_TEX_V_MAX);
    setSemiTrans(pPrim, 1);
    pPrim->tpage = GetTPage(1, 1, 0x140, 0x100);
    pPrim->clut = GetClut(0, 0x1FD);
    AddPrim(pOt, pPrim);
    setRECT(&rect, 0x140, 0x100, 0x38, 0x2B);
    LoadImage(&rect, g_BattleEffectImage);
    if (drawTile != 0) {
        pTile = &g_BattleEffectTiles[g_BattleDrawBufferIndex];
        if (shadeLevel > 0x80) {
            shade = shadeLevel * 2;
            setlen(pTile, 3);
            BattleSetRGBCFast(pTile, shade, shade, shade, 0x60);
            setSemiTrans(pTile, 1);
            BattleSetXY0Fast(pTile, 0, 0);
            BattleSetWHFast(pTile, BATTLE_EFFECT_QUAD_WIDTH, BATTLE_EFFECT_QUAD_HEIGHT);
            AddPrim(pOt, pTile);
        }
    }
    SetDrawMode(&g_BattleEffectDrawModes[g_BattleDrawBufferIndex], 0, 1, GetTPage(0, 2, 0, 0), NULL);
    AddPrim(pOt, &g_BattleEffectDrawModes[g_BattleDrawBufferIndex]);
}
void BattleInitSoundSlots(void) {
    BattleSlot* pSlot;
    s32 timer;
    u32 i;

    timer = 0x300;
    SoundStopAllElements();
    pSlot = g_BattleSoundSlots;
    for (i = 0; i < BATTLE_SOUND_SLOT_COUNT; i++) {
        pSlot[i].elementMask = timer;
        timer <<= 2;
        pSlot[i].active = 0;
        pSlot[i].timer = 0;
        pSlot[i].elementIndex = i * 2;
    }
}
void BattleIncrementSoundSlotTimers(void) {
    if (g_BattleSoundSlots[0].timer != BATTLE_SOUND_SLOT_TIMER_MAX) {
        g_BattleSoundSlots[0].timer++;
    }
    if (D_80096EC8 != BATTLE_SOUND_SLOT_TIMER_MAX) {
        D_80096EC8++;
    }
    if (D_80096EEC != BATTLE_SOUND_SLOT_TIMER_MAX) {
        D_80096EEC++;
    }
    if (D_80096F10 != BATTLE_SOUND_SLOT_TIMER_MAX) {
        D_80096F10++;
    }
}
u8* BattleSetActorSoundTable(BattleActor* pActor) {
    u8* pTable;

    switch (pActor->typeId) {
    case 9:
        pTable = g_BattleSoundTableType9;
        break;
    case 0x1D:
        pTable = g_BattleSoundTableType1D;
        break;
    case 0x1B:
        pTable = g_BattleSoundTableType1B;
        break;
    case 0x24:
        pTable = g_BattleSoundTableType24;
        break;
    default:
        pTable = g_BattleSoundTableDefault;
        break;
    }
    pActor->pSoundTable = pTable;
    return pTable;
}
void BattleAcquireSoundSlot(u8* p0, u32 idx, u8* pData, u32 flags) {
    BattleSlot* pSlot;
    BattleSlot* pSel;
    s32 best;
    s32 i;

    best = 0;
    pSel = &g_BattleSoundSlots[BATTLE_SOUND_SLOT_COUNT - 1];
    pSlot = &g_BattleSoundSlots[0];
    for (i = 0; i < BATTLE_SOUND_SLOT_COUNT; i++) {
        pSlot = &g_BattleSoundSlots[i];
        if (pSlot->active == 0 || (idx == 0 && pSlot->soundId == 0)) {
            do { pSel = pSlot; } while (0); // the block keeps the copy out of the beq delay slot
            break;
        }
        if (best < pSlot->timer) {
            best = pSlot->timer;
            pSel = pSlot;
        }
    }
    BattleIncrementSoundSlotTimers();
    pSlot = pSel;
    pSlot->soundId = idx;
    pSlot->spuAddr = (u32)p0;
    pSlot->active = 1;
    pSlot->flags = flags;
    pSlot->pData = (u32*)pData;
    if (pData != 0) {
        pSlot->data = *(BattleSoundData*)pData;
    }
    pSlot->timer = 0;
    if (idx == 0) {
        SoundPlaySoundEffectOnPairWithLevels(pSlot->spuAddr, pSlot->elementIndex, 0x7F, 0x40);
    }
}
void BattleUpdateSoundSlotPositions(void) {
    SVECTOR pos;
    s16 sxy[2];
    BattleSlot* pSlot;
    s32 depth;
    s32 x;
    s32 volume;
    s32 pan;
    s32 i;

    for (i = 0; i < BATTLE_SOUND_SLOT_COUNT; i++) {
        pSlot = &g_BattleSoundSlots[i];
        if (pSlot->active == 0) {
            continue;
        }
        if (pSlot->soundId == 0) {
            continue;
        }
        if (pSlot->soundId == 1) {
            pos.vx = pSlot->data.w[0];
            pos.vy = pSlot->data.w[1];
            pos.vz = pSlot->data.w[2];
        } else {
            pos.vx = pSlot->pData[0];
            pos.vy = pSlot->pData[1];
            pos.vz = pSlot->pData[2];
        }
        pos.vx -= g_BattleViewOrigin.vx;
        pos.vy -= g_BattleViewOrigin.vy;
        pos.vz -= g_BattleViewOrigin.vz;
        gte_ldv0(&pos);
        gte_rtps();
        gte_stsxy(sxy);
        gte_stsz(&depth);
        x = sxy[0];
        if (x < 0) {
            x = 0;
        }
        if (x > 0x140) {
            x = 0x140;
        }
        volume = (0x3000 - depth) * 127 / 0x3000;
        if (volume < 0x28) {
            volume = 0x28;
        }
        if (volume > 0x7F) {
            volume = 0x7F;
        }
        pan = x * 127 / 0x140;
        if (pSlot->timer != 0) {
            SoundSetElementPanByIndex(pSlot->elementIndex, pan);
            SoundSetElementVolumeByIndex(pSlot->elementIndex, volume);
        } else {
            SoundPlaySoundEffectOnPairWithLevels(pSlot->spuAddr, pSlot->elementIndex,
                                                 (s16)volume, (s16)pan);
        }
    }
    BattleIncrementSoundSlotTimers();
}
void BattleReleaseFinishedSoundSlots(void) {
    BattleSlot* pSlot;
    s32 i;

    pSlot = g_BattleSoundSlots;
    for (i = 0; i < BATTLE_SOUND_SLOT_COUNT; i++) {
        if ((SoundGetActiveElementMask(pSlot[i].spuAddr) & pSlot[i].elementMask) == 0) {
            pSlot[i].active = 0;
        }
    }
    BattleIncrementSoundSlotTimers();
}
void BattlePlaySoundEffectById(u8* p) {
    if (p != NULL) {
        BattleAcquireSoundSlot(p + BATTLE_SPU_ADDR_BASE, 0, NULL, g_SpuChannelMask);
    }
}
void BattlePlaySoundEffectByIdx(BattleActor* pActor, s32 idx, u8* pData, s32 flags) {
    if (idx != 0) {
        BattleAcquireSoundSlot((u8*)(idx + BATTLE_SPU_ADDR_BASE), flags, pData,
                      (idx & 0x7F) | ((pActor->flags >> 20) & 0x80));
    }
}
void BattlePlayActorSoundPair(BattleActor* pActor, s32 idx, u8* pData, s32 flags) {
    s32 i;
    s32 pad[2]; // unused: retail's frame is 0x30
    u8* pTable;

    if (idx == 0) {
        idx = rand() % 6 + 1;
    }
    idx = pActor->pSoundTable[idx];
    i = idx << 1;
    if (idx == BATTLE_SOUND_ID_NONE) {
        return;
    }
    // Load-bearing shapes: the duplicated read keeps the base in $s0 with the table
    // index in $s1, `*(pTable + i + 1)` gives retail's base-first `addu`, and the
    // do/while block schedules the tail as one unit. All three are worth 0.1-0.4%.
    do {
        if (idx) {
            idx = D_80091EE0[i];
            pTable = D_80091EE0;
        } else {
            idx = D_80091EE0[i];
            pTable = D_80091EE0;
        }
        if (idx != 0) {
            BattleAcquireSoundSlot((u8*)(idx | BATTLE_SPU_ADDR_BASE), flags, pData,
                          (idx & 0x7F) | ((pActor->flags >> 20) & 0x80));
        }
        idx = *(pTable + i + 1);
        if (idx != 0) {
            BattleAcquireSoundSlot((u8*)(idx | BATTLE_SPU_ADDR_BASE), flags, pData,
                          (idx & 0x7F) | ((pActor->flags >> 20) & 0x80));
        }
    } while (0);
}
void BattleStopSoundSlotsByFlags(u8 kind) {
    BattleSlot* pSlot;
    s32 i;

    pSlot = g_BattleSoundSlots;
    for (i = 0; i < BATTLE_SOUND_SLOT_COUNT; i++) {
        if (pSlot[i].active != 0 && pSlot[i].flags == kind) {
            SoundStopElementsByIndex(pSlot[i].elementIndex);
            pSlot[i].active = 0;
        }
    }
}
// best: objdiff 90.34% - 176 bytes = retail's 44 instructions. Round 479 re-measured the park and tried four more spellings on the two residuals, all 90.34% / 176 / 44 insns: (1) block 2 with the whole call argument in an s32 local, (2) block 2 with the byte in the reused u8 soundId local, (3) block 2 with only the flags half in a local, (4) the parked body itself. cc1's own .s (build/src/battling/main/misc17.c.s) shows sched2 hoisting the byte's andi to the FRONT of block 2 whatever the source order, so reorg then steals it into the beqz slot (retail keeps a nop there and fills the lw a0 load-delay slot with that andi instead); block 1 in retail has the andi in the branch slot, i.e. the same schedule as ours, so the two retail blocks were scheduled differently from an identical DAG. Residual 1 is the $s1/$s2 mirror (retail: index $s2, base $s1); preset screen this round: 2.7.2-cdk 71.98, 2.6.0 does not compile, so Default is right. Previous attempts that stand: base at its use 85.84/168, both reads D_80091EE0[i] 90.70/184, non-void return 90.11, 3-minute permuter base 145 best 25 nonsense volatile.
/*
void func_8008ED6C(BattleActor* pActor, s32 idx) {
    s32 pad[2];
    u8* pSoundTable;
    s32 i;
    u8 soundId;

    soundId = pActor->pSoundTable[idx];
    if (soundId == BATTLE_SOUND_ID_NONE) {
        return;
    }
    pSoundTable = D_80091EE0;
    i = soundId << 1;
    if (D_80091EE0[i] != 0) {
        BattleStopSoundSlotsByFlags(((pActor->flags >> 20) & 0x80) | (D_80091EE0[i] & 0x7F));
    }
    if (pSoundTable[i + 1] != 0) {
        BattleStopSoundSlotsByFlags(((pActor->flags >> 20) & 0x80) | (pSoundTable[i + 1] & 0x7F));
    }
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008ED6C);


void BattleUpdateActorApproach(BattleActor* pActor, s16 arg1, s16 arg2) {
    s32 pad[4];
    s32 step;
    s32 backStep;
    s32 flag;
    s32 i;

    step = pActor->stepForward;
    backStep = pActor->stepBack;
    if (pActor->flags & 0x100) {
        backStep = backStep * 2 / 3;
        flag = 0;
    } else {
        flag = 1;
    }
    for (i = 0; i < 2; i++) {
        if (arg2 != 0 && flag != 0) {
            pActor->distance += step;
            if (pActor->distance > arg2) {
                pActor->distance = arg2;
            }
        } else {
            pActor->distance -= backStep;
            if (pActor->distance < 0) {
                pActor->distance = 0;
            }
        }
    }
    pActor->angle = func_8008B650(pActor->angle, arg1, 0x40);
    pActor->facingOffset = 0;
}
void BattleCheckStateAndSetDelay60(BattleActor* pActor, BattleWork* pWork) {
    BattleTryQueueMessage5(pActor, pWork);
    pWork->delay = 0x3C;
}
void BattleQueueRandomMessagesAndDelay(BattleActor* pActor, BattleWork* pWork, s32 flag) {
    BattleQueueRandomMessages(pActor, pWork, flag);
    pWork->delay = flag ? 0x1E : 0xA;
}
void BattleQueueMessage3AndDelay60(BattleActor* pActor, BattleWork* pWork) {
    BattleQueueMessage(pActor, 3);
    pWork->delay = 0x3C;
}
void BattleToggleActionFlag2000(BattleActor* pActor, BattleWork* pWork) {
    u32 flags;

    flags = pWork->flags.flags;
    pWork->flags.flags = (flags & ~0x2000) | ((((flags >> 13) & 1) ^ 1) << 13);
    if (pWork->flags.flags & 0x2000) {
        pActor->flags |= 2;
        pWork->delay = 0x3C;
    } else {
        pActor->flags &= ~2;
        pActor->flags &= ~0x38;
        pWork->delay = 0x1E;
    }
}
void BattleQueueMessages4Then3Delay60(BattleActor* pActor, BattleWork* pWork) {
    BattleQueueMessage(pActor, 4);
    BattleQueueMessage(pActor, 3);
    pWork->delay = 0x3C;
}
void BattleQueueMessage4AndDelay60(BattleActor* pActor, BattleWork* pWork) {
    BattleQueueMessage(pActor, 4);
    pWork->delay = 0x3C;
}
void BattleSetRandomFacingAround800(BattleActor* pActor, BattleWork* pWork) {
    u16 timer;

    if (g_BattleActorDistance2D >= 0x801) {
        pWork->delay = 1;
        pWork->maxDistance = 0;
        return;
    }
    pWork->delay -= 1;
    if ((s16)pWork->delay != -1) {
        return;
    }
    pWork->targetAngle = rand() % 0x600 + 0x500;
    timer = rand() % 50 + 0xA;
    pWork->maxDistance = 0xFF;
    pWork->delay = timer;
}
void BattleSetRandomFacingAround0(BattleActor* pActor, BattleWork* pWork) {
    u16 timer;

    if (g_BattleActorDistance2D < 0x100) {
        pWork->delay = 1;
        pWork->maxDistance = 0;
        return;
    }
    pWork->delay -= 1;
    if ((s16)pWork->delay != -1) {
        return;
    }
    pWork->targetAngle = rand() % 0x600 - 0x300;
    timer = rand() % 120 + 0xA;
    pWork->maxDistance = 0xFF;
    pWork->delay = timer;
}
void BattleSetActionFieldFAndDispatch(BattleActor* pActor, BattleWork* pWork, u8 value) {
    pWork->difficulty = value;
    BattleDispatchWorkState(pActor);
}
void BattleRunActorStateHandler(BattleActor* pActor) {
    BattleWork* pWork;
    BattleWork** ppWork;

    // The extra indirection is load-bearing: gcc sinks a plain `pActor->pWork`
    // load below the flag store, retail loads it right after the prologue.
    ppWork = &pActor->pWork;
    pWork = *ppWork;
    g_BattlePlayer2ComControlled = 1;
    if (g_BattleAiAppliedBehaviorLevel != g_BattleAiBehaviorLevel) {
        pWork->delay = 0;
        pWork->maxDistance = 0;
        pActor->flags &= ~2;
        pActor->distance = 0;
        pActor->flags &= ~0x38;
        pWork->flags.flags &= ~0x2000;
        g_BattleAiAppliedBehaviorLevel = g_BattleAiBehaviorLevel;
        pActor->facingOffset = pActor->sideBaseAngle + BATTLE_ANGLE_HALF_TURN;
    }
    if (pWork->flags.flags & 0x2000) {
        pActor->flags |= 2;
    }
    switch (g_BattleAiBehaviorLevel) {
    case 2:
    case 8:
    case 9:
    case 11:
    case 12:
    case 13:
        break;
    default:
        pWork->delay -= 1;
        if ((s16)pWork->delay != -1) {
            return;
        }
        break;
    }
    switch (g_BattleAiBehaviorLevel) {
    case 3:
        BattleQueueRandomMessagesAndDelay(pActor, pWork, 0);
        break;
    case 4:
        BattleQueueRandomMessagesAndDelay(pActor, pWork, 1);
        break;
    case 5:
        BattleQueueMessage3AndDelay60(pActor, pWork);
        break;
    case 6:
        BattleQueueMessages4Then3Delay60(pActor, pWork);
        break;
    case 7:
        BattleQueueMessage4AndDelay60(pActor, pWork);
        break;
    case 9:
        BattleSetRandomFacingAround800(pActor, pWork);
        break;
    case 8:
        BattleSetRandomFacingAround0(pActor, pWork);
        break;
    case 10:
        BattleCheckStateAndSetDelay60(pActor, pWork);
        break;
    case 11:
        BattleSetActionFieldFAndDispatch(pActor, pWork, 0);
        return;
    case 12:
        BattleSetActionFieldFAndDispatch(pActor, pWork, 1);
        return;
    case 13:
        BattleSetActionFieldFAndDispatch(pActor, pWork, 2);
        return;
    case 2:
        g_BattlePlayer2ComControlled = 0;
        return;
    case 1:
        BattleToggleActionFlag2000(pActor, pWork);
        break;
    case 0:
    default:
        pWork->delay = 1;
        break;
    }
    BattleUpdateActorApproach(pActor, pWork->targetAngle, pWork->maxDistance);
}
s32 BattleCheckValueAboveScaledMax(BattleActor* pActor, s32 arg1) {
    return (pActor->gaugeMax * arg1) / BATTLE_GAUGE_FRACTION_SCALE < pActor->gaugeValue;
}
