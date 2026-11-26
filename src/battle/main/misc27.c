#include "common.h"
#include "field/actor.h"
#include "battle/actor.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"

extern void TimerWorkListSetTaskCallback();
extern void WorkListSetTaskCallback();
extern void func_8001E148();
extern void AnimScriptTick(void* pSprite);
extern s32 SpriteScaleValue(void* pSprite, s32 value);
extern void BattleEmitTrailSegments();
extern void func_800B3CD4();
extern void func_800B50D4();
extern void BattleUpdateDirectionTransformTask();
extern u8 D_800C356C[];
extern u8* D_800D2FD8;
extern s16 D_800C3E9C[];
extern s16 D_800C3D4C[];
extern s16 D_800D3334[];
extern u8* g_GfxCurWorkBuffer;
extern u8* g_GfxCurWorkBufferEnd;
extern u_long* g_GfxCurOT;
extern void BattleIntegrateSpriteVelocityTask();
extern void BattleDrawActorLinePrim();
extern void BattleDrawLinkedActorLinePrim();
extern void BattleGetActorJointOffset();
extern void BattleUpdateDistanceWaitTask();
extern u8* TimerWorkListAllocateTask();

typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x1 */ s8 unk1;
    /* 0x2 */ s8 unk2;
    /* 0x3 */ u8 unk3;
    /* 0x4 */ u8 unk4;
    /* 0x5 */ u8 unk5;
} BattleScriptOp;

// WorkListEntry of include/system/animation.h; an overlay unit cannot include that header
// (it holds tentative definitions of main-exe globals), so the layout is repeated. Both
// entries WorkListsAddTasks fills in point unk4 back at the block they were allocated in.
typedef struct BattleDirectionTask BattleDirectionTask;

typedef struct WorkListEntry {
    /* 0x00 */ void* unk0;
    /* 0x04 */ BattleDirectionTask* pTask;
    /* 0x08 */ void* onTriggerCallback;
    /* 0x0C */ void (*onFreeCallback)(void*);
    /* 0x10 */ u32 unk10;
    /* 0x14 */ u32 unk14;
    /* 0x18 */ struct WorkListEntry* pNext;
} WorkListEntry; // Size: 0x1C

// The 0xB8-byte block WorkListsAddTasks allocates for the direction-transform task: the
// timer and work entries, then the task's own fields.
struct BattleDirectionTask {
    /* 0x00 */ WorkListEntry timerTask;
    /* 0x1C */ WorkListEntry workTask;
    /* 0x38 */ SpriteData* pSpriteData;
    /* 0x3C */ u32 curSpriteFrame;
    /* 0x40 */ s32 animId;
    /* 0x44 */ u32 semiTrans;
    /* 0x48 */ s32 radius;
    /* 0x4C */ u8* pArgs;
    /* 0x50 */ SVECTOR targetPos[5];
    /* 0x78 */ SVECTOR curPos[5];
    /* 0xA0 */ u8 unkA0[0x18];
}; // Size: 0xB8

// One entry of the direction-transform table SpriteData.pBase points at
// (SpriteDirectionTransforms in include/field/actor.h): retail loads the translation as
// s8 and the rotations as u16, so this unit views the table with that signedness instead
// of changing the shared header, which other units depend on.
typedef struct {
    /* 0x0 */ s8 translationX;
    /* 0x1 */ s8 translationY;
    /* 0x2 */ u16 rotationX;
    /* 0x4 */ u16 rotationY;
    /* 0x6 */ u16 rotationZ;
} BattleDirectionTransform; // Size: 0x8

extern BattleDirectionTask* WorkListsAddTasks(s32 allocSize, void* pData, void* pTimerCallback,
                                              void* pWorkCallback, void* pOnFreeCallback);

// One 0x1C-byte slot of the battle actor table at g_BattleStateWork, and the
// animation frame the battle anim script keeps.
typedef struct {
    /* 0x00 */ u8 unk0[0x6];
    /* 0x06 */ u8 unk6;
    /* 0x07 */ u8 flags;
    /* 0x08 */ u8 inUse;
    /* 0x09 */ u8 unk9[0x5];
    /* 0x0E */ u16 x;
    /* 0x10 */ u16 z;
    /* 0x12 */ u8 unk12[0xA];
} BattleScriptActorSlot; // Size: 0x1C

typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ s16 x;
    /* 0x06 */ s16 y;
    /* 0x08 */ s16 z;
    /* 0x0A */ u8 unkA[0x2];
    /* 0x0C */ s16 xB;
    /* 0x0E */ s16 yB;
    /* 0x10 */ s16 zB;
    /* 0x12 */ u8 unk12[0xE];
} BattleAnimFramePos;

// A battle sound-effect bank: the id the script's sound opcodes play from is the
// 0x14 halfword, which the effect id packs into the high half of the argument.
typedef struct {
    /* 0x00 */ u8 unk0[0x14];
    /* 0x14 */ u16 bankId;
} BattleSoundBank;

// The battle actor slot a sprite occupies: flags_0xA8 holds the high bits of the
// index, flags_0xAC the low two.
#define BATTLE_SCRIPT_SLOT_INDEX(pSpriteData)                                                    \
    (((u32)(pSpriteData)->flags_0xA8 >> 30) | (((u32)(pSpriteData)->flags_0xAC.flags & 3) << 2))

// One SVECTOR per trail corner: the direction-transform table, the corner array and the
// segment count BattleEmitTrailSegments draws are all this long.
#define BATTLE_TRAIL_SEGMENTS 5

// The 0x3C flag word is read and written whole here, which the shared header's
// bitfield spelling cannot express (src/battle/main/misc27_b.c does the same).
#define BATTLE_SCRIPT_FLAGS(pSpriteData) (((BattleSpriteState*)(pSpriteData))->flags)

extern u8 D_800C3564;
// The sprite the battle script is currently animating; only its slot index is read.
extern SpriteData* D_800C3E1C;
extern u8 D_800C3664;
extern u8 D_800C3688;
extern u8 D_800C3CAC;
extern u8 D_800C3CB0[3];
extern u16 D_800C3626;
extern u16 D_80059454;
extern s32 g_DebugTable;
extern u8 g_BattleFrameDrawSuppressed;
extern u8 g_BattleAudioFadeStarted;
extern u8 g_BattleScreenFadeBlocked;
extern u8 g_BattleSlotDrawSuppressed;
extern u8 g_BattleActorArchiveLoadPending;
extern u8 g_BattleActorArchiveReady;
extern s32* g_BattleAudioManager;
extern u8 g_BattleStateWork[];
extern BattleSoundBank* g_BattleSoundBankMain;
extern SVECTOR g_BattleSlotVectors;
extern SVECTOR g_BattleSlotVectorsB;
extern BattleAnimFramePos g_BattleAnimFrame;
extern s16 g_BattleActorDelayCounter;
extern s16 g_BattleDelayCounter;
extern u16 g_BattleActorSlotMask;
extern s32 g_AnimScriptExtraTicks;
extern void* D_800C3568;

extern void func_800B3E04();
extern void SoundSetAudioManagerVolumeWithFade(s32* pAudioManager, s32 volume, s32 frames);
extern void SpriteSetAnimFile(void* pSpriteData, void* pAnimFile);
extern void BattleAttachCommandRowEffect(s32 slot);
extern void BattleActorUpdateFallMotion(SpriteData* pSpriteData);
extern void BattleSetBattleStateBit(u8 bit);
extern void SoundPlaySoundEffectOnFreePair(u32 sound);
extern void func_801FC898();
extern void SoundPlaySoundEffectOnLastPair(u32 sound);
extern void SoundPlaySoundEffectOnPair(u32 sound, u32 arg1);
extern void SoundStopElementsById(u32 sound);
extern void func_800B61F8(SpriteData* pSpriteData, u8* pArgs);
extern void func_800B626C(SpriteData* pSpriteData, u8* pArgs);
extern void func_800B62C8(SpriteData* pSpriteData, u8* pArgs);
extern void BattleRemapActorModelId(SpriteData* pSpriteData);
extern void BattleScriptApplyRectParams(SpriteData* pSpriteData, u8* pArgs);
extern void BattleScriptLoadDataAtWordOffset(SpriteData* pSpriteData, u8* pArgs);
extern void SpritePlayAnimation(SpriteData* pSpriteData, u32 animId);
extern void BattleSetPendingCommand(SpriteData* pSpriteData);
extern void BattleSetTaskCallback(SpriteData* pSpriteData, u8* pArgs);
extern void BattleScriptOffsetFrameCluts(SpriteData* pSpriteData, u8* pArgs);
extern void BattleScriptPlaySpriteAnimation(SpriteData* pSpriteData, u8* pArgs);
extern void BattleActorUpdateGroundHeightAndTriangle(SpriteData* pSpriteData);
extern void BattleActorFaceTarget(SpriteData* pSpriteData, u8* pArgs);
extern void func_800B65B0(SpriteData* pSpriteData, u8* pArgs);
extern void BattleActorSetDirectionTowardPosition(SpriteData* pSpriteData, u8* pArgs);
extern void BattleActorCopyVectorFromSource(SpriteData* pSpriteData, u8* pArgs);
extern void WorkListsRemoveEntriesForData(void* pData);
extern void BattleScriptLoadFixed3(SpriteData* pSpriteData, u8* pArgs);
extern void BattleReadScriptHalfwords3(s16* pOut, u8* pArgs);
extern void BattleScriptAddHalfwords3(s16* pOut, u8* pArgs);
extern void BattleScriptCall8001FC4C4WithActorState(SpriteData* pSpriteData, u8* pArgs);
extern void BattleScriptCall8001FC53CWithScaledBytes(SpriteData* pSpriteData, u8* pArgs);
extern void BattleMoveScreenImageToVram(SpriteData* pSpriteData, u8* pArgs);
extern void SpriteSetDirection(SpriteData* pSpriteData, s32 direction);
extern void BattleActorRotateVectorZ(SpriteData* pSpriteData, u8* pArgs);
extern void BattleSetRotationZFromVelocity(SpriteData* pSpriteData, u8* pArgs);
extern void BattleSetRotationXFromVelocity(SpriteData* pSpriteData, u8* pArgs);
extern void BattleSetRotationFromVelocity(SpriteData* pSpriteData, u8* pArgs);
extern void SpriteEnsureDirTransforms(SpriteData* pSpriteData);
extern void* AnimScriptResolveVariablePointer(SpriteData* pSpriteData, u8* pArgs);
extern void func_801FC7B0(SpriteData* pSpriteData, u8* pArgs);
extern void func_801FC6FC(SpriteData* pSpriteData, u8* pArgs);
extern WorkListEntry* TimerWorkListFindEntryForDataAndCallback(void* pData, void* pCallback);
extern WorkListEntry* TimerWorkListFindEntryByCallback(void* pCallback);
extern void BattleCreateSpriteFollowTask(SpriteData* pSpriteData, u8* pArgs);
extern void BattleUpdateSpriteFollowTask();
extern void BattleSetLinkedSpriteDrawWorkCallback(SpriteData* pSpriteData, u8* pArgs);
extern void BattleStartSpriteMotionTask(SpriteData* pSpriteData);
extern void BattleSetSpriteDrawWorkCallback(SpriteData* pSpriteData, u8* pArgs);
extern void BattleBeginAnimTask();
extern void BattleClearRecordTable8(SpriteData* pSpriteData, u8* pArgs);

// The battle animation script interpreter: `mode` selects the opcode, which mostly
// moves one value in or out of the sprite and its anim script state. 0 is not an
// opcode; 1..0x6B are. A 107-case switch, almost every case two to six instructions.
// 95.28% (4084 bytes / 1021 insns against retail's 4056 / 1014).
// THE FRAME IS SOLVED (round 506): retail's 0x68 frame with vecA at 0x38 is a DEAD 0x28-byte
// array local declared before vecA - `u_long scratch[10];`, never referenced, still gets a slot
// at expand time (levers.md [j-an-unused-local-array-is]). The frame becomes 0x68, vecA/vecB land
// on retail's 0x38/0x48, the prologue/epilogue and all 12 case-103 vector accesses become
// instruction-identical, and 965 differing lines drop to 921. Only the size of the dead array
// matters (0x28 = 40 bytes, rounded to 8 like every addressable local); its position must be
// before vecA. This is the largest frame that lever has closed (previous rounds closed 8/16/24).
// WHERE THE 28 SURPLUS BYTES GO, measured on the objdiff instruction diff: case 6 (mode 6) does
// NOT cross-jump into case 7's shared tail the way retail does. Retail is 'lui v0,%hi(D_800C3E1C);
// lw v1,%lo(...); j .L800B4E08' with the slot-index tail emitted ONCE at case 7 (.L800B4E04:
// lw v1,0x74(s1) falls into it), so retail spends 4 instructions there and this body spends 13
// (+9). Both spellings of case 6 (through a pAnimSprite local, and D_800C3E1C inline) leave the
// two copies unmerged, and the rest of the body is 2 instructions SHORT of retail (case 52's
// duplicate sw plus one more), i.e. 13-4=+9 against -2: cross-jumping is the whole size delta.
// THE SLOT-INDEX MACRO IS A TWO-WAY SCHEDULER TIE, MEASURED. BATTLE_SCRIPT_SLOT_INDEX as (flags_0xA8 >> 30) | ((flags_0xAC.flags & 3) << 2) is what this park holds: 95.26% / 4084 bytes / 1021 insns. The mirrored order ((flags_0xAC.flags & 3) << 2) | (flags_0xA8 >> 30) scores 94.61% / 4048 bytes / 1012 insns (8 bytes SHORT of retail, fewest differing lines). Retail loads 0xA8 (0x168) FIRST and yet has the 0xAC half as the FIRST operand of the or (or a0,a0,v0); each spelling matches one and misses the other at all six sites, so the load order is post-reload scheduling (this preset has -fschedule-insns and -insns2), not the expression. A statement-expression accumulator - open brace, u32 i = AC << 2, i |= A8 >> 30, i, close brace - matches retail's or operand order at every site but scores 93.18% / 4060 bytes / 1015 insns; applied to case 98 ALONE it reproduces retail's or a0,a0,v0 exactly while the two loads then order opposite.
// CASE 97 FIXED THIS ROUND: the s16 position halves must be read through an s32 local (targetX = xWhole + value, then field_0xA0 = targetX, likewise targetZ for field_0xA4). Stored straight into the 16-bit field, gcc narrows the loads to lhu; retail has lh 2(a2) and lh 0xA(a2). Took the arm from 6 differing instructions to 3, and the 3 left are allocator: retail stores through a0 after move a0,s1 where this body uses s1, and retail has a nop between lh and addu.
// CASE 52 owns retail's missing store: retail has BOTH sw v0,0x1C(s1) (the multiplicand, before mflo) and sw t1,0x1C(s1) (the product) for one assignment - a gcc expand_mult artifact no source spelling reaches (4 spellings tried).
// CASE 96: retail reads flags_0xAC with lw where every spelling here gets lbu (only the low two bits reach the use, so gcc narrows). Same value, different instruction.
// REGISTER-LEVEL ONLY (same instructions, different registers): case 23 keeps one extra move a1,a0; cases 40, 68, 69, 77, 78, 79, 82, 86 and 98 hold the same values in different registers.
// Established earlier and still true: vecA/vecB are VECTOR, case 100 sets flags_0xB0 bit 0x200, case 97 divides by 0x1000 with div, case 48 re-forms the direction-transform address per access, g_BattleActorSlotMask is u16, the sound calls pack id | (bank << 16).
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B3F04);
/*
void func_800B3F04(SpriteData* pSpriteData, s32 mode, u8* pArgs) {
    u_long scratch[10];
    BattleScriptActorSlot* pSlot;
    SpriteData* pAnimSprite;
    SpriteDataI1* pBase;
    WorkListEntry* pWork;
    SpriteDirectionTransform* pTransform;
    BattleSoundBank* pBank;
    VECTOR vecA;
    VECTOR vecB;
    s32 scale;
    s32 value;
    s32 targetX;
    s32 targetZ;
    s32 i;

    switch (mode) {
    case 107:
        func_800B3E04();
        break;
    case 106:
        g_BattleFrameDrawSuppressed = 1;
        SoundSetAudioManagerVolumeWithFade(g_BattleAudioManager, 0, 0x78);
        g_BattleAudioFadeStarted = 0;
        break;
    case 105:
        if (((u32)pSpriteData->flags_0xB0.flags >> 10) & 1) {
            SpriteSetAnimFile(pSpriteData, pSpriteData->pSpecialAnimFile);
            pSpriteData->flags_0xB0.flags |= 0x400;
        } else {
            SpriteSetAnimFile(pSpriteData, pSpriteData->pDefaultAnimFile);
            pSpriteData->flags_0xB0.flags &= ~0x400;
        }
        break;
    case 104:
        if ((s8)pSpriteData->flags_0xAC.bytes[3] < 0) {
            SpriteSetAnimFile(pSpriteData, pSpriteData->pSpecialAnimFile);
            pSpriteData->flags_0xB0.flags |= 0x400;
        } else {
            SpriteSetAnimFile(pSpriteData, pSpriteData->pDefaultAnimFile);
            pSpriteData->flags_0xB0.flags &= ~0x400;
        }
        break;
    case 102:
        D_800C3688 = 1;
        break;
    case 100:
        pSpriteData->flags_0xB0.flags |= 0x200;
    case 99:
        if ((s16)D_80059454 > 0x200) {
            pSpriteData->field_0x3A = D_80059454;
            pSpriteData->field_0x40 = (pSpriteData->field_0x40 & 0xFFFFE0FF) | 0x300;
            BATTLE_SCRIPT_FLAGS(pSpriteData) |= 0x10000000;
        }
        break;
    case 98:
        BattleAttachCommandRowEffect((s32)BATTLE_SCRIPT_SLOT_INDEX(pSpriteData));
        break;
    case 97:
        pAnimSprite = pSpriteData->pTargetEntitySprite;
        pSpriteData->position.y = (pSpriteData->field_0x84 - 1) << 16;
        value = (s8)pArgs[0] * pSpriteData->scale / 0x1000;
        pSlot = (BattleScriptActorSlot*)(g_BattleStateWork +
                                         BATTLE_SCRIPT_SLOT_INDEX(pAnimSprite) * sizeof(BattleScriptActorSlot));
        if (((BattleSpriteState*)pAnimSprite)->position.xWhole == pSlot->x) {
            if (!(((u32)pSpriteData->flags_0xAC.flags >> 2) & 1)) {
                value = -value;
            }
        } else if (((BattleSpriteState*)pAnimSprite)->position.xWhole < pSlot->x) {
            value = -value;
        }
        targetX = ((BattleSpriteState*)pAnimSprite)->position.xWhole + value;
        pSpriteData->field_0xA0 = targetX;
        pSpriteData->field_0xA2 = 0;
        targetZ = ((BattleSpriteState*)pAnimSprite)->position.zWhole;
        pSpriteData->field_0xA4 = targetZ;
        BattleActorUpdateFallMotion(pSpriteData);
        break;
    case 95:
        D_800C3564 = 1;
        break;
    case 96:
        D_800C3564 = BATTLE_SCRIPT_SLOT_INDEX(pSpriteData) + 2;
        break;
    case 89:
        g_BattleScreenFadeBlocked = 0;
        break;
    case 90:
        g_BattleScreenFadeBlocked = 1;
        break;
    case 87:
        BattleSetBattleStateBit(1);
        break;
    case 88:
        BattleSetBattleStateBit(0);
        break;
    case 86:
        if (g_BattleActorArchiveReady != 0) {
            pSlot = (BattleScriptActorSlot*)(g_BattleStateWork +
                                             BATTLE_SCRIPT_SLOT_INDEX(pSpriteData) * sizeof(BattleScriptActorSlot));
            value = pSlot->unk6;
            if (value == 9) {
                value = 2;
            }
            if (value == 0xA) {
                value = 6;
            }
            if (value == 8) {
                value = 0xF;
            }
            if (((D_800C3626 >> BATTLE_SCRIPT_SLOT_INDEX(pSpriteData)) & 1) == 0) {
                D_800C3626 |= 1 << BATTLE_SCRIPT_SLOT_INDEX(pSpriteData);
                SoundPlaySoundEffectOnFreePair((value + 0x52) | (g_BattleSoundBankMain->bankId << 16));
            }
        }
        break;
    case 84:
        func_801FC898();
        break;
    case 79:
        pBank = (BattleSoundBank*)pSpriteData->field_0x50;
        if (pBank != NULL) {
            SoundPlaySoundEffectOnLastPair(pArgs[0] | (pBank->bankId << 16));
        }
        break;
    case 82:
        pBank = (BattleSoundBank*)pSpriteData->field_0x50;
        if (pBank != NULL) {
            SoundPlaySoundEffectOnPair(pArgs[0] | (pBank->bankId << 16), pArgs[1]);
        }
        break;
    case 77:
        pBank = (BattleSoundBank*)pSpriteData->field_0x50;
        if (pBank != NULL) {
            SoundStopElementsById(pArgs[0] | (pBank->bankId << 16));
        }
        break;
    case 78:
        if (g_BattleSoundBankMain != NULL) {
            SoundStopElementsById(pArgs[0] | (g_BattleSoundBankMain->bankId << 16));
        }
        break;
    case 76:
        pSpriteData->field_0x40 |= 0x2;
        break;
    case 72:
        pSpriteData->field_0x40 |= 0x1;
        break;
    case 103:
        pSpriteData->animScriptWaitTimer++;
        vecA.vx = g_BattleAnimFrame.x - g_BattleSlotVectors.vx;
        vecA.vy = g_BattleAnimFrame.y - g_BattleSlotVectors.vy;
        vecA.vz = g_BattleAnimFrame.z - g_BattleSlotVectors.vz;
        Square0(&vecA, &vecA);
        vecB.vx = g_BattleAnimFrame.xB - g_BattleSlotVectorsB.vx;
        vecB.vy = g_BattleAnimFrame.yB - g_BattleSlotVectorsB.vy;
        vecB.vz = g_BattleAnimFrame.zB - g_BattleSlotVectorsB.vz;
        Square0(&vecB, &vecB);
        if (SquareRoot0(vecA.vx + vecA.vy + vecA.vz) < 4) {
            if (SquareRoot0(vecB.vx + vecB.vy + vecB.vz) < 4) {
                break;
            }
        }
        pSpriteData->pSpriteBytecode = (u8*)pSpriteData->pSpriteBytecode - 2;
        break;
    case 80:
        pSpriteData->animScriptWaitTimer++;
        if (g_BattleActorDelayCounter != 0) {
            g_BattleActorDelayCounter--;
            break;
        }
        pSpriteData->pSpriteBytecode = (u8*)pSpriteData->pSpriteBytecode - 2;
        break;
    case 70:
        func_800B61F8(pSpriteData, pArgs);
        g_BattleDelayCounter = 0;
        break;
    case 81:
        func_800B626C(pSpriteData, pArgs);
        break;
    case 71:
        func_800B62C8(pSpriteData, pArgs);
        break;
    case 68:
        if (g_DebugTable != -1) {
            D_800C3568 = pSpriteData;
        }
        break;
    case 69:
        if (g_DebugTable != -1) {
            D_800C3568 = NULL;
        }
        break;
    case 67:
        BATTLE_SCRIPT_FLAGS(pSpriteData) |= 0x80000000;
        break;
    case 62:
        BattleRemapActorModelId(pSpriteData);
        break;
    case 61:
        BattleScriptUploadSpriteFrame(pSpriteData);
        break;
    case 58:
        BattleScriptApplyRectParams(pSpriteData, pArgs);
        break;
    case 66:
        pSpriteData->flags_0xAC.flags &= ~0x20;
        break;
    case 56:
        BattleScriptLoadDataAtWordOffset(pSpriteData, pArgs);
        break;
    case 55:
        g_BattleActorArchiveLoadPending = 1;
        break;
    case 52:
        value = (s8)pArgs[0] * 2 * pSpriteData->field_0x82 / 0x1000;
        pSpriteData->gravity = (value << 5) * ((g_AnimScriptExtraTicks + 1) * (g_AnimScriptExtraTicks + 1));
        break;
    case 51:
        if (pSpriteData->pDefaultAnimFile != NULL) {
            SpritePlayAnimation(pSpriteData, pArgs[0]);
        }
        break;
    case 49:
        SetGeomOffset(0xA0, 0x70);
        break;
    case 50:
        SetGeomOffset(0xA0, 0xA4);
        break;
    case 48:
        pBase = pSpriteData->pBase;
        if (pBase != NULL) {
            if ((pSpriteData->field_0x3C_0 & 3) == 1) {
                if (pBase->pDirTransforms != NULL) {
                    pBase->offsetY =
                        ((BattleDirectionTransform*)pSpriteData->pBase->pDirTransforms)[pArgs[0]].translationY;
                    pBase->offsetX =
                        ((BattleDirectionTransform*)pSpriteData->pBase->pDirTransforms)[pArgs[0]].translationX;
                }
            }
        }
        break;
    case 47:
        BattleSetPendingCommand(pSpriteData);
        break;
    case 44:
        BattleSetTaskCallback(pSpriteData, pArgs);
        break;
    case 39:
        BattleScriptOffsetFrameCluts(pSpriteData, pArgs);
        break;
    case 38:
        BattleScriptPlaySpriteAnimation(pSpriteData, pArgs);
        break;
    case 40:
        pSpriteData->step.x = ((s8)pArgs[0] * 0x10 * pSpriteData->field_0x82) / 0x1000 << 8;
        break;
    case 41:
        pSpriteData->step.x += ((s8)pArgs[0] * 0x10 * pSpriteData->field_0x82) / 0x1000 << 8;
        break;
    case 33:
        pSpriteData->step.z = ((s8)pArgs[0] * 0x10 * pSpriteData->field_0x82) / 0x1000 << 12;
        break;
    case 34:
        pSpriteData->step.z += ((s8)pArgs[0] * 0x10 * pSpriteData->field_0x82) / 0x1000 << 8;
        break;
    case 42:
        pSpriteData->step.y = ((s8)pArgs[0] * 0x10 * pSpriteData->field_0x82) / 0x1000 << 12;
        break;
    case 43:
        pSpriteData->step.y += ((s8)pArgs[0] * 0x10 * pSpriteData->field_0x82) / 0x1000 << 8;
        break;
    case 31:
        BATTLE_SCRIPT_FLAGS(pSpriteData) &= ~0x4000000;
        BattleActorUpdateGroundHeightAndTriangle(pSpriteData);
        break;
    case 63:
        BATTLE_SCRIPT_FLAGS(pSpriteData) |= 0x4000000;
        break;
    case 59:
        BATTLE_SCRIPT_FLAGS(pSpriteData) |= 0x20000000;
        break;
    case 60:
        BATTLE_SCRIPT_FLAGS(pSpriteData) &= ~0x20000000;
        break;
    case 53:
        BATTLE_SCRIPT_FLAGS(pSpriteData) |= 0x8000000;
        break;
    case 54:
        BATTLE_SCRIPT_FLAGS(pSpriteData) &= ~0x8000000;
        break;
    case 35:
        BattleActorFaceTarget(pSpriteData, pArgs);
        break;
    case 64:
        func_800B65B0(pSpriteData, pArgs);
        break;
    case 28:
        BattleActorSetDirectionTowardPosition(pSpriteData, pArgs);
        break;
    case 27:
        BattleActorCopyVectorFromSource(pSpriteData, pArgs);
        break;
    case 26:
        WorkListsRemoveEntriesForData(pSpriteData->field_0x6C);
        break;
    case 45:
        BattleScriptLoadFixed3(pSpriteData, pArgs);
        break;
    case 46:
        BattleReadScriptHalfwords3(&pSpriteData->field_0xA0, pArgs);
        break;
    case 91:
        BattleReadScriptHalfwords3((s16*)((u8*)pSpriteData->pBase + 0x44), pArgs);
        break;
    case 92:
        BattleReadScriptHalfwords3((s16*)((u8*)pSpriteData->pBase + 0x4C), pArgs);
        break;
    case 93:
        BattleScriptAddHalfwords3((s16*)((u8*)pSpriteData->pBase + 0x44), pArgs);
        break;
    case 94:
        BattleScriptAddHalfwords3((s16*)((u8*)pSpriteData->pBase + 0x4C), pArgs);
        break;
    case 29:
        BattleScriptCall8001FC4C4WithActorState(pSpriteData, pArgs);
        break;
    case 25:
        BattleScriptCall8001FC53CWithScaledBytes(pSpriteData, pArgs);
        break;
    case 24:
        BattleMoveScreenImageToVram(pSpriteData, pArgs);
        break;
    case 23:
        scale = (s8)pArgs[0] * 4;
        pSpriteData->step.x = pSpriteData->step.x * scale / 0x100;
        pSpriteData->step.y = pSpriteData->step.y * scale / 0x100;
        pSpriteData->step.z = pSpriteData->step.z * scale / 0x100;
        break;
    case 22:
        BATTLE_SCRIPT_FLAGS(pSpriteData) |= 0x2000000;
        break;
    case 21:
        BATTLE_SCRIPT_FLAGS(pSpriteData) |= 0x1000000;
        break;
    case 17:
        D_800C3664 = 1;
        break;
    case 18:
        g_BattleSlotDrawSuppressed = 1;
        D_800C3CAC = g_BattleStateWork[0xB88];
        D_800C3CB0[0] = g_BattleStateWork[0xB89];
        D_800C3CB0[1] = g_BattleStateWork[0xB8A];
        D_800C3CB0[2] = g_BattleStateWork[0xB8B];
        g_BattleStateWork[0x4BF8] = 1;
        g_BattleStateWork[0xB88] = 1;
        g_BattleStateWork[0xB89] = 0;
        g_BattleStateWork[0x4BF9] = 0;
        g_BattleStateWork[0xB8A] = 0;
        g_BattleStateWork[0x4BFA] = 0;
        g_BattleStateWork[0xB8B] = 0;
        g_BattleStateWork[0x4BFB] = 0;
        break;
    case 19:
        D_800C3664 = 0;
        break;
    case 20:
        g_BattleSlotDrawSuppressed = 0;
        g_BattleStateWork[0x4BF8] = D_800C3CAC;
        g_BattleStateWork[0xB88] = D_800C3CAC;
        g_BattleStateWork[0xB89] = D_800C3CB0[0];
        g_BattleStateWork[0x4BF9] = D_800C3CB0[0];
        g_BattleStateWork[0xB8A] = D_800C3CB0[1];
        g_BattleStateWork[0x4BFA] = D_800C3CB0[1];
        g_BattleStateWork[0xB8B] = D_800C3CB0[2];
        g_BattleStateWork[0x4BFB] = D_800C3CB0[2];
        break;
    case 16:
        pSpriteData->position.x = g_BattleSlotVectorsB.vx << 16;
        pSpriteData->position.y = g_BattleSlotVectorsB.vy << 16;
        pSpriteData->position.z = g_BattleSlotVectorsB.vz << 16;
        break;
    case 83:
        pSpriteData->flags_0xAC.flags &= ~0x8;
        pSpriteData->flags_0xAC.flags &= ~0x4;
        BATTLE_SCRIPT_FLAGS(pSpriteData) &= ~0x8;
        BATTLE_SCRIPT_FLAGS(pSpriteData) &= ~0x10;
        break;
    case 15:
        pSpriteData->flags_0xAC.flags &= ~0x8;
        pSpriteData->flags_0xAC.flags &= ~0x4;
        BATTLE_SCRIPT_FLAGS(pSpriteData) &= ~0x8;
        BATTLE_SCRIPT_FLAGS(pSpriteData) &= ~0x10;
        SpriteSetDirection(pSpriteData, 0);
        break;
    case 14:
        SpriteSetDirection(pSpriteData, (s8)pArgs[0] << 4);
        break;
    case 13:
        BattleActorRotateVectorZ(pSpriteData, pArgs);
        break;
    case 10:
        BattleSetRotationZFromVelocity(pSpriteData, pArgs);
        break;
    case 11:
        BattleSetRotationXFromVelocity(pSpriteData, pArgs);
        break;
    case 73:
        pBase = pSpriteData->pBase;
        if (pBase != NULL) {
            pBase->rotation.x = pBase->rotation.x + (s8)pArgs[0];
            BATTLE_SCRIPT_FLAGS(pSpriteData) |= 0x10000000;
        }
        break;
    case 74:
        pBase = pSpriteData->pBase;
        if (pBase != NULL) {
            pBase->rotation.y = pBase->rotation.y + (s8)pArgs[0];
            BATTLE_SCRIPT_FLAGS(pSpriteData) |= 0x10000000;
        }
        break;
    case 75:
        pBase = pSpriteData->pBase;
        if (pBase != NULL) {
            pBase->rotation.z = pBase->rotation.z + (s8)pArgs[0];
            BATTLE_SCRIPT_FLAGS(pSpriteData) |= 0x10000000;
        }
        break;
    case 12:
        BattleSetRotationFromVelocity(pSpriteData, pArgs);
        break;
    case 9:
        pSpriteData->direction += 0x800;
        pSpriteData->step.x = -pSpriteData->step.x;
        pSpriteData->step.y = -pSpriteData->step.y;
        pSpriteData->step.z = -pSpriteData->step.z;
        break;
    case 1:
        SpriteEnsureDirTransforms(pSpriteData);
        BattleCreateDirectionTransformTask(pSpriteData, AnimScriptResolveVariablePointer(pSpriteData, pArgs));
        break;
    case 36:
        func_801FC7B0(pSpriteData, pArgs);
        break;
    case 37:
        func_801FC6FC(pSpriteData, pArgs);
        break;
    case 2:
        pWork = TimerWorkListFindEntryForDataAndCallback(pSpriteData->field_0x6C, BattleUpdateDirectionTransformTask);
        if (pWork != NULL) {
            pWork->onFreeCallback(pWork);
        }
        break;
    case 3:
        BattleCreateSpriteFollowTask(pSpriteData, pArgs);
        break;
    case 4:
        while ((pWork = TimerWorkListFindEntryByCallback(BattleUpdateSpriteFollowTask)) != NULL) {
            pWork->onFreeCallback(pWork);
        }
        break;
    case 30:
        BattleSetLinkedSpriteDrawWorkCallback(pSpriteData, pArgs);
        break;
    case 32:
        BattleStartSpriteMotionTask(pSpriteData);
        break;
    case 5:
        BattleSetSpriteDrawWorkCallback(pSpriteData, pArgs);
        break;
    case 6:
        BattleBeginAnimTask(1 << BATTLE_SCRIPT_SLOT_INDEX(D_800C3E1C));
        break;
    case 85:
        BattleBeginAnimTask((1 << BATTLE_SCRIPT_SLOT_INDEX(D_800C3E1C)) | g_BattleActorSlotMask);
        break;
    case 7:
        pAnimSprite = pSpriteData->pTargetEntitySprite;
        BattleBeginAnimTask(1 << BATTLE_SCRIPT_SLOT_INDEX(pAnimSprite));
        break;
    case 101:
        BattleBeginAnimTask(g_BattleActorSlotMask);
        break;
    case 65:
        BattleBeginAnimTask((1 << BATTLE_SCRIPT_SLOT_INDEX((SpriteData*)pSpriteData->pTargetEntitySprite)) |
                            (1 << BATTLE_SCRIPT_SLOT_INDEX(D_800C3E1C)));
        break;
    case 8:
        pSpriteData->field_0x40 |= 0x80000;
        BattleClearRecordTable8(pSpriteData, pArgs);
        break;
    case 57:
        break;
    }
}*/



// Uploads the current frame's texture and CLUT into the sprite's own slots: the
// 8x8 tile at the frame descriptor's texture-page rectangle goes to (0x3F0,0x1F0)
// and its CLUT to (0x3F0,0x1EE).
void BattleScriptUploadSpriteFrame(SpriteData* pSpriteData) {
    RECT rect;
    SpriteFrameData* pFrame;
    s16 texPage;
    s16 clut;

    pFrame = pSpriteData->pBase->pCurRenderFramesData;
    texPage = pFrame->texPage;
    rect.x = (pFrame->texX >> 2) + ((texPage & 0xF) << 6);
    rect.y = pFrame->texY + ((texPage << 4) & 0x100);
    rect.w = 8;
    rect.h = 8;
    MoveImage(&rect, 0x3F0, 0x1F0);
    clut = pFrame->clutID;
    rect.w = 0x10;
    rect.h = 1;
    rect.x = clut & 0x3F;
    rect.y = (clut >> 6) & 0x1FF;
    MoveImage(&rect, 0x3F0, 0x1EE);
}

void BattleSetActorDirectionMatrix(SpriteData* pSpriteData, s32 index, MATRIX* pMatrix) {
    BattleDirectionTransform* pTransform;
    SVECTOR rot;
    s32 x;
    s32 y;
    s16 scale;

    if (pSpriteData->pBase == NULL) {
        return;
    }
    pTransform = (BattleDirectionTransform*)(index * sizeof(BattleDirectionTransform) +
                                             (mem_addr)pSpriteData->pBase->pDirTransforms);
    y = pTransform->translationY;
    x = pTransform->translationX;
    if (((u32)pSpriteData->flags_0xAC.flags >> 2) & 1) {
        x = -x;
    }
    scale = pSpriteData->scale;
    y = (y * scale) / 0x1000;
    x = (x * scale) / 0x1000;
    rot.vx = pTransform->rotationX;
    rot.vy = ((BattleDirectionTransform*)pSpriteData->pBase->pDirTransforms)[index].rotationY;
    rot.vz = ((BattleDirectionTransform*)pSpriteData->pBase->pDirTransforms)[index].rotationZ;
    RotMatrix(&rot, pMatrix);
    pMatrix->t[0] = pSpriteData->pBase->transformMatrix.t[0] + x;
    pMatrix->t[1] = pSpriteData->pBase->transformMatrix.t[1] + y;
    pMatrix->t[2] = pSpriteData->pBase->transformMatrix.t[2];
    SetMulMatrix(pMatrix, &pSpriteData->pBase->transformMatrix);
}
// PARKED 94.64% at retail's exact 220 bytes / 55 insns (XenoBattleTailCdk0). The only
// residual is the two induction pointers' REGISTER HAND-OUT: retail is 'move t0,a1'
// (x base = a copy of pOut) in the FIRST guard's delay slot plus 'addiu a1,a1,4' in
// the preheader (y/z base = pOut + 4), i.e. the +4 lands on the parameter's register.
// Every spelling gets the MIRROR (x base coalesced onto $a1, y/z base
// 'addiu t0,a1,4'), which cc1 prefers because the bare copy is one instruction
// cheaper. levers.md [e-a-derived-base-beats-a-copy-of-the-parameter] lists the
// seven dead spellings; re-measured this round on the honest body:
//  - pX = (s16*)pOut; pYz = (s16*)pOut + 2;      (this body) 94.64, 55 insns / 220 B
//  - both pointers assigned BEFORE the three guards:      96.45% but 54 insns / 216 B
//    (the addiu folds into the guard's delay slot and $a1 is never copied, so it
//    cannot link at retail's size) - matches levers.md's note.
//  An older park body ('pYz = (s16*)pOut; pX = pYz - 2;') read 95.27 but is WRONG
//  CODE - it walks x at pOut-4+8i and y at -2 (retail writes x at pOut+8i, y +2,
//  z +4), i.e. it stores before the array, so its 95.27 is not a better answer.
//  Below 99%, so no permuter run.
/*
void func_800B50D4(SpriteData* pSpriteData, SVECTOR* pOut) {
    s16* pX;
    s16* pYz;
    s32 i;
    s32 x;
    s32 y;
    s16 scale;
    BattleDirectionTransform* pTransform;

    if ((pSpriteData->field_0x3C_0 & 3) != 1) {
        return;
    }
    if (pSpriteData->pBase == NULL) {
        return;
    }
    if (pSpriteData->pBase->pDirTransforms == NULL) {
        return;
    }
    pX = (s16*)pOut;
    pYz = (s16*)pOut + 2;
    for (i = 0; i != 5;) {
        pTransform = (BattleDirectionTransform*)(D_800C356C[i] * sizeof(BattleDirectionTransform) + (mem_addr)pSpriteData->pBase->pDirTransforms);
        x = pTransform->translationX;
        y = pTransform->translationY;
        if (((u32)pSpriteData->flags_0xAC.flags >> 2) & 1) {
            x = -x;
        }
        scale = pSpriteData->scale;
        x = (x * scale) / 0x2000;
        y = (y * scale) / 0x2000;
        i++;
        pX[0] = x;
        pYz[-1] = y;
        pYz[0] = 0;
        pYz += 4;
        pX += 4;
    }
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B50D4);




// The far screen edge of the last trail quad, which is the near edge of the next one:
// two screen pairs, one per corner of that edge.
extern u16 g_BattleTrailEdgeX0;
extern u16 g_BattleTrailEdgeY0;
extern u16 g_BattleTrailEdgeX1;
extern u16 g_BattleTrailEdgeY1;

// The trail's segment counter: the depth stagger and the texture column the quad
// samples are both taken from it, the column from its low byte.
typedef union {
    /* 0x0 */ s32 count;
    /* 0x0 */ u8 bytes[4];
} BattleTrailSegmentCount;

extern BattleTrailSegmentCount D_800D2FCC;

// The trail's per-segment renderer: one LINE_F2 for the segment and one textured POLY_FT4
// for the ribbon quad, whose far corners it leaves in g_BattleTrailEdge* for the next call.
// Three spellings below are load-bearing, not style - each one is the only shape whose
// register allocation matches (the unit is -G0 cdk with instruction scheduling on):
// - `count` is read into a local before the clut store, which is what puts the count in v1
//   and the clut value in the bnez delay slot;
// - `clut` and the two edges are named locals stored after the rgb group, not inline calls;
// - xEdge and yEdge are computed in two statements (sin/cos, then the 8192 scale).
void func_800B51B0(VECTOR* pFrom, VECTOR* pTo) {
    LINE_F2* pLine;
    POLY_FT4* pQuad;
    SVECTOR from;
    SVECTOR to;
    long scratch[2]; // RotTransPers3's third screen pair, its depth and its flags
    s32 abr;
    s32 depth;
    s32 angle;
    s32 dy;
    s32 dx;
    s32 yEdge;
    s32 xEdge;
    s32 radius;
    u16 clut;
    s32 count;
    u8* pLineBuf;
    u8* pQuadBuf;

    if (pFrom->vx == pTo->vx && pFrom->vy == pTo->vy && pFrom->vz == pTo->vz) {
        return;
    }
    from.vx = (u16)pFrom->vx;
    from.vy = (u16)pFrom->vy;
    from.vz = (u16)pFrom->vz;
    to.vx = (u16)pTo->vx;
    to.vy = (u16)pTo->vy;
    to.vz = (u16)pTo->vz;
    pLineBuf = g_GfxCurWorkBuffer;
    if (pLineBuf + 0x10 >= g_GfxCurWorkBufferEnd) {
        return;
    }
    pLine = (LINE_F2*)pLineBuf;
    g_GfxCurWorkBuffer = pLineBuf + 0x10;
    SetLineF2(pLine);
    abr = D_800C3D4C[0];
    if (abr != 0) {
        abr--;
        SetSemiTrans(pLine, 1);
    } else {
        SetSemiTrans(pLine, 0);
    }
    pLine->r0 = 0xF0;
    pLine->g0 = 0xF0;
    pLine->b0 = 0xF0;
    if (D_800D2FD8[D_800D2FCC.count / 8] != 0) {
        depth = 2;
    } else {
        depth = -2;
    }
    depth += D_800D3334[0];
    RotTransPers3(&from, &to, &from, (long*)&pLine->x0, (long*)&pLine->x1, scratch, scratch,
                  scratch + 1);
    if ((u32)(depth - 1) >= 0xFFF) {
        return;
    }
    radius = D_800C3E9C[0];
    dx = pLine->x1 - pLine->x0;
    dy = pLine->y1 - pLine->y0;
    angle = ratan2(dy, dx) + 0x400;
    xEdge = rsin(angle);
    xEdge = (xEdge * radius) / 0x2000;
    yEdge = rcos(angle);
    yEdge = (yEdge * radius) / 0x2000;
    pQuadBuf = g_GfxCurWorkBuffer;
    if (pQuadBuf + 0x28 >= g_GfxCurWorkBufferEnd) {
        return;
    }
    pQuad = (POLY_FT4*)pQuadBuf;
    g_GfxCurWorkBuffer = pQuadBuf + 0x28;
    SetPolyFT4(pQuad);
    SetShadeTex(pQuad, 1);
    pQuad->u0 = (D_800D2FCC.bytes[0] & 7) - 0x40;
    pQuad->v0 = 0xF0;
    pQuad->u1 = (D_800D2FCC.bytes[0] & 7) - 0x3F;
    pQuad->v1 = 0xF0;
    pQuad->u2 = (D_800D2FCC.bytes[0] & 7) - 0x40;
    pQuad->v2 = 0xF7;
    pQuad->u3 = (D_800D2FCC.bytes[0] & 7) - 0x3F;
    pQuad->v3 = 0xF7;
    pQuad->tpage = GetTPage(0, abr, 0x3F0, 0x1F0);
    clut = GetClut(0x3F0, 0x1EE);
    pQuad->r0 = 0xF0;
    pQuad->g0 = 0xF0;
    pQuad->b0 = 0xF0;
    count = D_800D2FCC.count;
    pQuad->clut = clut;
    if (count == 0) {
        pQuad->x0 = (u16)pLine->x0 - xEdge;
        pQuad->y0 = (u16)pLine->y0 - yEdge;
        pQuad->x1 = (u16)pLine->x0 + xEdge;
        pQuad->y1 = (u16)pLine->y0 + yEdge;
    } else {
        pQuad->x0 = g_BattleTrailEdgeX0;
        pQuad->y0 = g_BattleTrailEdgeY0;
        pQuad->x1 = g_BattleTrailEdgeX1;
        pQuad->y1 = g_BattleTrailEdgeY1;
    }
    pQuad->x2 = (u16)pLine->x1 - xEdge;
    g_BattleTrailEdgeX0 = pQuad->x2;
    pQuad->y2 = (u16)pLine->y1 - yEdge;
    g_BattleTrailEdgeY0 = pQuad->y2;
    pQuad->x3 = (u16)pLine->x1 + xEdge;
    g_BattleTrailEdgeX1 = pQuad->x3;
    pQuad->y3 = (u16)pLine->y1 + yEdge;
    g_BattleTrailEdgeY1 = pQuad->y3;
    AddPrim(&g_GfxCurOT[depth], pQuad);
    D_800D2FCC.count++;
}



// The trail renderer's per-frame tick: it publishes the direction-transform task's state
// through the four globals the trail quads read back, re-fills the cached corners when the
// sprite's animation frame changed, and lerps the last four corners halfway to the target.
// The head reads the two sprite halfwords BEFORE the first store on purpose - cdk's
// scheduler only keeps them above it when the source reads them there.
void BattleUpdateDirectionTransformTask(WorkListEntry* pWork) {
    BattleDirectionTask* pTask;
    SpriteData* pSpriteData;
    u16 radius;
    u16 field2e;
    u32 frame;
    u32 curFrame;
    s32 mode;
    s32 i;

    pTask = pWork->pTask;
    pSpriteData = pTask->pSpriteData;
    D_800D2FD8 = pTask->pArgs;
    radius = pTask->radius;
    // Both halfwords are read unsigned: the struct declares them s16, retail uses `lhu`.
    field2e = *(u16*)&pSpriteData->field_0x2e;
    frame = *(u16*)&pSpriteData->curSpriteFrame;
    D_800C3E9C[0] = radius;
    D_800C3D4C[0] = pTask->semiTrans;
    curFrame = pTask->curSpriteFrame;
    D_800D3334[0] = field2e;
    if (frame != curFrame) {
        pTask->curSpriteFrame = frame;
        func_800B50D4(pSpriteData, pTask->targetPos);
    }
    for (i = 1; i != BATTLE_TRAIL_SEGMENTS; i++) {
        pTask->curPos[i].vx = pTask->curPos[i].vx + (pTask->targetPos[i].vx - pTask->curPos[i].vx) / 2;
        pTask->curPos[i].vy = pTask->curPos[i].vy + (pTask->targetPos[i].vy - pTask->curPos[i].vy) / 2;
        pTask->curPos[i].vz = pTask->curPos[i].vz + (pTask->targetPos[i].vz - pTask->curPos[i].vz) / 2;
    }
    pTask->curPos[0].vx = pTask->targetPos[0].vx;
    pTask->curPos[0].vy = pTask->targetPos[0].vy;
    pTask->curPos[0].vz = pTask->targetPos[0].vz;
    if (pTask->animId == (s8)pTask->pSpriteData->flags_0xAC.bytes[3]) {
        mode = ((u32)pTask->pSpriteData->flags_0xA8 >> 28) & 3;
        if (mode != 0) {
            if (mode != 1) {
                return;
            }
        }
    }
    pTask->timerTask.onFreeCallback(pTask);
}


void BattleProcessDirectionTransformTask(WorkListEntry* pWork) {
    BattleDirectionTask* pTask;

    pTask = pWork->pTask;
    func_8001E148(pTask->pSpriteData);
    BattleEmitTrailSegments(BATTLE_TRAIL_SEGMENTS, pTask->curPos, func_800B51B0);
}

void BattleCreateDirectionTransformTask(SpriteData* pSpriteData, u8* pArgs) {
    BattleDirectionTask* pTask;

    pTask = WorkListsAddTasks(sizeof(BattleDirectionTask), pSpriteData->field_0x6C, BattleUpdateDirectionTransformTask,
                              BattleProcessDirectionTransformTask, NULL);
    pTask->pSpriteData = pSpriteData;
    // Read as unsigned: curSpriteFrame is s16 in the struct but -1 is a valid frame
    pTask->curSpriteFrame = *(u16*)&pSpriteData->curSpriteFrame;
    pTask->animId = *(s8*)((u8*)pSpriteData + 0xAF);
    pTask->pArgs = pArgs;
    pTask->semiTrans = *(u8*)((u8*)pSpriteData + 0x3C) >> 5;
    if (pArgs[0] == 0) {
        pTask->radius = 4;
    } else {
        pTask->radius = pArgs[0];
    }
    func_800B50D4(pSpriteData, pTask->curPos);
    func_800B50D4(pSpriteData, pTask->targetPos);
}
long BattleSpriteDistanceToTarget(BattleSpriteState* pSprite) {
    VECTOR v;
    VECTOR out;

    v.vx = pSprite->targetPosition.x - pSprite->position.xWhole;
    v.vy = pSprite->targetPosition.y - pSprite->position.yWhole;
    v.vz = pSprite->targetPosition.z - pSprite->position.zWhole;
    Square0(&v, &out);
    return SquareRoot0(out.vx + out.vz + out.vy);
}
