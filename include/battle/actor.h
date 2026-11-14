#ifndef _XENO_BATTLE_ACTOR_H
#define _XENO_BATTLE_ACTOR_H

#include "psyq/libgte.h"

// A queued movement/rotation command, allocated by BattleAllocBlock and linked
// into the owning sprite's p70 slot.
typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x1 */ u8 unk1;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 unk3;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
    /* 0x8 */ s16 unk8;
    /* 0xA */ s16 unkA;
    /* 0xC */ s16 unkC;
    /* 0xE */ s16 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ s16 unk12;
} BattleBlock; // Size: 0x14

// One part of an actor's model: its own transform plus the command queue.
// Also the element type of a battle actor's sprite array (stride 0x7C).
typedef struct {
    /* 0x00 */ u8 unk0[0x5];
    /* 0x05 */ u8 unk5;
    /* 0x06 */ u8 unk6[0x6];
    /* 0x0C */ MATRIX localTransform;
    /* 0x2C */ MATRIX globalTransform;
    /* 0x4C */ u8 unk4C[0x8];
    /* 0x54 */ s16 rot[3];
    /* 0x5A */ u16 pad5A;
    /* 0x5C */ s32 pos[3];
    /* 0x68 */ void* p68;
    /* 0x6C */ void* p6C;
    /* 0x70 */ BattleBlock* p70;
    /* 0x74 */ BattleBlock* p74;
    /* 0x78 */ BattleBlock* p78;
} BattleSprite; // Size: 0x7C

// A battle participant. D_800D3368[] holds one pointer per slot. Only the
// fields read by src/battle/main/misc24 are named; other battle units reach
// past 0x186, so 0x1A0 is a lower bound on the real size.
typedef struct {
    /* 0x000 */ u8 unk0[0x4];
    /* 0x004 */ BattleSprite* pSprites;
    /* 0x008 */ s32 unk8;
    /* 0x00C */ s32 unkC;
    /* 0x010 */ s32 unk10;
    /* 0x014 */ s32 unk14;
    /* 0x018 */ s32 unk18;
    /* 0x01C */ s16 unk1C;
    /* 0x01E */ s16 unk1E;
    /* 0x020 */ u8 unk20[0x3];
    /* 0x023 */ u8 unk23;
    /* 0x024 */ s16 unk24;
    /* 0x026 */ s16 unk26;
    /* 0x028 */ s16 unk28;
    /* 0x02A */ u8 unk2A[0x1];
    /* 0x02B */ u8 unk2B;
    /* 0x02C */ u8 unk2C[0x8];
    /* 0x034 */ u8 unk34;
    /* 0x035 */ u8 unk35;
    /* 0x036 */ u8 unk36;
    /* 0x037 */ u8 unk37;
    /* 0x038 */ u8 unk38;
    /* 0x039 */ u8 unk39;
    /* 0x03A */ s16 unk3A;
    /* 0x03C */ u16 unk3C;
    /* 0x03E */ u16 unk3E;
    /* 0x040 */ u16 unk40;
    /* 0x042 */ u16 unk42;
    /* 0x044 */ u8 unk44[0x6];
    /* 0x04A */ u16 flags4A;
    /* 0x04C */ s32 unk4C;
    /* 0x050 */ s32 unk50;
    /* 0x054 */ s32 unk54;
    /* 0x058 */ u16 unk58;
    /* 0x05A */ u16 unk5A;
    /* 0x05C */ u8 unk5C;
    /* 0x05D */ u8 unk5D;
    /* 0x05E */ s16 unk5E;
    /* 0x060 */ u8 unk60[0xA];
    /* 0x06A */ u16 unk6A;
    /* 0x06C */ u16 unk6C;
    /* 0x06E */ u16 unk6E;
    /* 0x070 */ u16 unk70[0xC];
    /* 0x088 */ s16 unk88;
    /* 0x08A */ s16 unk8A;
    /* 0x08C */ s16 unk8C;
    /* 0x08E */ u16 unk8E;
    /* 0x090 */ u8 unk90[0x8];
    /* 0x098 */ s16 unk98;
    /* 0x09A */ s16 unk9A;
    /* 0x09C */ s16 unk9C;
    /* 0x09E */ s16 unk9E;
    /* 0x0A0 */ mem_addr unkA0;
    /* 0x0A4 */ mem_addr unkA4;
    /* 0x0A8 */ u8 unkA8[0x8];
    /* 0x0B0 */ u8* unkB0;
    /* 0x0B4 */ u8* unkB4;
    /* 0x0B8 */ u8 unkB8[0x52];
    /* 0x10A */ u16 unk10A;
    /* 0x10C */ u8 unk10C;
    /* 0x10D */ u8 unk10D[0x3];
    /* 0x110 */ mem_addr unk110;
    /* 0x114 */ u8 unk114[0x8C];
} BattleActor; // Size: 0x1A0

// One 0x170-byte slot of the per-battle-slot table family; only the flag word
// at +0 is read by this unit.
typedef struct {
    /* 0x000 */ u16 flags;
    /* 0x002 */ u8 unk02[0x16E];
} BattleTableEntry; // Size: 0x170

#endif
