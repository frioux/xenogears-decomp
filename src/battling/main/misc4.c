#include "common.h"
#include "psyq/libgte.h"

extern s32 g_BattleMotionSpeedScale;
extern s32 g_BattleBothCombatantsDownCount;
extern s32 g_BattleElapsedFrames;
extern s32 g_BattleRoundNumber;
extern s16 g_BattleCombatant1DownCount;
extern s16 g_BattleCombatant0DownCount;
extern s16 g_BattleMotionSpeedScaleOption;
extern u8 g_BattleTextPadIndex;
extern u8 g_BattleFrameRateLevel;
extern u8 g_BattlePlayer1ComControlled;
extern u8 g_BattlePlayer2ComControlled;
extern u8 g_BattleMotionSpeedLevel;
extern u8 g_BattleAiBehaviorLevel;
typedef struct {
    /* 0x00 */ u8 pad00[0x2C];
    // Tested for zero to pick the scratch slot in BattleResetScratchState.
    /* 0x2C */ s16 flags;
} UnkA884; // Size: unknown

extern s32 g_BattleRoundOverFrames;
extern s32 D_80092668;
extern s32 D_8009266C;
extern s32 D_80092670;
extern s32 D_80092674;
extern s32 g_BattleTransitionCountdown;
extern s32 D_8009284C;
extern s32 g_BattleResultSide;
extern s32 g_BattleCameraTarget;
extern u8* g_BattleEffect;
extern UnkA884* D_8009790C;
extern UnkA884* D_80099028;
extern u8 D_80091150;
extern u8 D_80091151;
extern s32 g_BattleSceneOriginYMirror;
extern s32 g_BattleSceneOriginY;
extern s32 D_8009879C;
extern s32 D_80097080;
extern s32 D_80098680;
extern s32 D_80098684;
extern s32 g_BattleCameraPos;
extern s32 D_80098720;
extern s32 D_80098724;
extern u8 g_BattleStateSlotIndex;
extern s32 g_BattleSavedPositions;
extern s32 D_80092A38;
extern s32 D_80092A44;
extern s32 D_80092A48;
extern s32 D_80092A54;
extern s32 D_80092A58;
extern s32 D_80092A64;
extern s32 D_80092A68;
extern s32 D_80099068;
extern s32 D_8009794C;
extern s32 D_80099088;
extern s32 D_8009796C;
extern s32 D_80098798;
extern s32 D_8009707C;
extern s16 D_800987F8;
extern s16 D_800970DC;
extern u8 g_BattleCameraSpinDirection;
extern u8 g_BattleStageLightIndex;
extern u8 g_BattleInputEnabled;
extern u8 D_800928F0;
extern u8 D_80091144;
extern u8 D_80091145;
extern u8 g_BattleFrameSkipCountdown;
extern u8 g_BattleTransitionFlag;
extern u8 g_BattleResultCode;
extern s32 g_BattleCameraAngleStep;
extern s32 g_BattleRoundOver;
extern s32 g_BattlePromptTimer;
extern s32 g_BattleTransitionFrames;
extern s32 g_BattleEffectEntryCount;

extern void BattleEffectInitContext(void);
extern s32 rand(void);
extern void BattleClearSpriteTables();
extern void BattleEffectClearEntrySlots();
extern void BattleEffectClearSlots();
extern void BattleClearTextSpriteEntries();
extern void BattleInitSoundSlots();
extern void func_80074BA4(void);
extern void func_80078F00();
extern void BattleRandomizeCameraPosition(void);
extern void BattleEffectSetSpawnTimer();
extern void BattleClearInitFlag(void);
extern void SystemStringReset();
extern u8 g_BattleMessageWindow[];
extern void func_80074678();
extern void BattleTransformEntryVertex();
extern void BattleEffectSetColor();
extern s32 func_8007CD44();
extern void BattleEffectAddSlotFromVector();
extern void func_8007D7A8();
extern void func_8007DC74();
extern s32 func_80082488();
extern u32 BattleGetGridCellAtPosition();
extern void BattleMovePointInsideRadius();
extern void BattleUpdateControllerActuator();
extern s32 BattleGetScreenMode();
extern void BattleApplyOptionWord(void);
extern void BattleEffectCreateWork(void);
extern void func_8008EBD0();
extern void BattlePlaySoundEffectByIdx();
extern void func_8008ED6C();

// A 32-bit component vector; the actor's motion and edge fields are this wide.
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 w;
} Vec4; // Size: 0x10

typedef struct ActorScript ActorScript;
typedef struct ActorPart ActorPart;
typedef struct Actor Actor;

// Layout recovered from the offsets this unit touches; only those are known.
struct ActorScript {
    /* 0x0 */ u8 basePartIndex;
    /* 0x1 */ u8 partIndex0;
    /* 0x2 */ u8 partIndex1;
    /* 0x3 */ u8 radius;
    /* 0x4 */ s16 vertexIndex0;
    /* 0x6 */ s16 vertexIndex1;
    /* 0x8 */ s16 baseVertexIndex;
    /* 0xA */ u8 padA[0x2];
    // The actor's own speed scale in 8.8, the source of the two scales at 0x15F2/0x15F4.
    /* 0xC */ u16 speedScale;
    /* 0xE */ u8 padE[0x1];
    // The script's step scale, copied to the actor's 0x15F0 by func_80078F00.
    /* 0xF */ u8 stepScale;
    /* 0x10 */ u8 pad10[0x10];
    /* 0x20 */ u16 facing;
    /* 0x22 */ u8 pad22[0x2];
    // The actor's camera rig: func_8007A768 copies these into D_80092668..92674,
    // which func_8007A958 feeds to rcos/rsin to orbit the camera around the actor.
    /* 0x24 */ s16 cameraAngleOffset;
    /* 0x26 */ s16 cameraYOffset;
    // The camera's look-at height: func_8007A958 subtracts it from the actor's y.
    /* 0x28 */ s16 cameraTargetYOffset;
    /* 0x2A */ s16 cameraRadius;
}; // Size: unknown

typedef struct {
    /* 0x0 */ u8 pad00[0x6];
    /* 0x6 */ u16 flags;
    /* 0x8 */ u8 pad08[0x2];
    /* 0xA */ u16 flags2;
} ActorSub;

// Actor pointer at 0x15CC, aimed at the actor's own 0x9CC.

// The rotation block p->pPart->pRotation points at; the reset copies the script's
// facing word into all three of its components.
typedef struct {
    /* 0x00 */ u8 pad00[0x10];
    /* 0x10 */ SVECTOR rotation;
} ActorPartRot; // Size: 0x18

// Layout recovered from the offsets this unit touches; only those are known.
struct ActorPart {
    /* 0x00 */ u8 pad00[0x4];
    /* 0x04 */ ActorPartRot* pRotation;
    /* 0x08 */ u8 pad08[0x2C];
    /* 0x34 */ s32 x;
    /* 0x38 */ s32 y;
    /* 0x3C */ s32 z;
    /* 0x40 */ u8 pad40[0x6];
    // The actor's angle, copied in every frame by BattleActorSetPartTransform.
    /* 0x46 */ s16 angle;
}; // Size: unknown

// One of the actor's sixteen 0x54-byte model records; the reset clears the byte
// at the end of every one of them.
typedef struct {
    /* 0x00 */ u8 pad00[0x51];
    /* 0x51 */ u8 unk51;
    /* 0x52 */ u8 pad52[0x2];
} ActorModelRecord; // Size: 0x54

// One of the eight 0x44-byte slot records at 0x64C (src/battling/main/misc2
// walks the same array); the reset clears its in-use byte and its timer.
typedef struct {
    /* 0x00 */ u8 pad00[0x30];
    /* 0x30 */ u8 inUse;
    /* 0x31 */ u8 pad31[0xF];
    /* 0x40 */ s16 unk40;
    /* 0x42 */ u8 pad42[0x2];
} ActorSlotRecord; // Size: 0x44

// The 0x10-byte block the reset zeroes at p + 0x15D8.
typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
    /* 0x8 */ s16 unk8;
    /* 0xA */ s16 unkA;
    /* 0xC */ s16 unkC;
} UnkStruct1; // Size: 0xE

// One 0x14-byte record per battle slot, the same table src/battling/main/misc3
// reads; the reset clears everything but the pointer and the gauge byte.
typedef struct {
    /* 0x0 */ u32 value;
    /* 0x4 */ u8 percent;
    /* 0x5 */ u8 pad5[0x3];
    // Last frame's value; a change restamps frameStart (misc2 func_800740E4).
    /* 0x8 */ u32 prevValue;
    // The two 12.4 animation frames misc3's HUD draws as FRAME:%d-%d.
    /* 0xC */ u32 frameStart;
    /* 0x10 */ u32 frameCurrent;
} BattleSlotReset; // Size: 0x14

extern BattleSlotReset g_BattleSlotGauges[];

extern u32 D_8006FC74[];
// The two bytes D_8009264C carries; the reset points the actor's 0x84 slot at it
// and sets the flag, which retail reaches by its own name 0x8009264E.
extern u8 D_8009264C[];
extern u8 D_8009264E;

extern void BattleResetMessageQueue(Actor* p);
extern void BattleActorResetUnk15D8(UnkStruct1* p);
extern void BattleInitSemiTransQuadPacket(Actor* p);
extern void BattleInitActorWork(Actor* p);

struct Actor {
    /* 0x000 */ s32 x;
    /* 0x004 */ s32 y;
    /* 0x008 */ s32 z;
    /* 0x00C */ u8 pad0C[0x4];
    /* 0x010 */ s32 vx;
    /* 0x014 */ s32 vy;
    /* 0x018 */ s32 vz;
    /* 0x01C */ u8 pad1C[0x4];
    /* 0x020 */ s32 pushVx;
    /* 0x024 */ s32 pushVy;
    /* 0x028 */ s32 pushVz;
    /* 0x02C */ u8 pad2C[0x20];
    /* 0x04C */ u8 modelIndex;
    /* 0x04D */ u8 pad4D;
    /* 0x04E */ u8 prevPartIndex;
    // Per-frame part animation advance in 1/16-frame units; src/battling/main/misc2
    // (func_80074BA4) recomputes it from the part's timing table, the reset sets 0x10.
    /* 0x04F */ u8 animSpeed;
    /* 0x050 */ u8 pad50[0x2];
    /* 0x052 */ u8 animSpeedBonus;
    /* 0x053 */ u8 pad53;
    // Added to the script's camera angle and fed to rcos/rsin (func_8007A958).
    /* 0x054 */ s32 angle;
    /* 0x058 */ u8 pad58[0x4];
    /* 0x05C */ ActorPart* pPart;
    /* 0x060 */ u8 pad60[0xC];
    // The position x,y,z,pad copied here before the frame's movement
    // (func_8007A344); func_80078920 takes the angle between two of them.
    /* 0x06C */ Vec4 prevPosition;
    /* 0x07C */ u8 pad7C[0x8];
    /* 0x084 */ u8* pActionEntry;
    /* 0x088 */ u8 pad88[0x10];
    /* 0x098 */ s32 stepForward;
    /* 0x09C */ s32 stepBack;
    /* 0x0A0 */ u8 padA0[0x8];
    /* 0x0A8 */ s32 unkA8;
    /* 0x0AC */ u8 padAC[0x4];
    /* 0x0B0 */ s32 floorY;
    /* 0x0B4 */ s16 gaugeValue;
    /* 0x0B6 */ s16 gaugePoints;
    /* 0x0B8 */ s16 prevGaugeValue;
    /* 0x0BA */ s16 gaugeLevel;
    /* 0x0BC */ s16 gaugeMax;
    /* 0x0BE */ s16 breakThresholdOffset;
    /* 0x0C0 */ u8 gaugeWidth;
    /* 0x0C1 */ u8 prevGaugeWidth;
    // Frames of hit flash left, drained 0x10 at a time (src/battling/main/misc3).
    /* 0x0C2 */ u8 flashTimer;
    /* 0x0C3 */ u8 actionPhaseTimer;
    /* 0x0C4 */ u8 state;
    /* 0x0C5 */ u8 actionPhase;
    /* 0x0C6 */ u8 prevState;
    /* 0x0C7 */ u8 padC7;
    // Frames left in the current step; the same field misc3 counts down per frame.
    /* 0x0C8 */ s16 moveEndTimer;
    /* 0x0CA */ s16 moveTimer;
    /* 0x0CC */ s16 unkCC;
    /* 0x0CE */ s16 unkCE;
    /* 0x0D0 */ u32 flags;
    /* 0x0D4 */ u32 stateFlags;
    /* 0x0D8 */ u8 padD8[0x10];
    /* 0x0E8 */ s32 stateTimer;
    /* 0x0EC */ u8 padEC[0x8];
    /* 0x0F4 */ s32 unkF4;
    /* 0x0F8 */ ActorModelRecord unkF8[16];
    /* 0x638 */ u8 pad638[0x14];
    /* 0x64C */ ActorSlotRecord unk64C[8];
    /* 0x86C */ u8 pad86C[0x90];
    /* 0x8FC */ ActorScript* pScript;
    /* 0x900 */ u8 pad900[0xA];
    // Bits set per actor type (func_80084C88 stores one per typeId); func_8007920C
    // tests bit 0 before spawning the ground effect, misc3 bits 1 and 2.
    /* 0x90A */ u8 typeFlags;
    /* 0x90B */ u8 timer;
    /* 0x90C */ u8 pad90C[0x4];
    // Frames left before the gauge sound repeats (misc3 BattleUpdateSlotGaugeDisplay).
    /* 0x910 */ u8 soundTimer;
    /* 0x911 */ u8 pad911[0x3];
    // Counts down after an action entry was rejected; while it runs no action is
    // entered (src/battling/main/misc3).
    /* 0x914 */ s16 actionWaitTimer;
    // Frames the current move keeps being re-entered; the round reset clears it
    // (misc3 sets 0x28 when a move begins and decrements it per frame).
    /* 0x916 */ s16 moveReentryTimer;
    /* 0x918 */ u8 pad918[0x14];
    // Two world points whose y words bound the actor vertically; func_80078920
    // compares them with the other actor's before pushing the two apart.
    /* 0x92C */ Vec4 baseVertexPos;
    /* 0x93C */ Vec4 edgeMidPos;
    // Mean of the two points above, word for word (func_8007920C); func_8007A958
    // copies all four words out as the actor's draw position.
    /* 0x94C */ Vec4 midPosition;
    // The 0x93C block as it was before this frame's update: func_8007A344 copies
    // it here, and func_8007920C compares its .y with the new one.
    /* 0x95C */ Vec4 prevEdgeMidPos;
    // Last frame's y of the vertexIndex0 and vertexIndex1 vertices, compared
    // with a 6-unit tolerance in func_8007920C.
    /* 0x96C */ s16 prevVertex1Y;
    /* 0x96E */ s16 prevVertex0Y;
    // Gauge damage applied by this frame's update, then cleared (misc3 func_80077038).
    /* 0x970 */ s32 gaugeDamage;
    /* 0x974 */ u8 pad974[0x24];
    /* 0x998 */ s16 animFrame;
    /* 0x99A */ s16 animFrameStep;
    /* 0x99C */ u8 pad99C[0x27];
    /* 0x9C3 */ u8 actionIndex;
    /* 0x9C4 */ u8 pad9C4[0xC08];
    /* 0x15CC */ ActorSub* pState;
    /* 0x15D0 */ u8 pad15D0[0x4];
    // The colour the effects this actor spawns are tinted with: func_8007920C
    // passes it to BattleEffectSetColor, which takes a Color4.
    /* 0x15D4 */ CVECTOR effectColor;
    /* 0x15D8 */ UnkStruct1 unk15D8;
    /* 0x15E6 */ u8 pad15E6[0xA];
    /* 0x15F0 */ s16 stepScale;
    // 0x15F4 times the battle-wide g_BattleMotionSpeedScale: the scale the
    // per-frame step and part animation actually use (set by misc3
    // BattleUpdateSlotGaugeDisplay).
    /* 0x15F2 */ s16 speedScale;
    // The actor's own scale, copied from the script byte at 0xC.
    /* 0x15F4 */ s16 baseSpeedScale;
    /* 0x15F6 */ s16 unk15F6;
    /* 0x15F8 */ s16 unk15F8;
    /* 0x15FA */ u8 pad15FA[0x5A];
    /* 0x1654 */ s32 unk1654;
    /* 0x1658 */ s32 unk1658;
    /* 0x165C */ s32 unk165C;
    /* 0x1660 */ s32 unk1660;
    /* 0x1664 */ u8 pad1664[0x4];
    // Moves committed for this actor; the round reset below clears it (misc18
    // reads it as its moveCount).
    /* 0x1668 */ s16 moveCount;
}; // Size: unknown

extern Actor g_BattleActor1;
extern Actor g_BattleActor0;

extern s32 BattleActorSelectEdgeForSegment(Vec4* pFrom, Vec4* pTo, Actor* pEdges, Vec4** ppEdge);

// Pushes two actors apart along the perpendicular of the segment between their
// previous positions when their vertical spans meet.  Gives up entirely once the
// battle has run past 0x300 frames.
INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_80078920);

/* Best attempt: 97.75%. Instruction-for-instruction identical to the original
   (same 256 instructions, same frame, same calls and delay slots); the residual
   is register naming only.  Every remaining difference is an allocator tie: the
   original caches p->edgeMidPos.y in $a2 and q->baseVertexPos.y in $a1 for the overlap test
   (we get $a1/$a2 the other way round), keeps &edge1 in $s6 / &edge2 in $s7 (we
   are swapped), and reuses $s6 for the angle after &edge1 dies where we reach for
   $s3.  Two levers that were measured and are worth retrying: the function wants a
   NON-VOID return type - with void gcc 2.7.2 steals the second slt into the delay
   slot at 0x80078ee4 and the function comes out 4 bytes short (255 insns); and
   hoisting q->baseVertexPos.y into a variable that is live into the body (reusing the
   angle local, i.e. 'angle = q->baseVertexPos.y;' before the test, then using angle for
   the second term) reaches 99.13% - a dedicated local of its own does not (it
   goes back to 97.75%), so the lever is the variable's live range, not its
   existence.  The permuter from this body scored 430 and its best candidate (230)
   was that hoist.  Also note struct Actor's 0x6C is a 4-word vector (retail does
   lw at 0x6C/0x70/0x74), not the VECTOR it used to be declared as, and the two
   points at 0x92C/0x93C are Vec4s whose .y words this function compares.

s32 func_80078920(Actor* p, Actor* q) {
    s32 tmp[4]; // unused: reproduces the original's 0x10-byte frame slot
    Vec4 edge1;
    Vec4 edge2;
    Vec4 mid;
    Vec4 perp;
    Vec4* pEdge1;
    Vec4* pEdge2;
    Vec4* pPrev;
    s32 radiusP;
    s32 radiusQ;
    s32 radiusSum;
    s32 dist;
    s32 dist0;
    s32 dist2;
    s32 angle;
    s32 lenP;
    s32 lenQ;
    s32 lenSum;

    if (D_8009284C >= 0x301) {
        return;
    }
    if ((p->edgeMidPos.y < q->baseVertexPos.y && q->edgeMidPos.y < p->edgeMidPos.y) ||
        (q->baseVertexPos.y < p->baseVertexPos.y && p->baseVertexPos.y < q->edgeMidPos.y) ||
        (q->edgeMidPos.y < p->baseVertexPos.y && p->edgeMidPos.y < q->edgeMidPos.y) ||
        (p->baseVertexPos.y < q->edgeMidPos.y && q->baseVertexPos.y < p->edgeMidPos.y)) {
        mid.x = (p->prevPosition.x + q->prevPosition.x) / 2;
        mid.z = (p->prevPosition.z + q->prevPosition.z) / 2;
        perp.z = (q->prevPosition.x - p->prevPosition.x) + mid.z;
        perp.x = (p->prevPosition.z - q->prevPosition.z) + mid.x;
        radiusP = p->pScript->radius;
        radiusQ = q->pScript->radius;
        BattleActorSelectEdgeForSegment(&mid, &perp, p, &pEdge1);
        BattleActorSelectEdgeForSegment(&mid, &perp, q, &pEdge2);
        edge1 = *pEdge1;
        edge2 = *pEdge2;
        edge1.x -= p->prevPosition.x;
        edge1.y -= p->prevPosition.y;
        edge1.z -= p->prevPosition.z;
        edge2.x -= q->prevPosition.x;
        edge2.y -= q->prevPosition.y;
        edge2.z -= q->prevPosition.z;
        edge1.x += p->x;
        edge1.y += p->y;
        edge1.z += p->z;
        edge2.x += q->x;
        edge2.y += q->y;
        edge2.z += q->z;
        dist = BattleVectorDistance2D(&edge1, &edge2);
        radiusSum = radiusP + radiusQ;
        if (dist < radiusSum) {
            pPrev = &p->prevPosition;
            dist0 = BattleVectorDistance2D(pPrev, &edge1);
            dist2 = BattleVectorDistance2D(pPrev, &edge2);
            if (dist2 < dist0) {
                dist += radiusSum;
            } else {
                dist = radiusSum - dist;
            }
            angle = ratan2(p->prevPosition.x - q->prevPosition.x, p->prevPosition.z - q->prevPosition.z);
            lenP = BattleVectorLength2D(&p->vx);
            lenQ = BattleVectorLength2D(&q->vx);
            lenSum = lenP + lenQ;
            if (lenSum == 0) {
                lenQ = 1;
                lenP = 1;
                lenSum = 2;
            }
            lenP = ((lenP * dist) << 8) / lenSum;
            lenQ = ((lenQ * dist) << 8) / lenSum;
            p->x += (rcos(angle) * lenQ) >> 20;
            p->z += (rsin(angle) * lenQ) >> 20;
            q->x -= (rcos(angle) * lenP) >> 20;
            q->z -= (rsin(angle) * lenP) >> 20;
        }
    }
}
*/

void BattleActorUpdateGroundMovement(Actor* p) {
    s32 tmp[4]; // unused: reproduces the original's 0x10-byte frame slot

    p->floorY = func_80082488(p, 1);
    p->flags = (p->flags & 0x9FFFFFFF) | (((BattleGetGridCellAtPosition(p) >> 24) & 3) << 29);
    if ((p->flags & 0x60000000) == 0x60000000) {
        p->timer = 0xF;
    }
    BattleMovePointInsideRadius(p, &p->vx, 0x3E80);
    p->flags &= 0xFFFBFFFF;
    if (p->floorY < p->y + p->vy) {
        p->flags |= 0x40000;
        if (p->state == 4) {
            if (!(p->stateFlags & 0x10)) {
                func_8008EBD0(p, 0xD, p, 2);
                BattleUpdateControllerActuator(p, 6);
            }
            p->stateFlags |= 0x10;
            if (p->vy >= 0x40) {
                p->vy = -p->vy / 3;
            } else {
                p->vy = 0;
            }
        } else {
            p->vy = 0;
        }
        p->y = p->floorY;
    }
    p->x += p->vx;
    p->y += p->vy;
    p->z += p->vz;
}

void BattleActorSetPartTransform(Actor* p) {
    p->pPart->x = p->x;
    p->pPart->y = p->y;
    p->pPart->z = p->z;
    p->pPart->angle = p->angle;
}

void BattleActorResetUnk15D8(UnkStruct1* p) {
    p->unk0 = 0x100;
    p->unk4 = 0x10;
    p->unk2 = 0;
    p->unk6 = 0;
    p->unk8 = 0;
    p->unkA = 0x30;
    p->unkC = 0x30;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_80078F00);

/* Best attempt: 84.48%. 0x80078F00..0x800790C8 (the two clear loops, the
   constant stores and every call/delay slot) is instruction-identical; the
   residual is the flags/stateFlags block at 0x8007900C. Retail keeps one
   p->stateFlags load in $a1 and applies two masks to it (`and a1,a1,-0xD` then
   `and a1,a1,-4`) with a single store, i.e. the source held the word in a local
   across the p->flags statement, and materialises `| 0x2000000` and `| 0x20000`
   as two separate ors (gcc folds `(x | C1) | C2` into `| 0x2020000` for every
   single-expression spelling tried), so that too is not one expression in the
   original. Also unserialised: `lbu $v0,0xF($s1)` needs retail's load-delay nop
   before the `sh` into 0x15F0.
void func_80078F00(Actor* p) {
    ActorPart* pPart;
    ActorPartRot* pRot;
    ActorScript* pScript;
    u16 face;
    s32 i;

    pPart = p->pPart;
    pScript = p->pScript;
    p->z = 0;
    p->y = 0;
    p->x = 0;
    p->vz = 0;
    p->vy = 0;
    p->vx = 0;
    p->pushVz = 0;
    p->pushVy = 0;
    p->pushVx = 0;
    pRot = pPart->pRotation;
    face = pScript->facing;
    pRot->rotation.vz = face;
    pRot->rotation.vy = face;
    pRot->rotation.vx = face;
    p->stepForward = 0x10;
    p->stepBack = 0x10;
    p->unkA8 = 0x60;
    p->gaugeValue = 0x12C;
    p->prevGaugeValue = 0x12C;
    p->gaugeMax = 0x12C;
    p->breakThresholdOffset = 0x480;
    p->flashTimer = 0xF0;
    p->angle = 0;
    p->state = 0;
    p->actionPhase = 0;
    p->gaugePoints = 0;
    p->gaugeLevel = 0;
    p->soundTimer = 0;
    p->gaugeWidth = 0x7F;
    p->prevGaugeWidth = 0x7F;
    p->gaugeDamage = 0;
    p->actionIndex = 0;
    p->stateTimer = 0;
    p->moveReentryTimer = 0;
    p->moveEndTimer = 0;
    p->moveTimer = 0;
    for (i = 0; i < 8; i++) {
        p->unk64C[i].inUse = 0;
        p->unk64C[i].unk40 = 0;
    }
    for (i = 0xF; i >= 0; i--) {
        p->unkF8[i].unk51 = 0;
    }
    p->unkF4 = 1;
    BattleResetMessageQueue(p);
    p->prevPartIndex = 0xFF;
    p->stateFlags &= ~0xC;
    p->flags = (p->flags & ~0x1000) | 0x2000000 | 0x20000;
    p->unkCE = p->unkCC + 0x800;
    p->stateFlags &= ~3;
    p->flags &= 0xFF7FFFFF;
    p->stepScale = pScript->stepScale;
    p->unk15F6 = 0x100;
    p->unk15F8 = 0;
    p->unk1654 = 0;
    p->unk1658 = 0;
    p->unk165C = 0;
    p->unk1660 = 0;
    p->timer = 0;
    p->flags &= ~4;
    p->flags &= ~2;
    p->flags &= ~0x38;
    p->baseSpeedScale = pScript->speedScale;
    p->speedScale = pScript->speedScale;
    p->stateFlags &= ~0x10;
    p->pActionEntry = D_8009264C;
    D_8009264E = 1;
    g_BattleSlotGauges[(p->flags >> 27) & 1].value = (u32)D_8006FC74;
    g_BattleSlotGauges[(p->flags >> 27) & 1].prevValue = 0;
    g_BattleSlotGauges[(p->flags >> 27) & 1].percent = 0;
    g_BattleSlotGauges[(p->flags >> 27) & 1].frameStart = 0;
    g_BattleSlotGauges[(p->flags >> 27) & 1].frameCurrent = 0;
    BattleInitActorWork(p);
    BattleActorResetUnk15D8(&p->unk15D8);
    BattleInitSemiTransQuadPacket(p);
    p->actionWaitTimer = 0;
    p->moveCount = 0;
    p->stateFlags &= ~0x40;
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007920C);

/* Best attempt: 98.96%. Only the register names of the state block and
   the trailing p->flags/p->prevState update differ (mask in v1 vs a0).
void func_8007920C(Actor* p) {
    ActorScript* pScript = p->pScript;
    s32 pos[3][4];
    s32 flag;
    s32 state;

    BattleTransformEntryVertex(p, pScript->partIndex0, pScript->vertexIndex0, pos[0]);
    BattleTransformEntryVertex(p, pScript->partIndex1, pScript->vertexIndex1, pos[1]);
    p->edgeMidPos.x = (pos[0][0] + pos[1][0]) / 2;
    p->edgeMidPos.y = (pos[0][1] + pos[1][1]) / 2;
    p->edgeMidPos.z = (pos[0][2] + pos[1][2]) / 2;

    if ((p->flags & 0x80000) && (p->flags & 0x60000000) != 0x20000000 && !(p->typeFlags & 1)) {
        BattleEffectAddSlotFromVector(pos[0], 9);
        BattleEffectAddSlotFromVector(pos[1], 9);
    }

    if (p->timer != 0) {
        if (pos[0][1] + 0x40 > p->floorY - 0x20) {
            if (p->prevVertex0Y + 6 < pos[0][1]) {
                func_8007D7A8(pos[0], 0x20);
            }
        }
        if (pos[1][1] + 0x40 > p->floorY - 0x20) {
            if (p->prevVertex1Y + 6 < pos[1][1]) {
                func_8007D7A8(pos[1], 0x20);
            }
        }
        p->timer--;
    }

    if ((p->flags & 0x60000000) == 0x20000000) {
        func_8007DC74(&p->edgeMidPos, &p->prevEdgeMidPos);
    }

    if (BattleGetScreenMode() != 7 && (p->flags & 0x60000000) == 0x20000000 && p->edgeMidPos.y >= 0x81 &&
        p->prevEdgeMidPos.y < 0x80) {
        BattlePlaySoundEffectByIdx(p, 0x3A, &p->edgeMidPos, 2);
    }

    p->prevVertex0Y = pos[0][1];
    p->prevVertex1Y = pos[1][1];
    BattleTransformEntryVertex(p, pScript->basePartIndex, pScript->baseVertexIndex, &p->baseVertexPos);

    p->baseVertexPos.y -= 0x38;
    p->midPosition.x = (p->edgeMidPos.x + p->baseVertexPos.x) / 2;
    p->midPosition.z = (p->edgeMidPos.z + p->baseVertexPos.z) / 2;
    p->midPosition.y = (p->edgeMidPos.y + p->baseVertexPos.y) / 2;

    if ((p->flags & 0x8000) && p->actionPhase == 0 && (p->state & 1)) {
        BattleEffectSetColor(&p->effectColor);
        flag = (p->flags >> 27) & 1;
        func_8007CD44(flag, &p->baseVertexPos, &p->edgeMidPos, flag);
        if (!(p->flags & 0x10000)) {
            func_8008ED6C(p, 8);
            func_8008EBD0(p, 8, &p->edgeMidPos, 2);
        }
    }

    if (p->actionPhase != 4) {
        if (p->state != 1) {
            p->stateFlags &= ~3;
        } else {
            p->stateFlags = (p->stateFlags & ~3) | 1;
            if (p->flags & 0x8000) {
                p->stateFlags = (p->stateFlags & ~3) | 2;
            }
        }
        if (p->actionPhase == 2) {
            p->stateFlags |= 3;
        }
    }

    state = p->stateFlags & 3;
    if (state != ((p->stateFlags >> 2) & 3)) {
        switch (state) {
        case 0:
        case 3:
            func_8008ED6C(p, 7);
            func_8008ED6C(p, 9);
            break;
        case 1:
            func_8008EBD0(p, 7, p, 2);
            func_8008ED6C(p, 9);
            break;
        case 2:
            func_8008EBD0(p, 9, &p->edgeMidPos, 2);
            func_8008ED6C(p, 7);
            break;
        }
    }

    p->flags = (p->flags & 0xFFFEFFFF) | (((p->flags >> 15) & 1) << 16);
    p->prevState = p->state;
    p->stateFlags = (p->stateFlags & 0xFFFFFFF3) | ((p->stateFlags & 3) << 2);
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_800796B8);

void BattleInitOptionDefaults(void) {
    BattleEffectInitContext();
    BattleEffectCreateWork();
    BattleApplyOptionWord();
    g_BattlePlayer1ComControlled = 1;
    g_BattleMotionSpeedLevel = 3;
    g_BattleMotionSpeedScale = 0x100;
    g_BattlePlayer2ComControlled = 0;
    g_BattleFrameRateLevel = 0;
    g_BattleAiBehaviorLevel = 0;
    g_BattleMotionSpeedScaleOption = 0x100;
}

void BattleNoop(void) {
}

void BattleResetState(void) {
    g_BattleRoundNumber = 0;
    g_BattleCombatant1DownCount = 0;
    g_BattleCombatant0DownCount = 0;
    g_BattleBothCombatantsDownCount = 0;
    g_BattleElapsedFrames = 0;
    g_BattleTextPadIndex = 0;
}

// char return (value unused): retail's `beqz` delay slot holds a nop, and a void
// return lets gcc reorg steal the first insn of the if-body into it.
char BattleResetForNextRound(void) {    g_BattleActor1.unkCC = 0x400;
    g_BattleActor0.unkCC = -0x400;
    func_80078F00(&g_BattleActor1);
    func_80078F00(&g_BattleActor0);
    BattleEffectClearEntrySlots(g_BattleEffect);
    BattleClearSpriteTables();
    BattleEffectClearSlots();
    BattleRandomizeCameraPosition();
    g_BattleCameraSpinDirection = 1;
    g_BattleCameraAngleStep = -1;
    BattleEffectSetSpawnTimer(0);
    g_BattleRoundOver = 0;
    g_BattleRoundOverFrames = 0;
    g_BattleResultSide = 0;
    g_BattlePromptTimer = 0x5A;
    g_BattleTransitionFrames = 0;
    g_BattleEffectEntryCount = 0;
    BattleClearTextSpriteEntries();
    g_BattleResultCode = 0;
    BattleInitSoundSlots();
    BattleClearInitFlag();
    g_BattleInputEnabled = 0;
    D_800928F0 = 0;
    D_80091144 = 0;
    D_80091145 = 0;
    g_BattleFrameSkipCountdown = 0;
    g_BattleRoundNumber++;
    SystemStringReset(g_BattleMessageWindow);
    g_BattleFrameRateLevel = 0;
    if (g_BattleTransitionFlag != 0) {
        switch ((g_BattleRoundNumber - 1) % 5) {
        case 3:
            g_BattleStageLightIndex = 1;
            break;
        case 4:
            g_BattleStageLightIndex = 2;
            break;
        default:
            g_BattleStageLightIndex = 0;
            break;
        }
    }
}

void BattleActorSetUnk15CCFlags(Actor* p) {
    p->pState->flags2 = (p->pState->flags2 & 0xFEFF) | ((p->flags << 6) & 0x100);
    p->pState->flags = (p->pState->flags & 0xDFFF) | ((p->flags >> 2) & 0x2000);
    p->pState->flags = (p->pState->flags & 0xBFFF) | ((p->flags >> 5) & 0x4000);
}

void BattleActorLoadUnk15CCFlags(Actor* p) {
    u32 flag;
    u32 signBit = 0x80000000; // a literal here gives `sll 17; srl 31`, retail has `sll 17; slti 0`
    u32 masked;

    p->flags = (p->flags & ~4) | ((p->pState->flags2 >> 6) & 4);
    p->flags = (p->flags & 0xFFFF7FFF) | ((p->pState->flags << 2) & 0x8000);
    masked = p->flags & 0xFFF7FFFF;
    flag = ((p->pState->flags << 17) & signBit) != 0;
    p->flags = masked | (flag << 19);
}

void BattleResetUnk92640(void) {
    g_BattleRoundOverFrames = 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_80079DF0);

void BattleSaveActorVectors(s32 unk0) {
    if (unk0 < 0xFF) {
        g_BattleTransitionCountdown = unk0 - 2;
        g_BattleStateSlotIndex -= unk0;
    } else {
        g_BattleTransitionCountdown = 0xFF;
    }
    *(VECTOR*)&g_BattleSavedPositions = *(VECTOR*)&g_BattleActor1;
    *(VECTOR*)&D_80092A44 = *(VECTOR*)&g_BattleActor0;
    *(VECTOR*)&D_80092A54 = *(VECTOR*)&D_80099068;
    *(VECTOR*)&D_80092A64 = *(VECTOR*)&D_8009794C;
    D_80092A68 = 0x100;
    D_80092A58 = 0x100;
    D_80092A48 = 0x100;
    D_80092A38 = 0x100;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007A344);

void BattleActorSetModelByTimer(Actor* p) {
    g_BattleMotionSpeedScale = 0x100;
    if (g_BattleTransitionCountdown < 0x78) {
        p->animSpeed = 0x10;
        p->animSpeedBonus = 0;
        p->modelIndex = 0x10;
        p->flags |= 0x400;
    } else {
        p->animSpeed = 0x10;
        p->animSpeedBonus = 0;
        p->modelIndex = 0;
        p->flags |= 0x400;
    }
}

void BattleActorSetModelIndex9(Actor* p) {
    g_BattleMotionSpeedScale = 0x100;
    p->animSpeed = 0x10;
    p->animSpeedBonus = 0;
    p->modelIndex = 9;
    p->flags |= 0x2000400;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007A768);

/* Best attempt: 92.44%. The D_80098680/9867C/98720 block is the only difference.
   Retail loads p->y once into $v1 and reuses it for both the D_80098680 store
   and the '- 0x400' (addiu lands in $v1), while a plain 'p->y' reloads it after
   the global store (gcc will not CSE a load through the parameter across a store
   to a global) and reassociates '(x - 0x400) - D_80092670' into 'x - (D_80092670 + 0x400)'.
   A 's32 y = p->y' local removes the reload and keeps the subtraction grouped
   ('t = y - 0x400; D_80098720 = t - D_80092670;' gives retail's store order and
   addu $v0,$v0,-0x400) but reaches only 92.31%: gcc then also keeps 0x400 out of
   the addu and the register tie moves. Holding y and t in separate locals that
   let gcc see both uses at once drops to 84.83%.
void func_8007A768(Actor* p) {
    ActorScript* pScript = p->pScript;
    s32 r;

    D_80092668 = pScript->cameraAngleOffset;
    D_8009266C = pScript->cameraTargetYOffset;
    D_80092670 = pScript->cameraYOffset;
    D_80092674 = pScript->cameraRadius;
    SetGeomScreen(0x200);
    r = rand();
    D_80098680 = p->y;
    g_BattleCameraTarget = p->x;
    D_80098720 = p->y - 0x400 - D_80092670;
    D_80098684 = p->z;
    g_BattleCameraPos = g_BattleCameraTarget + (((rcos(r) << 2) * D_80092674) >> 12);
    D_80098724 = D_80098684 + (((rsin(r) << 2) * D_80092674) >> 12);
}
*/

void BattleResetScratchState(void) {
    UnkA884* p;
    s32 v;

    BattleClearSpriteTables();
    BattleEffectClearEntrySlots(g_BattleEffect);
    BattleEffectClearSlots();
    BattleClearTextSpriteEntries();
    BattleInitSoundSlots();
    p = (g_BattleResultSide != 0) ? D_8009790C : D_80099028;
    if (p->flags != 0) {
        D_80091150 = 1;
        v = 0x10;
    } else {
        D_80091150 = 2;
        v = -1;
    }
    D_80091151 = v;
    g_BattleSceneOriginYMirror = 0x100;
    g_BattleSceneOriginY = 0x100;
    D_8009879C = 0x100;
    D_80097080 = 0x100;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007A958);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007AC3C);

/* Best attempt: 78.58%. All 120 instructions are present; only two shapes differ.
   (a) Each 16-byte copy: the original groups three word loads then three word
   stores, then a separate fourth load/store with a delay nop; gcc 2.7.2 emits
   2/2/2/2 for a 16-byte block move (the shape BattleSaveActorVectors matches with) and
   2/2 + 1/1 for a 12-byte copy, so the original's copies are not plain VECTOR
   assignments. (b) The original materialises the destination address before the
   source and keeps g_BattleActor1/g_BattleActor0 in t0/a3 across the function; ours
   rematerialises them and picks a0 for the destination.
   Shapes tried: eight plain VECTOR assignments (77.46%), VECTOR* locals for the
   two reused bases (78.58%), Vec3(0xC) copy + separate [3] word store with
   global or pointer bases (52%/50% - gcc sinks all four word stores to the end
   of the function, 576 bytes).
void func_8007AC3C(void) {
    VECTOR* pA = (VECTOR*)&g_BattleActor1;
    VECTOR* pB = (VECTOR*)&g_BattleActor0;

    g_BattleTransitionCountdown = 0x96;
    g_BattleMotionSpeedScale = 0x100;
    SetGeomScreen(0x200);
    *pA = *(VECTOR*)&g_BattleSavedPositions;
    *pB = *(VECTOR*)&D_80092A44;
    *(VECTOR*)&D_80099068 = *(VECTOR*)&D_80092A54;
    *(VECTOR*)&D_8009794C = *(VECTOR*)&D_80092A64;
    *(VECTOR*)&D_80099088 = *(VECTOR*)&D_80099068;
    *(VECTOR*)&D_8009796C = *(VECTOR*)&D_8009794C;
    *(VECTOR*)&D_80098798 = *pA;
    *(VECTOR*)&D_8009707C = *pB;
    func_8007A768(g_BattleResultSide != 0 ? (Actor*)pB : (Actor*)pA);
}
*/


INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007AE10);

void BattleActorInitModelState(Actor* p, u8 unk4C) {
    p->pState = (ActorSub*)((u8*)p + 0x9CC);
    p->animSpeed = 0x10;
    p->modelIndex = unk4C;
    p->animSpeedBonus = 0;
    g_BattleMotionSpeedScale = 0x100;
    func_80074BA4();
    func_80074678(p, p->animFrame, p->animFrameStep);
}
