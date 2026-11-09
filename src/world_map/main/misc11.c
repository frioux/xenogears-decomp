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
    /* 0x48 */ undefined32 unk48;
    /* 0x4C */ void *unk4C;
    /* 0x50 */ s32 unk50;
    /* 0x54 */ void *unk54;
    /* 0x58 */ void *unk58;
    /* 0x5C */ s32 unk5C;
    /* 0x60 */ undefined32 unk60[8]; // 0x60..0x7F
} WorldMapActor; // Size: 0x80

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
extern void *func_80024524(void *, s16, s16, s16, s16, s16);
extern void func_800245D8(void *, s32);
extern void SpriteSetScale(void *, s16);

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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008BEC8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008BFD4);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008C040);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008C1DC);
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

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008C75C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008C844);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008D3F0);

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

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008D590);
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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc11", func_8008DF0C);
