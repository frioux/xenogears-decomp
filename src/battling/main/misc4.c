#include "common.h"
#include "psyq/libgte.h"
#include "system/controller.h"

// flags bit 18: cleared at the top of every movement update and set when the floor
// stops the actor's fall, i.e. the actor stands on the ground this frame (misc3
// gates its walking velocity on it).
#define BATTLE_ACTOR_FLAG_GROUNDED 0x40000
// flags bits 29-30: the grid cell type BattleGetGridCellAtPosition reports for the
// actor's position, shifted in every frame by BattleActorUpdateGroundMovement.
#define BATTLE_ACTOR_FLAGS_GRID_CELL_MASK 0x60000000
// g_BattleElapsedFrames stops counting here.
#define BATTLE_ELAPSED_FRAMES_MAX 179999
// The fall speed at which a landing in state 4 bounces back instead of stopping.
#define BATTLE_ACTOR_BOUNCE_VY_MIN 0x40
// The pair is only pushed apart while they are this close: farther apart and the
// edges cannot overlap (BattlePushActorsApart reads it as g_BattleActorDistance2D too).
#define BATTLE_ACTOR_SEPARATION_RANGE_MAX 768
// A quarter of the 12-bit angle rcos/rsin/ratan2 work in: the base facing each side
// starts at (misc3's BATTLE_ANGLE_HALF_TURN is the half turn of the same space).
#define BATTLE_ANGLE_QUARTER_TURN 0x400
// g_BattleTransitionCountdown is a byte countdown, so BattleSaveActorVectors
// saturates it at this.
#define BATTLE_TRANSITION_COUNTDOWN_MAX 255

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
extern s32 g_BattleCameraRigAngleOffset;
extern s32 g_BattleCameraRigTargetYOffset;
extern s32 g_BattleCameraRigYOffset;
extern s32 g_BattleCameraRigRadius;
extern s32 g_BattleTransitionCountdown;
extern s32 g_BattleActorDistance2D;
extern s32 g_BattleResultSide;
extern s32 g_BattleCameraTarget;
extern u8* g_BattleEffect;
extern UnkA884* D_8009790C;
extern UnkA884* D_80099028;
extern u8 g_BattleScratchVariantTag;
extern u8 g_BattleScratchVariantValue;
extern s32 g_BattleSceneOriginYMirror;
extern s32 g_BattleSceneOriginY;
extern s32 D_8009879C;
extern s32 D_80097080;
extern s32 g_BattleCameraTargetY;
extern s32 g_BattleCameraTargetZ;
extern s32 g_BattleCameraPos;
extern s32 g_BattleCameraPosY;
extern s32 g_BattleCameraPosZ;
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
extern u8 g_BattleReinitFlag;
extern u8 g_BattleReinitValue;
extern u8 g_BattleFrameSkipCountdown;
extern u8 g_BattleTransitionFlag;
extern u8 g_BattleCameraLerpFrames;
extern u8 g_BattleOptionOn;
extern u8 g_BattleFacingUpdateEnabled;
extern s32 g_BattleDrawFlags;
extern s32 g_BattleMode;
extern char g_StrBattleReplay[];
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
extern void func_80074BA4();
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
extern void BattleSpawnSpriteEntries();
extern void func_8007DC74();
extern s32 BattleGetFloorHeightAtPosition();
extern u32 BattleGetGridCellAtPosition();
extern void BattleMovePointInsideRadius();
extern void BattleUpdateControllerActuator();
extern s32 BattleGetScreenMode();
extern void BattleApplyOptionWord(void);
extern void BattleEffectCreateWork(void);
extern void BattlePlayActorSoundPair();
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
    // The actor's camera rig: func_8007A768 copies these into g_BattleCameraRigAngleOffset..92674,
    // which BattleUpdateCameraPosition feeds to rcos/rsin to orbit the camera around the actor.
    /* 0x24 */ s16 cameraAngleOffset;
    /* 0x26 */ s16 cameraYOffset;
    // The camera's look-at height: BattleUpdateCameraPosition subtracts it from the actor's y.
    /* 0x28 */ s16 cameraTargetYOffset;
    /* 0x2A */ s16 cameraRadius;
}; // Size: unknown

// One 0xC-byte record of the state-slot array at 0x9CC: p->pState and
// p->pNextState point at the two records g_BattleStateSlotIndex selects, one per
// actor.  The two 16-bit words the flag helpers read-modify-write are its 0x6 and
// 0xA fields; BattleUpdateCombatantPair sets the bytes at 0x9 and 0xA with the 0x1000 flag.
typedef struct {
    /* 0x0 */ u8 pad00[0x6];
    /* 0x6 */ u16 flags;
    /* 0x8 */ u8 pad08[0x1];
    /* 0x9 */ u8 unk9;
    /* 0xA */ u8 unkA;
    /* 0xB */ u8 unkB;
} ActorStateSlot; // Size: 0xC

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
// walks the same array); the reset clears its in-use byte and its life.
typedef struct {
    /* 0x00 */ u8 pad00[0x30];
    /* 0x30 */ u8 inUse;
    /* 0x31 */ u8 pad31[0xF];
    /* 0x40 */ s16 life;
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
    /* 0x02C */ u8 pad2C[0x1C];
    // Cleared together with the round's message queues while g_BattleRoundOver is up.
    /* 0x048 */ s32 unk48;
    /* 0x04C */ u8 modelIndex;
    /* 0x04D */ u8 pad4D;
    /* 0x04E */ u8 prevPartIndex;
    // Per-frame part animation advance in 1/16-frame units; src/battling/main/misc2
    // (func_80074BA4) recomputes it from the part's timing table, the reset sets 0x10.
    /* 0x04F */ u8 animSpeed;
    /* 0x050 */ u8 pad50[0x2];
    /* 0x052 */ u8 animSpeedBonus;
    /* 0x053 */ u8 pad53;
    // Added to the script's camera angle and fed to rcos/rsin (BattleUpdateCameraPosition).
    /* 0x054 */ s32 angle;
    /* 0x058 */ u8 pad58[0x4];
    /* 0x05C */ ActorPart* pPart;
    /* 0x060 */ u8 pad60[0xC];
    // The position x,y,z,pad copied here before the frame's movement
    // (BattleUpdateCombatantPair); BattlePushActorsApart takes the angle between two of them.
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
    // The actor's 12-bit base facing for its side: the round reset sets 0x400 for
    // one combatant and -0x400 for the other (misc3's sideBaseAngle).
    /* 0x0CC */ s16 sideBaseAngle;
    // Offset added to every facing this actor computes; derived from sideBaseAngle
    // by func_80076884 and set to sideBaseAngle + 0x800 by the reset below.
    /* 0x0CE */ s16 facingOffset;
    /* 0x0D0 */ u32 flags;
    /* 0x0D4 */ u32 stateFlags;
    /* 0x0D8 */ u8 padD8[0x10];
    /* 0x0E8 */ s32 stateTimer;
    /* 0x0EC */ u8 padEC[0x8];
    /* 0x0F4 */ s32 unkF4;
    /* 0x0F8 */ ActorModelRecord unkF8[16];
    /* 0x638 */ u8 pad638[0x14];
    /* 0x64C */ ActorSlotRecord effectSlots[8];
    /* 0x86C */ u8 pad86C[0x90];
    /* 0x8FC */ ActorScript* pScript;
    /* 0x900 */ u8 pad900[0x9];
    // Selects the record g_BattleResultRecords holds that the result screen prints.
    /* 0x909 */ u8 unk909;
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
    // Two world points whose y words bound the actor vertically; BattlePushActorsApart
    // compares them with the other actor's before pushing the two apart.
    /* 0x92C */ Vec4 baseVertexPos;
    /* 0x93C */ Vec4 edgeMidPos;
    // Mean of the two points above, word for word (func_8007920C); BattleUpdateCameraPosition
    // copies all four words out as the actor's draw position.
    /* 0x94C */ Vec4 midPosition;
    // The 0x93C block as it was before this frame's update: BattleUpdateCombatantPair copies
    // it here, and func_8007920C compares its .y with the new one.
    /* 0x95C */ Vec4 prevEdgeMidPos;
    // Last frame's y of the vertexIndex0 and vertexIndex1 vertices, compared
    // with a 6-unit tolerance in func_8007920C.
    /* 0x96C */ s16 prevVertex1Y;
    /* 0x96E */ s16 prevVertex0Y;
    // Gauge damage applied by this frame's update, then cleared (misc3 BattleUpdateActorStatus).
    /* 0x970 */ s32 gaugeDamage;
    /* 0x974 */ u8 pad974[0x24];
    /* 0x998 */ s16 animFrame;
    /* 0x99A */ s16 animFrameStep;
    /* 0x99C */ u8 pad99C[0x27];
    /* 0x9C3 */ u8 actionIndex;
    /* 0x9C4 */ u8 pad9C4[0x8];
    // One record per state the actor can be put in; g_BattleStateSlotIndex walks it
    // a record at a time as the battle advances.
    /* 0x9CC */ ActorStateSlot stateSlots[256];
    /* 0x15CC */ ActorStateSlot* pState;
    // The record after pState's, the one BattleUpdateCombatantPair sets up for the next state.
    /* 0x15D0 */ ActorStateSlot* pNextState;
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
    // Gauge levels this actor accumulated in each of the two categories; the
    // battle result code compares them with fractions of gaugeMax
    // (misc2 BattleAccumulateGaugePoints, BattleSetResultCode).
    /* 0x1654 */ s32 gaugeLevelsMode1;
    /* 0x1658 */ s32 gaugeLevelsMode2;
    // Counters func_80076884 bumps as it queues messages: message12Count for each
    // message 1 or 2, message3Count for each message 3.
    /* 0x165C */ s32 message3Count;
    /* 0x1660 */ s32 message12Count;
    /* 0x1664 */ u8 pad1664[0x4];
    // Moves committed for this actor; the round reset below clears it (misc18
    // reads it as its moveCount).
    /* 0x1668 */ s16 moveCount;
}; // Size: unknown

extern Actor g_BattleActor1;
extern Actor g_BattleActor0;

extern s32 BattleActorSelectEdgeForSegment(Vec4* pFrom, Vec4* pTo, Actor* pEdges, Vec4** ppEdge);

// Pushes two actors apart along the perpendicular of the segment between their
// previous positions when their vertical spans meet, sharing the push between them
// by the length of their velocity vectors.  Gives up while the pair is farther
// apart than BATTLE_ACTOR_SEPARATION_RANGE_MAX.
s32 BattlePushActorsApart(Actor* p, Actor* q) {
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
    s32 dist0; // reused: holds the first edge distance, then the push angle
    s32 dist2;
    s32 lenP;
    s32 lenQ;
    s32 lenSum;

    if (g_BattleActorDistance2D > BATTLE_ACTOR_SEPARATION_RANGE_MAX) {
        return;
    }
    if ((p->edgeMidPos.y < q->baseVertexPos.y && q->edgeMidPos.y < p->edgeMidPos.y) ||
        (q->baseVertexPos.y < p->baseVertexPos.y && p->baseVertexPos.y < q->edgeMidPos.y) ||
        (q->edgeMidPos.y < p->baseVertexPos.y && p->edgeMidPos.y < q->edgeMidPos.y) ||
        (p->baseVertexPos.y < q->baseVertexPos.y && q->baseVertexPos.y < p->edgeMidPos.y)) {
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
            dist0 = ratan2(p->prevPosition.x - q->prevPosition.x, p->prevPosition.z - q->prevPosition.z);
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
            p->x += (rcos(dist0) * lenQ) >> 20;
            p->z += (rsin(dist0) * lenQ) >> 20;
            q->x -= (rcos(dist0) * lenP) >> 20;
            q->z -= (rsin(dist0) * lenP) >> 20;
        }
    }
}

void BattleActorUpdateGroundMovement(Actor* p) {
    s32 tmp[4]; // unused: reproduces the original's 0x10-byte frame slot

    p->floorY = BattleGetFloorHeightAtPosition(p, 1);
    p->flags = (p->flags & ~BATTLE_ACTOR_FLAGS_GRID_CELL_MASK) |
               (((BattleGetGridCellAtPosition(p) >> 24) & 3) << 29);
    if ((p->flags & BATTLE_ACTOR_FLAGS_GRID_CELL_MASK) == BATTLE_ACTOR_FLAGS_GRID_CELL_MASK) {
        p->timer = 0xF;
    }
    BattleMovePointInsideRadius(p, &p->vx, 0x3E80);
    p->flags &= ~BATTLE_ACTOR_FLAG_GROUNDED;
    if (p->floorY < p->y + p->vy) {
        p->flags |= BATTLE_ACTOR_FLAG_GROUNDED;
        if (p->state == 4) {
            if (!(p->stateFlags & 0x10)) {
                BattlePlayActorSoundPair(p, 0xD, p, 2);
                BattleUpdateControllerActuator(p, 6);
            }
            p->stateFlags |= 0x10;
            if (p->vy >= BATTLE_ACTOR_BOUNCE_VY_MIN) {
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

// ROUND 487: un-parked and re-measured 84.48% / 764 bytes / 191 insns (retail 780 / 195); 0x80078F00..0x800790C8 is still instruction-identical. THREE REAL SOURCE BUGS FOUND AND MEASURED - the retail body has 4 instructions this park does not, and three of them are statements the park is missing:
//    1. retail stores zero to 0x4C (p->modelIndex) and 0xC3 (u8 actionPhaseTimer) in this block; the park has neither statement. Add p->modelIndex = 0; and p->actionPhaseTimer = 0;
//    2. retail materialises the flags update as TWO ors (lui a0,0x2 / or for 0x20000 and lui v1,0x200 / or for 0x2000000) with ONE load and ONE store. (x|C1)|C2 folds for every single-expression spelling, so split it into two statements: p->flags = (p->flags & ~0x1000) | 0x2000000;  then  p->flags |= 0x20000;  ADJACENT - cse then merges the two RMWs into retail's single lw/sw pair.
//    3. retail loads pScript->speedScale (0xC) ONCE and stores it to both 0x15F4 and 0x15F2; the park's two pScript->speedScale statements give two lbu because the intervening stores may alias. Store the field and copy it back: p->baseSpeedScale = pScript->speedScale; p->speedScale = p->baseSpeedScale;  (or a s16 local) - measured, one lhu + two sh, retail's shape.
//    Also: the two p->stateFlags &= statements must be ADJACENT in the source (stateFlags &= ~0xC; then &= ~3;). Separated by the flags/facing statements gcc emits two lw/and/sw groups; adjacent, cse merges them into retail's single lw + two ands + single sw.
//    With all four spellings in place the body measures 83.69% / 768 bytes / 192 insns: the instruction MULTISET then matches retail's exactly except that retail pays a load-delay nop after lbu 0xF(s1) (its sh to 0x15F0 stays adjacent) where ours sinks that store past the 0x1654 zero-stores, i.e. the residual is sched2's placement of one store, not a missing statement. The percentage DROPS because objdiff scores per position; take the statement fixes anyway, they are what retail actually is.
// ROUND 500: SHIPPABLE-BASE BODY, retail's EXACT 780 bytes / 195 insns (was 191). Instruction multiset now matches retail except TWO load-delay nops and one pair of split RMWs; 100 differing lines are register names + statement positions only.
//    NEW FACTS this round, measured:
//    1. pScript->speedScale must be read through a LOCAL (s16 speed = pScript->speedScale; right after p->stepScale) - gcc cannot hoist the lhu past the stores to p, so the two-statement 'p->baseSpeedScale = pScript->speedScale; p->speedScale = p->baseSpeedScale;' form loads it LATE. The local brings retail's early lhu AND restores the load-delay nop after 'lbu 0xF(s1)' (193 -> 195 insns).
//    2. The gauge stores' source order is value, prevValue(0x8), percent(0x4), frameStart, frameCurrent - NOT struct order. Retail really does store 0x8 before 0x4 (asm 0x80079148 sw then 0x80079170 sb). Do not 'fix' it to struct order.
//    3. Do NOT merge the two stateFlags statements into one expression ((p->stateFlags & ~0xC) & ~3 measured 191/764, worse).
//    RESIDUAL, one construct: retail has ONE 'lw a1,0xD4' / 'and ~0xC' ... / 'and ~3' / ONE 'sw a1,0xD4' with the second and and the store placed AFTER the flags block, i.e. the stateFlags pseudo is live across the flags RMWs in $a1. Every spelling tried either merges into a compact group (adjacent statements: 1 lw + 2 and + 1 sw but stored early, 193/772) or emits two load/store pairs (separated statements: 195/780, this body, 80.76%). Getting retail needs the merge AND the late store, which only sched2 can do once the register pressure puts the value in a1. Related: retail's 0xFF7FFFFF mask is in $a2 and its speed in $a0; this body has the mask in $a1.
//    Everything else measured this round: split flags into two statements (required, retail has two ors with lui 0x2/lui 0x200) - correct; modelIndex/actionPhaseTimer zeros present - correct.
/*
void func_80078F00(Actor* p) {
    ActorPart* pPart;
    ActorPartRot* pRot;
    ActorScript* pScript;
    u16 face;
    s16 speed;
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
        p->effectSlots[i].inUse = 0;
        p->effectSlots[i].life = 0;
    }
    for (i = 0xF; i >= 0; i--) {
        p->unkF8[i].unk51 = 0;
    }
    p->unkF4 = 1;
    BattleResetMessageQueue(p);
    p->prevPartIndex = 0xFF;
    p->stateFlags &= ~0xC;
    p->modelIndex = 0;
    p->actionPhaseTimer = 0;
    p->flags = (p->flags & ~0x1000) | 0x2000000;
    p->flags |= 0x20000;
    p->facingOffset = p->sideBaseAngle + 0x800;
    p->stateFlags &= ~3;
    p->flags &= 0xFF7FFFFF;
    p->stepScale = pScript->stepScale;
    speed = pScript->speedScale;
    p->unk15F6 = 0x100;
    p->unk15F8 = 0;
    p->gaugeLevelsMode1 = 0;
    p->gaugeLevelsMode2 = 0;
    p->message3Count = 0;
    p->message12Count = 0;
    p->timer = 0;
    p->flags &= ~4;
    p->flags &= ~2;
    p->flags &= ~0x38;
    p->baseSpeedScale = speed;
    p->speedScale = speed;
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
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_80078F00);




/* Best attempt: 98.96% at retail's EXACT 1196 bytes / 299 insns.  The whole body is
   instruction-identical except the REGISTER NAMES of three adjacent blocks, all from
   one allocator decision: retail puts p->stateFlags in v1 and p->flags in v0 in the
   actionPhase/state block, this body mirrors them, and the tail then follows (retail
   keeps the 0xFFFEFFFF mask in a0 and p->state in a1, ours v1 and a0).  Retail's
   choice is the LOWER priority pick, so its stateFlags pseudo was allocated after the
   flags one, which no spelling of these statements reproduces here.
   Already measured, do not repeat: a 4-minute permuter run (1832 candidates, base
   score 175, best 175, no lead), and a 'u32 flags' copy local for the tail block,
   which coalesces away and changes nothing.
   ROUND 408: re-measured 98.96% / 1196 bytes, both objects' symbol size 0x4AC. Per the
   m-a-park-s-wrong-field lever the residual was re-checked field by field: BOTH bodies
   read 0xD0 (flags), 0xD4 (stateFlags), 0xC4 (state), 0xC5 (actionPhase), 0xC6
   (prevState) and compare the same registers, so this is not a hidden wrong field -
   instructions 1..224 are identical and only the three tail hunks differ by register
   name. The -da dump (build/src/.../misc4.c.i compiled with the unit's cc_flags plus
   -da) gives the tail block as "Basic block 42: first insn 490, last 547" with
   dispositions 76/80/85/93/95/100/137/138/141 in $3 and 77/78/79/81/82/86/94/etc in
   $2, and lreg prints no per-qty ref counts for that block, so the margin between the
   two mirrored quartets was not quantified - what IS known is that the pair is a
   block-local allocator priority order and four spellings plus a permuter run have
   not moved it.
   ROUND 424: the diff isolates the ONE decision, and it is not stateFlags but the
   0xFFFEFFFF MASK: cc1 materialises it as 'lui'+'ori' in the join block and reorg
   sinks the lui into the res/sound arms' delay slots; retail gets $a0, this body $v1.
   Everything else follows with no freedom - retail a0=mask, a1=p->state, v0=flags,
   v1=stateFlags read/RMW result and the tail's li -13 also $v1 after the mask dies,
   so retail can emit 'li v1,-13' BEFORE 'sw a0,208' while this body must store the
   mask value first because v1 is still live in it.  i.e. the mask constant's pseudo
   is allocated BEFORE the state lbu there and AFTER it here.  A 'u32 mask' local
   assigned 'mask = 0xFFFEFFFF;' just above the statement re-measured at exactly
   98.96 / 34 differing lines again (the local coalesces into the constant, as the
   'u32 flags' copy local did), so a named mask is not the lever and the const has no
   second use to raise its ref count.  Do not spend another permuter run: this body
   already had one (base=175 best=175) and the residual is a reload/reg-order pick.
*/
/*
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
                BattleSpawnSpriteEntries(pos[0], 0x20);
            }
        }
        if (pos[1][1] + 0x40 > p->floorY - 0x20) {
            if (p->prevVertex1Y + 6 < pos[1][1]) {
                BattleSpawnSpriteEntries(pos[1], 0x20);
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
            BattlePlayActorSoundPair(p, 8, &p->edgeMidPos, 2);
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
            BattlePlayActorSoundPair(p, 7, p, 2);
            func_8008ED6C(p, 9);
            break;
        case 2:
            BattlePlayActorSoundPair(p, 9, &p->edgeMidPos, 2);
            func_8008ED6C(p, 7);
            break;
        }
    }

    p->flags = (p->flags & 0xFFFEFFFF) | (((p->flags >> 15) & 1) << 16);
    p->prevState = p->state;
    p->stateFlags = (p->stateFlags & 0xFFFFFFF3) | ((p->stateFlags & 3) << 2);
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007920C);

extern u8 g_BattleInitDone;
extern s32 g_BattleFacingAngle;
// The camera target's and the camera position's x words, declared a second time as
// arrays: taking the address of a scalar folds back to the symbol at every use, while
// an array decays to a value, which is what makes cc1 hold `&g_BattleCameraTarget` in
// a callee-saved register across the rcos/rsin calls as retail does.
extern s32 g_BattleCameraTargetAddr[] __asm__("g_BattleCameraTarget");
extern s32 g_BattleCameraPosAddr[] __asm__("g_BattleCameraPos");
extern s32 BattleVectorDistance2D(Vec4*, Vec4*);
extern s32 BattleVectorLength2D(Vec4*);

// Aim the camera at the two combatants: the facing angle and their distance are
// published for the rest of the battle code, the camera target is mirrored on their
// midpoint, and the camera orbits to the nearer of the two candidate points on an
// orbit of radius dist*2/3 + 0xC0, moving a fraction of the way each frame.
// Best attempt: 94.16%, 980 bytes / 244 instructions = retail's exact object size.
// The structure, every call, every argument and the frame are retail's; the two
// residuals left are these.
// (a) The two BattleVectorLength2D calls: retail copies call 1's result into s0
//     immediately after that call ('move s0,v0') and puts 'addiu a0,sp,0x20' in the
//     second call's delay slot; this body emits those two insns the other way round,
//     so reorg fills the slot with the copy. Measured identical: lenB as a local vs
//     the call written inline in the compare, and 'lenA > lenB' vs 'lenB < lenA' -
//     the copy placement is reload's, not the spelling's.
// (b) floorY: retail keeps 'floor - 0x100' in s0, the register '&g_BattleCameraPos'
//     dies in; this body uses v0. 'if (posA.y > floorY)' and 'if (floorY < posA.y)'
//     measure identically.
// What closed 84.47 -> 94.16 was the ADDRESS REGISTERS. Retail holds
// '&g_BattleCameraTarget' in s1 across the rcos/rsin calls (lui %hi + addiu, then
// sw 0(s1) and two lw 0(s1)); a scalar extern folds back to the symbol at every use,
// so gcc needs one fewer callee-saved register, the whole s-register set shifts by
// one and every line differs. Declaring the SAME symbols a second time as unbounded
// arrays with an asm label - 'extern s32 g_BattleCameraTargetAddr[]
// __asm__("g_BattleCameraTarget");' then 'pTargetX = g_BattleCameraTargetAddr;' -
// makes the address a value and reproduces retail's seven saved registers and the
// 0x50 frame. The third point's target.x read must stay the PLAIN scalar, because
// retail re-forms it there with lui/lw. The same alias on g_BattleCameraPos gives
// retail's s0 held across BattleMovePointInsideRadius and
// BattleGetFloorHeightAtPosition (before it: three separate lui/addiu and a
// 'lui at' store).
// The third load-bearing shape: 'height = dist + 0x40;' must be its own statement
// and 'posA.y = g_BattleCameraTargetY - height;' must come BEFORE the posA.x
// statement - that is what puts retail's 'addiu v1,s4,0x40' in the bnez delay slot
// (and a second copy after the clamp) and computes targetY - height above the rcos
// call. Inlining '(dist + 0x40)' makes cc1 reassociate to (targetY - 0x40) - dist,
// and putting the y statement after posA.x gives 89.71 with the call above the y.
// Not run: the permuter (the lane rule stops below 99%).
/*
void func_800796B8(Actor* p1, Actor* p2) {
    Vec4 posA;
    Vec4 posB;
    s32 angle;
    s32 dist2d;
    s32 dist;
    s32 radius;
    s32 lenA;
    s32 lenB;
    s32 height;
    s32 floorY;
    s32* pTargetX;
    s32* pCamPos;

    if (g_BattleInitDone != 0) {
        return;
    }
    angle = ratan2(p1->x - p2->x, p1->z - p2->z);
    dist2d = BattleVectorDistance2D((Vec4*)p1, (Vec4*)p2);
    g_BattleFacingAngle = angle;
    g_BattleActorDistance2D = dist2d;
    dist = BattleVectorDistance((Vec4*)p1, (Vec4*)p2);
    radius = dist * 2 / 3 + 0xC0;
    pTargetX = g_BattleCameraTargetAddr;
    *pTargetX = (p1->x + p2->x) / 2;
    g_BattleCameraTargetY = (p1->y + p2->y) / 2 - 0xA0;
    g_BattleCameraTargetZ = (p1->z + p2->z) / 2;

    // The two candidate points on the orbit, seen from the camera. Their y words are
    // never set here - retail reads the frame slot back unset - but the length test
    // below is 2D, so only x and z matter.
    posA.x = *pTargetX + ((rcos(angle - BATTLE_ANGLE_QUARTER_TURN) * radius) >> 12);
    posA.z = g_BattleCameraTargetZ + ((rsin(angle - BATTLE_ANGLE_QUARTER_TURN) * radius) >> 12);
    posB.x = *pTargetX + ((rcos(angle + BATTLE_ANGLE_QUARTER_TURN) * radius) >> 12);
    posB.z = g_BattleCameraTargetZ + ((rsin(angle + BATTLE_ANGLE_QUARTER_TURN) * radius) >> 12);
    posA.x -= g_BattleCameraPos;
    posA.y -= g_BattleCameraPosY;
    posA.z -= g_BattleCameraPosZ;
    posB.x -= g_BattleCameraPos;
    posB.y -= g_BattleCameraPosY;
    posB.z -= g_BattleCameraPosZ;
    lenA = BattleVectorLength2D(&posA);
    lenB = BattleVectorLength2D(&posB);
    if (lenB < lenA) {
        g_BattleCameraAngleStep = BATTLE_ANGLE_QUARTER_TURN;
        g_BattleCameraSpinDirection = 0;
    } else {
        g_BattleCameraAngleStep = -BATTLE_ANGLE_QUARTER_TURN;
        g_BattleCameraSpinDirection = 1;
    }

    // The camera's height over the orbit point follows the pair's separation, and is
    // kept above the ground by one tile.
    dist /= 4;
    if (dist > 0x300) {
        dist = 0x300;
    }
    height = dist + 0x40;
    posA.y = g_BattleCameraTargetY - height;
    posA.x = g_BattleCameraTarget + ((rcos(angle + g_BattleCameraAngleStep) * radius) >> 12);
    posA.z = g_BattleCameraTargetZ + ((rsin(angle + g_BattleCameraAngleStep) * radius) >> 12);
    pCamPos = g_BattleCameraPosAddr;
    posB.x = (posA.x - *pCamPos) / g_BattleCameraLerpFrames;
    posB.z = (posA.z - g_BattleCameraPosZ) / g_BattleCameraLerpFrames;
    BattleMovePointInsideRadius((Vec4*)pCamPos, &posB, 0x3A00);
    *pCamPos += posB.x;
    g_BattleCameraPosZ += posB.z;
    floorY = BattleGetFloorHeightAtPosition((Vec4*)pCamPos, 0) - 0x100;
    if (floorY < posA.y) {
        posA.y = floorY;
    }
    g_BattleCameraPosY += (posA.y - g_BattleCameraPosY) / g_BattleCameraLerpFrames;
    g_BattleCameraLerpFrames = 0x64;
}*/
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
char BattleResetForNextRound(void) {    g_BattleActor1.sideBaseAngle = BATTLE_ANGLE_QUARTER_TURN;
    g_BattleActor0.sideBaseAngle = -BATTLE_ANGLE_QUARTER_TURN;
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
    g_BattleReinitFlag = 0;
    g_BattleReinitValue = 0;
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
    *(u16*)&p->pState->unkA = (*(u16*)&p->pState->unkA & 0xFEFF) | ((p->flags << 6) & 0x100);
    p->pState->flags = (p->pState->flags & 0xDFFF) | ((p->flags >> 2) & 0x2000);
    p->pState->flags = (p->pState->flags & 0xBFFF) | ((p->flags >> 5) & 0x4000);
}

void BattleActorLoadUnk15CCFlags(Actor* p) {
    u32 flag;
    u32 signBit = 0x80000000; // a literal here gives `sll 17; srl 31`, retail has `sll 17; slti 0`
    u32 masked;

    p->flags = (p->flags & ~4) | ((*(u16*)&p->pState->unkA >> 6) & 4);
    p->flags = (p->flags & 0xFFFF7FFF) | ((p->pState->flags << 2) & 0x8000);
    masked = p->flags & 0xFFF7FFFF;
    flag = ((p->pState->flags << 17) & signBit) != 0;
    p->flags = masked | (flag << 19);
}

void BattleResetUnk92640(void) {
    g_BattleRoundOverFrames = 0;
}

extern s32 ControllerIsStateStackFull(void);
extern void ControllerResetState(void);
extern s32 ControllerPopState(void);
extern s32 ControllerGetType(s32);
extern void func_80076884(Actor*);
extern void BattleUpdateRoundOverState(Actor*, Actor*);
extern void BattleUpdateActorStatus(Actor*);
extern void BattleUpdateActorState(Actor*);
extern void BattleActorUpdateVelocity(Actor*);
extern void func_80074BA4(Actor* p);
extern void BattleStartTextScreen(s32);
extern void FontSetPositionsByOffsetClamped(s32, s32);
extern void BattleSetSpriteTimer(s32);
extern void BattleRunActorStateHandler(Actor*);
extern void BattleDispatchWorkState(Actor*);
extern void BattleStepDisplayState(void);

extern void BattleSetTextCursor(s32, s32);
extern void BattleDrawTextLeft(char*);
extern void func_80073644(Actor*);
extern void BattleUpdateSlotGaugeDisplay(Actor*);
extern void ControllerMergeStates(void);
extern void BattleSetScreenMode(s32);
extern void BattleRotateCamera(s32);
extern void BattleActorApplyAnimationCommand(Actor*);
extern void BattleEffectUpdateSlots(void);
extern void BattleEffectQueueLineByType(Vec4*, Vec4*, s32);
extern void BattleRandomizeCameraSpin(void);
extern void func_80075B50(Actor*);
extern void func_8007920C(Actor*);

// Best attempt: 99.96% at retail's EXACT 1068 bytes (0x42c), 267 vs 267 instructions,
// same frame and same saved set.  ONE residual: the register the p1->flags word is
// written through.  Retail does it in place on the register the flags load went to
// (lw v1,0xD0(s1) / and v1,v1,a1 / or v1,v1,v0 / sw v1,0xD0(s1)); this body keeps the
// load and the `and` in v1 (both match) but leaves the `or` in a2 (or a2,v1,v0 /
// sw a2,0xD0(s1)), because the expression is assigned to the scratch local and that
// local's allocno is the frame-rate one, which is in a2.
//
// The scratch-local reuse IS load bearing: with the direct form
//   p1->flags = (p1->flags & ~0x40) | ((g_BattlePlayer1ComControlled & 1) << 6);
// the frame-rate pseudo has one use, sched1 sinks its lbu to the countdown store and
// the whole state-slot block is re-allocated (v0/v1 swap for the pState stores, an
// extra move a0,s1 late instead of early): 96.31%, 268 insns, 1072 bytes.
//
// Measured in round 404, all 1068 bytes / 267 insns unless noted, none better:
//  - in-place compound chain on the same local (value = p1->flags; value &= ~0x40;
//    value |= ...; p1->flags = value;) -> ONE pseudo for the whole chain, so the load
//    and the and land in a2 too: 99.89.  This shape can never match: retail uses TWO
//    registers (a2 for the frame rate, v1 for the chain), so the chain must be split.
//  - own local for the chain, in place (flags = p1->flags; flags &= ~0x40; ...) -> the
//    chain is v1/v1/v1 as in retail, but the frame-rate load sinks again: 96.31%,
//    268 insns (the `flags` local also costs the `move a0,s1` postion).
//  - value = p1->flags; p1->flags = (value & ~0x40) | (...);  -> complementary 99.96:
//    the `or` is v1 (retail's) but the load and the and are a2 (lw a2,208(s1) /
//    and v1,a2,a1).  Same 4 differing lines.
//  - u8 scratch local instead of s32: identical to the direct form (96.31%).
//  - the embedded assignment p1->flags = (value = ...) is identical to this body.
//  - permuter 3-minute run from this body: RESULT PERMUTER_NONE, base 10 best 10,
//    1311 iterations (an earlier 4-minute run from the same body: 1975, also none).
//  - preset screen: the same body is 99.96 under gcc-2.7.2-cdk-psx and 99.96 under
//    gcc-2.6.0-psx as well, so the residual is not a compiler-version question.
// The remaining difference is one local-alloc coalesce decision: retail's chain is a
// single fresh pseudo (load/and/or all in v1) while its frame rate lives in a2; both
// halves are reachable separately (above) and the two together need the frame-rate
// pseudo pinned early, which every spelling that pins it does by making the chain
// reuse that same pseudo.  Parked as a tie, not a spelling.
//
// Round 417, all 1068 bytes / 267 insns unless noted:
//  - the residual is NOT the chain.  Per-combatant chain locals (flags1 = p1->flags;
//    flags1 &= ~0x40; flags1 |= ...; and flags2 for p2) give retail's chain EXACTLY for
//    both words (lw v1,0xD0 / and v1,v1,a1 / or v1,v1,v0 / sw v1, the ~0x40 in a1), but
//    with the chain off `value` the frame-rate local has one reference, its lbu is
//    deferred to the countdown store, lands in v0 (which the slot-address math also
//    wants) and the state-slot block re-allocates: 96.31%, 268 insns.  96.31/268 also
//    with the countdown store moved after the chains, with `u8 value`, and with a
//    `(u8)` cast at the store, so the sink does not depend on statement order.
//  - it is the frame-rate pseudo's register: retail has it in a2 with the lbu at the
//    top, which only happens when `value` has a second reference, and the only second
//    reference this body offers is the chain - which drags the chain back to a2.
//  - third 3-minute permuter run from the 99.96 body: PERMUTER_NONE, base 10 best 10,
//    776 iterations.
//  - sched1 IS ACTIVE here (Default -O2, gcc-2.7.2-psx; it is a MIPS cc1 default, not
//    a preset flag): compiling this unit's .i with and without -fno-schedule-insns
//    gives different code for this function.  Without it the lbu does stay at the top,
//    but the slot-index/frame-rate/arg-copy registers all shift down one, so that is
//    not a match either.  The tie is a pre-register-allocation priority contest.
INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_80079DF0);

/*

// The battle's frame update for the combatant pair: frame skipping, the state-slot
// pair, the gauge/animation refresh, the input pass (one controller-pop state per
// loop, which is where the pad whose START prompt is up is chosen) and the ground,
// edge and effect updates.
void func_80079DF0(Actor* p1, Actor* p2) {
    s32 value;

    if (g_BattleFrameSkipCountdown != 0) {
        g_BattleFrameSkipCountdown--;
        return;
    }

    value = g_BattleFrameRateLevel;
    p1->pState = &p1->stateSlots[g_BattleStateSlotIndex];
    p2->pState = &p2->stateSlots[g_BattleStateSlotIndex];
    p1->pNextState = p1->pState;
    g_BattleStateSlotIndex++;
    p2->pNextState = p2->pState;
    g_BattleFrameSkipCountdown = value;

    // The same scratch local holds both words: retail keeps the flags word in the
    // register the frame rate was loaded into.
    value = (p1->flags & ~0x40) | ((g_BattlePlayer1ComControlled & 1) << 6);
    p1->flags = value;
    p2->flags = (p2->flags & ~0x40) | ((g_BattlePlayer2ComControlled & 1) << 6);

    func_80073644(p1);
    func_80073644(p2);
    BattleUpdateSlotGaugeDisplay(p1);
    BattleUpdateSlotGaugeDisplay(p2);

    g_BattleEffectEntryCount = 0;
    func_80075B50(p1);
    func_80075B50(p2);

    FontSetPositionsByOffsetClamped(0xA, 0x60);
    BattleSetSpriteTimer(0);

    if (g_BattleInputEnabled != 0) {
        if (ControllerIsStateStackFull() != 0) {
            ControllerResetState();
        } else {
            while (ControllerPopState() != 0) {
                if ((((g_C1ButtonStateReleased | g_C2ButtonStateReleased) & CTRL_BTN_START) &&
                     g_BattlePromptTimer < 0x14) ||
                    ControllerGetType(0) == 0 ||
                    (ControllerGetType(1) == 0 && g_BattleMode == 2)) {
                    if ((g_C1ButtonStateReleased & CTRL_BTN_START) || ControllerGetType(0) == 0) {
                        g_BattleTextPadIndex = 0;
                    } else if (p2->flags & 0x40) {
                        // Leaves the pad index alone and skips the facing update below.
                        goto skipFacing;
                    } else {
                        g_BattleTextPadIndex = 1;
                    }
                    if (g_BattleFacingUpdateEnabled == 0) {
                        if (g_BattleMode == 4) {
                            BattleStartTextScreen(2);
                        } else {
                            BattleStartTextScreen(1);
                        }
                    }
skipFacing:
                    ;
                }
                func_80076884(p1);
                func_80076884(p2);
            }
        }
    } else {
        ControllerMergeStates();
    }

    if (g_BattleInputEnabled != 0) {
        if (g_BattleElapsedFrames != BATTLE_ELAPSED_FRAMES_MAX) {
            g_BattleElapsedFrames++;
        }
        if (g_BattleMode == 4) {
            BattleRunActorStateHandler(p2);
        } else {
            BattleDispatchWorkState(p1);
            BattleDispatchWorkState(p2);
        }
    }

    if (g_BattleRoundOver != 0) {
        p1->unk48 = 0;
        p2->unk48 = 0;
        BattleResetMessageQueue(p1);
        BattleResetMessageQueue(p2);
    }

    BattleUpdateRoundOverState(p1, p2);
    BattleUpdateActorStatus(p1);
    BattleUpdateActorStatus(p2);
    FontSetPositionsByOffsetClamped(0x4A, 0);
    BattleUpdateActorState(p1);
    FontSetPositionsByOffsetClamped(0x6A, 0);
    BattleUpdateActorState(p2);
    FontSetPositionsByOffsetClamped(0xA, 0x80);
    BattleActorUpdateVelocity(p1);
    BattleActorUpdateVelocity(p2);
    BattleActorUpdateGroundMovement(p2);
    BattleActorUpdateGroundMovement(p1);
    BattlePushActorsApart(p1, p2);
    BattleActorSetPartTransform(p2);
    BattleActorSetPartTransform(p1);
    BattleStepDisplayState();
    func_80074BA4(p1);
    func_80074BA4(p2);
    BattleActorSetUnk15CCFlags(p1);
    BattleActorSetUnk15CCFlags(p2);
    func_8007920C(p1);
    func_8007920C(p2);
    func_80074678(p1, p1->animFrame, p1->animFrameStep);
    func_80074678(p2, p2->animFrame, p2->animFrameStep);
    BattleEffectUpdateSlots();
    if (g_BattleOptionOn != 0) {
        BattleEffectQueueLineByType(&p1->midPosition, &p2->midPosition, 0x13);
    }
}
*/

void BattleSaveActorVectors(s32 unk0) {
    if (unk0 < BATTLE_TRANSITION_COUNTDOWN_MAX) {
        g_BattleTransitionCountdown = unk0 - 2;
        g_BattleStateSlotIndex -= unk0;
    } else {
        g_BattleTransitionCountdown = BATTLE_TRANSITION_COUNTDOWN_MAX;
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

// One frame of the battle's own update, run on both combatants: it points their
// state-slot pair at this frame's records, hands the input to BattleRotateCamera,
// refreshes the gauges and animation and the two cached vectors the edge code
// compares, and counts the transition down.
void BattleUpdateCombatantPair(Actor* p1, Actor* p2) {
    ActorStateSlot* pSlot1;
    ActorStateSlot* pSlot2;
    u8 slot;
    u8 nextSlot;
    s32 buttons;
    u32 cell;

    slot = g_BattleStateSlotIndex;
    nextSlot = slot + 1;
    pSlot1 = &p1->stateSlots[slot];
    pSlot2 = &p2->stateSlots[slot];
    g_BattleStateSlotIndex = nextSlot;
    p1->pState = pSlot1;
    p2->pState = pSlot2;
    p1->pNextState = &p1->stateSlots[nextSlot];
    p2->pNextState = &p2->stateSlots[nextSlot];

    if (g_BattleTransitionCountdown == BATTLE_TRANSITION_COUNTDOWN_MAX) {
        p1->pNextState->flags |= 0x1000;
        p2->pNextState->flags |= 0x1000;
        p1->pNextState->unkA = 1;
        p2->pNextState->unkA = 1;
        p1->pNextState->unk9 = 1;
        p2->pNextState->unk9 = 1;
    }

    if (g_BattleDrawFlags & 8) {
        BattleSetTextCursor(0x10, 0x10);
        BattleDrawTextLeft(g_StrBattleReplay);
    }

    func_80073644(p1);
    func_80073644(p2);
    BattleUpdateSlotGaugeDisplay(p1);
    BattleUpdateSlotGaugeDisplay(p2);

    p1->prevEdgeMidPos = p1->edgeMidPos;
    p2->prevEdgeMidPos = p2->edgeMidPos;
    p1->prevPosition = *(Vec4*)&p1->x;
    p2->prevPosition = *(Vec4*)&p2->x;

    g_BattleEffectEntryCount = 0;
    if (g_BattleCameraLerpFrames >= 2) {
        g_BattleCameraLerpFrames = 1;
    }

    ControllerMergeStates();
    if (g_BattleTextPadIndex == 1) {
        if (g_C2ButtonStateReleased & CTRL_BTN_CIRCLE) {
            BattleSetScreenMode(8);
        }
        buttons = g_C2ButtonState;
    } else {
        if (g_C1ButtonStateReleased & CTRL_BTN_CIRCLE) {
            BattleSetScreenMode(8);
        }
        buttons = g_C1ButtonState;
    }
    BattleRotateCamera(buttons);

    BattleActorApplyAnimationCommand(p1);
    BattleActorApplyAnimationCommand(p2);
    BattleActorSetPartTransform(p1);
    BattleActorSetPartTransform(p2);

    cell = BattleGetGridCellAtPosition(p1);
    p1->flags = (p1->flags & ~BATTLE_ACTOR_FLAGS_GRID_CELL_MASK) | ((cell & 0x3000000) << 5);
    cell = BattleGetGridCellAtPosition(p2);
    p2->flags = (p2->flags & ~BATTLE_ACTOR_FLAGS_GRID_CELL_MASK) | ((cell & 0x3000000) << 5);

    BattleActorLoadUnk15CCFlags(p1);
    BattleActorLoadUnk15CCFlags(p2);
    func_8007920C(p1);
    func_8007920C(p2);
    func_80075B50(p1);
    func_80075B50(p2);
    BattleEffectUpdateSlots();

    if (g_BattleOptionOn != 0) {
        BattleEffectQueueLineByType(&p1->midPosition, &p2->midPosition, 0x13);
    }
    if (g_BattleTransitionCountdown == BATTLE_TRANSITION_COUNTDOWN_MAX) {
        BattleRandomizeCameraSpin();
    }
    if (g_BattleTransitionCountdown == 0) {
        BattleSetScreenMode(8);
    }
    g_BattleTransitionCountdown--;
}

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

/* Best attempt: 96.90%, 284 bytes (retail is 288; both 71 instructions). The store
   order, the single p->y load and the addiu -0x400 on the y register all match now:
   what produced it was taking the ADDRESS of p->x into a local and loading through it
   ("new_var = &p->x; ... = *new_var;") with the PosY/TargetZ pair grouped in a
   do-while block.  The address-taken pointer is what stops gcc hoisting the p->x load
   above the store to g_BattleCameraTargetY.
   Residual: (a) retail fills the rand() jal delay slot with "move s1,v0" and loads
   p->y after it, this body does the reverse; (b) retail loads p->x into v0 and p->z
   into a1, this body reuses a0 for both.  Nothing else differs.
   Already measured, do not repeat: plain p->y twice (the old 92.44% body), a y local
   with split statements (85.11%), "PosY = y - 0x400 - rigY" with the y local (90.89%,
   the addiu reassociates onto rigYOffset), *(s32*)&p->x spellings (92.15% / 90.82%),
   a u32 flags-style copy local, and -O1 (60.86) / -O2 (90.82) on the old body.
   A 4-minute permuter run from the 85.11% body went 625 -> 70 and is where the
   address-taken pointer came from. The permuter has NOT been run from this 96.90 body.
   THIS ROUND (round 408): the object symbol size is retail's EXACT 284 bytes
   (0x11C), not 288 - the "retail is 288" in the note above is stale, so the park is
   at retail's exact size and the residual is only the two items below.
   Re-measured on the 96.90 body: the plain-statement form with no address-taken
   pointer and no do-while is 85.11 (the addiu reassociates onto rigYOffset and the
   PosY/TargetZ pair swaps, as recorded above); moving "g_BattleCameraTarget = *new_var"
   above "y -= 0x400" is byte-identical 96.90. Preset screen on this body:
   gcc-2.7.2-cdk 96.90, gcc-2.6.0 91.31, -O3 96.90, -G0 does not compile - the unit's
   Default preset is right, so do not re-run try_flags.
   ROUND 424: an "s32 x" local for p->x instead of the address-taken pointer is 94.15
   (gcc hoists the p->x load above the TargetY store); the plain statements WITHOUT the
   do-while are 92.15 and there p->z DOES take a1 and move s1,v0 comes before the p->y
   load, but the TargetY/Target stores sink past the rigYOffset and p->z loads.  So
   both halves of the residual are reachable, just not in one body: the do-while is
   what holds the stores in order and it is what hands p->z a0.  A 4-minute permuter
   run from THIS body (2608 iterations) is base=80 best=80, no lead.  Park it.
*/
/*
void func_8007A768(Actor* p) {
    s32* new_var;
    ActorScript* pScript = p->pScript;
    s32 r;
    s32 y;

    g_BattleCameraRigAngleOffset = pScript->cameraAngleOffset;
    g_BattleCameraRigTargetYOffset = pScript->cameraTargetYOffset;
    g_BattleCameraRigYOffset = pScript->cameraYOffset;
    g_BattleCameraRigRadius = pScript->cameraRadius;
    SetGeomScreen(0x200);
    r = rand();
    y = p->y;
    g_BattleCameraTargetY = y;
    new_var = &p->x;
    y -= 0x400;
    g_BattleCameraTarget = *new_var;
 do { g_BattleCameraPosY = y - g_BattleCameraRigYOffset; g_BattleCameraTargetZ = p->z; } while (0);
    g_BattleCameraPos = g_BattleCameraTarget + (((rcos(r) << 2) * g_BattleCameraRigRadius) >> 12);
    g_BattleCameraPosZ = g_BattleCameraTargetZ + (((rsin(r) << 2) * g_BattleCameraRigRadius) >> 12);
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007A768);

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
        g_BattleScratchVariantTag = 1;
        v = 0x10;
    } else {
        g_BattleScratchVariantTag = 2;
        v = -1;
    }
    g_BattleScratchVariantValue = v;
    g_BattleSceneOriginYMirror = 0x100;
    g_BattleSceneOriginY = 0x100;
    D_8009879C = 0x100;
    D_80097080 = 0x100;
}

// Set while the battle's camera debug display is up.
extern s32 g_BattleCameraDebugEnabled;

// The camera tuner's four debug read-outs (the offsets func_8007A768 loads).
extern char g_StrCameraAngleFormat[];
extern char g_StrCameraRefFormat[];
extern char g_StrCameraCamFormat[];
extern char g_StrCameraLenFormat[];

extern s32 BattleVectorDistance(Vec4*, Vec4*);
extern void BattleStepCameraTargetTowardVector(Vec4*, s32);
extern void BattleMoveCameraPosTowardVector(Vec4*, s32);
extern void func_800379C8(char*, s32);

// Places the battle camera on the actor's midpoint, at the orbit the tuner's
// offsets give, then closes the last stretch two units a frame until the camera
// is within 0x200 of the actor.  While the debug display flag is up the d-pad and
// the shoulder buttons move the orbit offsets and print all four of them.
void BattleUpdateCameraPosition(Actor* p) {
    Vec4* pCam;
    Vec4 pos;
    u16 buttons;

    if (g_BattleCameraDebugEnabled != 0) {
        buttons = g_C1ButtonState;
        if (buttons & CTRL_BTN_UP) {
            g_BattleCameraRigTargetYOffset += 4;
        }
        if (buttons & CTRL_BTN_DOWN) {
            g_BattleCameraRigTargetYOffset -= 4;
        }
        if (buttons & CTRL_BTN_TRIANGLE) {
            g_BattleCameraRigYOffset += 4;
        }
        if (buttons & CTRL_BTN_CROSS) {
            g_BattleCameraRigYOffset -= 4;
        }
        if (buttons & CTRL_BTN_RIGHT) {
            g_BattleCameraRigAngleOffset -= 0x20;
        }
        if (buttons & CTRL_BTN_LEFT) {
            g_BattleCameraRigAngleOffset += 0x20;
        }
        if (buttons & CTRL_BTN_R1) {
            g_BattleCameraRigRadius -= 0x10;
        }
        if (buttons & CTRL_BTN_R2) {
            g_BattleCameraRigRadius += 0x10;
        }
        func_800379C8(g_StrCameraAngleFormat, g_BattleCameraRigAngleOffset & 0xFFF);
        func_800379C8(g_StrCameraRefFormat, g_BattleCameraRigTargetYOffset);
        func_800379C8(g_StrCameraCamFormat, g_BattleCameraRigYOffset);
        func_800379C8(g_StrCameraLenFormat, g_BattleCameraRigRadius);
    }

    pos = p->midPosition;
    pos.y = p->y - g_BattleCameraRigTargetYOffset;
    BattleStepCameraTargetTowardVector(&pos, 8);
    pos.y = p->y - g_BattleCameraRigYOffset;
    pos.x = p->x + ((rcos(p->angle + g_BattleCameraRigAngleOffset) * g_BattleCameraRigRadius) >> 12);
    pos.z = p->z + ((rsin(p->angle + g_BattleCameraRigAngleOffset) * g_BattleCameraRigRadius) >> 12);
    BattleMoveCameraPosTowardVector(&pos, 0x10);

    pCam = (Vec4*)&g_BattleCameraPos;
    while (BattleVectorDistance(pCam, (Vec4*)p) < 0x200) {
        g_BattleCameraPosY -= 2;
        pCam->x -= 2;
    }
}

extern void func_8007A768(Actor* p);

// Best attempt: 88.89%, 472 bytes, 118 instructions (retail: 117, 472 bytes).
//    Reached from BattleSetScreenMode: it puts the pair back where the transition
//    saved them, re-seeds the cached camera vectors and points the camera rig at the
//    side the result belongs to.
//    Every 16-byte block move already has retail's exact shape - the four that touch
//    an actor base come out 3 loads / 3 stores / load / nop / store, the other four
//    2/2/2/2 - so the residual is register allocation plus one tail instruction.
//    Retail keeps &g_BattleActor1 in $t0 and &g_BattleActor0 in $a3 across the whole
//    function, uses $a0/$a1 as the mid copies' destination/source base and $v0/$v1 as
//    their temps; ours puts &g_BattleActor1 in $v1, which pushes the temps to
//    $v0/$a0 and the bases to $a1/$a2. Cause: gcc lets the tail's pActor copy
//    coalesce with the register already holding &g_BattleActor1, so that arm needs no
//    instruction and the tail becomes "beqz v0 / nop / move v1,a3 / jal / move a0,v1"
//    where retail has "bnez v0 / addu a0,a3,zero / addu a0,t0,zero / jal". In retail
//    pActor is therefore a separate pseudo in $a0 (a move in BOTH arms), and the
//    allocator hands the two actor addresses the late registers.
//    Measured, do not repeat: the plain eight VECTOR assignments with the ternary
//    inline 88.89; the same with an Actor* pActor local and if/else 88.89; a VECTOR*
//    local for the D_8009794C source 88.89; the condition spelled "== 0" with the arms
//    swapped 87.01; the two actor copies typed as g_BattleActorN.prevPosition (which
//    is what D_80098798 / D_8009707C are: actor+0x6C) 88.76; VECTOR* locals for both
//    actor bases (the older park) 78.58; a 12-byte copy plus a separate word 52/50.
//    Preset screen on the best body: gcc-2.7.2-cdk 69.84, gcc-2.6.0 88.89, -O3 88.89,
//    so the unit's Default preset is right. A 4-minute permuter run reported
//    PERMUTER_ZERO on a candidate that adds a dead "else if (new_var)" arm and a
//    VECTOR* local for the copy-6 source; porting that local alone reproduces 88.89
//    byte for byte, so the ZERO is a scoring artefact, not a find.
//    Untried lead: levers.md's a-semantically-empty-branch-on - a semantically empty
//    two-arm gate around the tail call is what raises a pointer's allocno priority, so
//    try gating the func_8007A768 call on pActor before spending another lane on the
//    allocator by hand.
//    Evidence for the next agent: the four sources of the first four copies are
//    g_BattleSavedPositions + 0/0x10/0x20/0x30, i.e. a Vec4[4] of saved vectors (the
//    D_80092A38/48/58/68 words BattleSaveActorVectors sets to 0x100 are their .y), and
//    the two destinations at the end are g_BattleActor0.prevPosition /
//    g_BattleActor1.prevPosition filled from each actor's own offset 0.
//    ROUND 433, re-measured from this body: 88.89% / 472 bytes / 118 insns, unchanged.
//    Both remaining leads tried and dead: a semantically empty two-arm gate on pActor
//    around the tail call (if (pActor) func_8007A768(pActor); else func_8007A768(pActor))
//    is 87.35% / 484 bytes / 121 insns - the extra test costs a register and the tail
//    grows by three instructions; and "pActor = &g_BattleActor1; if (side) pActor =
//    &g_BattleActor0;" (default-then-override) is byte-identical to the two-arm if/else,
//    88.89% / 118 insns. Do not retry either. The whole residual is one local-alloc
//    decision - &g_BattleActor1 in $t0 (retail) against $v1 (here) - and every other
//    register in the function follows from it: retail's temps are then $v0/$v1/$a0 with
//    the source in $a1, ours $v0/$a0/$a1 with the source in $a2. So one register, six
//    diff hunks.
//    ROUND 433b: also tried the source-shaped form of the park's own evidence - ONE
//    VECTOR* pSrc and ONE VECTOR* pDst variable reused by all eight copies (which is
//    what retail's reused $a1/$a0 look like) - byte-identical 88.89%: gcc coalesces
//    each pointer assignment into the per-copy address pseudo, so the copies keep
//    their own allocnos. Only a value that is a real computation (BattlePushActorsApart in
//    this unit needed exactly that) survives as one long-lived pseudo.
// ROUND 487: re-measured from the un-parked body: 88.89% / 472 bytes / 118 insns, unchanged.
// Also tried the tail as a call argument, func_8007A768(g_BattleResultSide != 0 ? &g_BattleActor0 : &g_BattleActor1) - byte-identical 88.89, so the argument pseudo coalesces the same way. lever.py's regalloc entries all describe CALLEE-saved pairs under global-alloc; this is one caller-saved pseudo inside a single basic block (local-alloc), so no lever applies. Do not re-brief without a -da/-dl dump of the local-alloc priorities.
// ROUND 500: re-measured 88.89% / 472 bytes / 118 insns (retail 472 / 117), unchanged. No new best; the park below is the four-round body and every lead in the note under it is measured dead.
// 
/*
void func_8007AC3C(void) {
    Actor* pActor;

    g_BattleTransitionCountdown = 150;
    g_BattleMotionSpeedScale = 0x100;
    SetGeomScreen(0x200);
    *(VECTOR*)&g_BattleActor1 = *(VECTOR*)&g_BattleSavedPositions;
    *(VECTOR*)&g_BattleActor0 = *(VECTOR*)&D_80092A44;
    *(VECTOR*)&D_80099068 = *(VECTOR*)&D_80092A54;
    *(VECTOR*)&D_8009794C = *(VECTOR*)&D_80092A64;
    *(VECTOR*)&D_80099088 = *(VECTOR*)&D_80099068;
    *(VECTOR*)&D_8009796C = *(VECTOR*)&D_8009794C;
    *(VECTOR*)&D_80098798 = *(VECTOR*)&g_BattleActor1;
    *(VECTOR*)&D_8009707C = *(VECTOR*)&g_BattleActor0;
    pActor = &g_BattleActor1;
    if (g_BattleResultSide != 0) {
        pActor = &g_BattleActor0;
    }
    func_8007A768(pActor);
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc4", func_8007AC3C);





extern char g_StrBattleMoveCheck3[];
extern char g_StrBattleResultVictor[];
extern char g_StrBattleResult1PlayerVictory[];
extern char g_StrBattleResultCom1Victory[];
extern char g_StrBattleResultDefeated[];
extern char D_8006FDC8[];
extern char g_StrBattleResultCom2Victory[];
extern char g_StrBattleResultByFormat[];
extern s32 Sprintf(char*, char*, ...);
extern void BattleByteRegTestReg(char*);

// A 0xC-byte record of the table the result screen's value comes from, one per
// actor type; only the first word is read here.
typedef struct {
    /* 0x0 */ s32 value;
    /* 0x4 */ s32 unk4;
    /* 0x8 */ s32 unk8;
} BattleResultRecord; // Size: 0xC

extern BattleResultRecord g_BattleResultRecords[];

// The battle's closing pass for the combatant pair.  It walks the transition
// countdown's two ends (resetting the scratch state at its start, reopening it at
// its end), sets the pad state up for the START prompt, picks the camera rig of the
// side the result belongs to together with the line the debug read-out shows, then
// runs the ground, model and animation updates for both actors.
void BattleUpdateCombatantPairTransition(Actor* p1, Actor* p2) {
    char buf[0x40];
    u16 buttons;
    s32 floorY;

    if (g_BattleTransitionCountdown == 0x95) {
        BattleResetScratchState();
    }
    p1->flags &= ~0x40;
    p2->flags &= ~0x40;
    BattleUpdateSlotGaugeDisplay(p1);
    BattleUpdateSlotGaugeDisplay(p2);
    g_BattleEffectEntryCount = 0;
    ControllerMergeStates();
    if (g_BattleTextPadIndex == 1) {
        buttons = g_C2ButtonStateReleased;
    } else {
        buttons = g_C1ButtonStateReleased;
    }
    if (buttons & CTRL_BTN_CIRCLE) {
        BattleSetScreenMode(2);
    }

    g_BattleTransitionCountdown--;
    if (g_BattleTransitionCountdown == -1) {
        if (g_BattleCameraDebugEnabled == 0 || g_BattleMode == 3) {
            BattleSetScreenMode(2);
        } else {
            p1->flags |= 0x2000000;
            g_BattleTransitionCountdown = 0x95;
            p2->flags |= 0x2000000;
        }
    }
    BattleSetTextCursor(0xA0, 0xA0);

    switch (g_BattleResultSide) {
    case 0:
        BattleUpdateCameraPosition(p1);
        switch (g_BattleMode) {
        case 1:
            BattleByteRegTestReg(g_StrBattleResultVictor);
            break;
        case 2:
            BattleByteRegTestReg(g_StrBattleResult1PlayerVictory);
            break;
        case 3:
            BattleByteRegTestReg(g_StrBattleResultCom1Victory);
            break;
        }
        break;
    case 1:
        BattleUpdateCameraPosition(p2);
        switch (g_BattleMode) {
        case 1:
            BattleByteRegTestReg(g_StrBattleResultDefeated);
            Sprintf(buf, g_StrBattleResultByFormat, g_BattleResultRecords[p2->unk909].value);
            BattleByteRegTestReg(buf);
            break;
        case 2:
            BattleByteRegTestReg(D_8006FDC8);
            break;
        case 3:
            BattleByteRegTestReg(g_StrBattleResultCom2Victory);
            break;
        }
        break;
    case 2:
        BattleByteRegTestReg(g_StrBattleMoveCheck3);
        break;
    }

    floorY = BattleGetFloorHeightAtPosition(&g_BattleCameraPos, 0) - g_BattleCameraRigYOffset;
    if (floorY < g_BattleCameraPosY) {
        g_BattleCameraPosY = floorY;
    }

    p1->unk48 = 0;
    p2->unk48 = 0;
    BattleResetMessageQueue(p1);
    BattleResetMessageQueue(p2);
    BattleUpdateSlotGaugeDisplay(p1);
    BattleUpdateSlotGaugeDisplay(p2);
    g_BattleEffectEntryCount = 0;
    BattleActorUpdateVelocity(p1);
    BattleActorUpdateVelocity(p2);
    BattleActorUpdateGroundMovement(p2);
    BattleActorUpdateGroundMovement(p1);
    BattlePushActorsApart(p1, p2);
    BattleActorSetPartTransform(p2);
    BattleActorSetPartTransform(p1);
    if (p1->flags & 0x800000) {
        BattleActorSetModelByTimer(p2);
        BattleActorSetModelIndex9(p1);
    } else {
        BattleActorSetModelByTimer(p1);
        BattleActorSetModelIndex9(p2);
    }
    func_80074BA4(p1);
    func_80074BA4(p2);
    func_8007920C(p1);
    func_8007920C(p2);
    func_80074678(p1, p1->animFrame, p1->animFrameStep);
    func_80074678(p2, p2->animFrame, p2->animFrameStep);
    BattleEffectUpdateSlots();
}

void BattleActorInitModelState(Actor* p, u8 unk4C) {
    p->pState = (ActorStateSlot*)((u8*)p + 0x9CC);
    p->animSpeed = 0x10;
    p->modelIndex = unk4C;
    p->animSpeedBonus = 0;
    g_BattleMotionSpeedScale = 0x100;
    func_80074BA4(p);
    func_80074678(p, p->animFrame, p->animFrameStep);
}
