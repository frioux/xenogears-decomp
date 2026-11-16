#include "common.h"
#include "psyq/libgpu.h"

extern u8 D_80092954[];

extern void BattleSetScreenMode(int mode);
extern void SystemStringFree(void* pString);

void BattleFreeMessageString(void) {
    BattleSetScreenMode(3);
    SystemStringFree(&D_80092954[0]);
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

extern s32 D_8009867C;
extern s32 D_80098680;
extern s32 D_80098684;
extern s32 D_800925F4;
extern s32 D_800925F8;
extern s32 D_800925FC;
extern s32 D_80097058;
extern s32 D_800970E0;
extern s32 D_80098774;
extern s32 D_800987FC;

/* 66.68%: original keeps &D_8009867C in $s0 across the first call and reads and
   stores through it; ours rematerialises the address (lui at / lw / sw). The same
   C without the pointer local scores the same; -G0 65.72, gcc 2.6.0 65.72.
   func_80070808's body shows D_8009867C/80/84 are the vx/vy/vz of one VECTOR:
   it reads 0x0(a0) with 0x0(s0) and stores the result back through 0x0(s0).
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80070808);

/* 66.68% attempt:
void func_80070808(s32* p, s32 step) {
    s32* pX = &D_8009867C;

    *pX += BattleRoundDivAwayFromZero(p[0], *pX, step);
    D_80098684 += BattleRoundDivAwayFromZero(p[2], D_80098684, step);
    D_80098680 += BattleRoundDivAwayFromZero(p[1], D_80098680 + D_800925F4, step);
}
*/

extern s32 D_8009871C;
extern s32 D_80098720;
extern s32 D_80098724;
extern void func_800828F8(s32* pA, s32* pB, s32 limit);

/* 74.00%: same missing shape as func_80070808, the original keeps &D_8009871C in
   $s2 across all four calls and reaches it through the register; ours re-forms it
   with lui/addiu each time. D_8009871C/20/24 are also one VECTOR seen by
   func_800708C4 as 0x0/0x4/0x8(s2).
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_800708C4);

/* 74.00% attempt:
void func_800708C4(s32* p, s32 step) {
    s32 values[3];

    values[0] = BattleRoundDivAwayFromZero(p[0], D_8009871C, step);
    values[1] = BattleRoundDivAwayFromZero(p[1], D_80098720, step);
    values[2] = BattleRoundDivAwayFromZero(p[2], D_80098724, step);
    func_800828F8(&D_8009871C, values, 0x3D00);
    D_8009871C += values[0];
    D_80098720 += values[1];
    D_80098724 += values[2];
}
*/

extern VECTOR D_80097010;
extern VECTOR D_8009872C;
extern VECTOR D_80099078;
extern s32 D_80097014;
extern s32 D_80097018;
extern s32 D_80098780;
extern s32 D_80097064;
extern s32 D_80098730;
extern s32 D_80098734;
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
   &D_8009867C in $s0 across the rcos call (sw v0,0(s0), then lw v0,0(s0)) where
   ours rematerialises the symbol as a direct %lo store. A pointer local does
   not stop that. Do not re-derive the shape: the do-while around the
   D_800925F4 store is load-bearing (91.24 -> 92.82, and it restores retail's
   byte count), and the y local is what stops gcc folding
   (D_80098730 - 0x20) - D_800925F4 into D_80098730 - (D_800925F4 + 0x20).
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
        func_80083738(&D_8009872C, &D_80097010);
        break;
    case 1:
        vec = D_8009872C;
        goto place_message;
    case 2:
        vec = D_80097010;
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
        vec.vx = D_8009872C.vx + ((rcos(D_80098780 + 0xA80) * 0xD0) >> 12);
        y = D_80098730 - 0x20;
        vec.vy = y - D_800925F4;
        vec.vz = D_80098734 + ((rsin(D_80098780 + 0xA80) * 0xD0) >> 12);
        func_800708C4(&vec, 0x46);
        offset = func_80082488(&D_8009871C, 0) - 0x40;
        offset -= D_800925F4;
        if (offset < D_80098720) {
            D_80098720 = offset;
        }
        break;
    case 5:
        pBase = &D_8009867C;
        *pBase = D_80097010.vx;
        D_80098680 = D_80097014 - 0xC0;
        D_80098684 = D_80097018;
        D_8009871C = *pBase + ((rcos(D_80097064 + 0x900) * 0xE0) >> 12);
        D_80098720 = D_80097014 - 0xD0;
        D_80098724 = D_80098684 + ((rsin(D_80097064 + 0x900) * 0xE0) >> 12);
        break;
    }
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80070C7C);

void BattleSetValueAndClearFlags(s32 value) {
    D_800925F8 = value;
    D_80098774 = 0;
    D_80097058 = 0;
    D_800925FC = 0;
    D_800987FC &= 0xFFFF7FFF;
    D_800970E0 &= 0xFFFF7FFF;
}

extern s32 D_80092934;

typedef struct BattleScriptBlock BattleScriptBlock;

// Partial view of the battle actor record these functions receive; only the
// fields src/battling/main/misc1 reads are named. The record is far larger
// than this (it holds the actor's script pointer at +0x8FC).
typedef struct {
    /* 0x000 */ VECTOR position;
    /* 0x010 */ u8 unk10[0x24];
    /* 0x034 */ VECTOR unk34;
    /* 0x044 */ u8 unk44[0x4];
    /* 0x048 */ u32 unk48;
    /* 0x04C */ u8 unk4C[0xC];
    /* 0x058 */ s32 unk58;
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
        p->unk58 = 0x800 - D_80092934;
    } else {
        p->unk58 = 0xC00 - D_80092934;
    }
    p->unk48 = 0xFF;
    p->unkCE = 0;
    return 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_8007107C);

extern s32 D_800925E0;
extern s32 D_800925E4;
extern u8 D_800928A0;
extern DR_TPAGE D_800929E4[];

// The packet BattleQueueSpriteAndTpagePrims fills and queues: its primitive tag
// sits at +0x74, the word the battle stores alongside it at +0x7C.
typedef struct {
    /* 0x00 */ u8 unk0[0x74];
    /* 0x74 */ u_long tag;
    /* 0x78 */ u8 unk78[0x4];
    /* 0x7C */ s32 unk7C;
} BattleSpritePacket; // Size: unknown

extern BattleSpritePacket* D_80092868;

void BattleQueueSpriteAndTpagePrims(void* pOt) {
    BattleSpritePacket* pPacket;

    pPacket = D_80092868;
    pPacket->unk7C = D_800925E0 | (D_800925E4 << 16);
    AddPrim(pOt, &pPacket->tag);
    AddPrim(pOt, &D_800929E4[D_800928A0]);
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
   gcc re-forms the second load's base (lui v1,%hi(D_800929E4); lw v1,4(v1)) where
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
    pTPage = (DR_TPAGE*)&D_800929E4[0];
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

extern s32 D_800925D4;
extern s32 D_800925D8;
extern u8 D_8009868C[];
extern s32 func_80032F54(void* pString, s16 a, s16 b, s16 c, s16 d, s16 e, s16 f);

void BattleInitMessageWindow(void) {
    D_800925D8 = -1;
    D_800925D4 = 0;
    func_80032F54(&D_8009868C[0], 0x140, 0x30, 0x1C, 0x9A, 0x40, 4);
}

extern u8 D_80092608;
extern s16 D_80098698;
extern s16 D_80098692;
extern s32 D_8009105C[];
extern s32 D_800925E8;
extern s32 D_800925E0;
extern s32 D_800925EC;
extern s32 D_800925E4;
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
    D_800925E0 = 0xA0;
    D_800925EC = 0x6D;
    D_800925E4 = 0x6D;
    D_80092600 = 0;
    D_800987E0 = D_800987E8;
    D_800970C4 = D_800970CC;
}
*/

extern u8 D_80099D9D;
extern u8 D_80099D9E;
extern s32 D_800928C8;
extern u8 D_80092884;
extern u8* D_80092880;
extern u8 D_800929BC;
extern s32 D_800925DC;
extern u8 D_80092604;
extern s32 D_80092904;
extern s32 D_80092900;
extern u8 D_80090F38[];
extern u8* GetStringEntry(u8* pData, s32 index);
extern void SystemStringPushBuffer(void* pString, u8* pEntry);

void BattleInitTutorialMessage(void) {
    D_80099D9D = 0;
    D_80099D9E = 0;
    BattleSetScreenMode(7);
    D_800928C8 = 5;
    D_80092884 = 0;
    func_80032F54(&D_80092954[0], 0x140, 0x70, 0xA2, 0x2A, 0x1C, 8);
    SystemStringPushBuffer(&D_80092954[0], GetStringEntry(D_80092880, 0x42));
    D_800929BC = 0x1E;
    D_800925DC = 0;
    BattleSetValueAndClearFlags(&D_80090F38[0]);
    D_80092604 = 0;
    D_80092904 = 0;
    D_80092900 = 0;
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
   access, a s32 value local, and ((s32*)&D_8009871C)[1] so the address is a
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
        func_80071DA4((u8*)&D_8009872C);
        if (D_800970C4 * 0x100 / D_800970CC < D_800987E0 * 0x100 / D_800987E8) {
            D_800925D4 = 0x43;
        } else {
            D_800925D4 = 0x44;
        }
        break;
    case 2:
        func_80071DA4((u8*)&D_80097010);
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


extern u8 D_8009293C;

void BattleClearInitFlag(void) {
    D_8009293C = 0;
}

extern u8 D_800910C4;
extern u8 D_800928D4;
extern u8 D_80099D9D;
extern u8 D_80099D9E;
extern s32 D_80092900;
extern s32 D_80092A00;
extern s32 D_80092A10;
extern s32 D_80092A20;
extern s32 D_800970F8;
extern s32 D_80098814;

void BattleInitOnce(void) {
    if (D_8009293C != 0) {
        return;
    }
    BattleSetValueAndClearFlags((s32)&D_800910C4);
    D_8009293C = 1;
    D_800928D4 = 0;
    D_80099D9D = 0;
    D_80099D9E = 0;
    D_80092900 = 0;
    D_800925D4 = 0;
    D_800925D8 = 0;
    D_80092A00 = 1;
    D_80092A10 = 1;
    D_80092A20 = 1;
    D_800970F8 = 0;
    D_80098814 = 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80072170);

void BattleEmptyFunction(void) {
}

extern void* BattleCreateOrderingTablePair(s32 a0);
extern void* BattleCreateLightGroup(void* p);
extern void* BattleInitObjectTree(s32 a0);
extern void func_80078F00(void* p);
extern s16 D_800928D0;
extern void* D_80092614;
extern s32 D_80096FB0;
extern s32 D_80096FAC;
extern s32 D_80096FA8;
extern u8 D_80092618;
extern s32 D_8009261C;
extern s32 D_80092620;
extern s32 D_80092624;
extern s32 D_80092628;
extern s32 D_8009262C;
extern s16 D_80092632;
extern void* D_800910F0;
extern void* D_80092610;

// The block the actor's script pointer (+0x8FC) addresses: the five signed
// halfwords and the unsigned one BattleInitRenderScene copies into the render
// state at D_80092618..D_80092632.
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
    D_800910F0 = pAnim;
    D_80092610 = BattleInitObjectTree(id);
    BattleSetScreenResolution(0x280, 0xDA);
    SetGeomScreen(0x400);
    D_800928D0 = 0;
    D_80092614 = p;
    func_80078F00(p);
    // retail reads the script pointer after this cluster, not in front of it:
    // the block stops cc1 hoisting the load over the three stores.
    do {
        D_80096FB0 = 0;
        D_80096FAC = 0;
        D_80096FA8 = 0;
    } while (0);
    pScript = p->pScript;
    x = pScript->unk14;
    y = pScript->unk16;
    z = pScript->unk18;
    w = pScript->unk1A;
    u = pScript->unk1C;
    v = pScript->unk1E;
    D_80092618 = 1;
    D_8009261C = x;
    D_80092620 = y;
    D_80092624 = z;
    D_80092628 = w;
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
extern void* D_800910F0;
extern void* D_80092610;

void BattleFreeRenderScene(void) {
    func_8008BC04();
    BattleClearTextSpriteEntries();
    func_80030988(1, 1, 0x40, 0x40);
    BattleFreeLightGroup(D_800910F0);
    BattleFreeNodeTree(D_80092610);
    BattleSetScreenResolution(0x140, 0xDA);
    SetGeomScreen(0xC0);
    BattleSetScreenMode(3);
    BattleSetTextScale(0x100);
    BattleResetTextPrimQueue();
}

extern MATRIX D_80091C0C;

void BattleComposeCameraMatrix(BattleActorView* p, MATRIX* pIn, MATRIX* pOut) {
    MATRIX mtx;

    *pOut = *pIn;
    mtx = D_80091C0C;
    mtx.t[0] = p->unk34.vx - D_80096FA8;
    mtx.t[1] = -D_80096FAC;
    mtx.t[2] = p->unk34.vz - D_80096FB0;
    CompMatrix(pIn, &mtx, &mtx);
    pOut->t[0] = mtx.t[0];
    pOut->t[1] = mtx.t[1];
    pOut->t[2] = mtx.t[2];
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80072858);
