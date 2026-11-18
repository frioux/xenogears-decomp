#include "common.h"
#include "psyq/libgte.h"
#include "system/memory.h"
#include "system/kernel.h"
#include "system/sound.h"
#include "field/actor.h"

extern void SoundFreeWdsEntry(SoundWDSEntry *pWds);
extern void SoundRemoveSedsEntry(void *pSoundFile);
extern SoundWDSEntry *g_BattleWdsEntry;
extern SoundWDSEntry *SoundLoadWdsFile(SoundWDSEntry *pWdsFile, s32 arg1);
extern void SoundAddSedsEntry(SoundFile *pSoundFile);
extern s16 SoundGetPendingSpuCommandType(s32 arg0);
extern void SpriteSetupAnimPackage(SpriteAnimPackage *pAnimPackage, SpriteAnimPackageFileHeader *pAnimPackageFile,
                                   SVEC2 tex, SVEC2 clut, u32 field_0x3C_1);
extern SpriteAnimPackage g_SpriteAnimPackageDirs6;
extern u8 g_BattleSoundArchiveLoaded;
extern u8 g_BattleActorArchiveReady;

// A packed battle archive buffer: the word at 0 counts the three header words that
// follow it plus one offset word per chunk, so a directory walk runs count - 3
// times over the offsets at 0x10. Each offset is relative to the buffer start and
// the chunk it points at begins with a signature word saying what the chunk holds.
typedef struct {
    /* 0x00 */ s32 dirWordCount;
    /* 0x04 */ u8 unk4[0xC];
    /* 0x10 */ s32 chunkOffsets[1];
} SoundArchiveBuffer;

void BattleCalcLookAtRotation(SVECTOR *pFrom, SVECTOR *pTo, u16 *pRot) {
    MATRIX mtx; // unused: reproduces retail's 0x20-byte stack slot
    VECTOR diff;
    VECTOR sq;
    s32 len;

    diff.vx = pFrom->vx - pTo->vx;
    diff.vy = pFrom->vy - pTo->vy;
    diff.vz = pFrom->vz - pTo->vz;
    Square0(&diff, &sq);
    len = SquareRoot0(sq.vx + sq.vz);
    pRot[1] = ratan2(diff.vz, diff.vx);
    pRot[2] = ratan2(diff.vy, len);
    pRot[0] = 0;
}
typedef void (*BattleVectorCallback)(VECTOR *pFrom, VECTOR *pTo);

extern void func_800C0D18(s32 group, s32 angle, SVECTOR *pPos, VECTOR *pOut);
extern s32 D_800D2FCC;

// Emits the trail segments for a direction-transform task: for each of the five
// rotation groups it averages the four source positions under the group's eight
// rotation matrices and hands the result to pCallback together with the previous
// one. partCount below 5 pads the source array up to five entries first.
void func_800C08CC(s32 partCount, SVECTOR *pVectors, BattleVectorCallback pCallback) {
    VECTOR vecA;
    VECTOR vecB;
    SVECTOR vecC; // unused: reproduces retail's 0x60 frame
    s32 i;
    s32 n;

    D_800D2FCC = 0;
    if (partCount < 5) {
        if (partCount < 3) {
            if (partCount < 2) {
                return;
            }
            vecA.vx = pVectors[0].vx;
            vecA.vy = pVectors[0].vy;
            vecA.vz = pVectors[0].vz;
            vecB.vx = pVectors[1].vx;
            vecB.vy = pVectors[1].vy;
            vecB.vz = pVectors[1].vz;
            pCallback(&vecA, &vecB);
            return;
        }
        switch (partCount) {
        case 3:
            pVectors[3].vx = pVectors[2].vx;
            pVectors[3].vy = pVectors[2].vy;
            pVectors[3].vz = pVectors[2].vz;
            pVectors[4].vx = pVectors[2].vx;
            pVectors[4].vy = pVectors[2].vy;
            pVectors[4].vz = pVectors[2].vz;
            break;
        case 4:
            pVectors[4].vx = pVectors[3].vx;
            pVectors[4].vy = pVectors[3].vy;
            pVectors[4].vz = pVectors[3].vz;
            break;
        }
        partCount = 5;
    }
    func_800C0D18(0, 0, pVectors, &vecA);
    n = 0;
    for (i = 1; i != 8; i++) {
        if (i & 1) {
            func_800C0D18(0, i, &pVectors[n], &vecB);
            pCallback(&vecA, &vecB);
        } else {
            func_800C0D18(0, i, &pVectors[n], &vecA);
            pCallback(&vecB, &vecA);
        }
    }
    if (partCount >= 6) {
        n = 1;
        func_800C0D18(1, 0, &pVectors[n], &vecA);
        pCallback(&vecB, &vecA);
        for (i = 1; i != 8; i++) {
            if (i & 1) {
                func_800C0D18(1, i, &pVectors[n], &vecB);
                pCallback(&vecA, &vecB);
            } else {
                func_800C0D18(1, i, &pVectors[n], &vecA);
                pCallback(&vecB, &vecA);
            }
        }
    }
    for (n = 2; n < partCount - 5; n++) {
        func_800C0D18(2, 0, &pVectors[n], &vecA);
        pCallback(&vecB, &vecA);
        for (i = 1; i != 8; i++) {
            if (i & 1) {
                func_800C0D18(2, i, &pVectors[n], &vecB);
                pCallback(&vecA, &vecB);
            } else {
                func_800C0D18(2, i, &pVectors[n], &vecA);
                pCallback(&vecB, &vecA);
            }
        }
    }
    if (partCount >= 7) {
        n = partCount - 5;
        func_800C0D18(3, 0, &pVectors[n], &vecA);
        pCallback(&vecB, &vecA);
        for (i = 1; i != 8; i++) {
            if (i & 1) {
                func_800C0D18(3, i, &pVectors[n], &vecB);
                pCallback(&vecA, &vecB);
            } else {
                func_800C0D18(3, i, &pVectors[n], &vecA);
                pCallback(&vecB, &vecA);
            }
        }
    }
    n = partCount - 4;
    func_800C0D18(4, 0, &pVectors[n], &vecA);
    pCallback(&vecB, &vecA);
    for (i = 1; i != 8; i++) {
        if (i & 1) {
            func_800C0D18(4, i, &pVectors[n], &vecB);
            pCallback(&vecA, &vecB);
        } else {
            func_800C0D18(4, i, &pVectors[n], &vecA);
            pCallback(&vecB, &vecA);
        }
    }
    if (partCount >= 5) {
        vecA.vx = pVectors[partCount - 1].vx;
        vecA.vy = pVectors[partCount - 1].vy;
        vecA.vz = pVectors[partCount - 1].vz;
        pCallback(&vecB, &vecA);
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc35", func_800C0D18);

void BattleFreeWdsEntry(void) {
    if (g_BattleWdsEntry != 0) {
        SoundFreeWdsEntry(g_BattleWdsEntry);
    }
    g_BattleWdsEntry = 0;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc35", func_800C0FAC);
// 96.88% (400 bytes against retail's 404). The loop, the switch arm order (seds
// listed first is what puts the seds body inline and the WDS arm out of line, as
// retail has it) and the frame are exact. The one residual: retail hoists ONLY the
// high half of the WDS entry address into $s7 in the loop preheader and keeps the
// store's %lo displacement (lui $s7,%hi / sw $v0,%lo($s7)), while we re-form the
// pair inside the loop body (lui $v1,%hi / sw $v0,%lo($v1)) - 4 bytes. Measured
// dead ends: a pointer local for the entry (lui+addiu, 408 bytes), reading
// g_DebugTable directly instead of through pDebugState (392, loses a saved
// register), a volatile read of it, and a 4-minute permuter run (2122 iterations,
// base=321 best=321). The dead pDebugState assignment below (unreachable after the
// seds arm's break; gcc still hoists its address into $s7) is what reproduces the
// frame, the saved-register set and the dead 0xFFFF0000 preheader constant, so it
// is load-bearing - do not delete it without re-measuring.
/*
// Feeds one battle actor archive buffer to the sound player: every SEDS chunk is
// added, every WDS chunk is (re)loaded after freeing the loaded one, and every
// other chunk sets up an animation package. Returns the last SEDS chunk added.
SoundFile *func_800C0FAC(SoundArchiveBuffer *pBuffer) {
    SoundFile *pSeds;
    SoundFile *pChunk;
    s32 *pEntry;
    s32 *pDebugState;
    SVEC2 tex;
    SVEC2 clut;
    s32 i;

    pSeds = NULL;
    pEntry = (s32 *)pBuffer;
    i = pEntry[0] - 3;
    pEntry += 4;
    while (i > 0) {
        pChunk = (SoundFile *)(pEntry[0] + (s32)pBuffer);
        switch (pChunk->magic) {
        case FILE_SIGNATURE('s', 'e', 'd', 's'):
            pSeds = pChunk;
            SoundAddSedsEntry(pSeds);
            break;
            pDebugState = &g_DebugTable;
        case FILE_SIGNATURE('w', 'd', 's', ' '):
            BattleFreeWdsEntry();
            g_BattleSoundArchiveLoaded = 0;
            g_BattleActorArchiveReady = 0;
            g_BattleWdsEntry = SoundLoadWdsFile((SoundWDSEntry *)pChunk, 0);
            while (SoundGetPendingSpuCommandType(0) != 0) {
                if (*pDebugState != -1) {
                    asm("break 0x400");
                }
            }
            if (i == 1) {
                HeapInsertAlloc((HeapBlock *)pBuffer, pEntry[0]);
            }
            break;
        default:
            tex.x = 0x380;
            tex.y = 0x100;
            clut.x = 0;
            clut.y = 0x1F4;
            SpriteSetupAnimPackage(&g_SpriteAnimPackageDirs6, (SpriteAnimPackageFileHeader *)pChunk, tex, clut, 0);
            break;
        }
        i--;
        pEntry++;
    }
    return pSeds;
}
*/

void BattleRemoveSedsEntries(s32 *pArg) {
    s32 *pEntry = pArg;
    s32 i = pEntry[0] - 3;

    pEntry += 4;
    for (; i > 0; i--) {
        s32 *pChunk = (s32 *)(pEntry[0] + (s32)pArg);

        if (*pChunk == 0x73646573) {
            SoundRemoveSedsEntry(pChunk);
        }
        pEntry++;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc35", func_800C11CC);
