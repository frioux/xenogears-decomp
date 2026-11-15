#include "common.h"
#include "psyq/libgpu.h"
#include "libgear/main.h"

// Partial view of the per-frame battle work block (D_800D2D28). The shared
// include/battle/battle_work.h owns the type other units read; this view names
// only the sub-image upload state this unit advances each frame.
typedef struct {
    /* 0x000 */ u8 unk0[0x30];
    /* 0x030 */ u32 unk30; // source page index, advances 4 words per upload
    /* 0x034 */ u8 unk34[0x75];
    /* 0x0A9 */ u8 unkA9;
    /* 0x0AA */ u8 unkAA;
    /* 0x0AB */ u8 unkAB;
    /* 0x0AC */ u8 unkAC[0x1E];
    /* 0x0CA */ u8 unkCA;
    /* 0x0CB */ u8 unkCB[0x4];
    /* 0x0CF */ u8 unkCF;
} BattleUploadState; // Size: 0xD0 (lower bound: other units reach 0xFC)

// Partial view of the battle work block (D_800C3EA4): the sub-image descriptors
// LoadImage uploads and the scroll BattleUpdateImageUpload animates.
typedef struct {
    /* 0x0000 */ u8 unk0[0x6410];
    /* 0x6410 */ s32 unk6410; // sub-image scroll, steps by 4 between 0x7C and 0x81
    /* 0x6414 */ u8 unk6414;
    /* 0x6415 */ u8 unk6415;
    /* 0x6416 */ u8 unk6416;
    /* 0x6417 */ u8 unk6417[0x2539];
    /* 0x8950 */ RECT uploadRect[4];
} BattleUploadWork; // Size: 0x8970 (lower bound: other units reach 0xA2C8)

// The prim pool's head is a 0x38-stride table of per-slot records; only the two
// fields this unit touches are named (see BattlePrimPool for the pool tail).
typedef struct {
    /* 0x00 */ u8 unk0[0x26];
    /* 0x26 */ s16 unk26;
    /* 0x28 */ u8 unk28;
    /* 0x29 */ u8 unk29[0xF];
} BattlePrimPoolSlot; // Size: 0x38

extern u16 D_800C3468[];
extern u16 D_800C3448[];
extern u8 D_800C3E4C;
extern u8 D_800CCDC4[];
extern u8 D_800CCDC6[];
extern u8 D_800C3CF9[];
extern s32 D_800C3A9C;
extern u8 D_800C3A94;
extern s32 D_800C3A8C;
extern u8 D_800C3A98;
extern s32 D_800C3A90;
extern s32 D_800C2080;
extern s32 D_800C2084;
extern s32 D_800C3A7C;
extern s32 D_800C3A84;
extern s32 D_800C3A80;
extern s32 D_800C3A88;
extern u8 D_800C207C;
extern u8 D_800D2C36;
extern u8 D_800C3CFA[];
extern u8 D_800D2C38;
extern u16 D_800D2C30;
extern s32 BattleMaskHasStatusBit(s32 mask, u8 idx);
extern u8 D_800CCB34;
extern u8* D_800D2DB4;
extern s32 BattleDrawSpritePackage(s32 arg0, s32 arg1, s16 arg2, s16 arg3);
extern void BattleSetPrimGrey80BlendAdd(u8* p);
extern Temp8* D_8005919C;
extern void SoundPlaySoundEffectOnLastPair(s32 arg0);
extern void func_8008860C(void);
extern void BattleBuildFlaggedSlotSpriteColumn(void);
extern void BattleBuildBlendedSprite(void);
extern void BattleBuildGrey80BlendSpriteColumnPair(void);
extern void BattleBuildGrey80BlendSpriteColumn5(void);
extern void func_8008946C(void);
extern void BattleBuildGrey80BlendSpriteColumn7(void);
extern void BattleBuildGrey80BlendSpriteColumn2(void);
extern void func_8008A3EC(u8 arg0);
extern void BattleUpdateImageUpload(u8 arg0);
extern void func_8008A684(u8 arg0);
extern int rand(void);
extern BattleUploadWork *D_800C3EA4;
extern BattleUploadState *D_800D2D28;
extern u8 D_800C3444;
extern BattlePrimPoolSlot *D_800D3278;
extern u8 D_800C48EA;
extern u8 D_800D3014;
extern s32 *D_8005917C;
extern u16 g_C1ButtonStatePressedOnce;
extern u16 g_C1ButtonStateReleased;
extern int D_80059488;
extern int ControllerGetType(int controllerIndex);
extern int ControllerIsStateStackFull(void);
extern void ControllerResetState(void);
extern int ControllerPopState(void);
extern void GraphicsDrawPauseLetters(int x, int y);
extern void SoundMuteAllSpuChannels(void);
extern void SoundEnableAllSpuChannels(void);
extern u8 D_800C3CF8[];
extern u8 D_800C3CFB[];
extern u8 D_800CCC58;
extern u8 D_800D2C34;
extern u8 D_800D2C35;
extern u8 D_800D39D4;
extern s32 D_800D3288;
extern u16 D_800D2C2A;
extern void func_8007171C(void);
extern void func_80089CCC(s32 arg0);
extern void BattleParseNumberToDigits(s32 arg0);
extern void BattleSetPrimGrey80BlendSub(u8 *p);

void BattleStepTwoAxisInterpolation(void) {
    s32 i = 0;

    for (i = 0; i < D_800C3A9C; i++) {
        if (D_800C3A94 != 0) {
            D_800C2080 -= D_800C3A8C;
        } else {
            D_800C2080 += D_800C3A8C;
        }
        if (D_800C3A98 != 0) {
            D_800C2084 -= D_800C3A90;
        } else {
            D_800C2084 += D_800C3A90;
        }
    }
    if (D_800C3A8C == 0x100) {
        if (D_800C3A94 != 0) {
            if (D_800C2080 / 0x100 + D_800C3A7C < D_800C3A84) {
                D_800C207C = 1;
            }
        } else if (D_800C3A84 < D_800C2080 / 0x100 + D_800C3A7C) {
            D_800C207C = 1;
        }
    } else if (D_800C3A98 != 0) {
        if (D_800C2084 / 0x100 + D_800C3A80 < D_800C3A88) {
            D_800C207C = 1;
        }
    } else if (D_800C3A88 < D_800C2084 / 0x100 + D_800C3A80) {
        D_800C207C = 1;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_80088B80);
void BattleBuildFlaggedSlotSpriteColumn(void) {
    s32 i = 0;
    s32 pitch = 0x6E << 16;
    s32 off;

    D_800D2DB4[0x5DA1] = 0;
    for (i = 0; i < 5; i++) {
        if ((u16)BattleMaskHasStatusBit(D_800D2C30, i) != 0) {
            off = D_800D2DB4[0x5DA1] * 0x50 + 0x4CE0;
            D_800D2DB4[0x5DA1] += BattleDrawSpritePackage(i + 0xC4, (s32)(off + (s32)D_800D2DB4), 0xE0, pitch >> 16);
            pitch += 0xA0000;
        }
    }
    D_800D2DB4[0x5DA0] = D_800CCB34;
    *(u16*)(D_800D2DB4 + 0x5DA2) = 0;
}
void BattleBuildBlendedSprite(void) {
    s32 arg0;
    s32 pEntry;
    s32 i;

    arg0 = 0xA0;
    if (D_800D2C38 != 0) {
        arg0 = 0xA1;
    }
    D_800D2DB4[0x5D75] = BattleDrawSpritePackage(arg0, (s32)(D_800D2DB4 + 0x3AC0), 0xA0, 0x64);
    D_800D2DB4[0x5D84] = D_800CCB34;
    for (i = 0; i < D_800D2DB4[0x5D75]; i++) {
        pEntry = (i * 2 + D_800D2DB4[0x5D84]) * 0x28 + 0x3AC0;
        BattleSetPrimGrey80BlendAdd((u8*)(pEntry + (s32)D_800D2DB4));
    }
}
void BattleBuildGrey80BlendSpriteColumnPair(void) {
    u8 v;
    u8 slotA;
    u8 slotB;
    s32 i;

    v = D_800D2C34;
    slotA = v - 0x5D;
    slotB = v - 0x25;
    D_800D2DB4[0x5D77] = BattleDrawSpritePackage(slotA, (s32)(D_800D2DB4 + 0x3E80), 0xA0, 0x64);
    D_800D2DB4[0x5D86] = D_800CCB34;
    D_800D2DB4[0x5D7F] = BattleDrawSpritePackage(slotB, (s32)(D_800D2DB4 + 0x43D0), 0xA0, 0x64);
    D_800D2DB4[0x5D8E] = D_800CCB34;
    for (i = 0; i < D_800D2DB4[0x5D77]; i++) {
        BattleSetPrimGrey80BlendAdd(D_800D2DB4 + ((i * 2 + D_800D2DB4[0x5D86]) * 0x28 + 0x3E80));
    }
    for (i = 0; i < D_800D2DB4[0x5D7F]; i++) {
        BattleSetPrimGrey80BlendSub(D_800D2DB4 + ((i * 2 + D_800D2DB4[0x5D8E]) * 0x28 + 0x43D0));
    }
}
void BattleBuildGrey80BlendSpriteColumn5(void) {
    s32 i = 0;
    s32 pitch = 0x11A << 16;
    s32 off;
    s32 off2;

    BattleParseNumberToDigits(D_800D2C2A);
    while (i < 5) {
        s32 slot = D_800C3CF8[i++];
        if (slot != 0xFF) {
            off = D_800D2DB4[0x5D78] * 0x50 + 0x4E70;
            D_800D2DB4[0x5D78] += BattleDrawSpritePackage(slot + 0x92, (s32)(off + (s32)D_800D2DB4), pitch >> 16, 0x46);
            pitch += 0x60000;
        }
    }
    D_800D2DB4[0x5D87] = D_800CCB34;
    for (i = 0; i < D_800D2DB4[0x5D78]; i++) {
        BattleSetPrimGrey80BlendAdd((u8*)((off2 = (i * 2 + D_800D2DB4[0x5D87]) * 0x28 + 0x4E70) + (s32)D_800D2DB4));
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_8008946C);
void BattleBuildGrey80BlendSpriteColumn7(void) {
    s32 i = 0;
    s32 columnBase;
    s32 n = 0;
    s32 pitch = 0x11A << 16;
    s32 off;
    s32 off2;

    columnBase = 0x50A0;
    BattleParseNumberToDigits(D_800D2C36);
    for (i = 0; i < 3; i++) {
        if (D_800C3CFA[i] != 0xFF) {
            off = D_800D2DB4[0x5D7A] * 0x50 + 0x50A0;
            D_800D2DB4[0x5D7A] += BattleDrawSpritePackage(D_800C3CFA[i] + 0x92, (s32)(off + (s32)D_800D2DB4), pitch >> 16, 0x56);
            pitch += 0x60000;
            n++;
        }
    }
    D_800D2DB4[0x5D7A] += BattleDrawSpritePackage(0x9D, (s32)((D_800D2DB4[0x5D7A] * 0x50 + columnBase) + (s32)D_800D2DB4), 0x11A + n * 6, 0x56);
    D_800D2DB4[0x5D89] = D_800CCB34;
    for (i = 0; i < D_800D2DB4[0x5D7A]; i++) {
        off2 = (i * 2 + D_800D2DB4[0x5D89]) * 0x28 + 0x50A0;
        BattleSetPrimGrey80BlendAdd(&D_800D2DB4[off2]);
    }
}

void BattleBuildGrey80BlendSpriteColumn2(void) {
    s32 i = 0;
    s32 pitch = 0x11A << 16;
    s32 off;
    s32 off2;

    BattleParseNumberToDigits(D_800D2C35);
    while (i < 2) {
        s32 slot = D_800C3CFB[i++];
        if (slot != 0xFF) {
            off = D_800D2DB4[0x5D7B] * 0x50 + 0x51E0;
            D_800D2DB4[0x5D7B] += BattleDrawSpritePackage(slot + 0x92, (s32)(off + (s32)D_800D2DB4), pitch >> 16, 0x5E);
            pitch += 0x60000;
        }
    }
    D_800D2DB4[0x5D8A] = D_800CCB34;
    for (i = 0; i < D_800D2DB4[0x5D7B]; i++) {
        BattleSetPrimGrey80BlendAdd((u8*)((off2 = (i * 2 + D_800D2DB4[0x5D8A]) * 0x28 + 0x51E0) + (s32)D_800D2DB4));
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_800898F0);
// Best attempt: objdiff 91.38% (528 bytes, the original size). Only two classes of diff,
// both shared with the rest of the BattleDrawSpritePackage family in this unit: every call site puts
// the D_800D2DB4 base in $a1 and the offset chain in $v0 where the original has base $v1 /
// chain $a1 (the final addu is the mirror image), and the second loop's "pitch = 0x48 << 16"
// plus its 0x80000 increment are scheduled a few instructions differently. Pitch values are
// 0x20<<16 / 0x48<<16 with a +0x80000 step, the step is OUTSIDE the if, and the table
// lookup index is arg0 * 0x170 into a u8 table at D_800CCDC4 / D_800CCDC6.
/*
void func_800898F0(u8 arg0) {
    s32 i = 0;
    s32 pitch = 0x20 << 16;
    s32 off;

    D_800D2DB4[0x5D7C] = 0;
    BattleParseNumberToDigits(*(u16*)(D_800CCDC4 + arg0 * 0x170));
    for (i = 0; i < 4; i++) {
        if (D_800C3CF9[i] != 0xFF) {
            off = D_800D2DB4[0x5D7C] * 0x50 + 0x5280;
            D_800D2DB4[0x5D7C] += BattleDrawSpritePackage(D_800C3CF9[i] + 0x92, (s32)(off + (s32)D_800D2DB4), pitch >> 16, 0xCC);
        }
        pitch += 0x80000;
    }
    D_800D2DB4[0x5D8B] = D_800CCB34;
    D_800D2DB4[0x5D7D] = 0;
    BattleParseNumberToDigits(*(u16*)(D_800CCDC6 + arg0 * 0x170));
    pitch = 0x48 << 16;
    for (i = 0; i < 4; i++) {
        if (D_800C3CF9[i] != 0xFF) {
            off = D_800D2DB4[0x5D7D] * 0x50 + 0x53C0;
            D_800D2DB4[0x5D7D] += BattleDrawSpritePackage(D_800C3CF9[i] + 0x92, (s32)(off + (s32)D_800D2DB4), pitch >> 16, 0xCC);
        }
        pitch += 0x80000;
    }
    off = D_800D2DB4[0x5D7D] * 0x50 + 0x53C0;
    D_800D2DB4[0x5D7D] += BattleDrawSpritePackage(0x9C, (s32)(off + (s32)D_800D2DB4), 0x41, 0xCC);
    D_800D2DB4[0x5D8C] = D_800CCB34;
}
*/
void BattleBuildSpritePrimLists(void) {
    func_8008860C();
    BattleBuildFlaggedSlotSpriteColumn();
    BattleBuildBlendedSprite();
    BattleBuildGrey80BlendSpriteColumnPair();
    BattleBuildGrey80BlendSpriteColumn5();
    func_8008946C();
    BattleBuildGrey80BlendSpriteColumn7();
    BattleBuildGrey80BlendSpriteColumn2();
}
u16 BattleRandomRange(u16 arg0, u16 arg1) {
    s32 rem;
    s32 range;

    if (arg0 == 0xFFFF) {
        return 0xFFFF;
    }
    if (arg1 == 0) {
        return 0;
    }
    if (arg0 == arg1) {
        return arg0;
    }
    range = arg1 - arg0;
    if (range > 0xFFFE) {
        return rand();
    }
    rem = (u16)rand() % (range + 1);
    return (u16)(arg0 + rem);
}
u16 BattleStatusBitMask(u8 idx) {
    return D_800C3468[idx];
}
u16 BattleSlotBitMask(u8 idx) {
    return D_800C3448[idx];
}
u16 BattleStatusBitMaskInverted(u8 idx) {
    return ~D_800C3468[idx];
}
u16 BattleSlotBitMaskInverted(u8 idx) {
    return ~D_800C3448[idx];
}
s32 BattleMaskHasStatusBit(s32 mask, u8 idx) {
    s32 flags;

    if (idx < 0x10) {
        flags = D_800C3468[idx] & mask;
    } else {
        flags = 0;
    }
    return flags;
}
s32 BattleMaskHasSlotBit(s32 mask, u8 idx) {
    s32 flags;

    if (idx < 0x10) {
        flags = D_800C3448[idx] & mask;
    } else {
        flags = 0;
    }
    return flags;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_80089CCC);
void BattleUploadImageFrame(void) {
    /* declared out of call order: gcc's register tie for the LoadImage arguments depends on it */
    s32 vramOffset3;
    s32 vramOffset0;
    s32 vramOffset1;
    s32 vramOffset2;
    D_800D2D28->unkAA += 1;
    if ((D_800D2D28->unkAA & 1) == 0) {
        return;
    }
    vramOffset0 = 0x8970;
    LoadImage(&D_800C3EA4->uploadRect[0], (u_long*)(D_800D2D28->unk30 * 4 + vramOffset0 + (s32)D_800C3EA4));
    vramOffset1 = 0x8FA0;
    LoadImage(&D_800C3EA4->uploadRect[1], (u_long*)(D_800D2D28->unk30 * 4 + vramOffset1 + (s32)D_800C3EA4));
    vramOffset2 = 0x95D0;
    LoadImage(&D_800C3EA4->uploadRect[2], (u_long*)(D_800D2D28->unk30 * 4 + vramOffset2 + (s32)D_800C3EA4));
    vramOffset3 = 0x9C00;
    LoadImage(&D_800C3EA4->uploadRect[3], (u_long*)(D_800D2D28->unk30 * 4 + vramOffset3 + (s32)D_800C3EA4));
    D_800D2D28->unk30 += 4;
    if (D_800D2D28->unk30 >= 0xC7) {
        D_800D2D28->unk30 = 0;
    }
}
void BattleUpdateImageUpload(u8 arg0) {
    if (D_800CCC58 != 0) {
        func_8007171C();
    }
    if (arg0 == 0) {
        func_80089CCC(0);
    }
    D_800D2D28->unkA9 += 6;
    D_800D2D28->unkAB += 1;
    if (D_800C3EA4->unk6415 != 0) {
        if (D_800C3EA4->unk6416 == 0) {
            D_800C3EA4->unk6410 += 4;
            if (D_800C3EA4->unk6410 >= 0x81) {
                D_800C3EA4->unk6410 = 0x7C;
                D_800C3EA4->unk6416 = 1;
            }
        } else {
            D_800C3EA4->unk6410 -= 4;
            if (D_800C3EA4->unk6410 < 0) {
                D_800C3EA4->unk6410 = 4;
                D_800C3EA4->unk6416 = 0;
            }
        }
    }
    switch (D_800D39D4) {
    case 1:
    case 3:
        D_800D3288 += 1;
        break;
    case 2:
    case 4:
        D_800D3288 -= 1;
        break;
    }
    BattleUploadImageFrame();
}

void func_8008A3EC(u8 arg0) {
    s32 i;
    s32 channelMask;
    u8 muted;
    u8 waiting;

    waiting = 1;
    muted = 0;
    do {
        if (ControllerGetType(0) == 0) {
            if (muted == 0) {
                GraphicsDrawPauseLetters(0x88, 0x64);
                GraphicsDrawPauseLetters(0x88, 0x144);
                SoundMuteAllSpuChannels();
                muted++;
                channelMask = D_80059488;
            }
        } else {
            if (muted != 0) {
                SoundEnableAllSpuChannels();
                D_80059488 = channelMask;
            }
            waiting = 0;
        }
    } while (waiting != 0);

    BattleUploadImageFrame();
    if (D_800D2D28->unkCA != 0) {
        for (i = 0; i < 0x10; i++) {
            if (D_800D3278[i].unk28 != 0) {
                if (--D_800D3278[i].unk26 < 0) {
                    D_800D3278[i].unk26 = 0;
                }
            }
        }
    }

    muted = 1;
    do {
        if (D_800D2D28->unkCF == 0) {
            D_800D3014 = 0xFF;
        }
        if (ControllerIsStateStackFull() != 0) {
            ControllerResetState();
        } else {
            while (ControllerPopState() != 0) {
                if (D_800C3444 != 0) {
                    if (*D_8005917C != -1) {
                        if ((g_C1ButtonStatePressedOnce & 4) != 0) {
                            if ((g_C1ButtonStatePressedOnce & 8) != 0) {
                                D_800C48EA = muted;
                                SoundEnableAllSpuChannels();
                                D_80059488 = channelMask;
                                D_800C3444 = 0;
                                break;
                            }
                        }
                    }
                } else if ((g_C1ButtonStateReleased & 0x20) != 0) {
                    D_800D3014 = 4;
                    break;
                }
                if ((g_C1ButtonStateReleased & 0x800) == 0) {
                    continue;
                }
                if (D_800CCC58 == 0) {
                    break;
                }
                if (D_800C3444 == 0) {
                    SoundMuteAllSpuChannels();
                    GraphicsDrawPauseLetters(0x88, 0x64);
                    GraphicsDrawPauseLetters(0x88, 0x144);
                    channelMask = D_80059488;
                    D_800C3444 = muted;
                } else {
                    SoundEnableAllSpuChannels();
                    D_80059488 = channelMask;
                    D_800C3444 = 0;
                }
                break;
            }
        }
    } while (D_800C3444 != 0);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_8008A684);
void BattleUpdateEffectByScreenMode(u8 arg0) {
    switch (D_800C3E4C) {
    case 0:
        func_8008A684(arg0);
        break;
    case 1:
        BattleUpdateImageUpload(arg0);
        break;
    case 2:
        func_8008A3EC(arg0);
        break;
    }
}
void BattlePlaySoundWithBaseId(u8 arg0) {
    SoundPlaySoundEffectOnLastPair((D_8005919C->unk14 << 16) | arg0);
}
