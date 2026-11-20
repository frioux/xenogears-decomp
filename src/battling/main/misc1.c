#include "common.h"
#include "psyq/libgpu.h"
#include "system/controller.h"

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

// The position every effect vector is placed against: the three 16.16 words of
// a long-word VECTOR.
extern VECTOR g_BattleCameraTarget;
extern s32 g_BattleCameraTargetY;
extern s32 g_BattleCameraTargetZ;
extern s32 g_BattleEffectPlaceYOffset;
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
    g_BattleCameraTargetZ += BattleRoundDivAwayFromZero(p[2], g_BattleCameraTargetZ, step);
    g_BattleCameraTargetY += BattleRoundDivAwayFromZero(p[1], g_BattleCameraTargetY + g_BattleEffectPlaceYOffset, step);
}
*/

extern VECTOR g_BattleCameraPos;
extern s32 g_BattleCameraPosY;
extern s32 g_BattleCameraPosZ;
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
    values[1] = BattleRoundDivAwayFromZero(p[1], g_BattleCameraPosY, step);
    values[2] = BattleRoundDivAwayFromZero(p[2], g_BattleCameraPosZ, step);
    BattleMovePointInsideRadius(&g_BattleCameraPos, values, 0x3D00);
    g_BattleCameraPos += values[0];
    g_BattleCameraPosY += values[1];
    g_BattleCameraPosZ += values[2];
}
*/

extern VECTOR g_BattleActor0;
extern VECTOR g_BattleEffectPlaceOrigin;
extern s32 D_80098780;
extern s32 D_80097064;
extern s32 g_BattleSceneOriginYMirror;
extern s32 g_BattleSceneOriginZ;

// One past the last of the three effect-cell frame counters.
extern s32 g_BattleEffectCellsEnd;
extern void func_80070808(VECTOR* pVec, s32 step);
extern void func_800708C4(VECTOR* pVec, s32 step);
extern void BattleAimCameraAtCombatants(VECTOR* pA, VECTOR* pB);
extern s32 func_80082488(VECTOR* pVec, s32 mode);

/* 92.82%: same size as retail and every statement/call/constant is right; the
   gaps are allocation and position. Remaining diffs, all in the case 3/4 tail:
   (a) retail forms &vec (s0) after the g_BattleEffectPlaceOrigin copy, ours hoists the
   addiu s0 above it; (b) the func_80082488 delta keeps retail's -0x40 in $v0
   (the call result) while ours copies it to $a0 first; (c) case 5 keeps
   &g_BattleCameraTarget in $s0 across the rcos call (sw v0,0(s0), then lw v0,0(s0)) where
   ours rematerialises the symbol as a direct %lo store. A pointer local does
   not stop that. Do not re-derive the shape: the do-while around the
   g_BattleEffectPlaceYOffset store is load-bearing (91.24 -> 92.82, and it restores retail's
   byte count), and the y local is what stops gcc folding
   (g_BattleSceneOriginYMirror - 0x20) - g_BattleEffectPlaceYOffset into g_BattleSceneOriginYMirror - (g_BattleEffectPlaceYOffset + 0x20).
*/

// The battling overlay's battle state (g_BattleActor1): the anchor position every
// effect vector is placed against, the model index and gauge words at +0x4C..+0xB6
// (the same record src/battling/main/misc3 and misc4 name), and the frame flags
// the effect cells below are switched on through. Its position is a VECTOR:
// three sites copy 16 bytes out of it, and the g_BattleActor0 it is copied
// against is a VECTOR.
typedef struct {
    /* 0x000 */ VECTOR position;
    /* 0x010 */ u8 unk10[0x3C];
    /* 0x04C */ u8 modelIndex;
    /* 0x04D */ u8 unk4D[0x2];
    /* 0x04F */ u8 animSpeed;
    /* 0x050 */ u8 unk50[0x2];
    /* 0x052 */ u8 animSpeedBonus;
    /* 0x053 */ u8 unk53[0x5D];
    // The height func_80082488 reports for the actor's position, refreshed every
    // frame by the grid walk below and by BattleActorUpdateGroundMovement.
    /* 0x0B0 */ s32 floorY;
    /* 0x0B4 */ s16 gaugeValue;
    /* 0x0B6 */ s16 gaugePoints;
    /* 0x0B8 */ u8 unkB8[0x18];
    /* 0x0D0 */ u32 flags;
} BattleState; // Size: 0xD4

extern BattleState g_BattleActor1;

// Places the effect vector (a stack VECTOR) and the camera for the placement
// mode g_BattleEffectPlaceMode names: recentres the tutorial message, or snaps
// the camera onto the front actor.  objdiff 99.95%: the only difference left is
// the switch's table operand (jtbl_ vs .rodata), check_bin.sh battling is OK.
// Three spellings are load-bearing and look gratuitous:
//  - the do-while plus its identical if/else arms: the arms are the same store,
//    and the join is what puts &vec's addiu after the g_BattleEffectPlaceOrigin copy;
//  - the delta/offset copy keeps the func_80082488 result in $v0 for the -0x40;
//  - pTargetX is a pointer to the anchor's vx word: the plain field spelling
//    folds the address into a %lo store, and retail keeps it in $s0 across the
//    rcos call.
void BattlePlaceEffectByMode(s32 mode) {
    VECTOR vec;
    VECTOR* pVec;
    s32* pTargetX;
    s32 offset;
    s32 delta;
    s32 scale;
    s32 y;

    switch (mode) {
    case 0:
        BattleAimCameraAtCombatants(&g_BattleActor1.position, &g_BattleActor0);
        break;
    case 1:
        vec = g_BattleActor1.position;
        goto place_message;
    case 2:
        vec = g_BattleActor0;
    place_message:
        vec.vy -= 0xA0;
        g_BattleEffectPlaceYOffset = 0;
        func_80070808(&vec, 4);
        break;
    case 3:
        scale = 0xA0;
        goto place_follow;
    case 4:
        scale = 0x80;
    place_follow:
        // the block keeps the scale store in front of the vector copy
        do {
            if (mode) {
                g_BattleEffectPlaceYOffset = scale;
            } else {
                g_BattleEffectPlaceYOffset = scale;
            }
        } while (0);
        vec = g_BattleEffectPlaceOrigin;
        pVec = &vec;
        vec.vy += g_BattleEffectPlaceYOffset;
        func_80070808(pVec, 0x10);
        vec.vx = g_BattleActor1.position.vx + ((rcos(D_80098780 + 0xA80) * 0xD0) >> 12);
        y = g_BattleActor1.position.vy - 0x20;
        vec.vy = y - g_BattleEffectPlaceYOffset;
        vec.vz = g_BattleActor1.position.vz + ((rsin(D_80098780 + 0xA80) * 0xD0) >> 12);
        func_800708C4(pVec, 0x46);
        delta = func_80082488(&g_BattleCameraPos, 0) - 0x40;
        offset = delta;
        offset -= g_BattleEffectPlaceYOffset;
        if (offset < g_BattleCameraPos.vy) {
            g_BattleCameraPos.vy = offset;
        }
        break;
    case 5:
        pTargetX = &g_BattleCameraTarget.vx;
        *pTargetX = g_BattleActor0.vx;
        g_BattleCameraTarget.vy = g_BattleActor0.vy - 0xC0;
        g_BattleCameraTarget.vz = g_BattleActor0.vz;
        g_BattleCameraPos.vx = *pTargetX + ((rcos(D_80097064 + 0x900) * 0xE0) >> 12);
        g_BattleCameraPos.vy = g_BattleActor0.vy - 0xD0;
        g_BattleCameraPos.vz = g_BattleCameraTarget.vz + ((rsin(D_80097064 + 0x900) * 0xE0) >> 12);
        break;
    }
}


extern void BattleClearSpriteTables(void);

// Places the battle state's anchor, the actor at g_BattleActor0 and the camera
// around the midpoint of the first two, offset by the amount the mode picks.
void BattleRecenterViewPositions(s32 mode) {
    VECTOR anchor;
    VECTOR actor;
    VECTOR camera;
    VECTOR offset;

    anchor = g_BattleActor1.position;
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
    g_BattleActor1.position = anchor;
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
    /* 0x05C */ s32 pModelLink;
    /* 0x060 */ u8 unk60[0x6E];
    // Offset added to every facing this actor computes; zeroed when
    // BattleSetFacingAngleFromVector rebuilds one.
    /* 0x0CE */ s16 facingOffset;
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
    p->facingOffset = 0;
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
    // The two RECTs src/battling/main/misc14.c hands to SetDrawArea and func_8004546C
    // as &g_BattleDrawSlot[0] and [1].
    /* 0x00 */ RECT drawArea;
    /* 0x08 */ RECT drawOffset;
    /* 0x10 */ u8 unk10[0x64];
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
// SystemStringInit fills in, the line count at +0xC and the redraw flag at +0x68.
// The record is a SystemString (src/slus_006.64/system/system.c owns it, and the
// global is 0x90 bytes up to g_BattleCameraPos), so +0x0/+0x2/+0x8 are that
// struct's lineWidth/charIndex/spriteWidth.
typedef struct {
    /* 0x00 */ s16 lineWidth;
    /* 0x02 */ s16 charIndex;
    /* 0x04 */ s16 x;
    /* 0x06 */ s16 y;
    /* 0x08 */ s16 spriteWidth;
    /* 0x0A */ s16 flags;
    /* 0x0C */ s16 lineCount;
    /* 0x0E */ u8 unkE[0x5A];
    /* 0x68 */ u8 redraw;
} BattleMessageWindow; // Size: 0x69

extern BattleMessageWindow g_BattleMessageWindow;
extern s32 SystemStringInit(void* pString, s16 a, s16 b, s16 c, s16 d, s16 e, s16 f);

void BattleInitMessageWindow(void) {
    g_BattleMessageShownIndex = -1;
    g_BattleMessageIndex = 0;
    SystemStringInit(&g_BattleMessageWindow, 0x140, 0x30, 0x1C, 0x9A, 0x40, 4);
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
    SystemStringInit(&g_BattleTutorialWindow[0], 0x140, 0x70, 0xA2, 0x2A, 0x1C, 8);
    SystemStringPushBuffer(&g_BattleTutorialWindow[0], GetStringEntry(g_BattleMessageStringTable, 0x42));
    D_800929BC = 0x1E;
    D_800925DC = 0;
    BattleSetValueAndClearFlags(&g_BattleTutorialScript[0]);
    g_BattleTutorialLineOffset = 0;
    g_BattleEffectPlaceMode = 0;
    g_BattleStateStep = 0;
    func_8007191C(9);
}

extern u8 D_800925F0;
extern s32 g_BattleDrawFlags;
extern u8* g_BattleRootOrderingTable;
extern u16 g_C1ButtonStatePressedOnce;
extern u16 g_C1ButtonStateReleased;
extern void ControllerMergeStates(void);
extern void SystemStringSetColor(void* pString, s32 red, s32 green, s32 blue);
extern void SystemStringSetLineOffset(void* pString, s32 lineOffset);
extern void SystemStringReset(void* pString);
extern void func_80034888(void* pString, void* pOt, s32 arg);
extern void func_80079DF0(void* pActor1, void* pActor0);

// The tutorial text has this many lines; the pad scrolls the window's first line.
#define BATTLE_TUTORIAL_LINE_COUNT 8

/* The per-frame tutorial drawing/text step. The line offset is stepped as an
   unsigned byte (lbu/addiu/sb) and read back as a signed one when it is clamped
   and handed to SystemStringSetLineOffset (lb), so the three casts are what makes
   gcc choose the sign-extending load. g_BattleScriptCursor is held as an s32 by
   BattleSetValueAndClearFlags, so the script byte it points at is read through a
   cast rather than through a typed cursor.
*/
void BattleUpdateTutorialScreen(void) {
    BattleMessageWindow* pMsg;
    u8* pWindow;
    u8* pCursor;

    if (D_800925F0 != 0) {
        if ((g_BattleDrawFlags & 4) != 0) {
            BattleQueueSpriteAndTpagePrims(g_BattleRootOrderingTable);
        }
    }
    ControllerMergeStates();
    pMsg = &g_BattleMessageWindow;
    if (D_80092608 != 0) {
        if (g_C1ButtonStatePressedOnce & CTRL_BTN_UP) {
            BattlePlaySoundEffectById(0x1E);
            g_BattleTutorialLineOffset--;
        }
        if (g_C1ButtonStatePressedOnce & CTRL_BTN_DOWN) {
            BattlePlaySoundEffectById(0x1E);
            g_BattleTutorialLineOffset++;
        }
        if ((s8)g_BattleTutorialLineOffset >= BATTLE_TUTORIAL_LINE_COUNT) {
            g_BattleTutorialLineOffset = 0;
        }
        if ((s8)g_BattleTutorialLineOffset < 0) {
            g_BattleTutorialLineOffset = BATTLE_TUTORIAL_LINE_COUNT - 1;
        }
        pWindow = &g_BattleTutorialWindow[0];
        SystemStringSetColor(pWindow, (g_BattleDrawFlags * 7) & 0x3F, 0xC0, 0x10);
        SystemStringSetLineOffset(pWindow, (s8)g_BattleTutorialLineOffset);
        if (g_C1ButtonStateReleased & CTRL_BTN_CIRCLE) {
            BattlePlaySoundEffectById(0x21);
            func_8007191C((s8)g_BattleTutorialLineOffset + 1);
            SystemStringReset(pMsg);
        }
    }
    pCursor = (u8*)g_BattleScriptCursor;
    if (*pCursor == 0) {
        func_8007191C(0);
    }
    func_8007107C();
    g_BattleWindowPosX += BattleRoundDivAwayFromZero(D_800925E8, g_BattleWindowPosX, 4);
    g_BattleWindowPosY += BattleRoundDivAwayFromZero(D_800925EC, g_BattleWindowPosY, 4);
    if (D_80092608 != 0) {
        func_80034888(&g_BattleTutorialWindow[0], g_BattleRootOrderingTable, g_BattleDrawBufferIndex);
    }
    if (g_BattleMessageIndex != g_BattleMessageShownIndex) {
        pMsg->redraw = 3;
        SystemStringReset(pMsg);
        SystemStringPushBuffer(pMsg, GetStringEntry(g_BattleMessageStringTable, g_BattleMessageIndex));
        g_BattleMessageShownIndex = g_BattleMessageIndex;
    }
    func_80079DF0(&g_BattleActor1, &g_BattleActor0);
    BattlePlaceEffectByMode(g_BattleEffectPlaceMode);
}

#define BATTLE_GRID_NEIGHBOUR_COUNT 8
#define BATTLE_GRID_CELL_STEP 0xC0
// A neighbour has to be this much lower before the actor steps into it, and the
// walk gives up after this many steps.
#define BATTLE_GRID_WALK_TOLERANCE 0x40
#define BATTLE_GRID_WALK_MAX_STEPS 20
// flags bits 29-30: the grid cell type BattleGetGridCellAtPosition reports.
#define BATTLE_ACTOR_FLAGS_GRID_CELL_MASK 0x60000000

// The eight cells around a battle grid cell, as the (x, z) step from the cell to
// each neighbour in BATTLE_GRID_CELL_STEP units: the x column is g_BattleGridNeighbourSteps and
// the z column four bytes behind the next entry, so both are walked with a
// two-word stride. Retail reaches the x column through the symbol and keeps a
// pointer to the z column, which it re-forms for the final step; that is the
// shape the (i * 2) subscripts and pStepZ below reproduce (the same table spelled
// as an array of a two-field struct measures 82.12%).
extern s32 g_BattleGridNeighbourSteps[];
extern s32 D_80091088[];
extern u32 BattleGetGridCellAtPosition(VECTOR* pPos);

// Walks the actor's position down the battle grid, one grid cell at a time, as
// long as a lower neighbour cell exists, then stamps the height it ended on and
// the grid cell type it is standing in.
/* Walks the actor down the grid: while a neighbouring cell is a tolerance lower
   than the height it is standing at, step into the lowest neighbour, and stamp
   the height and cell type it ended on. pActor and pStepZ are the two copies
   retail keeps of a pointer it also needs outside the loop.
*/
void BattleActorWalkDownGrid(BattleState* p) {
    BattleState* pActor;
    s32* pStepZ;
    VECTOR pos;
    s32 bestIndex;
    s32 best;
    s32 count;
    s32 i;
    s32 value;

    pActor = p;
    pStepZ = D_80091088;
    count = 0;
    do {
        best = pActor->position.vy = func_80082488(&pActor->position, 1);
        for (i = 0; i < BATTLE_GRID_NEIGHBOUR_COUNT; i++) {
            pos = pActor->position;
            pos.vx += g_BattleGridNeighbourSteps[i * 2] * BATTLE_GRID_CELL_STEP;
            pos.vz += pStepZ[i * 2] * BATTLE_GRID_CELL_STEP;
            value = func_80082488(&pos, 1);
            if (value < best - BATTLE_GRID_WALK_TOLERANCE) {
                bestIndex = i;
                best = value;
            }
        }
        if (best >= pActor->position.vy - BATTLE_GRID_WALK_TOLERANCE) {
            pActor->position.vy = func_80082488(&pActor->position, 1);
            break;
        }
        pActor->position.vx -= g_BattleGridNeighbourSteps[bestIndex * 2] * BATTLE_GRID_CELL_STEP;
        pActor->position.vz -= pStepZ[bestIndex * 2] * BATTLE_GRID_CELL_STEP;
        count++;
    } while (count < BATTLE_GRID_WALK_MAX_STEPS);
    p->floorY = func_80082488(&p->position, 1);
    p->flags = (p->flags & ~BATTLE_ACTOR_FLAGS_GRID_CELL_MASK) |
               (((BattleGetGridCellAtPosition(&p->position) >> 24) & 3) << 29);
}

extern s32 g_BattleActorDistance2D;
extern s32 g_BattlingAudioManager;
extern void SoundStopAudioManager(s32 arg);
extern void SoundStopAllElements(void);
extern void BattleActorWalkDownGrid(BattleState* p);

/* 91.77%: case 3 is the only difference. retail materialises &g_BattleCameraPosY into
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
        SoundStopAudioManager(g_BattlingAudioManager);
        SoundStopAllElements();
        break;
    case 1:
        BattleActorWalkDownGrid((u8*)&g_BattleActor1);
        if (D_800970C4 * 0x100 / D_800970CC < D_800987E0 * 0x100 / D_800987E8) {
            g_BattleMessageIndex = 0x43;
        } else {
            g_BattleMessageIndex = 0x44;
        }
        break;
    case 2:
        BattleActorWalkDownGrid((u8*)&g_BattleActor0);
        break;
    case 3:
        g_BattleCameraPosY = -g_BattleActorDistance2D;
        if (g_BattleCameraPosY < -0x600) {
            g_BattleCameraPosY = -0x600;
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

// One past the last of the three effect-cell frame counters.
extern s32 g_BattleEffectCellsEnd;

// One of the effect cells BattleStepDisplayState steps: the 16.16 origin a burst is
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
extern u8* g_BattleRootOrderingTable;
extern s16 g_BattleScreenFadeLevel;
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
extern void BattlePlaceEffectByMode(s32 mode);

/* Steps the battle display state machine for the current frame: plays the effect
   sound, places a burst from a random source actor on the effect cells, fades the
   screen and keeps the tutorial message text in step.
   objdiff 99.81% at retail's 1080 bytes: the only differences are the switch's
   table operand (jtbl_ vs .rodata), the y/z stores naming g_BattleEffectCells+4/+8
   where retail's object names D_800929F8/D_800929FC, and the anchor position
   words reached as g_BattleActor1+4/+8 where retail names them D_80098730/
   D_80098734 - all of which the linker resolves identically. The case-1 loop
   needs the base as a mem_addr local (a pointer local lets gcc address the cells
   through it and drops retail's repeated 'lui $at/%hi + addu $at,$at,offset'
   stores) and a byte offset rather than an index (an indexed access gives loop.c
   a second induction pointer, +2 insns: 1096 bytes, 97.53%). The three placement
   stores keep their casts for the same reason: retail re-forms the address from
   the symbol for each of them, and neither a typed element pointer built from the
   sum (94.71% at 1068 and 95.96% at 1060 for one pointer local covering all three)
   nor a (BattleEffectEmitter*)(offset + base) view (95.77% at 1044) re-forms it.
*/

void BattleStepDisplayState(void) {
    BattleState* pState;
    BattleMessageWindow* pMsg;
    VECTOR* pAnchor;
    BattleEffectEmitter* pCell;
    BattleEffectOrigin* pOrigin;
    s32* pFrames;
    mem_addr base;
    VECTOR vec;
    s32 offset;
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
        base = (mem_addr)g_BattleEffectCells;
        pFrames = &g_BattleEffectCells[0].frames;
        offset = 0;
        pCell = g_BattleEffectCells;
        do {
            frames = *pFrames - 1;
            pCell->frames = frames;
            if (frames == -1) {
                pOrigin = g_BattleEffectPool->pPool->pOrigins[rand() % g_BattleEffectPool->pPool->count];
                ((BattleEffectEmitter*)((u8*)g_BattleEffectCells + offset))->x = pOrigin->x + pAnchor->vx;
                ((BattleEffectEmitter*)((u8*)g_BattleEffectCells + offset))->y = pOrigin->y + pAnchor->vy;
                ((BattleEffectEmitter*)((u8*)g_BattleEffectCells + offset))->z = pOrigin->z + pAnchor->vz;
                *pFrames = rand() % 16 + 8;
            }
            if (*pFrames & 1) {
                BattleEffectAddSlotFromVector((VECTOR*)(offset + base), 9);
            }
            pFrames += 4;
            offset += 0x10;
            pCell++;
        } while ((s32)pFrames < (s32)&g_BattleEffectCellsEnd);
        break;
    case 2:
        pState->gaugeValue = 1;
        g_BattleScreenFadeLevel = 0xFF;
        g_BattleStateStep++;
    case 3:
        func_8008E2B8(g_BattleRootOrderingTable, g_BattleScreenFadeLevel, 0);
        if (g_BattleScreenFadeLevel < 0) {
            g_BattleScreenFadeLevel = 0;
        }
        g_BattleScreenFadeLevel -= 8;
fade:
        pState->animSpeed = 0x10;
        pState->modelIndex = 0xA;
        pState->animSpeedBonus = 0;
        pState->gaugePoints = 0x1000;
        pState->flags |= 0x400;
        if (rand() & 1) {
            count = g_BattleEffectPool->pPool->count;
            pOrigin = g_BattleEffectPool->pPool->pOrigins[rand() % count];
            vec.vx = pOrigin->x + g_BattleActor1.position.vx;
            vec.vy = pOrigin->y + g_BattleActor1.position.vy;
            vec.vz = pOrigin->z + g_BattleActor1.position.vz;
            BattleEffectAddSlotFromVector(&vec, 0xB);
            BattleEffectAddSlotFromVector(&vec, 8);
        }
        break;
    case 4:
        g_BattleScreenFadeLevel += 3;
        if (g_BattleScreenFadeLevel >= 0x100) {
            g_BattleScreenFadeLevel = 0xFF;
            g_BattleResultCode = 0x7F;
            BattleClearScreen(0);
            BattleReturnToMainLoop();
        }
        func_8008E2B8(g_BattleRootOrderingTable, g_BattleScreenFadeLevel, 1);
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
    BattlePlaceEffectByMode(g_BattleEffectPlaceMode);
    D_80098774 = 0;
}




void BattleEmptyFunction(void) {
}

extern void* BattleCreateOrderingTablePair(s32 a0);
extern void* BattleCreateLightGroup(void* p);
extern void* BattleInitObjectTree(s32 a0);
extern void func_80078F00(void* p);
extern s16 g_BattleDebugFlags;
extern void* g_BattleRenderActor;
extern s32 g_BattleViewOriginZ;
extern s32 g_BattleViewOriginY;
extern s32 g_BattleViewOrigin;
extern u8 g_BattleRenderToggleCount;
extern s32 g_BattleRenderAngleX;
extern s32 g_BattleRenderAngleYStep;
extern s32 g_BattleRenderOffsetY;
extern s32 g_BattleRenderOffsetZ;
extern s32 g_BattleRenderModelColor;
extern s16 g_BattleRenderAngleY;
extern void* g_BattleLightGroup;
extern void* g_BattleNodeTree;

// The block the actor's script pointer (+0x8FC) addresses: the five signed
// halfwords and the unsigned one BattleInitRenderScene copies into the render
// state at g_BattleRenderToggleCount..g_BattleRenderAngleY.
struct BattleScriptBlock {
    /* 0x00 */ u8 unk00[0x14];
    /* 0x14 */ s16 angleX;
    /* 0x16 */ s16 angleYStep;
    /* 0x18 */ s16 offsetY;
    /* 0x1A */ s16 offsetZ;
    /* 0x1C */ s16 modelColor;
    /* 0x1E */ u16 angleY;
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
    id = p->pModelLink;
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
        g_BattleViewOriginZ = 0;
        g_BattleViewOriginY = 0;
        g_BattleViewOrigin = 0;
    } while (0);
    pScript = p->pScript;
    x = pScript->angleX;
    y = pScript->angleYStep;
    z = pScript->offsetY;
    w = pScript->offsetZ;
    u = pScript->modelColor;
    v = pScript->angleY;
    g_BattleRenderToggleCount = 1;
    g_BattleRenderAngleX = x;
    g_BattleRenderAngleYStep = y;
    g_BattleRenderOffsetY = z;
    g_BattleRenderOffsetZ = w;
    g_BattleRenderModelColor = u;
    g_BattleRenderAngleY = v;
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
    mtx.t[1] = -g_BattleViewOriginY;
    mtx.t[2] = p->unk34.vz - g_BattleViewOriginZ;
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
