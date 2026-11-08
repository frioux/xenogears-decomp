#include "common.h"
#include "field/actor.h"
#include "field/main.h"
#include "psyq/libgpu.h"
#include "psyq/libetc.h"
#include "system/archive.h"
#include "system/memory.h"
#include "system/controller.h"
#include "field/effects.h"

extern int g_FrameDeltaTime;

extern void ArchiveCdDataSync(int mode);
extern s32 D_800ADBD0;
extern s32 g_PlayerActorIndex;
extern s16 D_800B2344;

// g_FieldActors is volatile; reading it through a non-volatile lvalue reproduces
// the original's delay-slot filling.
#define FIELD_ACTORS (*((FieldActor**)(void*)&g_FieldActors))

extern s32 g_GameSceneMapNum;
extern int func_8001B484(int fileIndex, s32 arg1);
void FieldInitializeControllersAndMouse(void);
void func_80077884(void);
void func_80077AB4(void);

void FieldInitializeControllers(void) {
    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
    ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);
    FieldInitializeControllersAndMouse();
}

void FieldRenderSync(void) {
    DrawSync(0);
    Vsync(0);
}

extern s32 D_8004F344;
extern void* D_8005A4A0;
extern s16 D_800ADC44[];
extern s16 D_800C2690;
extern s16 D_800C2692;
extern s16 D_800C38FC;
extern s16 D_800C38FE;
extern RECT D_800B004C;
extern u_long D_800AFC08[];

extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern void ResolveArchiveEntryPointers(u32* header);
extern void FieldLoadTIMWithClut(u_long* pTimData, short x, short y, short clutX, short clutY,
                                 short clutWidth, short clutHeight);

INCLUDE_ASM("asm/field/nonmatchings/main/main", FieldLoadUITextures);
// 85.72%: D_800ADC44 is 8 entries of six shorts (x, y, clutX, clutY, clutWidth, clutHeight).
// The original walks the table twice with pointer variables (one at offset 0 for x, one at
// 0xA for clutHeight) AND re-derives `&D_800ADC44 + i * 12` from the symbol for the middle
// four arguments - four separate `addu $v0,$s2,$s0` with offsets 2/4/6/8, i.e. it does not
// CSE the base. Every C form tried (flat s16 array indexed i*6+k, struct array indexed,
// pointer locals for all six) makes gcc either fold +2 into the base register or merge the
// index into the pointer, and the RECT stores below come out as %hi(D_800B004C)+2 instead
// of the original's separate D_800B004E/50/52. Frame and prologue match.
/*
typedef struct {
    s16 x; s16 y; s16 clutX; s16 clutY; s16 clutWidth; s16 clutHeight;
} FieldUITextureRect; // Size: 0xC

extern FieldUITextureRect D_800ADC44[];

void FieldLoadUITextures(void) {
    FieldUITextureRect* pTable;
    s16* pRect;
    s16* pHeight;
    s32* pEntry;
    int i;

    if (D_8004F344 == 0) {
        D_8005A4A0 = HeapAlloc(ArchiveDecodeAlignedSize(0xA7), 1);
        HeapPinBlock(D_8005A4A0);
        ArchiveReadFileToBuffer(0xA7, D_8005A4A0, 0, 0x80);
        ArchiveCdDataSync(0);
    }
    HeapUnpinBlock(D_8005A4A0);
    D_8004F344 = 0;
    D_800C2692 = 0;
    D_800C2690 = 0;
    D_800C38FE = 0;
    D_800C38FC = 0;
    ResolveArchiveEntryPointers(D_8005A4A0);
    pTable = D_800ADC44;
    pRect = D_800ADC44;
    pHeight = &D_800ADC44[5];
    pEntry = (s32*)((u8*)D_8005A4A0 + 4);
    for (i = 0; i < 8; i++) {
        FieldLoadTIMWithClut((u_long*)pEntry[i], *pRect, pTable[i].y, pTable[i].clutX,
            pTable[i].clutY, pTable[i].clutWidth, *pHeight);
        pRect += 6;
        pHeight += 6;
        DrawSync(0);
    }
    D_800B004C.x = 0;
    D_800B004C.y = 0xFB;
    D_800B004C.w = 0x10;
    D_800B004C.h = 1;
    StoreImage(&D_800B004C, D_800AFC08);
    DrawSync(0);
    HeapFree(D_8005A4A0);
}
*/

void func_800777DC(void) {
    ArchiveCdDataSync(0);
    while (func_8001B484((g_GameSceneMapNum & 0xFFF) * 2, 0) != 0) {
    }
}

void FieldUpdateDeltaTime(void) {
    g_FrameDeltaTime = Vsync(1);
}

void func_80077844(s16* pOut, int x0, int x1, int x2, int x3, int x4, int x5, int x6, int x7, int x8) {
    pOut[0] = x0;
    pOut[1] = x1;
    pOut[2] = x2;
    pOut[3] = x3;
    pOut[4] = x4;
    pOut[5] = x5;
    pOut[6] = x6;
    pOut[7] = x7;
    pOut[8] = x8;
}

INCLUDE_ASM("asm/field/nonmatchings/main/main", func_80077884);

extern s32 D_8005A420[];
extern s32 D_8005A450[];
extern s32* D_801E8644;
extern void* D_801E8670[];
extern u8 D_800B225C;
extern u8 D_800B225D;
extern u8 D_800B225E;
extern s16 D_800B234A;
extern s32 D_800B2264;

extern void func_8008A520(void);
extern void FieldRenderSyncAndFlush(void);
extern void func_801E738C(s16 arg0);
extern void func_801E742C(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s16 arg4, s32 arg5, s32 arg6,
                          s32 arg7, SVECTOR* pVector);

INCLUDE_ASM("asm/field/nonmatchings/main/main", func_80077AB4);
// 64.39%: the C below is structurally right (count guard, then HeapChangeCurrentUser(8, NULL))
// but the original materialises &D_800B2264 into $s0 ONCE and reaches the loop vectors and
// D_800B223C with `addiu` off it, while gcc rematerialises a %hi/%lo pair for every one of
// those addresses. Same root cause as func_8007520C (misc2) and func_800A0EE8 (misc6).
// The loop itself is: 8 max entries of three zeroed SVECTOR vertices (8 bytes apart) plus a
// per-entry s32 result; the third and eighth arguments are a `0xFC` byte counter (offsets
// into g_FieldEffects) and `0xFC0000 + i * 0x10000` read as `>> 16`.
/*
void func_80077AB4(void) {
    s32* pCount;
    SVECTOR* pVector;
    s32* pData;
    s32 i;
    s32 clut;
    int texOffset;

    pCount = &D_800B2264;
    if (*pCount == 0) {
        return;
    }

    func_8008A520();
    FieldRenderSyncAndFlush();
    func_801E738C(D_800B234A);
    D_801E8644 = pCount - 10;
    SetBackColor(D_800B225C, D_800B225D, D_800B225E);
    if (*pCount > 0) {
        pVector = (SVECTOR*)(pCount - 30);
        clut = 0xFC0000;
        texOffset = 0xFC;
        pData = D_8005A450;
        for (i = 0; i < *pCount; i++) {
            pVector->vx = 0;
            pVector->vy = 0;
            pVector->vz = 0;
            func_801E742C(i, 0, D_8005A420[i], pData[i],
                (s16)(0x240 - ((i + ((u8*)&g_FieldEffects)[0xEB + texOffset]) * 0x40)), 0x100, 0,
                clut >> 16, pVector++);
            texOffset++;
            HeapFree(pData[i]);
            ((s32*)(pCount - 22))[i] = *(s16*)((u8*)D_801E8670[i] + 0x1C);
            clut += 0x10000;
        }
    }
    HeapChangeCurrentUser(8, NULL);
}
*/

void func_80077C60(void) {
    func_80077884();
    func_80077AB4();
}

extern void* g_PartyDataBuffers[];

void FieldPartyAllocateSkinDataBuffers(void) {
    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
    g_PartyDataBuffers[0] = HeapAlloc(0x14000, 0);
    g_PartyDataBuffers[1] = HeapAlloc(0x14000, 0);
    g_PartyDataBuffers[2] = HeapAlloc(0x14000, 0);
    HeapPinBlock(g_PartyDataBuffers[0]);
    HeapPinBlock(g_PartyDataBuffers[1]);
    HeapPinBlock(g_PartyDataBuffers[2]);
}

void FieldPartyFreeSkinDataBuffers(void) {
    HeapUnpinBlock(g_PartyDataBuffers[0]);
    HeapUnpinBlock(g_PartyDataBuffers[1]);
    HeapUnpinBlock(g_PartyDataBuffers[2]);
    HeapFree(g_PartyDataBuffers[0]);
    HeapFree(g_PartyDataBuffers[1]);
    HeapFree(g_PartyDataBuffers[2]);
}

extern s32 D_800ADB9C;

// .rodata
// Sized to the 0xC bytes splat assigns this unit's rodata subsegment. The last byte
// (0x6B) sits past the string's terminator but is present in the original binary.
const char D_8006FB80[0xC] = { 'C', 'l', 'e', 'a', 'r', ' ', 'O', 'T', 'A', 'G', 0, 0x6B };

void func_80077DAC(void) {
    D_800ADB9C = Vsync(1);
    FieldClearAndSwapOTag();
    FieldPollControllers();
    if (g_FieldSystemMode == SYSTEM_MODE_PC_HDD) {
        func_80281B00(D_8006FB80);
    }
    func_800A31E8();
}

INCLUDE_ASM("asm/field/nonmatchings/main/main", func_80077E10);
// 96.00%: only register allocation differs. The original keeps the result in $v0
// (initialised unconditionally in the first branch's delay slot) and uses $a0/$v1
// for the two comparison temporaries; gcc gives the result $a0 here and the
// temporaries $v1/$v0. Same size (120 bytes) and same instruction sequence.
/*
s32 func_80077E10(void) {
    s32 result = 0;

    if (D_800ADBD0 == 1 && D_800B2344 == 0) {
        result = -((FIELD_ACTORS[g_PlayerActorIndex].pActorData->scriptFlags.flags & 0x800) != 0);
    }
    return result;
}
*/

INCLUDE_ASM("asm/field/nonmatchings/main/main", FieldMain);
