#include "common.h"
#include "system/memory.h"
#include "psyq/libgte.h"
#include "field/actor.h"

// One battle sprite: the transform steps at 0x40..0x48 that BattleStepActorFadeOut
// advances, the clip colour at 0x68..0x6A, the model parts at 0x74/0x7C and the
// direction/target pair at 0xA0..0xA4 that BattleAngleToTarget measures. Every
// function in this unit drives one of these.
typedef struct {
    /* 0x00 */ u8 unk0[0x2];
    /* 0x02 */ s16 x; // the point BattleAngleToPoint/SpriteComputeAngleFromPoints measures from
    /* 0x04 */ u8 unk4[0x6];
    /* 0x0A */ s16 y;
    /* 0x0C */ void (*pCallback)(); // called with this record when countdown reaches 0
    /* 0x10 */ u8 unk10[0x2C];
    /* 0x3C */ u16 rotZ; // z angle RotMatrix is given; rotZStep is added to it every frame
    /* 0x3E */ u8 unk3E[0x2];
    /* 0x40 */ s32 scaleX; // the vector BattleRenderSpriteBody passes to ScaleMatrix
    /* 0x44 */ s32 scaleY;
    /* 0x48 */ s32 scaleZ;
    /* 0x4C */ u8 unk4C[0x14];
    /* 0x60 */ s32 countdown; // decremented every frame; the callback fires on the frame it hits 0
    /* 0x64 */ u16 rotZStep;
    /* 0x66 */ u8 unk66[0x2];
    /* 0x68 */ u8 colorR; // clip colour, clamped by SpriteColorAddClamp as the record fades out
    /* 0x69 */ u8 colorG;
    /* 0x6A */ u8 colorB;
    /* 0x6B */ u8 unk6B[0x9];
    /* 0x74 */ void *pTargetSprite; // the sprite this one is pointed at
    /* 0x78 */ u8 unk78[0x4];
    /* 0x7C */ s32 *pArchiveEntryIndex; // the entry id BattleLoadActorArchiveFile loads
    /* 0x80 */ u8 unk80[0x20];
    /* 0xA0 */ u16 targetX; // the point BattleAngleToTarget measures from x/y
    /* 0xA2 */ u8 unkA2[0x2];
    /* 0xA4 */ u16 targetY;
    /* 0xA6 */ u8 unkA6[0x2];
    /* 0xA8 */ u32 unkA8;
    // func_800BF3E8 reads byte 3 as a signed byte (\`lb 0xAF\`), so the 0xAC word and
    // the 0xAF byte are one storage read at two widths.
    /* 0xAC */ union {
        /* 0xAC */ u32 word;
        /* 0xAC */ u8 bytes[0x4];
        /* 0xAC */ s_char sbytes[0x4];
    } flags;
    /* 0xB0 */ s8 animId; // animation id SpritePlayAnimation is handed
} BattleActorSprite; // Size: 0xB4

typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ BattleActorSprite *pCurSprite;
    /* 0x08 */ void (*pUpdate)(); // the state machine's body; the frame loop calls through it
    /* 0x0C */ u8 unkC[0x10];
    /* 0x1C */ s32 stateStep;
    /* 0x20 */ u8 unk20[0x4];
    /* 0x24 */ s32 actorIndex;
    /* 0x28 */ u32 partKey;
    /* 0x2C */ s32 pathIndex; // cursor into the target-position list at g_BattleStateWork + 0xA3C
    /* 0x30 */ s32 unk30;
    /* 0x34 */ s32 timer; // BattleIncrementStateTimer raises it, func_800B9F78 counts it down once a frame
    /* 0x38 */ u8 unk38[0xC];
    /* 0x44 */ s32 targetDistance; // measured by BattleFaceTargetAndMeasureDistance
    /* 0x48 */ u8 unk48;
    /* 0x49 */ u8 pendingStep; // stateStep copied by BattleApplyStateStep, run once and cleared
    /* 0x4A */ u8 unk4A;
    /* 0x4B */ u8 pad4B;
    /* 0x4C */ BattleActorSprite *pTargetSprite; // the actor BattleActorSnapToTargetAndFace is aimed at
} BattleMisc33State; // Size: 0x50

extern BattleMisc33State *g_BattleState;
extern s32 g_BattleCommandRowIndex;

// A 0x48-byte row of the per-battle actor table that g_BattleStateWork + 0x108 starts;
// misc25 indexes the same rows as g_BattleActorCommandIds[g_BattleCommandRowIndex - 1].
typedef struct {
    /* 0x00 */ u8 unk0[0x46];
    /* 0x46 */ u16 slotMask;
} BattleActorRow; // Size: 0x48

extern BattleActorRow D_800C3FB8[];
// One 0x1C-byte row of the sensor table the battle state work block starts with; a
// sprite's row index is its animation direction (unkA8>>30) and part key (flags&3).
typedef struct {
    /* 0x00 */ u8 unk0[0x7];
    /* 0x07 */ u8 unk7;
    /* 0x08 */ u8 unk8[0x14];
} BattleSpriteSensorRow; // Size: 0x1C
extern BattleActorSprite *g_BattleActorList;
extern u8 D_800C3780;
extern s16 g_BattleDigitTextureIds[];
extern s32 g_BattleColorMode;
extern u8 *g_BattleSpriteArchive;
extern s16 D_800D3634;
extern s16 g_BattleActorListCount;
extern s32 BattleCollectMaskedSlotActors(u16 mask, void **pOut, s32 arg2);
extern void func_800B9F78();
extern s32 D_800C3E20;
extern u16 g_BattleSlotBitMask;
extern u8 g_BattleSedsLoaded;
extern u8 g_BattleHexDigitTable[];
extern u8 g_BattleStateWork[];
extern u32 D_800C37A4[];
extern u8 g_FlagTimerWorkListAdds;
extern s32 g_NumFlaggedTimerWorkListEntries;


extern s16 SpriteComputeAngleFromPoints(DVECTOR, DVECTOR);
extern s32 func_800C0FAC(s32);
extern u8 SpriteColorAddClamp(u8, s16);
extern s32 BattleComputeVectorDistance2D(DVECTOR vec0, DVECTOR vec1);
extern void SpriteSetDirection(BattleActorSprite *p, s16 arg1);
extern void func_800223B0(BattleActorSprite *p, s16 arg1);
extern void SpritePlayAnimation(BattleActorSprite *p, s32 arg1);

// One 0x18-byte part of a battle sprite: the record ResourceHelperGetSpriteList
// converts a menu dirent into, with its width/height at 0x6/0x7.
typedef struct {
    /* 0x00 */ u16 x;
    /* 0x02 */ u16 y;
    /* 0x04 */ u8 u;
    /* 0x05 */ u8 v;
    /* 0x06 */ u8 width;
    /* 0x07 */ u8 height;
    /* 0x08 */ u8 unk8[0x10];
} BattleSpritePart; // Size: 0x18

// What WorkListsAddTasks allocates for the fade-out task (0x130 bytes) and what
// BattleRenderSpriteBody draws: a rotation at 0x38, the scale steps at 0x40 that
// BattleStepActorFadeOut advances, the clip colour at 0x68, and the part list at
// 0x70 whose count is at 0x6C.
typedef struct {
    /* 0x00 */ u8 unk0[0x38];
    /* 0x38 */ SVECTOR rot;
    /* 0x40 */ VECTOR scale;
    /* 0x50 */ u8 unk50[0x10];
    /* 0x60 */ s32 countdown;
    /* 0x64 */ s32 rotZStep;
    // Set up one channel at a time, but drawn as the whole prim colour word.
    /* 0x68 */ union {
        /* 0x68 */ u32 word;
        /* 0x68 */ struct {
            /* 0x68 */ u8 red;
            /* 0x69 */ u8 green;
            /* 0x6A */ u8 blue;
            /* 0x6B */ u8 code; // the packet's POLY_FT4 code; 0x2E is a textured quad with setSemiTrans
        } channels;
    } color;
    /* 0x6C */ s32 partCount;
    /* 0x70 */ BattleSpritePart parts[8];
} BattleSpriteBody; // Size: 0x130

// The work-list entry the frame loop passes to each callback: +4 is the data block
// WorkListsAddTasks allocated for the task.
typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ BattleSpriteBody *pBody;
} BattleSpriteBodyTask; // Size: 0x8

extern BattleSpriteBody *WorkListsAddTasks(s32 allocSize, void *pData, void *pTimerCallback,
                                          void *pWorkCallback, void *pOnFreeCallback);
extern s32 func_800BE6E8(s32 value, u8 *pBuf, s32 n, u8 started, s32 base);
extern s32 ResourceHelperGetSpriteList(u8 *pDir, u32 index, BattleSpritePart *pOut, s16 x, s16 y);
extern s32 BattleAttachEffectSprite(BattleActorSprite *p, s32 arg1, s32 arg2);
extern void BattleFreeTimerTaskIfActive(void);
extern void func_800BE790(void);
extern s32 BattleGetAdjustedWaitCountdown(void);
extern void BattleRemoveSedsEntries(s32);
extern void BattleUnloadActorArchive();
extern int ArchiveSetIndex(int directoryIndex, int entryIndex);
extern int ArchiveDecodeAlignedSize(u32 entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void *pBuffer, u32 arg2, u32 flags);
extern void *g_BattleActorArchiveBuffer;
extern u32 g_BattleActorArchivePartKey;

void BattleStepActorFadeOut(BattleActorSprite *p) {
    p->rotZ = p->rotZ + p->rotZStep;
    p->scaleX += 0x330;
    p->scaleY += 0x330;
    p->scaleZ += 0x330;
    p->colorR = SpriteColorAddClamp(p->colorR, -4);
    p->colorG = SpriteColorAddClamp(p->colorG, -4);
    p->colorB = SpriteColorAddClamp(p->colorB, -4);
    if (--p->countdown == 0) {
        p->pCallback(p);
    }
}
// The GTE geom-output block g_BattleGeomMatrix doubles as a matrix: CompMatrix writes
// into it and its t[2] word is the geom screen offset (misc32 reads the same
// 0x1C word as D_800C377C).
typedef struct {
    /* 0x00 */ u8 unk0[0x1C];
    /* 0x1C */ s32 geomScreen;
} BattleGeomMatrix; // Size: 0x20

extern BattleGeomMatrix g_BattleGeomMatrix;
extern void func_800BD810(BattleSpritePart *pPart, u32 color);

void BattleRenderSpriteBody(BattleSpriteBodyTask *pTask) {
    BattleSpriteBody *pBody;
    BattleSpritePart *pPart;
    MATRIX mtx;
    SVECTOR padVec; // unused: reproduces retail's 0x80 frame
    SVEC vec;
    VECTOR sxy;
    VECTOR scale;
    s32 ofx;
    s32 ofy;
    s32 i;
    s32 j;

    pBody = pTask->pBody;
    ReadGeomOffset(&ofx, &ofy);
    sxy.vx = (0xA0 - ofx) * 2;
    sxy.vy = (0x46 - ofy) * 2;
    sxy.vz = ReadGeomScreen();
    g_BattleGeomMatrix.geomScreen = ReadGeomScreen();
    SpriteCopySVec(&vec, (SVEC *)&pBody->rot);
    scale.vx = pBody->scale.vx;
    scale.vy = pBody->scale.vy;
    scale.vz = pBody->scale.vz;
    for (i = 0; i != 6; i++) {
        RotMatrix((SVECTOR *)&vec, &mtx);
        TransMatrix(&mtx, &sxy);
        CompMatrix((MATRIX *)&g_BattleGeomMatrix, &mtx, &mtx);
        ScaleMatrix(&mtx, &scale);
        SetRotMatrix(&mtx);
        SetTransMatrix(&mtx);
        j = 0;
        pPart = pBody->parts;
        while (j != pBody->partCount) {
            func_800BD810(pPart, pBody->color.word);
            j++;
            pPart++;
        }
        vec.z -= 5;
        scale.vx -= 0x330;
        scale.vy -= 0x330;
        scale.vz -= 0x330;
    }
}
void BattleCreateNumberSpriteTask(s32 value) {
    BattleSpriteBody *pBody;
    u8 digits[6];
    s32 i;
    s32 mode;
    s32 step;
    s32 tex;

    pBody = WorkListsAddTasks(0x130, NULL, BattleStepActorFadeOut, BattleRenderSpriteBody, NULL);
    step = ((rand() & 3) - 2) * 8;
    pBody->rotZStep = -step;
    if (pBody->rotZStep == 0) {
        pBody->rotZStep = 6;
    }
    pBody->countdown = 0x20;
    pBody->color.channels.code = 0x2E;
    pBody->scale.vx = 0x2000;
    pBody->scale.vy = 0x2000;
    pBody->scale.vz = 0x2000;
    mode = g_BattleColorMode;
    pBody->partCount = 0;
    pBody->rot.vx = 0;
    pBody->rot.vy = 0;
    pBody->rot.vz = 0;
    switch (mode) {
    case 0xB:
        pBody->color.channels.red = 0;
        pBody->color.channels.green = 0;
        pBody->color.channels.blue = 0x80;
        break;
    case 3:
        pBody->color.channels.red = 0x80;
        pBody->color.channels.green = 0;
        pBody->color.channels.blue = 0x80;
        break;
    case 2:
        pBody->color.channels.red = 0;
        pBody->color.channels.green = 0x80;
        pBody->color.channels.blue = 0;
        break;
    default:
        pBody->color.channels.red = 0x80;
        pBody->color.channels.green = 0x80;
        pBody->color.channels.blue = 0x80;
        break;
    }
    func_800BE6E8(value, digits, 5, 0, 0);
    tex = g_BattleDigitTextureIds[digits[0]];
    pBody->partCount = 0;
    i = 0;
    while (i != digits[0]) {
        pBody->partCount += ResourceHelperGetSpriteList(g_BattleSpriteArchive, digits[i + 1] + 0x72,
                                                        &pBody->parts[pBody->partCount], tex, -8);
        i++;
        tex += 10;
    }
    if (pBody->partCount != 0) {
        for (i = 0; i != pBody->partCount; i++) {
            pBody->parts[i].width--;
            pBody->parts[i].height--;
        }
    }
}
void BattleAttachActorAnimScripts(s32 index, s32 arg1, s32 arg2, s32 arg3) {
    BattleActorSprite *pSprite;
    BattleMisc33State *pState;
    u8 *pBase;
    s32 anim;

    g_NumFlaggedTimerWorkListEntries = 0;
    g_FlagTimerWorkListAdds = 1;
    pBase = g_BattleStateWork;
    pSprite = *(BattleActorSprite **)(pBase + index * 4 + 0x8C8C);
    D_800C3780 = 1;
    if (pSprite != NULL) {
        anim = pSprite->flags.sbytes[3];
        SpritePlayAnimation(pSprite, 0xA);
        pState = g_BattleState;
        g_BattleState = (BattleMisc33State *)1;
        if (arg1 != 0) {
            BattleAttachEffectSprite(pSprite, arg1, 0);
        }
        if (arg2 != 0) {
            BattleAttachEffectSprite(pSprite, arg2, 1);
        }
        if (arg3 != 0) {
            BattleAttachEffectSprite(pSprite, arg3, 0xA);
        }
        g_BattleState = pState;
        while (BattleGetAdjustedWaitCountdown() != 0) {
            func_800BE790();
        }
        if (pSprite->flags.sbytes[3] == 0xA) {
            while (pSprite->flags.sbytes[3] == 0xA) {
                func_800BE790();
            }
        }
        SpritePlayAnimation(pSprite, anim);
    }
    D_800C3780 = 0;
    BattleFreeTimerTaskIfActive();
    g_NumFlaggedTimerWorkListEntries = 0;
    g_FlagTimerWorkListAdds = 0;
}
void BattleWriteHexDigits(s32 value, u8 *pBuf, s32 n, s32 base) {
    s32 i;

    for (i = 0; i != n; i++) {
        s32 last = n - 1;

        pBuf[i + 1] = g_BattleHexDigitTable[(value >> ((last - i) * 4)) & 0xF] + base;
    }
    pBuf[0] = n;
}
/* best 88.33%: everything matches but the loop-entry order.  Retail emits
   `beqz a2` with `i = 0` in its delay slot, so its RTL had the `last = n - 1`
   local hoisted AFTER the entry test; declaring `last` before the loop puts it
   in front, and inlining `n - 1 - i` instead makes gcc hoist `n - 1` badly
   (73.06%).  Two instructions differ, no missing or extra insn.
void BattleWriteHexDigits(s32 arg0, u8 *p, s32 n, s32 arg3) {
    s32 i;
    s32 last;

    last = n - 1;
    for (i = 0; i != n; i++) {
        p[i + 1] = g_BattleHexDigitTable[(arg0 >> ((last - i) * 4)) & 0xF] + arg3;
    }
    p[0] = n;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BE6E8);
/* best 92.86% (164 bytes, retail 168).  Structure is right: the loop is entered
   through a PEELED first remainder (`value %= *pTab; pTab--;` before the loop,
   which is why retail's preheader `j`s into the latch block and the latch is also
   the entry), then the body divides by *pTab, n--, and the latch re-remainders with
   *pTab before pTab--.  The two `lw 0(v1)` are NOT CSE'd because the `sb` of the
   digit sits between them.  The two remaining diffs: retail emits the preheader
   `lui/addiu` of D_800C37A4 BEFORE the `sll v1,a2,2`, and retail's exit path has a
   DEAD `addiu v1,v1,4` (a giv bump-back no source shape here reproduces; 3-minute
   permuter run from this body: NONE, base=160).  Parked pending that dead insn.
   s32 func_800BE6E8(s32 value, u8 *pBuf, s32 n, u8 started, s32 base) {
       u8 *pDst;
       u8 digit;
       s32 count;
       u32 *pTab;

       count = 0;
       pDst = pBuf + 1;
       if (value < 0) {
           count = 1;
           pBuf[1] = '-';
           pDst = pBuf + 2;
           value = -value;
           n--;
       }
       pTab = &D_800C37A4[n];
       value = (u32)value % *pTab;
       pTab--;
       for (; n != 0; pTab--) {
           digit = (u32)value / *pTab;
           n--;
           if (digit != 0) {
               started = 1;
           }
           if (started != 0) {
               *pDst = digit + base;
               pDst++;
               count++;
           }
           value = (u32)value % *pTab;
       }
       *pDst = value + base;
       pBuf[0] = count + 1;
       return count + 1;
   }
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BE790);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEB04);
typedef struct {
    /* 0x00 */ u16 cur;
    /* 0x02 */ u16 pressed;
    /* 0x04 */ u16 released;
    /* 0x06 */ u16 unk6;
} BattlePadFrame; // Size: 0x8

typedef struct {
    /* 0x0000 */ u8 unk0[0xC58];
    /* 0xC58 */ u16 cur[2];
    /* 0xC5C */ u16 pressed[2];
    /* 0xC60 */ u16 released[2];
    /* 0xC64 */ BattlePadFrame history[4];
} BattlePadState; // Size: 0xC84

extern int ControllerGetButtonState(int controllerIndex);
extern u16 D_800D30E4;

extern u16 g_BattleWaitFrameCountdown;
extern void Vsync(s32 mode);
extern void BattleUpdatePadState(void);

void BattlePollPadAndSkipWaitOnStart(void) {
    u8 *pStateBase;
    BattlePadState *pState;

    BattleUpdatePadState();
    pStateBase = g_BattleStateWork;
    pState = (BattlePadState *)(pStateBase + 0x8000);
    if (pState->cur[0] & 0x100) {
        Vsync(8);
        g_BattleWaitFrameCountdown = 0;
    }
}
void BattleUpdatePadState(void) {
    u8 *pStateBase;
    BattlePadState *pState;
    s32 cur;
    u16 prev;
    u16 prev2;

    /* the mask is what makes gcc narrow the call result */
    cur = ControllerGetButtonState(0) & 0xFFFF;
    /* a symbol plus literal folds into one lui/addiu; retail adds 0x8000 in a register */
    pStateBase = g_BattleStateWork;
    pState = (BattlePadState *)(pStateBase + 0x8000);
    prev = pState->cur[0];
    pState->cur[0] = cur;
    pState->pressed[0] = ~prev & cur;
    pState->released[0] = prev & ~cur;

    cur = ControllerGetButtonState(1);
    prev2 = pState->cur[1];
    pState->pressed[1] = ~prev2 & cur;
    pState->cur[1] = cur;
    pState->released[1] = pState->cur[0] & ~cur;

    if (pState->cur[0] == pState->history[0].cur) {
        return;
    }
    pState->history[3] = pState->history[2];
    pState->history[2] = pState->history[1];
    pState->history[1] = pState->history[0];
    pState->history[0].cur = pState->cur[0];
    pState->history[0].pressed = pState->pressed[0];
    pState->history[0].released = pState->released[0];
    pState->history[0].unk6 = D_800D30E4;
}

void BattleResetStateGlobals(void) {
    D_800C3E20 = 0;
    g_BattleState = NULL;
    g_BattleSlotBitMask = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BED4C);
/* best 88.03% (156 bytes = retail's size).  The address of g_BattleState has to be
   live ACROSS the BattleSetStateStep call (retail reloads it into $s0 for every store
   and has ONE dead post-call `lw v0,0(s0)`), which only happens if a g_BattleState
   store follows the call -- so `g_BattleState->timer = 0;` is after it, NOT before
   (before the call the body is 144 bytes, 85.18%).  Retail then reorg'd the timer
   store into the jal delay slot using the reload it made BEFORE the call, leaving the
   post-call load dead; ours reloads after the call and keeps the store last, and the
   %hi/%lo base registers come out v0 where retail has v1.  Same instruction multiset.
   The PRE-call order is fixed and matches: pUpdate, g_BattleState, g_BattleCommandRowIndex,
   unk4A, unk48(1), unk30(0), pendingStep, pathIndex(1), pCurSprite.
   void func_800BED4C(void) {
       BattleMisc33State *pState;

       pState = HeapAlloc(0x50, 0);
       pState->pUpdate = func_800B9F78;
       g_BattleState = pState;
       g_BattleCommandRowIndex = 0;
       pState->unk4A = 0;
       g_BattleState->unk48 = 1;
       g_BattleState->unk30 = 0;
       g_BattleState->pendingStep = 0;
       g_BattleState->pathIndex = 1;
       g_BattleState->pCurSprite = NULL;
       BattleSetStateStep(0);
       g_BattleState->timer = 0;
       g_NumFlaggedTimerWorkListEntries = 0;
       g_FlagTimerWorkListAdds = 1;
   }
*/
void BattleDestroyState(void) {
    HeapFree(g_BattleState);
    g_NumFlaggedTimerWorkListEntries = 0;
    g_BattleState = NULL;
    g_FlagTimerWorkListAdds = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEE2C);
/* Not convertible: retail switches the stack pointer into the 0x1000-byte
   HeapAlloc block for the BattleQueueActorCommand call (sw $sp,0($t0); addiu $t0,-4;
   addu $sp,$t0, then addiu $sp,4; lw $sp,0($sp)), which the original source
   wrote as inline asm -- the same shape as GfxLoadImageAccelerated in
   system/graphics.  No C produces it. */
s32 BattleCollectMaskedSlotActors(u16 mask, void **pOut, s32 arg2) {
    u8 *pBase;
    BattleActorSprite *pSprite;
    s32 i;
    s32 count;

    pBase = g_BattleStateWork;
    i = 0;
    count = 0;
    do {
        if ((mask & 1) != 0) {
            pSprite = *(BattleActorSprite **)(pBase + i * 4 + 0x8C8C);
            if (pSprite != NULL) {
                pSprite->pTargetSprite = (void *)arg2;
                pOut[count] = pSprite;
                count++;
            }
        }
        i++;
        mask >>= 1;
    } while (i != 0xB);
    pOut[count] = NULL;
    return count;
}
s16 BattleAngleToPoint(BattleActorSprite *p0, BattleActorSprite *p1) {
    DVECTOR vec0;
    DVECTOR vec1;
    s32 c0;
    s32 c1;
    s32 c2;
    s32 c3;

    c0 = p0->x;
    vec0.vx = c0;
    c1 = p0->y;
    vec0.vy = c1;
    c2 = p1->x;
    vec1.vx = c2;
    c3 = p1->y;
    vec1.vy = c3;
    return SpriteComputeAngleFromPoints(vec1, vec0);
}
s16 BattleAngleToTarget(BattleActorSprite *p) {
    DVECTOR vec0;
    DVECTOR vec1;
    s32 c0;
    s32 c1;
    s32 c2;
    s32 c3;

    c0 = p->x;
    vec0.vx = c0;
    c1 = p->y;
    vec0.vy = c1;
    c2 = p->targetX;
    vec1.vx = c2;
    c3 = p->targetY;
    vec1.vy = c3;
    return SpriteComputeAngleFromPoints(vec1, vec0);
}
void func_800BEFF4(s32 actorIndex) {
    BattleActorSprite *pSprite;
    u8 *pBase;
    u32 part;

    pSprite = g_BattleState->pCurSprite;
    if (pSprite != NULL && g_BattleState->actorIndex != actorIndex) {
        part = pSprite->unkA8 >> 30;
        if (((BattleSpriteSensorRow *)g_BattleStateWork)[((g_BattleState->pCurSprite->flags.word & 3) << 2) | part].unk7 == 0) {
            SpritePlayAnimation(g_BattleState->pCurSprite, g_BattleState->pCurSprite->animId);
        }
    }
    g_BattleState->actorIndex = actorIndex;
    pBase = g_BattleStateWork;
    g_BattleState->pCurSprite = *(void **)(pBase + actorIndex * 4 + 0x8C8C);
}

void BattleSetStateStep(s32 arg0) {
    g_BattleState->stateStep = arg0;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BF0C4);
void BattleFaceTargetAndMeasureDistance(BattleActorSprite *p, s32 arg1) {
    DVECTOR vec0;
    DVECTOR vec1;
    s32 c0;
    s32 c1;
    s32 c2;
    s32 c3;

    c0 = p->x;
    vec0.vx = c0;
    c1 = p->y;
    vec0.vy = c1;
    c2 = p->targetX;
    vec1.vx = c2;
    c3 = p->targetY;
    vec1.vy = c3;
    g_BattleState->targetDistance = BattleComputeVectorDistance2D(vec0, vec1);
    SpriteSetDirection(p, BattleAngleToTarget(p));
    func_800223B0(p, BattleAngleToTarget(p));
    SpritePlayAnimation(p, arg1);
    BattleSetStateStep(6);
}
void BattleLoadActorArchiveFile(BattleActorSprite *pSprite) {
    s32 entry;
    void *pBuffer;
    u32 part;

    BattleUnloadActorArchive();
    ArchiveSetIndex(0x2C, 1);
    entry = *pSprite->pArchiveEntryIndex;
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(entry), 1);
    ArchiveReadFileToBuffer(entry, pBuffer, 0, 0x80);
    g_BattleActorArchiveBuffer = pBuffer;
    part = pSprite->unkA8 >> 30;
    g_BattleActorArchivePartKey = ((pSprite->flags.word & 3) << 2) | part;
}
s32 BattleLoadSoundSeds(void) {
    s32 ret;

    if (g_BattleSedsLoaded == 0) {
        ret = func_800C0FAC(g_BattleActorArchiveBuffer);
        g_BattleSedsLoaded = 1;
    }
    return ret;
}
void BattleFreeSoundSeds(void) {
    if (g_BattleSedsLoaded != 0) {
        BattleRemoveSedsEntries(g_BattleActorArchiveBuffer);
        g_BattleSedsLoaded = 0;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BF3E8);
/* best 79.86%: everything below is retail's instruction set except one `lw`
   (retail re-reads g_BattleActorList for the pState->pTargetSprite store: `lw a0,0(s2)` after
   the two 0x74 stores, so its address lives in $s2 and the loaded head in $s0;
   ours CSEs the third read onto s0 and needs no second saved register) and the
   unkA8/flags load order.  The two `lhu` of the mask ARE two separate C
   expressions -- one per statement, with the D_800D3634 store between them --
   and the call argument must be the later statement's load, so the argument
   expression is written last.  The table row is 0x48 bytes at g_BattleStateWork+0x108
   (misc25 reaches the same rows as g_BattleActorCommandIds[g_BattleCommandRowIndex - 1]), field 0x46.
void func_800BF3E8(BattleActorSprite *p) {
    BattleActorSprite *pLast;
    BattleMisc33State *pState;
    s32 count;

    D_800D3634 = D_800C3FB8[g_BattleCommandRowIndex].slotMask;
    count = BattleCollectMaskedSlotActors(D_800C3FB8[g_BattleCommandRowIndex].slotMask, (void **)&g_BattleActorList, (s32)p);
    g_BattleActorListCount = count;
    if (count == 0) {
        g_BattleActorList = p;
    }
    pLast = g_BattleActorList;
    pState = g_BattleState;
    p->pTargetSprite = g_BattleActorList;
    ((BattleActorSprite *)g_BattleActorList)->pTargetSprite = p;
    pState->pTargetSprite = g_BattleActorList;
    pState->partKey = ((pLast->flags.word & 3) << 2) | (pLast->unkA8 >> 30);
    func_800223B0(p, BattleAngleToPoint(p, p->pTargetSprite));
    if (pLast->flags.sbytes[3] != 0x15) {
        func_800223B0(pLast, BattleAngleToPoint(p->pTargetSprite, p));
    }
}
*/
