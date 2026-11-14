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

typedef struct {
    /* 0x0 */ s32 unk00;
    /* 0x4 */ s32 unk04;
    /* 0x8 */ s32 unk08;
    /* 0xC */ s32 unk0C;
} WorldMapVec; // Size: 0x10

typedef struct {
    /* 0x00 */ WorldMapVec vec;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ u16 pad12;
} WorldMapTrail; // Size: 0x14

// 12.4 fixed point world map position; the range test below uses the middle axis too.
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 unk4;
    /* 0x8 */ s32 z;
} WorldMapPos; // Size: 0xC

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
extern void WorldMapWrapDeltaScaled(WorldMapPos *);
extern s32 SquareRoot0(s32);

extern s32 func_8008C364(WorldMapActor *, s32);
extern void func_80089160(s32, void *, void *);
extern void func_800894C8(s32);
extern s16 WorldMapGetTileHalfIndex(void *);
extern s32 WorldMapGetTilePlaneHeight(s32, s32);
extern void *D_8009BDFC[];
extern void *D_8009BE00[];
extern void *func_80024524(void *, s16, s16, s16, s16, s16);
extern void func_800245D8(void *, s32);
extern void SpriteSetScale(void *, s16);
extern void WorldMapWrapPosition(void *);
extern s32 D_8009C5AC;
extern s32 D_8009C5B4;
extern s32 D_8009C584;
extern WorldMapVec D_8009D55C;
extern WorldMapTrail D_8009CEC4[];
extern u16 D_8009D52C;
extern u16 D_8009D154;

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
    pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
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
    pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
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
    WorldMapWrapPosition(&pActor->unk28);
    pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
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
/* 99.37% (412 bytes, same size). The frame is now exactly retail's 96 bytes: the
   original has 16 bytes of unreferenced local area at sp+0x00 (diff sits at sp+0x10
   and is the only addressable local), and a dead "s32 tmp[4]" local reproduces it --
   every sp offset in the prologue and epilogue matches. The only remaining
   difference is the allocator tie s1/s2 between the loop-carried entry pointer
   (retail s1, ours s2) and the index (retail s2, ours s1); every other insn is
   byte-identical. Tried: indexed &D_8009BE6C[i] (95.09), the >>12 hoisted into each
   arm (98.19), shared hi/lo subtraction (94.31), i declared first, pEntry declared
   first (both identical), pEntry++,i++ instead of i++,pEntry++ (97.80, 408 bytes),
   tmp[4] in four declaration positions (99.32-99.37). At global-alloc time the
   pointer only has ~2 refs (loop.c moves the body's loads onto the pEntry+0x14 biv,
   which is why retail's base register is entry+0x14 with negative displacements)
   against the index's 3, which is what puts the index in the lower register.
void func_8008C040(WorldMapPos *pPos, s32 arg1, s32 arg2, u8 *pOut, u8 *pOut2) {
    WorldMapEntry *pEntry = D_8009BE6C;
    s32 i;
    WorldMapPos diff;
    s32 tmp[4];
    s32 pos;
    s32 base;
    s32 dist;

    *pOut = 0;
    *pOut2 = 0;
    if (D_8009BD04 > 0) {
        for (i = 0; i < D_8009BD04; i++, pEntry++) {
            pos = pPos->unk4;
            if (pos < pEntry->unk08) {
                base = pos - arg2 * 0x1000;
                pos = pEntry->unk08;
            } else {
                base = pEntry->unk08 - pEntry->unk10 * 0x1000;
            }
            if (((pos - base) >> 12) < arg2 + pEntry->unk10) {
                diff.x = (pEntry->unk04 - pPos->x) >> 12;
                diff.z = (pEntry->unk0C - pPos->z) >> 12;
                WorldMapWrapDeltaScaled(&diff);
                dist = SquareRoot0(diff.x * diff.x + diff.z * diff.z);
                if (dist < pEntry->unk14 + arg1) {
                    *pOut = 2;
                } else if (dist < pEntry->unk14 + arg1 + 0x10) {
                    *pOut = 1;
                } else {
                    continue;
                }
                *pOut2 = pEntry->unk00;
                break;
            }
        }
    }
}
*/
void func_8008C1DC(s32 arg0, WorldMapActor *pActor, WorldMapScreenPos *pPos) {
    if (WorldMapGetTileHalfIndex(&pActor->unk28) == 3) {
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
/* 78.83% (432 bytes; retail is 444). Structurally right and byte-identical from the
   WorldMapGetTilePlaneHeight jal onward, including the 32-iteration 0x14-stride copy to
   D_8009CEC4 and the tail. Two source facts established:
   (1) the mode chain must be written inverted -- "if (mode >= 4) { if (mode < 8) {
       unk20 = 2; unk24 = 1; } } else { ...gearRide... }" -- because that is the only
       spelling that puts the gearRide block out of line behind "bnez v0, A" like
       retail; the natural "if (mode < 4) A else if (mode < 8) B" compiles to the
       mirror image (79.81% but with the two blocks swapped).
   (2) the 16-byte copies are struct assignments (WorldMapVec), which is what makes
       gcc emit the 3-load / 3-store / 1-load / 1-store block move retail has; four
       separate field stores interleave load/store instead.
   What still differs is the same store-sink wall as func_8008D3F0 and func_8008DD6C
   in this unit: retail keeps the three "unk40/unk3C/unk38 = 0" stores above the
   D_8009BE10 load, gcc sinks them below it, and inside the gearRide block retail
   keeps "unk20 = 1" first while gcc sinks it below the two D_8009C5AC/D_8009C5B4
   global loads. Source order of the stores does not change it. Everything else
   (lh reload of unk48 for unk5C, the delay slot store, unk4A = 0xC, the tail) matches.
s32 func_8008C530(int index) {
    WorldMapActor *pActor = &D_8009BE24[index];
    s32 result = func_8008C364(pActor, 0);
    WorldMapTrail *pTrail;
    s32 mode;
    s32 i;

    pActor->unk40 = 0;
    pActor->unk3C = 0;
    pActor->unk38 = 0;
    pActor->unk48 = *(u16 *)&g_GameState.unk1826[0];
    pActor->unk4A = 0xC;
    pActor->unk5C = pActor->unk48;
    mode = D_8009BE10;
    if (mode > 0) {
        if (mode >= 4) {
            if (mode < 8) {
                pActor->unk20 = 2;
                pActor->unk24 = 1;
            }
        } else {
            if (g_GameState.gearRide[0] == 1) {
                pActor->unk20 = 1;
                pActor->unk28 = D_8009C5AC;
                pActor->unk30 = D_8009C5B4;
                pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
                pActor->unk48 = D_8009C584;
                D_8009D55C = *(WorldMapVec *)&pActor->unk28;
                D_8009D52C = pActor->unk48;
                D_8009D154 = 0;
                pTrail = D_8009CEC4;
                for (i = 0; i < 0x20; i++, pTrail++) {
                    pTrail->vec = *(WorldMapVec *)&pActor->unk28;
                    pTrail->unk10 = pActor->unk48;
                }
            }
        }
    }
    g_GameState.unk1930[0x16] = pActor->unk28 >> 12;
    g_GameState.unk1930[0x17] = pActor->unk30 >> 12;
    *(u16 *)&g_GameState.unk1826[0] = pActor->unk48;
    return result;
}

*/
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
    pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
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
   Re-diffed under the current XenoWorldMap flags, still 81.33%. Two new facts:
   the source ORDER of the zero stores makes no difference - gcc's post-reload
   scheduler sinks all three to just before the branch and floats the sh/lh pair
   above the D_8009BE10 load whatever the C order is, and hoisting the mode load
   to the top of the head only makes it worse. And storing unk48 through a
   volatile u16 pointer reproduces the original's lh reload (instead of an
   sll/sra pair, which makes the function 304 bytes instead of the original 300):
   that variant scores 80.07% but has the original's exact instruction multiset,
   with only the position of the two blocks differing, so it is the base to
   continue from rather than the text below.
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
                pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
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
    pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
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
/* 81.33%: structurally the same function as func_8008D3F0 (mode 2, gearRide[2],
   g_GameState+0x182A and unk1930[0x1C]/[0x1D]); the whole branch body and the tail
   are byte-identical. Two head differences remain: (1) the original emits the three
   zero stores to 0x40/0x3C/0x38 immediately after the func_8008C364 call and the
   lhu of g_GameState+0x182A right after them, while gcc sinks the zero stores below
   the D_8009BE10 load; (2) the original re-reads unk48 with lh and sign-extends for
   unk5C, gcc keeps the value in a register (sll/sra pair). Also tried a u16 temp for
   the g_GameState load: same 81.33%, and it makes the unk5C store worse.
int func_8008DD6C(int index) {
    WorldMapActor *pActor = &D_8009BE24[index];
    s32 result = func_8008C364(pActor, 2);
    s32 mode;

    pActor->unk40 = 0;
    pActor->unk3C = 0;
    pActor->unk38 = 0;
    pActor->unk4A = 0xC;
    pActor->unk58 = 0x1F;
    pActor->unk48 = *(u16 *)&g_GameState.unk1826[4];
    pActor->unk5C = pActor->unk48;
    mode = D_8009BE10;
    if (mode > 0) {
        if (mode < 4) {
            if (g_GameState.gearRide[2] == 1) {
                pActor->unk20 = 1;
                pActor->unk28 = D_8009C5AC;
                pActor->unk30 = D_8009C5B4;
                pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
                pActor->unk48 = D_8009C584;
            }
        } else if (mode < 8) {
            pActor->unk20 = 2;
            pActor->unk24 = 1;
        }
    }
    g_GameState.unk1930[0x1C] = pActor->unk28 >> 12;
    g_GameState.unk1930[0x1D] = pActor->unk30 >> 12;
    *(u16 *)&g_GameState.unk1826[4] = pActor->unk48;
    return result;
}
*/
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
    pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
    pActor->unk24 = 1;
    pActor->unk40 = 0;
    pActor->unk3C = 0;
    pActor->unk38 = 0;
    pActor->unk4A = 0xC;
    pActor->unk48 = *(u16 *)&g_GameState.unk1826[4];
    return 1;
}
