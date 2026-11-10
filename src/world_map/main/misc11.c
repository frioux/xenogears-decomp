#include "common.h"
#include "main/game.h"

typedef struct {
    /* 0x00 */ undefined32 unk00;
    /* 0x04 */ u16 unk04;
    /* 0x06 */ u16 unk06;
    /* 0x08 */ undefined32 unk08[6]; // 0x08..0x1F
    /* 0x20 */ u16 unk20;
    /* 0x22 */ u16 unk22;
    /* 0x24 */ s16 unk24;
    /* 0x26 */ undefined16 unk26;
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ undefined32 unk34;
    /* 0x38 */ s32 unk38;
    /* 0x3C */ u32 unk3C;
    /* 0x40 */ s32 unk40;
    /* 0x44 */ undefined32 unk44;
    /* 0x48 */ s16 unk48;
    /* 0x4A */ s16 unk4A;
    /* 0x4C */ void *unk4C;
    /* 0x50 */ s32 unk50;
    /* 0x54 */ void *unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ s32 unk5C;
    /* 0x60 */ undefined32 unk60[8]; // 0x60..0x7F
} WorldMapActor; // Size: 0x80

typedef struct {
    /* 0x000 */ undefined32 unk000[40]; // 0x000..0x09F
    /* 0x0A0 */ s16 unk0A0;
    /* 0x0A2 */ s16 unk0A2;
    /* 0x0A4 */ s16 unk0A4;
    /* 0x0A6 */ undefined16 unk0A6;
    /* 0x0A8 */ s16 unk0A8;
    /* 0x0AA */ s16 unk0AA;
    /* 0x0AC */ s16 unk0AC;
    /* 0x0AE */ undefined16 unk0AE;
} WorldMapScreenPos; // Size: 0xB0

typedef struct {
    /* 0x00 */ s16 unk00;
    /* 0x02 */ undefined16 unk02;
    /* 0x04 */ s32 unk04;
    /* 0x08 */ s32 unk08;
    /* 0x0C */ s32 unk0C;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ undefined16 unk16;
} WorldMapEntry; // Size: 0x18

extern WorldMapActor *D_8009BE24;
extern s32 D_8009BE10;
extern s16 D_8009BD04;
extern WorldMapEntry D_8009BE6C[];
extern s16 D_8009B18C[];
extern s16 D_8009B194[];
extern s16 D_8009B19C[];
extern s16 D_8009B1A4[];
extern void *D_8009BDF8[];
extern void *D_8009CD38;
extern void *D_8009CD3C;

extern s32 func_8008C364(WorldMapActor *, s32);
extern void func_80089160(s32, void *, void *);
extern void func_800894C8(s32);
extern s16 func_80093F18(void *);
extern s32 func_80093978(s32, s32);
extern void *D_8009BDFC[];
extern void *D_8009BE00[];
extern void *func_80024524(void *, s16, s16, s16, s16, s16);
extern void func_800245D8(void *, s32);
extern void SpriteSetScale(void *, s16);
extern void func_80093354(void *);
extern s32 D_8009C5AC;
extern s32 D_8009C5B4;
extern s32 D_8009C584;

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008A72C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008B2BC);
int func_8008B498(int index) {
    WorldMapActor *pActor = &D_8009BE24[index];
    s32 result = 1;

    if (g_GameState.partyMembers[1] != 0xFF) {
        pActor->unk4C = func_80024524(D_8009CD38, 0x110, 0x1E0, 0x150, 0x100, 0x40);
        func_800245D8(pActor->unk4C, 0);
        SpriteSetScale(pActor->unk4C, 0x1800);
        *(u32 *)((u8 *)pActor->unk4C + 0x3C) &= ~4;
    } else {
        result = 3;
    }
    return result;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008B54C);
/* 93.37% (re-diffed under the current XenoWorldMap flags; cdk 83.19%, plain -G0/-O
   variants lower). Everything is byte-identical except three insns in the prologue:
   the original computes index*0x80 in v1 BEFORE the 0x100 push and stores the
   0x40 stack argument just before the jal, while gcc emits the store right after
   li 0x40 and puts the shift in v0 after it. The SAME call shape with the SAME
   neighbours (func_8008C75C, matched) reproduces the original prologue exactly, so
   the trigger is something in this function's tail, not the call: the tail below is
   itself byte-identical. Tried: flat tail without the unk48 temp (83.05), pointer
   built as (u8*)D_8009BE24 + index*0x80 (93.37), cdk (83.19).
int func_8008B54C(int index) {
    WorldMapActor *pActor = &D_8009BE24[index];

    pActor->unk4C = func_80024524(D_8009CD38, 0x110, 0x1E0, 0x150, 0x100, 0x40);
    func_800245D8(pActor->unk4C, 0);
    SpriteSetScale(pActor->unk4C, 0x1800);
    *(u32 *)((u8 *)pActor->unk4C + 0x3C) &= ~4;
    pActor->unk28 = g_GameState.unk1820 << 12;
    pActor->unk30 = g_GameState.unk1822 << 12;
    pActor->unk2C = func_80093978(pActor->unk28, pActor->unk30);
    {
        u16 unk48 = g_GameState.unk1824;
        pActor->unk24 = 1;
        pActor->unk4A = 8;
        pActor->unk20 = 2;
        pActor->unk40 = 0;
        pActor->unk3C = 0;
        pActor->unk38 = 0;
        pActor->unk58 = 0xF;
        pActor->unk48 = unk48;
    }
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008B644);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008BB40);
int func_8008BD1C(int index) {
    WorldMapActor *pActor = &D_8009BE24[index];
    s32 result = 1;

    if (g_GameState.partyMembers[2] != 0xFF) {
        pActor->unk4C = func_80024524(D_8009CD3C, 0x120, 0x1E0, 0x160, 0x100, 0x40);
        func_800245D8(pActor->unk4C, 0);
        SpriteSetScale(pActor->unk4C, 0x1800);
        *(u32 *)((u8 *)pActor->unk4C + 0x3C) &= ~4;
    } else {
        result = 3;
    }
    return result;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008BDD0);
/* 93.37% (was 83.03% before the preset change). Byte-identical apart from the same
   three prologue insns as func_8008B54C: original shifts index into v1 before the
   0x100 push and stores the 0x40 argument in front of the jal; gcc stores it right
   after li 0x40 and shifts into v0. Tail is byte-identical.
int func_8008BDD0(int index) {
    WorldMapActor *pActor = &D_8009BE24[index];

    pActor->unk4C = func_80024524(D_8009CD3C, 0x120, 0x1E0, 0x160, 0x100, 0x40);
    func_800245D8(pActor->unk4C, 0);
    SpriteSetScale(pActor->unk4C, 0x1800);
    *(u32 *)((u8 *)pActor->unk4C + 0x3C) &= ~4;
    pActor->unk28 = g_GameState.unk1820 << 12;
    pActor->unk30 = g_GameState.unk1822 << 12;
    pActor->unk2C = func_80093978(pActor->unk28, pActor->unk30);
    {
        u16 unk48 = g_GameState.unk1824;
        pActor->unk24 = 1;
        pActor->unk4A = 8;
        pActor->unk20 = 2;
        pActor->unk40 = 0;
        pActor->unk3C = 0;
        pActor->unk38 = 0;
        pActor->unk58 = 0x1E;
        pActor->unk48 = unk48;
    }
    return 1;
}
*/
int func_8008BEC8(WorldMapActor *pActor) {
    s32 dx;
    s32 dz;
    s32 result = 0;

    dx = pActor->unk50 - (pActor->unk28 >> 12);
    if ((dx < 0 ? -dx : dx) >= 5) {
        pActor->unk28 += pActor->unk38 * (pActor->unk4A / 2);
    } else {
        result = 1;
    }
    dz = pActor->unk54 - (pActor->unk30 >> 12);
    if ((dz < 0 ? -dz : dz) >= 5) {
        pActor->unk30 += pActor->unk40 * (pActor->unk4A / 2);
    } else {
        result |= 2;
    }
    func_80093354(&pActor->unk28);
    pActor->unk2C = func_80093978(pActor->unk28, pActor->unk30);
    return result;
}
void func_8008BFD4(s16 arg0, s32 *pArray, s16 arg2, s16 arg3) {
    WorldMapEntry *pEntry = &D_8009BE6C[D_8009BD04];

    pEntry->unk00 = arg0;
    pEntry->unk04 = pArray[0];
    pEntry->unk08 = pArray[1];
    pEntry->unk0C = pArray[2];
    pEntry->unk10 = arg3;
    pEntry->unk14 = arg2;
    D_8009BD04 = (D_8009BD04 + 1) & 0x1F;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008C040);
void func_8008C1DC(s32 arg0, WorldMapActor *pActor, WorldMapScreenPos *pPos) {
    if (func_80093F18(&pActor->unk28) == 3) {
        pPos->unk0A0 = pActor->unk28 >> 12;
        pPos->unk0A2 = pActor->unk2C >> 12;
        pPos->unk0A4 = pActor->unk30 >> 12;
        pPos->unk0AC = 0;
        pPos->unk0A8 = 0;
        pPos->unk0AA = pActor->unk5C;
        func_80089160(arg0, &pPos->unk0A0, &pPos->unk0A8);
    } else {
        func_800894C8(arg0);
    }
}
void func_8008C28C(WorldMapActor *pActor, int index) {
    void *pSprite;

    pSprite = func_80024524(D_8009BDF8[index], D_8009B18C[index], D_8009B194[index], D_8009B19C[index],
                            D_8009B1A4[index], 0x40);
    pActor->unk4C = pSprite;
    if (g_GameState.gearRide[index] == 1) {
        func_800245D8(pSprite, 0);
    } else {
        func_800245D8(pSprite, 3);
    }
    SpriteSetScale(pActor->unk4C, 0x2000);
    *(u32 *)((u8 *)pActor->unk4C + 0x3C) &= ~4;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008C364);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008C530);

int func_8008C6EC(int index) {
    WorldMapActor *pActor = &D_8009BE24[index];
    s32 result = func_8008C364(pActor, 0);
    s32 mode = D_8009BE10;

    if (mode > 0) {
        if (mode < 4) {
            return result;
        }
        if (mode < 8) {
            pActor->unk24 = 1;
        }
    }
    return result;
}

int func_8008C75C(int index) {
    WorldMapActor *pActor = &D_8009BE24[index];

    pActor->unk4C = func_80024524(D_8009BDF8[0], 0x100, 0x1FD, 0x140, 0x140, 0x40);
    func_800245D8(pActor->unk4C, 0);
    SpriteSetScale(pActor->unk4C, 0x2000);
    *(u32 *)((u8 *)pActor->unk4C + 0x3C) &= ~4;
    pActor->unk28 = g_GameState.unk1930[0x16] << 12;
    pActor->unk30 = g_GameState.unk1930[0x17] << 12;
    pActor->unk2C = func_80093978(pActor->unk28, pActor->unk30);
    pActor->unk24 = 1;
    pActor->unk40 = 0;
    pActor->unk3C = 0;
    pActor->unk38 = 0;
    pActor->unk4A = 0xC;
    pActor->unk48 = *(u16 *)&g_GameState.unk1826[0];
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008C844);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008D3F0);
/* 81.33%: everything after the func_8008C364 call is correct except the store
   order in the head: the original puts the three zero stores (0x40, 0x3C, 0x38)
   first, then hoists the g_GameState+0x1828 load above them, and re-reads
   unk48 with lh; gcc here sinks the zero stores below the sll/sra chain and cse
   folds the re-read into the lhu. The mode < 4 / mode < 8 / gearRide blocks and
   the whole tail are byte-identical. Also tried: unk5C assigned after the
   D_8009BE10 load (identical output), ascending zero stores.
int func_8008D3F0(int index) {
    WorldMapActor *pActor = &D_8009BE24[index];
    s32 result = func_8008C364(pActor, 1);
    s32 mode;

    pActor->unk40 = 0;
    pActor->unk3C = 0;
    pActor->unk38 = 0;
    pActor->unk4A = 0xC;
    pActor->unk58 = 0xF;
    pActor->unk48 = *(u16 *)&g_GameState.unk1826[2];
    pActor->unk5C = pActor->unk48;
    mode = D_8009BE10;
    if (mode > 0) {
        if (mode < 4) {
            if (g_GameState.gearRide[1] == 1) {
                pActor->unk20 = 1;
                pActor->unk28 = D_8009C5AC;
                pActor->unk30 = D_8009C5B4;
                pActor->unk2C = func_80093978(pActor->unk28, pActor->unk30);
                pActor->unk48 = D_8009C584;
            }
        } else if (mode < 8) {
            pActor->unk20 = 2;
            pActor->unk24 = 1;
        }
    }
    g_GameState.unk1930[0x19] = pActor->unk28 >> 12;
    g_GameState.unk1930[0x1A] = pActor->unk30 >> 12;
    *(u16 *)&g_GameState.unk1826[2] = pActor->unk48;
    return result;
}*/


int func_8008D520(int index) {
    WorldMapActor *pActor = &D_8009BE24[index];
    s32 result = func_8008C364(pActor, 1);
    s32 mode = D_8009BE10;

    if (mode > 0) {
        if (mode < 4) {
            return result;
        }
        if (mode < 8) {
            pActor->unk24 = 1;
        }
    }
    return result;
}

int func_8008D590(int index) {
    WorldMapActor *pActor = &D_8009BE24[index];

    pActor->unk4C = func_80024524(D_8009BDFC[0], 0x100, 0x1FC, 0x160, 0x140, 0x40);
    func_800245D8(pActor->unk4C, 0);
    SpriteSetScale(pActor->unk4C, 0x2000);
    *(u32 *)((u8 *)pActor->unk4C + 0x3C) &= ~4;
    pActor->unk28 = g_GameState.unk1930[0x19] << 12;
    pActor->unk30 = g_GameState.unk1930[0x1A] << 12;
    pActor->unk2C = func_80093978(pActor->unk28, pActor->unk30);
    pActor->unk24 = 1;
    pActor->unk40 = 0;
    pActor->unk3C = 0;
    pActor->unk38 = 0;
    pActor->unk4A = 0xC;
    pActor->unk48 = *(u16 *)&g_GameState.unk1826[2];
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008D678);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008DD6C);
int func_8008DE9C(int index) {
    WorldMapActor *pActor = &D_8009BE24[index];
    s32 result = func_8008C364(pActor, 2);
    s32 mode = D_8009BE10;

    if (mode > 0) {
        if (mode < 4) {
            return result;
        }
        if (mode < 8) {
            pActor->unk24 = 1;
        }
    }
    return result;
}
int func_8008DF0C(int index) {
    WorldMapActor *pActor = &D_8009BE24[index];

    pActor->unk4C = func_80024524(D_8009BE00[0], 0x100, 0x1FB, 0x280, 0x100, 0x40);
    func_800245D8(pActor->unk4C, 0);
    SpriteSetScale(pActor->unk4C, 0x2000);
    *(u32 *)((u8 *)pActor->unk4C + 0x3C) &= ~4;
    pActor->unk28 = g_GameState.unk1930[0x1C] << 12;
    pActor->unk30 = g_GameState.unk1930[0x1D] << 12;
    pActor->unk2C = func_80093978(pActor->unk28, pActor->unk30);
    pActor->unk24 = 1;
    pActor->unk40 = 0;
    pActor->unk3C = 0;
    pActor->unk38 = 0;
    pActor->unk4A = 0xC;
    pActor->unk48 = *(u16 *)&g_GameState.unk1826[4];
    return 1;
}
