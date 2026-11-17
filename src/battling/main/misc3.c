#include "common.h"
#include "psyq/libgte.h"
#include "system/memory.h"

extern void func_800379C8(char*, ...);
extern char D_8006FCC0[];
extern char D_8006FCC4[];
extern char D_8006FCC8[];
extern s32 D_8009284C;
extern u8 D_80091178[];
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
    /* 0x18 */ u8 unk18;
} BattleActionData; // Size: 0x19 (a lower bound)

// A battle participant as this unit drives it. Only the fields this unit
// touches are named; 0x166A is a lower bound on the real size (other battle
// units reach further).
typedef struct BattleActor {
    /* 0x0000 */ Vec4 position;
    /* 0x0010 */ s32 vx;
    /* 0x0014 */ s32 vy;
    /* 0x0018 */ s32 vz;
    /* 0x001C */ u8 unk1C[0x4];
    /* 0x0020 */ s32 pushVx;
    /* 0x0024 */ s32 pushVy;
    /* 0x0028 */ s32 pushVz;
    /* 0x002C */ s32 unk2C;
    /* 0x0030 */ s32 steerVx;
    /* 0x0034 */ s32 steerVy;
    /* 0x0038 */ s32 steerVz;
    /* 0x003C */ u8 unk3C[0x4];
    /* 0x0040 */ s32 stepDistance;
    /* 0x0044 */ s32 steerSpeed;
    /* 0x0048 */ s32 distance;
    /* 0x004C */ u8 modelIndex;
    /* 0x004D */ u8 unk4D;
    /* 0x004E */ u8 prevPartIndex;
    /* 0x004F */ u8 animSpeed;
    /* 0x0050 */ s16 lastFrame;
    // Added to animSpeed when the part animation advances (misc2 func_80074BA4);
    // the movement sets it from the distance and the speed scale.
    /* 0x0052 */ u8 animSpeedBonus;
    /* 0x0053 */ u8 unk53;
    /* 0x0054 */ s32 angle;
    /* 0x0058 */ s32 targetAngle;
    /* 0x005C */ u8 unk5C[0x10];
    /* 0x006C */ Vec4 prevPosition;
    /* 0x007C */ u8 unk7C[0x4];
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
    /* 0x00A0 */ u8 unkA0[0x10];
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
    // record when the step is taken, func_80077038 counts it down, and at zero
    // the move's frame count is cleared for the stop animation (0x300000).
    /* 0x00C8 */ s16 moveEndTimer;
    /* 0x00CA */ s16 moveTimer;
    // A 12-bit angle pair: 0xCE is always derived from 0xCC (its negated value
    // when the camera spin direction flips, itself otherwise, or 0xCC + 0x800 in
    // misc4's round reset, which sets 0xCC to -0x400/0x400 for the two sides) and
    // func_80076884 adds 0xCE to the facings it computes.
    /* 0x00CC */ u16 unkCC;
    /* 0x00CE */ s16 unkCE;
    /* 0x00D0 */ u32 flags;
    /* 0x00D4 */ u32 stateFlags;
    /* 0x00D8 */ struct BattleActor* pTarget;
    /* 0x00DC */ u8 unkDC[0xC];
    /* 0x00E8 */ s32 stateTimer;
    /* 0x00EC */ u8 unkEC[0x4];
    /* 0x00F0 */ s16 moveFrameCount;
    /* 0x00F2 */ u8 unkF2[0xA];
    /* 0x00FC */ s32 angleOffset;
    /* 0x0100 */ s32 movePhase;
    /* 0x0104 */ u8 unk104[0x540];
    /* 0x0644 */ s32 gaugePercent;
    /* 0x0648 */ s32 facingAngle;
    /* 0x064C */ u8 unk64C[0x2BD];
    /* 0x0909 */ u8 typeId;
    // One bit per actor type (see src/battling/main/misc4.c): bit 1 makes the
    // actor queue message 5, bit 2 quadruples the gauge drain.
    /* 0x090A */ u8 typeFlags;
    /* 0x090B */ u8 unk90B;
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
    /* 0x091C */ Vec4 unk91C;
    // One of the two world points whose y words bound the actor; 0x93C is the
    // other (src/battling/main/misc4.c names the pair).
    /* 0x092C */ Vec4 baseVertexPos;
    /* 0x093C */ Vec4 centerPosition;
    /* 0x094C */ u8 unk94C[0x10];
    /* 0x095C */ Vec4 prevCenterPosition;
    /* 0x096C */ u8 unk96C[0x4];
    /* 0x0970 */ s32 gaugeDrain;
    // Position of the target slot the current step walks to, copied from the
    // selected record by func_80075B50.
    /* 0x0974 */ Vec4 moveGoalPosition;
    /* 0x0984 */ u8 unk984[0x1C];
    /* 0x09A0 */ BattleMessageQueue messageQueue;
    /* 0x09C3 */ u8 actionIndex;
    /* 0x09C4 */ u8 unk9C4[0xC24];
    /* 0x15E8 */ s16 unk15E8;
    /* 0x15EA */ s16 unk15EA;
    /* 0x15EC */ u8 unk15EC[0x4];
    // The actor's step scale, copied from its script's byte at +0xF: it multiplies
    // the distance into stepDistance.
    /* 0x15F0 */ s16 stepScale;
    /* 0x15F2 */ s16 moveSpeed;
    /* 0x15F4 */ s16 moveSpeedBase;
    /* 0x15F6 */ u8 unk15F6[0xA];
    /* 0x1600 */ BattleActionData* pActionData;
    /* 0x1604 */ u8 unk1604[0x50];
    // Gauge damage taken in each of two categories, compared with fractions of
    // gaugeMax to pick the battle result code (misc2 BattleAccumulateGaugePoints,
    // BattleSetResultCode).
    /* 0x1654 */ s32 unk1654;
    /* 0x1658 */ s32 unk1658;
    // Counters the input handler bumps as it queues messages: 0x1660 for each
    // message 1 or 2, 0x165C for each message 3. No reader in this overlay.
    /* 0x165C */ s32 unk165C;
    /* 0x1660 */ s32 unk1660;
    // The sound table BattleSetActorSoundTable picks by typeId; misc17 indexes it
    // with a byte index.
    /* 0x1664 */ u8* pSoundTable;
    /* 0x1668 */ u16 unk1668;
} BattleActor; // Size: 0x166A

s32 BattleActorDeltaUnk15E8(BattleActor* p0, BattleActor* p1) {
    return p1->unk15EA - p0->unk15E8;
}

void BattleNoop2(void) {
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80075750);

/* 85.58%: the whole body, frame (0x40, three saved regs), the two divides,
   the 0xFFF rounding and the tail all line up; the only residual is that gcc
   gives $s1 to `ex = x3 - x1` and $s0 to `ey = y3 - y1` while the original has
   the reverse (every downstream mult operand is swapped with them). Tried:
   both declaration orders for ex/ey, both assignment orders (that one also
   swaps the $v1/$t0 temps and drops to 85.13%), ex/ey declared after the other
   locals. Allocation order follows the RTL definition order, so the pair cannot
   be flipped without also flipping the emitted order.
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
    if (along < 0) {
        return 0;
    }
    return (cross * cross - r * r < 0) & (dist + r >= along);
}
*/

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


INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80075B50);

void BattleQueueMessage(BattleActor* p, u8 value) {
    u8 idx;

    if (p->messageQueue.count < 0x20) {
        idx = p->messageQueue.writeIdx;
        p->messageQueue.writeIdx = idx + 1;
        p->messageQueue.buf[idx & 0x1F] = value;
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
    value = p->messageQueue.buf[idx & 0x1F];
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
    func_800379C8(D_8006FCC0);
    for (i = 0; i < p->messageQueue.count; i++) {
        func_800379C8(D_8006FCC4, p->messageQueue.buf[(idx++) & 0x1F]);
    }
    func_800379C8(D_8006FCC8);
}

extern s32 BattleGetScreenMode(void);
extern void BattleSetTextCursor(s32 a0, s32 a1);
extern s32 g_BattleMotionSpeedScale;
extern u8 g_BattlePlayer2Vibration;
extern u8 g_BattlePlayer1Vibration;
extern u32 D_80091198[];
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
    // the do-block keeps the 0x8000000 test out of the mult shadow
    do {
        prod = p->moveSpeedBase * g_BattleMotionSpeedScale;
        flags = p->flags;
        flags &= ~0x80000;
        p->flags = flags;
        p->moveSpeed = prod >> 8;
    } while (0);
    flags &= 0x8000000;
    if (!flags) {
        BattleSetTextCursor(0x28, 0x28);
    } else {
        BattleSetTextCursor(0xF0, 0x28);
    }
    if (p->actionIndex != 0) {
        g_BattleSlotGauges[((p->flags >> 27) & 1)].value = D_80091198[p->actionIndex];
        pActionData = p->pActionData;
        g_BattleSlotGauges[((p->flags >> 27) & 1)].percent = (pActionData->points[p->actionIndex] * pActionData->scale) / 100;
    }
}

extern void BattleSetTextScale(s32 color);
extern void BattleDrawTextLeft(char* pStr);
extern s32 g_BattleMode;
extern s32 D_8009112C;
extern char D_8006FD04[];
extern char D_8006FD08[];
extern char D_8006FD10[];

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
    if (p->flags & 0x8000000) {
        w = 0xB4;
    }
    if (g_BattleMode != 4) {
        h = 0xB2;
    }
    BattleSetTextCursor(w, h);
    Sprintf(buf, D_8006FD04, g_BattleSlotGauges[((p->flags >> 27) & 1)].value);
    if (g_BattleSlotGauges[((p->flags >> 27) & 1)].value == D_8009112C) {
        color = 0xE7;
    } else {
        color = 0x100;
    }
    BattleSetTextScale(color);
    BattleDrawTextLeft(buf);
    BattleSetTextScale(0x100);
    BattleSetTextCursor(w + 0x34, h);
    Sprintf(buf, D_8006FD08, g_BattleSlotGauges[((p->flags >> 27) & 1)].percent);
    BattleDrawTextLeft(buf);
    BattleSetTextCursor(w, h + 0x14);
    h = g_BattleSlotGauges[((p->flags >> 27) & 1)].frameStart >> 4;
    Sprintf(buf, D_8006FD10, h, g_BattleSlotGauges[((p->flags >> 27) & 1)].frameCurrent >> 4);
    BattleDrawTextLeft(buf);
}

/* non-void return type: as void gcc fills the beq delay slots that retail
   leaves as nop. */
short BattleTryQueueMessage5(BattleActor* p) {
    u32 flags;

    if (D_8009284C >= 0x201) {
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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80076884);

extern u8 g_BattleFacingUpdateEnabled;
extern s32 g_BattleFacingAngle;
extern s32 BattleCheckValueAboveScaledMax(BattleActor* p, s32 v);
extern void BattlePlaySoundEffectById(s32 arg);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80077038);

/* 99.06%: every instruction is retail's, the function is 4 bytes short (1348 vs
   1352) for one reason. At the `p->flags |= 0x400` store retail RELOADS the word
   for the 0x8000000 test (`sw v1,208(s0); lw v0,208(s0); lui v1,0x800; and`)
   while gcc forwards the stored value into the `and`, so `lui` is hoisted and
   the reload disappears. Every other shape of the body is byte-identical.
   Tried: `p->flags = p->flags | 0x400`, a `flags` local, both statement orders
   for the |= 0x1000 / unk14 / |= 0x400 trio, a do/while block around the store.
   The permuter (3 min) reached base=326 best=120 only by moving `p->gaugeDrain = 0`
   inside the if, which fixes the reload but costs retail's store placement.
*/
/*
s32 func_80077038(BattleActor* p) {
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
        p->flags |= 0x1000;
        p->vy -= 0x50;
        p->flags |= 0x400;
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
*/

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
    if (p->vy < -0x82) {
        p->vy = -0x82;
    }
}

void BattleActorBeginMove(BattleActor* p, s32 ang, s32 shift, s32 dec) {
    p->flags |= 0x1000;
    p->modelIndex = 8;
    p->prevPartIndex = 0xFF;
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
    if (flags & 0x8000000) {
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
            p->prevPartIndex = 0xFF;
            p->flags |= 0x1000;
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
            if ((flags & 0x700000) == 0x100000) {
                p->modelIndex = 7;
                p->prevPartIndex = 0xFF;
                p->flags = (p->flags | 0x1000) & 0xFF8FFFFF;
            } else {
                p->modelIndex = 6;
                p->prevPartIndex = 0xFF;
                p->flags = ((p->flags | 0x1000) & 0xFF8FFFFF) | 0x100000;
            }
        } else {
            if ((flags & 0x700000) == 0x200000) {
                p->modelIndex = 7;
                p->prevPartIndex = 0xFF;
                p->flags = (p->flags | 0x1000) & 0xFF8FFFFF;
            } else {
                p->modelIndex = 5;
                p->prevPartIndex = 0xFF;
                p->flags = ((p->flags | 0x1000) & 0xFF8FFFFF) | 0x200000;
            }
        }
    }
    ang = ratan2(p->position.x - p->moveGoalPosition.x, p->position.z - p->moveGoalPosition.z);
    dist = BattleVectorDistance2D(&p->position, p->pTarget);
    if (dist < BattleVectorDistance2D(&p->moveGoalPosition, p->pTarget)) {
        ang += 0x800;
    }
    decel = 1;
    switch ((p->movePhase - 1) & 7) {
    case 2:
        p->modelIndex = 6;
        p->prevPartIndex = 0xFF;
        p->flags |= 0x1000;
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
        p->actionIndex = D_80091178[idx];
    } else {
        p->actionIndex = D_80091179[idx];
    }
    return p->pActionEntries + p->actionIndex * 4;
}

void BattleActorSetFlag80000(BattleActor* p) {
    p->flags |= 0x80000;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80077A9C);

/* 97.90%: 430 of 430 instructions, same size (1720) and same .rodata (both jump
   tables in case order). The whole instruction multiset matches; three residuals,
   all of them allocation/layout, not structure:
   - the `p->actionWaitTimer = 0xF` block of the reset path is placed after the case-1
     guard here, and after the shared case-1/2 body (falling straight into
     BattleResetMessageQueue, whose copy case 0 also uses) in the original. The
     two cases' reset blocks are identical so cc1 cross-jumps them and keeps the
     FIRST copy; retail kept the LAST one. Tried: case order 1,0,2 and 0,1,2
     (92.52 both, 1760 bytes), case order 2,1 (97.74), `if (pEntry[0] != 0)
     {body} else {reset}` and the body-first `if (...) {body; break;} reset`
     (95.52 both - the cross-jump then does not happen at all), early-break
     spelling of the case bodies (same 97.90).
   - after BattleActorSelectActionEntry the original copies the result to $a1
     immediately and tests p->state in $v0; gcc 2.7.2 keeps it in $v0 and copies
     to $a1 in the bnez delay slot (so the flags load lands in $v1 and the
     `ori 0x1000` in $a0).
   - case 3 of the first switch: `flag = 1` gives `li $s2,1` two instructions
     earlier than retail.
   Structure worth keeping: the switch on BattleDequeueMessage takes 6 cases and
   the first switch's cases must be LISTED 1, 2, 0, 3, 4, 5 (retail's body order);
   the second switch on p->state is ascending 0-4, and its case 1 recomputes
   p->animSpeedBonus with `/ 4096` (gcc's signed divide-by-power-of-2 sequence) while the
   shared tail uses `>> 16`. BattleDequeueMessage must be declared s32: with a u8
   return every caller gets an `andi v0,v0,0xff` retail does not have (verified:
   the callee still matches 100.00% as s32).
   Permuter: 1679 iterations from this body found nothing (base=462, best=462).
extern s32 BattleEffectAddSlotFromVector(Vec4* pVec, s32 action);
extern void func_8008ED6C(u8* p, s32 idx);
extern void func_8008EBD0(u8* p, s32 idx, void* pData, s32 flags);
extern s32 func_8008B650(s32 value, s32 max, s32 step);
extern s32 BattleAccumulateGaugePoints(BattleActor* p, s32 points, s32 mode);
extern u8 g_BattleCameraLerpFrames;

// The battle actor's action state machine: a queued message selects the action
// (the first switch), then the state at 0xC4 recomputes the gauge word.
void func_80077A9C(BattleActor* p) {
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
                if (!(p->state & 2)) {
                    if (pEntry[0] == 0) {
                        p->actionWaitTimer = 0xF;
                        BattleResetMessageQueue(p);
                    } else {
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
                    }
                }
            }
            break;
        case 2:
            if (p->actionWaitTimer == 0) {
                pEntry = BattleActorSelectActionEntry(p, 0);
                if (!(p->state & 2)) {
                    if (pEntry[0] == 0) {
                        p->actionWaitTimer = 0xF;
                        BattleResetMessageQueue(p);
                    } else {
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
                    }
                }
            }
            break;
        case 0:
            BattleResetMessageQueue(p);
            break;
        case 3:
            if (p->actionWaitTimer == 0 && p->pActionData->unk18 != 0 && p->typeId != 0x29) {
                if (p->state & 2) {
                    p->modelIndex = 4;
                } else {
                    p->modelIndex = 3;
                }
                p->prevPartIndex = 0xFF;
                p->flags |= 0x1000;
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
                func_8008EBD0((u8*)p, 0xA, &p->centerPosition, 2);
            }
            break;
        case 5:
            p->modelIndex = 0xF;
            p->prevPartIndex = 0xFF;
            p->flags |= 0x1000;
            func_8008EBD0((u8*)p, 0xE, p, 2);
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
*/


void BattleActorApplyCenterPosition(BattleActor* p) {
    p->position = p->centerPosition;
    p->modelIndex = 0;
    p->state = 0;
    p->actionPhase = 0;
    p->flags |= 0x2000000;
}

extern s32 D_80092850;
extern u8 g_BattleOptionOn;
extern s32 func_8008B650(s32 value, s32 max, s32 step);
extern u8 g_BattleCameraLerpFrames;

/* 89.19%: every call, both lerp limiter paths, the actionPhase==4 reset, the 0x40000
   cos/sin pair, the p->steerSpeed steer block, the D_80092850 target-follow block and
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
   - the D_80092850 block: retail issues all three p->pTarget loads first (raw
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
        if (D_8009284C >= 0x101) {
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
    if (D_80092850 >= 0x281 && g_BattleOptionOn != 0) {
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
