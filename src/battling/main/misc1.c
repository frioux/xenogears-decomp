#include "common.h"
#include "psyq/libgpu.h"
#include "system/controller.h"

// The tutorial message window: the SystemString the tutorial text is pushed into,
// then the byte BattleInitTutorialMessage seeds at +0x68. 0x90 bytes; the
// g_BattleCameraPos VECTOR follows it.
typedef struct {
    /* 0x00 */ u8 unk0[0x68];
    /* 0x68 */ u8 unk68;
} BattleTutorialWindow; // Size: 0x90

extern BattleTutorialWindow g_BattleTutorialWindow;

extern void BattleSetScreenMode(int mode);
extern void SystemStringFree(void* pString);

void BattleFreeMessageString(void) {
    BattleSetScreenMode(3);
    SystemStringFree(&g_BattleTutorialWindow);
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

// Advances the camera anchor vx/vz/vy one step toward the vector's position. The two
// pointer locals are load-bearing: retail keeps the anchor's vx address in $s0 across
// the first call and reads the sources through a pointer, and the field spellings
// instead make gcc hoist that source load above the vz store.
void BattleStepCameraTargetTowardVector(VECTOR* pVec, s32 step) {
    s32* pTargetX = &g_BattleCameraTarget.vx;
    s32* pSourceY = &pVec->vy;

    *pTargetX += BattleRoundDivAwayFromZero(pVec->vx, *pTargetX, step);
    g_BattleCameraTargetZ += BattleRoundDivAwayFromZero(pVec->vz, g_BattleCameraTargetZ, step);
    g_BattleCameraTargetY += BattleRoundDivAwayFromZero(*pSourceY, g_BattleCameraTargetY + g_BattleEffectPlaceYOffset, step);
}
extern VECTOR g_BattleCameraPos;
extern s32 g_BattleCameraPosY;
extern s32 g_BattleCameraPosZ;
extern void BattleMovePointInsideRadius(VECTOR* pPos, VECTOR* pDelta, s32 radius);

// Moves the camera to within 0x3D00 of the vector: the three step deltas are clamped
// as one offset before they are added back, so the anchor's vx is read and written
// through the pointer while vy/vz stay named globals (retail addresses them that way).
void BattleMoveCameraPosTowardVector(VECTOR* pVec, s32 step) {
    VECTOR delta;
    VECTOR* pPos;

    pPos = &g_BattleCameraPos;
    delta.vx = BattleRoundDivAwayFromZero(pVec->vx, pPos->vx, step);
    delta.vy = BattleRoundDivAwayFromZero(pVec->vy, g_BattleCameraPosY, step);
    delta.vz = BattleRoundDivAwayFromZero(pVec->vz, g_BattleCameraPosZ, step);
    BattleMovePointInsideRadius(pPos, &delta, 0x3D00);
    pPos->vx += delta.vx;
    g_BattleCameraPosY += delta.vy;
    g_BattleCameraPosZ += delta.vz;
}

// The battling overlay's battle state (g_BattleActor0/g_BattleActor1): the anchor
// position every effect vector is placed against, the model index and gauge words
// at +0x4C..+0xB6 (the same record src/battling/main/misc3 and misc4 name), and
// the frame flags the effect cells below are switched on through. Its position is
// a VECTOR: three sites copy 16 bytes out of it, and the two actors are the
// VECTORs the camera is aimed between.
typedef struct BattleStateTag {
    /* 0x000 */ VECTOR position;
    /* 0x010 */ u8 unk10[0x38];
    // Reset by BattleSetValueAndClearFlags, and set to 0xFF when
    // BattleSetFacingAngleFromVector rebuilds a facing.
    /* 0x048 */ u32 distance;
    /* 0x04C */ u8 modelIndex;
    /* 0x04D */ u8 unk4D[0x2];
    /* 0x04F */ u8 animSpeed;
    /* 0x050 */ u8 unk50[0x2];
    /* 0x052 */ u8 animSpeedBonus;
    /* 0x053 */ u8 unk53[0x5];
    // The facing BattleSetFacingAngleFromVector rebuilds, and the idle turn the
    // battle script's wait opcodes start.
    /* 0x058 */ s32 targetAngle;
    /* 0x05C */ u8 unk5C[0x54];
    // The height BattleGetFloorHeightAtPosition reports for the actor's position, refreshed every
    // frame by the grid walk below and by BattleActorUpdateGroundMovement.
    /* 0x0B0 */ s32 floorY;
    /* 0x0B4 */ s16 gaugeValue;
    /* 0x0B6 */ s16 gaugePoints;
    /* 0x0B8 */ u8 unkB8[0x4];
    /* 0x0BC */ u16 unkBC;
    /* 0x0BE */ u8 unkBE[0x10];
    // Offset added to every facing this actor computes; cleared when a script
    // turn opcode takes the facing over.
    /* 0x0CE */ s16 facingOffset;
    /* 0x0D0 */ u32 flags;
    /* 0x0D4 */ u8 unkD4[0x4];
    // The record the script's two gauge opcodes mirror into.
    /* 0x0D8 */ struct BattleStateTag* unkD8;
    /* 0x0DC */ u8 unkDC[0xC];
    /* 0x0E8 */ s32 unkE8;
} BattleState; // Size: 0xEC

extern BattleState g_BattleActor0;
extern BattleState g_BattleActor1;
extern VECTOR g_BattleEffectPlaceOrigin;
extern s32 D_80098780;
extern s32 D_80097064;
extern s32 g_BattleSceneOriginYMirror;
extern s32 g_BattleSceneOriginZ;

extern void BattleStepCameraTargetTowardVector(VECTOR* pVec, s32 step);
extern void BattleMoveCameraPosTowardVector(VECTOR* pVec, s32 step);
extern void BattleAimCameraAtCombatants(VECTOR* pA, VECTOR* pB);
extern s32 BattleGetFloorHeightAtPosition(VECTOR* pVec, s32 mode);

/* 92.82%: same size as retail and every statement/call/constant is right; the
   gaps are allocation and position. Remaining diffs, all in the case 3/4 tail:
   (a) retail forms &vec (s0) after the g_BattleEffectPlaceOrigin copy, ours hoists the
   addiu s0 above it; (b) the BattleGetFloorHeightAtPosition delta keeps retail's -0x40 in $v0
   (the call result) while ours copies it to $a0 first; (c) case 5 keeps
   &g_BattleCameraTarget in $s0 across the rcos call (sw v0,0(s0), then lw v0,0(s0)) where
   ours rematerialises the symbol as a direct %lo store. A pointer local does
   not stop that. Do not re-derive the shape: the do-while around the
   g_BattleEffectPlaceYOffset store is load-bearing (91.24 -> 92.82, and it restores retail's
   byte count), and the y local is what stops gcc folding
   (g_BattleSceneOriginYMirror - 0x20) - g_BattleEffectPlaceYOffset into g_BattleSceneOriginYMirror - (g_BattleEffectPlaceYOffset + 0x20).
*/

// Places the effect vector (a stack VECTOR) and the camera for the placement
// mode g_BattleEffectPlaceMode names: recentres the tutorial message, or snaps
// the camera onto the front actor.  objdiff 99.29%: the only difference left is
// the switch's table operand (jtbl_ vs .rodata), check_bin.sh battling is OK.
// Three spellings are load-bearing and look gratuitous:
//  - the do-while plus its identical if/else arms: the arms are the same store,
//    and the join is what puts &vec's addiu after the g_BattleEffectPlaceOrigin copy;
//  - the delta/offset copy keeps the BattleGetFloorHeightAtPosition result in $v0 for the -0x40;
//  - pTargetX is a pointer to the anchor's vx word: the plain field spelling
//    folds the address into a %lo store, and retail keeps it in $s0 across the
//    rcos call.
// D_80098780/D_80097064 are +0x54 of g_BattleActor1/g_BattleActor0 (the angle the
// two placement orbits start from). Typing them as actor fields is measured worse:
// cse then shares the base with the actor's position reads, holds it in $s0 across
// the calls and the frame grows to 0x30 (744 bytes, 96.71), where the splat-named
// words are re-formed per use; a volatile read of the field measures the same.
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
        BattleAimCameraAtCombatants(&g_BattleActor1.position, &g_BattleActor0.position);
        break;
    case 1:
        vec = g_BattleActor1.position;
        goto place_message;
    case 2:
        vec = g_BattleActor0.position;
    place_message:
        vec.vy -= 0xA0;
        g_BattleEffectPlaceYOffset = 0;
        BattleStepCameraTargetTowardVector(&vec, 4);
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
        BattleStepCameraTargetTowardVector(pVec, 0x10);
        vec.vx = g_BattleActor1.position.vx + ((rcos(D_80098780 + 0xA80) * 0xD0) >> 12);
        y = g_BattleActor1.position.vy - 0x20;
        vec.vy = y - g_BattleEffectPlaceYOffset;
        vec.vz = g_BattleActor1.position.vz + ((rsin(D_80098780 + 0xA80) * 0xD0) >> 12);
        BattleMoveCameraPosTowardVector(pVec, 0x46);
        delta = BattleGetFloorHeightAtPosition(&g_BattleCameraPos, 0) - 0x40;
        offset = delta;
        offset -= g_BattleEffectPlaceYOffset;
        if (offset < g_BattleCameraPos.vy) {
            g_BattleCameraPos.vy = offset;
        }
        break;
    case 5:
        pTargetX = &g_BattleCameraTarget.vx;
        *pTargetX = g_BattleActor0.position.vx;
        g_BattleCameraTarget.vy = g_BattleActor0.position.vy - 0xC0;
        g_BattleCameraTarget.vz = g_BattleActor0.position.vz;
        g_BattleCameraPos.vx = *pTargetX + ((rcos(D_80097064 + 0x900) * 0xE0) >> 12);
        g_BattleCameraPos.vy = g_BattleActor0.position.vy - 0xD0;
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
    actor = g_BattleActor0.position;
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
    g_BattleActor0.position = actor;
    g_BattleCameraPos = camera;
    BattleClearSpriteTables();
}

void BattleSetValueAndClearFlags(s32 value) {
    g_BattleScriptCursor = value;
    g_BattleActor1.distance = 0;
    g_BattleActor0.distance = 0;
    g_BattleScriptWait = 0;
    g_BattleActor1.flags &= 0xFFFF7FFF;
    g_BattleActor0.flags &= 0xFFFF7FFF;
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

s32 BattleSetFacingAngleFromVector(BattleState* p) {
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
extern BattleState* D_80092894;
extern s16 D_80092600;
extern s32 g_BattleActorDistance2D;
extern s32 g_BattleEffectPlaceMode;
extern s32 g_BattleStateStep;
extern u16 g_C1ButtonStateReleased;
extern s32 g_BattleMessageIndex;
extern s32 g_BattleWindowTargetX;
extern s32 g_BattleWindowPosX;
extern s32 g_BattleWindowTargetY;
extern s32 g_BattleWindowPosY;
extern u8 g_BattleWindowPrimsEnabled;
extern void BattleResetMessageQueue(BattleState* pActor);
extern void BattleQueueMessage(BattleState* pActor, s32 index);
extern s32 BattleCheckPositionPastMapCenter(VECTOR* pPos);
extern s32 SystemStringGetUnk6B(void* pString);
extern void SystemStringClearStateFlags(void* pString);
extern void BattleRecenterViewPositions(s32 mode);
extern s32 func_80071F8C(s32 mode);

// The battle script g_BattleScriptCursor walks: an opcode byte, then up to two
// operand bytes. The two stop distances a turn opcode waits for, and the distance
// a started turn sets on the actor.
#define BATTLE_SCRIPT_OPCODE 0
#define BATTLE_SCRIPT_OPERAND0 1
#define BATTLE_SCRIPT_OPERAND1 2
#define BATTLE_TURN_DISTANCE 0xF0
#define BATTLE_TURN_NEAR_DISTANCE 0x100
#define BATTLE_TURN_MID_DISTANCE 0x400
#define BATTLE_TURN_FAR_DISTANCE 0x800
#define BATTLE_TURN_RADIUS 0x400

/* 89.47% (1684 bytes against retail's 1704, 421 insns against 426): the whole
   interpreter is right - all 34 opcode arms, the two shared cursor tails, the
   arm-the-wait block and the turn block gcc merged out of cases 12/13/14/15 - and
   what is left is block placement inside four arms plus one lost insn per shared
   tail. Spelling facts that are load-bearing and were measured:
   - the arms must be written in retail's PHYSICAL order (1,2,3,4,5,6,8,7,10,9,11,
     12,13,15,14,20,18,19,27,21,22,23,24,25,26,28,29,30,31,32,33,34): gcc emits the
     case bodies in source order, so the natural 1..34 order puts every arm 4-8
     bytes off and objdiff collapses to 76.16% with the same instruction stream;
   - `case 0: return;` is real: retail's guard is `sltiu v1,0x23` with table entry 0
     pointing at the return, so opcode 0 ends the script and is not the default;
   - the table lands at .rodata+0x18, retail's jtbl_8006FB0C offset;
   - cases 12/13/14/15 share one `turn` label (goto) and 1/14/15/20 one `armWait`
     label; gcc does merge those tails, so they must be labels, not copies.
   Residuals, each 4-8 insns of the diff: (a) cases 11, 12 and 13 each end with
   their own copy of `li v0,<angle>` + turn tail where retail enters ONE turn block
   with the angle already in v0 (`j .L800712F8` with `li v0,0x800` in the branch
   delay) - the store `sw s1,72(s0)` sits at each site, only the tail is shared;
   (b) case 2/3 form `&g_BattleActor1` straight into s0 where retail forms it in v1
   and does `move s0,v1`; (c) the dispatch's `beqz` delay slot is `sll` here and
   `nop` in retail (1 insn, and it is why this is 5 insns short); (d) case 21
   schedules the second `lbu` after the `sb`, retail loads both operands first;
   (e) case 18's two cursor stores come out with one load here, retail stores +1
   and then +2 from one loaded cursor (a `s32 value = g_BattleScriptCursor;`
   local reproduces the two stores but not the single load).
   D_80092894 is the pointer the script engine drives; the DA_/gauge fields added
   to BattleState above are the +0x58/+0xBC/+0xCE/+0xD8 slots this interpreter
   touches. */
/* 94.01% (1688 bytes against retail's 1704, 422 insns against 426). Structure and
   block PLACEMENT are the win here: the armWait and turn label blocks belong INSIDE
   the switch, physically between the arms gcc emits around them (turn between case 14
   and case 20, armWait between case 20 and case 18) - that alone took 89.47 -> 94.06.
   Also measured: case 29 stores the same value in both arms (a `s32 value = 1;` above
   the if); writing 0 in the else arm was wrong code and retail has 1 in both, but the
   inline literal form breaks the frame (saves s3, sp 0x30) while the local keeps
   retail's 0x28 frame.
   PRESET screen: cdk 88.30, 2.6.0 89.47 (identical), -G0 compile-failed. Not a preset.
   Remaining residuals, each named: (a) cases 2/3 - retail forms &g_BattleActorN in v1,
   loads the cursor, then `addu s0,v1,zero`; every spelling tried puts the `la` straight
   into s0 (8 insns vs retail's 9) and the cursor load lands before it; (b) the dispatch
   beqz delay slot is `sll v0,v1,2` here and `nop` in retail (1 insn); (c) case 12's
   turn constant still needs a hop block (j +0x218) where retail puts `ori v0,0x800` in
   the beqz delay slot, while case 13 keeps a `nop` there; case 15/14 are exact; (d) case
   21 stores g_BattleWindowPrimsEnabled before the sll/sw of targetX, retail after both
   operand loads, and uses v1 for the cursor where this uses a0; (e) case 29's two
   stores pick a0 where retail picks v0. */
void func_8007107C(void) {
    BattleState* pActor;
    s32 angle;
    s32 value;

    pActor = D_80092894;
    pActor->distance = 0;
    if (D_80092600 != 0) {
        if (D_80092600 < pActor->gaugePoints) {
            D_80092600 = pActor->gaugePoints;
        }
        pActor->gaugePoints = D_80092600;
    }
    for (;;) {
        switch (((u8*)g_BattleScriptCursor)[BATTLE_SCRIPT_OPCODE]) {
        case 0:
            return;
        case 1:
            if (g_BattleScriptWait == 0) {
                goto armWait;
            }
            if (--g_BattleScriptWait != 0) {
                return;
            }
            g_BattleScriptCursor += 2;
            break;
        case 2:
            D_80092894 = pActor = &g_BattleActor1;
            g_BattleScriptCursor++;
            break;
        case 3:
            D_80092894 = pActor = &g_BattleActor0;
            g_BattleScriptCursor++;
            break;
        case 4:
            BattleResetMessageQueue(pActor);
            g_BattleScriptCursor++;
            break;
        case 5:
            BattleQueueMessage(pActor, 1);
            g_BattleScriptCursor++;
            break;
        case 6:
            BattleQueueMessage(pActor, 2);
            g_BattleScriptCursor++;
            break;
        case 8:
            BattleQueueMessage(pActor, 3);
            g_BattleScriptCursor++;
            break;
        case 7:
            BattleQueueMessage(pActor, 4);
            g_BattleScriptCursor++;
            break;
        case 10:
            BattleQueueMessage(pActor, 5);
            g_BattleScriptCursor++;
            break;
        case 9:
            BattleQueueMessage(pActor, 5);
            g_BattleScriptCursor++;
            break;
        case 11:
            if (g_BattleActorDistance2D >= BATTLE_TURN_NEAR_DISTANCE) {
                pActor->distance = BATTLE_TURN_DISTANCE;
                pActor->targetAngle = 0;
                pActor->facingOffset = 0;
                return;
            }
            g_BattleScriptCursor++;
            break;
        case 12:
            if (g_BattleActorDistance2D < BATTLE_TURN_MID_DISTANCE + 1) {
                angle = BATTLE_TURN_FAR_DISTANCE;
                pActor->distance = BATTLE_TURN_DISTANCE;
                pActor->targetAngle = angle;
                pActor->facingOffset = 0;
                return;
            }
            g_BattleScriptCursor++;
            break;
        case 13:
            if (BattleCheckPositionPastMapCenter(&pActor->position) != 0) {
                BattleSetFacingAngleFromVector(pActor);
                return;
            }
            if (g_BattleActorDistance2D < BATTLE_TURN_FAR_DISTANCE + 1) {
                angle = BATTLE_TURN_FAR_DISTANCE;
                pActor->distance = BATTLE_TURN_DISTANCE;
                pActor->targetAngle = angle;
                pActor->facingOffset = 0;
                return;
            }
            g_BattleScriptCursor++;
            break;
        case 15:
            if (g_BattleScriptWait == 0) {
                goto armWait;
            }
            if (--g_BattleScriptWait != 0) {
                angle = BATTLE_TURN_RADIUS;
                pActor->distance = BATTLE_TURN_DISTANCE;
                pActor->targetAngle = angle;
                pActor->facingOffset = 0;
                return;
            }
            g_BattleScriptCursor += 2;
            break;
        case 14:
            if (g_BattleScriptWait == 0) {
                goto armWait;
            }
            if (--g_BattleScriptWait != 0) {
                angle = -BATTLE_TURN_RADIUS;
                pActor->distance = BATTLE_TURN_DISTANCE;
                pActor->targetAngle = angle;
                pActor->facingOffset = 0;
                return;
            }
            g_BattleScriptCursor += 2;
            break;
        case 20:
            if (g_BattleScriptWait == 0) {
                goto armWait;
            }
            if (--g_BattleScriptWait != 0) {
                pActor->distance = BATTLE_TURN_DISTANCE;
                pActor->targetAngle = -BATTLE_TURN_RADIUS;
                pActor->facingOffset = 0;
                pActor->flags |= 0x8000;
                return;
            }
            g_BattleScriptCursor += 2;
            break;
        armWait:
            g_BattleScriptWait = ((u8*)g_BattleScriptCursor)[BATTLE_SCRIPT_OPERAND0];
            return;
        case 18:
            value = g_BattleScriptCursor;
            g_BattleScriptCursor = value + 1;
            g_BattleMessageIndex = ((u8*)value)[BATTLE_SCRIPT_OPERAND0];
            g_BattleScriptCursor = value + 2;
            break;
        case 19:
            SystemStringClearStateFlags(&g_BattleMessageWindow);
            g_BattleScriptCursor++;
            break;
        case 27:
            g_BattleWindowTargetX = 0xA0;
            g_BattleWindowPosX = 0xA0;
            g_BattleWindowPrimsEnabled = 0;
            g_BattleWindowTargetY = 0x6D;
            g_BattleWindowPosY = 0x6D;
            g_BattleScriptCursor++;
            break;
        case 21:
            g_BattleWindowTargetX = ((u8*)g_BattleScriptCursor)[BATTLE_SCRIPT_OPERAND0] << 1;
            g_BattleWindowPrimsEnabled = 1;
            g_BattleWindowTargetY = ((u8*)g_BattleScriptCursor)[BATTLE_SCRIPT_OPERAND1];
            g_BattleScriptCursor += 3;
            break;
        case 22:
            func_80071F8C(((u8*)g_BattleScriptCursor)[BATTLE_SCRIPT_OPERAND0]);
            g_BattleScriptCursor += 2;
            break;
        case 23:
            g_BattleEffectPlaceMode = ((u8*)g_BattleScriptCursor)[BATTLE_SCRIPT_OPERAND0];
            g_BattleScriptCursor += 2;
            break;
        case 24:
            g_BattleStateStep = ((u8*)g_BattleScriptCursor)[BATTLE_SCRIPT_OPERAND0];
            g_BattleScriptCursor += 2;
            break;
        case 25:
            if (SystemStringGetUnk6B(&g_BattleMessageWindow) == 0) {
                return;
            }
            if ((g_C1ButtonStateReleased & CTRL_BTN_CIRCLE) == 0) {
                return;
            }
            if (SystemStringGetUnk6B(&g_BattleMessageWindow) == 1) {
                g_BattleScriptCursor++;
            }
            SystemStringClearStateFlags(&g_BattleMessageWindow);
            break;
        case 26:
            if (SystemStringGetUnk6B(&g_BattleMessageWindow) == 0) {
                return;
            }
            if ((g_C1ButtonStateReleased & CTRL_BTN_CIRCLE) == 0) {
                return;
            }
            value = SystemStringGetUnk6B(&g_BattleMessageWindow);
            if (value >= 4) {
                return;
            }
            if (value < 2) {
                return;
            }
            SystemStringClearStateFlags(&g_BattleMessageWindow);
            g_BattleScriptCursor++;
            return;
        case 28:
            if (((u8*)g_BattleScriptCursor)[BATTLE_SCRIPT_OPERAND0] != 0) {
                pActor->unkD8->gaugeValue = pActor->unkD8->unkBC;
            } else {
                pActor->gaugeValue = pActor->unkBC;
            }
            g_BattleScriptCursor += 2;
            break;
        case 29:
            value = 1;
            if (((u8*)g_BattleScriptCursor)[BATTLE_SCRIPT_OPERAND0] != 0) {
                pActor->unkD8->gaugeValue = value;
            } else {
                pActor->gaugeValue = value;
            }
            g_BattleScriptCursor += 2;
            break;
        case 30:
            BattleFreeMessageString();
            g_BattleScriptCursor++;
            break;
        case 31:
            pActor->gaugePoints = ((u8*)g_BattleScriptCursor)[BATTLE_SCRIPT_OPERAND0] << 4;
            g_BattleScriptCursor += 2;
            break;
        case 32:
            D_80092600 = ((u8*)g_BattleScriptCursor)[BATTLE_SCRIPT_OPERAND0] << 4;
            g_BattleScriptCursor += 2;
            break;
        case 33:
            BattleRecenterViewPositions(((u8*)g_BattleScriptCursor)[BATTLE_SCRIPT_OPERAND0]);
            g_BattleScriptCursor += 2;
            break;
        case 34:
            if (((u8*)g_BattleScriptCursor)[BATTLE_SCRIPT_OPERAND0] != 0) {
                pActor->flags |= 2;
            } else {
                pActor->flags &= ~2;
                pActor->flags &= ~0x38;
            }
            g_BattleScriptCursor += 2;
            break;
        case 16:
        case 17:
            break;
        default:
            return;
        }
        continue;
    }
}




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

// The battle texture archive. This unit only reads the TIM at 0x60.
typedef struct {
    /* 0x00 */ u32 pointerCount;
    /* 0x04 */ u_long* unk04[0x17];
    /* 0x60 */ u_long* pTim;
} BattleTextureArchive; // Size: 0x64

void func_80071794(BattleTextureArchive* pArchive) {
    TIM_IMAGE tim;
    u_short tpage;
    u16 tpageBits;
    s16* pClut;
    RECT rect; // retail's frame has eight bytes the C never touches

    OpenTIM(pArchive->pTim);
    ReadTIM(&tim);
    pClut = (s16*)tim.caddr;
    pClut[0] = 0;
    pClut[2] = -0x8000;
    pClut[3] = -1;
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    tpage = GetTPage(0, 1, tim.prect->x, tim.prect->y);
    SetDrawTPage(g_BattleTpagePrims, 0, 0, tpage);
    g_BattleTpagePrims[1] = g_BattleTpagePrims[0];
    tpageBits = *(u16*)tim.prect;
    D_8009A14C.u0 = (tpageBits & 0x3F) * 4;
    D_8009A14C.v0 = ((u16*)tim.prect)[1];
    D_8009A14C.clut = GetClut(tim.crect->x, tim.crect->y);
    D_8009A244 = D_8009A14C;
}
extern s32 g_BattleMessageIndex;
extern s32 g_BattleMessageShownIndex;

extern s32 SystemStringInit(void* pString, s16 a, s16 b, s16 c, s16 d, s16 e, s16 f);

void BattleInitMessageWindow(void) {
    g_BattleMessageShownIndex = -1;
    g_BattleMessageIndex = 0;
    SystemStringInit(&g_BattleMessageWindow, 0x140, 0x30, 0x1C, 0x9A, 0x40, 4);
}

extern u8 g_BattleTutorialWindowEnabled;
extern s16 D_80098698;
extern s16 D_80098692;
extern s32 g_BattleTutorialScriptEntryPoints[];
extern s32 g_BattleWindowTargetX;
extern s32 g_BattleWindowPosX;
extern s32 g_BattleWindowTargetY;
extern s32 g_BattleWindowPosY;
extern s16 D_80092600;
extern s16 D_800987E0;
extern s16 D_800987E8;
extern s16 D_800970C4;
extern s16 D_800970CC;
extern void BattlePlaySoundEffectById(s32 id);

// Selects the tutorial page's text window: page 0 plays the page sound and uses
// its own two size words. The D_80098692 store is written per arm, not through a
// shared local: gcc cross-jumps the two identical tails into retail's single
// store, and a `value` local instead measures 89.15% (the else arm's constant is
// then stolen into the bnez delay slot and the shared store lands after the call's
// argument load).
void BattleSelectTutorialPage(u32 idx) {
    g_BattleTutorialWindowEnabled = idx < 1;
    if (idx == 0) {
        BattlePlaySoundEffectById(0x37);
        D_80098698 = 2;
        D_80098692 = 0xB4;
    } else {
        D_80098698 = 4;
        D_80098692 = 0x9A;
    }
    BattleSetValueAndClearFlags(g_BattleTutorialScriptEntryPoints[idx]);
    g_BattleWindowTargetX = 0xA0;
    g_BattleWindowPosX = 0xA0;
    g_BattleWindowTargetY = 0x6D;
    g_BattleWindowPosY = 0x6D;
    D_80092600 = 0;
    D_800987E0 = D_800987E8;
    D_800970C4 = D_800970CC;
}


extern u8 g_BattlePlayer1ComControlled;
extern u8 g_BattlePlayer2ComControlled;
extern s32 g_BattleMode;
extern u8 g_BattleOptionOn;
extern u8* g_BattleMessageStringTable;
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
    SystemStringInit(&g_BattleTutorialWindow, 0x140, 0x70, 0xA2, 0x2A, 0x1C, 8);
    SystemStringPushBuffer(&g_BattleTutorialWindow, GetStringEntry(g_BattleMessageStringTable, 0x42));
    g_BattleTutorialWindow.unk68 = 0x1E;
    D_800925DC = 0;
    BattleSetValueAndClearFlags(&g_BattleTutorialScript[0]);
    g_BattleTutorialLineOffset = 0;
    g_BattleEffectPlaceMode = 0;
    g_BattleStateStep = 0;
    BattleSelectTutorialPage(9);
}

extern u8 g_BattleWindowPrimsEnabled;
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

    if (g_BattleWindowPrimsEnabled != 0) {
        if ((g_BattleDrawFlags & 4) != 0) {
            BattleQueueSpriteAndTpagePrims(g_BattleRootOrderingTable);
        }
    }
    ControllerMergeStates();
    pMsg = &g_BattleMessageWindow;
    if (g_BattleTutorialWindowEnabled != 0) {
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
        pWindow = (u8*)&g_BattleTutorialWindow;
        SystemStringSetColor(pWindow, (g_BattleDrawFlags * 7) & 0x3F, 0xC0, 0x10);
        SystemStringSetLineOffset(pWindow, (s8)g_BattleTutorialLineOffset);
        if (g_C1ButtonStateReleased & CTRL_BTN_CIRCLE) {
            BattlePlaySoundEffectById(0x21);
            BattleSelectTutorialPage((s8)g_BattleTutorialLineOffset + 1);
            SystemStringReset(pMsg);
        }
    }
    pCursor = (u8*)g_BattleScriptCursor;
    if (*pCursor == 0) {
        BattleSelectTutorialPage(0);
    }
    func_8007107C();
    g_BattleWindowPosX += BattleRoundDivAwayFromZero(g_BattleWindowTargetX, g_BattleWindowPosX, 4);
    g_BattleWindowPosY += BattleRoundDivAwayFromZero(g_BattleWindowTargetY, g_BattleWindowPosY, 4);
    if (g_BattleTutorialWindowEnabled != 0) {
        func_80034888(&g_BattleTutorialWindow, g_BattleRootOrderingTable, g_BattleDrawBufferIndex);
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
// as an array of a two-field struct measures 82.12%). D_80091088 is the z column
// (0x80091084 + 4) and cannot be folded into g_BattleGridNeighbourSteps: spelled
// as &g_BattleGridNeighbourSteps[1] gcc derives it from the x-column base (one
// addiu s8,s8,4 in the prologue) and stops re-forming it for the final step, which
// is 96.31% at the exact 488 bytes against this spelling's 100.00%.
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
        best = pActor->position.vy = BattleGetFloorHeightAtPosition(&pActor->position, 1);
        for (i = 0; i < BATTLE_GRID_NEIGHBOUR_COUNT; i++) {
            pos = pActor->position;
            pos.vx += g_BattleGridNeighbourSteps[i * 2] * BATTLE_GRID_CELL_STEP;
            pos.vz += pStepZ[i * 2] * BATTLE_GRID_CELL_STEP;
            value = BattleGetFloorHeightAtPosition(&pos, 1);
            if (value < best - BATTLE_GRID_WALK_TOLERANCE) {
                bestIndex = i;
                best = value;
            }
        }
        if (best >= pActor->position.vy - BATTLE_GRID_WALK_TOLERANCE) {
            pActor->position.vy = BattleGetFloorHeightAtPosition(&pActor->position, 1);
            break;
        }
        pActor->position.vx -= g_BattleGridNeighbourSteps[bestIndex * 2] * BATTLE_GRID_CELL_STEP;
        pActor->position.vz -= pStepZ[bestIndex * 2] * BATTLE_GRID_CELL_STEP;
        count++;
    } while (count < BATTLE_GRID_WALK_MAX_STEPS);
    p->floorY = BattleGetFloorHeightAtPosition(&p->position, 1);
    p->flags = (p->flags & ~BATTLE_ACTOR_FLAGS_GRID_CELL_MASK) |
               (((BattleGetGridCellAtPosition(&p->position) >> 24) & 3) << 29);
}

extern s32 g_BattleActorDistance2D;
extern s32 g_BattlingAudioManager;
extern void SoundStopAudioManager(s32 arg);
extern void SoundStopAllElements(void);
extern void BattleActorWalkDownGrid(BattleState* p);

#define BATTLE_CAMERA_POS_Y_MIN (-1536)

// The pointer to &g_BattleCameraPos.vy is load-bearing: it is what makes retail keep
// the address in a register and store through it twice. The s32 return keeps reorg
// from stealing the clamp's constant into the beqz delay slot (no value is returned).
s32 func_80071F8C(s32 mode) {
    switch (mode) {
    case 0:
        SoundStopAudioManager(g_BattlingAudioManager);
        SoundStopAllElements();
        break;
    case 1:
        BattleActorWalkDownGrid((u8*)&g_BattleActor1);
        if (D_800987E0 * 0x100 / D_800987E8 > D_800970C4 * 0x100 / D_800970CC) {
            g_BattleMessageIndex = 0x43;
        } else {
            g_BattleMessageIndex = 0x44;
        }
        break;
    case 2:
        BattleActorWalkDownGrid((u8*)&g_BattleActor0);
        break;
    case 3: {
        s32 value = -g_BattleActorDistance2D;
        s32* pCameraY = &g_BattleCameraPos.vy;

        *pCameraY = value;
        if (value < BATTLE_CAMERA_POS_Y_MIN) {
            *pCameraY = BATTLE_CAMERA_POS_Y_MIN;
        }
        break;
    }
    }
}

#define BATTLE_EFFECT_CELL_COUNT 3

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

extern u8 g_BattleInitDone;

void BattleClearInitFlag(void) {
    g_BattleInitDone = 0;
}

extern u8 g_BattleInitScript;
extern u8 g_BattleInputEnabled;
extern u8 g_BattlePlayer1ComControlled;
extern u8 g_BattlePlayer2ComControlled;
extern s32 g_BattleStateStep;

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
    g_BattleEffectCells[0].frames = 1;
    g_BattleEffectCells[1].frames = 1;
    g_BattleEffectCells[2].frames = 1;
    g_BattleActor0.unkE8 = 0;
    g_BattleActor1.unkE8 = 0;
}

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
extern void BattleDrawEffectTile(u8* p0, s32 color, s32 flag);
extern void BattleClearScreen(s32 arg);
extern void BattleReturnToMainLoop(void);
extern void func_8007107C(void);
extern void BattlePlaceEffectByMode(s32 mode);

/* Steps the battle display state machine for the current frame: plays the effect
   sound, places a burst from a random source actor on the effect cells, fades the
   screen and keeps the tutorial message text in step.
   Retail's exact 1080 bytes and instruction stream: every remaining difference
   is an operand the linker resolves to the same address - the switch's table
   (jtbl_ vs .rodata), the y/z stores naming g_BattleEffectCells+4/+8 where
   retail's object names D_800929F8/D_800929FC, the anchor words reached as
   g_BattleActor1+4/+8 where retail names them g_BattleSceneOriginYMirror/
   g_BattleSceneOriginZ, and the cell-walk bound reached as
   g_BattleEffectCells+BATTLE_EFFECT_CELL_COUNT*0x10+0xC where retail names the
   end symbol. The case-1 loop
   needs the base as a mem_addr local (a pointer local lets gcc address the cells
   through it and drops retail's repeated 'lui $at/%hi + addu $at,$at,offset'
   stores) and a byte offset rather than an index (an indexed access gives loop.c
   a second induction pointer, +2 insns: 1096 bytes, 97.53%). The three placement
   stores keep their casts for the same reason: retail re-forms the address from
   the symbol for each of them, and no typed spelling does - the pCell cursor
   (94.63% at 1068) and a typed view of offset + base (95.66% at 1044) both let
   gcc hoist the three source-actor loads above the store group instead of
   interleaving them. Same family as the load-bearing cast store in battling/misc2.
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
            pFrames += sizeof(BattleEffectEmitter) / sizeof(*pFrames);
            offset += sizeof(BattleEffectEmitter);
            pCell++;
        } while ((s32)pFrames < (s32)&g_BattleEffectCells[BATTLE_EFFECT_CELL_COUNT].frames);
        break;
    case 2:
        pState->gaugeValue = 1;
        g_BattleScreenFadeLevel = 0xFF;
        g_BattleStateStep++;
    case 3:
        BattleDrawEffectTile(g_BattleRootOrderingTable, g_BattleScreenFadeLevel, 0);
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
        BattleDrawEffectTile(g_BattleRootOrderingTable, g_BattleScreenFadeLevel, 1);
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
    g_BattleActor1.distance = 0;
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
