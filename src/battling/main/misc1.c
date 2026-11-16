#include "common.h"
#include "psyq/libgpu.h"

extern u8 g_BattleTutorialWindow[];

extern void BattleSetScreenMode(int mode);
extern void SystemStringFree(void* pString);

void BattleFreeMessageString(void) {
    BattleSetScreenMode(3);
    SystemStringFree(&g_BattleTutorialWindow[0]);
}

s32 BattleRoundDivAwayFromZero(s32 a0, s32 a1, s32 a2) {
    s32 d;

    d = a0 - a1;
    if (d < 0) {
        d++;
        d -= a2;
    } else {
        d--;
        d += a2;
    }
    return d / a2;
}

extern s32 g_BattleCameraTarget;
extern s32 D_80098680;
extern s32 D_80098684;
extern s32 D_800925F4;
extern s32 g_BattleScriptCursor;
extern s32 g_BattleScriptWait;
extern s32 D_80097058;
extern s32 D_800970E0;
extern s32 D_80098774;
extern s32 D_800987FC;

/* 66.68%: original keeps &g_BattleCameraTarget in $s0 across the first call and reads and
   stores through it; ours rematerialises the address (lui at / lw / sw). The same
   C without the pointer local scores the same; -G0 65.72, gcc 2.6.0 65.72.
   func_80070808's body shows g_BattleCameraTarget/80/84 are the vx/vy/vz of one VECTOR:
   it reads 0x0(a0) with 0x0(s0) and stores the result back through 0x0(s0).
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80070808);

/* 66.68% attempt:
void func_80070808(s32* p, s32 step) {
    s32* pX = &g_BattleCameraTarget;

    *pX += BattleRoundDivAwayFromZero(p[0], *pX, step);
    D_80098684 += BattleRoundDivAwayFromZero(p[2], D_80098684, step);
    D_80098680 += BattleRoundDivAwayFromZero(p[1], D_80098680 + D_800925F4, step);
}
*/

extern VECTOR g_BattleCameraPos;
extern s32 D_80098720;
extern s32 D_80098724;
extern void BattleMovePointInsideRadius(s32* pA, s32* pB, s32 limit);

/* 74.00%: same missing shape as func_80070808, the original keeps &g_BattleCameraPos in
   $s2 across all four calls and reaches it through the register; ours re-forms it
   with lui/addiu each time. g_BattleCameraPos/20/24 are also one VECTOR seen by
   func_800708C4 as 0x0/0x4/0x8(s2).
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_800708C4);

/* 74.00% attempt:
void func_800708C4(s32* p, s32 step) {
    s32 values[3];

    values[0] = BattleRoundDivAwayFromZero(p[0], g_BattleCameraPos, step);
    values[1] = BattleRoundDivAwayFromZero(p[1], D_80098720, step);
    values[2] = BattleRoundDivAwayFromZero(p[2], D_80098724, step);
    BattleMovePointInsideRadius(&g_BattleCameraPos, values, 0x3D00);
    g_BattleCameraPos += values[0];
    D_80098720 += values[1];
    D_80098724 += values[2];
}
*/

extern VECTOR g_BattleActor0;
extern VECTOR D_80099078;
extern s32 g_BattleSceneOriginY;
extern s32 D_80097018;
extern s32 D_80098780;
extern s32 D_80097064;
extern s32 g_BattleSceneOriginYMirror;
extern s32 D_80098734;

// One past the last of the three effect-cell frame counters.
extern s32 D_80092A30;
extern void func_80070808(VECTOR* pVec, s32 step);
extern void func_800708C4(VECTOR* pVec, s32 step);
extern void func_80083738(VECTOR* pA, VECTOR* pB);
extern s32 func_80082488(VECTOR* pVec, s32 mode);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_8007099C);

/* 92.82%: same size as retail and every statement/call/constant is right; the
   gaps are allocation and position. Remaining diffs, all in the case 3/4 tail:
   (a) retail forms &vec (s0) after the D_80099078 copy, ours hoists the
   addiu s0 above it; (b) the func_80082488 delta keeps retail's -0x40 in $v0
   (the call result) while ours copies it to $a0 first; (c) case 5 keeps
   &g_BattleCameraTarget in $s0 across the rcos call (sw v0,0(s0), then lw v0,0(s0)) where
   ours rematerialises the symbol as a direct %lo store. A pointer local does
   not stop that. Do not re-derive the shape: the do-while around the
   D_800925F4 store is load-bearing (91.24 -> 92.82, and it restores retail's
   byte count), and the y local is what stops gcc folding
   (g_BattleSceneOriginYMirror - 0x20) - D_800925F4 into g_BattleSceneOriginYMirror - (D_800925F4 + 0x20).
*/
/* attempt:
void func_8007099C(s32 mode) {
    VECTOR vec;
    s32 scale;
    s32 y;
    s32 offset;
    s32* pBase;

    switch (mode) {
    case 0:
        func_80083738(&g_BattleActor1, &g_BattleActor0);
        break;
    case 1:
        vec = g_BattleActor1;
        goto place_message;
    case 2:
        vec = g_BattleActor0;
    place_message:
        vec.vy -= 0xA0;
        D_800925F4 = 0;
        func_80070808(&vec, 4);
        break;
    case 3:
        scale = 0xA0;
        goto rotate_message;
    case 4:
        scale = 0x80;
    rotate_message:
        // the block keeps the scale store in front of the vector copy
        do {
            D_800925F4 = scale;
        } while (0);
        vec = D_80099078;
        vec.vy += D_800925F4;
        func_80070808(&vec, 0x10);
        vec.vx = g_BattleActor1.vx + ((rcos(D_80098780 + 0xA80) * 0xD0) >> 12);
        y = g_BattleSceneOriginYMirror - 0x20;
        vec.vy = y - D_800925F4;
        vec.vz = D_80098734 + ((rsin(D_80098780 + 0xA80) * 0xD0) >> 12);
        func_800708C4(&vec, 0x46);
        offset = func_80082488(&g_BattleCameraPos, 0) - 0x40;
        offset -= D_800925F4;
        if (offset < D_80098720) {
            D_80098720 = offset;
        }
        break;
    case 5:
        pBase = &g_BattleCameraTarget;
        *pBase = g_BattleActor0.vx;
        D_80098680 = g_BattleSceneOriginY - 0xC0;
        D_80098684 = D_80097018;
        g_BattleCameraPos = *pBase + ((rcos(D_80097064 + 0x900) * 0xE0) >> 12);
        D_80098720 = g_BattleSceneOriginY - 0xD0;
        D_80098724 = D_80098684 + ((rsin(D_80097064 + 0x900) * 0xE0) >> 12);
        break;
    }
}
*/

// The position every effect vector is placed against: three 16.16 words.
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
} BattleAnchor; // Size: 0xC

// The battling overlay's battle state (g_BattleActor1): the anchor position every
// effect vector is placed against, the model index and gauge words at +0x4C..+0xB6
// (the same record src/battling/main/misc3 and misc4 name), and the frame flags
// the effect cells below are switched on through.
typedef struct {
    /* 0x000 */ BattleAnchor position;
    /* 0x00C */ u8 unkC[0x40];
    /* 0x04C */ u8 modelIndex;
    /* 0x04D */ u8 unk4D[0x2];
    /* 0x04F */ u8 unk4F;
    /* 0x050 */ u8 unk50[0x2];
    /* 0x052 */ u8 unk52;
    /* 0x053 */ u8 unk53[0x61];
    /* 0x0B4 */ s16 gaugeValue;
    /* 0x0B6 */ s16 gaugePoints;
    /* 0x0B8 */ u8 unkB8[0x18];
    /* 0x0D0 */ u32 flags;
} BattleState; // Size: 0xD4

extern BattleState g_BattleActor1;

extern void BattleClearSpriteTables(void);

// Places the battle state's anchor, the actor at g_BattleActor0 and the camera
// around the midpoint of the first two, offset by the amount the mode picks.
void BattleRecenterViewPositions(s32 mode) {
    VECTOR anchor;
    VECTOR actor;
    VECTOR camera;
    VECTOR offset;

    anchor = *(VECTOR*)&g_BattleActor1;
    actor = g_BattleActor0;
    camera = g_BattleCameraPos;
    offset = anchor;
    offset.vx += actor.vx;
    offset.vy += actor.vy;
    offset.vz += actor.vz;
    offset.vx /= 2;
    offset.vy /= 2;
    offset.vz /= 2;
    anchor.vx -= offset.vx;
    anchor.vy -= offset.vy;
    anchor.vz -= offset.vz;
    actor.vx -= offset.vx;
    actor.vy -= offset.vy;
    actor.vz -= offset.vz;
    camera.vx -= offset.vx;
    camera.vy -= offset.vy;
    camera.vz -= offset.vz;
    switch (mode) {
    case 0:
        offset.vx = 0x4000;
        offset.vy = 0;
        offset.vz = 0x4000;
        break;
    case 1:
        offset.vx = 0x6000;
        offset.vy = 0;
        offset.vz = 0x6000;
        break;
    case 2:
        offset.vx = 0x2000;
        offset.vy = 0;
        offset.vz = 0x2000;
        break;
    case 3:
        offset.vx = 0x4000;
        offset.vy = 0;
        offset.vz = 0x2400;
        break;
    }
    anchor.vx += offset.vx;
    anchor.vy += offset.vy;
    anchor.vz += offset.vz;
    actor.vx += offset.vx;
    actor.vy += offset.vy;
    actor.vz += offset.vz;
    camera.vx += offset.vx;
    camera.vy += offset.vy;
    camera.vz += offset.vz;
    anchor.vy = 0;
    actor.vy = 0;
    camera.vy = -0x300;
    *(VECTOR*)&g_BattleActor1 = anchor;
    g_BattleActor0 = actor;
    g_BattleCameraPos = camera;
    BattleClearSpriteTables();
}

void BattleSetValueAndClearFlags(s32 value) {
    g_BattleScriptCursor = value;
    D_80098774 = 0;
    D_80097058 = 0;
    g_BattleScriptWait = 0;
    D_800987FC &= 0xFFFF7FFF;
    D_800970E0 &= 0xFFFF7FFF;
}

extern s32 g_BattleFacingAngle;

typedef struct BattleScriptBlock BattleScriptBlock;

// Partial view of the battle actor record these functions receive; only the
// fields src/battling/main/misc1 reads are named. The record is far larger
// than this (it holds the actor's script pointer at +0x8FC).
typedef struct {
    /* 0x000 */ VECTOR position;
    /* 0x010 */ u8 unk10[0x24];
    /* 0x034 */ VECTOR unk34;
    /* 0x044 */ u8 unk44[0x4];
    /* 0x048 */ u32 distance;
    /* 0x04C */ u8 unk4C[0xC];
    /* 0x058 */ s32 targetAngle;
    /* 0x05C */ s32 unk5C;
    /* 0x060 */ u8 unk60[0x6E];
    /* 0x0CE */ s16 unkCE;
    /* 0x0D0 */ u8 unkD0[0x82C];
    /* 0x8FC */ BattleScriptBlock* pScript;
} BattleActorView; // Size: unknown

s32 BattleSetFacingAngleFromVector(BattleActorView* p) {
    VECTOR vec;
    s32 angle;

    vec = p->position;
    vec.vx -= 0x3F80;
    vec.vz -= 0x3F80;
    angle = ratan2(vec.vx, vec.vz) & 0xFFF;
    if (angle >= 0x201) {
        p->targetAngle = 0x800 - g_BattleFacingAngle;
    } else {
        p->targetAngle = 0xC00 - g_BattleFacingAngle;
    }
    p->distance = 0xFF;
    p->unkCE = 0;
    return 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_8007107C);

extern s32 g_BattleWindowPosX;
extern s32 g_BattleWindowPosY;
extern u8 g_BattleDrawBufferIndex;
extern DR_TPAGE g_BattleTpagePrims[];

// The packet BattleQueueSpriteAndTpagePrims fills and queues: its primitive tag
// sits at +0x74, the word the battle stores alongside it at +0x7C.
typedef struct {
    /* 0x00 */ u8 unk0[0x74];
    /* 0x74 */ u_long tag;
    /* 0x78 */ u8 unk78[0x4];
    /* 0x7C */ s32 xy;
} BattleSpritePacket; // Size: unknown

extern BattleSpritePacket* g_BattleDrawSlot;

void BattleQueueSpriteAndTpagePrims(void* pOt) {
    BattleSpritePacket* pPacket;

    pPacket = g_BattleDrawSlot;
    pPacket->xy = g_BattleWindowPosX | (g_BattleWindowPosY << 16);
    AddPrim(pOt, &pPacket->tag);
    AddPrim(pOt, &g_BattleTpagePrims[g_BattleDrawBufferIndex]);
}

extern u32 D_800929EC;
extern u32 D_800929F0;
extern u8 D_8009A158;
extern u8 D_8009A159;
extern u16 D_8009A15A;

/* 0x10-byte texture primitive template: tag, colour/code, xy, then UV and CLUT */
typedef struct {
    /* 0x0 */ u_long tag;
    /* 0x4 */ u_char r0, g0, b0, code;
    /* 0x8 */ s16 x0, y0;
    /* 0xC */ u_char u0, v0;
    /* 0xE */ u16 clut;
} BattleTexBlock; // Size: 0x10

extern BattleTexBlock D_8009A14C;
extern BattleTexBlock D_8009A244;

/* 79.36%: every statement is right and the size is retail's 300 bytes, but
   gcc re-forms the second load's base (lui v1,%hi(g_BattleTpagePrims); lw v1,4(v1)) where
   retail reuses the $s0 already holding the address, hoists the GetClut argument
   loads (lh a0/lh a1) above the D_800929EC stores, and sinks the `sh v0,
   D_8009A15A` of GetClut's result below the 16-byte struct copy. Tried: two u32
   loads hoisted into locals, (u32*)pTPage[i] vs pTPage->tag/->code[0] (both
   rematerialise), a DR_TPAGE struct copy for the pair (fixes $s0 but makes the
   second store a %lo(D_800929EC+4) reloc), copy via pointer local, and
   (u_short*)tim.prect instead of RECT fields (gcc still narrows to lbu where
   retail has lhu, so the andi 0x3F follows a halfword load). The unused RECT
   local is load-bearing: it reproduces retail's 0x38 frame.
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80071794);

/* 79.36% attempt:
void func_80071794(u8* p) {
    TIM_IMAGE tim;
    u_short tpage;
    s16* pClut;
    DR_TPAGE* pTPage;
    BattleTexBlock* pBlock;
    RECT rect;

    OpenTIM(*(u_long**)(p + 0x60));
    ReadTIM(&tim);
    pClut = (s16*)tim.caddr;
    pClut[0] = 0;
    pClut[2] = -0x8000;
    pClut[3] = -1;
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    tpage = GetTPage(0, 1, tim.prect->x, tim.prect->y);
    pTPage = (DR_TPAGE*)&g_BattleTpagePrims[0];
    SetDrawTPage(pTPage, 0, 0, tpage);
    D_800929EC = pTPage->tag;
    D_800929F0 = pTPage->code[0];
    D_8009A158 = (((u_short*)tim.prect)[0] & 0x3F) << 2;
    D_8009A159 = ((u_short*)tim.prect)[1];
    D_8009A15A = GetClut(tim.crect->x, tim.crect->y);
    pBlock = &D_8009A244;
    *pBlock = D_8009A14C;
}
*/

extern s32 g_BattleMessageIndex;
extern s32 g_BattleMessageShownIndex;

// The battle message window the tutorial text is drawn into: the box rectangle
// func_80032F54 fills in, the line count at +0xC and the redraw flag at +0x68.
typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 x;
    /* 0x06 */ s16 y;
    /* 0x08 */ s16 unk8;
    /* 0x0A */ s16 flags;
    /* 0x0C */ s16 lineCount;
    /* 0x0E */ u8 unkE[0x5A];
    /* 0x68 */ u8 redraw;
} BattleMessageWindow; // Size: 0x69

extern BattleMessageWindow g_BattleMessageWindow;
extern s32 func_80032F54(void* pString, s16 a, s16 b, s16 c, s16 d, s16 e, s16 f);

void BattleInitMessageWindow(void) {
    g_BattleMessageShownIndex = -1;
    g_BattleMessageIndex = 0;
    func_80032F54(&g_BattleMessageWindow, 0x140, 0x30, 0x1C, 0x9A, 0x40, 4);
}

extern u8 D_80092608;
extern s16 D_80098698;
extern s16 D_80098692;
extern s32 D_8009105C[];
extern s32 D_800925E8;
extern s32 g_BattleWindowPosX;
extern s32 D_800925EC;
extern s32 g_BattleWindowPosY;
extern s16 D_80092600;
extern s16 D_800987E0;
extern u16 D_800987E8;
extern s16 D_800970C4;
extern u16 D_800970CC;
extern void BattlePlaySoundEffectById(s32 id);

/* 89.15%: reorg steals the else branch's li v0,0x4 into the bnez delay slot
   where the original leaves a nop, which makes our value pseudo land in v1. */
INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_8007191C);

/* 89.15% attempt:
void func_8007191C(u32 idx) {
    s32 value;

    D_80092608 = idx < 1;
    if (idx == 0) {
        BattlePlaySoundEffectById(0x37);
        D_80098698 = 2;
        value = 0xB4;
    } else {
        D_80098698 = 4;
        value = 0x9A;
    }
    D_80098692 = value;
    BattleSetValueAndClearFlags(D_8009105C[idx]);
    D_800925E8 = 0xA0;
    g_BattleWindowPosX = 0xA0;
    D_800925EC = 0x6D;
    g_BattleWindowPosY = 0x6D;
    D_80092600 = 0;
    D_800987E0 = D_800987E8;
    D_800970C4 = D_800970CC;
}
*/

extern u8 g_BattlePlayer1ComControlled;
extern u8 g_BattlePlayer2ComControlled;
extern s32 g_BattleMode;
extern u8 g_BattleOptionOn;
extern u8* g_BattleMessageStringTable;
extern u8 D_800929BC;
extern s32 D_800925DC;
extern u8 g_BattleTutorialLineOffset;
extern s32 g_BattleEffectPlaceMode;
extern s32 g_BattleStateStep;
extern u8 g_BattleTutorialScript[];
extern u8* GetStringEntry(u8* pData, s32 index);
extern void SystemStringPushBuffer(void* pString, u8* pEntry);

void BattleInitTutorialMessage(void) {
    g_BattlePlayer1ComControlled = 0;
    g_BattlePlayer2ComControlled = 0;
    BattleSetScreenMode(7);
    g_BattleMode = 5;
    g_BattleOptionOn = 0;
    func_80032F54(&g_BattleTutorialWindow[0], 0x140, 0x70, 0xA2, 0x2A, 0x1C, 8);
    SystemStringPushBuffer(&g_BattleTutorialWindow[0], GetStringEntry(g_BattleMessageStringTable, 0x42));
    D_800929BC = 0x1E;
    D_800925DC = 0;
    BattleSetValueAndClearFlags(&g_BattleTutorialScript[0]);
    g_BattleTutorialLineOffset = 0;
    g_BattleEffectPlaceMode = 0;
    g_BattleStateStep = 0;
    func_8007191C(9);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80071AD0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80071DA4);

extern s32 D_8009284C;
extern s32 D_80092948;
extern void SoundStopAudioManager(s32 arg);
extern void SoundStopAllElements(void);
extern void func_80071DA4(u8* p);

/* 91.77%: case 3 is the only difference. retail materialises &D_80098720 into
   $v1 and stores through it twice (lui v1/%hi; addiu v1/%lo; sw v0,0(v1));
   ours keeps both stores as direct absolute references, which maspsx expands to
   lui $at/%hi; sw %lo($at), and drops the load for the compare (the value is
   already in $v0, as in retail). Tried: a pointer local, pTPage-style field
   access, a s32 value local, and ((s32*)&g_BattleCameraPos)[1] so the address is a
   symbol plus a constant offset - gcc folds every one of them back into a direct
   symbol store.
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80071F8C);

/* 91.77% attempt:
void func_80071F8C(s32 mode) {
    switch (mode) {
    case 0:
        SoundStopAudioManager(D_80092948);
        SoundStopAllElements();
        break;
    case 1:
        func_80071DA4((u8*)&g_BattleActor1);
        if (D_800970C4 * 0x100 / D_800970CC < D_800987E0 * 0x100 / D_800987E8) {
            g_BattleMessageIndex = 0x43;
        } else {
            g_BattleMessageIndex = 0x44;
        }
        break;
    case 2:
        func_80071DA4((u8*)&g_BattleActor0);
        break;
    case 3:
        D_80098720 = -D_8009284C;
        if (D_80098720 < -0x600) {
            D_80098720 = -0x600;
        }
        break;
    }
}
*/


extern u8 g_BattleInitDone;

void BattleClearInitFlag(void) {
    g_BattleInitDone = 0;
}

extern u8 g_BattleInitScript;
extern u8 g_BattleInputEnabled;
extern u8 g_BattlePlayer1ComControlled;
extern u8 g_BattlePlayer2ComControlled;
extern s32 g_BattleStateStep;
extern s32 D_80092A00;
extern s32 D_80092A10;
extern s32 D_80092A20;
extern s32 D_800970F8;
extern s32 D_80098814;

void BattleInitOnce(void) {
    if (g_BattleInitDone != 0) {
        return;
    }
    BattleSetValueAndClearFlags((s32)&g_BattleInitScript);
    g_BattleInitDone = 1;
    g_BattleInputEnabled = 0;
    g_BattlePlayer1ComControlled = 0;
    g_BattlePlayer2ComControlled = 0;
    g_BattleStateStep = 0;
    g_BattleMessageIndex = 0;
    g_BattleMessageShownIndex = 0;
    D_80092A00 = 1;
    D_80092A10 = 1;
    D_80092A20 = 1;
    D_800970F8 = 0;
    D_80098814 = 0;
}

// The anchor's y and z words: retail's object reaches them as their own
// symbols, not as displacements off g_BattleActor1.
extern s32 g_BattleSceneOriginYMirror;
extern s32 D_80098734;

// One past the last of the three effect-cell frame counters.
extern s32 D_80092A30;

// One of the effect cells func_80072170 steps: the 16.16 origin a burst is
// placed at (a source actor's origin plus the battle state's anchor) and the
// frames left before the next burst.
typedef struct {
    /* 0x00 */ s32 x;
    /* 0x04 */ s32 y;
    /* 0x08 */ s32 z;
    /* 0x0C */ s32 frames;
} BattleEffectEmitter; // Size: 0x10

extern BattleEffectEmitter g_BattleEffectCells[];

typedef struct {
    /* 0x00 */ u8 unk0[0x60];
    /* 0x60 */ s32 x;
    /* 0x64 */ s32 y;
    /* 0x68 */ s32 z;
} BattleEffectOrigin; // Size: 0x6C

// The origins a burst is jittered around: the number of entries and the array
// of per-sprite origins, reached through the pool g_BattleEffectPool points at.
typedef struct {
    /* 0x0 */ s16 count;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ BattleEffectOrigin** pOrigins;
} BattleEffectPool; // Size: 0x8

typedef struct {
    /* 0x0 */ u8 unk0[0x4];
    /* 0x4 */ BattleEffectPool* pPool;
} BattleEffectPoolRef; // Size: 0x8

extern BattleEffectPoolRef* g_BattleEffectPool;

extern s32 g_BattleStateStep;
extern s32 g_BattleEffectPlaceMode;
extern s32 D_80092938;
extern u16 D_8009260C;
extern u8 g_BattleResultCode;
extern u8* g_BattleMessageStringTable;
extern void BattleEffectAddSlotFromVector(VECTOR* pVec, s32 action);
extern void BattlePlaySoundEffectById(s32 id);
extern void SystemStringFreeBufferStack(void* pString);
extern u8* GetStringEntry(u8* pData, s32 index);
extern void SystemStringPushBuffer(void* pString, u8* pEntry);
extern void func_8008E2B8(u8* p0, s32 color, s32 flag);
extern void BattleClearScreen(s32 arg);
extern void BattleReturnToMainLoop(void);
extern void func_8007107C(void);
extern void func_8007099C(s32 mode);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80072170);

/* 98.87% (1088 bytes vs retail's 1080): every statement, call, constant, case
   order (11, 1, 2, 3, 4), jump-table entry and the shared fade: block reached
   from case 3 (fallthrough) and case 4 (goto) is right; what is left is the
   case-1 loop only. Retail hoists the effect-cell base out of the loop
   ('lui s4,%hi(g_BattleEffectCells); addiu s4') and passes the element address to
   BattleEffectAddSlotFromVector as 'addu a0,s0,s4' - base register plus the
   i*0x10 byte offset that the x/y/z stores already need - which is why it keeps
   SIX saved registers (frame 0x40). Every body tried here either re-forms that
   base inside the loop (lui/addiu/addu, +2 insns) or lets loop.c turn the
   address into its own induction pointer (separate increment in the delay
   slot). Things that are settled and must not be re-derived: the loop bound is
   a SIGNED compare ('slt', the bound cast); D_8009260C is read as a signed
   halfword and both func_8008E2B8 call sites need the (s16) cast or they load
   lhu; g_BattleSceneOriginYMirror/D_80098734 are reached as their own symbols in the fade block
   while case 1 reads the same words off g_BattleActor1; the pCounter copy of pFrames
   and the pVec local in front of the add-slot call are what took the previous
   park from 97.21 to 98.87 (both found by the permuter). The 97.21% park WAS
   retail's exact 1080 bytes (five saved registers, the call address
   materialised with lui/addiu/addu inside the loop) - if the base hoist cannot
   be expressed, that shape at least links.
   A permuter run from this body found nothing better.
   attempt:
// Steps the battle display state machine for the current frame: plays the
// effect sound, places a burst from a random source actor on the effect cells,
// fades the screen and keeps the tutorial message text in step.
void func_80072170(void) {
    BattleState* pState;
    BattleMessageWindow* pMsg;
    BattleAnchor* pAnchor;
    VECTOR* pVec;
    BattleEffectEmitter* pEmitter;
    BattleEffectOrigin* pOrigin;
    s32* pFrames;
    s32* pCounter;
    long vec[3];
    s32 i;
    s32 frames;
    s32 count;

    if (g_BattleInitDone == 0) {
        return;
    }
    pState = &g_BattleActor1;
    pMsg = &g_BattleMessageWindow;
    switch (g_BattleStateStep) {
    case 11:
        BattlePlaySoundEffectById(0x2A);
        BattlePlaySoundEffectById(0x2B);
        BattlePlaySoundEffectById(0x2C);
        g_BattleStateStep = 1;
    case 1:
        pAnchor = &g_BattleActor1.position;
        pEmitter = g_BattleEffectCells;
        pFrames = &g_BattleEffectCells[0].frames;
        i = 0;
        do {
            pCounter = pFrames;
            frames = *pCounter - 1;
            pEmitter->frames = frames;
            if (frames == -1) {
                pOrigin = g_BattleEffectPool->pPool->pOrigins[rand() % g_BattleEffectPool->pPool->count];
                g_BattleEffectCells[i].x = pOrigin->x + pAnchor->x;
                g_BattleEffectCells[i].y = pOrigin->y + pAnchor->y;
                g_BattleEffectCells[i].z = pOrigin->z + pAnchor->z;
                *pCounter = rand() % 16 + 8;
            }
            pVec = (VECTOR*)&g_BattleEffectCells[i];
            if (*pCounter & 1) {
                BattleEffectAddSlotFromVector(pVec, 9);
            }
            pFrames += 4;
            i++;
            pEmitter++;
        } while ((s32)pFrames < (s32)&D_80092A30);
        break;
    case 2:
        pState->gaugeValue = 1;
        D_8009260C = 0xFF;
        g_BattleStateStep++;
    case 3:
        func_8008E2B8((u8*)D_80092938, (s16)D_8009260C, 0);
        if ((s16)D_8009260C < 0) {
            D_8009260C = 0;
        }
        D_8009260C -= 8;
fade:
        pState->unk4F = 0x10;
        pState->modelIndex = 0xA;
        pState->unk52 = 0;
        pState->gaugePoints = 0x1000;
        pState->flags |= 0x400;
        if (rand() & 1) {
            count = g_BattleEffectPool->pPool->count;
            pOrigin = g_BattleEffectPool->pPool->pOrigins[rand() % count];
            vec[0] = pOrigin->x + g_BattleActor1.position.x;
            vec[1] = pOrigin->y + g_BattleSceneOriginYMirror;
            vec[2] = pOrigin->z + D_80098734;
            BattleEffectAddSlotFromVector((VECTOR*)vec, 0xB);
            BattleEffectAddSlotFromVector((VECTOR*)vec, 8);
        }
        break;
    case 4:
        D_8009260C += 3;
        if ((s16)D_8009260C >= 0x100) {
            D_8009260C = 0xFF;
            g_BattleResultCode = 0x7F;
            BattleClearScreen(0);
            BattleReturnToMainLoop();
        }
        func_8008E2B8((u8*)D_80092938, (s16)D_8009260C, 1);
        goto fade;
    }
    func_8007107C();
    if (g_BattleMessageIndex != g_BattleMessageShownIndex) {
        pMsg->redraw = 1;
        SystemStringFreeBufferStack(pMsg);
        SystemStringPushBuffer(pMsg, GetStringEntry(g_BattleMessageStringTable, g_BattleMessageIndex));
        pMsg->lineCount = 2;
        pMsg->y = 0xB4;
        g_BattleMessageShownIndex = g_BattleMessageIndex;
    }
    func_8007099C(g_BattleEffectPlaceMode);
    D_80098774 = 0;
}
*/




void BattleEmptyFunction(void) {
}

extern void* BattleCreateOrderingTablePair(s32 a0);
extern void* BattleCreateLightGroup(void* p);
extern void* BattleInitObjectTree(s32 a0);
extern void func_80078F00(void* p);
extern s16 g_BattleDebugFlags;
extern void* g_BattleRenderActor;
extern s32 D_80096FB0;
extern s32 D_80096FAC;
extern s32 g_BattleViewOrigin;
extern u8 g_BattleRenderToggleCount;
extern s32 g_BattleRenderAngleX;
extern s32 g_BattleRenderAngleYStep;
extern s32 g_BattleRenderOffsetY;
extern s32 g_BattleRenderOffsetZ;
extern s32 D_8009262C;
extern s16 D_80092632;
extern void* g_BattleLightGroup;
extern void* g_BattleNodeTree;

// The block the actor's script pointer (+0x8FC) addresses: the five signed
// halfwords and the unsigned one BattleInitRenderScene copies into the render
// state at g_BattleRenderToggleCount..D_80092632.
struct BattleScriptBlock {
    /* 0x00 */ u8 unk00[0x14];
    /* 0x14 */ s16 unk14;
    /* 0x16 */ s16 unk16;
    /* 0x18 */ s16 unk18;
    /* 0x1A */ s16 unk1A;
    /* 0x1C */ s16 unk1C;
    /* 0x1E */ u16 unk1E;
};

void BattleInitRenderScene(BattleActorView* p) {
    BattleScriptBlock* pScript;
    void* pAnim;
    s32 id;
    s32 x;
    s32 y;
    s32 z;
    s32 w;
    s32 u;
    u16 v;

    func_80030988(5, 4, 0x40, 0x40);
    pAnim = BattleCreateLightGroup(BattleCreateOrderingTablePair(0x10));
    id = p->unk5C;
    g_BattleLightGroup = pAnim;
    g_BattleNodeTree = BattleInitObjectTree(id);
    BattleSetScreenResolution(0x280, 0xDA);
    SetGeomScreen(0x400);
    g_BattleDebugFlags = 0;
    g_BattleRenderActor = p;
    func_80078F00(p);
    // retail reads the script pointer after this cluster, not in front of it:
    // the block stops cc1 hoisting the load over the three stores.
    do {
        D_80096FB0 = 0;
        D_80096FAC = 0;
        g_BattleViewOrigin = 0;
    } while (0);
    pScript = p->pScript;
    x = pScript->unk14;
    y = pScript->unk16;
    z = pScript->unk18;
    w = pScript->unk1A;
    u = pScript->unk1C;
    v = pScript->unk1E;
    g_BattleRenderToggleCount = 1;
    g_BattleRenderAngleX = x;
    g_BattleRenderAngleYStep = y;
    g_BattleRenderOffsetY = z;
    g_BattleRenderOffsetZ = w;
    D_8009262C = u;
    D_80092632 = v;
}

extern void func_8008BC04(void);
extern void BattleClearTextSpriteEntries(void);
extern void func_80030988(s32 a0, s32 a1, s32 a2, s32 a3);
extern void BattleFreeLightGroup(void* p);
extern void BattleFreeNodeTree(void* p);
extern void BattleSetScreenResolution(s32 a0, s32 a1);
extern void BattleSetScreenMode(s32 mode);
extern void BattleSetTextScale(s32 value);
extern void BattleResetTextPrimQueue(void);
extern void* g_BattleLightGroup;
extern void* g_BattleNodeTree;

void BattleFreeRenderScene(void) {
    func_8008BC04();
    BattleClearTextSpriteEntries();
    func_80030988(1, 1, 0x40, 0x40);
    BattleFreeLightGroup(g_BattleLightGroup);
    BattleFreeNodeTree(g_BattleNodeTree);
    BattleSetScreenResolution(0x140, 0xDA);
    SetGeomScreen(0xC0);
    BattleSetScreenMode(3);
    BattleSetTextScale(0x100);
    BattleResetTextPrimQueue();
}

extern MATRIX g_BattleBaseMatrix;

void BattleComposeCameraMatrix(BattleActorView* p, MATRIX* pIn, MATRIX* pOut) {
    MATRIX mtx;

    *pOut = *pIn;
    mtx = g_BattleBaseMatrix;
    mtx.t[0] = p->unk34.vx - g_BattleViewOrigin;
    mtx.t[1] = -D_80096FAC;
    mtx.t[2] = p->unk34.vz - D_80096FB0;
    CompMatrix(pIn, &mtx, &mtx);
    pOut->t[0] = mtx.t[0];
    pOut->t[1] = mtx.t[1];
    pOut->t[2] = mtx.t[2];
}

/* PARKED, not attempted: 304 instructions, and splat marks the object "Handwritten
   function". The tail fuses the GTE rotation and translation matrix loads
   (lw $t4/$t5/$t6 interleaved with ctc2 $0-$7 off g_BattleNodeTree+0xC) in a shape no
   macro in include/psyq/inline_c.h emits - gte_SetRotMatrix/gte_SetTransMatrix
   load through $8-$12 with a different interleave. The rest is a controller
   state machine (ControllerMergeStates, g_C1ButtonState/g_C2ButtonStateReleased,
   BattleFreeRenderScene, the Sprintf'd debug overlay) around a 0xC8 frame.
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80072858);
