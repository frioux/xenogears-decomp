#include "common.h"
#include "psyq/libgte.h"
#include "system/animation.h"

extern void func_8001F6B0(void *pObj);
extern void func_80025180(void *pData);
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BCC60);
extern s16 g_BattleActorMask;

void BattleClearActorMask(void) {
    g_BattleActorMask = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BCD98);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BCEAC);
// The sprite record a color-ramp task animates.
typedef struct {
    /* 0x00 */ u8 unk0[0x2];
    /* 0x02 */ s16 unk2;
    /* 0x04 */ u8 unk4[0x2];
    /* 0x06 */ s16 unk6;
    /* 0x08 */ u8 unk8[0x2];
    /* 0x0A */ s16 unkA;
    /* 0x0C */ u8 unkC[0x1C];
    /* 0x28 */ u8 unk28[0x3]; // colors, written by BattleSetSpriteColorFromAngle
    /* 0x2B */ u8 unk2B;
} BattleEffectSprite; // Size: 0x2C

// The color-ramp task's own data block, reached through the task entry's +0x4.
typedef struct {
    /* 0x00 */ u8 unk0[0x38];
    /* 0x38 */ BattleEffectSprite *pSprite;
    /* 0x3C */ s32 unk3C;
    /* 0x40 */ s32 unk40;
    /* 0x44 */ s32 unk44;
    /* 0x48 */ u8 unk48[0x6];
    /* 0x4E */ u16 unk4E;
    /* 0x50 */ u8 unk50[0x4];
    /* 0x54 */ s32 unk54;
    /* 0x58 */ s32 unk58; // ramp angle, advanced once per step
    /* 0x5C */ void *unk5C;
} BattleEffectColorRampData; // Size: 0x60

typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ BattleEffectColorRampData *pData;
} BattleEffectTask; // Size: 0x8

void BattleApplyEffectSpriteColorRamp(BattleEffectTask *pTask) {
    BattleEffectColorRampData *pData;
    BattleEffectSprite *pSprite;
    s32 timer;
    s16 unk6;
    s16 unkA;

    pData = pTask->pData;
    pSprite = pData->pSprite;
    timer = pData->unk58 + 1;
    pData->unk3C = pSprite->unk2;
    unk6 = pSprite->unk6;
    pData->unk40 = unk6; // retail stores unk6, then overwrites it below
    unkA = pSprite->unkA;
    pData->unk44 = unkA;
    pData->unk58 = timer;
    // kept after the timer store: retail loads 0x54 in that slot
    pData->unk40 = unk6 - pData->unk54 - 0x20;
    BattleSetSpriteColorFromAngle(pSprite, timer, unkA);
    pData->unk4E += 0x10;
}

void BattleFreeEffectSpriteTask(BattleEffectTask *pTask) {
    BattleEffectColorRampData *pData;
    BattleEffectSprite *pSprite;

    pData = pTask->pData;
    pSprite = pData->pSprite;
    pSprite->unk2B |= 1;
    func_8001F6B0(pSprite);
    func_80025180(pData->unk5C);
    WorkListsRemoveEntriesForData(pTask);
    TimerWorkListRemoveTask(pTask);
    HeapFree(pData);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD098);
extern s32 g_BattleCommandRowIndex;
extern void *D_800C3E1C;
extern s32 D_800C3D38;
extern s32 g_BattleColorMode;

// The battle actor sprite func_800BD3AC spawns an effect for: this unit reads only
// its fixed-point position and bit 2 of its flags word. misc33 declares the whole
// record as BattleActorSprite.
typedef struct {
    /* 0x00 */ IVEC3 position; // 16.16, copied into the effect task
    /* 0x0C */ u8 unkC[0xA0];
    /* 0xAC */ u32 flags;
} Misc32ActorSprite; // Size: 0xB0

// One 0x18-byte sprite-list entry ResourceHelperGetSpriteList fills in (misc33
// declares the same record as BattleSpritePart).
typedef struct {
    /* 0x00 */ u8 unk0[0x18];
} Misc32SpritePart; // Size: 0x18

// The effect task WorkListsAddTasks allocates for func_800BD3AC (0x1B0 bytes): the
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
    /* 0x48 */ s32 posX;
    /* 0x4C */ s32 posY;
    /* 0x50 */ s32 posZ;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ s32 scaleX;
    /* 0x5C */ s32 scaleY;
    /* 0x60 */ s32 scaleZ;
    /* 0x64 */ s32 unk64;
    /* 0x68 */ u8 unk68[0x10];
    /* 0x78 */ Misc32ActorSprite *pSprite; // the actor the effect is drawn for
    /* 0x7C */ u8 unk7C; // nonzero: posX steps the other way
    /* 0x7D */ u8 unk7D[0x3];
    /* 0x80 */ u8 unk80[0x3]; // colors, faded by BattleStepEffectColorFade
    /* 0x83 */ u8 unk83;
    /* 0x84 */ u32 unk84;
    /* 0x88 */ s32 timer;
    /* 0x8C */ s32 partCount;
    /* 0x90 */ Misc32SpritePart parts[12];
} Misc32Task; // Size: 0x1B0

extern Misc32Task *g_BattleTaskUnlinkList;
extern void *g_BattleState;
extern u8 *g_BattleSpriteArchive;
extern s16 g_BattleDigitTextureIds[];
extern u8 D_800C4929;
extern Misc32Task *WorkListsAddTasks(s32 allocSize, void *pData, void *pTimerCallback,
                                     void *pWorkCallback, void *pOnFreeCallback);
extern s32 ResourceHelperGetSpriteList(u8 *pDir, u32 index, Misc32SpritePart *pOut, s16 x, s16 y);
extern s32 func_800BE6E8(s32 value, u8 *pBuf, s32 n, u8 started, s32 base);
extern void func_800BDE58(void);
extern void func_800BDA1C(u8 *pTask);
extern void BattleUnlinkAndFreeTask(void *pEntry);
extern void BattleStepEffectColorFade(Misc32Task *pTask);
extern void BattleStepEffectPosition(Misc32Task *pTask);
extern void TimerWorkListSetTaskCallback(Misc32Task *pTask, void (*pCallback)(Misc32Task *));

extern void func_800BD3AC(Misc32ActorSprite *pSprite, s32 value, s32 kind);
extern void func_800BD1FC(s32 slot);

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
/*
// The battle work block as the command-row code sees it: this unit only reaches one
// row's four 11-entry per-slot arrays.
typedef struct {
    u8 unk0[0x138];  // 0x000
    u16 unk138[11];  // 0x138 command values, one per slot
    u8 pad14E[0x2];  // 0x14E
    u8 unk150[11];   // 0x150 command kinds
    u8 pad15B[0x1];  // 0x15B
    u16 unk15C[11];  // 0x15C timers
    u8 pad172[0x2];  // 0x172
    u8 unk174[11];   // 0x174 sprite color modes
    u8 pad17F[0x1];  // 0x17F
} BattleCommandRowTable;  // Size: 0x180

void func_800BD1FC(s32 slot) {
    u8 *pBase;
    BattleCommandRowTable *pRow;
    void *pSprite;
    s32 kind;

    pBase = g_BattleStateWork;
    pRow = (BattleCommandRowTable *)(pBase + (g_BattleCommandRowIndex - 1) * 0x48);
    if (pRow->unk150[slot] == 0xFF) {
        return;
    }
    if (pRow->unk138[slot] == 0xFFFF) {
        return;
    }
    pSprite = *(void **)(pBase + slot * 4 + 0x8C8C);
    if (pSprite == NULL) {
        return;
    }
    kind = pRow->unk150[slot];
    D_800C3D38 = pRow->unk15C[slot];
    g_BattleColorMode = pRow->unk174[slot];
    func_800BD3AC(pSprite, pRow->unk138[slot], kind);
    pRow = (BattleCommandRowTable *)(pBase + (g_BattleCommandRowIndex - 1) * 0x48);
    pRow->unk138[slot] = 0xFFFF;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD1FC);

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
                func_800BD1FC(i);
            } else {
                func_800BD1FC(i);
            }
        }
        pSlot += 4;
        i++;
    }
}
// Spawns the sprite-list effect that draws <value> next to <pSprite>. <kind> selects
// the frame the effect is in: 4 is the spawn frame, before the number's digits are
// added to the sprite list; the other kinds pick the number's colour channels.
void func_800BD3AC(Misc32ActorSprite *pSprite, s32 value, s32 kind) {
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
    func_800BDE58();
    pCur = g_BattleTaskUnlinkList;
    D_800C4929 = 0;
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
    pTask->posX = pSprite->position.x;
    g_BattleTaskUnlinkList = pTask;
    pTask->posY = pSprite->position.y;
    pTask->posZ = pSprite->position.z;
    pTask->scaleX = 0x2000;
    pTask->scaleY = 0x2000;
    pTask->scaleZ = 0x2000;
    pTask->unk83 = 0x2D;
    pTask->rot.vx = 0;
    pTask->rot.vy = 0;
    pTask->rot.vz = 0;
    pTask->unk80[0] = 0x80;
    pTask->unk80[1] = 0x80;
    pTask->unk80[2] = 0x80;
    pTask->unk7C = (pSprite->flags >> 2) & 1;
    pTask->partCount = 0;
    if (kind != 4) {
        func_800BE6E8(value, digits, 5, 0, 0);
        tex = g_BattleDigitTextureIds[digits[0]];
    }
    switch (kind) {
    // retail emits the case bodies in this order, so the labels are listed in it
    case 4:
        pTask->timer = 0x40;
        pTask->partCount = ResourceHelperGetSpriteList(g_BattleSpriteArchive, 0x7C, &pTask->parts[0], -0x14, -0x10);
        pTask->unk83 = (pTask->unk83 & 0xFE) | 2;
        TimerWorkListSetTaskCallback(pTask, BattleStepEffectColorFade);
        break;
    case 5:
        pTask->unk80[0] = 0x80;
        pTask->unk80[1] = 0;
        pTask->unk80[2] = 0;
        pTask->unk83 = (pTask->unk83 & 0xFE);
        break;
    case 10:
        pTask->partCount += ResourceHelperGetSpriteList(g_BattleSpriteArchive, 0x7F,
                                                        &pTask->parts[pTask->partCount], tex, 0);
        break;
    case 11:
        pTask->partCount += ResourceHelperGetSpriteList(g_BattleSpriteArchive, 0x91,
                                                        &pTask->parts[pTask->partCount], tex, 0);
        pTask->unk80[2] = 0x80;
        pTask->unk80[0] = 0;
        pTask->unk80[1] = 0;
        pTask->unk83 = (pTask->unk83 & 0xFE);
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
        pTask->unk80[0] = 0x80;
        pTask->unk80[2] = 0x80;
        pTask->unk80[1] = 0;
        pTask->unk83 = (pTask->unk83 & 0xFE);
        break;
    case 2:
        pTask->unk80[0] = 0;
        pTask->unk80[1] = 0x80;
        pTask->unk80[2] = 0;
        pTask->unk83 = (pTask->unk83 & 0xFE);
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BD810);

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
extern void func_800BD810(u8 *pEntry, u8 *pData);

// 96.95% under this unit's cdk preset (89.20 on 2.7.2). Left: (1) retail keeps the
// g_BattleGeomMatrix base in $s0 and stores `sw $v0,0x1C($s0)`, reusing $s0 for CompMatrix;
// gcc folds the store into one address or goes through $at. (2) retail loads the
// three SVECTOR halves with `lh`, gcc with `lhu`. (3) retail's `i = 0` lands after
// the ReadGeomScreen block, gcc hoists it into the delay slot. The declarations
// above must stay LIVE for this body to build (they used to sit inside the block).
/*
void func_800BDA1C(u8 *pTask) {
    u8 *pData;
    MATRIX mtx;
    SVECTOR vec;
    s32 sxy[3];
    s32 i;
    u8 *pEntry;

    pData = *(u8 **)(pTask + 0x4);
    g_BattleGeomMatrix.geomScreen = ReadGeomScreen();
    vec.vx = *(s16 *)(pData + 0x4A);
    vec.vy = *(s16 *)(pData + 0x4E);
    vec.vz = *(s16 *)(pData + 0x52);
    BattleProjectPointToScreen(&vec, sxy);
    RotMatrix((SVECTOR *)(pData + 0x40), &mtx);
    TransMatrix(&mtx, sxy);
    CompMatrix((MATRIX *)&g_BattleGeomMatrix, &mtx, &mtx);
    ScaleMatrix(&mtx, (VECTOR *)(pData + 0x58));
    SetRotMatrix(&mtx);
    SetTransMatrix(&mtx);
    i = 0;
    pEntry = pData + 0x90;
    while (i != *(s32 *)(pData + 0x8C)) {
        func_800BD810(pEntry, *(u8 **)(pData + 0x80));
        i++;
        pEntry += 0x18;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDA1C);
// Declared after the struct: the timer update takes the task. It reads none of
// the argument's fields, it works off g_BattleTimerTask.
extern void func_800BDF1C(Misc32Task *pTask);

void BattleStepEffectColorFade(Misc32Task *pTask) {
    WorkListCallback_t pCallback;

    func_800BDF1C(pTask);
    pTask->unk80[0] -= 2;
    pTask->unk80[1] -= 2;
    pTask->unk80[2] -= 2;
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
    func_800BDF1C(pTask);
    // the volatile re-reads are required: without them cse forwards the previous
    // call's stored result into the next call's argument
    pEffect->unk80[0] = SpriteColorAddClamp(pEffect->unk80[0], -8);
    pEffect->unk80[1] = SpriteColorAddClamp(*(volatile u8 *)&pTask->unk80[0], -8);
    pEffect->unk80[2] = SpriteColorAddClamp(*(volatile u8 *)&pEffect->unk80[0], -8);
    if (--pTask->timer < 0 || (pTask->unk80[0] | pTask->unk80[1] | pEffect->unk80[2]) == 0) {
        pCallback = pTask->timerEntry.onFreeCallback;
        pCallback(pEffect);
    }
}

extern void TimerWorkListSetTaskCallback(Misc32Task *pTask, void (*pCallback)(Misc32Task *));
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
// Round 116: moving the unk83 STORE after the call (compute the byte into a `u8
// flags` local before it, `pTask->unk83 = flags;` after it) DOES defeat
// combine_regs - the parameter then stays in a saved register with a real
// `move a0,sN` argument copy - but the flag byte then lives across the call and
// needs a SECOND saved register (0x20 frame, 108 bytes, 70.36%). Wrapping only the
// timer store in do{...}while(0), the lever that closed BattleStepEffectPosition
// next door, reproduces that function's shape instead (store straight after the
// bgez, so reorg steals the load-delay nop: 96 bytes, 68.96%). A 4-min permuter run
// from the 91.60 body: base=70 best=70, 1645 iterations, nothing. Retail's object
// has the same instruction multiset as the 91.60 body; the only differences are
// $a0 vs $s0 on the sw/sb and the position of the move, i.e. a local-alloc choice.
/*
void func_800BDC14(Misc32Task *pTask) {
    s32 nextTimer;

    func_800BDF1C(pTask);
    nextTimer = 0x10;
    if (--pTask->timer < 0) {
        pTask->timer = nextTimer;
        pTask->unk83 = (pTask->unk83 | 2) & 0xFE;
        TimerWorkListSetTaskCallback(pTask, BattleStepEffectColorFadeFromBase);
    }
}*/
// Round 165: re-measured at 91.60% / 100 bytes (retail's size, same instruction
// multiset), body compiles cleanly against today's externs, so the diagnosis above
// stands: this is the local-alloc tie it says it is.
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDC14);

extern void func_800BDC14(Misc32Task *pTask);

void BattleStepEffectPosition(Misc32Task *pTask) {
    func_800BDF1C(pTask);
    if (pTask->unk7C != 0) {
        pTask->posX += 0xC0000;
    } else {
        pTask->posX -= 0xC0000;
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDD3C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDE58);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc32", func_800BDF1C);
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
