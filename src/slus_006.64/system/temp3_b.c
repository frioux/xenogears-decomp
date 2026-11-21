#include "common.h"
#include "system/archive.h"
#include "system/memory.h"
#include "system/sound.h"

extern u8 g_GameSelectedSkinLoadPending;
extern u8 D_8005946C;
extern u8 g_SystemBackdropColor[3];
extern s32 g_SystemSemiTransMode;

void func_8001B970(void);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp3_b", func_8001BB50);
/* Best: 80.00% (108 bytes vs retail's 92) in this TU with the four globals as `extern`; the
   only difference is the four stores, `lui $at,%hi / sb %lo($at)` here against retail's
   `sb $v0,0($gp)` + GPREL16. Declaring them as DEFINITIONS in this TU (maspsx relaxation is
   object-wide) gives retail's exact 92 bytes / 23 insns with only the D_800594D5/D_800594D6
   vs g_SystemBackdropColor+1/+2 relocation names differing -- the linked-bytes-identical
   case. They are externs here on purpose: this file is the objdiff-config state of the split
   (byte-identical build proven first), and the conversion is the next commit.
void func_8001BB50(void) {
    g_GameSelectedSkinLoadPending = 1;
    D_8005946C = 0;
    func_8001B970();
    ArchiveCdDataSync(0);
    g_SystemBackdropColor[0] = 0x88;
    g_SystemBackdropColor[1] = 0x76;
    g_SystemBackdropColor[2] = 0x54;
    g_SystemSemiTransMode = 2;
} */

// .bss
u32 g_SoundStreamReserveStart;
u32 g_SoundStreamReserveBlock;
u8 g_SoundEffectBankIndex;
SoundFile* g_SoundSedsBank;
void* g_SoundStreamBuffer;

extern StreamDataQueueEntry g_SoundSedsStreamQueue[];
extern u8 g_SoundEffectIds[];
extern void SoundAddSedsEntry(SoundFile* pSoundFile);
extern void SoundPlaySoundEffectOnLastPair(s32 arg0);

void SoundLoadAndPlaySedsBank(void) {
    int i;
    u8 soundEffectId;
    void* pEndAddress;

    HeapChangeCurrentUser(HEAP_USER_HIG, 0);
    ArchiveSetIndex(0xC, 0);

    g_SoundStreamReserveStart = HeapAlloc(4, 1);
    g_SoundStreamReserveBlock = HeapAlloc(g_SoundStreamReserveStart + 0x7FE1C000, 1);
    pEndAddress = (void*)0x801E4000;
    g_SoundSedsBank = HeapAlloc(ArchiveDecodeAlignedSize(2), 1);
    g_SoundStreamBuffer = HeapAlloc(ArchiveDecodeAlignedSize(3), 1);

    g_SoundSedsStreamQueue[0].archiveIndex = 2;
    g_SoundSedsStreamQueue[1].archiveIndex = 3;
    g_SoundSedsStreamQueue[1].pData = g_SoundStreamBuffer;
    g_SoundSedsStreamQueue[2].archiveIndex = 4;
    g_SoundSedsStreamQueue[2].pData = pEndAddress;
    g_SoundSedsStreamQueue[3].archiveIndex = 0;
    g_SoundSedsStreamQueue[3].pData = NULL;
    g_SoundSedsStreamQueue[0].pData = g_SoundSedsBank;
    ArchiveStartStreamQueue(g_SoundSedsStreamQueue, 0, 0x80);

    while (ArchiveDataSync() == 3) {}

    SoundAddSedsEntry(g_SoundSedsBank);

    if (g_SoundEffectBankIndex != 4) {
        for (i = 0; i < 3;) {
            soundEffectId = g_SoundEffectIds[g_SoundEffectBankIndex * 3 + i];
            i++;
            if (soundEffectId != 0xFF) {
                SoundPlaySoundEffectOnLastPair((g_SoundSedsBank->sedId << 16) | soundEffectId);
            }
        }
    }
}

extern int rand(void);

s32 GameRandomByteRange(u32 arg0, u32 arg1) {
    u32 arg0Byte;
    u32 arg1Byte;
    s32 range;
    s32 result;

    arg0Byte = arg0 & 0xFF;
    if (arg0Byte == 0xFF) {
        return 0xFF;
    }
    arg1Byte = arg1 & 0xFF;
    if (arg1Byte == 0) {
        return 0;
    }
    if (arg0Byte == arg1Byte) {
        return arg0Byte;
    }
    range = arg1Byte - arg0Byte;
    if (range < 0xFF) {
        result = (arg0 + (rand() & 0xFF) % (range + 1)) & 0xFF;
    } else {
        result = rand() & 0xFF;
    }
    return result;
}
