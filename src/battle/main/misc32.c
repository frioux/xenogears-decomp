#include "common.h"
#include "psyq/libgte.h"
#include "system/animation.h"

extern void func_8001F6B0(void *pObj);
extern void GfxQueueSpriteTileData(void *pData);
extern void WorkListsRemoveEntriesForData(void *pTask);
extern void TimerWorkListRemoveTask(void *pTask);
// left unprototyped: the ramp passes a third argument the callee (misc31) does not read
extern void BattleSetSpriteColorFromAngle();
extern MATRIX g_BattleViewMatrix;
extern u8 g_BattleStateWork[];

/* Built with XenoBattleTailCdk0 (gcc-2.7.2-cdk, -G0). With -G0 the cdk cc1 splits
   every absolute symbol access itself (`lui $r,%hi(sym)` / `%lo(sym)($r)`, any free
   register), which is what retail's "atfree" stubs contain; at -G8 it leaves the
   bare macro to the assembler and emits `lui $at`. */

// The sprite record a color-ramp task animates. x/y/z are the integer halves of
// SpriteData's 16.16 IVEC3 position: this unit reads only those, with `lh`, so the low
// half of each is an untouched pad here.
typedef struct {
    /* 0x00 */ u8 pad0[0x2];
    /* 0x02 */ s16 x;
    /* 0x04 */ u8 pad4[0x2];
    /* 0x06 */ s16 y;
    /* 0x08 */ u8 pad8[0x2];
    /* 0x0A */ s16 z;
    /* 0x0C */ u8 unkC[0x1C];
    /* 0x28 */ u8 colors[0x3]; // written by BattleSetSpriteColorFromAngle
    /* 0x2B */ u8 prim; // bit 0 cleared on spawn (BattleStartEffectColorRamp), set on free
    /* 0x2C */ u8 unk2C[0xA];
    /* 0x36 */ u16 yOffset; // the sprite's own vertical, taken once when the ramp starts
} BattleEffectSprite; // Size: 0x38

// The color-ramp task's own data block, reached through the task entry's +0x4: the two
// work-list entries WorkListsAddTasks fills in, then the task's own fields.
typedef struct {
    /* 0x00 */ WorkListEntry timerEntry;
    /* 0x1C */ WorkListEntry workEntry;
    /* 0x38 */ BattleEffectSprite *pSprite;
    /* 0x3C */ s32 posX; // the VECTOR BattleDrawEffectColorRamp hands TransMatrix
    /* 0x40 */ s32 posY;
    /* 0x44 */ s32 posZ;
    /* 0x48 */ u8 pad48[0x4]; // the unused 4th word of the VECTOR posX..posZ
    /* 0x4C */ u16 rotX; // the SVECTOR BattleDrawEffectColorRamp hands RotMatrix
    /* 0x4E */ u16 rotY; // advanced 0x10 per step
    /* 0x50 */ u16 rotZ;
    /* 0x52 */ u16 pad52;
    /* 0x54 */ s32 yOffset; // the sprite's 0x36 word, subtracted from posY
    /* 0x58 */ s32 angle; // advanced once per step
    /* 0x5C */ void *pPrimBuffers[2]; // the double-buffered prim block, one per render context
    /* 0x64 */ void *pFrame; // the model frame func_800B1F6C transforms
} BattleEffectColorRampData; // Size: 0x68

typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ BattleEffectColorRampData *pData;
} BattleEffectTask; // Size: 0x8

// The sprite colour pulse task g_BattleSpriteColorTask holds (misc31 owns the type and
// its tick): the work-list entry, the sprite it tints and the battle slot it was
// created for, which g_BattleSpriteColorTaskSlot retires.
typedef struct {
    /* 0x00 */ WorkListEntry entry;
    /* 0x1C */ SpriteData *pSpriteData;
    /* 0x20 */ s32 angle;
    /* 0x24 */ s32 slot;
} BattleSpriteColorTask; // Size: 0x28

extern BattleSpriteColorTask *g_BattleSpriteColorTask;
extern u8 g_BattleSpriteColorTaskSlot;
extern u8 g_FlagTimerWorkListAdds;
extern s32 g_NumFlaggedTimerWorkListEntries;
extern void BattleFreeSpriteColorTask();
extern void BattleSpriteColorTaskTick(BattleSpriteColorTask *pTask);
extern void *TimerWorkListAllocateTask(void *pData, s32 dataSize);

// Pulses the current slot's sprite colour: retires the task still running for another
// slot, then makes one for this slot's sprite out of the two per-slot pointer tables the
// battle state work block ends with (0x8C8C is the sprite misc33 animates, 0x8CB8 the
// record TimerWorkListAllocateTask takes).
void BattleStartSpriteColorPulse(void) {
    BattleSpriteColorTask *pTask;
    SpriteData *pSpriteData;
    u8 *pBase;

    if (g_BattleSpriteColorTask != NULL) {
        if (g_BattleSpriteColorTask->slot == g_BattleSpriteColorTaskSlot) {
            return;
        }
        BattleFreeSpriteColorTask(0);
    }
    pBase = g_BattleStateWork;
    if (*(SpriteData **)(pBase + pBase[0xA72] * 4 + 0x8CB8) == NULL) {
        return;
    }
    pTask = TimerWorkListAllocateTask(*(SpriteData **)(pBase + pBase[0xA72] * 4 + 0x8CB8), 0xC);
    g_BattleSpriteColorTask = pTask;
    TimerWorkListSetTaskCallback(pTask, BattleSpriteColorTaskTick);
    WorkListTaskSetOnFreeCallback(&pTask->entry, BattleFreeSpriteColorTask);
    if (g_FlagTimerWorkListAdds != 0) {
        g_NumFlaggedTimerWorkListEntries--;
    }
    pTask->entry.unk14_3 = 0; // clears the flagged bit the work list just set
    pSpriteData = *(SpriteData **)(pBase + pBase[0xA72] * 4 + 0x8C8C);
    pTask->pSpriteData = pSpriteData;
    pTask->slot = pBase[0xA72];
    pSpriteData->prim &= 0xFE;
}
extern s16 g_BattleActorMask;

void BattleClearActorMask(void) {
    g_BattleActorMask = 0;
}
// The per-slot effect record the ramp's timer entries are keyed by: the second of the two
// pointer tables the battle state work block ends with, holding the sprite each slot's
// ramp animates.
typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ BattleEffectSprite *pSprite;
} BattleEffectSlotRecord; // Size: 0x8

extern WorkListEntry *TimerWorkListFindEntryForDataAndCallback(void *pData, void *pCallback);
extern void BattleStartEffectColorRamp(BattleEffectSlotRecord *pRecord);
extern void BattleApplyEffectSpriteColorRamp(BattleEffectTask *pTask);

// The tail of the battle state work block, as this unit's effects reach it: the render
// context func_800B1F6C's double buffer is indexed by, then the per-slot pointer tables.
typedef struct {
    /* 0x0000 */ u8 unk0[0x8C84];
    /* 0x8C84 */ s32 renderContext; // which of the two prim buffers the draw uses
    /* 0x8C88 */ u8 unk8C88[0x4];
    /* 0x8C8C */ SpriteData *slotSprites[11];
    /* 0x8CB8 */ BattleEffectSlotRecord *slotEffects[11];
} BattleStateWorkTail;

// Applies the battle's slot mask to the per-slot effects: turns the sprite colour pulse on
// when any slot is masked in and off when none is, then walks the slot records in mask
// order, starting the ramp of a masked-in slot that has no timer entry yet and running the
// entry's on-free callback for a masked-out one. The records come from the tail of the work
// block, whose 0x8CB8 offset is what makes gcc split the address the way retail does.
void BattleApplySlotMaskToEffects(u16 mask) {
    BattleStateWorkTail *pWork;
    BattleEffectSlotRecord *pRecord;
    WorkListEntry *pEntry;
    s32 i;

    g_BattleActorMask = mask;
    if (mask != 0) {
        BattleStartSpriteColorPulse();
    } else {
        BattleFreeSpriteColorTask(0);
    }
    pWork = (BattleStateWorkTail *)g_BattleStateWork;
    for (i = 0; i != 0xB; i++, mask >>= 1) {
        if (mask & 1) {
            pRecord = pWork->slotEffects[i];
            if (pRecord != NULL
                && TimerWorkListFindEntryForDataAndCallback(pRecord, BattleApplyEffectSpriteColorRamp) == NULL) {
                BattleStartEffectColorRamp(pRecord);
            }
        } else {
            pRecord = pWork->slotEffects[i];
            if (pRecord != NULL) {
                pEntry = TimerWorkListFindEntryForDataAndCallback(pRecord, BattleApplyEffectSpriteColorRamp);
                if (pEntry != NULL) {
                    pEntry->onFreeCallback(pEntry);
                }
            }
        }
    }
}

extern void SpriteSetIVec3(IVEC3 *pVector, s32 x, s32 y, s32 z);
extern void func_800B1F6C(void *pFrame, void *pPrimBuffer, u_long *pOt, s32 flags, s32 arg4, s32 arg5);

// Draws one step of a colour-ramp effect: the block's position and rotation go through the
// sprite base matrix, the perspective scale comes from the transformed depth, and the model
// frame's primitives are handed to func_800B1F6C out of the buffer the render context picks.
void BattleDrawEffectColorRamp(WorkListEntry *pEntry) {
    BattleEffectColorRampData *pData;
    MATRIX mtx;
    IVEC3 vec;
    s32 depth;
    s32 scale;

    pData = (BattleEffectColorRampData *)pEntry->unk4;
    TransMatrix(&mtx, (VECTOR *)&pData->posX);
    RotMatrix((SVECTOR *)&pData->rotX, &mtx);
    CompMatrix(&g_GfxBaseMatrix, &mtx, &mtx);
    scale = ReadGeomScreen() << 12;
    depth = mtx.t[2];
    if (depth != 0) {
        scale = scale / depth;
    }
    scale = 0x1000000 / scale / 2;
    SpriteSetIVec3(&vec, scale, scale, scale);
    ScaleMatrixL(&mtx, (VECTOR *)&vec);
    SetRotMatrix(&mtx);
    SetTransMatrix(&mtx);
    func_800B1F6C(pData->pFrame,
                  pData->pPrimBuffers[((BattleStateWorkTail *)g_BattleStateWork)->renderContext],
                  g_GfxCurOT, 0, 0, 0);
}

void BattleApplyEffectSpriteColorRamp(BattleEffectTask *pTask) {
    BattleEffectColorRampData *pData;
    BattleEffectSprite *pSprite;
    s32 timer;
    s16 y;
    s16 z;

    pData = pTask->pData;
    pSprite = pData->pSprite;
    timer = pData->angle + 1;
    pData->posX = pSprite->x;
    y = pSprite->y;
    pData->posY = y; // retail stores y, then overwrites it below
    z = pSprite->z;
    pData->posZ = z;
    pData->angle = timer;
    // kept after the timer store: retail loads 0x54 in that slot
    pData->posY = y - pData->yOffset - 0x20;
    BattleSetSpriteColorFromAngle(pSprite, timer, z);
    pData->rotY += 0x10;
}

void BattleFreeEffectSpriteTask(BattleEffectTask *pTask) {
    BattleEffectColorRampData *pData;
    BattleEffectSprite *pSprite;

    pData = pTask->pData;
    pSprite = pData->pSprite;
    pSprite->prim |= 1;
    func_8001F6B0(pSprite);
    GfxQueueSpriteTileData(pData->pPrimBuffers[0]);
    WorkListsRemoveEntriesForData(pTask);
    TimerWorkListRemoveTask(pTask);
    HeapFree(pData);
}

extern u8 *BattleGetFrame(u8 *pBase, s32 n);
extern s32 BattleGetPrimitiveBufferSize(void *pFrame);
extern void BattleExpandModelPrims(void *pFrame, u8 *pDst, s32 mode, u8 shadeTex);
extern void SpriteSetSVec(SVECTOR *pVector, s16 x, s16 y, s16 z);
extern void *memcpy(void *pDst, void *pSrc, u_int size);
extern u8 D_8001C76C[];

// Starts the colour ramp for one of the battle's slot effects: allocates its data block,
// points it at the slot's sprite, and builds the double-buffered prim block (one copy of
// the model frame's packets per render context) the draw callback hands to func_800B1F6C.
void BattleStartEffectColorRamp(BattleEffectSlotRecord *pRecord) {
    BattleEffectColorRampData *pData;
    BattleEffectSprite *pSprite;
    u8 *pBuffer;
    s32 size;

    pSprite = pRecord->pSprite;
    pData = (BattleEffectColorRampData *)WorkListsAddTasks(0x68, pRecord, BattleApplyEffectSpriteColorRamp,
                                                           BattleDrawEffectColorRamp, BattleFreeEffectSpriteTask);
    pData->pSprite = pSprite;
    pData->yOffset = pSprite->yOffset;
    if (g_FlagTimerWorkListAdds != 0) {
        g_NumFlaggedTimerWorkListEntries--;
    }
    pData->timerEntry.unk14_3 = 0; // clears the flagged bit the work list just set
    SpriteSetSVec((SVECTOR *)&pData->rotX, 0, 0, 0);
    size = BattleGetPrimitiveBufferSize(BattleGetFrame(D_8001C76C, 0));
    pBuffer = HeapAlloc(size * 2, 0);
    BattleExpandModelPrims(BattleGetFrame(D_8001C76C, 0), pBuffer, 0, 1);
    memcpy(pBuffer + size, pBuffer, size);
    pData->pPrimBuffers[0] = pBuffer;
    pData->pPrimBuffers[1] = pBuffer + size;
    pData->pFrame = BattleGetFrame(D_8001C76C, 0);
    pSprite->prim &= 0xFE;
    func_8001F6B0(pSprite);
    BattleApplyEffectSpriteColorRamp((BattleEffectTask *)pData);
}
extern s32 g_BattleCommandRowIndex;
extern void *D_800C3E1C;
extern s32 g_BattleEffectTimer;
extern s32 g_BattleColorMode;

// The battle actor sprite BattleAttachEffectSprite spawns an effect for: this unit reads only
// its fixed-point position and bit 2 of its flags word. misc33 declares the whole
// record as BattleActorSprite.
typedef struct {
    /* 0x00 */ IVEC3 position; // 16.16, copied into the effect task
    /* 0x0C */ u8 unkC[0xA0];
    /* 0xAC */ u32 flags;
} Misc32ActorSprite; // Size: 0xB0

// One 0x18-byte sprite-list entry ResourceHelperGetSpriteList fills in (misc33
// declares the same record as BattleSpritePart). BattleDrawSpritePartQuad projects the four
// corners (x,y) + (width,height) and copies the two ids into the primitive.
typedef struct {
    /* 0x00 */ u16 x;
    /* 0x02 */ u16 y;
    /* 0x04 */ u8 u;
    /* 0x05 */ u8 v;
    /* 0x06 */ u8 width;
    /* 0x07 */ u8 height;
    /* 0x08 */ u8 unk8[0x2];
    /* 0x0A */ u16 texPage; // the word BattleDrawSpritePartQuad copies into the primitive's +0x16
    /* 0x0C */ u16 clutID; // and the one it copies into +0x0E
    /* 0x0E */ u8 unkE[0xA];
} Misc32SpritePart; // Size: 0x18

// The effect task WorkListsAddTasks allocates for BattleAttachEffectSprite (0x1B0 bytes): the
// two work-list entries, the render transform func_800BDA1C walks (the rotation at
// 0x40, the fixed-point position the step callbacks advance at 0x48, the scale at
// 0x58) and the sprite list at 0x90 whose count is at 0x8C. Every function in this
// unit drives one.
typedef struct Misc32Task {
    /* 0x00 */ WorkListEntry timerEntry;
    /* 0x1C */ WorkListEntry workEntry;
    /* 0x38 */ struct Misc32Task *pNext; // link in g_BattleTaskUnlinkList
    /* 0x3C */ s32 unk3C;
    /* 0x40 */ SVECTOR rot;
    /* 0x48 */ VECTOR pos; // 16.16; the step callbacks advance vx
    /* 0x58 */ VECTOR scale;
    /* 0x68 */ u8 unk68[0x10];
    /* 0x78 */ Misc32ActorSprite *pSprite; // the actor the effect is drawn for
    /* 0x7C */ u8 mirrored; // the actor sprite's flip bit; nonzero: posX steps the other way
    /* 0x7D */ u8 unk7D[0x3];
    // Redrawn as one word every part quad is drawn with, and as the bytes
    // BattleStepEffectColorFade fades and the effect's flag bits.
    /* 0x80 */ union {
        /* 0x80 */ u32 word;
        /* 0x80 */ struct {
            /* 0x80 */ u8 red;
            /* 0x81 */ u8 green;
            /* 0x82 */ u8 blue;
            /* 0x83 */ u8 flags; // bit 0 cleared and bit 1 set as the effect starts
        } channels;
    } color;
    /* 0x84 */ u32 unk84;
    /* 0x88 */ s32 timer;
    /* 0x8C */ s32 partCount;
    /* 0x90 */ Misc32SpritePart parts[12];
} Misc32Task; // Size: 0x1B0

extern Misc32Task *g_BattleTaskUnlinkList;
extern void *g_BattleState;
extern u8 *g_BattleSpriteArchive;
extern s16 g_BattleDigitTextureIds[];
extern u8 g_BattleTaskListBusy;
extern Misc32Task *WorkListsAddTasks(s32 allocSize, void *pData, void *pTimerCallback,
                                     void *pWorkCallback, void *pOnFreeCallback);
extern s32 ResourceHelperGetSpriteList(u8 *pDir, u32 index, Misc32SpritePart *pOut, s16 x, s16 y);
extern s32 BattleWriteDecimalDigits(s32 value, u8 *pBuf, s32 n, u8 started, s32 base);
extern void BattleInitNumberSpriteTask(void);
extern void func_800BDA1C(WorkListEntry *pEntry);
extern void BattleUnlinkAndFreeTask(void *pEntry);
extern void BattleStepEffectColorFade(Misc32Task *pTask);
extern void BattleStepEffectPosition(Misc32Task *pTask);
extern void TimerWorkListSetTaskCallback(void *pTask, void *pCallback);

extern void BattleAttachEffectSprite(Misc32ActorSprite *pSprite, s32 value, s32 kind);
extern void BattleAttachCommandRowEffect(s32 slot);

// Parked at 92.59%, one instruction SHORT (228 bytes vs retail's 232), so it cannot
// link: retail loads the slot's sprite into $t0 and copies it into $a0 in the jal's
// delay slot (`move a0,t0`), while gcc here allocates the sprite straight to $a0,
// deletes the copy and puts the g_BattleColorMode store in the slot instead. Every
// other instruction is retail's, including the two differing addu operand orders
// (`addu a3,slot*2,row` for the u16 arrays, `addu a1,row,slot` for the u8 ones) that
// the typed row access reproduces and the raw pointer spelling does not: that older
// form measured 91.07% and 236 bytes, one instruction LONG. The row is
// g_BattleStateWork + (g_BattleCommandRowIndex - 1) * 0x48, re-derived after the call
// because retail reloads the global there; its four per-slot arrays are the block
// misc12 reaches as g_BattleCommandRows[], so the view below starts at
// g_BattleStateWork and carries the 0x138 offset the arguments need.
// Tried: one pointer reused for both index sums, (u32) casts of the sums, the row
// base split over two statements, the `kind` local vs. the direct argument, both
// store orders, and direct (u8 *) pointer arithmetic. Shapes that do force a real
// argument copy need a second saved register (frame 0x28 instead of 0x20).
// The battle work block as the command-row code sees it: this unit only reaches one
// row's four 11-entry per-slot arrays.
typedef struct {
    u8 unk0[0x138];        // 0x000
    u16 commandValues[11]; // 0x138 the value the slot's effect sprite draws
    u8 pad14E[0x2];        // 0x14E
    u8 commandKinds[11];   // 0x150 the effect kind; 0xFF: the slot has no command
    u8 pad15B[0x1];        // 0x15B
    u16 timers[11];        // 0x15C published as g_BattleEffectTimer
    u8 pad172[0x2];        // 0x172
    u8 colorModes[11];     // 0x174 published as g_BattleColorMode
    u8 pad17F[0x1];        // 0x17F
} BattleCommandRowTable;  // Size: 0x180

// Skips a slot whose row has no command, no command value or no sprite, and hands the
// row's kind and the sprite to BattleAttachEffectSprite after publishing the row's
// timer and colour mode. The row is g_BattleStateWork + (g_BattleCommandRowIndex - 1) *
// 0x48, re-derived after the call because retail reloads that global there; its four
// per-slot arrays are the block misc12 reaches as g_BattleCommandRows[], so the view
// below starts at g_BattleStateWork and carries the 0x138 offset the arguments need.
// Every shape here is load-bearing (found by permuter, re-verified one edit at a time):
// the s32 timer local next to the u8 colour-mode local fixes which of the two loads
// retail schedules first, and the re-derivation goes through the u8 * because a single
// cast expression costs the jal its `move a0,t0` delay slot.
void BattleAttachCommandRowEffect(s32 slot) {
    u8 *pBase;
    BattleCommandRowTable *pRow;
    s32 nextTimer;
    u8 *pRowAddr;
    u8 colorMode;
    void *pSprite;
    s32 kind;

    pBase = g_BattleStateWork;
    pRow = (BattleCommandRowTable *)(pBase + (g_BattleCommandRowIndex - 1) * 0x48);
    if (pRow->commandKinds[slot] == 0xFF) {
        return;
    }
    if (pRow->commandValues[slot] == 0xFFFF) {
        return;
    }
    pSprite = *(void **)(pBase + slot * 4 + 0x8C8C);
    if (pSprite == NULL) {
        return;
    }
    kind = pRow->commandKinds[slot];
    nextTimer = pRow->timers[slot];
    colorMode = pRow->colorModes[slot];
    g_BattleEffectTimer = nextTimer;
    g_BattleColorMode = colorMode;
    BattleAttachEffectSprite(pSprite, pRow->commandValues[slot], kind);
    pRowAddr = pBase + (g_BattleCommandRowIndex - 1) * 0x48;
    pRow = (BattleCommandRowTable *)pRowAddr;
    pRow->commandValues[slot] = 0xFFFF;
}

void BattleUpdateCommandRows(void) {
    u8 *pSlot;
    u8 *pBase;
    void **pEntry;
    u8 flag;
    mem_addr rowAddr;
    s32 i;

    flag = 0;
    i = 0;
    pSlot = g_BattleStateWork;
    pBase = pSlot;
    while (i != 0xB) {
        rowAddr = (g_BattleCommandRowIndex - 1) * 0x48 + (mem_addr)pBase + i + 0x150;
        if (*(u8 *)rowAddr == 7) {
            flag = 1;
        }
        pEntry = (void **)(pSlot + 0x8C8C);
        if (*pEntry != NULL && (D_800C3E1C != *pEntry || !flag)) {
            // Two copies of the call: loop.c counts the extra branch before jump
            // opt merges them, which is what keeps the g_BattleCommandRowIndex address from
            // being hoisted out of the loop.
            if (flag) {
                BattleAttachCommandRowEffect(i);
            } else {
                BattleAttachCommandRowEffect(i);
            }
        }
        pSlot += 4;
        i++;
    }
}
// Spawns the sprite-list effect that draws <value> next to <pSprite>. <kind> selects
// the frame the effect is in: 4 is the spawn frame, before the number's digits are
// added to the sprite list; the other kinds pick the number's colour channels.
void BattleAttachEffectSprite(Misc32ActorSprite *pSprite, s32 value, s32 kind) {
    Misc32Task *pTask;
    Misc32Task *pCur;
    WorkListCallback_t pCallback;
    u8 digits[6];
    u8 unkBuf[0x30]; // unused: reproduces retail's 0x68 frame
    s32 tex;
    s32 i;

    if (g_BattleState == NULL) {
        return;
    }
    BattleInitNumberSpriteTask();
    pCur = g_BattleTaskUnlinkList;
    g_BattleTaskListBusy = 0;
    while (pCur != NULL) {
        if (pCur->pSprite == pSprite) {
            pCallback = pCur->timerEntry.onFreeCallback;
            pCallback(pCur);
        }
        pCur = pCur->pNext;
    }
    pTask = WorkListsAddTasks(0x1B0, NULL, BattleStepEffectPosition, func_800BDA1C, BattleUnlinkAndFreeTask);
    pTask->pSprite = pSprite;
    pTask->timer = 8;
    pTask->pNext = g_BattleTaskUnlinkList;
    pTask->pos.vx = pSprite->position.x;
    g_BattleTaskUnlinkList = pTask;
    pTask->pos.vy = pSprite->position.y;
    pTask->pos.vz = pSprite->position.z;
    pTask->scale.vx = 0x2000;
    pTask->scale.vy = 0x2000;
    pTask->scale.vz = 0x2000;
    pTask->color.channels.flags = 0x2D;
    pTask->rot.vx = 0;
    pTask->rot.vy = 0;
    pTask->rot.vz = 0;
    pTask->color.channels.red = 0x80;
    pTask->color.channels.green = 0x80;
    pTask->color.channels.blue = 0x80;
    pTask->mirrored = (pSprite->flags >> 2) & 1;
    pTask->partCount = 0;
    if (kind != 4) {
        BattleWriteDecimalDigits(value, digits, 5, 0, 0);
        tex = g_BattleDigitTextureIds[digits[0]];
    }
    switch (kind) {
    // retail emits the case bodies in this order, so the labels are listed in it
    case 4:
        pTask->timer = 0x40;
        pTask->partCount = ResourceHelperGetSpriteList(g_BattleSpriteArchive, 0x7C, &pTask->parts[0], -0x14, -0x10);
        pTask->color.channels.flags = (pTask->color.channels.flags & 0xFE) | 2;
        TimerWorkListSetTaskCallback(pTask, BattleStepEffectColorFade);
        break;
    case 5:
        pTask->color.channels.red = 0x80;
        pTask->color.channels.green = 0;
        pTask->color.channels.blue = 0;
        pTask->color.channels.flags = (pTask->color.channels.flags & 0xFE);
        break;
    case 10:
        pTask->partCount += ResourceHelperGetSpriteList(g_BattleSpriteArchive, 0x7F,
                                                        &pTask->parts[pTask->partCount], tex, 0);
        break;
    case 11:
        pTask->partCount += ResourceHelperGetSpriteList(g_BattleSpriteArchive, 0x91,
                                                        &pTask->parts[pTask->partCount], tex, 0);
        pTask->color.channels.blue = 0x80;
        pTask->color.channels.red = 0;
        pTask->color.channels.green = 0;
        pTask->color.channels.flags = (pTask->color.channels.flags & 0xFE);
        break;
    case 1:
        pTask->partCount += ResourceHelperGetSpriteList(g_BattleSpriteArchive, 0x80,
                                                        &pTask->parts[pTask->partCount], tex, -0x10);
        tex += 0x20;
        break;
    case 3:
        pTask->partCount += ResourceHelperGetSpriteList(g_BattleSpriteArchive, 0x80,
                                                        &pTask->parts[pTask->partCount], tex, -0x10);
        tex += 0x20;
        pTask->color.channels.red = 0x80;
        pTask->color.channels.blue = 0x80;
        pTask->color.channels.green = 0;
        pTask->color.channels.flags = (pTask->color.channels.flags & 0xFE);
        break;
    case 2:
        pTask->color.channels.red = 0;
        pTask->color.channels.green = 0x80;
        pTask->color.channels.blue = 0;
        pTask->color.channels.flags = (pTask->color.channels.flags & 0xFE);
        break;
    }
    if (kind != 4) {
        i = 0;
        while (i != digits[0]) {
            pTask->partCount += ResourceHelperGetSpriteList(g_BattleSpriteArchive, digits[i + 1] + 0x72,
                                                            &pTask->parts[pTask->partCount], tex, -0x10);
            i++;
            tex += 10;
        }
    }
}

extern void WorkListsDeleteTasks(WorkListEntry *pTasks);

// The g_BattleTaskUnlinkList list's on-free callback: unlink the entry, then free it.
void BattleUnlinkAndFreeTask(void *pEntry) {
    Misc32Task *pCur;
    Misc32Task *pPrev;
    Misc32Task *pTarget;

    pCur = g_BattleTaskUnlinkList;
    pPrev = NULL;
    pTarget = pEntry;
    while (pCur != NULL) {
        if (pCur == pTarget) {
            if (pPrev != NULL) {
                pPrev->pNext = pCur->pNext;
            } else {
                g_BattleTaskUnlinkList = pCur->pNext;
            }
            break;
        }
        pPrev = pCur;
        pCur = pCur->pNext;
    }
    WorkListsDeleteTasks(pEntry);
}

// The four sprite corners BattleDrawSpritePartQuad hands RotTransPers4, reused by every sprite of a
// frame: the entry's rectangle in its own order, top-left, top-right, bottom-right, bottom-left.
extern SVECTOR g_BattleSpriteQuadCorners[4];

#define BATTLE_SPRITE_QUAD_TAG_LEN 9 // POLY_FT4's packet length in words, minus the tag's own word

// Draws one sprite-list entry: fills a textured quad in the packet buffer from the entry's
// rectangle and texture page, projects its four corners through the battle's view matrix and
// links the prim into the ordering table. Nothing is drawn once the packet buffer is full.
// The right/bottom sum pairs and the local ordering-table pointer are load-bearing: each copy
// fixes one scheduling/register choice that retail makes (the scheduler runs here, so the
// statement order decides nothing, but which pseudo carries the sum does).
void BattleDrawSpritePartQuad(Misc32SpritePart *pPart, u32 color) {
    POLY_FT4 *pPoly;
    u_long *pOt;
    s32 width;
    s32 height;
    s32 x;
    s32 y;
    s32 right;
    s32 rightEdge;
    s32 bottom;
    s32 u;
    s32 v;
    s32 p;
    s32 bottomEdge;
    s32 flag;

    pPoly = (POLY_FT4 *)g_GfxCurWorkBuffer;
    if ((u8 *)(pPoly + 1) >= g_GfxCurWorkBufferEnd) {
        return;
    }
    g_GfxCurWorkBuffer = pPoly + 1;
    setlen(pPoly, BATTLE_SPRITE_QUAD_TAG_LEN);
    *(u_long *)&pPoly->r0 = color;
    pPoly->tpage = pPart->texPage;
    pPoly->clut = pPart->clutID;
    width = pPart->width;
    height = pPart->height;
    x = pPart->x;
    y = pPart->y;
    rightEdge = x + width;
    right = rightEdge;
    bottomEdge = y + height;
    bottom = bottomEdge;
    g_BattleSpriteQuadCorners[0].vx = x;
    g_BattleSpriteQuadCorners[0].vy = y;
    g_BattleSpriteQuadCorners[1].vx = right;
    g_BattleSpriteQuadCorners[1].vy = y;
    g_BattleSpriteQuadCorners[2].vx = right;
    g_BattleSpriteQuadCorners[2].vy = bottom;
    g_BattleSpriteQuadCorners[3].vx = x;
    g_BattleSpriteQuadCorners[3].vy = bottom;
    // the screen corners go in the prim's vertex order, top-left, top-right, bottom-left,
    // bottom-right, which is how the GPU splits a quad into (0,1,2) and (1,2,3)
    RotTransPers4(&g_BattleSpriteQuadCorners[0], &g_BattleSpriteQuadCorners[1], &g_BattleSpriteQuadCorners[2], &g_BattleSpriteQuadCorners[3],
                  (long *)&pPoly->x0, (long *)&pPoly->x1, (long *)&pPoly->x3, (long *)&pPoly->x2,
                  &p, &flag);
    u = pPart->u;
    v = pPart->v;
    right = u + pPart->width;
    bottom = v + pPart->height;
    pPoly->u0 = u;
    pPoly->v0 = v;
    pPoly->u1 = right;
    pPoly->v1 = v;
    pPoly->u2 = u;
    pPoly->v2 = bottom;
    pPoly->u3 = right;
    pPoly->v3 = bottom;
    pOt = g_GfxCurOT;
    addPrim(pOt, pPoly);
}

void BattleProjectPointToScreen(SVECTOR *pVec, s32 *pOut) {
    SVECTOR vec; // unused: reproduces retail's 0x40 frame
    s16 sxy[4];
    s32 p;
    s32 flag;
    s32 ofx;
    s32 ofy;

    SetRotMatrix(&g_BattleViewMatrix);
    SetTransMatrix(&g_BattleViewMatrix);
    RotTransPers(pVec, (s32 *)sxy, &p, &flag);
    ReadGeomOffset(&ofx, &ofy);
    pOut[0] = (sxy[0] - ofx) * 2;
    pOut[1] = (sxy[1] - ofy) * 2;
    pOut[2] = ReadGeomScreen();
}
// The GTE geom-output block g_BattleGeomMatrix doubles as a matrix: its t[2] word is the
// geom screen offset (misc33 declares the same struct as BattleGeomMatrix).
typedef struct {
    /* 0x00 */ u8 unk0[0x1C];
    /* 0x1C */ s32 geomScreen;
} BattleGeomMatrix; // Size: 0x20

extern BattleGeomMatrix g_BattleGeomMatrix;

// The effect task's work callback: builds the render transform from the ten fixed-point
// position, the rotation and the scale, then draws every part. The three `>> 16` reads
// are what turns into retail's `lh` loads of the position's integer halves.
void func_800BDA1C(WorkListEntry *pEntry) {
    Misc32Task *pTask;
    MATRIX mtx;
    SVECTOR vec;
    s32 sxy[4];
    s32 i;
    Misc32SpritePart *pPart;

    pTask = (Misc32Task *)pEntry->unk4;
    g_BattleGeomMatrix.geomScreen = ReadGeomScreen();
    vec.vx = pTask->pos.vx >> 16;
    vec.vy = pTask->pos.vy >> 16;
    vec.vz = pTask->pos.vz >> 16;
    BattleProjectPointToScreen(&vec, sxy);
    RotMatrix(&pTask->rot, &mtx);
    TransMatrix(&mtx, (VECTOR *)sxy); // only x/y/z are set by the projection
    CompMatrix((MATRIX *)&g_BattleGeomMatrix, &mtx, &mtx);
    ScaleMatrix(&mtx, &pTask->scale);
    SetRotMatrix(&mtx);
    SetTransMatrix(&mtx);
    i = 0;
    pPart = pTask->parts;
    while (i != pTask->partCount) {
        BattleDrawSpritePartQuad(pPart, pTask->color.word);
        i++;
        pPart++;
    }
}// Declared after the struct: the timer update takes the task. It reads none of
// the argument's fields, it works off g_BattleTimerTask.
extern void BattleRefreshNumberSprite(Misc32Task *pTask);

void BattleStepEffectColorFade(Misc32Task *pTask) {
    WorkListCallback_t pCallback;

    BattleRefreshNumberSprite(pTask);
    pTask->color.channels.red -= 2;
    pTask->color.channels.green -= 2;
    pTask->color.channels.blue -= 2;
    if (--pTask->timer < 0) {
        pCallback = pTask->timerEntry.onFreeCallback;
        pCallback(pTask);
    }
}

extern u8 SpriteColorAddClamp(u8 arg0, s16 arg1);

// The effect task, reached through its own alias: retail keeps a second copy of the
// task pointer in $s1 (0x20 frame) and makes the callback through it, so the fade
// stores and the timer update must not share one variable. The two volatile
// re-reads are required too: without them cse forwards call 1's stored result into
// call 2's argument (86.00 instead of 100.00).
void BattleStepEffectColorFadeFromBase(Misc32Task *pTask) {
    Misc32Task *pEffect;
    WorkListCallback_t pCallback;

    pEffect = pTask;
    BattleRefreshNumberSprite(pTask);
    // the volatile re-reads are required: without them cse forwards the previous
    // call's stored result into the next call's argument
    pEffect->color.channels.red = SpriteColorAddClamp(pEffect->color.channels.red, -8);
    pEffect->color.channels.green = SpriteColorAddClamp(*(volatile u8 *)&pTask->color.channels.red, -8);
    pEffect->color.channels.blue = SpriteColorAddClamp(*(volatile u8 *)&pEffect->color.channels.red, -8);
    if (--pTask->timer < 0 || (pTask->color.channels.red | pTask->color.channels.green | pEffect->color.channels.blue) == 0) {
        pCallback = pTask->timerEntry.onFreeCallback;
        pCallback(pEffect);
    }
}

extern void TimerWorkListSetTaskCallback(void *pTask, void *pCallback);
extern void BattleStepEffectColorFadeFromBase(Misc32Task *pTask);

// 91.60%: COMPILES (parked body below). `nextTimer` is worth +8.2 (it stops gcc
// folding 0x10 into the branch); the rest of the block is retail's, per the diff.
// CAUSE (round 115, from the cc1 -da dumps: misc32.flow vs misc32.lreg): the
// argument copy `(set (reg 4 a0) (reg 80))` is the parameter's LAST use, so
// local-alloc's combine_regs merges reg 80 into $a0, rewrites the two stores to
// `sw v1,136(a0)` / `sb v0,131(a0)` and reorders the block to put the copy first.
// Retail's own object proves combine_regs did not fire there: its `move $a0,$s0`
// sits at block position 3, before its `sw v1,136(s0)`, so $a0 is defined before
// those stores and they still use $s0. The lever is therefore a shape in which the
// parameter is still live when the copy is scanned, or one whose stores reach the
// base via a pseudo combine_regs cannot merge into $a0.
// Tried (all 91.60 or worse): a `u8 flags` temp, both store orders, the whole block
// in do{...}while(0) (55.56), the call alone in do{...}while(0) (67.40), a `pEffect`
// alias for the call argument (cse folds it), an alias for the stores, u8 pointer,
// offset casts, a callback local, indexed forms, and a 3-min permuter run.
// Round 116: moving the flags STORE after the call (compute the byte into a `u8
// flags` local before it, `pTask->color.channels.flags = flags;` after it) DOES defeat
// combine_regs - the parameter then stays in a saved register with a real
// `move a0,sN` argument copy - but the flag byte then lives across the call and
// needs a SECOND saved register (0x20 frame, 108 bytes, 70.36%). Wrapping only the
// timer store in do{...}while(0), the lever that closed BattleStepEffectPosition
// next door, reproduces that function's shape instead (store straight after the
// bgez, so reorg steals the load-delay nop: 96 bytes, 68.96%). A 4-min permuter run
// from the 91.60 body: base=70 best=70, 1645 iterations, nothing. Retail's object
// has the same instruction multiset as the 91.60 body; the only differences are
// $a0 vs $s0 on the sw/sb and the position of the move, i.e. a local-alloc choice.
// Round 185 re-measured: 91.60% / 100 bytes with the nextTimer local (91.40% without
// it), so the diagnosis below stands. Dropping the local moves retail's `move a0,s0`
// one slot later but does not change $a0-vs-$s0 on the three store bases: retail's
// timer/flags base is the call-saved copy $s0 while gcc merges the call's argument
// pseudo into it and stores through $a0. The instruction ORDER is already retail's.
// Round 405 re-measured, four more shapes, all <= 91.60 (100 bytes = retail's):
// a `pEffect = pTask;` alias for the three stores (identical 91.60 code, gcc
// coalesces the alias), `&pTask->timerEntry` as the call's first argument (timerEntry
// is at 0, folded back), the call FIRST in the block then the two stores (86.60:
// s0 becomes the store base but the jal lands before the stores), and a
// do{...}while(0) around the flag store (67.00) or around both stores (55.56,
// 96 bytes). The residual is one local-alloc choice: the store base shares $a0
// with the call's argument copy. Retail's own RTL kept the argument copy in the
// middle of the block (pre-reload sched-insns hoisted it there), so $s0 still had
// uses after it and combine_regs did not fire; every spelling here puts the copy
// last, s0 dies there and the stores are rewritten to $a0.
// REDO LANE, two new measurements (both 100 bytes = retail's): (1) moving the timer
// reset AFTER the call in source does keep every store base on $s0 and lands the flag
// `sb` in the jal delay slot, but scores 81.60 - so the residual is the store BASE
// ($a0 vs $s0) and not the order; and the timer store then stays after the call,
// because pre-reload sched-insns will not move an independent store across a jal, so
// "retail typed the reset last" is ruled out. (2) preset screen: gcc-2.6.0-psx 76.76,
// gcc-2.7.2-psx under the unit flags compile-failed - not a preset question.
// ROUND 437 re-measured the park (91.60%, 100 bytes = retail's) and found the
// mechanism, which no earlier round named: local-alloc gives the store ADDRESS
// pseudo $a0 because $a0 is where the argument copy lands, so one `move a0,s0`
// serves both the two stores and the call; when the call's argument is instead a
// value the compiler must compute, $a0 does not hold pTask at local-alloc time and
// the store addresses FOLD back into the $s0 offset form retail has. Measured it
// with the argument read back out of the task's own entry
// (`pEffect->timerEntry.unk4`, equal to pTask here because BattleInitNumberSpriteTask
// stores pTask there): both stores on $s0 and 97.60% at retail's 100 bytes - but
// retail has `move a0,s0` there, not `lw a0,4(s0)`, so that body is not the
// original and was rejected. Other round-437 measurements, all 91.60% / 100 bytes
// and byte-identical to the parked body: the flags store through a
// `u8 *pFlags = (u8 *)pTask + 0x83;` (the address folds into the $s0 offset for the
// lbu and into the $a0 base for the sb), passing the `pEffect` alias itself as the
// call's first argument, and a `do { pTask->timer = nextTimer; } while (0)` around
// the timer store with the flags store before it (that one is 49.16: the barrier
// drags the constant into the load-delay slot and moves the whole arm).
// ROUND 477 (this round) CONFIRMED the merge is the whole residual and measured the one
// construct that flips it: an EMPTY `do { } while (0);` between the flags update and the
// call is a basic-block boundary, so local-alloc has no copy in that block and both store
// bases land on $s0 with a real `move a0,s0` at retail's 100 bytes - but the block split
// then gives the scheduler a different block, so the a1 address pair splits around the jal
// and the sb leaves the delay slot: 67.40%. So the shape that yields $s0 bases and the
// shape that yields retail's schedule are the same block, where the pseudo dies at the
// argument copy. Also measured byte-identical to the 91.60 body: a `void *pArg` parameter
// with a `pTask = (Misc32Task *)pArg;` local and the call passing pArg (cse folds the
// cast, so the copy is free while the same value is live at the call), and a dead store to a
// never-read `Misc32Task *pKeep;` after the call (deleted before local-alloc, so it does
// NOT keep the pseudo alive). A park note that says "retail kept the argument copy in the
// middle of the block" is right about the emission and wrong about the handle: the copy's
// position is a consequence of combine_regs, not a schedule that can be re-ordered.
// ROUND 490: three more measurements, none a new best. The `nextTimer` local is load-bearing:
// writing the literal `pTask->timer = 0x10;` inside the arm gives 91.40 and folds the colour
// `lbu` into $a0 as well as the two stores (the pre-if constant keeps the load on $s0).
// Swapping the two arm statements (colour first, timer second) is byte-identical to that, so
// source statement order is inert here, as sched-insns implies. Diff shape at 91.60 is exactly
// one displaced insn: retail has `move a0,s0` 3rd in the arm (after `lui a1`) and both stores
// on $s0, this build schedules the same `move a0,s0` 1st in the arm and combines the two
// store addresses into $a0.
/*
void func_800BDC14(Misc32Task *pTask) {
    s32 nextTimer;

    BattleRefreshNumberSprite(pTask);
    nextTimer = 0x10;
    if (--pTask->timer < 0) {
        pTask->timer = nextTimer;
        pTask->color.channels.flags = (pTask->color.channels.flags | 2) & 0xFE;
        TimerWorkListSetTaskCallback(pTask, BattleStepEffectColorFadeFromBase);
    }
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDC14);


extern void func_800BDC14(Misc32Task *pTask);

void BattleStepEffectPosition(Misc32Task *pTask) {
    BattleRefreshNumberSprite(pTask);
    if (pTask->mirrored != 0) {
        pTask->pos.vx += 0xC0000;
    } else {
        pTask->pos.vx -= 0xC0000;
    }
    if (--pTask->timer < 0) {
        do { /* the do-while stops reorg hoisting the call's move a0,s0 */
            pTask->timer = 0x10;
        } while (0);
        TimerWorkListSetTaskCallback(pTask, func_800BDC14);
    }
}
extern Misc32Task *g_BattleTimerTask;
extern void WorkListRemoveTask(WorkListEntry *pEntry);

void BattleFreeTimerTask(Misc32Task *pTask) {
    g_BattleTimerTask = NULL;
    WorkListRemoveTask(&pTask->workEntry);
    TimerWorkListRemoveTask(&pTask->timerEntry);
}
void BattleTimerTaskNoOp(void) {
}

// The battle's static timer task block (g_BattleNumberSpriteTask): the timer entry, the work entry
// BattleDrawNumberSpriteTask draws, and the number sprite that entry transforms. Its colour word,
// part count and parts sit 0x10 lower than misc33's BattleSpriteBody because the two
// work-list entries come in front of them.
typedef struct {
    /* 0x00 */ WorkListEntry timerEntry;
    /* 0x1C */ WorkListEntry workEntry;
    /* 0x38 */ SVECTOR rot; // passed to RotMatrix
    /* 0x40 */ VECTOR scale; // and to ScaleMatrix
    /* 0x50 */ s16 x; // handed to func_80026BA4 as the sprite it draws and its position
    /* 0x52 */ s16 y;
    /* 0x54 */ u8 unk54[0xC];
    // Set up one channel at a time by BattleRefreshNumberSprite, drawn as the whole prim colour word.
    /* 0x60 */ union {
        /* 0x60 */ u32 word;
        /* 0x60 */ struct {
            /* 0x60 */ u8 red;
            /* 0x61 */ u8 green;
            /* 0x62 */ u8 blue;
            /* 0x63 */ u8 code; // the quad's code byte; 0x2C is textured with semi-transparency
        } channels;
    } color;
    /* 0x64 */ s32 partCount;
    /* 0x68 */ Misc32SpritePart parts[6]; // a body part plus the five digits
} BattleNumberTask; // Size: 0xF8

extern BattleNumberTask g_BattleNumberSpriteTask;
extern s32 g_BattleNumberSpriteValue;
extern u8 g_BattleBlockingEffectActive;

extern void func_80026BA4(u8 *pArchive, u32 index, s32 x, s32 y, u_long *pOt);
extern void WorkListAddTask(WorkListEntry *pData, WorkListEntry *pEntry);
extern void WorkListSetTaskCallback(WorkListEntry *pEntry, void *pCallback);
extern void WorkListTaskSetOnFreeCallback(WorkListEntry *pEntry, void *pCallback);
extern void TimerWorkListAddTask(void *pData, WorkListEntry *pEntry);

// The number sprite's draw callback: draws the sprite body func_80026BA4 hands out at
// the centre of the screen, then the same transform block misc33's BattleRenderSpriteBody
// uses with this task's rotation and scale, and one textured quad per part.
void BattleDrawNumberSpriteTask(WorkListEntry *pEntry) {
    BattleNumberTask *pTask;
    Misc32SpritePart *pPart;
    MATRIX mtx;
    SVECTOR vec; // unused: reproduces retail's 0x68 frame
    s32 pos[4]; // the VECTOR TransMatrix is given; only x/y/z are set
    s32 ofx;
    s32 ofy;
    s32 i;

    pTask = (BattleNumberTask *)pEntry->unk4;
    ReadGeomOffset(&ofx, &ofy);
    func_80026BA4(g_BattleSpriteArchive, 0x81, pTask->x, pTask->y, g_GfxCurOT);
    pos[0] = (0xA0 - ofx) * 2;
    pos[1] = (0x46 - ofy) * 2;
    pos[2] = ReadGeomScreen();
    g_BattleGeomMatrix.geomScreen = ReadGeomScreen();
    RotMatrix(&pTask->rot, &mtx);
    TransMatrix(&mtx, (VECTOR *)pos);
    CompMatrix((MATRIX *)&g_BattleGeomMatrix, &mtx, &mtx);
    ScaleMatrix(&mtx, &pTask->scale);
    SetRotMatrix(&mtx);
    SetTransMatrix(&mtx);
    i = 0;
    pPart = pTask->parts;
    while (i != pTask->partCount) {
        BattleDrawSpritePartQuad(pPart, pTask->color.word);
        i++;
        pPart++;
    }
}

// Creates the static number-sprite task once per battle: both work-list entries, the
// callbacks, the fixed on-screen position and the colour word BattleRefreshNumberSprite fills in.
void BattleInitNumberSpriteTask(void) {
    BattleNumberTask *pTask;

    if (g_BattleTimerTask != NULL) {
        return;
    }
    if (g_BattleBlockingEffectActive != 0) {
        return;
    }
    pTask = &g_BattleNumberSpriteTask;
    g_BattleTimerTask = (Misc32Task *)pTask;
    TimerWorkListAddTask(NULL, &pTask->timerEntry);
    WorkListAddTask(&pTask->timerEntry, &pTask->workEntry);
    TimerWorkListSetTaskCallback((Misc32Task *)pTask, BattleTimerTaskNoOp);
    WorkListSetTaskCallback(&pTask->workEntry, BattleDrawNumberSpriteTask);
    WorkListTaskSetOnFreeCallback(&pTask->timerEntry, BattleFreeTimerTask);
    pTask->x = 0x90;
    pTask->y = 0x2A;
    pTask->timerEntry.unk4 = pTask;
    pTask->workEntry.unk4 = pTask;
    pTask->partCount = 0;
    g_BattleNumberSpriteValue = -1;
}
extern void BattleCreateNumberSpriteTask(s32 value);

// Refills the static number sprite whenever the battle's effect timer changes: hands the new
// value to the sprite body, colours it the way the battle's colour mode asks and rebuilds its
// part list from the digits of the value.
void BattleRefreshNumberSprite(Misc32Task *pTask) {
    BattleNumberTask *pNumber;
    u8 digits[6];
    s32 value;
    s32 mode;
    s32 i;
    s32 tex;

    value = g_BattleEffectTimer;
    if (g_BattleTimerTask == NULL) {
        return;
    }
    if (g_BattleBlockingEffectActive != 0) {
        return;
    }
    pNumber = (BattleNumberTask *)g_BattleTimerTask;
    if (value == g_BattleNumberSpriteValue) {
        return;
    }
    g_BattleNumberSpriteValue = value;
    BattleCreateNumberSpriteTask(value);
    pNumber->color.channels.code = 0x2D;
    pNumber->scale.vx = 0x2000;
    pNumber->scale.vy = 0x2000;
    pNumber->scale.vz = 0x2000;
    pNumber->color.channels.code &= 0xFE;
    mode = g_BattleColorMode;
    pNumber->rot.vx = 0;
    pNumber->rot.vy = 0;
    pNumber->rot.vz = 0;
    switch (mode) {
    // retail emits the case bodies in this order, so the labels are listed in it
    case 0xB:
        pNumber->color.channels.red = 0;
        pNumber->color.channels.green = 0;
        pNumber->color.channels.blue = 0x80;
        break;
    case 3:
        pNumber->color.channels.red = 0x80;
        pNumber->color.channels.green = 0;
        pNumber->color.channels.blue = 0x80;
        break;
    case 2:
        pNumber->color.channels.red = 0;
        pNumber->color.channels.green = 0x80;
        pNumber->color.channels.blue = 0;
        break;
    default:
        pNumber->color.channels.red = 0x80;
        pNumber->color.channels.green = 0x80;
        pNumber->color.channels.blue = 0x80;
        break;
    }
    BattleWriteDecimalDigits(value, digits, sizeof(digits) - 1, 0, 0);
    tex = g_BattleDigitTextureIds[digits[0]];
    pNumber->partCount = 0;
    i = 0;
    while (i != digits[0]) {
        pNumber->partCount += ResourceHelperGetSpriteList(g_BattleSpriteArchive, digits[i + 1] + 0x72,
                                                          &pNumber->parts[pNumber->partCount], tex, -8);
        i++;
        tex += 10;
    }
}
extern void BattleFreeTimerTask(Misc32Task *pTask);

void BattleFreeTimerTaskIfActive(void) {
    if (g_BattleTimerTask != NULL) {
        BattleFreeTimerTask(g_BattleTimerTask);
    }
}
extern s32 D_800C374C;

void BattleResetTimerTaskGlobals(void) {
    g_BattleTimerTask = NULL;
    D_800C374C = 0;
}
