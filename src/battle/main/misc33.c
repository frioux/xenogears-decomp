#include "common.h"

// Same value as include/battle/actor.h's BATTLE_ACTOR_SLOT_COUNT; duplicated here
// because this unit does not include that header.
#define BATTLE_ACTOR_SLOT_COUNT 11
#include "system/memory.h"
#include "psyq/libgte.h"
#include "field/actor.h"
#include "psyq/libetc.h"

// The number sprite BattleCreateNumberSpriteTask builds: how many frames it lives.
#define BATTLE_NUMBER_SPRITE_LIFETIME 32
// Per-frame step SpriteColorAddClamp is handed while a sprite fades out.
#define BATTLE_SPRITE_FADE_COLOR_STEP (-4)
// One bit per actor slot in the slot mask, matching the 11-entry per-slot tables.

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
    /* 0xA2 */ u16 targetHeight; // the target's vertical: BattleSetActorTargetFromPathPosition zeroes it for a ground target, so the 2D angle ignores it
    /* 0xA4 */ u16 targetY;
    /* 0xA6 */ u8 unkA6[0x2];
    /* 0xA8 */ u32 flags_0xA8; // the flag word SpriteData (include/field/actor.h) and every battle unit spell flags_0xA8; this unit shifts out its top two bits as the sprite's animation direction
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
    /* 0x20 */ s32 initialActorIndex; // saved at battle start; the init hands it to BattleSetCurrentActorSprite again
    /* 0x24 */ s32 actorIndex;
    /* 0x28 */ u32 partKey;
    /* 0x2C */ s32 pathIndex; // cursor into the target-position list at g_BattleStateWork + 0xA3C
    /* 0x30 */ s32 unk30;
    /* 0x34 */ s32 timer; // BattleIncrementStateTimer raises it, BattleUpdateStateStep counts it down once a frame
    /* 0x38 */ u8 unk38[0x8];
    /* 0x40 */ s32 savedActorSlot; // the slot BattleLoadActorSlotAnimation reloads when the battle ends
    /* 0x44 */ s32 targetDistance; // measured by BattleFaceTargetAndMeasureDistance
    /* 0x48 */ u8 actionDone; // set when the current step's action has finished; state 4 waits on it
    /* 0x49 */ u8 pendingStep; // stateStep copied by BattleApplyStateStep, run once and cleared
    /* 0x4A */ u8 commandUnhandled; // set by func_800B9C78's unknown-command arm, read and cleared by its next dispatch; the init clears it
    /* 0x4B */ u8 pad4B;
    /* 0x4C */ BattleActorSprite *pTargetSprite; // the actor BattleActorSnapToTargetAndFace is aimed at
} BattleMisc33State; // Size: 0x50

extern BattleMisc33State *g_BattleState;
extern s32 g_BattleCommandRowIndex;

// A 0x48-byte row of the per-battle actor table that g_BattleStateWork + 0x108 starts;
// misc25 indexes the same rows as g_BattleActorCommandIds[g_BattleCommandRowIndex - 1].
typedef struct {
    /* 0x00 */ u8 unk0[0x2F];
    /* 0x2F */ u8 commandCode; // 0xF7: queued but not dispatched yet, 0xFF: end of the queue (misc24 BattleGetQueuedCommandCode)
    /* 0x30 */ u8 unk30[0x16];
    /* 0x46 */ u16 slotMask;
} BattleActorRow; // Size: 0x48

extern BattleActorRow D_800C3FB8[];
// One 0x1C-byte row of the sensor table the battle state work block starts with; a
// sprite's row index is its animation direction (flags_0xA8>>30) and part key (flags&3).
typedef struct {
    /* 0x00 */ u8 unk0[0x7];
    /* 0x07 */ u8 flags; // only masked and tested: bit 7 by BattleActorSlotHasFlag80, clear means idle
    /* 0x08 */ u8 inUse; // set to 1 when the slot is claimed; every reader tests it against 0 (misc29/misc34 BattleActorSlot.inUse)
    /* 0x09 */ u8 unk9;
    /* 0x0A */ u8 sideFlag; // non-zero faces the slot's sprite 0x800 (half a turn); the angle misc30's BattleActorSetFacingFromSideFlag passes to func_800223B0
    /* 0x0B */ u8 unkB[0x3];
    /* 0x0E */ u16 x; // whole x the slot's sprite takes its 16.16 position from (misc29 BattleActorInitSpriteFromSlot)
    /* 0x10 */ u16 z;
    /* 0x12 */ u8 unk12[0xA];
} BattleSpriteSensorRow; // Size: 0x1C
extern BattleActorSprite *g_BattleActorList;
extern u8 g_BattleBlockingEffectActive;
extern s16 g_BattleDigitTextureIds[];
extern s32 g_BattleColorMode;
extern u8 *g_BattleSpriteArchive;
extern s16 g_BattleActorSlotMask;
extern s16 g_BattleActorListCount;
extern s32 BattleCollectMaskedSlotActors(u16 mask, void **pOut, s32 arg2);
extern void BattleUpdateStateStep();
extern s32 D_800C3E20;
extern u16 g_BattleSlotBitMask;
extern u8 g_BattleSedsLoaded;
extern u8 g_BattleHexDigitTable[];
extern u8 g_BattleStateWork[];
extern u32 g_BattleDecimalDivisors[];
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
    /* 0x08 */ u8 unk8[0x2];
    /* 0x0A */ u16 texPage; // the texture page BattleDrawSpritePartQuad copies into the primitive's tpage
    /* 0x0C */ u16 clutID; // and the clut id it copies into the primitive's clut
    /* 0x0E */ u8 unkE[0xA];
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
extern s32 BattleWriteDecimalDigits(s32 value, u8 *pBuf, s32 n, u8 started, s32 base);
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
    p->colorR = SpriteColorAddClamp(p->colorR, BATTLE_SPRITE_FADE_COLOR_STEP);
    p->colorG = SpriteColorAddClamp(p->colorG, BATTLE_SPRITE_FADE_COLOR_STEP);
    p->colorB = SpriteColorAddClamp(p->colorB, BATTLE_SPRITE_FADE_COLOR_STEP);
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
extern void BattleDrawSpritePartQuad(BattleSpritePart *pPart, u32 color);

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
            BattleDrawSpritePartQuad(pPart, pBody->color.word);
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

    pBody = WorkListsAddTasks(sizeof(BattleSpriteBody), NULL, BattleStepActorFadeOut, BattleRenderSpriteBody, NULL);
    step = ((rand() & 3) - 2) * 8;
    pBody->rotZStep = -step;
    if (pBody->rotZStep == 0) {
        pBody->rotZStep = 6;
    }
    pBody->countdown = BATTLE_NUMBER_SPRITE_LIFETIME;
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
    BattleWriteDecimalDigits(value, digits, sizeof(digits) - 1, 0, 0);
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
    g_BattleBlockingEffectActive = 1;
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
    g_BattleBlockingEffectActive = 0;
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
// Writes value as decimal digits into pBuf: pBuf[0] receives the number of
// characters written and pBuf[1..] hold the digits (a leading '-' for a negative
// value). n is the number of leading places leading zeros are suppressed in, so
// the widest field is n + 1 characters; started != 0 keeps those leading zeros,
// and base is added to every digit. Returns the number of characters written.
// g_BattleDecimalDivisors holds the decimal place divisors, 10^1 through 10^9.
s32 BattleWriteDecimalDigits(s32 value, u8 *pBuf, s32 n, u8 started, s32 base) {
    u8 *pDst;
    u8 digit;
    s32 count;
    u32 *pPlace;
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
    pPlace = g_BattleDecimalDivisors;
    pTab = pPlace + n;
    for (;;) {
        value = (u32)value % *pTab;
        if (n == 0) {
            break;
        }
        pTab--;
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
    }
    *pDst = value + base;
    pBuf[0] = count + 1;
    return count + 1;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BE790);
extern u8 g_BattleStageFileLoadDone;
extern u8 g_BattleLoadedStageFileId;
extern u8 g_BattleRequestedStageFileId;
extern void BattleWaitArchiveSync(void);
extern int ArchiveGetArchiveOffsetIndices(s32 *pAligned, s32 *pRemainder);

// Reloads the battle backdrop sprite sheet when g_BattleRequestedStageFileId names a different
// sheet than the one g_BattleLoadedStageFileId records; the file goes to the fixed upload buffer.
void BattleReloadStageFile(void) {
    s32 alignedIndex;
    s32 remainder;
    u8 loaded;
    u8 sheet;

    loaded = g_BattleLoadedStageFileId;
    sheet = g_BattleRequestedStageFileId;
    if (loaded != sheet) {
        g_BattleLoadedStageFileId = sheet;
        BattleWaitArchiveSync();
        ArchiveGetArchiveOffsetIndices(&alignedIndex, &remainder);
        ArchiveSetIndex(0xC, 2);
        ArchiveReadFileToBuffer(sheet + 2, (void *)0x801FC000, 0, 0x80);
        BattleWaitArchiveSync();
        ArchiveSetIndex(alignedIndex, remainder);
        DrawSync(0);
        Vsync(0);
        EnterCriticalSection();
        FlushCache();
        ExitCriticalSection();
    }
    g_BattleStageFileLoadDone = 1;
}
typedef struct {
    /* 0x00 */ u16 cur;
    /* 0x02 */ u16 pressed;
    /* 0x04 */ u16 released;
    /* 0x06 */ u16 vsyncCount; // D_800D30E4, the vsync counter func_800BE790 records
} BattlePadFrame; // Size: 0x8

typedef struct {
    /* 0x0000 */ u8 unk0[0xC50];
    /* 0xC50 */ u8 *pRenderEnv; // one of the two double-buffered env blocks: DRAWENV at +0, DISPENV at +0x5C
    /* 0xC54 */ u_long *pCurrentOT; // that block's ordering table at +0x70; what GfxSetCurrentOT is handed
    /* 0x0C58 */ u16 cur[2];
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
    if (pState->cur[0] & PADselect) {
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
    pState->history[0].vsyncCount = D_800D30E4;
}

void BattleResetStateGlobals(void) {
    D_800C3E20 = 0;
    g_BattleState = NULL;
    g_BattleSlotBitMask = 0;
}
BattleMisc33State *BattleCreateState(void) {
    // retail re-reads g_BattleState once per group of stores; one local for the
    // whole body makes the reloads land in other registers (78.15%).
    BattleMisc33State *pState;
    BattleMisc33State *pPending;
    BattleMisc33State *pTimer;

    g_BattleState = HeapAlloc(sizeof(BattleMisc33State), 0);
    g_BattleState->pUpdate = BattleUpdateStateStep;
    g_BattleCommandRowIndex = 0;
    g_BattleState->commandUnhandled = 0;
    pState = g_BattleState;
    pState->actionDone = 1;
    pState->unk30 = 0;
    pPending = g_BattleState;
    pPending->pendingStep = 0;
    pPending->pathIndex = 1;
    pPending->pCurSprite = NULL;
    pTimer = g_BattleState;
    pTimer->timer = 0;
    BattleSetStateStep(0);
    g_NumFlaggedTimerWorkListEntries = 0;
    g_FlagTimerWorkListAdds = 1;
    return g_BattleState;
}

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
    } while (i != BATTLE_ACTOR_SLOT_COUNT);
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
void BattleSetCurrentActorSprite(s32 actorIndex) {
    BattleActorSprite *pSprite;
    u8 *pBase;
    u32 part;

    pSprite = g_BattleState->pCurSprite;
    if (pSprite != NULL && g_BattleState->actorIndex != actorIndex) {
        part = pSprite->flags_0xA8 >> 30;
        if (((BattleSpriteSensorRow *)g_BattleStateWork)[((g_BattleState->pCurSprite->flags.word & 3) << 2) | part].flags == 0) {
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

// One 6-byte entry of the path-position list an actor walks: the point it moves to
// and a flag that makes it face the point from further away.
typedef struct {
    /* 0x0 */ u16 x;
    /* 0x2 */ u16 y;
    /* 0x4 */ u8 blocked;
} BattleTargetPosition; // Size: 0x6

// The battle work buffer up to the path-position list an actor walks; the list runs on
// past this view.
typedef struct {
    /* 0x0000 */ u8 unk0[0xA3C];
    /* 0x0A3C */ BattleTargetPosition pathPositions[0];
} BattleWorkPathPositions;

#define BATTLE_TARGET_POSITION_NONE 0xFFFF
#define BATTLE_PATH_POSITION(index) \
    (((BattleWorkPathPositions *)g_BattleStateWork)->pathPositions[(index)])

extern void BattleStepActorAlongPath(BattleActorSprite *p, void *pTarget);

// The last state of an actor's walk along the path-position list: while the list has a
// point left it copies that point into the sprite and faces it, and once the list is
// exhausted it aims the sprite at its own position.
void BattleSetActorTargetFromPathPosition(BattleActorSprite *p) {
    s32 posX;
    s32 posY;
    s32 faceArg;

    if (BATTLE_PATH_POSITION(g_BattleState->pathIndex).x == BATTLE_TARGET_POSITION_NONE &&
        BATTLE_PATH_POSITION(g_BattleState->pathIndex).y == BATTLE_TARGET_POSITION_NONE) {
        posX = p->x;
        posY = p->y;
        p->targetHeight = 0;
        p->targetX = posX;
        p->targetY = posY;
        BattleStepActorAlongPath(p, p->pTargetSprite);
    } else {
        p->targetX = BATTLE_PATH_POSITION(g_BattleState->pathIndex).x;
        p->targetY = BATTLE_PATH_POSITION(g_BattleState->pathIndex).y;
        p->targetHeight = 0;
        faceArg = 2;
        if (BATTLE_PATH_POSITION(g_BattleState->pathIndex).blocked != 0) {
            faceArg = 3;
        }
        BattleFaceTargetAndMeasureDistance(p, faceArg);
        g_BattleState->pathIndex++;
    }
}
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
    part = pSprite->flags_0xA8 >> 30;
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
/*
PARKED 85.91 percent at retail's exact 264 bytes / 66 instructions, the best of four
   spellings measured this round and every register right (pLast in s0, pState in a1,
   the reload in a0). Residual is three sched1/allocator interleaves, not source order:
   retail hoists the %hi(g_BattleStateWork) lui above the %hi(g_BattleCommandRowIndex)
   one and keeps 0x14E in the load's DISPLACEMENT (addiu v1,v1,%lo only), while this
   body folds the offset into the addiu (addiu v1,v1,334 + lhu 0(v0)) and orders the
   mask store before the second load; retail gives the jal delay slot to that sh and
   puts move a2,s1 up with the saves; and this body hands pState a0 instead of a1 and
   stores 0x74(a0) above the two flag loads. This unit runs -fschedule-insns, so
   statement order does not move those (a do-while grouping measured 85.82, and two
   4-minute permuter runs from this body found only reorderings, base 970 best 810 and
   base 1020 best 815).
   MEASURED (round 437, all 264 bytes / 66 insns unless noted): a pointer local for the
   row, pWork = g_BattleStateWork + g_BattleCommandRowIndex * 0x48 then
   *(u16 *)(pWork + 0x14E), DOES reproduce retail's displacement form (addiu v1,v1,%lo
   with lhu 334(v0)) but it moves the index multiply into v1 off v0, gives pState a0
   and hoists the reload: 76.32. The same pointer local with the plain global read
   g_BattleState->pTargetSprite = g_BattleActorList drops the second g_BattleActorList
   load entirely (63 insns / 252 bytes, 71.50) and puts the table address in s0, so the
   volatile lvalue at the read is load-bearing. The volatile cast is the only spelling
   that emits the reload (rounds 411 and 424 agree).
   The mask is g_BattleStateWork + g_BattleCommandRowIndex * 0x48 + 0x14E, i.e.
   D_800C3FB8 + 0x46; D_800C3FB8 is not in config/symbol_addrs.battle.txt, so keep the
   base spelling and keep the offset inside the expression.
*/
/*
void func_800BF3E8(BattleActorSprite *p) {
    BattleActorSprite *pLast;
    BattleMisc33State *pState;
    s16 count;
    u32 dir;

    g_BattleActorSlotMask = *(u16 *)(g_BattleStateWork + g_BattleCommandRowIndex * 0x48 + 0x14E);
    count = BattleCollectMaskedSlotActors(*(u16 *)(g_BattleStateWork + g_BattleCommandRowIndex * 0x48 + 0x14E), (void **)&g_BattleActorList, (s32)p);
    g_BattleActorListCount = count;
    if (count == 0) {
        g_BattleActorList = p;
    }
    pLast = g_BattleActorList;
    pState = g_BattleState;
    p->pTargetSprite = g_BattleActorList;
    pState->pTargetSprite = *(BattleActorSprite *volatile *)&g_BattleActorList;
    dir = pLast->flags_0xA8 >> 30;
    pState->partKey = ((pLast->flags.word & 3) << 2) | dir;
    ((BattleActorSprite *)g_BattleActorList)->pTargetSprite = p;
    func_800223B0(p, BattleAngleToPoint(p, p->pTargetSprite));
    if (pLast->flags.sbytes[3] != 0x15) {
        func_800223B0(pLast, BattleAngleToPoint(p->pTargetSprite, p));
    }
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BF3E8);
