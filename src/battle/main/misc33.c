#include "common.h"
#include "system/memory.h"
#include "psyq/libgte.h"
#include "field/actor.h"

// One battle sprite: the transform steps at 0x40..0x48 that BattleStepActorFadeOut
// advances, the clip colour at 0x68..0x6A, the model parts at 0x74/0x7C and the
// direction/target pair at 0xA0..0xA4 that BattleAngleToTarget measures. Every
// function in this unit drives one of these.
typedef struct {
    /* 0x00 */ u8 unk0[0x2];
    /* 0x02 */ s16 unk2;
    /* 0x04 */ u8 unk4[0x6];
    /* 0x0A */ s16 unkA;
    /* 0x0C */ void (*unkC)();
    /* 0x10 */ u8 unk10[0x2C];
    /* 0x3C */ u16 unk3C;
    /* 0x3E */ u8 unk3E[0x2];
    /* 0x40 */ s32 unk40;
    /* 0x44 */ s32 unk44;
    /* 0x48 */ s32 unk48;
    /* 0x4C */ u8 unk4C[0x14];
    /* 0x60 */ s32 unk60;
    /* 0x64 */ u16 unk64;
    /* 0x66 */ u8 unk66[0x2];
    /* 0x68 */ u8 unk68;
    /* 0x69 */ u8 unk69;
    /* 0x6A */ u8 unk6A;
    /* 0x6B */ u8 unk6B[0x9];
    /* 0x74 */ void *unk74;
    /* 0x78 */ u8 unk78[0x4];
    /* 0x7C */ s32 *unk7C;
    /* 0x80 */ u8 unk80[0x20];
    /* 0xA0 */ u16 unkA0;
    /* 0xA2 */ u8 unkA2[0x2];
    /* 0xA4 */ u16 unkA4;
    /* 0xA6 */ u8 unkA6[0x2];
    /* 0xA8 */ u32 unkA8;
    // func_800BF3E8 reads byte 3 as a signed byte (\`lb 0xAF\`), so the 0xAC word and
    // the 0xAF byte are one storage read at two widths.
    /* 0xAC */ union {
        /* 0xAC */ u32 word;
        /* 0xAC */ u8 bytes[0x4];
        /* 0xAC */ s_char sbytes[0x4];
    } unkAC;
    /* 0xB0 */ s8 unkB0;
} BattleActorSprite; // Size: 0xB4

typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ BattleActorSprite *unk4;
    /* 0x08 */ void (*unk8)();
    /* 0x0C */ u8 unkC[0x10];
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ u8 unk20[0x4];
    /* 0x24 */ s32 unk24;
    /* 0x28 */ u32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ s32 unk34;
    /* 0x38 */ u8 unk38[0xC];
    /* 0x44 */ s32 unk44;
    /* 0x48 */ u8 unk48;
    /* 0x49 */ u8 unk49;
    /* 0x4A */ u8 unk4A;
    /* 0x4B */ u8 pad4B;
    /* 0x4C */ BattleActorSprite *unk4C;
} BattleMisc33State; // Size: 0x50

extern BattleMisc33State *D_800C3610;
extern s32 D_800C360C;

// A 0x48-byte row of the per-battle actor table that D_800C3EB0 + 0x108 starts;
// misc25 indexes the same rows as D_800C4000[D_800C360C - 1].
typedef struct {
    /* 0x00 */ u8 unk0[0x46];
    /* 0x46 */ u16 unk46;
} BattleActorRow; // Size: 0x48

extern BattleActorRow D_800C3FB8[];
extern BattleActorSprite *D_800D363C;
extern u8 D_800C3780;
extern s16 D_800C3752[];
extern s32 D_800D3630;
extern u8 *D_800D2F5C;
extern s16 D_800D3634;
extern s16 D_800D3678;
extern s32 BattleCollectMaskedSlotActors(u16 mask, void **pOut, s32 arg2);
extern void func_800B9F78();
extern s32 D_800C3E20;
extern u16 D_800D2E54;
extern u8 D_800D3350;
extern u8 D_800C3784[];
extern u8 D_800C3EB0[];
extern u32 D_800C37A4[];
extern u8 D_800591AC;
extern s32 D_80059464;


extern s16 SpriteComputeAngleFromPoints(DVECTOR, DVECTOR);
extern s32 func_800C0FAC(s32);
extern u8 SpriteColorAddClamp(u8, s16);
extern s32 BattleComputeVectorDistance2D(DVECTOR vec0, DVECTOR vec1);
extern void SpriteSetDirection(BattleActorSprite *p, s16 arg1);
extern void func_800223B0(BattleActorSprite *p, s16 arg1);
extern void SpritePlayAnimation(BattleActorSprite *p, s32 arg1);

// One 0x18-byte part of a battle sprite: the record ResourceHelperGetSpriteList
// converts a menu dirent into, with its width/height at 0x6/0x7.
typedef struct {
    /* 0x00 */ u16 x;
    /* 0x02 */ u16 y;
    /* 0x04 */ u8 u;
    /* 0x05 */ u8 v;
    /* 0x06 */ u8 width;
    /* 0x07 */ u8 height;
    /* 0x08 */ u8 unk8[0x10];
} BattleSpritePart; // Size: 0x18

// What WorkListsAddTasks allocates for the fade-out task (0x130 bytes) and what
// func_800BE1C4 draws: a rotation at 0x38, the scale steps at 0x40 that
// BattleStepActorFadeOut advances, the clip colour at 0x68, and the part list at
// 0x70 whose count is at 0x6C.
typedef struct {
    /* 0x00 */ u8 unk0[0x38];
    /* 0x38 */ SVECTOR rot;
    /* 0x40 */ VECTOR scale;
    /* 0x50 */ u8 unk50[0x10];
    /* 0x60 */ s32 unk60;
    /* 0x64 */ s32 unk64;
    /* 0x68 */ u8 unk68;
    /* 0x69 */ u8 unk69;
    /* 0x6A */ u8 unk6A;
    /* 0x6B */ u8 unk6B;
    /* 0x6C */ s32 unk6C;
    /* 0x70 */ BattleSpritePart parts[8];
} BattleSpriteBody; // Size: 0x130

extern BattleSpriteBody *WorkListsAddTasks(s32 allocSize, void *pData, void *pTimerCallback,
                                          void *pWorkCallback, void *pOnFreeCallback);
extern s32 func_800BE6E8(s32 value, u8 *pBuf, s32 n, u8 started, s32 base);
extern s32 ResourceHelperGetSpriteList(u8 *pDir, u32 index, BattleSpritePart *pOut, s16 x, s16 y);
extern s32 func_800BD3AC(BattleActorSprite *p, s32 arg1, s32 arg2);
extern void func_800BE0DC(void);
extern void func_800BE790(void);
extern s32 BattleGetAdjustedWaitCountdown(void);
extern void func_800C1140(s32);
extern void func_800B8D7C();
extern int ArchiveSetIndex(int directoryIndex, int entryIndex);
extern int ArchiveDecodeAlignedSize(u32 entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void *pBuffer, u32 arg2, u32 flags);
extern void *D_800C3618;
extern u32 D_800C361C;

void BattleStepActorFadeOut(BattleActorSprite *p) {
    p->unk3C = p->unk3C + p->unk64;
    p->unk40 += 0x330;
    p->unk44 += 0x330;
    p->unk48 += 0x330;
    p->unk68 = SpriteColorAddClamp(p->unk68, -4);
    p->unk69 = SpriteColorAddClamp(p->unk69, -4);
    p->unk6A = SpriteColorAddClamp(p->unk6A, -4);
    if (--p->unk60 == 0) {
        p->unkC(p);
    }
}
// The GTE geom-output block D_800C3760 doubles as a matrix: CompMatrix writes
// into it and its t[2] word is the geom screen offset (misc32 reads the same
// 0x1C word as D_800C377C).
typedef struct {
    /* 0x00 */ u8 unk0[0x1C];
    /* 0x1C */ s32 geomScreen;
} BattleGeomMatrix; // Size: 0x20

extern BattleGeomMatrix D_800C3760;
extern void func_800BD810(u8 *pEntry, u8 *pData);

void func_800BE1C4(u8 *pTask) {
    u8 *pData;
    u8 *pEntry;
    MATRIX mtx;
    SVECTOR padVec; // unused: reproduces retail's 0x80 frame
    SVEC vec;
    VECTOR sxy;
    VECTOR scale;
    s32 ofx;
    s32 ofy;
    s32 i;
    s32 j;

    pData = *(u8 **)(pTask + 0x4);
    ReadGeomOffset(&ofx, &ofy);
    sxy.vx = (0xA0 - ofx) * 2;
    sxy.vy = (0x46 - ofy) * 2;
    sxy.vz = ReadGeomScreen();
    D_800C3760.geomScreen = ReadGeomScreen();
    SpriteCopySVec(&vec, (SVEC *)(pData + 0x38));
    scale.vx = *(s32 *)(pData + 0x40);
    scale.vy = *(s32 *)(pData + 0x44);
    scale.vz = *(s32 *)(pData + 0x48);
    for (i = 0; i != 6; i++) {
        RotMatrix((SVECTOR *)&vec, &mtx);
        TransMatrix(&mtx, &sxy);
        CompMatrix((MATRIX *)&D_800C3760, &mtx, &mtx);
        ScaleMatrix(&mtx, &scale);
        SetRotMatrix(&mtx);
        SetTransMatrix(&mtx);
        j = 0;
        pEntry = pData + 0x70;
        while (j != *(s32 *)(pData + 0x6C)) {
            func_800BD810(pEntry, *(u8 **)(pData + 0x68));
            j++;
            pEntry += 0x18;
        }
        vec.z -= 5;
        scale.vx -= 0x330;
        scale.vy -= 0x330;
        scale.vz -= 0x330;
    }
}
void BattleCreateNumberSpriteTask(s32 value) {
    BattleSpriteBody *pBody;
    u8 digits[6];
    s32 i;
    s32 mode;
    s32 step;
    s32 tex;

    pBody = WorkListsAddTasks(0x130, NULL, BattleStepActorFadeOut, func_800BE1C4, NULL);
    step = ((rand() & 3) - 2) * 8;
    pBody->unk64 = -step;
    if (pBody->unk64 == 0) {
        pBody->unk64 = 6;
    }
    pBody->unk60 = 0x20;
    pBody->unk6B = 0x2E;
    pBody->scale.vx = 0x2000;
    pBody->scale.vy = 0x2000;
    pBody->scale.vz = 0x2000;
    mode = D_800D3630;
    pBody->unk6C = 0;
    pBody->rot.vx = 0;
    pBody->rot.vy = 0;
    pBody->rot.vz = 0;
    switch (mode) {
    case 0xB:
        pBody->unk68 = 0;
        pBody->unk69 = 0;
        pBody->unk6A = 0x80;
        break;
    case 3:
        pBody->unk68 = 0x80;
        pBody->unk69 = 0;
        pBody->unk6A = 0x80;
        break;
    case 2:
        pBody->unk68 = 0;
        pBody->unk69 = 0x80;
        pBody->unk6A = 0;
        break;
    default:
        pBody->unk68 = 0x80;
        pBody->unk69 = 0x80;
        pBody->unk6A = 0x80;
        break;
    }
    func_800BE6E8(value, digits, 5, 0, 0);
    tex = D_800C3752[digits[0]];
    pBody->unk6C = 0;
    i = 0;
    while (i != digits[0]) {
        pBody->unk6C += ResourceHelperGetSpriteList(D_800D2F5C, digits[i + 1] + 0x72,
                                                    &pBody->parts[pBody->unk6C], tex, -8);
        i++;
        tex += 10;
    }
    if (pBody->unk6C != 0) {
        for (i = 0; i != pBody->unk6C; i++) {
            pBody->parts[i].width--;
            pBody->parts[i].height--;
        }
    }
}
void BattleAttachActorAnimScripts(s32 index, s32 arg1, s32 arg2, s32 arg3) {
    BattleActorSprite *pSprite;
    BattleMisc33State *pState;
    u8 *pBase;
    s32 anim;

    D_80059464 = 0;
    D_800591AC = 1;
    pBase = D_800C3EB0;
    pSprite = *(BattleActorSprite **)(pBase + index * 4 + 0x8C8C);
    D_800C3780 = 1;
    if (pSprite != NULL) {
        anim = pSprite->unkAC.sbytes[3];
        SpritePlayAnimation(pSprite, 0xA);
        pState = D_800C3610;
        D_800C3610 = (BattleMisc33State *)1;
        if (arg1 != 0) {
            func_800BD3AC(pSprite, arg1, 0);
        }
        if (arg2 != 0) {
            func_800BD3AC(pSprite, arg2, 1);
        }
        if (arg3 != 0) {
            func_800BD3AC(pSprite, arg3, 0xA);
        }
        D_800C3610 = pState;
        while (BattleGetAdjustedWaitCountdown() != 0) {
            func_800BE790();
        }
        if (pSprite->unkAC.sbytes[3] == 0xA) {
            while (pSprite->unkAC.sbytes[3] == 0xA) {
                func_800BE790();
            }
        }
        SpritePlayAnimation(pSprite, anim);
    }
    D_800C3780 = 0;
    func_800BE0DC();
    D_80059464 = 0;
    D_800591AC = 0;
}
void BattleWriteHexDigits(s32 value, u8 *pBuf, s32 n, s32 base) {
    s32 i;

    for (i = 0; i != n; i++) {
        s32 last = n - 1;

        pBuf[i + 1] = D_800C3784[(value >> ((last - i) * 4)) & 0xF] + base;
    }
    pBuf[0] = n;
}
/* best 88.33%: everything matches but the loop-entry order.  Retail emits
   `beqz a2` with `i = 0` in its delay slot, so its RTL had the `last = n - 1`
   local hoisted AFTER the entry test; declaring `last` before the loop puts it
   in front, and inlining `n - 1 - i` instead makes gcc hoist `n - 1` badly
   (73.06%).  Two instructions differ, no missing or extra insn.
void BattleWriteHexDigits(s32 arg0, u8 *p, s32 n, s32 arg3) {
    s32 i;
    s32 last;

    last = n - 1;
    for (i = 0; i != n; i++) {
        p[i + 1] = D_800C3784[(arg0 >> ((last - i) * 4)) & 0xF] + arg3;
    }
    p[0] = n;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BE6E8);
/* best 82.71%: retail walks ONE pointer that is decremented in the loop-test
   block's branch delay slot, so both `lw` reads are at offset 0(v1) and the
   pointer is restored with a dead `addiu v1,v1,4` after the loop; gcc keeps the
   source pointer as the base here and reads the quotient at -4(v1), and it also
   emits a `beqz` loop guard instead of retail's `j` into the test block.  The
   divisors really are tab[n-1] (quotient) and tab[n] (remainder) -- the two
   divu are NOT CSE'd, so a single `divisor` local (which merges them into one
   div with mflo+mfhi) is wrong.  `(u32)` on both divisions is required: retail
   uses divu, and the table D_800C37A4 holds u32.
s32 func_800BE6E8(s32 value, u8 *pBuf, s32 n, u8 started, s32 base) {
    u8 *pDst;
    u8 digit;
    s32 count;
    u32 *pTab;

    count = 0;
    pDst = pBuf + 1;
    if (value < 0) {
        count = 1;
        pBuf[1] = '-';
        pDst = pBuf + 2;
        value = -value;
        n--;
    }
    pTab = &D_800C37A4[n];
    while (n != 0) {
        digit = (u32)value / pTab[-1];
        if (digit != 0) {
            started = 1;
        }
        n--;
        if (started != 0) {
            *pDst = digit + base;
            pDst++;
            count++;
        }
        value = (u32)value % *pTab;
        pTab--;
    }
    *pDst = value + base;
    pBuf[0] = count + 1;
    return count + 1;
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BE790);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEB04);
typedef struct {
    /* 0x00 */ u16 cur;
    /* 0x02 */ u16 pressed;
    /* 0x04 */ u16 released;
    /* 0x06 */ u16 unk6;
} BattlePadFrame; // Size: 0x8

typedef struct {
    /* 0x0000 */ u8 unk0[0xC58];
    /* 0xC58 */ u16 cur[2];
    /* 0xC5C */ u16 pressed[2];
    /* 0xC60 */ u16 released[2];
    /* 0xC64 */ BattlePadFrame history[4];
} BattlePadState; // Size: 0xC84

extern int ControllerGetButtonState(int controllerIndex);
extern u16 D_800D30E4;

extern u16 D_80059494;
extern void Vsync(s32 mode);
extern void BattleUpdatePadState(void);

void BattlePollPadAndSkipWaitOnStart(void) {
    u8 *pStateBase;
    BattlePadState *pState;

    BattleUpdatePadState();
    pStateBase = D_800C3EB0;
    pState = (BattlePadState *)(pStateBase + 0x8000);
    if (pState->cur[0] & 0x100) {
        Vsync(8);
        D_80059494 = 0;
    }
}
void BattleUpdatePadState(void) {
    u8 *pStateBase;
    BattlePadState *pState;
    s32 cur;
    u16 prev;
    u16 prev2;

    /* the mask is what makes gcc narrow the call result */
    cur = ControllerGetButtonState(0) & 0xFFFF;
    /* a symbol plus literal folds into one lui/addiu; retail adds 0x8000 in a register */
    pStateBase = D_800C3EB0;
    pState = (BattlePadState *)(pStateBase + 0x8000);
    prev = pState->cur[0];
    pState->cur[0] = cur;
    pState->pressed[0] = ~prev & cur;
    pState->released[0] = prev & ~cur;

    cur = ControllerGetButtonState(1);
    prev2 = pState->cur[1];
    pState->pressed[1] = ~prev2 & cur;
    pState->cur[1] = cur;
    pState->released[1] = pState->cur[0] & ~cur;

    if (pState->cur[0] == pState->history[0].cur) {
        return;
    }
    pState->history[3] = pState->history[2];
    pState->history[2] = pState->history[1];
    pState->history[1] = pState->history[0];
    pState->history[0].cur = pState->cur[0];
    pState->history[0].pressed = pState->pressed[0];
    pState->history[0].released = pState->released[0];
    pState->history[0].unk6 = D_800D30E4;
}

void BattleResetStateGlobals(void) {
    D_800C3E20 = 0;
    D_800C3610 = NULL;
    D_800D2E54 = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BED4C);
/* best 85.18% (144 bytes, retail 156).  The 12 missing bytes are one extra reload:
   after the BattleSetStateStep call retail does `lw v0,%lo(D_800C3610)($s0)` and never
   uses it, so its D_800C3610 address pseudo is live across the call and lands in $s0
   (with the constant 1 in $s1 where ours uses $s0); ours keeps the address in $a1.
   A plain dead `pState = D_800C3610;` after the call is deleted by gcc, so what retail
   actually read is still unknown.  Retail store order: unk8, D_800C3610, D_800C360C,
   unk4A, unk48, unk30, unk49, unk2C, unk4, unk34 (in the call's delay slot).
void func_800BED4C(void) {
    BattleMisc33State *pState;

    pState = HeapAlloc(0x50, 0);
    pState->unk8 = func_800B9F78;
    D_800C3610 = pState;
    D_800C360C = 0;
    pState->unk4A = 0;
    D_800C3610->unk48 = 1;
    D_800C3610->unk30 = 0;
    D_800C3610->unk49 = 0;
    D_800C3610->unk2C = 1;
    D_800C3610->unk4 = NULL;
    D_800C3610->unk34 = 0;
    BattleSetStateStep(0);
    pState = D_800C3610;
    D_80059464 = 0;
    D_800591AC = 1;
}
*/
void BattleDestroyState(void) {
    HeapFree(D_800C3610);
    D_80059464 = 0;
    D_800C3610 = NULL;
    D_800591AC = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEE2C);
/* Not convertible: retail switches the stack pointer into the 0x1000-byte
   HeapAlloc block for the BattleQueueActorCommand call (sw $sp,0($t0); addiu $t0,-4;
   addu $sp,$t0, then addiu $sp,4; lw $sp,0($sp)), which the original source
   wrote as inline asm -- the same shape as GfxLoadImageAccelerated in
   system/graphics.  No C produces it. */
s32 BattleCollectMaskedSlotActors(u16 mask, void **pOut, s32 arg2) {
    u8 *pBase;
    BattleActorSprite *pSprite;
    s32 i;
    s32 count;

    pBase = D_800C3EB0;
    i = 0;
    count = 0;
    do {
        if ((mask & 1) != 0) {
            pSprite = *(BattleActorSprite **)(pBase + i * 4 + 0x8C8C);
            if (pSprite != NULL) {
                pSprite->unk74 = (void *)arg2;
                pOut[count] = pSprite;
                count++;
            }
        }
        i++;
        mask >>= 1;
    } while (i != 0xB);
    pOut[count] = NULL;
    return count;
}
s16 BattleAngleToPoint(BattleActorSprite *p0, BattleActorSprite *p1) {
    DVECTOR vec0;
    DVECTOR vec1;
    s32 c0;
    s32 c1;
    s32 c2;
    s32 c3;

    c0 = p0->unk2;
    vec0.vx = c0;
    c1 = p0->unkA;
    vec0.vy = c1;
    c2 = p1->unk2;
    vec1.vx = c2;
    c3 = p1->unkA;
    vec1.vy = c3;
    return SpriteComputeAngleFromPoints(vec1, vec0);
}
s16 BattleAngleToTarget(BattleActorSprite *p) {
    DVECTOR vec0;
    DVECTOR vec1;
    s32 c0;
    s32 c1;
    s32 c2;
    s32 c3;

    c0 = p->unk2;
    vec0.vx = c0;
    c1 = p->unkA;
    vec0.vy = c1;
    c2 = p->unkA0;
    vec1.vx = c2;
    c3 = p->unkA4;
    vec1.vy = c3;
    return SpriteComputeAngleFromPoints(vec1, vec0);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BEFF4);
/* best 94.02%: every instruction is right but one register tie.  Retail keeps the
   new sprite pointer in $a2 and copies it with `move a0,a2` in the SpritePlayAnimation
   jal delay slot; ours allocates it straight to $a0 (so the slot gets a nop) and the
   sensor-table base lands in $a1 instead of $v1.  The sprite-table entry is the same
   0x1C-byte stride D_800C3EB0 table that func_800BF3E8 walks, and the 4-bit key is
   the (unkA8>>30) | ((unkAC.word&3)<<2) pairing seen in BattleLoadActorArchiveFile.  A permuter job.
void func_800BEFF4(s32 actorIndex) {
    BattleActorSprite *pSprite;
    u8 *pBase;
    u32 part;

    pSprite = D_800C3610->unk4;
    if (pSprite != NULL && D_800C3610->unk24 != actorIndex) {
        part = pSprite->unkA8 >> 30;
        pBase = D_800C3EB0;
        if (*(u8 *)(pBase + (((pSprite->unkAC.word & 3) << 2) | part) * 0x1C + 0x7) == 0) {
            SpritePlayAnimation(pSprite, pSprite->unkB0);
        }
    }
    D_800C3610->unk24 = actorIndex;
    pBase = D_800C3EB0;
    D_800C3610->unk4 = *(void **)(pBase + actorIndex * 4 + 0x8C8C);
}
*/

void BattleSetStateStep(s32 arg0) {
    D_800C3610->unk1C = arg0;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BF0C4);
void BattleFaceTargetAndMeasureDistance(BattleActorSprite *p, s32 arg1) {
    DVECTOR vec0;
    DVECTOR vec1;
    s32 c0;
    s32 c1;
    s32 c2;
    s32 c3;

    c0 = p->unk2;
    vec0.vx = c0;
    c1 = p->unkA;
    vec0.vy = c1;
    c2 = p->unkA0;
    vec1.vx = c2;
    c3 = p->unkA4;
    vec1.vy = c3;
    D_800C3610->unk44 = BattleComputeVectorDistance2D(vec0, vec1);
    SpriteSetDirection(p, BattleAngleToTarget(p));
    func_800223B0(p, BattleAngleToTarget(p));
    SpritePlayAnimation(p, arg1);
    BattleSetStateStep(6);
}
void BattleLoadActorArchiveFile(BattleActorSprite *pSprite) {
    s32 entry;
    void *pBuffer;
    u32 part;

    func_800B8D7C();
    ArchiveSetIndex(0x2C, 1);
    entry = *pSprite->unk7C;
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(entry), 1);
    ArchiveReadFileToBuffer(entry, pBuffer, 0, 0x80);
    D_800C3618 = pBuffer;
    part = pSprite->unkA8 >> 30;
    D_800C361C = ((pSprite->unkAC.word & 3) << 2) | part;
}
s32 BattleLoadSoundSeds(void) {
    s32 ret;

    if (D_800D3350 == 0) {
        ret = func_800C0FAC(D_800C3618);
        D_800D3350 = 1;
    }
    return ret;
}
void BattleFreeSoundSeds(void) {
    if (D_800D3350 != 0) {
        func_800C1140(D_800C3618);
        D_800D3350 = 0;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc33", func_800BF3E8);
/* best 79.86%: everything below is retail's instruction set except one `lw`
   (retail re-reads D_800D363C for the pState->unk4C store: `lw a0,0(s2)` after
   the two 0x74 stores, so its address lives in $s2 and the loaded head in $s0;
   ours CSEs the third read onto s0 and needs no second saved register) and the
   unkA8/unkAC load order.  The two `lhu` of the mask ARE two separate C
   expressions -- one per statement, with the D_800D3634 store between them --
   and the call argument must be the later statement's load, so the argument
   expression is written last.  The table row is 0x48 bytes at D_800C3EB0+0x108
   (misc25 reaches the same rows as D_800C4000[D_800C360C - 1]), field 0x46.
void func_800BF3E8(BattleActorSprite *p) {
    BattleActorSprite *pLast;
    BattleMisc33State *pState;
    s32 count;

    D_800D3634 = D_800C3FB8[D_800C360C].unk46;
    count = BattleCollectMaskedSlotActors(D_800C3FB8[D_800C360C].unk46, (void **)&D_800D363C, (s32)p);
    D_800D3678 = count;
    if (count == 0) {
        D_800D363C = p;
    }
    pLast = D_800D363C;
    pState = D_800C3610;
    p->unk74 = D_800D363C;
    ((BattleActorSprite *)D_800D363C)->unk74 = p;
    pState->unk4C = D_800D363C;
    pState->unk28 = ((pLast->unkAC.word & 3) << 2) | (pLast->unkA8 >> 30);
    func_800223B0(p, BattleAngleToPoint(p, p->unk74));
    if (pLast->unkAC.sbytes[3] != 0x15) {
        func_800223B0(pLast, BattleAngleToPoint(p->unk74, p));
    }
}
*/
