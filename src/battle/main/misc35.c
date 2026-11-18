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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc35", func_800C08CC);
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
// retail has it) and the frame are exact; retail's preheader holds
// lui $s7,%hi(g_BattleWdsEntry), li $s6,-1 and a dead lui $s5,0xFFFF0000, and the
// store becomes sw $v0,%lo(g_BattleWdsEntry)($s7). Ours re-forms the address inline
// (lui $v1,%hi / sw $v0,%lo($v1)) - the missing 4 bytes. Making g_BattleWdsEntry a
// pointer-deref store instead gives lui+addiu and an 0x88 frame (worse); the dead
// pDebugState assignment below (unreachable after the seds arm's break; gcc still
// hoists its address) is what reproduces the frame, the saved-register set and the
// dead 0xFFFF0000 preheader constant, so it is load-bearing - do not delete it
// without re-measuring. Two 5-minute permuter runs both plateaued at a block
// duplication of the default arm (score 321 against 592), which is what pointed at
// the pointer-local read of g_DebugTable in the first place.
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
                // Debugging
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
