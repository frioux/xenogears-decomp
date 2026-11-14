#ifndef _XENO_BATTLE_WORK_H
#define _XENO_BATTLE_WORK_H

#include "psyq/libgpu.h"

// Per-frame battle display state (D_800D2D28): the flags and atlas indices the
// battle draw units read while building the ordering table. Only the fields
// src/battle/main/misc2 touches are named; other units reach past 0x104, so
// 0x108 is a lower bound on the real size.
typedef struct {
    /* 0x000 */ u8 unk0[0x9C];
    /* 0x09C */ u8 unk9C;
    /* 0x09D */ u8 unk9D;
    /* 0x09E */ u8 unk9E;
    /* 0x09F */ u8 unk9F;
    /* 0x0A0 */ u8 unkA0;
    /* 0x0A1 */ u8 unkA1;
    /* 0x0A2 */ u8 unkA2;
    /* 0x0A3 */ u8 unkA3;
    /* 0x0A4 */ u8 unkA4;
    /* 0x0A5 */ u8 unkA5;
    /* 0x0A6 */ u8 unkA6;
    /* 0x0A7 */ u8 unkA7;
    /* 0x0A8 */ u8 unkA8;
    /* 0x0A9 */ u8 unkA9[0x1D];
    /* 0x0C6 */ u8 unkC6;
    /* 0x0C7 */ u8 unkC7;
    /* 0x0C8 */ u8 unkC8;
    /* 0x0C9 */ u8 unkC9;
    /* 0x0CA */ u8 unkCA;
    /* 0x0CB */ u8 unkCB;
    /* 0x0CC */ u8 unkCC[0x4];
    /* 0x0D0 */ s32 unkD0[2];
    /* 0x0D8 */ u8 unkD8[0x20];
    /* 0x0F8 */ s32 unkF8;
    /* 0x0FC */ s32 unkFC;
    /* 0x100 */ s32 unk100;
    /* 0x104 */ u8 unk104[0x4];
} BattleWork; // Size: 0x108

// Prim pool for the sprite package drawn by BattleAddPrimAndRenderObject
// (D_800D3278). 0x7A4 holds the two POLY_FT4 the 0x7F4 counter selects between.
typedef struct {
    /* 0x000 */ u8 unk0[0x7A4];
    /* 0x7A4 */ POLY_FT4 unk7A4[2];
    /* 0x7F4 */ u8 unk7F4;
} BattlePrimPool; // Size: 0x7F5

#endif
