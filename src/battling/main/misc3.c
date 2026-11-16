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
    /* 0x0020 */ s32 unk20;
    /* 0x0024 */ s32 unk24;
    /* 0x0028 */ s32 unk28;
    /* 0x002C */ s32 unk2C;
    /* 0x0030 */ s32 unk30;
    /* 0x0034 */ s32 unk34;
    /* 0x0038 */ s32 unk38;
    /* 0x003C */ u8 unk3C[0x4];
    /* 0x0040 */ s32 stepDistance;
    /* 0x0044 */ s32 unk44;
    /* 0x0048 */ s32 distance;
    /* 0x004C */ u8 modelIndex;
    /* 0x004D */ u8 unk4D;
    /* 0x004E */ u8 prevPartIndex;
    /* 0x004F */ u8 unk4F;
    /* 0x0050 */ s16 unk50;
    /* 0x0052 */ u8 unk52;
    /* 0x0053 */ u8 unk53;
    /* 0x0054 */ s32 angle;
    /* 0x0058 */ s32 unk58;
    /* 0x005C */ u8 unk5C[0x10];
    /* 0x006C */ Vec4 prevPosition;
    /* 0x007C */ u8 unk7C[0x4];
    /* 0x0080 */ u8* pActionEntries;
    /* 0x0084 */ u8* pActionEntry;
    /* 0x0088 */ s32 unk88;
    /* 0x008C */ s32 unk8C;
    /* 0x0090 */ s32 unk90;
    /* 0x0094 */ s32 unk94;
    /* 0x0098 */ u8 unk98[0x18];
    /* 0x00B0 */ s32 floorY;
    /* 0x00B4 */ s16 gaugeValue;
    /* 0x00B6 */ s16 gaugePoints;
    /* 0x00B8 */ s16 prevGaugeValue;
    /* 0x00BA */ s16 gaugeLevel;
    /* 0x00BC */ s16 gaugeMax;
    /* 0x00BE */ s16 unkBE;
    /* 0x00C0 */ u8 gaugeWidth;
    /* 0x00C1 */ u8 prevGaugeWidth;
    /* 0x00C2 */ u8 unkC2;
    /* 0x00C3 */ u8 unkC3;
    /* 0x00C4 */ u8 state;
    /* 0x00C5 */ u8 actionPhase;
    /* 0x00C6 */ u8 unkC6[0x2];
    /* 0x00C8 */ s16 unkC8;
    /* 0x00CA */ s16 moveTimer;
    /* 0x00CC */ u8 unkCC[0x4];
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
    /* 0x090A */ u8 unk90A;
    /* 0x090B */ u8 unk90B;
    /* 0x090C */ s16 unk90C;
    /* 0x090E */ u8 unk90E;
    /* 0x090F */ u8 unk90F;
    /* 0x0910 */ u8 soundTimer;
    /* 0x0911 */ u8 unk911[0x3];
    /* 0x0914 */ s16 unk914;
    /* 0x0916 */ s16 unk916;
    /* 0x0918 */ u8 unk918[0x14];
    /* 0x092C */ Vec4 unk92C;
    /* 0x093C */ Vec4 centerPosition;
    /* 0x094C */ u8 unk94C[0x10];
    /* 0x095C */ Vec4 unk95C;
    /* 0x096C */ u8 unk96C[0x4];
    /* 0x0970 */ s32 unk970;
    /* 0x0974 */ Vec4 unk974;
    /* 0x0984 */ u8 unk984[0x1C];
    /* 0x09A0 */ BattleMessageQueue messageQueue;
    /* 0x09C3 */ u8 actionIndex;
    /* 0x09C4 */ u8 unk9C4[0xC24];
    /* 0x15E8 */ s16 unk15E8;
    /* 0x15EA */ s16 unk15EA;
    /* 0x15EC */ u8 unk15EC[0x4];
    /* 0x15F0 */ s16 unk15F0;
    /* 0x15F2 */ s16 unk15F2;
    /* 0x15F4 */ s16 unk15F4;
    /* 0x15F6 */ u8 unk15F6[0xA];
    /* 0x1600 */ BattleActionData* pActionData;
    /* 0x1604 */ u8 unk1604[0x66];
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
    if (along < 0) {
        return 0;
    }
    return (cross * cross - r * r < 0) & (dist + r >= along);
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80075888);

extern s32 D_80092654;
extern s32 D_80092658;

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80075A4C);

/* 83.62%: two residuals. The original stores D_80092654/58 AFTER the four
   arg-register loads (order: stack stores, arg loads, global stores, jal);
   this source emits the global stores before the stack stores. And the
   original's three `return 1` blocks are threaded into the bnez delay slots
   while gcc leaves the third as a j + li block after the last call. Tried:
   if/return chain, || chain (adds a saved reg), r = call(); if (r != 0)...,
   and `? 1 : 0` for the tail.
s32 func_80075A4C(u8* p, s32 a, s32 b, s32 c) {
    s32 r;

    D_80092654 = a;
    D_80092658 = b;
    r = func_80075750(*(s32*)(p + 0x10), *(s32*)(p + 0x18), *(s32*)(p + 0x0), *(s32*)(p + 0x8), a, b, c);
    if (r != 0) {
        return 1;
    }
    r = func_80075750(*(s32*)(p + 0x30), *(s32*)(p + 0x38), *(s32*)(p + 0x20), *(s32*)(p + 0x28), a, b, c);
    if (r != 0) {
        return 1;
    }
    r = func_80075750(*(s32*)(p + 0x0), *(s32*)(p + 0x8), *(s32*)(p + 0x20), *(s32*)(p + 0x28), a, b, c);
    if (r != 0) {
        return 1;
    }
    return func_80075750(*(s32*)(p + 0x10), *(s32*)(p + 0x18), *(s32*)(p + 0x30), *(s32*)(p + 0x38), a, b, c) ? 1 : 0;
}
*/

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
extern s32 D_8009292C;
extern u8 g_BattlePlayer2Vibration;
extern u8 g_BattlePlayer1Vibration;
extern u32 D_80091198[];
// One 0x14-byte record per battle slot; D_80096FBC, which the battle draw code
// also reads, is this table's +0x4 field.
typedef struct {
    /* 0x0 */ u32 value;
    /* 0x4 */ u8 percent;
    /* 0x5 */ u8 unk5[0xF];
} BattleSlotGauge; // Size: 0x14

extern BattleSlotGauge D_80096FB8[];
extern u8 D_80096FBC[];

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
        prod = p->unk15F4 * D_8009292C;
        flags = p->flags;
        flags &= ~0x80000;
        p->flags = flags;
        p->unk15F2 = prod >> 8;
    } while (0);
    flags &= 0x8000000;
    if (!flags) {
        BattleSetTextCursor(0x28, 0x28);
    } else {
        BattleSetTextCursor(0xF0, 0x28);
    }
    if (p->actionIndex != 0) {
        D_80096FB8[((p->flags >> 27) & 1)].value = D_80091198[p->actionIndex];
        pActionData = p->pActionData;
        D_80096FB8[((p->flags >> 27) & 1)].percent = (pActionData->points[p->actionIndex] * pActionData->scale) / 100;
    }
}

extern void BattleSetTextScale(s32 color);
extern void BattleDrawTextLeft(char* pStr);
extern s32 g_BattleMode;
extern s32 D_8009112C;
extern s32 D_80096FC4[];
extern s32 D_80096FC8[];
extern char D_8006FD04[];
extern char D_8006FD08[];
extern char D_8006FD10[];

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_8007661C);

/* 99.91%: instruction stream and size (428 bytes) are exact; only the frame
   differs, 0x38 vs 0x30 -- gcc reserves 8 unused bytes between buf and the saved
   regs (vars=24 vs retail's 16; buf sits at sp+0x10 in both). Bisected: the pad
   appears as soon as the mid-body BattleSetTextScale call is present, in either
   shape (ternary Cond ? 0xE7 : 0x100, or if/else into an s32 local) -- both emit
   retail's exact insns, and dropping that call alone brings vars back to 16, so
   it looks like a reload spill slot the allocator keeps. buf sizes 0x8/0xC/0x10/
   0x18 and both w/h declaration orders do not move it. permuter reports ZERO
   because it does not score stack offsets.
/*
void func_8007661C(u8* p) {
    char buf[0x10];
    s32 h;
    s32 w;
    s32 color;

    h = 0xA;
    w = 0x14;
    if (*(u32*)(p + 0xD0) & 0x8000000) {
        w = 0xB4;
    }
    if (g_BattleMode != 4) {
        h = 0xB2;
    }
    BattleSetTextCursor(w, h);
    Sprintf(buf, D_8006FD04, D_80096FB8[((*(u32*)(p + 0xD0) >> 27) & 1) * 5]);
    if (D_80096FB8[((*(u32*)(p + 0xD0) >> 27) & 1) * 5] == D_8009112C) {
        color = 0xE7;
    } else {
        color = 0x100;
    }
    BattleSetTextScale(color);
    BattleDrawTextLeft(buf);
    BattleSetTextScale(0x100);
    BattleSetTextCursor(w + 0x34, h);
    Sprintf(buf, D_8006FD08, ((u8*)D_80096FBC)[((((*(u32*)(p + 0xD0)) >> 23) >> 4) & 1) * 20]);
    BattleDrawTextLeft(buf);
    BattleSetTextCursor(w, h + 0x14);
    h = D_80096FC4[((*(u32*)(p + 0xD0) >> 27) & 1) * 5] >> 4;
    Sprintf(buf, D_8006FD10, h, D_80096FC8[((*(u32*)(p + 0xD0) >> 27) & 1) * 5] >> 4);
    BattleDrawTextLeft(buf);
}
*/

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
    if ((flags & 0x60000000) != 0x20000000 || (p->unk90A & 2)) {
        BattleResetMessageQueue(p);
        BattleQueueMessage(p, 5);
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80076884);

extern u8 D_800928C4;
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
   The permuter (3 min) reached base=326 best=120 only by moving `p->unk970 = 0`
   inside the if, which fixes the reload but costs retail's store placement.
*/
/*
s32 func_80077038(BattleActor* p) {
    u32 flags;

    if (p->unkC2 != 0) {
        p->unkC2 -= 0x10;
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
    if (p->unk916 != 0) {
        p->unk916--;
    }
    if (p->unk914 != 0) {
        p->unk914--;
    }
    if (p->unkC8 != 0) {
        p->unkC8--;
    } else {
        p->moveFrameCount = 0;
        p->flags = (p->flags & 0xFF8FFFFF) | 0x300000;
    }
    if (p->unkC3 != 0) {
        p->actionPhase = 4;
        p->unkC3--;
    } else if (p->actionPhase == 4) {
        p->actionPhase = 0;
    }
    if (p->moveTimer != 0) {
        p->moveTimer--;
    }
    if (p->gaugePoints != 0) {
        if (p->state & 1) {
            if ((p->flags & 0x60000000) == 0x60000000) {
                if (p->unk90A & 4) {
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
        switch (p->unk90E) {
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
    p->gaugeValue -= p->unk970;
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
    if (D_800928C4 != 0 && BattleCheckValueAboveScaledMax(p, 0x50) == 0 && (p->flags & 0x8000000)) {
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
        p->unkC2 = 0xF0;
    }
    p->gaugeLevel = 0;
    p->unk970 = 0;
    if (g_BattleMode != 4 && p->gaugeValue == 0 && g_BattleMode != 6 && !(p->flags & 0x800000)) {
        p->modelIndex = 0xD;
        p->prevPartIndex = 0xFF;
        p->state = 5;
        p->actionPhase = 0;
        p->flags |= 0x1000;
        p->vy -= 0x50;
        p->flags |= 0x400;
        if (p->flags & 0x8000000) {
            p->unk20 -= rcos(g_BattleFacingAngle) >> 9;
            p->unk28 -= rsin(g_BattleFacingAngle) >> 9;
        } else {
            p->unk20 += rcos(g_BattleFacingAngle) >> 9;
            p->unk28 += rsin(g_BattleFacingAngle) >> 9;
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
    p->unk20 += rcos(ang) >> shift;
    p->unk28 += rsin(ang) >> shift;
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
    p->unk916 = 0x28;
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
        mid = (p->unk92C.y - p->centerPosition.y) * 2 / 3 + p->centerPosition.y;
        if (p->unk974.y < mid) {
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
    ang = ratan2(p->position.x - p->unk974.x, p->position.z - p->unk974.z);
    dist = BattleVectorDistance2D(&p->position, p->pTarget);
    if (dist < BattleVectorDistance2D(&p->unk974, p->pTarget)) {
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
        if (p->unk916 >= 0x26 || p->moveFrameCount >= 4) {
            BattleActorBeginMove(p, ang, 0xA, 0xA0);
            mode = 0xF;
            p->stateTimer = 0;
            p->unk916 = 0;
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
   - the `p->unk914 = 0xF` block of the reset path is placed after the case-1
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
   p->unk52 with `/ 4096` (gcc's signed divide-by-power-of-2 sequence) while the
   shared tail uses `>> 16`. BattleDequeueMessage must be declared s32: with a u8
   return every caller gets an `andi v0,v0,0xff` retail does not have (verified:
   the callee still matches 100.00% as s32).
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
    if (p->unk914 != 0) {
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
            if (p->unk914 == 0) {
                pEntry = BattleActorSelectActionEntry(p, 1);
                if (!(p->state & 2)) {
                    if (pEntry[0] == 0) {
                        p->unk914 = 0xF;
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
            if (p->unk914 == 0) {
                pEntry = BattleActorSelectActionEntry(p, 0);
                if (!(p->state & 2)) {
                    if (pEntry[0] == 0) {
                        p->unk914 = 0xF;
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
            if (p->unk914 == 0 && p->pActionData->unk18 != 0 && p->typeId != 0x29) {
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
            p->unkC3 = 6;
            p->actionPhase = 4;
            break;
        }
    }
    p->unk52 = 0;
    p->flags &= ~4;
    switch (p->state) {
    case 0:
        p->unk58 = 0;
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
                p->unk44 += 0x40;
            }
            break;
        case 4:
            BattleActorSetFlag80000(p);
            break;
        }
        p->angleOffset = func_8008B650(p->angleOffset, p->unk58, 0x100);
        break;
    case 1:
        p->unk52 = p->distance * p->unk15F2 / 4096;
        switch (p->actionPhase) {
        case 0:
            p->modelIndex = 1;
            break;
        case 2:
            if (flag != 0) {
                p->unk44 += 0x40;
            }
            break;
        case 4:
            BattleActorSetFlag80000(p);
            break;
        }
        p->angleOffset = func_8008B650(p->angleOffset, p->unk58, 0x100);
        if ((p->flags & 0x60000000) == 0x20000000 && !(p->unk90A & 2)) {
            p->flags &= ~0x8000;
        }
        break;
    case 2:
        p->distance = 0;
        p->unk44 = 0;
        p->unk58 = 0;
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
    p->stepDistance = p->distance * p->unk15F0 * p->unk15F2 >> 16;
    if (!(p->flags & 0x8000)) {
        p->stateFlags &= ~0x40;
    }
    if (p->flags & 0x8000) {
        if (p->actionPhase == 0 && !(p->stateFlags & 0x40) && p->stepDistance >= 0x31 && (u32)(p->state - 2) >= 2) {
            if (BattleAccumulateGaugePoints(p, 0x20, 2) != 0) {
                p->stepDistance *= 2;
                p->unk52 *= 2;
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
extern u8 D_80092884;
extern s32 func_8008B650(s32 value, s32 max, s32 step);
extern u8 g_BattleCameraLerpFrames;

/* 89.19%: every call, both lerp limiter paths, the actionPhase==4 reset, the 0x40000
   cos/sin pair, the p->unk44 steer block, the D_80092850 target-follow block and
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
    p->unk95C = p->centerPosition;
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
        angle = -(p->unkC3 * 0x50) / 3;
        p->angle = target;
        p->facingAngle = target;
        p->angleOffset = 0;
        p->stepDistance = 0;
        p->unk94 = 0;
        p->unk8C = 0;
        p->unk88 = 0;
        p->unk90 = 0;
    } else {
        p->angle = p->facingAngle + p->angleOffset;
    }
    if (p->actionPhase == 2) {
        if ((p->flags & 1) || (p->state & 2)) {
            p->unk90C = target;
        }
        p->flags = (p->flags | 0x100) & 0xFFF7FFFF;
        p->angle = p->unk90C;
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
    if (p->unk44 != 0 && p->position.y == p->floorY) {
        p->unk44 = p->unk44 * 0xA0 / 256;
        p->unk30 = -(rcos(target) * p->unk44) >> 12;
        p->unk38 = -(rsin(target) * p->unk44) >> 12;
        p->vx += p->unk30;
        p->vy += p->unk34;
        p->vz += p->unk38;
    }
    if (D_80092850 >= 0x281 && D_80092884 != 0) {
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
        p->unk20 = p->unk20 * alpha / 256;
        p->unk28 = p->unk28 * alpha / 256;
    }
    p->vx += p->unk20;
    p->vy += p->unk24;
    p->vz += p->unk28;
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
    side1 = dz * pEdges->unk92C.x + dx * pEdges->unk92C.z + pFrom->x * pTo->z - pTo->x * pFrom->z;
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
            s32 dist92C = BattleVectorDistance2D(&pEdges->unk92C, pFrom);
            if (BattleVectorDistance2D(&pEdges->centerPosition, pFrom) < dist92C) {
                *ppEdge = &pEdges->unk92C;
            } else {
                *ppEdge = &pEdges->centerPosition;
            }
            return 0;
        }
        *ppEdge = &pEdges->unk92C;
        return 1;
    }
    if (prod2 < 0) {
        *ppEdge = &pEdges->centerPosition;
        return 2;
    }
    {
        s32 dist92C = BattleVectorDistance2D(&pEdges->unk92C, pFrom);
        if (dist92C < BattleVectorDistance2D(&pEdges->centerPosition, pFrom)) {
            *ppEdge = &pEdges->unk92C;
        } else {
            *ppEdge = &pEdges->centerPosition;
        }
    }
    return 3;
}
