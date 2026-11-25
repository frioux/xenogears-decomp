#include "common.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"
#include "system/controller.h"
#include "system/memory.h"

// Denominator of a 0..100 percent (misc2's BATTLE_GAUGE_FRACTION_SCALE is the 0..255 one).
#define BATTLE_PERCENT_SCALE 100
// flags bit 27: the actor is on the second player's side, so it drives gauge record 1,
// g_BattlePlayer2Vibration and the right-hand HUD column.
#define BATTLE_ACTOR_FLAG_PLAYER2 0x8000000
// flags bit 12: set by every modelIndex assignment in this unit, so the bit is the
// actor's "picked a new model" mark.
#define BATTLE_ACTOR_FLAG_MODEL_CHANGE 0x1000
// flags bit 24: func_80076884 keeps it set while the input direction points into
// the half-turn window behind the actor's facing; BattleTryQueueMessage5 refuses
// while it is set.
#define BATTLE_ACTOR_FLAG_FACING_AWAY 0x1000000
// flags bits 20-22: the vertical direction of the step the actor is taking. It is
// cleared and re-set to one of the two directions when BattleUpdateActorMovement
// picks the step model for the goal it walks to, and cleared on the frame after.
#define BATTLE_ACTOR_MOVE_DIR_MASK 0x700000
#define BATTLE_ACTOR_MOVE_DIR_DOWN 0x100000
#define BATTLE_ACTOR_MOVE_DIR_UP 0x200000
// What prevPartIndex holds when the next model has to restart the part animation.
#define BATTLE_PART_INDEX_NONE 0xFF
// Half a turn of the 12-bit angle rcos/rsin/ratan2 work in.
#define BATTLE_ANGLE_HALF_TURN 0x800
// The lowest vy a step may reach: func_80077584 clamps the fall to it.
#define BATTLE_ACTOR_VY_MIN (-0x82)

extern void func_800379C8(char*, ...);
extern char g_StrMessageQueueHeader[];
extern char g_StrMessageQueueValueFormat[];
extern char g_StrMessageQueueFooter[];
extern s32 g_BattleActorDistance2D;
extern u8 g_BattleActionEntryIndices[];
extern u8 D_80091179[];
extern s32 g_BattleHitPointX;
extern s32 g_BattleHitPointZ;

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 w;
} Vec4; // Size: 0x10

// The ring buffer of pending battle messages (32 entries): writeIdx advances on
// a push, readIdx on a pop, and count holds the number queued.
typedef struct {
    /* 0x00 */ u8 buf[0x20];
    /* 0x20 */ u8 writeIdx;
    /* 0x21 */ u8 readIdx;
    /* 0x22 */ u8 count;
} BattleMessageQueue; // Size: 0x23

// The action data p->pActionData points at: a 16-bit scale word at +0 and, from
// +9, one byte per action index (the slot gauge display turns the current
// action's byte into a percentage of the scale).
typedef struct {
    /* 0x00 */ s16 scale;
    /* 0x02 */ u8 unk2[0x7];
    /* 0x09 */ u8 points[0xF];
    // Stored verbatim into the slot gauge's percent byte (misc2 func_800740E4),
    // and message 3 only swaps in its model while this is non-zero.
    /* 0x18 */ u8 percent;
} BattleActionData; // Size: 0x19 (a lower bound)

// One of the actor's eight 0x44-byte effect slots; the layout and the names are
// misc2's, the unit that uses its fields.
typedef struct {
    /* 0x00 */ VECTOR pos;
    /* 0x10 */ VECTOR prevPos;
    /* 0x20 */ SVECTOR velocity;
    /* 0x28 */ SVECTOR direction;
    /* 0x30 */ u8 active;
    /* 0x31 */ u8 scale;
    /* 0x32 */ u8 drawKind;
    /* 0x33 */ u8 mode;
    /* 0x34 */ s32 unk34;
    /* 0x38 */ s32 key;
    /* 0x3C */ s16 percent;
    /* 0x3E */ s16 dirWeight;
    /* 0x40 */ s16 life;
    /* 0x42 */ u8 pad42[0x2];
} BattleEffectSlot; // Size: 0x44

// The word at +0x44 of a slot record, read as flags and as four bytes; the
// layout is misc2's (BattleSlotFlags), the unit that owns the record.
typedef union {
    /* 0x0 */ u32 flags;
    struct {
        /* 0x0 */ u8 unk44;
        /* 0x1 */ u8 state;
        /* 0x2 */ u8 actionId;
        /* 0x3 */ u8 gaugePercent;
    } byte;
} BattleSlotFlags; // Size: 0x4

// One of the actor's sixteen 0x54-byte slot records, misc2's BattleActorSlot: the
// four quad positions a slot animates through and the state machine that walks
// it.
typedef struct {
    /* 0x00 */ Vec4 pos;
    /* 0x10 */ Vec4 prevPos;
    /* 0x20 */ Vec4 endPos;
    /* 0x30 */ Vec4 prevEndPos;
    /* 0x40 */ u8 unk40[0x3];
    /* 0x43 */ u8 partIndex;
    /* 0x44 */ BattleSlotFlags slotFlags;
    /* 0x48 */ u8 unk48;
    /* 0x4C */ s32 unk4C;
    /* 0x50 */ s32 pActionEntry;
} BattleActorSlot; // Size: 0x54

// A battle participant as this unit drives it. Only the fields this unit
// touches are named; 0x166A is a lower bound on the real size (other battle
// units reach further).
typedef struct BattleActor {
    /* 0x0000 */ Vec4 position;
    // The three VECTOR triples below carry the Psy-Q VECTOR's unused fourth word at
    // 0x1C/0x2C/0x3C: nothing in any binary accesses those, so they are pad.
    /* 0x0010 */ s32 vx;
    /* 0x0014 */ s32 vy;
    /* 0x0018 */ s32 vz;
    /* 0x001C */ u8 pad1C[0x4];
    /* 0x0020 */ s32 pushVx;
    /* 0x0024 */ s32 pushVy;
    /* 0x0028 */ s32 pushVz;
    /* 0x002C */ u8 pad2C[0x4];
    /* 0x0030 */ s32 steerVx;
    /* 0x0034 */ s32 steerVy;
    /* 0x0038 */ s32 steerVz;
    /* 0x003C */ u8 pad3C[0x4];
    /* 0x0040 */ s32 stepDistance;
    /* 0x0044 */ s32 steerSpeed;
    /* 0x0048 */ s32 distance;
    /* 0x004C */ u8 modelIndex;
    /* 0x004D */ u8 pad4D;
    /* 0x004E */ u8 prevPartIndex;
    /* 0x004F */ u8 animSpeed;
    /* 0x0050 */ s16 lastFrame;
    // Added to animSpeed when the part animation advances (misc2 func_80074BA4);
    // the movement sets it from the distance and the speed scale.
    /* 0x0052 */ u8 animSpeedBonus;
    /* 0x0053 */ u8 pad53;
    /* 0x0054 */ s32 angle;
    /* 0x0058 */ s32 targetAngle;
    // The actor's model part (misc4's struct ActorPart): its BattleActorSetPartTransform
    // copies the actor's position and angle into it every frame.
    /* 0x005C */ void* pPart;
    // The draw object func_80084C88 creates with BattleCreateObject and links into
    // the part list; func_80072858 writes its local matrix (misc10's pObject).
    /* 0x0060 */ u8* pObject;
    /* 0x0064 */ u8 pad64[0x8];
    /* 0x006C */ Vec4 prevPosition;
    // The part timing table indexed by partIndex: a frame limit and a per-frame
    // step per part (misc2's BattlePartTiming), copied from the resource block by
    // misc10's actor init.
    /* 0x007C */ void* pPartTimings;
    /* 0x0080 */ u8* pActionEntries;
    /* 0x0084 */ u8* pActionEntry;
    // Ramped 0..0x100 deflection of one input direction each: func_80076884
    // charges the direction a 0xF000 flag bit or the stick axis selects and
    // decays the other three; (right - left, up - down) give distance and
    // targetAngle.
    /* 0x0088 */ s32 inputLeft;
    /* 0x008C */ s32 inputUp;
    /* 0x0090 */ s32 inputRight;
    /* 0x0094 */ s32 inputDown;
    // Per-frame charge and decay applied to those four ramps (func_80076884).
    /* 0x0098 */ s32 inputRampStep;
    /* 0x009C */ s32 inputDecayStep;
    /* 0x00A0 */ u8 unkA0[0x8];
    // misc4's round reset stores 0x60 here; read nowhere.
    /* 0x00A8 */ s32 unkA8;
    /* 0x00AC */ u8 padAC[0x4];
    /* 0x00B0 */ s32 floorY;
    /* 0x00B4 */ s16 gaugeValue;
    /* 0x00B6 */ s16 gaugePoints;
    /* 0x00B8 */ s16 prevGaugeValue;
    /* 0x00BA */ s16 gaugeLevel;
    /* 0x00BC */ s16 gaugeMax;
    /* 0x00BE */ s16 breakThresholdOffset;
    /* 0x00C0 */ u8 gaugeWidth;
    /* 0x00C1 */ u8 prevGaugeWidth;
    /* 0x00C2 */ u8 flashTimer;
    /* 0x00C3 */ u8 actionPhaseTimer;
    /* 0x00C4 */ u8 state;
    /* 0x00C5 */ u8 actionPhase;
    /* 0x00C6 */ u8 prevState;
    /* 0x00C7 */ u8 padC7;
    // Frames left in the current step: func_80075B50 sets it from the move
    // record when the step is taken, BattleUpdateActorStatus counts it down, and at zero
    // the move's frame count is cleared for the stop animation (0x300000).
    /* 0x00C8 */ s16 moveEndTimer;
    /* 0x00CA */ s16 moveTimer;
    // A 12-bit angle pair: facingOffset is always derived from sideBaseAngle (its
    // negated value when the camera spin direction flips, itself otherwise, or
    // sideBaseAngle + 0x800 in misc4's round reset, which sets sideBaseAngle to
    // -0x400/0x400 for the two sides) and func_80076884 adds facingOffset to the
    // facings it computes.
    /* 0x00CC */ u16 sideBaseAngle;
    /* 0x00CE */ s16 facingOffset;
    /* 0x00D0 */ u32 flags;
    /* 0x00D4 */ u32 stateFlags;
    /* 0x00D8 */ struct BattleActor* pTarget;
    /* 0x00DC */ u8 unkDC[0xC];
    /* 0x00E8 */ s32 stateTimer;
    /* 0x00EC */ u8 unkEC[0x4];
    /* 0x00F0 */ s16 moveFrameCount;
    // func_80072858 reads it as a signed halfword and prints it in the battle
    // information screen; nothing writes it, so what it counts is unknown.
    /* 0x00F2 */ s16 unkF2;
    // misc4's round reset stores 1 here; read nowhere.
    /* 0x00F4 */ s32 unkF4;
    /* 0x00F8 */ u8 padF8[0x4];
    /* 0x00FC */ s32 angleOffset;
    /* 0x0100 */ s32 movePhase;
    /* 0x0104 */ BattleActorSlot slots[16];
    /* 0x0644 */ s32 gaugePercent;
    /* 0x0648 */ s32 facingAngle;
    /* 0x064C */ BattleEffectSlot effectSlots[8];
    /* 0x086C */ u8 unk86C[0x88];
    // The distance from the actor to the nearest active effect slot and that slot,
    // recorded every frame by func_80073644 (0x10000 when none is active); the state
    // handlers read the target's distance to tell whether it stands in an effect.
    /* 0x08F4 */ u32 nearestEffectDistance;
    /* 0x08F8 */ BattleEffectSlot* pNearestEffect;
    // The model script block func_80084C88 stores the resource record's +0x10 here:
    // its scale word (+0xC), step scale byte (+0xF) and a word at +0x20 are read.
    /* 0x08FC */ void* pScript;
    // One script offset per model index, the table right after the header: the part
    // animation func_80074678 indexes it with modelIndex to find the model's entry
    // list in pScript.
    /* 0x0900 */ s16* pModelScriptOffsets;
    // The part indices BattleUpdatePartDrawFlags walks, one byte each (misc2);
    // func_80084C88 points it into the script block and copies its count from +0xE.
    /* 0x0904 */ u8* pVisiblePartIndices;
    /* 0x0908 */ u8 visiblePartCount;
    /* 0x0909 */ u8 typeId;
    // One bit per actor type (see src/battling/main/misc4.c): bit 1 makes the
    // actor queue message 5, bit 2 quadruples the gauge drain.
    /* 0x090A */ u8 typeFlags;
    // Set to 0xF while the flags word's 0x60000000 bits read 3 and counted down
    // by misc4's func_8007920C, which spawns the ground effect while it runs.
    /* 0x090B */ u8 timer;
    // The angle held while actionPhase 2 plays: func_80078194 latches the facing
    // target here when the phase starts and copies it into `angle` every frame.
    /* 0x090C */ s16 actionAngle;
    // The model index in effect the last time it was recorded; the model's own
    // animation decides when the actor may start moving.
    /* 0x090E */ u8 savedModelIndex;
    /* 0x090F */ u8 unk90F;
    /* 0x0910 */ u8 soundTimer;
    /* 0x0911 */ u8 actionCount;
    /* 0x0912 */ u8 pad912[0x2];
    // Counts down after an action entry was rejected; while it runs the queue is
    // flushed each frame and no action is entered.
    /* 0x0914 */ s16 actionWaitTimer;
    // Set to 0x28 with stateTimer when a move begins (BattleActorBeginMove) and
    // advanced by the same step as stateTimer in func_80075B50, but decremented
    // per frame instead: whenever it still reads >= 0x26 (the first frames after
    // a move began) BattleUpdateActorMovement re-enters the move.
    /* 0x0916 */ s16 moveReentryTimer;
    // The actor's stage-light intensity, set when a step starts here and drained
    // 0x18 a frame by misc9's light pass (BattleFadeActorLights).
    /* 0x0918 */ u8 lightAlpha;
    /* 0x0919 */ u8 pad919[0x3];
    // The point the hit effect spawned at: func_80075B50 stores the same words it
    // copies into g_BattleEffectPosX/Y/Z here.
    /* 0x091C */ Vec4 effectPosition;
    // One of the two world points whose y words bound the actor; 0x93C is the
    // other (src/battling/main/misc4.c names the pair).
    /* 0x092C */ Vec4 baseVertexPos;
    /* 0x093C */ Vec4 centerPosition;
    // The midpoint of the two bound points above, word for word (misc4's
    // func_8007920C computes it on the way to prevCenterPosition).
    /* 0x094C */ Vec4 midPosition;
    /* 0x095C */ Vec4 prevCenterPosition;
    // The y of the vertex each bound point came from, last frame (misc4's
    // func_8007920C compares them with a 6-unit tolerance).
    /* 0x096C */ s16 prevVertex1Y;
    /* 0x096E */ s16 prevVertex0Y;
    /* 0x0970 */ s32 gaugeDrain;
    // Position of the target slot the current step walks to, copied from the
    // selected record by func_80075B50.
    /* 0x0974 */ Vec4 moveGoalPosition;
    /* 0x0984 */ u8 unk984[0x14];
    // The part animation's current frame and its per-call step, handed to
    // func_80074678 (misc4 BattleActorInitModelState).
    /* 0x0998 */ s16 animFrame;
    /* 0x099A */ s16 animFrameStep;
    // The part animation's frame limit for this part and its 12.4 frame
    // accumulator (misc2's func_80074BA4, which sets both).
    /* 0x099C */ s16 frameLimit;
    /* 0x099E */ s16 frameAccum;
    /* 0x09A0 */ BattleMessageQueue messageQueue;
    /* 0x09C3 */ u8 actionIndex;
    /* 0x09C4 */ u8 unk9C4[0x8];
    // The actor's state record, the block the pState pointer at 0x15CC points at:
    // misc4's BattleActorInitModelState sets pState to this address, misc2 writes the
    // record's position and flags through it (its BattleActorState).
    /* 0x09CC */ u8 stateRecord[0xC00];
    /* 0x15CC */ void* pState;
    // The animation record misc2's BattleActorAnimData names: BattleUpdateCombatantPair swaps its
    // flags word (+6) and its two frame bytes (+9, +0xA) every frame.
    /* 0x15D0 */ void* pAnimData;
    // The effect tint, copied from the script block's +0x10..+0x12 by func_80084C88
    // (misc4 hands it to BattleEffectSetColor).
    /* 0x15D4 */ CVECTOR effectColor;
    /* 0x15D8 */ u8 unk15D8[0x10];
    /* 0x15E8 */ s16 unk15E8;
    /* 0x15EA */ s16 unk15EA;
    // Read as two 16-bit words by misc2's BattleComputeScaledFieldDelta.
    /* 0x15EC */ s16 unk15EC;
    /* 0x15EE */ s16 unk15EE;
    // The actor's step scale, copied from its script's byte at +0xF: it multiplies
    // the distance into stepDistance.
    /* 0x15F0 */ s16 stepScale;
    /* 0x15F2 */ s16 moveSpeed;
    /* 0x15F4 */ s16 moveSpeedBase;
    // The round reset sets 0x100 here and 0 at the next word (misc4).
    /* 0x15F6 */ s16 unk15F6;
    /* 0x15F8 */ s16 unk15F8;
    /* 0x15FA */ u8 pad15FA[0x2];
    // The per-slot work record the state handlers run on (misc17's and misc18's pWork).
    /* 0x15FC */ void* pWork;
    /* 0x1600 */ BattleActionData* pActionData;
    // The two 0x28-byte semi-transparent flat-textured quad packets
    // BattleInitSemiTransQuadPacket fills, the second a copy of the first.
    /* 0x1604 */ POLY_FT4 quadPackets[2];
    // The gauge levels this actor gained in each of two categories:
    // BattleAccumulateGaugePoints adds the levels it gains to the one its mode
    // argument selects (1 or 2).
    /* 0x1654 */ s32 gaugeLevelsMode1;
    /* 0x1658 */ s32 gaugeLevelsMode2;
    // Counters the input handler bumps as it queues messages: message12Count for
    // each message 1 or 2, message3Count for each message 3. No reader in this
    // overlay.
    /* 0x165C */ s32 message3Count;
    /* 0x1660 */ s32 message12Count;
    // The sound table BattleSetActorSoundTable picks by typeId; misc17 indexes it
    // with a byte index.
    /* 0x1664 */ u8* pSoundTable;
    // Bumped by func_80075B50 every time it commits a move for the actor; the
    // round reset clears it (misc18 reads it as its moveCount).
    /* 0x1668 */ u16 moveCount;
} BattleActor; // Size: 0x166A

s32 BattleActorDeltaUnk15E8(BattleActor* p0, BattleActor* p1) {
    return p1->unk15EA - p0->unk15E8;
}

void BattleNoop2(void) {
}

// 92.31% (308 vs retail's 312 bytes, one instruction short): the whole body lines up - frame 0x40, three saved regs, both divides, both 0xFFF roundings, the mult/mflo order - except three residuals. (1) gcc gives $s0 to ey and $s1 to ex where retail has ex in $s0 (every downstream mult operand is swapped with them). (2) retail's tail negates its last operand: slt,(dist+r),along / xori 1 / NEGU / and, while every plain '&' spelling here compiles to a bare and (one instruction short). Probed c1-c3, b1-b6, f1-f5: '&', '&&', '& -cond', 'cond - 1', '& !cond', a mask temp negated, hit-then-test - the only shapes that invent the negu turn the along test into a branch, so it looks like retail's source masked the comparison value itself. (3) the along<0 test is scheduled one block earlier (bltz before the subu) than retail's (bgez after it). Tried: 'r * r' (85.58, 320 bytes - the wrong operand, retail loads vSq.vz), inline vs a diff temp (85.58 / 92.31), 'diff' temp + vSq.vz is the best, (along >= 0) & ... & ... (91.28, emits nor/srl instead of bgez), operand-swapped & (86.92).
/*
s32 func_80075750(s32 x1, s32 y1, s32 x2, s32 y2, s32 x3, s32 y3, s32 r) {
    VECTOR v;
    VECTOR vSq;
    s32 dist;
    s32 nx;
    s32 ny;
    s32 along;
    s32 cross;
    s32 ey;
    s32 ex;
    s32 diff;

    v.vx = x2 - x1;
    v.vy = y2 - y1;
    v.vz = r;
    ex = x3 - x1;
    ey = y3 - y1;
    Square0(&v, &vSq);
    dist = SquareRoot0(vSq.vx + vSq.vy);
    if (dist == 0) {
        return 0;
    }
    nx = (v.vx << 12) / dist;
    ny = (v.vy << 12) / dist;
    along = (ex * nx + ey * ny) / 4096;
    cross = (ey * nx - ex * ny) / 4096;
    diff = cross * cross - vSq.vz;
    if (along < 0) {
        return 0;
    }
    return (diff < 0) & (dist + r >= along);
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80075750);



INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80075888);

/* 95.65% (460 vs retail's 452 bytes): the instruction stream is IDENTICAL - the
   two divides, both 0xFFF roundings, the mult/mflo order, both delta blocks, the
   SquareRoot0 hit-point pass and the two g_BattleHitPointX/58 stores all line up. The
   only difference is register allocation: retail needs 7 saved registers
   (s0=ex, s1=ey, s2=nx, s3=ny, s4=x1, s5=y1, s6=r) and this body needs 8
   (along also gets s0, pushing y1 to s6, r to s7 and growing the frame 0x50 ->
   0x58). Cause: gcc coalesces `along` with the quotient, so the merged pseudo
   crosses the hit-point SquareRoot0 call and must be call-saved; retail kept
   the pre-divide value in $v1 (a call-clobbered temp) and only the quotient in
   s0, reusing ex's register after ex dies.
   Tried: along/cross declared in both orders; both roundings written after both
   divides; the explicit `if (x < 0) x += 0xFFF; x >>= 12;` spelling instead of
   `/ 4096` (identical code either way); `- r * r` vs `- vSq.vz` (vSq.vz is
   retail's, and is what this body uses). Permuter: 2095 iterations, base=454;
   its only better candidate (190) copies ny into a temporary and then computes
   `hy = t * t` after that temporary has been overwritten by the hit-point pass
   - wrong code, discarded.
// The closest approach of the segment (x1,y1)-(x2,y2) to the circle at
// (x3,y3) with radius r, in 12-bit fixed point: returns 1 when the segment
// passes through the circle and stores that point in g_BattleHitPointX/g_BattleHitPointZ.
s32 func_80075888(s32 x1, s32 y1, s32 x2, s32 y2, s32 x3, s32 y3, s32 r) {
    VECTOR v;
    VECTOR vSq;
    s32 ex;
    s32 ey;
    s32 dist;
    s32 nx;
    s32 ny;
    s32 along;
    s32 cross;
    s32 delta;
    s32 hit;
    s32 t;

    v.vx = x2 - x1;
    v.vy = y2 - y1;
    v.vz = r;
    ex = x3 - x1;
    ey = y3 - y1;
    Square0(&v, &vSq);
    dist = SquareRoot0(vSq.vx + vSq.vy);
    if (dist == 0) {
        return 0;
    }
    nx = (v.vx << 12) / dist;
    ny = (v.vy << 12) / dist;
    along = ex * nx + ey * ny;
    if (along < 0) {
        along += 0xFFF;
    }
    along >>= 12;
    cross = ey * nx - ex * ny;
    if (cross < 0) {
        cross += 0xFFF;
    }
    cross >>= 12;
    delta = cross * cross - vSq.vz;
    hit = delta < 0;
    if (along < 0) {
        hit = 0;
    }
    if (dist + r < along) {
        hit = 0;
    }
    if (hit) {
        if (delta < 0) {
            delta = -delta;
        }
        t = along - SquareRoot0(delta);
        g_BattleHitPointX = x1 + (t * nx) / 4096;
        g_BattleHitPointZ = y1 + (t * ny) / 4096;
    }
    return hit;
}
*/


extern s32 func_80075750(s32 x1, s32 y1, s32 x2, s32 y2, s32 x3, s32 y3, s32 r);

s32 BattleActorHitTestCircle(BattleActor* p, s32 x, s32 z, s32 r) {
    s32 hit;

    g_BattleHitPointX = x;
    g_BattleHitPointZ = z;
    hit = func_80075750(p->vx, p->vz, p->position.x, p->position.z, x, z, r);
    if (hit) {
        hit = TRUE;
        goto done;
    }
    hit = func_80075750(p->steerVx, p->steerVz, p->pushVx, p->pushVz, x, z, r);
    if (hit) {
        hit = TRUE;
        goto done;
    }
    hit = func_80075750(p->position.x, p->position.z, p->pushVx, p->pushVz, x, z, r);
    if (hit) {
        hit = TRUE;
        goto done;
    }
    hit = func_80075750(p->vx, p->vz, p->steerVx, p->steerVz, x, z, r) != 0;
done:
    return hit;
}


extern s32 func_80075888(s32 x1, s32 y1, s32 x2, s32 y2, s32 x3, s32 y3, s32 r);
extern void BattleCommitSlotStates(BattleActor* p);
extern s32 g_BattleEffectEntryCount;
extern Vec4 g_BattleEffectPosX;

// 96.42% (2124 bytes = retail's exact size, 528 vs 528 instructions, 164 differing
// lines). Everything but the callee-saved hand-out is retail's: both loops (8
// effect slots, then 16 slot records), the calls, both interpolation divides, the
// ratan2 behind/facing test, the sound pairs, the four-word position copies and the
// tail. Retail holds the effect-slot pointer in $s2, the slot byte-offset giv in
// $s4 and the percent/amount family in $s0; this body has pointer $s0, offset $s2,
// amount $s4. p $s1, i $s3, bestAmount $s5, hitCount $s6, magic $s7, actionId $fp
// and the 0x88 frame all match retail.
// PROVEN a pinned hand-out rather than a missing spelling (round 447): cc1 -da/-dl
// on build/src/battling/main/misc3.c.i gives 42 allocnos, processing order
// 278 85 231 81 88 103 193 352 84 226 83 72 106 234 281 82 207 230 279 76 120 248
// 102 74 134 262 294 127 255 308 301 73 86 168 77 75 379 376 80 79 194 78, and
// dispositions 72 in s1, 74 in s2, 73 in s0, 86 in s2, 77 in s4. The offset (74, 36
// refs / 170 insns) is processed before the pointer (73, 28 / 141) yet cannot take
// $s0 - it lists 16, the register 73 ends up in, in its conflict set - so the
// pointer falls into the lower-numbered $s0 and amount (77, 26 / 165 = 1575) is
// pushed to $s4 under both (1985 and 2647). All three terms are pinned by retail's
// own instruction stream: the ref counts are its instructions, the live range is the
// loop in both bodies, and the mode is SI.
// Screened this round: gcc-2.6.0 95.19, 2.7.2-cdk 94.09, -O3 83.99, -G0 96.42 - the
// Default preset is right and no preset moves it. Also measured this round: amount
// and angle as ONE variable with in-place amount /= 2 and amount *= 2 (the shape
// retail's in-place sra/sll suggested) = 95.41 / 2112 bytes / 525 insns, so retail's
// two are separate pseudos that share $s0; loop 2's amount split into its own
// slotAmount = byte-identical at 96.42, so a per-loop split is not a handle here.
// Rides along regardless: hit is $t0 in retail and $a3 here, retail keeps a li 1 in
// EACH of the two point-selection arms where this body sinks one to the join, and
// three lhu/store order pairs differ.
// Earlier attempts: the arms as if/else-if rather than the current if (!inRange)
// nest; hitCount/center/base copied before bestAmount/actionId; step folded back
// into two amount * 2 / 3; velocity.vz for the second ratan2 argument; the
// stateTimer/moveReentryTimer swap kept in loop 1; and a 4-minute permuter run
// (base 1890, best 1690) whose candidate deleted bestAmount = amount - wrong code.
// 98.38% (2124 bytes = retail's EXACT size, 528 vs 528 instructions, 20 differing
// lines, all of them the two "step added to two globals" sites).
// THE HANDLE THAT BROKE THE PARK WIDE OPEN (round 460): share ONE scratch variable
// for every value that flows into the effect slots' vertical logic - the loop-1
// interpolation `t`, the abs'd dBase, the percent/angle and its in-place halving and
// doubling, the loop-2 interpolation `t`, the loop-2 average `y` - instead of giving
// each a local. That single change moved the callee-saved hand-out from the slot
// POINTER to the amount family ($s0 in retail), and with it 144 of the 164 differing
// lines: 96.42 -> 96.95 at once, then 97.50 / 97.56 / 97.61 from the spelling fixes
// below, then 98.37 and 98.38 from two statement orders. Retail's s0 really does hold
// six different values in disjoint ranges (mflo s0 at the loop-1 divide, move s0,v0 /
// negu s0,s0 for the abs, the in-place sra/sll of the amount, the loop-2 divides, and
// addiu s0,sp,64 for &point later on), which is what a shared variable looks like and
// what the old "pinned allocno" note misread as an unreachable rotation.
// Spelling fixes measured on the way, all byte-neutral except the score:
//  - the three point interpolations are `amount * (base.X - center.X)`, not the other
//    way round: retail's `mult s0,v0` takes the amount first (9 lines);
//  - `hit = 1;` comes BEFORE `point = base;` / `point = center;` (retail's `li t0,0x1`
//    fills the delay slot of the two point-selection arms, 2 lines);
//  - `bestAmount = amount;` comes AFTER `p->moveEndTimer = amount / 3 + 12;`, which is
//    what keeps the `/3` multiply on $s0 in the `beqz amount` delay slot (24 -> 20);
//  - loop 2 updates stateTimer BEFORE moveReentryTimer while loop 1 does the reverse
//    (retail is genuinely inconsistent between the two, 2 differing lines).
// WHAT IS LEFT, and it is scheduling only: retail groups BOTH loads and then BOTH
// stores at each site (`lw v1,232(s1) / lhu a0,2326(s1) / mfhi / subu / addu / addu /
// sw / sh`) while this body lets the second load slide past the mfhi+subu and stores
// in between (`lw / mfhi / subu / lhu / addu / sw / addu / sh`); in loop 2 retail also
// hoists `lhu v1,2326(s1)` six instructions above the moveEndTimer store. Both sites
// are the same construct, both bodies are retail's exact size, and swapping the two
// statements, splitting step per loop, and a 4-minute permuter run (base 865 best 692)
// do not move it - the permuter's winner overwrote stateTimer instead of adding to it,
// i.e. wrong code. Next lever: an explicit local for the two loads, or the permuter.
// Screened before the shared-variable find (so re-screen if it stalls): 2.6.0 95.19,
// 2.7.2-cdk 94.09, -O3 83.99, -G0 96.42 at the old body; Default is right.
// 99.92% (2124 bytes = retail EXACT size, 528 vs 528 insns, 12 differing lines, ALL of them
// in loop 2): loop 1 is now byte-identical where the park below sat at 98.38 with 20 differing lines.
// WHAT CLOSED LOOP 1 (round 473): the two timer updates are NOT `+=` statements at all. Read each
// timer into its own local first (u16 for moveReentryTimer - an s32 destination makes gcc emit lh
// where retail has lhu at both sites - s32 for stateTimer), then store the CSE of a single inline
// division through an IN-PLACE add, i.e. `p->stateTimer = (timer += amount * 2 / 3);`. Three
// measured facts came out of that: the explicit reads give retail`s grouped `lw / lhu` ahead of the
// mfhi (a bare `+=` lets sched2 sink the lhu past the subu), the embedded assignment keeps the
// add in place on the loaded register (`addu v1,v1,v0`, not `addu v0,a1,v1`), and inlining the
// division twice (instead of a `step` local) is what moves the quotient out of a0 into retail`s
// v0 - deleting the `step` local alone took 99.88 -> 99.92 and made loop 1 exact.
// THE TWO LOOPS ARE GENUINELY INCONSISTENT IN RETAIL, and the park body mirrors it: loop 1 emits
// stateTimer BEFORE moveReentryTimer (source: state first, both through locals), loop 2 emits
// moveReentryTimer first with its load hoisted above the moveEndTimer store AND before stateTimer`s
// lw, which only comes out when loop 2`s state update is a plain `p->stateTimer += amount * 6 / 5;`
// with no timer local (with the local both loads hoist to the block head, 99.12 / 22 lines).
// WHAT IS LEFT - one register pair, loop 2 only: retail has reentry in v1 and the quotient in a0
// (`lhu v1,2326` ... `sra a0,t2,1 / subu a0,a0,v0`), this body has reentry a0 and the quotient v1
// (`lhu a0,2326` ... `sra v1,t2,1 / subu v1,v1,v0`); stateTimer is v0 in both and every address,
// load and store is retail`s. So it is a local-alloc hand-out between two caller-saved temps, with
// the instruction multiset, the block layout and the byte count already exact.
// Measured dead on this body: a second `u16 reentry2` for loop 2 (BYTE-IDENTICAL - copy
// propagation folds it, the same trap a-second-pointer-alias-carries-the-reads-retail-names
// warns about), a timer local at loop 2 (99.12), plain `+=` at both loops (99.13), a `step` local
// used by both loops (99.88), swapping loop 1 to state-then-reentry inside the `+=` form (98.37)
// and loop 2 to reentry-then-state (98.37), and the pre-find bodies. Preset screen on this body:
// 2.6.0 96.71, 2.7.2-cdk compile-failed; Default is right. Permuter 3 min -j 2: base=65 best=65,
// 763 iterations, PERMUTER_NONE. Next lever: a spelling that changes the refs/live quotient of
// the loop-2 reentry pseudo (it has 2 refs to the quotient`s 3 yet is allocated first), or `-dl`
// on the two allocnos; do not spend another permuter run.
// Earlier rounds (kept for the levers already burnt): the shared-scratch-variable find that took
// this from 96.42 to 98.38 (one scratch for the loop-1 interpolation, the abs`d dBase and the
// percent halving/doubling), the three point interpolations being `amount * (base.X - center.X)`,
// `hit = 1;` before `point = base;`, `bestAmount = amount;` after the moveEndTimer store, and a
// 4-minute permuter run (base 1890 best 1690) whose candidate deleted bestAmount - wrong code.
/*
s32 func_80075B50(BattleActor* p) {
    BattleEffectSlot* pSlot;
    BattleActorSlot* pRecord;
    Vec4 center;
    Vec4 base;
    Vec4 point;
    Vec4 goal;
    s32 i;
    s32 hit;
    s32 amount;
    s32 actionId;
    s32 hitCount;
    s32 bestAmount;
    s32 dCenter;
    s32 dPos;
    s32 diff;
    u16 reentry;
    s32 timer;

    hitCount = 0;
    center = p->centerPosition;
    base = p->baseVertexPos;
    bestAmount = 0;
    actionId = 0;
    for (i = 0; i < 8; i++) {
        pSlot = &p->pTarget->effectSlots[i];
        if (!pSlot->active) {
            continue;
        }
        hit = 0;
        if (base.y < pSlot->pos.vy && pSlot->pos.vy < center.y) {
            amount = ((pSlot->pos.vy - center.y) << 12) / (base.y - center.y);
            point.x = (((amount * (base.x - center.x))) >> 12) + center.x;
            point.y = (((amount * (base.y - center.y))) >> 12) + center.y;
            point.z = (((amount * (base.z - center.z))) >> 12) + center.z;
            hit = func_80075888(pSlot->prevPos.vx, pSlot->prevPos.vz, pSlot->pos.vx, pSlot->pos.vz, point.x, point.z, ((u8*)p->pScript)[0x13]);
        } else if (pSlot->unk34 < ((u8*)p->pScript)[0x13]) {
            amount = __builtin_abs(base.y - pSlot->prevPos.vy);
            dCenter = __builtin_abs(center.y - pSlot->prevPos.vy);
            dPos = __builtin_abs(pSlot->pos.vy - pSlot->prevPos.vy);
            if (amount < dCenter) {
                if (amount < dPos) {
                    hit = 1;
                    point = base;
                }
            } else if (dCenter < dPos) {
                hit = 1;
                point = center;
            }
        }
        if (!hit) {
            continue;
        }
        p->lightAlpha = 0xFF;
        p->stateFlags |= 0x20;
        amount = pSlot->percent;
        diff = (ratan2(pSlot->velocity.vx, pSlot->velocity.vz) - p->angle) & 0xFFF;
        if ((u32)(diff - 0x601) >= 0x3FF) {
            if (p->flags & 4) {
                BattlePlayActorSoundPair((u8*)p, 0xF, &point, 1);
                amount = amount / 2;
                p->lightAlpha = 0x80;
            } else {
                BattlePlayActorSoundPair((u8*)p, 0xC, &point, 1);
            }
        } else {
            amount = amount * 2;
            actionId = 3;
            BattlePlayActorSoundPair((u8*)p, 0xC, &point, 1);
        }
        timer = p->stateTimer;
        reentry = p->moveReentryTimer;
        p->stateTimer = (timer += amount * 2 / 3);
        p->moveReentryTimer = (reentry += amount * 2 / 3);
        p->effectPosition = point;
        if (amount) {
            hitCount++;
        }
        g_BattleEffectPosX = point;
        g_BattleEffectEntryCount = 0x10;
        p->moveEndTimer = amount / 3 + 12;
        bestAmount = amount;
        goal = point;
        pSlot->active = 0;
        break;
    }
    for (i = 0; i < 16; i++) {
        pRecord = &p->pTarget->slots[i];
        if (pRecord->slotFlags.byte.state != 1) {
            continue;
        }
        if (pRecord->slotFlags.flags & 1) {
            continue;
        }
        hit = 0;
        if (pRecord->slotFlags.flags & 2) {
            if (base.y < pRecord->pos.y && pRecord->pos.y < center.y) {
                amount = ((pRecord->pos.y - center.y) << 12) / (base.y - center.y);
                point.x = (((amount * (base.x - center.x))) >> 12) + center.x;
                point.y = (((amount * (base.y - center.y))) >> 12) + center.y;
                point.z = (((amount * (base.z - center.z))) >> 12) + center.z;
                hit = func_80075888(pRecord->prevPos.x, pRecord->prevPos.z, pRecord->pos.x, pRecord->pos.z, point.x, point.z, ((u8*)p->pScript)[0x13]);
            }
        } else {
            amount = (pRecord->pos.y + pRecord->prevPos.y + pRecord->endPos.y + pRecord->prevEndPos.y) / 4;
            if (base.y < amount && amount < center.y) {
                amount = ((amount - center.y) << 12) / (base.y - center.y);
                point.x = (((amount * (base.x - center.x))) >> 12) + center.x;
                point.y = (((amount * (base.y - center.y))) >> 12) + center.y;
                point.z = (((amount * (base.z - center.z))) >> 12) + center.z;
                hit = BattleActorHitTestCircle((BattleActor*)pRecord, point.x, point.z, ((u8*)p->pScript)[0x13]);
            }
        }
        if (!hit) {
            continue;
        }
        p->pTarget->flags &= ~0x4000000;
        point.x = g_BattleHitPointX;
        point.z = g_BattleHitPointZ;
        if (p->flags & 4) {
            BattlePlayActorSoundPair((u8*)p, 0xF, &point, 1);
            amount = pRecord->slotFlags.byte.gaugePercent >> 1;
            g_BattleEffectPosX = point;
            g_BattleEffectEntryCount = 4;
            p->lightAlpha = 0xC0;
            p->stateFlags |= 0x20;
        } else {
            BattlePlayActorSoundPair((u8*)p, *(s16*)pRecord->unk40, &point, 1);
            BattleEffectAddSlotFromVector(&point, 0);
            g_BattleEffectPosX = point;
            g_BattleEffectEntryCount = 0x10;
            p->lightAlpha = 0xFF;
            p->stateFlags &= ~0x20;
            amount = pRecord->slotFlags.byte.gaugePercent;
        }
        p->moveEndTimer = amount;
        reentry = p->moveReentryTimer;
        p->moveReentryTimer = (reentry += amount * 6 / 5);
        p->stateTimer += amount * 6 / 5;
        p->effectPosition = point;
        if (amount) {
            hitCount++;
        }
        if (bestAmount < amount) {
            bestAmount = amount;
            goal = pRecord->pos;
            actionId = pRecord->slotFlags.byte.actionId;
        }
        BattleCommitSlotStates(p->pTarget);
        break;
    }
    if (hitCount) {
        p->movePhase = actionId + 1;
        p->gaugeDrain = bestAmount;
        p->moveGoalPosition = goal;
        p->moveCount++;
    } else {
        p->movePhase = 0;
    }
    return 0;
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80075B50);





void BattleQueueMessage(BattleActor* p, u8 value) {
    u8 idx;

    if (p->messageQueue.count < sizeof(p->messageQueue.buf)) {
        idx = p->messageQueue.writeIdx;
        p->messageQueue.writeIdx = idx + 1;
        p->messageQueue.buf[idx & (sizeof(p->messageQueue.buf) - 1)] = value;
        p->messageQueue.count++;
    }
}

s32 BattleDequeueMessage(BattleActor* p) {
    u8 idx;
    u8 value;

    if (p->messageQueue.count == 0) {
        return 0;
    }
    idx = p->messageQueue.readIdx++;
    value = p->messageQueue.buf[idx & (sizeof(p->messageQueue.buf) - 1)];
    p->messageQueue.count--;
    return value;
}

void BattleResetMessageQueue(BattleActor* p) {
    p->messageQueue.count = 0;
    p->messageQueue.writeIdx = 0;
    p->messageQueue.readIdx = 0;
    p->actionIndex = 0;
}

void BattlePrintMessageQueue(BattleActor* p) {
    s32 i;
    s32 idx;

    idx = p->messageQueue.readIdx;
    func_800379C8(g_StrMessageQueueHeader);
    for (i = 0; i < p->messageQueue.count; i++) {
        func_800379C8(g_StrMessageQueueValueFormat, p->messageQueue.buf[(idx++) & (sizeof(p->messageQueue.buf) - 1)]);
    }
    func_800379C8(g_StrMessageQueueFooter);
}

extern s32 BattleGetScreenMode(void);
extern void BattleSetTextCursor(s32 a0, s32 a1);
extern s32 g_BattleMotionSpeedScale;
extern u8 g_BattlePlayer2Vibration;
extern u8 g_BattlePlayer1Vibration;
extern u32 g_BattleActionGaugeValues[];
// One 0x14-byte record per battle slot. The draw code in BattleDrawFrameHud reads
// the same record's +0xC and +0x10 words.
typedef struct {
    /* 0x00 */ u32 value;
    /* 0x04 */ u8 percent;
    /* 0x05 */ u8 pad5[0x3];
    // Last value drawn; a change restamps frameStart (misc4 names the same record).
    /* 0x08 */ u32 prevValue;
    // 12.4 animation frames, printed as FRAME:%d-%d: the frame the value last
    // changed at, and the actor's current one (misc2 func_800740E4 writes both).
    /* 0x0C */ s32 frameStart;
    /* 0x10 */ s32 frameCurrent;
} BattleSlotGauge; // Size: 0x14

extern BattleSlotGauge g_BattleSlotGauges[];

void BattleUpdateSlotGaugeDisplay(BattleActor* p) {
    u32 flags;
    s32 prod;
    BattleActionData* pActionData;

    p->stepDistance = 0;
    p->movePhase = 0;
    if (BattleGetScreenMode() != 7) {
        flags = p->flags;
        flags &= 0xFFFF7FFF;
        flags &= ~2;
        p->flags = flags;
    }
    // the do-block keeps the BATTLE_ACTOR_FLAG_PLAYER2 test out of the mult shadow
    do {
        prod = p->moveSpeedBase * g_BattleMotionSpeedScale;
        flags = p->flags;
        flags &= ~0x80000;
        p->flags = flags;
        p->moveSpeed = prod >> 8;
    } while (0);
    flags &= BATTLE_ACTOR_FLAG_PLAYER2;
    if (!flags) {
        BattleSetTextCursor(0x28, 0x28);
    } else {
        BattleSetTextCursor(0xF0, 0x28);
    }
    if (p->actionIndex != 0) {
        g_BattleSlotGauges[((p->flags >> 27) & 1)].value = g_BattleActionGaugeValues[p->actionIndex];
        pActionData = p->pActionData;
        g_BattleSlotGauges[((p->flags >> 27) & 1)].percent = (pActionData->points[p->actionIndex] * pActionData->scale) / BATTLE_PERCENT_SCALE;
    }
}

extern void BattleSetTextScale(s32 color);
extern void BattleDrawTextLeft(char* pStr);
extern s32 g_BattleMode;
extern s32 g_BattleGaugeHighlightValue;
extern char g_StrGaugeValueFormat[];
extern char g_StrGaugePercentFormat[];
extern char g_StrGaugeFrameFormat[];

// The three later gauge reads reach g_BattleSlotGauges with the record offsets folded in
// where retail's object names D_80096FBC/FC4/FC8 -- the same three addresses, so
// the linked bytes are identical and diff_func.sh prints MATCH at retail's 428
// bytes; objdiff stops at 99.72% on that relocation spelling alone.
void BattleDrawFrameHud(BattleActor* p) {
    char buf[0x10];
    s32 h;
    s32 w;
    s32 color;

    h = 0xA;
    w = 0x14;
    if (p->flags & BATTLE_ACTOR_FLAG_PLAYER2) {
        w = 0xB4;
    }
    if (g_BattleMode != 4) {
        h = 0xB2;
    }
    BattleSetTextCursor(w, h);
    Sprintf(buf, g_StrGaugeValueFormat, g_BattleSlotGauges[((p->flags >> 27) & 1)].value);
    if (g_BattleSlotGauges[((p->flags >> 27) & 1)].value == g_BattleGaugeHighlightValue) {
        color = 0xE7;
    } else {
        color = 0x100;
    }
    BattleSetTextScale(color);
    BattleDrawTextLeft(buf);
    BattleSetTextScale(0x100);
    BattleSetTextCursor(w + 0x34, h);
    Sprintf(buf, g_StrGaugePercentFormat, g_BattleSlotGauges[((p->flags >> 27) & 1)].percent);
    BattleDrawTextLeft(buf);
    BattleSetTextCursor(w, h + 0x14);
    h = g_BattleSlotGauges[((p->flags >> 27) & 1)].frameStart >> 4;
    Sprintf(buf, g_StrGaugeFrameFormat, h, g_BattleSlotGauges[((p->flags >> 27) & 1)].frameCurrent >> 4);
    BattleDrawTextLeft(buf);
}

/* non-void return type: as void gcc fills the beq delay slots that retail
   leaves as nop. */
short BattleTryQueueMessage5(BattleActor* p) {
    u32 flags;

    if (g_BattleActorDistance2D >= 0x201) {
        return;
    }
    if (p->actionPhase == 4) {
        return;
    }
    flags = p->flags;
    if (flags & 0x1000000) {
        return;
    }
    if (!(flags & 0x40000)) {
        return;
    }
    if (!(p->pTarget->flags & 0x40000)) {
        return;
    }
    if ((flags & 0x60000000) != 0x20000000 || (p->typeFlags & 2)) {
        BattleResetMessageQueue(p);
        BattleQueueMessage(p, 5);
    }
}

extern u8 g_BattleCameraSpinDirection;
extern s32 ControllerGetType(s32 controllerIndex);
extern u8 ControllerStickToAnalogX(s32 buttonState);
extern u8 ControllerStickToAnalogY(s32 buttonState);

/* 99.43% (1972 bytes = retail's exact size, 491 vs 491 instructions, 22 differing
   lines). Both residuals are register-name ties; nothing structural is left.
   (1) 17 lines, the flags read-modify-write of 0xD0: retail has the mask in $v1
   and the flags word in $a0 and its `lw` of 0x648/0xFC land before
   `sw $a0,0xD0`; this body has the pair the other way round ($a0 mask, $v1
   flags), so its store sinks below those loads. That it is a regno question is
   PROVEN by the -dl dump (cc1 -dl on build/src/battling/main/misc3.c.i): the
   `and`'s constant operand is pseudo 117, created AFTER the flags load's pseudo
   115, because expand forces the constant only once the mem is expanded, and
   local-alloc allocates in regno order - so the load takes the first free
   register ($3) and the mask $4. Retail's mirror needs the mask to carry the
   LOWER regno, which the expander cannot produce for `flags & const`.
   Measured, all identical to this body or worse: a single-use `u32 newFlags =
   p->flags;` and a single-use `u32 clearSpin = ~0x20000;` local (both folded
   back into the constant by copy propagation); splitting the AND into a local
   (`newFlags = p->flags & ~0x20000; p->flags = newFlags | ...;`, round 431 -
   byte-identical at 99.43, the AND result coalesces the same way); the constant
   written first as `(~0x20000 & p->flags)` (gcc canonicalises commutative
   operands); the same `flags` variable reassigned before the RMW (one pseudo
   for the whole function, so BOTH live ranges land in $4: 99.41, 30 lines); an
   explicit spinBit local (98.86); the two `+= BATTLE_ANGLE_HALF_TURN` moved
   above the RMW (the schedule collapses, worse).
   (2) 5 lines: `subu` ties its output to the SECOND operand ($v0) where retail
   works in place on the first ($v1); the following negu and slti ride the same
   register, so it is one pseudo's hand-out. A pad-angle local is 98.99, and
   `diff = direction & 0xFFF; diff -= ...` makes diff callee-saved (98.81,
   36 differing lines).
   Permuter from this body: 784 iterations (round ~415) and 1181 iterations
   (round 431), both base 145 best 145 - two runs agree, nothing left there.
   Preset screen on this body (round 431): gcc-2.7.2-cdk 98.38, gcc-2.6.0 97.88,
   Default 99.43, so the unit preset is right and this is not a cdk scheduling
   case.
   Keep these two spellings, they closed the previous 97.34 park: `limit =
   __builtin_abs(x)` inside the x < 0 arms (a ternary folds to a bare negu and
   loses retail's move+negu pair) and `p->distance = distance;` hoisted out of
   its if (retail stores distance unconditionally, in the beqz delay slot). */
/* The controller the actor reads: player 2's actor takes controller 1. A digital
   pad's sticks come straight from the axis globals, an analog one's from the
   button state. The four input ramps charge one direction at a time and decay
   the rest, and (right - left, up - down) becomes the movement vector. */
/*
s32 func_80076884(BattleActor* p) {
    u16 buttons;
    u16 released;
    u8 stickX;
    u8 stickY;
    s32 analog;
    s32 rampStep;
    s32 decayStep;
    s32 enabled;
    s32 stickY2;
    s32 stickX2;
    s32 angle;
    s32 direction;
    s32 distance;
    s32 limit;
    s32 diff;
    u32 flags;

    if (p->flags & BATTLE_ACTOR_FLAG_PLAYER2) {
        analog = (u32)(ControllerGetType(1) - CONTROLLER_TYPE_ANALOG_PAD) < 2;
        buttons = g_C2ButtonState;
        released = g_C2ButtonStateReleased;
        if (analog) {
            stickX = g_C2LeftStickXAxis;
            stickY = g_C2LeftStickYAxis;
        } else {
            stickX = ControllerStickToAnalogX(g_C2ButtonState);
            stickY = ControllerStickToAnalogY(g_C2ButtonState);
        }
    } else {
        analog = (u32)(ControllerGetType(CONTROLLER_TYPE_NONE) - CONTROLLER_TYPE_ANALOG_PAD) < 2;
        buttons = g_C1ButtonState;
        released = g_C1ButtonStateReleased;
        if (analog) {
            stickX = g_C1LeftStickXAxis;
            stickY = g_C1LeftStickYAxis;
        } else {
            stickX = ControllerStickToAnalogX(g_C1ButtonState);
            stickY = ControllerStickToAnalogY(g_C1ButtonState);
        }
    }
    flags = p->flags;
    if (flags & 0x40) {
        return;
    }
    if (g_BattleCameraSpinDirection != ((flags >> 17) & 1)) {
        if (g_BattleCameraSpinDirection != 0) {
            p->facingOffset = -p->sideBaseAngle;
        } else {
            p->facingOffset = p->sideBaseAngle;
        }
        p->flags = (p->flags & ~0x20000) | ((g_BattleCameraSpinDirection & 1) << 17);
        p->facingAngle += BATTLE_ANGLE_HALF_TURN;
        p->angleOffset += BATTLE_ANGLE_HALF_TURN;
    }
    if (released & CTRL_BTN_TRIANGLE) {
        BattleQueueMessage(p, 1);
        p->message12Count++;
    }
    if (released & CTRL_BTN_CIRCLE) {
        BattleQueueMessage(p, 2);
        p->message12Count++;
    }
    if (released & CTRL_BTN_R1) {
        BattleQueueMessage(p, 3);
        p->message3Count++;
    }
    if (released & CTRL_BTN_SQUARE) {
        BattleResetMessageQueue(p);
        BattleQueueMessage(p, 4);
    }
    if (buttons & 0x40) {
        p->flags |= 0x8000;
    }
    if (released & 0x40) {
        BattleResetMessageQueue(p);
    }
    decayStep = p->inputDecayStep;
    if (buttons & CTRL_BTN_L1) {
        p->flags |= 2;
        BattleResetMessageQueue(p);
        p->flags |= 0x100;
        decayStep *= 4;
    } else {
        p->flags &= ~0x38;
    }
    if ((p->flags & 0x60000000) == 0x60000000 && !(p->typeFlags & 4)) {
        rampStep = p->inputRampStep / 2;
    } else {
        rampStep = p->inputRampStep;
    }
    if ((p->flags & 0x60000000) == 0x20000000 && (p->typeFlags & 4)) {
        rampStep = p->inputRampStep / 2;
    }
    if (p->flags & 0x100) {
        decayStep = decayStep * 2 / 3;
        enabled = 0;
    } else {
        enabled = 1;
    }
    stickY2 = (stickY - 0x80) * 2;
    stickX2 = (stickX - 0x80) * 2;
    angle = ratan2(stickY2, stickX2) + p->facingOffset;
    if (buttons & (CTRL_BTN_UP | CTRL_BTN_RIGHT | CTRL_BTN_DOWN | CTRL_BTN_LEFT)) {
        if ((buttons & CTRL_BTN_UP) && enabled) {
            p->inputUp += rampStep;
            if (p->inputUp > 0x100) {
                p->inputUp = 0x100;
            }
        } else {
            p->inputUp -= decayStep;
            if (p->inputUp < 0) {
                p->inputUp = 0;
            }
        }
        if ((buttons & CTRL_BTN_DOWN) && enabled) {
            p->inputDown += rampStep;
            if (p->inputDown > 0x100) {
                p->inputDown = 0x100;
            }
        } else {
            p->inputDown -= decayStep;
            if (p->inputDown < 0) {
                p->inputDown = 0;
            }
        }
        if ((buttons & CTRL_BTN_LEFT) && enabled) {
            p->inputLeft += rampStep;
            if (p->inputLeft > 0x100) {
                p->inputLeft = 0x100;
            }
        } else {
            p->inputLeft -= decayStep;
            if (p->inputLeft < 0) {
                p->inputLeft = 0;
            }
        }
        if ((buttons & CTRL_BTN_RIGHT) && enabled) {
            p->inputRight += rampStep;
            if (p->inputRight > 0x100) {
                p->inputRight = 0x100;
            }
        } else {
            p->inputRight -= decayStep;
            if (p->inputRight < 0) {
                p->inputRight = 0;
            }
        }
        if (!(buttons & (CTRL_BTN_UP | CTRL_BTN_RIGHT | CTRL_BTN_DOWN | CTRL_BTN_LEFT))) {
            angle = 0;
        }
    } else {
        if (SquareRoot0(stickY2 * stickY2 + stickX2 * stickX2) < 0x30) {
            angle = 0;
        }
        if (stickY2 < 0 && enabled) {
            limit = __builtin_abs(stickY2);
            p->inputUp += rampStep;
            if (p->inputUp > limit) {
                p->inputUp = limit;
            }
        } else {
            p->inputUp -= decayStep;
            if (p->inputUp < 0) {
                p->inputUp = 0;
            }
        }
        if (stickY2 > 0 && enabled) {
            p->inputDown += rampStep;
            if (p->inputDown > stickY2) {
                p->inputDown = stickY2;
            }
        } else {
            p->inputDown -= decayStep;
            if (p->inputDown < 0) {
                p->inputDown = 0;
            }
        }
        if (stickX2 < 0 && enabled) {
            limit = __builtin_abs(stickX2);
            p->inputLeft += rampStep;
            if (p->inputLeft > limit) {
                p->inputLeft = limit;
            }
        } else {
            p->inputLeft -= decayStep;
            if (p->inputLeft < 0) {
                p->inputLeft = 0;
            }
        }
        if (stickX2 > 0 && enabled) {
            p->inputRight += rampStep;
            if (p->inputRight > stickX2) {
                p->inputRight = stickX2;
            }
        } else {
            p->inputRight -= decayStep;
            if (p->inputRight < 0) {
                p->inputRight = 0;
            }
        }
    }
    stickY2 = p->inputUp - p->inputDown;
    stickX2 = p->inputRight - p->inputLeft;
    direction = ratan2(stickX2, stickY2);
    distance = SquareRoot0(stickY2 * stickY2 + stickX2 * stickX2);
    if (distance > 0x100) {
        distance = 0x100;
    }
    if (distance < 0x30) {
        distance = 0;
    }
    p->distance = distance;
    if (distance != 0) {
        p->targetAngle = direction + p->facingOffset;
    } else {
        p->targetAngle = 0;
    }
    angle &= 0xFFF;
    if ((u32)(angle - 0x201) < 0x3FF) {
        BattleTryQueueMessage5(p);
        p->flags |= BATTLE_ACTOR_FLAG_FACING_AWAY;
    } else {
        p->flags &= ~BATTLE_ACTOR_FLAG_FACING_AWAY;
    }
    if ((p->flags & 0x80000) && p->distance != 0) {
        diff = (direction & 0xFFF) - ((ratan2(stickY - 0x80, stickX - 0x80) + 0x400) & 0xFFF);
        if (diff < 0) {
            diff = -diff;
        }
        if (diff >= 0x201) {
            p->flags |= 0x80000;
        }
    }
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80076884);

extern u8 g_BattleFacingUpdateEnabled;
extern s32 g_BattleFacingAngle;
extern s32 BattleCheckValueAboveScaledMax(BattleActor* p, s32 v);
extern void BattlePlaySoundEffectById(s32 arg);

/* Non-void return type with bare returns: as void gcc fills the beq delay slots
   that retail leaves as nop. The two volatile flag stores are load-bearing: each
   one stops gcc from forwarding the stored word into the next flags read, which
   is why retail reloads 0xD0 three times here (99.06% without them). */
s32 BattleUpdateActorStatus(BattleActor* p) {
    u32 flags;

    if (p->flashTimer != 0) {
        p->flashTimer -= 0x10;
    }
    if (p->state == 5) {
        return;
    }
    if (p->stateTimer >= 0x15) {
        p->stateTimer = 0x14;
    }
    if (p->stateTimer != 0) {
        p->stateTimer--;
    } else {
        p->flags &= ~0x1000;
    }
    if (p->moveReentryTimer != 0) {
        p->moveReentryTimer--;
    }
    if (p->actionWaitTimer != 0) {
        p->actionWaitTimer--;
    }
    if (p->moveEndTimer != 0) {
        p->moveEndTimer--;
    } else {
        p->moveFrameCount = 0;
        p->flags = (p->flags & 0xFF8FFFFF) | 0x300000;
    }
    if (p->actionPhaseTimer != 0) {
        p->actionPhase = 4;
        p->actionPhaseTimer--;
    } else if (p->actionPhase == 4) {
        p->actionPhase = 0;
    }
    if (p->moveTimer != 0) {
        p->moveTimer--;
    }
    if (p->gaugePoints != 0) {
        if (p->state & 1) {
            if ((p->flags & 0x60000000) == 0x60000000) {
                if (p->typeFlags & 4) {
                    p->gaugePoints -= 8;
                } else {
                    p->gaugePoints -= 2;
                }
            } else if ((p->flags & 0x60000000) == 0x20000000) {
                p->gaugePoints -= 6;
            } else {
                p->gaugePoints -= 4;
            }
        } else {
            p->gaugePoints -= 8;
        }
        if (p->gaugePoints < 0) {
            p->gaugePoints = 0;
        }
    }
    if (p->state == 4) {
        if (p->moveTimer == 0) {
            p->state = 0;
            p->actionPhase = 0;
        }
    } else {
        if (p->floorY - 0x30 < p->position.y) {
            p->state &= ~2;
        }
        if (p->distance != 0) {
            p->state |= 1;
        } else {
            p->state &= ~1;
        }
    }
    flags = p->flags;
    if (flags & 0x800) {
        switch (p->savedModelIndex) {
        case 9:
            if (p->moveTimer == 0) {
                p->flags = flags | 0x400;
            }
            break;
        case 0xF:
            if (p->actionPhase != 4) {
                p->flags = flags | 0x400;
            }
            break;
        case 2:
            if (p->state & 2) {
                p->flags = flags | 0x400;
            }
            break;
        }
    }
    p->gaugeValue -= p->gaugeLevel;
    p->gaugeValue -= p->gaugeDrain;
    if (p->gaugeLevel != 0) {
        if (p->soundTimer == 0) {
            if (BattleGetScreenMode() != 4) {
                BattlePlaySoundEffectById(0x2D);
            }
            p->soundTimer = 0x14;
        }
        p->soundTimer--;
    } else {
        p->soundTimer = 0;
    }
    if (g_BattleFacingUpdateEnabled != 0 && BattleCheckValueAboveScaledMax(p, 0x50) == 0 && (p->flags & 0x8000000)) {
        p->gaugeValue = p->gaugeMax * 0x50 / 0x100;
    }
    if (p->gaugeValue < 0) {
        p->gaugeValue = 0;
    }
    if (p->prevGaugeValue != p->gaugeValue) {
        p->prevGaugeValue = p->gaugeValue;
        p->prevGaugeWidth = p->gaugeWidth;
        p->gaugeWidth = (p->gaugeValue * 0x80 + p->gaugeMax - 1) / p->gaugeMax;
        if (p->gaugeWidth == 1) {
            p->gaugeWidth = 2;
        }
        p->flashTimer = 0xF0;
    }
    p->gaugeLevel = 0;
    p->gaugeDrain = 0;
    if (g_BattleMode != 4 && p->gaugeValue == 0 && g_BattleMode != 6 && !(p->flags & 0x800000)) {
        p->modelIndex = 0xD;
        p->prevPartIndex = 0xFF;
        p->state = 5;
        p->actionPhase = 0;
        *(volatile u32 *)&p->flags |= 0x1000;
        p->vy -= 0x50;
        *(volatile u32 *)&p->flags |= 0x400;
        if (p->flags & 0x8000000) {
            p->pushVx -= rcos(g_BattleFacingAngle) >> 9;
            p->pushVz -= rsin(g_BattleFacingAngle) >> 9;
        } else {
            p->pushVx += rcos(g_BattleFacingAngle) >> 9;
            p->pushVz += rsin(g_BattleFacingAngle) >> 9;
        }
        p->flags |= 0x800000;
        BattleUpdateControllerActuator(p, 0x14);
        p->movePhase = 0;
    }
    return;
}

/* Old-style definition and a non-void return both matter: BattleActorBeginMove
   calls it with one argument, and as void gcc fills the beqz delay slot with the
   clamp's addiu where retail leaves a nop. */
char func_80077584(p, ang, shift, dec)
BattleActor* p;
s32 ang;
s32 shift;
s32 dec;
{
    p->pushVx += rcos(ang) >> shift;
    p->pushVz += rsin(ang) >> shift;
    if (p->vy >= -0x8B) {
        p->vy -= dec;
    }
    if (p->vy < BATTLE_ACTOR_VY_MIN) {
        p->vy = BATTLE_ACTOR_VY_MIN;
    }
}

void BattleActorBeginMove(BattleActor* p, s32 ang, s32 shift, s32 dec) {
    p->flags |= BATTLE_ACTOR_FLAG_MODEL_CHANGE;
    p->modelIndex = 8;
    p->prevPartIndex = BATTLE_PART_INDEX_NONE;
    p->moveTimer = 0x3C;
    p->moveReentryTimer = 0x28;
    p->stateTimer = 0x28;
    p->state = 4;
    p->actionPhase = 0;
    p->stateFlags &= ~0x10;
    p->flags &= ~0x400;
    func_80077584(p, ang, shift, dec);
}

/* non-void return type, bare returns: as void gcc fills the beqz/beq delay
   slots that retail leaves as nop. */
s32 BattleUpdateControllerActuator(BattleActor* p, s32 mode) {
    u32 flags;

    if (BattleGetScreenMode() == 4) {
        return;
    }
    flags = p->flags;
    if (flags & BATTLE_ACTOR_FLAG_PLAYER2) {
        if (g_BattleMode != 2 && g_BattleMode != 4) {
            return;
        }
        if (!(g_BattlePlayer2Vibration & 1)) {
            return;
        }
        if (flags & 0x40) {
            return;
        }
        ControllerSetActuatorDuration(1, mode);
    } else {
        if (!(g_BattlePlayer1Vibration & 1)) {
            return;
        }
        if (flags & 0x40) {
            return;
        }
        ControllerSetActuatorDuration(0, mode);
    }
    return;
}

/* Non-void return type: as void gcc fills the beq delay slots that retail
   leaves as nop. */
short BattleUpdateActorMovement(BattleActor* p) {
    s32 dist;
    s32 ang;
    s32 mode;
    s32 mid;
    s32 decel;
    u32 flags;

    mode = 4;
    p->moveFrameCount++;
    if (p->moveTimer != 0) {
        if (p->modelIndex == 9) {
            p->modelIndex = 0xC;
            p->prevPartIndex = BATTLE_PART_INDEX_NONE;
            p->flags |= BATTLE_ACTOR_FLAG_MODEL_CHANGE;
            goto done;
        }
    } else {
        flags = p->flags;
        if (flags & 4) {
            mode = 2;
            goto done;
        }
        mid = (p->baseVertexPos.y - p->centerPosition.y) * 2 / 3 + p->centerPosition.y;
        if (p->moveGoalPosition.y < mid) {
            if ((flags & BATTLE_ACTOR_MOVE_DIR_MASK) == BATTLE_ACTOR_MOVE_DIR_DOWN) {
                p->modelIndex = 7;
                p->prevPartIndex = BATTLE_PART_INDEX_NONE;
                p->flags = (p->flags | BATTLE_ACTOR_FLAG_MODEL_CHANGE) & ~BATTLE_ACTOR_MOVE_DIR_MASK;
            } else {
                p->modelIndex = 6;
                p->prevPartIndex = BATTLE_PART_INDEX_NONE;
                p->flags = ((p->flags | BATTLE_ACTOR_FLAG_MODEL_CHANGE) & ~BATTLE_ACTOR_MOVE_DIR_MASK) | BATTLE_ACTOR_MOVE_DIR_DOWN;
            }
        } else {
            if ((flags & BATTLE_ACTOR_MOVE_DIR_MASK) == BATTLE_ACTOR_MOVE_DIR_UP) {
                p->modelIndex = 7;
                p->prevPartIndex = BATTLE_PART_INDEX_NONE;
                p->flags = (p->flags | BATTLE_ACTOR_FLAG_MODEL_CHANGE) & ~BATTLE_ACTOR_MOVE_DIR_MASK;
            } else {
                p->modelIndex = 5;
                p->prevPartIndex = BATTLE_PART_INDEX_NONE;
                p->flags = ((p->flags | BATTLE_ACTOR_FLAG_MODEL_CHANGE) & ~BATTLE_ACTOR_MOVE_DIR_MASK) | BATTLE_ACTOR_MOVE_DIR_UP;
            }
        }
    }
    ang = ratan2(p->position.x - p->moveGoalPosition.x, p->position.z - p->moveGoalPosition.z);
    dist = BattleVectorDistance2D(&p->position, p->pTarget);
    if (dist < BattleVectorDistance2D(&p->moveGoalPosition, p->pTarget)) {
        ang += BATTLE_ANGLE_HALF_TURN;
    }
    decel = 1;
    switch ((p->movePhase - 1) & 7) {
    case 2:
        p->modelIndex = 6;
        p->prevPartIndex = BATTLE_PART_INDEX_NONE;
        p->flags |= BATTLE_ACTOR_FLAG_MODEL_CHANGE;
        func_80077584(p, ang, 0xC, 0x64);
        break;
    case 3:
        BattleActorBeginMove(p, ang, 0xB, 0x80);
        break;
    case 1:
        decel = 0;
        /* fallthrough */
    default:
        if (p->moveReentryTimer >= 0x26 || p->moveFrameCount >= 4) {
            BattleActorBeginMove(p, ang, 0xA, 0xA0);
            mode = 0xF;
            p->stateTimer = 0;
            p->moveReentryTimer = 0;
        } else {
            func_80077584(p, ang, 0xA, -decel & 0x1E);
        }
        break;
    }
done:
    BattleUpdateControllerActuator(p, mode);
}

u8* BattleActorSelectActionEntry(BattleActor* p, s32 flag) {
    s32 idx;

    idx = p->actionIndex * 2;
    if (flag != 0) {
        p->actionIndex = g_BattleActionEntryIndices[idx];
    } else {
        p->actionIndex = D_80091179[idx];
    }
    return p->pActionEntries + p->actionIndex * 4;
}

void BattleActorSetFlag80000(BattleActor* p) {
    p->flags |= 0x80000;
}

extern s32 BattleEffectAddSlotFromVector(Vec4* pVec, s32 action);
extern void func_8008ED6C(u8* p, s32 idx);
extern void BattlePlayActorSoundPair(u8* p, s32 idx, void* pData, s32 flags);
extern s32 func_8008B650(s32 value, s32 max, s32 step);
extern s32 BattleAccumulateGaugePoints(BattleActor* p, s32 points, s32 mode);
extern u8 g_BattleCameraLerpFrames;

void BattleUpdateActorState(BattleActor* p) {
    u8* pEntry;
    u32 flags;
    s32 flag;

    if (p->state == 5) {
        return;
    }
    if (p->movePhase != 0) {
        BattleUpdateActorMovement(p);
        return;
    }
    flag = 0;
    if (p->stateTimer != 0) {
        BattleResetMessageQueue(p);
        return;
    }
    p->flags &= ~1;
    if (p->actionWaitTimer != 0) {
        BattleResetMessageQueue(p);
    }
    if (p->flags & 0x800) {
        if (p->actionPhase != 4) {
            p->actionPhase = 0;
        }
    }
    if (p->actionPhase == 0 && p->state != 4) {
        switch (BattleDequeueMessage(p)) {
        case 1:
            if (p->actionWaitTimer == 0) {
                pEntry = BattleActorSelectActionEntry(p, 1);
                goto actionEntry;
            }
            break;
        case 2:
            if (p->actionWaitTimer == 0) {
                pEntry = BattleActorSelectActionEntry(p, 0);
                goto actionEntry;
            }
            break;
        actionEntry:
            if (!(p->state & 2)) {
                if (pEntry[0] != 0) {
                    p->modelIndex = pEntry[0] + 0x12;
                    p->prevPartIndex = 0xFF;
                    flags = p->flags | 0x1000;
                    p->flags = flags;
                    if (pEntry[1] != 0) {
                        flag = 1;
                    }
                    p->flags = flags | 1;
                    p->actionPhase = 2;
                    p->pActionEntry = pEntry;
                    p->gaugePercent = p->pActionData->points[p->actionIndex] * p->pActionData->scale / 100;
                } else {
                    p->actionWaitTimer = 0xF;
                    BattleResetMessageQueue(p);
                }
            }
            break;
        case 0:
            BattleResetMessageQueue(p);
            break;
        case 3:
            if (p->actionWaitTimer == 0 && p->pActionData->percent != 0 && p->typeId != 0x29) {
                if (p->state & 2) {
                    p->modelIndex = 4;
                } else {
                    p->modelIndex = 3;
                }
                p->prevPartIndex = 0xFF;
                p->flags |= 0x1000;
                // empty loop is a scheduling boundary: without it gcc hoists the
                // constant above the flags read-modify-write (levers.md, round 433)
                do { } while (0);
                flag = 1;
                p->actionPhase = 2;
                p->flags |= 1;
            }
            break;
        case 4:
            if (p->moveTimer != 0) {
                return;
            }
            if (!(p->state & 2)) {
                BattleEffectAddSlotFromVector(&p->centerPosition, 9);
                p->vy -= 0x8C;
                p->state |= 2;
                func_8008ED6C((u8*)p, 0xA);
                BattlePlayActorSoundPair((u8*)p, 0xA, &p->centerPosition, 2);
            }
            break;
        case 5:
            p->modelIndex = 0xF;
            p->prevPartIndex = 0xFF;
            p->flags |= 0x1000;
            BattlePlayActorSoundPair((u8*)p, 0xE, p, 2);
            p->actionPhaseTimer = 6;
            p->actionPhase = 4;
            break;
        }
    }
    p->animSpeedBonus = 0;
    p->flags &= ~4;
    switch (p->state) {
    case 0:
        p->targetAngle = 0;
        switch (p->actionPhase) {
        case 0:
            if (p->flags & 2) {
                p->modelIndex = 0x11;
                if ((p->flags & 0x38) == 0x10) {
                    p->flags |= 4;
                } else {
                    p->flags = (p->flags & ~0x38) | ((((p->flags >> 3) & 7) + 1) & 7) << 3;
                }
            } else {
                p->modelIndex = 0;
            }
            break;
        case 2:
            if (flag != 0) {
                p->steerSpeed += 0x40;
            }
            break;
        case 4:
            BattleActorSetFlag80000(p);
            break;
        }
        p->angleOffset = func_8008B650(p->angleOffset, p->targetAngle, 0x100);
        break;
    case 1:
        p->animSpeedBonus = p->distance * p->moveSpeed / 4096;
        switch (p->actionPhase) {
        case 0:
            p->modelIndex = 1;
            break;
        case 2:
            if (flag != 0) {
                p->steerSpeed += 0x40;
            }
            break;
        case 4:
            BattleActorSetFlag80000(p);
            break;
        }
        p->angleOffset = func_8008B650(p->angleOffset, p->targetAngle, 0x100);
        if ((p->flags & 0x60000000) == 0x20000000 && !(p->typeFlags & 2)) {
            p->flags &= ~0x8000;
        }
        break;
    case 2:
        p->distance = 0;
        p->steerSpeed = 0;
        p->targetAngle = 0;
        if (p->actionPhase == 0) {
            p->modelIndex = 2;
        }
        break;
    case 3:
        if (p->actionPhase == 0) {
            p->modelIndex = 2;
        }
        break;
    case 4:
        p->distance = 0;
        break;
    }
    p->stepDistance = p->distance * p->stepScale * p->moveSpeed >> 16;
    if (!(p->flags & 0x8000)) {
        p->stateFlags &= ~0x40;
    }
    if (p->flags & 0x8000) {
        if (p->actionPhase == 0 && !(p->stateFlags & 0x40) && p->stepDistance >= 0x31 && (u32)(p->state - 2) >= 2) {
            if (BattleAccumulateGaugePoints(p, 0x20, 2) != 0) {
                p->stepDistance *= 2;
                p->animSpeedBonus *= 2;
            } else {
                p->flags &= ~0x8000;
                p->stateFlags |= 0x40;
            }
            if (g_BattleCameraLerpFrames >= 2) {
                g_BattleCameraLerpFrames = 1;
            }
        }
    }
    if (p->state == 0 || (p->stateFlags & 0x40)) {
        p->flags &= ~0x8000;
    }
}

void BattleActorApplyCenterPosition(BattleActor* p) {
    p->position = p->centerPosition;
    p->modelIndex = 0;
    p->state = 0;
    p->actionPhase = 0;
    p->flags |= 0x2000000;
}


extern s32 g_BattleAiTargetDistance;
extern u8 g_BattleOptionOn;
extern s32 func_8008B650(s32 value, s32 max, s32 step);
extern u8 g_BattleCameraLerpFrames;

/* 89.19%: every call, both lerp limiter paths, the actionPhase==4 reset, the 0x40000
   cos/sin pair, the p->steerSpeed steer block, the g_BattleAiTargetDistance target-follow block and
   the final fade all line up, and the frame is retail's exactly (0x30, s0/s1/s2,
   delta at sp+0x10). Three residuals, all allocator/scheduling, no structure:
   - the p->actionPhase==2 block: retail keeps the 0xFFF7FFFF mask in $a0, the 0x90C
     value in $v1 and the flags in $v0, and its two `sw 0xD0` stores are merged by
     cross-jumping into one at the join; ours uses $v0/$a0/$v1 and sinks the store
     into the following `if (p->flags & 0x40000)` arm. Declaring a shared `u32
     flags` local and storing once after the if/else removes the duplicate store
     but gcc then sinks it 2 instructions late (worth 1 store, lost elsewhere).
   - every `mult` here yields `mfhi $t1` in retail and `mfhi $a3` in ours (6
     sites); the multiset is otherwise identical.
   - the g_BattleAiTargetDistance block: retail issues all three p->pTarget loads first (raw
     values spilled to sp+0x10..0x18) and pipelines the three mults; ours computes
     one delta at a time. A `Vec4 target = p->pTarget->position;` copy at the head
     of the block gets the pipelined shape but costs 0x10 of frame (0x40 vs 0x30),
     which is the wrong trade. The switch on ((p->flags >> 29) & 3) needs its
     operand in a local (`mode`) to get retail's slti pivot; alpha must be
     `= 0x80` before the switch with an empty case 2, otherwise gcc emits a flat
     compare chain.
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80078194);
/*
void func_80078194(BattleActor* p) {
    s32 angle;
    s32 target;
    s32 alpha;
    s32 mode;

    p->prevPosition = p->position;
    p->prevCenterPosition = p->centerPosition;
    angle = p->stepDistance;
    target = g_BattleFacingAngle;
    if (p->flags & 0x8000000) {
        target += 0x800;
    }
    if (p->state == 0) {
        if (p->angleOffset != 0) {
            p->facingAngle = target;
        } else {
            p->facingAngle = func_8008B650(p->facingAngle, target, 0x100);
        }
        if (g_BattleCameraLerpFrames >= 5) {
            g_BattleCameraLerpFrames = 4;
        }
    } else if (p->state == 1) {
        if (g_BattleActorDistance2D >= 0x101) {
            if (p->angleOffset != 0) {
                p->facingAngle = target;
            } else {
                p->facingAngle = func_8008B650(p->facingAngle, target, 0x100);
            }
        }
        if (g_BattleCameraLerpFrames >= 3) {
            g_BattleCameraLerpFrames = 2;
        }
    }
    if (p->actionPhase == 4) {
        angle = -(p->actionPhaseTimer * 0x50) / 3;
        p->angle = target;
        p->facingAngle = target;
        p->angleOffset = 0;
        p->stepDistance = 0;
        p->inputDown = 0;
        p->inputUp = 0;
        p->inputLeft = 0;
        p->inputRight = 0;
    } else {
        p->angle = p->facingAngle + p->angleOffset;
    }
    if (p->actionPhase == 2) {
        if ((p->flags & 1) || (p->state & 2)) {
            p->actionAngle = target;
        }
        p->flags = (p->flags | 0x100) & 0xFFF7FFFF;
        p->angle = p->actionAngle;
    } else {
        p->flags &= ~0x100;
    }
    if (p->flags & 0x40000) {
        p->vx = -(rcos(p->facingAngle + p->angleOffset) * angle) >> 12;
        p->vz = -(rsin(p->facingAngle + p->angleOffset) * angle) >> 12;
    } else {
        p->flags &= 0xFFF7FFFF;
    }
    p->vy += 0xA;
    if (p->steerSpeed != 0 && p->position.y == p->floorY) {
        p->steerSpeed = p->steerSpeed * 0xA0 / 256;
        p->steerVx = -(rcos(target) * p->steerSpeed) >> 12;
        p->steerVz = -(rsin(target) * p->steerSpeed) >> 12;
        p->vx += p->steerVx;
        p->vy += p->steerVy;
        p->vz += p->steerVz;
    }
    if (g_BattleAiTargetDistance >= 0x281 && g_BattleOptionOn != 0) {
        Vec4 delta;

        delta.x = (p->pTarget->position.x - p->position.x) / 12;
        delta.y = (p->pTarget->position.y - p->position.y) / 12;
        delta.z = (p->pTarget->position.z - p->position.z) / 12;
        p->vx += delta.x;
        p->vy += delta.y;
        p->vz += delta.z;
    }
    mode = (p->flags >> 29) & 3;
    alpha = 0x80;
    switch (mode) {
    case 1:
        alpha = 0;
        break;
    case 0:
        alpha = 0xFF;
        break;
    case 2:
        break;
    case 3:
        alpha = 0;
        break;
    }
    if (p->state == 4) {
        alpha = 0xFF;
    }
    if (p->flags & 0x40000) {
        p->pushVx = p->pushVx * alpha / 256;
        p->pushVz = p->pushVz * alpha / 256;
    }
    p->vx += p->pushVx;
    p->vy += p->pushVy;
    p->vz += p->pushVz;
}

*/

extern s32 BattleVectorDistance2D(Vec4*, Vec4*);

// Returns the index of the edge the segment pFrom..pTo meets, and writes the
// edge's vector to *ppEdge.
s32 BattleActorSelectEdgeForSegment(Vec4* pFrom, Vec4* pTo, BattleActor* pEdges, Vec4** ppEdge) {
    s32 dz;
    s32 dx;
    s32 side0;
    s32 side1;
    s32 side2;
    s32 prod1;
    s32 prod2;

    dz = pFrom->z - pTo->z;
    dx = pTo->x - pFrom->x;
    side0 = dz * pEdges->prevPosition.x + dx * pEdges->prevPosition.z + pFrom->x * pTo->z - pTo->x * pFrom->z;
    side1 = dz * pEdges->baseVertexPos.x + dx * pEdges->baseVertexPos.z + pFrom->x * pTo->z - pTo->x * pFrom->z;
    side2 = dz * pEdges->centerPosition.x + dx * pEdges->centerPosition.z + pFrom->x * pTo->z - pTo->x * pFrom->z;
    if (side0 < 0) {
        side0 = -1;
    } else if (side0 > 0) {
        side0 = 1;
    }
    if (side1 < 0) {
        side1 = -1;
    } else if (side1 > 0) {
        side1 = 1;
    }
    if (side2 < 0) {
        side2 = -1;
    } else if (side2 > 0) {
        side2 = 1;
    }
    prod1 = side1 * side0;
    prod2 = side2 * side0;
    if (prod1 < 0) {
        if (prod2 < 0) {
            s32 dist92C = BattleVectorDistance2D(&pEdges->baseVertexPos, pFrom);
            if (BattleVectorDistance2D(&pEdges->centerPosition, pFrom) < dist92C) {
                *ppEdge = &pEdges->baseVertexPos;
            } else {
                *ppEdge = &pEdges->centerPosition;
            }
            return 0;
        }
        *ppEdge = &pEdges->baseVertexPos;
        return 1;
    }
    if (prod2 < 0) {
        *ppEdge = &pEdges->centerPosition;
        return 2;
    }
    {
        s32 dist92C = BattleVectorDistance2D(&pEdges->baseVertexPos, pFrom);
        if (dist92C < BattleVectorDistance2D(&pEdges->centerPosition, pFrom)) {
            *ppEdge = &pEdges->baseVertexPos;
        } else {
            *ppEdge = &pEdges->centerPosition;
        }
    }
    return 3;
}
