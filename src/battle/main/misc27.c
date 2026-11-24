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
extern void func_800B51B0();
extern void func_800B50D4();
extern void func_800B5588();
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
    ((((u32)(pSpriteData)->flags_0xAC.flags & 3) << 2) | ((u32)(pSpriteData)->flags_0xA8 >> 30))

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
extern s16 g_BattleActorSlotMask;
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
extern void BattleBeginAnimTask(u16 slotMask);
extern void BattleClearRecordTable8(SpriteData* pSpriteData, u8* pArgs);

// The battle animation script interpreter: `mode` selects the opcode, which mostly
// moves one value in or out of the sprite and its anim script state. 0 is not an
// opcode; 1..0x6B are.
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B3F04);
// 89.96% (4020 bytes / 1005 insns against retail's 4056 / 1014). This is the whole
// 107-case battle animation-script interpreter transcribed in address order from the
// disassembly; every arm's structure, call and constant was read off the .s, so what is
// left is 9 instructions and the frame (retail 0x68, this body 0x40 - retail keeps the
// two Square0 vectors at 0x38..0x50 live across the whole function, this body scopes
// them into the arm) plus the arm-level residuals below. Nothing has been tried against
// them yet.
// Known open residuals, each measured once on this body:
//  - case 103/80's `pSpriteBytecode - 2` tail and case 52's double store to 0x1C are the
//    arms that own the missing instructions.
//  - case 97 reads the target's slot index and its position.xWhole/zWhole, and the
//    comparisons against the slot's 0xE halfword are the `bnez`+`j`/`negv` shape.
//  - case 9's `direction += 0x800` and cases 73/74/75's rotation adds read lh where
//    retail has lhu, i.e. the original viewed those halfwords as unsigned.
//  - case 15/83's four masked ANDs (two constants, -9/-5/-0x11) are spelled as four
//    statements; retail keeps one register per word.
/*
void func_800B3F04(SpriteData* pSpriteData, s32 mode, u8* pArgs) {
    BattleScriptActorSlot* pSlot;
    SpriteData* pAnimSprite;
    SpriteDataI1* pBase;
    WorkListEntry* pWork;
    SpriteDirectionTransform* pTransform;
    BattleSoundBank* pBank;
    s32 vecA[3];
    s32 vecB[3];
    s32 scale;
    s32 value;
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
        pSpriteData->field_0x40 |= 0x200;
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
        pSpriteData->step.y = (pSpriteData->field_0x84 - 1) << 16;
        value = (s8)pArgs[0] * pSpriteData->scale;
        pSlot = (BattleScriptActorSlot*)(g_BattleStateWork +
                                         BATTLE_SCRIPT_SLOT_INDEX(pAnimSprite) * sizeof(BattleScriptActorSlot));
        value >>= 12;
        if (((BattleSpriteState*)pAnimSprite)->position.xWhole == pSlot->x) {
            if (!(((u32)pSpriteData->flags_0xAC.flags >> 2) & 1)) {
                value = -value;
            }
        } else if (((BattleSpriteState*)pAnimSprite)->position.xWhole < pSlot->x) {
            value = -value;
        }
        pSpriteData->field_0xA0 = ((BattleSpriteState*)pAnimSprite)->position.xWhole + value;
        pSpriteData->field_0xA2 = 0;
        pSpriteData->field_0xA4 = ((BattleSpriteState*)pAnimSprite)->position.zWhole;
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
                SoundPlaySoundEffectOnFreePair((g_BattleSoundBankMain->bankId << 16) | (value + 0x52));
            }
        }
        break;
    case 84:
        func_801FC898();
        break;
    case 79:
        pBank = (BattleSoundBank*)pSpriteData->field_0x50;
        if (pBank != NULL) {
            SoundPlaySoundEffectOnLastPair((pBank->bankId << 16) | pArgs[0]);
        }
        break;
    case 82:
        pBank = (BattleSoundBank*)pSpriteData->field_0x50;
        if (pBank != NULL) {
            SoundPlaySoundEffectOnPair((pBank->bankId << 16) | pArgs[0], pArgs[1]);
        }
        break;
    case 77:
        pBank = (BattleSoundBank*)pSpriteData->field_0x50;
        if (pBank != NULL) {
            SoundStopElementsById((pBank->bankId << 16) | pArgs[0]);
        }
        break;
    case 78:
        if (g_BattleSoundBankMain != NULL) {
            SoundStopElementsById((g_BattleSoundBankMain->bankId << 16) | pArgs[0]);
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
        vecA[0] = g_BattleAnimFrame.x - g_BattleSlotVectors.vx;
        vecA[1] = g_BattleAnimFrame.y - g_BattleSlotVectors.vy;
        vecA[2] = g_BattleAnimFrame.z - g_BattleSlotVectors.vz;
        Square0((VECTOR*)vecA, (VECTOR*)vecA);
        vecB[0] = g_BattleAnimFrame.xB - g_BattleSlotVectorsB.vx;
        vecB[1] = g_BattleAnimFrame.yB - g_BattleSlotVectorsB.vy;
        vecB[2] = g_BattleAnimFrame.zB - g_BattleSlotVectorsB.vz;
        Square0((VECTOR*)vecB, (VECTOR*)vecB);
        if (SquareRoot0(vecA[0] + vecA[1] + vecA[2]) < 4) {
            if (SquareRoot0(vecB[0] + vecB[1] + vecB[2]) < 4) {
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
        value = (s8)pArgs[0] * 2 * pSpriteData->field_0x82;
        value /= 0x1000;
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
                    pTransform = &pBase->pDirTransforms->dirTransforms[pArgs[0]];
                    pBase->offsetY = pTransform->translationY;
                    pBase->offsetX = pTransform->translationX;
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
        g_BattleStateWork[0xB88] = 1;
        g_BattleStateWork[0x4BF8] = 1;
        g_BattleStateWork[0xB89] = 0;
        g_BattleStateWork[0xB8A] = 0;
        g_BattleStateWork[0xB8B] = 0;
        g_BattleStateWork[0x4BF9] = 0;
        g_BattleStateWork[0x4BFA] = 0;
        g_BattleStateWork[0x4BFB] = 0;
        break;
    case 19:
        D_800C3664 = 0;
        break;
    case 20:
        g_BattleSlotDrawSuppressed = 0;
        g_BattleStateWork[0xB88] = D_800C3CAC;
        g_BattleStateWork[0x4BF8] = D_800C3CAC;
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
        pWork = TimerWorkListFindEntryForDataAndCallback(pSpriteData->field_0x6C, func_800B5588);
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
        BattleBeginAnimTask(1 << BATTLE_SCRIPT_SLOT_INDEX((SpriteData*)pSpriteData->pTargetEntitySprite));
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
}
*/
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
// 95.27% at retail's exact 220 bytes / 55 insns: everything matches except the two
// induction pointers' REGISTER HAND-OUT. Retail is `move t0,a1` (x base = copy of
// pOut) in the first guard's delay slot and `addiu a1,a1,4` in the preheader (y/z
// base = pOut + 4), i.e. the +4 lands on the parameter's register; every spelling
// here gets the mirror (x base = $a1 coalesced, y/z base = `addiu t0,a1,4`), which
// cc1 prefers because it costs one instruction less. Measured, all 220 bytes:
//  - pYz = (s16*)pOut; pX = pYz - 2;            (this body) 95.27 - addiu on the copy
//  - pX = (s16*)pOut; pYz = (s16*)pOut + 2;                 94.64
//  - pYz = (s16*)pOut + 2; pX = (s16*)pOut;                 94.64
//  - pYz = (s16*)pOut + 2; pX = pYz - 2;                    94.64
//  - SVECTOR* pDst = pOut; s16* pYz = (s16*)pOut + 2;       94.64
//  - pYz assigned BEFORE the guards, pX after them           96.45 but 216 bytes
//    (54 insns - the add is folded into the guard's delay slot and a1 is never
//    copied, so it cannot link)
//  - pOut[i].vx / .vy / .vz struct form                     84.84 / 216 bytes (one
//    base with offsets -8/+2/+4, and the i++ sinks below the stores)
// A likely reading of retail is two pointer locals 4 bytes apart (`move t0,a1` then
// the add into the dying $a1), which combine_regs merges for us. Below 99%, so no
// permuter run is allowed on it.
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
    pYz = (s16*)pOut;
    pX = pYz - 2;
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
extern u16 D_800C3CA4[2];
extern u16 D_800C3CA8[2];

// The trail's segment counter: the depth stagger and the texture column the quad
// samples are both taken from it, the column from its low byte.
typedef union {
    /* 0x0 */ s32 count;
    /* 0x0 */ u8 bytes[4];
} BattleTrailSegmentCount;

extern BattleTrailSegmentCount D_800D2FCC;

INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B51B0);
// 84.19% (992 bytes / 248 insns against retail's 984 / 246). Structure, every call, every
// constant and the whole first half are instruction-identical; what is left is three
// things, all measured:
//  - REGISTER HAND-OUT: retail is yEdge=$s2, xEdge=$s3, the 0xF0 constant=$s4, abr=$s5;
//    this body gets yEdge=$s2, the constant=$s3, abr=$s4, xEdge=$s5, i.e. abr outranks
//    xEdge where retail ranks xEdge first (the `v0,v0,s3`/`v0,v0,s2` addu sites and the
//    `move a1,s5` GetTPage argument are the visible half of it). Tried: 0xF0 as a `color`
//    local (identical), and `abr` off D_800C3D4C spelled as a countdown local (this body).
//  - TWO EXTRA INSTRUCTIONS. Retail reaches the four edge halfwords with four independent
//    `lui %hi(D_800C3CA4 + 0x2)`-style address loads; written as `extern u16
//    D_800C3CA4[2]` gcc's function-cse forms the base once and adds `addiu v0,v0,0` for
//    the +2 sibling (branch-B loads) - equal count there - and one more per pair in the
//    tail, which is the 2 instructions. Spelled with four __asm__-aliased scalars
//    (`D_800C3CA6[] __asm__("D_800C3CA4 + 0x2")`) the tail's relocations all come out
//    right (4 lui + 4 sh, retail's shape) but the body then measures 249 insns / 78.85%.
//  - SCHEDULING: retail's ratan2 argument loads are [lh 0xC][lh 0xE][lh 0xA][lh 0x8] with
//    dy's `subu` before the jal and dx's in the delay slot; this body swaps the x1/y1
//    loads and puts dx's subu first. -fschedule-insns is on in this preset, so statement
//    order does not reach it.
// Found on the way: the guard's buffer pointer and the quad's must be two separate locals
// (`pLineBuf`/`pQuadBuf`) - one shared `pBuf` makes cc1 hoist the g_GfxCurWorkBuffer load
// above the six SVECTOR copies, which clobbers $a0 and costs a `move a2,a0` plus a
// different address register. Also `depth += D_800D3334[0]` must come BEFORE the
// RotTransPers3 call (retail has the lh before the jal and the addu in its delay slot).
/*
// Draws one segment of the sprite's direction trail: the two positions the effect
// callback hands over are projected as a line, and that line's screen endpoints are
// widened into a textured quad by the trail radius and the line's screen angle.
void func_800B51B0(VECTOR* pFrom, VECTOR* pTo) {
    LINE_F2* pLine;
    POLY_FT4* pQuad;
    SVECTOR from;
    SVECTOR to;
    long scratch[2]; // RotTransPers3's third screen pair, its depth and its flags
    s32 abr;
    s32 depth;
    s32 angle;
    s32 xEdge;
    s32 yEdge;
    s32 radius;
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
    angle = ratan2(pLine->y1 - pLine->y0, pLine->x1 - pLine->x0) + 0x400;
    xEdge = (rsin(angle) * radius) / 0x2000;
    yEdge = (rcos(angle) * radius) / 0x2000;
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
    pQuad->clut = GetClut(0x3F0, 0x1EE);
    pQuad->r0 = 0xF0;
    pQuad->g0 = 0xF0;
    pQuad->b0 = 0xF0;
    if (D_800D2FCC.count == 0) {
        pQuad->x0 = (u16)pLine->x0 - xEdge;
        pQuad->y0 = (u16)pLine->y0 - yEdge;
        pQuad->x1 = (u16)pLine->x0 + xEdge;
        pQuad->y1 = (u16)pLine->y0 + yEdge;
    } else {
        pQuad->x0 = D_800C3CA4[0];
        pQuad->y0 = D_800C3CA4[1];
        pQuad->x1 = D_800C3CA8[0];
        pQuad->y1 = D_800C3CA8[1];
    }
    pQuad->x2 = (u16)pLine->x1 - xEdge;
    D_800C3CA4[0] = pQuad->x2;
    pQuad->y2 = (u16)pLine->y1 - yEdge;
    D_800C3CA4[1] = pQuad->y2;
    pQuad->x3 = (u16)pLine->x1 + xEdge;
    D_800C3CA8[0] = pQuad->x3;
    pQuad->y3 = (u16)pLine->y1 + yEdge;
    D_800C3CA8[1] = pQuad->y3;
    AddPrim(&g_GfxCurOT[depth], pQuad);
    D_800D2FCC.count++;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc27", func_800B5588);
// 90.01% (348 bytes): the four global stores and the loop match; retail's position
// reads are unsigned at some sites (lhu) and signed at others (lh), which suggests the
// original curPos/targetPos view is u16 and the delta is read through an s16 view - not
// reproduced here. Two other measured findings:
//  - The four globals must be extern ARRAYS (`D_800C3E9C[0] = ...`): as plain scalars
//    they go through `lui $at` where retail uses `lui $v0,%hi` + the store off $v0
//    (the atfree signature; with the array spelling that part matches exactly).
//  - retail keeps only $s0 (0x18 frame); holding pWork across the tail call costs an
//    $s1, so the onFree callback must be reached through pTask, not pWork.
//  - The tail mode test is UNMERGED in retail (`beqz v1` / `li v0,1` / `bne v1,v0`)
//    where gcc 2.7.2 merges `mode != 0 && mode != 1` into `sltiu v0,v1,2`; that is
//    the 2.6.0/preset branch-shape class.
/*
void func_800B5588(WorkListEntry* pWork) {
    BattleDirectionTask* pTask;
    SpriteData* pSpriteData;
    s32 mode;
    s32 i;

    pTask = pWork->pTask;
    pSpriteData = pTask->pSpriteData;
    D_800D2FD8[0] = (u32)pTask->pArgs;
    D_800C3E9C[0] = pTask->radius;
    D_800C3D4C[0] = pTask->semiTrans;
    D_800D3334[0] = pSpriteData->field_0x2e;
    if (pSpriteData->curSpriteFrame != pTask->curSpriteFrame) {
        pTask->curSpriteFrame = pSpriteData->curSpriteFrame;
        func_800B50D4(pSpriteData, pTask->targetPos);
    }
    for (i = 1; i != 5; i++) {
        pTask->curPos[i].vx = pTask->curPos[i].vx + (pTask->targetPos[i].vx - pTask->curPos[i].vx) / 2;
        pTask->curPos[i].vy = pTask->curPos[i].vy + (pTask->targetPos[i].vy - pTask->curPos[i].vy) / 2;
        pTask->curPos[i].vz = pTask->curPos[i].vz + (pTask->targetPos[i].vz - pTask->curPos[i].vz) / 2;
    }
    pTask->curPos[0].vx = pTask->targetPos[0].vx;
    pTask->curPos[0].vy = pTask->targetPos[0].vy;
    pTask->curPos[0].vz = pTask->targetPos[0].vz;
    if (pTask->animId == (s8)pTask->pSpriteData->flags_0xAC.bytes[3]) {
        mode = ((u32)pTask->pSpriteData->flags_0xA8 >> 28) & 3;
        if (mode != 0 && mode != 1) {
            return;
        }
    }
    pTask->timerTask.onFreeCallback(pTask);
}
*/
void BattleProcessDirectionTransformTask(WorkListEntry* pWork) {
    BattleDirectionTask* pTask;

    pTask = pWork->pTask;
    func_8001E148(pTask->pSpriteData);
    BattleEmitTrailSegments(5, pTask->curPos, func_800B51B0);
}

void BattleCreateDirectionTransformTask(SpriteData* pSpriteData, u8* pArgs) {
    BattleDirectionTask* pTask;

    pTask = WorkListsAddTasks(sizeof(BattleDirectionTask), pSpriteData->field_0x6C, func_800B5588,
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
