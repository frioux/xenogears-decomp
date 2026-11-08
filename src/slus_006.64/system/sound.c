#include "common.h"
#include "system/sound.h"
#include "psyq/kernel.h"
#include "psyq/libspu.h"

//----------------------------------------------------------------------------------------------------------------------
// SPU DECLARATIONS
//----------------------------------------------------------------------------------------------------------------------
typedef struct {
    /* 0x0 */ SpuVolume volume;
    /* 0x4 */ u16 pitch;
    /* 0x6 */ u16 addr;
    /* 0x8 */ u16 adsr[2];
    /* 0xC */ u16 volumex;
    /* 0xE */ u16 loop_addr;
} SPU_VOICE_REG;

#define SPU_VOICE_REG_VOLUME_L    0
#define SPU_VOICE_REG_VOLUME_R    1
#define SPU_VOICE_REG_PITCH       2
#define SPU_VOICE_REG_ADDR        3
#define SPU_VOICE_REG_ADSR1       4
#define SPU_VOICE_REG_ADSR2       5
#define SPU_VOICE_REG_VOLUMEX     6
#define SPU_VOICE_REG_LOOP_ADDR   7
#define SPU_VOICE_REG_SIZE        8
#define NUM_VOICES               24

typedef struct {
    // APF Displacement registers (1F801DC0h - 1F801DC2h)
    u16 m_dAPF1;
    u16 m_dAPF2;

    // Volume registers (1F801DC4h - 1F801DD2h)
    s16 m_vIIR;
    s16 m_vCOMB1;
    s16 m_vCOMB2;
    s16 m_vCOMB3;
    s16 m_vCOMB4;
    s16 m_vWALL;
    s16 m_vAPF1;
    s16 m_vAPF2;

    // Same Side Reflection Address registers (1F801DD4h - 1F801DD6h)
    u16 m_mLSAME;
    u16 m_mRSAME;

    // Comb Address registers (1F801DD8h - 1F801DDEh)
    u16 m_mLCOMB1;
    u16 m_mRCOMB1;
    u16 m_mLCOMB2;
    u16 m_mRCOMB2;

    // Same Side Reflection Address 2 registers (1F801DE0h - 1F801DE2h)
    u16 m_dLSAME;
    u16 m_dRSAME;

    // Different Side Reflection Address registers (1F801DE4h - 1F801DE6h)
    u16 m_mLDIFF;
    u16 m_mRDIFF;

    // Comb Address registers 3-4 (1F801DE8h - 1F801DEEh)
    u16 m_mLCOMB3;
    u16 m_mRCOMB3;
    u16 m_mLCOMB4;
    u16 m_mRCOMB4;

    // Different Side Reflection Address 2 registers (1F801DF0h - 1F801DF2h)
    u16 m_dLDIFF;
    u16 m_dRDIFF;

    // APF Address registers (1F801DF4h - 1F801DFAh)
    u16 m_mLAPF1;
    u16 m_mRAPF1;
    u16 m_mLAPF2;
    u16 m_mRAPF2;

    // Input Volume registers (1F801DFCh - 1F801DFEh)
    s16 m_vLIN;
    s16 m_vRIN;
} ReverbRegisters;

typedef struct {
    SPU_VOICE_REG voice[NUM_VOICES];
    // Volumes
    SpuVolume main_vol; // 1-bit for Volume Mode, 15-bits for Volume
    SpuVolume rev_vol; // Full 16 bits for volume

    // Voice Flags
    u16 key_on[2];
    u16 key_off[2];
    u16 chan_fm[2];
    u16 noise_mode[2];
    u16 rev_mode[2];
    u32 m_EndxFlags;

    u16 unk;

    // Memory
    u16 rev_work_addr;
    u16 irq_addr;
    u16 trans_addr;
    u16 trans_fifo;

    // Control
    u16 spucnt;
    u16 data_trans;
    u16 spustat;

    // Aux volumes
    SpuVolume cd_vol;
    SpuVolume ex_vol;

    SpuVolume main_volx;

    u32 unk2;

    ReverbRegisters m_Reverb;
} SPU_RXX;

// Voice Registers (0x00 - 0xBF)
#define SPU_RXX_VOICE_BASE              0x00    // Voice registers start (24 voices × 8 regs each)
#define SPU_RXX_VOICE_SIZE              8       // Registers per voice
#define SPU_RXX_VOICE_END               0xBF    // Last voice register
// Global Volume Registers (0xC0 - 0xC3)
#define SPU_RXX_MAIN_VOL_L              0xC0    // Main volume left
#define SPU_RXX_MAIN_VOL_R              0xC1    // Main volume right
#define SPU_RXX_REV_VOL_L               0xC2    // Reverb volume left
#define SPU_RXX_REV_VOL_R               0xC3    // Reverb volume right
// Voice Flag Registers (0xC4 - 0xCF) - 32-bit values stored as pairs
#define SPU_RXX_KEY_ON_LOW              0xC4    // Key on flags (bits 0-15)
#define SPU_RXX_KEY_ON_HIGH             0xC5    // Key on flags (bits 16-31)
#define SPU_RXX_KEY_OFF_LOW             0xC6    // Key off flags (bits 0-15)
#define SPU_RXX_KEY_OFF_HIGH            0xC7    // Key off flags (bits 16-31)
#define SPU_RXX_PITCH_MOD_LOW           0xC8    // Pitch modulation flags (bits 0-15)
#define SPU_RXX_PITCH_MOD_HIGH          0xC9    // Pitch modulation flags (bits 16-31)
#define SPU_RXX_NOISE_LOW               0xCA    // Noise flags (bits 0-15)
#define SPU_RXX_NOISE_HIGH              0xCB    // Noise flags (bits 16-31)
#define SPU_RXX_REVERB_LOW              0xCC    // Reverb flags (bits 0-15)
#define SPU_RXX_REVERB_HIGH             0xCD    // Reverb flags (bits 16-31)
#define SPU_RXX_ENDX_LOW                0xCE    // End flags (bits 0-15)
#define SPU_RXX_ENDX_HIGH               0xCF    // End flags (bits 16-31)
// Memory Address Registers (0xD0 - 0xD4)
#define SPU_RXX_UNKNOWN_D0              0xD0    // Unknown register
#define SPU_RXX_REV_WA_START_ADDR       0xD1    // Reverb work area start address
#define SPU_RXX_IRQ_ADDR                0xD2    // IRQ address
#define SPU_RXX_TRANS_ADDR              0xD3    // Transfer address
#define SPU_RXX_TRANS_FIFO              0xD4    // Transfer FIFO
// Control Registers (0xD5 - 0xD7)
#define SPU_RXX_SPUCNT                  0xD5    // SPU control register
#define SPU_RXX_TRANS_CTRL              0xD6    // Transfer control
#define SPU_RXX_SPUSTAT                 0xD7    // SPU status register
// Audio Input Volume Registers (0xD8 - 0xDB)
#define SPU_RXX_CD_VOL_L                0xD8    // CD input volume left
#define SPU_RXX_CD_VOL_R                0xD9    // CD input volume right
#define SPU_RXX_EXT_VOL_L               0xDA    // External input volume left
#define SPU_RXX_EXT_VOL_R               0xDB    // External input volume right
// Current Volume Registers (0xDC - 0xDD)
#define SPU_RXX_CURR_MAIN_VOL_L         0xDC    // Current main volume left
#define SPU_RXX_CURR_MAIN_VOL_R         0xDD    // Current main volume right
// Unknown Register (0xDE - 0xDF)
#define SPU_RXX_UNKNOWN2_LOW            0xDE    // Unknown register (bits 0-15)
#define SPU_RXX_UNKNOWN2_HIGH           0xDF    // Unknown register (bits 16-31)
// Reverb Registers start at 0xE0
#define SPU_RXX_REVERB_BASE             0xE0    // Reverb parameter registers start

// SPU Register volume modes
#define SPU_VOL_MODE_DIRECT     0x0000
#define SPU_VOL_MODE_LINEARIncN 0x8000
#define SPU_VOL_MODE_LINEARIncR 0x9000
#define SPU_VOL_MODE_LINEARDecN 0xA000
#define SPU_VOL_MODE_LINEARDecR 0xB000
#define SPU_VOL_MODE_EXPIncN    0xC000
#define SPU_VOL_MODE_EXPIncR    0xD000
#define SPU_VOL_MODE_EXPDec     0xE000

#define SPU_VOL_MODE_MASK (1 << 15)
#define SPU_VOL_MAX 0x7F

// SPU Control Register (SPUCNT) bit masks
#define SPU_CTRL_MASK_CD_AUDIO_ENABLE        (1 <<  0)              // 0
#define SPU_CTRL_MASK_EXT_AUDIO_ENABLE       (1 <<  1)              // 1
#define SPU_CTRL_MASK_CD_AUDIO_REVERB        (1 <<  2)              // 2
#define SPU_CTRL_MASK_EXT_AUDIO_REVERB       (1 <<  3)              // 3
#define SPU_CTRL_MASK_SRAM_TRANSFER_MODE    ((1 <<  4) | (1 << 5))  // 4-5
#define SPU_CTRL_MASK_IRQ9_ENABLE            (1 <<  6)              // 6
#define SPU_CTRL_MASK_REVERB_MASTER_ENABLE   (1 <<  7)              // 7
#define SPU_CTRL_MASK_NOISE_FREQ_STEP       ((1 <<  8) | (1 << 9))  // 8-9
#define SPU_CTRL_MASK_NOISE_FREQ_SHIFT      ((1 << 10) | (1 << 11) | (1 << 12) | (1 << 13))  // 10-13
#define SPU_CTRL_MASK_MUTE_SPU               (1 << 14)              // 14
#define SPU_CTRL_MASK_SPU_ENABLE             (1 << 15)              // 15

// SPU Control Register shift amounts for multi-bit fields
#define SPU_CTRL_SRAM_TRANSFER_SHIFT     4
#define SPU_CTRL_NOISE_FREQ_STEP_SHIFT   8
#define SPU_CTRL_NOISE_FREQ_SHIFT_SHIFT 10

#define SPU_CTRL_TRANSFER_MODE_STOP         ( 0 << SPU_CTRL_SRAM_TRANSFER_SHIFT ) // 0x00
#define SPU_CTRL_TRANSFER_MODE_MANUAL_WRITE ( 1 << SPU_CTRL_SRAM_TRANSFER_SHIFT ) // 0x10
#define SPU_CTRL_TRANSFER_MODE_DMA_WRITE    ( 2 << SPU_CTRL_SRAM_TRANSFER_SHIFT ) // 0x20
#define SPU_CTRL_TRANSFER_MODE_DMA_READ     ( 3 << SPU_CTRL_SRAM_TRANSFER_SHIFT ) // 0x30

// SPU Status Register (SPUSTAT) bit masks
#define SPU_STAT_MASK_CURRENT_SPU_MODE      ((1 <<  0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 <<  4) | (1 <<  5))  // 0-5
#define SPU_STAT_MASK_IRQ9_FLAG              (1 <<  6)              // 6
#define SPU_STAT_MASK_DMA_READ_WRITE_REQUEST (1 <<  7)              // 7
#define SPU_STAT_MASK_DMA_WRITE_REQUEST      (1 <<  8)              // 8
#define SPU_STAT_MASK_DMA_READ_REQUEST       (1 <<  9)              // 9
#define SPU_STAT_MASK_DATA_TRANSFER_BUSY     (1 << 10)              // 10
#define SPU_STAT_MASK_CAPTURE_BUFFER_HALF    (1 << 11)              // 11
#define SPU_STAT_MASK_UNKNOWN_UNUSED        ((1 << 12) | (1 << 13) | (1 << 14) | (1 << 15))  // 12-15

// SPU Status Register shift amounts for multi-bit fields
#define SPU_STAT_CURRENT_SPU_MODE_SHIFT      0
#define SPU_STAT_UNKNOWN_UNUSED_SHIFT        12

// SPU Status Register values
#define SPU_STAT_CAPTURE_FIRSTHALF           (0 << 11)  // Writing to first half
#define SPU_STAT_CAPTURE_SECONDHALF          (1 << 11)  // Writing to second half
#define SPU_STAT_TRANSFER_READY              (0 << 10)  // Transfer ready
#define SPU_STAT_TRANSFER_BUSY               (1 << 10)  // Transfer busy

typedef union {
    SPU_RXX _rxx;
    volatile SPU_RXX rxx;
    u16 _raw[0x100];
    volatile u16 raw[0x100];
} SpuUnion;

extern SpuUnion* g_pSoundSpuRegisters;
//----------------------------------------------------------------------------------------------------------------------

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", SoundInitialize);

void SoundReset(void) {
    int i;

    if (g_SoundControlFlags == 0x0) {
        SoundHandleError(0x29);
        return;
    }
    
    EnterCriticalSection();
    g_SoundControlFlags = 0;
    SpuSetIRQ(SPU_OFF);
    SpuSetTransferCallback(NULL);
    SpuSetIRQCallback(NULL);
    StopRCnt(RCntCNT2);
    CloseEvent(g_unk_SoundEvent);
    ExitCriticalSection();
    for (i = 0; i < NUM_VOICES; i++) {
        SoundSetVoiceAdsrReleaseShiftAndMode(i, 6, 3);
    }
    SoundSetVoiceKeyOff(0xFFFFFF); // Release all voices
    SpuSetReverbModeDepth(0, 0);
    SpuSetReverbModeType(SPU_REV_MODE_OFF);
    g_SoundSpuErrorId = 0;
}

void SoundEnableAllSpuChannels(void) {
    SoundVoiceData** pChannel = g_SoundChannels;
    s32 i = 0;

    do {
        if (*pChannel) {
            (*pChannel)->flags |= 0x1F5;
        }
        i++;
        pChannel++;
    } while (i < NUM_VOICES);

    g_SoundControlFlags &= ~0x40;
}

void SoundMuteAllSpuChannels(void) {
    SPU_VOICE_REG* pVoice = g_pSoundSpuRegisters->_rxx.voice;
    s32 i;

    g_SoundControlFlags |= 0x40;
    for (i = 0; i < NUM_VOICES; pVoice++, i++) {
        pVoice->volume.left = 0;
        pVoice->volume.right = 0;
        pVoice->pitch = 0;
        pVoice->adsr[0] = (pVoice->adsr[0] & 0xFF) + 0x7F00;
        pVoice->adsr[1] = 0x1FDF;
    }
}

void func_80037F44(void) {
    if (!(g_SoundControlFlags & 1)) {
        g_SoundControlFlags |= 1;
        EnableEvent(g_unk_SoundEvent);
    }
}

void func_80037F88(void) {
    if (g_SoundControlFlags & 1) {
        DisableEvent(g_unk_SoundEvent);
        g_SoundControlFlags &= ~1;
    }
}



INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", SoundLoadWdsFile);

// Loads part of a WDS file, basically a sized SoundLoadWdsFile?
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_800380D0);

void SoundSpuMemoryAllocateWDS(SoundWDSEntry* pWdsFile, int mode) {
    if (mode == SOUND_WDS_ALLOCATE_AT_ADDRESS) {
        mode = pWdsFile->spuMemoryAddress;
    } else if (mode == SOUND_WDS_ALLOCATE_AUTOMATIC) {
        mode = 0;
    }
    
    if (mode == 0) {
        SoundSpuMemoryAllocateBlock(pWdsFile->adpcmDataSize, pWdsFile->unk1E);
        return;
    }
    
    SoundSpuMemoryAllocateBlockAtAddress(pWdsFile->adpcmDataSize, pWdsFile->spuMemoryAddress, pWdsFile->unk1E);
}

void SoundWdsSetTransferParamters(int transferAddress, int numBytesToTransfer) {
    g_SoundWdsCurSpuAddress = transferAddress;
    g_SoundWdsRemainingBytes = numBytesToTransfer;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", SoundTransferWdsPart);

void SoundFreeWdsEntry(SoundWDSEntry* pTargetEntry) {
    SoundWDSEntry* pPrev;
    SoundWDSEntry* pCurrent;

    pPrev = NULL;
    for (pCurrent = g_SoundWdsLinkedList; pCurrent != NULL; pCurrent = pCurrent->pNext) {
        if (pCurrent == pTargetEntry) 
            break;
        
        pPrev = pCurrent;
    }

    if (pCurrent == NULL) {
        SoundHandleError(0x11U);
        return;
    }

    DisableEvent(g_unk_SoundEvent);
    if (pPrev != NULL)
        pPrev->pNext = pTargetEntry->pNext;
    else 
        g_SoundWdsLinkedList = pTargetEntry->pNext;
    EnableEvent(g_unk_SoundEvent);
    
    if (pTargetEntry->spuMemoryAddress != SoundSpuMemoryFreeBlock(pTargetEntry->spuMemoryAddress)) {
        SoundHandleError(0x24U);
    }
    
    SoundHeapFree(pTargetEntry);
}

SoundWDSEntry* SoundFindWdsEntry(int targetID) {
    SoundWDSEntry* pCurrent;

    for (pCurrent = g_SoundWdsLinkedList; pCurrent != NULL; pCurrent = pCurrent->pNext) {
        if (pCurrent->id == targetID) {
            return pCurrent;
        }
    }
    return pCurrent;
}


void SoundAddSedsEntry(SoundFile* pSoundFile) {
    SoundFile* pEntry;
    short nSedsStatus;
    SoundFile** pList;
    SoundFile* pSoundFileToVerify = pSoundFile;

    // Ensure that an entry with the same SED ID does not exists in the linked list already
    if (!(g_SoundControlFlags & 0x80)) {
        for (pEntry = g_SoundSedsLinkedList; pEntry != NULL; pEntry = pEntry->pNext) {
            if (pSoundFile->sedId == pEntry->sedId) {
                SoundHandleError(SOUND_ERR_ENTRY_ALREADY_EXISTS);
                return;
            }
        }
    }

    // Validate SEDS File
    nSedsStatus = SoundValidateFile(pSoundFileToVerify, FILE_SIGNATURE('s','e','d','s'), 0x101);
    if (nSedsStatus != SOUND_STATUS_OK) {
        SoundHandleError(nSedsStatus);
        return;
    }

    // Add the SED Entry to the linked list
    DisableEvent(g_unk_SoundEvent);
    pList = &g_SoundSedsLinkedList;
    while (*pList != NULL)
        pList = &((*pList)->pNext);
    *pList = pSoundFile;
    pSoundFile->pNext = NULL;
    EnableEvent(g_unk_SoundEvent);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003852C);

//----------------------------------------------------------------------------------------------------------------------
void func_80038624(void) {
    func_80039FF8();
    g_SoundSedsLinkedList = 0;
}

//----------------------------------------------------------------------------------------------------------------------
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003864C);
/*
Attempt (99.50%, same size): only difference is that gcc keeps the raw sedId in v0
where the original used a1.
SoundFile* func_8003864C(SoundFile* pSoundFile) {
    SoundFile* pEntry;
    u16 sedId;

    if (pSoundFile != NULL) {
        sedId = pSoundFile->sedId;
    }
    pEntry = g_SoundSedsLinkedList;

    while (pEntry != NULL && (s16)sedId != pEntry->sedId) {
        pEntry = pEntry->pNext;
    }

    return pEntry;
}
*/

//----------------------------------------------------------------------------------------------------------------------
void func_8003869C(void) {
    func_80039CC4();
    func_80039FF8();
}

//----------------------------------------------------------------------------------------------------------------------
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_800386C4);

//----------------------------------------------------------------------------------------------------------------------
s32 func_80038824(void) {
    s32 out;

    if (g_SoundControlFlags & ((1 << 8) | (1 << 9) | (1 << 10))) {
        out = 1;
        if (g_SoundControlFlags & ((1 << 9) | (1 << 10))) {
            out = 2;
        }
    } else {
        out = 0;
    }

    return out;
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSetupCdMix(s32 vol) {
    s32 pan;

    g_SoundVolumeController.unk_field2 = vol;

    if (g_SoundControlFlags & 0x700) {
        pan = vol;
        vol = 0;
    } else {
        vol >>= 1;
        pan = vol;
    }
    g_SoundCdRomAttenuation.val2 = pan;
    g_SoundCdRomAttenuation.val0 = pan;
    // chained: reproduces the original's store order
    g_SoundCdRomAttenuation.val1 = g_SoundCdRomAttenuation.val3 = vol;

    CdMix(&g_SoundCdRomAttenuation);
}

//----------------------------------------------------------------------------------------------------------------------
void func_800388D4(s32 arg0) {

    if (arg0 != 0) {
        g_SoundControlFlags |= (1 << 12);
    } else {
        g_SoundControlFlags &= ~(1 << 12);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void func_8003890C(AudioManager* manager, s32 bIn) {
    if (bIn != 0) {
        manager->unk_Flags &= ~(1 << 0);
    } else {
        manager->unk_Flags |= (1 << 0);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSetReverbModeWithAllocation(s32 reverbType, s32 reverbDepth, s32 reverbDelay, s32 reverbFeedback) {
    SpuReverbAttr reverbAttr;
    s32 currentReverbType;
    s32 workAreaSize;
    s32 workAreaStartAddr;
    s32 allocationSize;
    s32 memoryHandle;
    b32 bAllocated;

    bAllocated = false;

    if (reverbType == -2) {
        return;
    }

    if (reverbType == SPU_REV_MODE_OFF) {
        reverbFeedback = 0;
        reverbDelay = 0;
        reverbDepth = 0;
    } else if (reverbType == SPU_REV_MODE_CHECK) {
        reverbType = g_SoundReverbType;
    }

    SpuGetReverbModeType(&currentReverbType);
    if (currentReverbType != reverbType || reverbType == SPU_REV_MODE_OFF) {

        if (g_SoundReverbMemoryHandle != -1) {
            SoundSpuMemoryFreeBlock(g_SoundReverbMemoryHandle);
        }

        workAreaSize = g_ReverbWorkAreaSizes[reverbType];
        workAreaStartAddr = workAreaSize;
        allocationSize = 0x80000 - workAreaSize;

        memoryHandle = SoundSpuMemoryAllocateBlockAtAddress(workAreaStartAddr, allocationSize, 5);
        g_SoundReverbMemoryHandle = memoryHandle;

        if (memoryHandle == 0) {
            SoundHandleError(0x20);
            reverbType = 0;
            reverbFeedback = 0;
            reverbDelay = 0;
            reverbDepth = 0;
        }
        bAllocated = true;

    }

    g_SoundReverbType = reverbType;
    g_SoundVolumeController.currentReverbDepth = reverbDepth;
    g_SoundReverbDelay = reverbDelay;
    g_SoundReverbFeedback = reverbFeedback;

    SoundApplyVolumeSettings();

    if (bAllocated) {
        // New reverb type - initialize with zero depth, set type, clear work area (probably)
        SpuSetReverbModeDepth(0, 0);
        SpuSetReverbModeType(reverbType);
        SoundInitiateReverbWorkAreaTransfer(allocationSize, workAreaStartAddr);
    } else {
        // Existing reverb type - apply current settings
        SpuSetReverbModeDepth(g_SoundReverbDepth.left, g_SoundReverbDepth.right);
        SpuSetReverbModeDelayTime(reverbDelay);
        SpuSetReverbModeFeedback(reverbFeedback);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SoundInitiateReverbWorkAreaTransfer(s32 addr, s32 size) {
    s32 temp_v0;

    g_SoundUploadSourceAddr = addr;
    g_SoundUploadBytesRemaining = size;
    if (g_SoundUploadDestBuffer == 0) {
        temp_v0 = SoundHeapAllocate(0x840);
        g_SoundUploadDestBuffer = temp_v0;
        if (temp_v0 == 0) {
            SoundHandleError(0x1E);
        }
    }
    g_SoundControlFlags |= (1 << 5);
    SoundExecuteReverbWorkAreaTransfer();
}

//----------------------------------------------------------------------------------------------------------------------
void SoundExecuteReverbWorkAreaTransfer(void) {
    s32 bytesRemaining;
    s32 chunkSize;
    s32 currentSourceAddr;

    bytesRemaining = g_SoundUploadBytesRemaining;

    if (bytesRemaining == 0) {
        SoundHeapFree(g_SoundUploadDestBuffer);
        g_SoundUploadDestBuffer = 0;

        SpuSetReverbModeDepth(g_SoundReverbDepth.left, g_SoundReverbDepth.right);
        SpuSetReverbModeDelayTime(g_SoundReverbDelay);
        SpuSetReverbModeFeedback(g_SoundReverbFeedback);

        g_SoundControlFlags &= ~(1 << 5);

    } else {
        chunkSize = (bytesRemaining <= 0x840) ? bytesRemaining : 0x800;

        currentSourceAddr = g_SoundUploadSourceAddr;

        g_SoundUploadBytesRemaining = bytesRemaining - chunkSize;
        g_SoundUploadSourceAddr = currentSourceAddr + chunkSize;

        SoundQueueSpuWriteCommand(currentSourceAddr, g_SoundUploadDestBuffer, chunkSize, SoundExecuteReverbWorkAreaTransfer);

        if ((g_SoundControlFlags & (1 << 4)) == 0) {
            SoundQueueSpuWriteCommand(currentSourceAddr, g_SoundUploadDestBuffer, chunkSize, NULL);
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSetMasterVolumeWithFade(s32 volume, s32 frames)
{
    g_SoundVolumeController.masterInterpolator.targetValue = (s16)volume;
    if (frames == 0) {
        g_SoundVolumeController.masterInterpolator.currentValue = volume << 0x10;
        g_SoundVolumeController.masterInterpolator.counter = 0;
        g_SoundVolumeController.currentMasterVolume = g_SoundVolumeController.masterInterpolator.targetValue;
        SoundSetVolumeWithPhase(g_SoundVolumeController.masterInterpolator.targetValue, &g_SoundVolumeController.commonAttr.mvol, 0);
        g_SoundVolumeController.commonAttr.mask |= SPU_COMMON_MVOLL | SPU_COMMON_MVOLR;
    } else {
        s32 volumeDifference = (volume << 8) - (g_SoundVolumeController.masterInterpolator.currentValue >> 8);
        if (volumeDifference != 0) {
            g_SoundVolumeController.masterInterpolator.stepIncrement = volumeDifference / frames << 8;
            g_SoundVolumeController.masterInterpolator.counter = (s16)frames;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSetCdVolumeWithFade(s32 targetVolume, s32 fadeFrames) {
    s16 volumeValue;

    g_SoundVolumeController.cdInterpolator.targetValue = targetVolume;

    if (fadeFrames == 0) {
        g_SoundVolumeController.cdInterpolator.currentValue = targetVolume << 16;
        volumeValue = targetVolume;
        g_SoundVolumeController.cdInterpolator.counter = 0;
        g_SoundVolumeController.currentCdVolume = volumeValue;
        g_SoundVolumeController.commonAttr.cd.volume.right = volumeValue;
        g_SoundVolumeController.commonAttr.cd.volume.left = volumeValue;
        g_SoundVolumeController.commonAttr.mask |= SPU_COMMON_CDVOLL | SPU_COMMON_CDVOLR;

    } else {
        s32 currentVolume = g_SoundVolumeController.cdInterpolator.currentValue >> 8;
        s32 volumeDifference = (targetVolume << 8) - currentVolume;

        if (volumeDifference != 0) {
            s32 stepPerFrame = volumeDifference / fadeFrames;
            g_SoundVolumeController.cdInterpolator.counter = fadeFrames;
            g_SoundVolumeController.cdInterpolator.stepIncrement = stepPerFrame << 8;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSetCdAttr(long reverb, long mix) {
    g_SoundVolumeController.commonAttr.cd.reverb = reverb;
    g_SoundVolumeController.commonAttr.cd.mix = mix;
    g_SoundVolumeController.commonAttr.mask = g_SoundVolumeController.commonAttr.mask | SPU_COMMON_CDREV | SPU_COMMON_CDMIX;
    SpuSetCommonAttr(&g_SoundVolumeController.commonAttr);
}

//----------------------------------------------------------------------------------------------------------------------
void SoundApplyVolumeSettings(void) {
    SoundSetVolumeWithPhase(g_SoundVolumeController.currentMasterVolume, &g_SoundVolumeController.commonAttr.mvol, 0);
    g_SoundVolumeController.commonAttr.cd.volume.right = g_SoundVolumeController.currentCdVolume;
    g_SoundVolumeController.commonAttr.cd.volume.left = g_SoundVolumeController.currentCdVolume;;
    SoundSetVolumeWithPhase(g_SoundVolumeController.currentReverbDepth, &g_SoundReverbDepth, 1);
    g_SoundVolumeController.commonAttr.mask |= SPU_COMMON_MVOLL | SPU_COMMON_MVOLR | SPU_COMMON_CDVOLL | SPU_COMMON_CDVOLR;
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSetVolumeWithPhase(s32 volume, SpuVolume* pVolume, s32 channelSelect)
{
    pVolume->right = volume;
    pVolume->left = volume;

    if ((g_SoundControlFlags & ((1 << 9) | (1 << 10))) != 0) {
        channelSelect &= 0xFF;
        if (!(g_SoundControlFlags & (1 << 9))) {
            if ((channelSelect ^ 1) != 0) {
                pVolume->left = -volume;
            } else {
                pVolume->right = -volume;
            }
        } else {
            if (channelSelect != CHANNEL_RIGHT) {
                pVolume->left = -volume;
            } else {
                pVolume->right = -volume;
            }
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SoundHeapInitialize(void* startAddress, unsigned int size) {
    SoundHeapBlockHeader* pHeapBlock;
    unsigned int nAlignedSize;

    // Align start address and size
    pHeapBlock = (SoundHeapBlockHeader*) startAddress;
    nAlignedSize = size & ~0xF;
    if ((u32)pHeapBlock & 0xF) {
        nAlignedSize -= 0x10;
        pHeapBlock = ((u32)pHeapBlock + 0xF) & ~0xF;
    }
    
    g_SoundHeapEnd = (u32)pHeapBlock + nAlignedSize;
    pHeapBlock->unk0 = 0x8000;
    g_SoundHeapHead = pHeapBlock;
    g_SoundHeapSize = nAlignedSize;
    pHeapBlock->unk2 = 0;
    pHeapBlock->unk4 = 0;
    pHeapBlock->pPrev = pHeapBlock + 1;
    pHeapBlock->pNext = NULL;
}

void* SoundHeapAllocate(unsigned int allocSize) {
    void* pMemory;
    SoundHeapBlockHeader* pNewBlock;
    unsigned int nTotalSize;
    unsigned int nSize;
    SoundHeapBlockHeader* pNext;
    SoundHeapBlockHeader* pHeapBlock;

    DisableEvent(g_unk_SoundEvent);
    nTotalSize = ((allocSize + 0xF) & ~0xF) + sizeof(SoundHeapBlockHeader);
    
    for (pHeapBlock = g_SoundHeapHead; pHeapBlock->pNext != NULL; pHeapBlock = pHeapBlock->pNext) {
        pNext = pHeapBlock->pNext;
        nSize = (u32)pHeapBlock->pNext - (u32)pHeapBlock->pPrev;
        if (nSize >= nTotalSize) {
            goto alloc_new_block;
        }
    }
    
    pNext = (SoundHeapBlockHeader*)g_SoundHeapEnd;
    if ((u32)pNext - (u32)pHeapBlock->pPrev >= nTotalSize) {
    alloc_new_block:
        pNewBlock = (SoundHeapBlockHeader*)(((u32)pHeapBlock->pPrev + 0xF) & ~0xF);
        pMemory = pNewBlock + 1;
        pNewBlock->pPrev = (SoundHeapBlockHeader*)((u32)pMemory + allocSize);
        pNewBlock->pNext = NULL;
        pNewBlock->unk4 = 0;
        pNewBlock->unk0 = 2;
        pNewBlock->unk2 = 0;
        pNewBlock->pNext = pHeapBlock->pNext;
        pHeapBlock->pNext = pNewBlock;
        EnableEvent(g_unk_SoundEvent);
        SoundHeapClearBlockMemory(pMemory, allocSize);
        return pMemory;
    }
    
    return NULL;
}

// SoundHeapAllocate, but slightly different
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_80039024);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", SoundHeapFree);
/*
Attempt (96.62%, same size): every instruction is right and pBlock=s1 / pCurrent=s0
match; the original copies the loop variable to v0 (`move v0,s0`) and iterates on v0,
gcc keeps iterating on s0. Adding a separate loop variable coalesces back to s0.
void SoundHeapFree(void* pMemory) {
    SoundHeapBlockHeader* pBlock;
    SoundHeapBlockHeader* pCurrent = g_SoundHeapHead;
    SoundHeapBlockHeader* pPrev;
    SoundHeapBlockHeader* pEntry;

    DisableEvent(g_unk_SoundEvent);
    pBlock = (SoundHeapBlockHeader*)pMemory - 1;
    pPrev = NULL;
    for (pEntry = pCurrent; pEntry != pBlock; pEntry = pEntry->pNext) {
        pPrev = pEntry;
    }
    if (pPrev != NULL) {
        pPrev->pNext = pBlock->pNext;
    }
    EnableEvent(g_unk_SoundEvent);
}
*/

u32 func_800391CC(void) {
    SoundHeapBlockHeader* pHeapBlock;
    s32 nMaxSize;
    s32 nSize;

    pHeapBlock = g_SoundHeapHead;
    nMaxSize = 0;
    if (pHeapBlock->pNext != NULL) {
        do {
            nSize = (u32)pHeapBlock->pNext - (u32)pHeapBlock->pPrev;
            if (nMaxSize < nSize) {
                nMaxSize = nSize;
            }
            pHeapBlock = pHeapBlock->pNext;
        } while (pHeapBlock->pNext != NULL);
    }

    nSize = g_SoundHeapEnd - (u32)pHeapBlock->pPrev;
    if (nMaxSize < nSize) {
        nMaxSize = nSize;
    }

    return nMaxSize & ~0xF;
}

void SoundHeapSetBlockMemory(void* pBlockMemory, void* pSrc, int size) {
    u32* pCurSrc;
    u32* pCurDst;
    u32* pNextSrc;
    u32* pNextDst;
    u32 v0,v1,v2,v3;

    unsigned int nCount;
    pCurDst = pBlockMemory;
    pCurSrc = pSrc;
    
    for (nCount = size >> 4; nCount != 0; nCount--) {
        v0 = pCurSrc[0];
        v1 = pCurSrc[1];
        v2 = pCurSrc[2];
        v3 = pCurSrc[3];
        
        pCurDst[0] = v0;
        pCurDst[1] = v1;
        pCurDst[2] = v2;
        pCurDst[3] = v3;

        pCurSrc += 4;
        pCurDst += 4;
    }

    for (nCount = (size >> 2) & 3; nCount != 0; nCount--) {
        *pCurDst++ = *pCurSrc++;
    }

    for (nCount = size & 3; nCount != 0; nCount--) {
        *((u8*)pCurDst)++ = *((u8*)pCurSrc)++;
    }
}

void SoundHeapClearBlockMemory(void* pMemory, int size) {
    unsigned int nCount;
    u32* pDword;
    u8* pByte;

    pDword = pMemory;

    for (nCount = size >> 4; nCount != 0; nCount--) {
        pDword[3] = 0;
        pDword[2] = 0;
        pDword[1] = 0;
        pDword[0] = 0;
        pDword += 4;
    }
    
    for (nCount = (size >> 2) & 3; nCount != 0; nCount--) {
        *pDword++ = 0;
    }
    
    pByte = (u8*) pDword;
    for (nCount = size & 3; nCount != 0; nCount--) {
        *pByte++ = 0;
    }
}

void SoundSpuMemoryInitialize(void) {
    int i;
    
    for (i = MAX_SPU_MEMORY_BLOCKS - 1; i >= 0; i--) {
        g_SoundSpuMemoryBlocks[i].flags = SPU_MEMORY_FREE;
    }
    
    g_SoundSpuMemoryBlocks[0].flags = SPU_MEMORY_RESERVED | SPU_MEMORY_IN_USE;
    g_SoundSpuMemoryBlocks[0].unk1 = 5;
    g_SoundSpuMemoryBlocks[0].spuAddress = 0;
    g_SoundSpuMemoryBlocks[0].size = 0x1010;
    g_SoundSpuMemoryBlocks[0].nextBlockIndex = 0;
}


INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", SoundSpuMemoryAllocateBlock);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_800394B8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", SoundSpuMemoryAllocateBlockAtAddress);

int SoundSpuMemoryFreeBlock(int targetAddress) {
    SoundSpuMemoryBlock* pCurBlock;
    SoundSpuMemoryBlock* pPrevBlock;
    short nNextIndex;

    pCurBlock = g_SoundSpuMemoryBlocks;
    pPrevBlock = NULL;

    while (1) {
        if (pCurBlock->spuAddress == targetAddress) {
            pPrevBlock->nextBlockIndex = pCurBlock->nextBlockIndex;
            pCurBlock->flags = SPU_MEMORY_FREE;
            pCurBlock->unk1 = 0;
            pCurBlock->spuAddress = 0x0;
            pCurBlock->nextBlockIndex = 0;
            return targetAddress;
        }
        
        nNextIndex = pCurBlock->nextBlockIndex;
        pPrevBlock = pCurBlock;
        
        if (nNextIndex != 0) {
            pCurBlock = &g_SoundSpuMemoryBlocks[nNextIndex];
            continue;
        }
        
        return NULL;
    }
}

void func_80039748(int targetAddress, u_char value) {
    SoundSpuMemoryBlock *pBlock;

    pBlock = SoundSpuMemoryFindBlock(targetAddress);
    if (pBlock != NULL) {
        pBlock->unk1 = value;
    }
}

s32 func_8003977C(void) { return 0; }

int SoundSpuMemoryGetFreeBlock() {
    int i = 0;

    while (i < MAX_SPU_MEMORY_BLOCKS) {
        if (g_SoundSpuMemoryBlocks[i].flags == SPU_MEMORY_FREE)
            return i;
        i++;
    }

    return 0;
}

// Possibly misleading name
SoundSpuMemoryBlock* SoundSpuMemoryFindBlock(int targetAddress) {
    SoundSpuMemoryBlock* pCurrent;
    SoundSpuMemoryBlock* pRes;
    
    pCurrent = g_SoundSpuMemoryBlocks;
    pRes = g_SoundSpuMemoryBlocks;
    
    while (1) {
        if (pCurrent->spuAddress != targetAddress) {
            if (pCurrent->nextBlockIndex != 0) {
                return NULL;
            }
            pCurrent = pRes;
            continue;
        }
        return pCurrent;
    }
}

AudioManager* func_800397FC(SoundFile* pFile, s32 arg1, s32 arg2) {
    AudioManager* pManager = func_80039850(pFile);

    func_80039A80(pManager, arg1, arg2);
    return pManager;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_80039850);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_80039910);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_800399D4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_80039A80);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_80039B68);

//----------------------------------------------------------------------------------------------------------------------
void func_80039C4C(AudioManager* manager) {
    if (manager == NULL) {
        SoundHandleError(5);
        return;
    }
    manager->unk_Flags &= ~(1 << 15);
    SoundReleaseAllVoices();
}

//----------------------------------------------------------------------------------------------------------------------
void func_80039C8C(AudioManager* manager, s32 arg1) {
    if (manager == NULL) {
        SoundHandleError(5);
        return;
    }
    func_8003A89C(manager, 0, arg1);
}

//----------------------------------------------------------------------------------------------------------------------
void func_80039CC4(void) {
    AudioManager* pManager;

    pManager = g_SoundAudioManagerListHead;
    if (pManager != NULL) {
        do {
            if (pManager->unk_Flags & 1) {
                pManager->unk_Flags &= ~(1 << 15);
                SoundReleaseAllVoices(pManager);
            }
            pManager = pManager->next;
        } while (pManager != NULL);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void func_80039D24(void) {}

//----------------------------------------------------------------------------------------------------------------------
void func_80039D2C(s32 bIn) {
    if (bIn != 0) {
        g_SoundControlFlags |= (1 << 11);
    } else {
        func_80039FF8();
        g_SoundControlFlags &= ~(1 << 11);
    }
}

s32 func_80039D78(s32 count) {
    if (count) {
        if (count > 0x10) {
            count = 0x10;
        }
        if (count < 4) {
            count = 4;
        }
        D_80059544 = count & 0xFE;
    }

    return D_80059544;
}

void func_80039DB8(s32 arg0) {
    if (g_SoundControlFlags & 0x800) {
        s32 value = D_80059478;

        value -= 2;
        D_80059404 = 2;
        func_8003B644((s16)(value | (s16)0x8000), arg0, 0x6000, 0x4000);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Wowoweewa flag central
void func_80039E18(s32 arg0) {
    if (g_SoundControlFlags & (1 << 11)) {
        D_80059404 = 2;
        func_8003B644((1 << 2) | (1 << 3) | (1 << 13) | (1 << 14), arg0, (1 << 13) | ( 1 << 14), (1 << 14));
    }
}

//----------------------------------------------------------------------------------------------------------------------
void func_80039E60(s32 arg0) {
    if (g_SoundControlFlags & 0x800) {
        s32 value = func_8003A65C(arg0, 2);

        D_80059404 = 2;
        func_8003B644((s16)(value | 0x2000), arg0, 0x6000, 0x4000);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void func_80039EC4(s32 arg0, s32 arg1) {
    if (g_SoundControlFlags & (1 << 11)) {
        D_80059404 = 2;
        func_8003B644(
            ((arg1 & 0xFE) ^ (1 << 3)) | (1 << 13), // wtf is this... we really need to figure out some of these macros
            arg0,
            (1 << 13) | (1 << 14),
            (1 << 14)
        );
    }
}

//----------------------------------------------------------------------------------------------------------------------
void func_80039F18(s32 arg0, s32 arg1, s32 arg2) {
    if (g_SoundControlFlags & 0x800) {
        s32 value = func_8003A65C(arg0, 2);

        D_80059404 = 2;
        func_8003B644((s16)(value | 0x2000), arg0, (s16)(arg1 << 8), (s16)(arg2 << 8));
    }
}

void func_80039F9C(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    if (g_SoundControlFlags & 0x800) {
        D_80059404 = 2;
        func_8003B644(((arg1 & 0xFE) ^ 0x8) | 0x2000, arg0, (s16)(arg2 << 8), (s16)(arg3 << 8));
    }
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_80039FF8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003A094);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003A14C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003A20C);

void func_8003A2D4(void) {}

void func_8003A2DC(void) {}

void func_8003A2E4(s32 id, s32 value) {
    AudioManager* pManager = D_800595D8;
    AudioElement* pElement = &pManager->elements[0];
    u32 cnt = D_80059478;

    do {
        if (pElement->active_flag & 0x1) {
            if (pElement->unk_0x8 == id) {
                pElement->unk_0x76 = value << 8;
                pElement->status_flags = 0x100;
            }
        }
        pElement++;
        cnt--;
    } while (cnt);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003A344);
/*
Attempt (79.31%, correct size and registers): the only difference is that gcc
schedules the `andi a0,0xfe` / `xori a0,0x8` pair after the loop-preheader
constants (`li a2,2`, `sll a1,a1,8`, `li a3,0x100`) while the original emits
them first. Everything else - including the second pointer to status_flags -
matches. Stable against declaration order and against a statement-per-line form.
void func_8003A344(s32 elementIndex, s32 value) {
    s32 index = (elementIndex & 0xFE) ^ 8;
    AudioElement* pElement = &D_800595D8->elements[index];
    s32 volume = value << 8;
    u32 cnt = 2;

    do {
        if (pElement->active_flag & 0x1) {
            pElement->unk_0x76 = volume;
            pElement->status_flags = 0x100;
        }
        pElement++;
        cnt--;
    } while (cnt);
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003A3B8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003A450);

void func_8003A4FC(s32 id, s32 value) {
    AudioManager* pManager = D_800595D8;
    AudioElement* pElement = &pManager->elements[0];
    u32 cnt = D_80059478;

    do {
        if (pElement->active_flag & 0x1) {
            if (pElement->unk_0x8 == id) {
                pElement->unk_0x74 = value << 8;
                pElement->status_flags = 0x100;
            }
        }
        pElement++;
        cnt--;
    } while (cnt);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003A55C);
/*
Attempt (79.31%, correct size and registers): the only difference is that gcc
schedules the `andi a0,0xfe` / `xori a0,0x8` pair after the loop-preheader
constants (`li a2,2`, `sll a1,a1,8`, `li a3,0x100`) while the original emits
them first. Everything else - including the second pointer to status_flags -
matches. Stable against declaration order and against a statement-per-line form.
void func_8003A55C(s32 elementIndex, s32 value) {
    s32 index = (elementIndex & 0xFE) ^ 8;
    AudioElement* pElement = &D_800595D8->elements[index];
    s32 volume = value << 8;
    u32 cnt = 2;

    do {
        if (pElement->active_flag & 0x1) {
            pElement->unk_0x74 = volume;
            pElement->status_flags = 0x100;
        }
        pElement++;
        cnt--;
    } while (cnt);
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003A5D0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003A65C);

s32 func_8003A82C(AudioManager* pManager) {
    return (u16)pManager->unk_Flags >> 15;
}

void func_8003A838(AudioManager* pManager, s32 volume, s32 frames) {
    s32 delta;

    if (volume == 0) {
        volume = 0x100;
    }

    pManager->unk_Interpolator_0x64.targetValue = volume;

    if (frames == 0) {
        s32 product = (s16)(pManager->unk_0x58 >> 16) * volume;

        pManager->unk_Interpolator_0x64.counter = 0;
        pManager->unk_Interpolator_0x64.currentValue = volume << 16;
        pManager->unk_0x54 = product;
    } else {
        delta = (volume << 16) - pManager->unk_Interpolator_0x64.currentValue;
        if (delta != 0) {
            pManager->unk_Interpolator_0x64.counter = frames;
            pManager->unk_Interpolator_0x64.stepIncrement = delta / frames;
        }
    }
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003A89C);

void func_8003A948(AudioManager* pManager, s16 volume, s32 frames) {
    s32 delta;

    pManager->unk_Interpolator_0x7c.targetValue = volume << 8;

    if (frames == 0) {
        pManager->unk_Interpolator_0x7c.currentValue = volume << 24;
        pManager->unk_Interpolator_0x7c.counter = 0;
        unk_SoundSetFlagsOnActiveVoices(0x200, pManager);
    } else {
        delta = (volume << 16) - (pManager->unk_Interpolator_0x7c.currentValue >> 8);
        if (delta != 0) {
            pManager->unk_Interpolator_0x7c.counter = frames;
            pManager->unk_Interpolator_0x7c.stepIncrement = (delta / frames) << 8;
        }
    }
}

void func_8003A9BC(AudioManager* pManager, s16 volume, s32 frames) {
    s32 delta;

    pManager->unk_Interpolator_0x88.targetValue = volume << 8;

    if (frames == 0) {
        pManager->unk_Interpolator_0x88.currentValue = volume << 24;
        pManager->unk_Interpolator_0x88.counter = 0;
        unk_SoundSetFlagsOnActiveVoices(0x100, pManager);
    } else {
        delta = (volume << 16) - (pManager->unk_Interpolator_0x88.currentValue >> 8);
        if (delta != 0) {
            pManager->unk_Interpolator_0x88.counter = frames;
            pManager->unk_Interpolator_0x88.stepIncrement = (delta / frames) << 8;
        }
    }
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003AA30);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003AAC4);

void func_8003ABE8(AudioManager* pManager, u8 value) {
    pManager->unk_0x1b = value;
}

void func_8003ABF0(AudioManager* pManager, u16* pOut) {
    u32 total = (u32)pManager->unk_0x28 >> 8;
    u32 field = total / 240;
    u32 index = field / 60;

    *(u32*)pOut = pManager->unk_0x24;
    pOut[2] = total - field * 240;
    pOut[3] = field - index * 60;
    pOut[4] = index;
}

u16* func_8003AC58(AudioManager* pManager) {
    AudioElement* pElement = pManager->elements;
    u16 count = pManager->elementCount;
    u16* pResult = (u16*)&pManager->unk_0x30;
    u16 best = 0xFFFF;

    do {
        if (pElement->active_flag != 0 && pElement->unk_0x20 < best) {
            best = pElement->unk_0x20;
        }
        pElement++;
    } while (--count);

    if (best == 0xFFFF) {
        best = 0;
    }
    *pResult = best;
    return pResult;
}

s32 func_8003ACC8(AudioManager* pManager) {
    AudioManager* pCurrent = g_SoundAudioManagerListHead;
    s32 result;

    if (pManager != NULL) {
        while (pCurrent != NULL && pCurrent != pManager) {
            pCurrent = pCurrent->next;
        }
    }

    if (pCurrent != NULL) {
        u8* pBase = (u8*)pCurrent->unk_0x8;

        result = *(u16*)(pBase + 0x1E) + (s32)pBase;
    } else {
        result = 0;
    }

    return result;
}

void func_8003AD20(AudioManager* pManager, s32 mode) {
    switch (mode) {
    case 0:
        func_8003AD98(pManager);
        break;
    case 1:
        func_8003ADCC(pManager);
        break;
    case 2:
        func_8003AE84(pManager);
        break;
    }
}

void func_8003AD98(AudioManager* pManager) {
    if (pManager->unk_Flags & 0x10) {
        pManager->unk_Flags &= ~0x10;
        func_8003B930(pManager);
    }
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003ADCC);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003AE84);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003AF24);
/*
Attempt (87.10%, same size): every instruction is right; only the first eight are
ordered differently. The original schedules `lui/lw a0` (DisableEvent's argument)
before `sw ra`/`lhu`/`sh a1,30(pManager)`, gcc puts them after. Statement order of the
two field updates does not change it.
void func_8003AF24(AudioManager* pManager, u16 volume) {
    s32 newVolume = 0x7F00;

    pManager->unk_Flags |= 0x20;
    *(u16*)pManager->unk_0x1e = volume;
    DisableEvent(g_unk_SoundEvent);
    SoundReleaseAllVoices(pManager);
    EnableEvent(g_unk_SoundEvent);
    pManager->unk_Interpolator_0x64.targetValue = newVolume;
    pManager->unk_Interpolator_0x64.counter = 0;
    pManager->unk_Interpolator_0x64.currentValue = newVolume << 16;
    pManager->unk_0x54 = (pManager->unk_0x58 >> 16) * newVolume;
}
*/

//----------------------------------------------------------------------------------------------------------------------
void func_8003AFA0(AudioManager* manager) {
    AudioElement* pElement;
    u32 cnt;

    cnt = manager->elementCount;
    pElement = &manager->elements[0];

    do {
        // What's going on here? Are the first two fields just a u32?
        if ((*(u32*)pElement & 0x101) == 0x101) {
            // I'm beginning to think that this isn't just a flag for activity considering this mask
            if ((pElement->active_flag & 0x30) == 0) {
                pElement->status_flags |= 0x1;
            }
        }
        pElement++;
        cnt--;
    } while (cnt);
}

//----------------------------------------------------------------------------------------------------------------------
void SoundAbortAllVoices(AudioManager* manager) {
    AudioElement* pElement;
    u32 cnt;

    cnt = manager->elementCount;
    pElement = &manager->elements[0];

    do {

        if (pElement->active_flag) {
            SoundAbortVoiceOnChannel(&pElement->voice_data, pElement->voice_number);
        }
        pElement++;
        cnt--;
    } while (cnt);
}

//----------------------------------------------------------------------------------------------------------------------
void SoundReleaseAllVoices(AudioManager* manager) {
    AudioElement* pElement;
    u32 cnt;

    cnt = manager->elementCount;
    pElement = &manager->elements[0];

    do {
        SoundReleaseVoiceFromChannel(&pElement->voice_data, pElement->voice_number);
        pElement++;
        cnt--;
    } while (cnt);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003B0AC);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003B148);

void func_8003B1FC(AudioManager* pManager) {
    SoundRemoveAudioManagerFromList(pManager);
    SoundHeapFree(pManager);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003B22C);

void func_8003B32C(AudioManager* pManager) {
    pManager->unk_Flags = 2;
    pManager->unk_0x12 = 0x7FFF;
    pManager->unk_0x16 = 0;
    pManager->unk_0x18 = 0x7F;
    pManager->elementCount = D_80059478;
    SoundInitializeAudioManager(pManager);
}

//----------------------------------------------------------------------------------------------------------------------
// TODO(jperos): Boy, we really need some names of this stuff
void SoundInitializeAudioManager(AudioManager* manager) {
    func_8003B930(manager);

    manager->unk_0x32 = 1;
    manager->unk_0x1a = 0;
    manager->unk_0x1b = 0;
    manager->unk_0x30 = 0;
    manager->unk_0x34 = 0;
    manager->unk_0x38 = 4;
    manager->unk_0x36 = 1;
    manager->unk_0x3a = 0x30;
    manager->unk_0x3c = 4;
    manager->unk_0x3e = 4;

    manager->unk_Interpolator_0x64.currentValue = 0x01000000;
    manager->unk_Interpolator_0x70.currentValue = 0x7F000000;

    manager->unk_0x58 = 0x00660000;
    manager->unk_0x54 = 0x6600;
    manager->unk_0x28 = 0;
    manager->unk_0x24 = 0;
    manager->unk_0x20 = 0;
    manager->unk_0x48 = 0;
    manager->unk_Interpolator_0x7c.currentValue = 0;
    manager->unk_Interpolator_0x88.currentValue = 0;
    manager->unk_Interpolator_0x64.counter = 0;
    manager->unk_Interpolator_0x70.counter = 0;
    manager->unk_Interpolator_0x7c.counter = 0;
    manager->unk_Interpolator_0x88.counter = 0;
    manager->unk_0x5c = 0;
    manager->unk_0x60 = 0;
    manager->unk_0x50 = 0x00010000;
}

//----------------------------------------------------------------------------------------------------------------------
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003B424);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003B644);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003B930);
/*
Attempt (79.63%, same size): gcc keeps pManager in v0 (dies before the call) where the
original gave it s0, and puts the loop-variable copy before `bnez` instead of in its
delay slot.
void func_8003B930(AudioManager* pManager) {
    AudioManager* pEntry = pManager->unk_Manager_0x4;

    if (pEntry == NULL) {
        return;
    }

    pManager->unk_Manager_0x4 = NULL;
    while (pEntry != NULL) {
        AudioManager* pNext = pEntry->unk_Manager_0x4;

        SoundHeapFree(pEntry);
        pEntry = pNext;
    }
}
*/

//----------------------------------------------------------------------------------------------------------------------
void SoundCopyAudioManagerData(AudioManager* pDest, AudioManager* pSrc) {
    AudioManager* savedNext;
    AudioManager* savedUnk;

    savedNext = pDest->next;
    savedUnk = pDest->unk_Manager_0x4;
    SoundHeapSetBlockMemory(pDest, pSrc, SoundCalculateAudioManagerSize(pDest->elementCount));
    pDest->next = savedNext;
    pDest->unk_Manager_0x4 = savedUnk;
}

//----------------------------------------------------------------------------------------------------------------------
void SoundAddAudioManagerToList(AudioManager* manager)
{
    AudioManager* temp;

    DisableEvent(g_unk_SoundEvent);
    temp = manager;
    manager->next = g_SoundAudioManagerListHead;
    g_SoundAudioManagerListHead = temp;
    EnableEvent(g_unk_SoundEvent);
}

//----------------------------------------------------------------------------------------------------------------------
s32 SoundRemoveAudioManagerFromList(AudioManager* manager) {
    AudioManager* current = g_SoundAudioManagerListHead;
    AudioManager* previous = NULL;

    // Search for target AudioManager in linked list
    while (current != NULL) {
        if (current == manager) {
            break;
        }
        previous = current;
        current = current->next;
    }

    // If not found, return error
    if (current == NULL) {
        SoundHandleError(SOUND_ERR_MANAGER_NOT_IN_LIST);
        return -1;
    }

    // Handle cleanup if needed (0x8000 flag set)
    if (manager->unk_Flags & 0x8000) {
        if (manager == NULL) {
            SoundHandleError(5);  // Invalid cleanup state
        } else {
            // Clear cleanup flag and release resources
            manager->unk_Flags &= ~(1 << 15);  // Clear bit 15
            SoundReleaseAllVoices(manager);
        }
    }

    // Remove from linked list
    if (previous != NULL) {
        // Removing head node
        previous->next = manager->next;
    } else {
        // Removing middle/end node
        g_SoundAudioManagerListHead = manager->next;
    }

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
void func_8003BB08(u16 flags, AudioManager* pManager) {
    AudioElement* pElement = &pManager->elements[0];
    u32 cnt = pManager->elementCount;

    do {
        if (pElement->active_flag) {
            pElement->active_flag |= flags;
        }
        pElement++;
        cnt--;
    } while (cnt);
}

//----------------------------------------------------------------------------------------------------------------------
s32 SoundCalculateAudioManagerSize(s32 elementCount) {
    return (elementCount * sizeof(AudioElement)) + offsetof(AudioManager, elements);
}

//----------------------------------------------------------------------------------------------------------------------
void SoundOnTransferCallback(void) {
    SoundCommandCallback_t pCallback;

    pCallback = (&g_SoundTransferQueue[g_SoundTransferQueueReadIndex])->pCallbackFn;
    g_SoundControlFlags |= 4;
    if (pCallback) {
        pCallback();
    }
    
    g_SoundControlFlags &= 0xFFEF;
    if (g_SoundTransferQueueReadIndex != g_SoundTransferQueueWriteIndex) {
        SoundProcessTransferCommand();
    }
    g_SoundControlFlags &= 0xFFFB;
}

void SoundQueueSpuWriteCommand(u32 transferAddress, void* pData, u_long dataSize, SoundCommandCallback_t pCallback) {
    SoundQueueTransferCommand(transferAddress, pData, dataSize, pCallback, SOUND_SPU_COMMAND_WRITE);
}

void SoundQueueSpuReadCommand(u32 transferAddress, void* pData, u_long dataSize, SoundCommandCallback_t pCallback) {
    SoundQueueTransferCommand(transferAddress, pData, dataSize, pCallback, SOUND_SPU_COMMAND_READ);
}

// Queue SPU ReadDecodedData SPU_ALL Command
void func_8003BC58(u32 transferAddress, void* pData, u_long dataSize, SoundCommandCallback_t pCallback) {
    SoundQueueTransferCommand(transferAddress, pData, dataSize, pCallback, 3);
}

// Queue SPU ReadDecodedData SPU_CDONLY Command
void func_8003BC7C(u32 transferAddress, void* pData, u_long dataSize, SoundCommandCallback_t pCallback) {
    SoundQueueTransferCommand(transferAddress, pData, dataSize, pCallback, 4);
}

void SoundQueueTransferCommand(u32 transferAddress, void* pData, u_long dataSize, SoundCommandCallback_t pCallback, unsigned short commandType) {
    SoundTransferCommand* pCmd;
    unsigned short nControlFlags;
    unsigned short nNextIndex;

    nControlFlags = g_SoundControlFlags;
    if (!(nControlFlags & 4)) {
        while (SoundTransferQueueSync());
        EnterCriticalSection();
    }

    nNextIndex = g_SoundTransferQueueWriteIndex + 1;
    if (nNextIndex >= SOUND_TRANSFER_QUEUE_SIZE) {
        nNextIndex = 0;
    }
    g_SoundTransferQueueWriteIndex = nNextIndex;
    
    // TODO: pCmd = &g_SoundTransferQueue[nNextIndex]; doesn't match, but there should be a cleaner line here
    pCmd = nNextIndex * sizeof(SoundTransferCommand) + (u32)g_SoundTransferQueue;
    pCmd->commandType = commandType & 0xF;
    pCmd->unk2 = 0;
    pCmd->pSpuData = pData;
    pCmd->pTransferAddress = transferAddress & 0x7FFF8;
    pCmd->dataSize = dataSize;
    pCmd->pCallbackFn = pCallback;
    
    if (!(g_SoundControlFlags & 0x10)) {
        SoundProcessTransferCommand();
    }
    
    if (!(nControlFlags & 4)) {
        ExitCriticalSection();
    }
}

int SoundTransferQueueSync() {
    unsigned short nWriteIndex = g_SoundTransferQueueWriteIndex;
    if (nWriteIndex < g_SoundTransferQueueReadIndex) {
        nWriteIndex += SOUND_TRANSFER_QUEUE_SIZE;
    }
    return (nWriteIndex - g_SoundTransferQueueReadIndex < 6) ^ 1;
}

void func_8003BDF4(void) {}

// Waits for the transfer queue to finish (arg & 0x10) and returns the mode of the
// command at the read index, or 0 when the queue is idle.
s32 func_8003BDFC(u32 flags) {
    if (flags & 0x10) {
        while (*(volatile u16*)&g_SoundControlFlags & 0x10) {
        }
    }

    if (!(g_SoundControlFlags & 0x10)) {
        return 0;
    }

    return *(s16*)&g_SoundTransferQueue[g_SoundTransferQueueReadIndex];
}

//----------------------------------------------------------------------------------------------------------------------
void SoundProcessTransferCommand(void) {
    SpuTransferCallbackProc pPrevCallback;
    unsigned short nNextIndex;
    SoundTransferCommand* pCmd;

    nNextIndex = g_SoundTransferQueueReadIndex + 1;
    if (nNextIndex >= SOUND_TRANSFER_QUEUE_SIZE) {
        nNextIndex = 0;
    }
    g_SoundTransferQueueReadIndex = nNextIndex;
    
    g_SoundControlFlags |= 0x10;

    // TODO: pCmd = &g_SoundTransferQueue[nNextIndex]; doesn't match, but there should be a cleaner line here
    pCmd = nNextIndex * sizeof(SoundTransferCommand) + (u32)g_SoundTransferQueue;
    
    pPrevCallback = SpuSetTransferCallback(&SoundOnTransferCallback);
    SpuSetTransferMode(SPU_TRANSFER_BY_DMA);
    SpuSetTransferStartAddr(pCmd->pTransferAddress);
    switch (pCmd->commandType) {
        case 0:
            break;
        case SOUND_SPU_COMMAND_WRITE:
            SpuWrite(pCmd->pSpuData, pCmd->dataSize);
            break;
        case SOUND_SPU_COMMAND_READ:
            SpuRead(pCmd->pSpuData, pCmd->dataSize);
            break;
        case 3:
            D_80059548 = SpuReadDecodedData(pCmd->pSpuData, SPU_ALL);
            break;
        case 4:
            D_80059548 = SpuReadDecodedData(pCmd->pSpuData, SPU_CDONLY);
            break;
    }
    
    if (pPrevCallback != &SoundOnTransferCallback) {
        SoundHandleError(SOUND_ERR_UNEXPECTED_CALLBACK);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSpuIRQHandler(void) {
    g_SoundControlFlags |= SOUND_CTL_FLAG_IRQ_HANDLER;
    g_SoundSpuIRQCount++;
    if (g_SoundSpuIrqCallbackFn) {
        g_SoundSpuIrqCallbackFn();
    }
    g_SoundControlFlags &= ~SOUND_CTL_FLAG_IRQ_HANDLER;
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSetSpuIrqCallback(u32 func) {
    g_SoundSpuIrqCallbackFn = func;
}

//----------------------------------------------------------------------------------------------------------------------
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003C020);

//----------------------------------------------------------------------------------------------------------------------
void SoundTickInterpolator(AudioInterpolator* interpolator) {
    interpolator->counter--;

    if (interpolator->counter != 0) {
        interpolator->currentValue += interpolator->stepIncrement;
    } else {
        interpolator->currentValue = (s32)(interpolator->targetValue << 16);
    }
}

//----------------------------------------------------------------------------------------------------------------------
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003C4C4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003C6E8);

void func_8003CC84(AudioManager* pAudioManager, AudioElement* pElement, u8 percussionIndex) {
    u8* pPercussionData;
    s32 note;
    u16 statusFlags;

    pPercussionData = (u8*)pAudioManager->unk_0xc + (percussionIndex * 4);

    func_8003E5BC(pPercussionData[0], pElement);

    note = (pPercussionData[1] << 8) + pElement->unk_0x6E + pElement->unk_0x6C;
    pElement->unk68 = note << 16;
    statusFlags = pElement->status_flags;
    pElement->unk_0x74 = pPercussionData[3] << 8;
    pElement->status_flags = statusFlags | SOUND_STATUS_VOLUME_CHANGE;
}

u8* SoundScriptDefaultHandler(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    return pScript;
}

// Rest note handler?
u8* func_8003CD08(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->fermataDuration = *pScript;
    pAudioElements->active_flag |= SOUND_CHANNEL_REST_NOTE;
    pAudioElements->status_flags |= SOUND_STATUS_REST_NOTE;
    return pScript + 1;
}

// Fermata / Hold note
u8* SoundScriptFermata(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 fermataDuration = *pScript;
    pAudioElements->active_flag |= SOUND_CHANNEL_HOLD_NOTE;
    pAudioElements->fermataDuration = fermataDuration;
    return pScript + 1;
}

u8* SoundScriptNop3(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    return pScript;
}

u8* func_8003CD54(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 value = *pScript++;

    if (value == pAudioManager->unk_0x1b) {
        pAudioElements->savedScriptIP = pScript;
        pAudioElements->savedOctave = pAudioElements->octave;
    }

    return pScript;
}

u8* SoundScriptDefaultSkip3(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    return pScript + 3;
}

u8* SoundScriptNop4(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    return pScript;
}

u8* func_8003CD8C(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8* pSavedScript = pAudioElements->savedScriptIP;

    if (pSavedScript) {
        pScript = pSavedScript;
        pAudioElements->unk_0x20++;
        pAudioElements->octave = pAudioElements->savedOctave;
    } else {
        pAudioElements->status_flags &= ~0x3;
        SoundReleaseVoiceFromChannel(&pAudioElements->voice_data, pAudioElements->voice_number);
        pAudioElements->active_flag = 0;
    }

    return pScript;
}

u8* SoundScriptSaveOctaveAndIP(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->savedScriptIP = pScript;
    pAudioElements->savedOctave = pAudioElements->octave;
    return pScript;
}

u8* SoundScriptSetOctave(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->octave = *pScript * NUM_NOTES_PER_OCTAVE;
    return pScript + 1;
}

u8* SoundScriptRaiseOctave(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->octave += NUM_NOTES_PER_OCTAVE;
    return pScript;
}

u8* SoundScriptLowerOctave(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->octave -= NUM_NOTES_PER_OCTAVE;
    return pScript;
}

// Time signature handler?
u8* func_8003CE68(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 divisor = pScript[1];
    u16 step = 0xC0 / divisor;
    u16 value = *pScript;

    pAudioManager->unk_0x3a = step;
    pAudioManager->unk_0x3c = divisor;
    pAudioManager->unk_0x38 = value;
    pAudioManager->unk_0x3e = value;
    pAudioManager->unk_0x36 = pAudioManager->unk_0x3a;
    return pScript + 2;
}

u8* func_8003CE9C(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioManager->unk_0x32 = pScript[0];
    pAudioManager->unk_0x34 = pScript[1];
    pAudioManager->unk_0x36 = pAudioManager->unk_0x3a;
    return pScript + 2;
}

u8* SoundScriptSetManagerUnk1a(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioManager->unk_0x1a = *pScript;
    return pScript + 1;
}

u8* SoundScriptAddManagerUnk1a(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioManager->unk_0x1a += *pScript;
    return pScript + 1;
}

// Loop start handler?
u8* func_8003CEF0(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    AudioLoop* pLoop;

    pAudioElements->unk_0x72++;
    pLoop = &pAudioElements->loops[pAudioElements->unk_0x72];
    pLoop->count = *pScript++ - 1;
    pLoop->start = pScript;
    pLoop->savedOctave = pAudioElements->octave;
    return pScript;
}

// Loop end handler?
u8* func_8003CF38(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    AudioLoop* pLoop = &pAudioElements->loops[pAudioElements->unk_0x72];
    u8* pResult = pScript;

    if (--pLoop->count != 0xFF) {
        pLoop->end = pScript;
        pLoop->unk3 = pAudioElements->octave;
        pResult = pLoop->start;
        pAudioElements->octave = pLoop->savedOctave;
    } else {
        pAudioElements->unk_0x72--;
    }

    return pResult;
}

u8* func_8003CFA4(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    AudioLoop* pLoop = &pAudioElements->loops[pAudioElements->unk_0x72];
    u8* pResult = pScript;

    if (pLoop->count == 0) {
        pResult = pLoop->end;
        pAudioElements->octave = pLoop->unk3;
        pAudioElements->unk_0x72--;
    }

    return pResult;
}

u8* func_8003CFF0(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    func_80039F18(pScript[0] | (pScript[1] << 8), 0x7F, 0x40);
    return pScript + 3;
}

u8* func_8003D034(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    func_8003A14C(pScript[0] | (pScript[1] << 8));
    return pScript + 2;
}

u8* func_8003D070(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    SoundFile* pSoundFile = g_SoundSedsLinkedList;
    s16 sedId = *(s16*)((u8*)pAudioElements + 0xA);
    s16 instrument = pScript[0] | (pScript[1] << 8);
    u8 channel = pScript[2];

    if (sedId != 0) {
        while (pSoundFile->sedId != sedId) {
            pSoundFile = pSoundFile->pNext;
            if (pSoundFile == NULL) {
                return pScript;
            }
        }
    }

    pScript = (u8*)pSoundFile + pSoundFile->scriptOffsets[instrument * 2 + channel];
    return pScript + 3;
}

// Set tempo handler?
u8* func_8003D0E8(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 value = *pScript++;

    pAudioManager->unk_0x58 = value << 16;
    pAudioManager->unk_0x54 = (value & 0xFF) * (s16)(pAudioManager->unk_Interpolator_0x64.currentValue >> 16);
    return pScript;
}

u8* func_8003D110(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 pad[2]; // unused: reproduces the original's 0x8-byte frame
    s8 value = *pScript;

    pAudioManager->unk_0x58 += value << 16;
    pAudioManager->unk_0x54 = 0;
    return pScript + 1;
}
/*
Attempt (67.27%): the original has an unused 0x8-byte stack frame
(`addiu sp,sp,-0x8` ... `addiu sp,sp,0x8`) and computes the return value before the
epilogue, so its source had something that forces a frame this form does not.
u8* func_8003D110(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s8 value = *pScript;

    pAudioManager->unk_0x58 += value << 16;
    pAudioManager->unk_0x54 = 0;
    return pScript + 1;
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003D13C);
/*
Attempt (96.25%, correct size): original has `move a2,a3` where gcc emits
`andi a2,a3,0xff`; with an `s32` local gcc drops the copy altogether (93.12%).
u8* func_8003D13C(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 unk0 = pScript[0];
    u8 unk1 = pScript[1];
    s32 delta;

    pAudioManager->unk_0x62 = unk1;
    delta = (unk1 << 16) - pAudioManager->unk_0x58;
    if (unk0 != 0 && delta != 0) {
        pAudioManager->unk_0x60 = unk0;
        pAudioManager->unk_0x5c = delta / unk0;
    }
    return pScript + 2;
}
*/

u8* func_8003D17C(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioManager->unk_Interpolator_0x70.currentValue = *pScript++ << 24;
    unk_SoundSetFlagsOnActiveVoices(0x100, pAudioManager);
    return pScript;
}

u8* func_8003D1BC(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 step = pScript[0];
    u8 target = pScript[1];
    u16 counter = step << 5;
    s32 delta = (target << 24) - pAudioManager->unk_Interpolator_0x70.currentValue;

    if ((step << 5) != 0 && delta != 0) {
        pAudioManager->unk_Interpolator_0x70.counter = counter;
        pAudioManager->unk_Interpolator_0x70.targetValue = target << 8;
        pAudioManager->unk_Interpolator_0x70.stepIncrement = delta / (step << 5);
    }
    return pScript + 2;
}

u8* SoundScriptSetUnk62(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->unk_0x62 = *pScript;
    return pScript + 1;
}

u8* func_8003D21C(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 note;

    note = *pScript++;

    if (note < 0x19) {
        SoundReleaseVoiceFromChannel(&pAudioElements->voice_data, pAudioElements->voice_number);
        SoundAssignVoiceToChannelAndStop(&pAudioElements->voice_data, pAudioElements->voice_number = note);
    }

    return pScript;
}

u8* func_8003D298(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    func_8003E5BC(*pScript++, pAudioElements);
    return pScript;
}

u8* func_8003D2D0(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 value = *pScript++;

    if (value) {
        pAudioElements->unk_0x60 += value;
    } else {
        pAudioElements->unk_0x60 = 0;
    }

    return pScript;
}

// Percussion On handler?
u8* func_8003D300(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    if (pAudioManager->unk_0xc) {
        pAudioElements->active_flag |= SOUND_CHANNEL_PERCUSSION_ACTIVE;
    }

    return pScript;
}

u8* SoundScriptPercussionOff(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->active_flag &= ~SOUND_CHANNEL_PERCUSSION_ACTIVE;
    return pScript;
}

u8* SoundScriptSetActiveFlag800(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->active_flag |= 0x800;
    return pScript;
}

u8* SoundScriptClearActiveFlag800(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->active_flag &= ~0x800;
    return pScript;
}

u8* func_8003D370(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    if (pAudioElements->voice_number & 0x1) {
        pAudioElements->voice_data.flags |= 0x1000;
        pAudioElements->voice_data.modeFlags |= 0x10;
    }

    return pScript;
}

u8* func_8003D3A4(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    if (pAudioElements->voice_number & 0x1) {
        pAudioElements->voice_data.flags |= 0x1000;
        pAudioElements->voice_data.modeFlags &= ~0x10;
    }

    return pScript;
}

u8* func_8003D3D8(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioManager->unk_0x1c = *pScript++;
    SpuSetNoiseClock(pAudioManager->unk_0x1c);
    pAudioElements->voice_data.flags |= 0x2000;
    pAudioElements->voice_data.modeFlags |= 0x20;
    return pScript;
}

u8* func_8003D438(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioManager->unk_0x1c = (pAudioManager->unk_0x1c + *pScript++) & 0x3F;
    SpuSetNoiseClock(pAudioManager->unk_0x1c);
    pAudioElements->voice_data.flags |= 0x2000;
    pAudioElements->voice_data.modeFlags |= 0x20;
    return pScript;
}

u8* SoundScriptSetVoiceFlags2000AndMode(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->voice_data.flags |= 0x2000;
    pAudioElements->voice_data.modeFlags |= 0x20;
    return pScript;
}

u8* SoundScriptSetVoiceFlags2000ClearMode(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->voice_data.flags |= 0x2000;
    pAudioElements->voice_data.modeFlags &= ~0x20;
    return pScript;
}

u8* func_8003D4E4(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s16 depth;
    s32 delay;
    s32 feedback;

    depth = pScript[0] << 8;
    pAudioManager->reverbDepth = depth;
    delay = (s8)pScript[1];
    pAudioManager->reverbDelay = delay;
    feedback = (s8)pScript[2];
    pAudioManager->reverbFeedback = feedback;
    SoundSetReverbModeWithAllocation(SPU_REV_MODE_CHECK, depth, delay, feedback);
    return pScript + 3;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003D53C);
/*
Attempt (95.83%, same size): the first two tests must fall through to a shared
`return pScript`, which is why the original has `move v0,a0` in all three branch
delay slots; gcc also emits a fourth copy at the top of the body block.
u8* func_8003D53C(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    if (pAudioManager->unk_Flags & 0x6) {
        if ((g_SoundControlFlags & 0x2000) == 0) {
            return pScript;
        }
        if (pAudioElements->active_flag & 0x2) {
            return pScript;
        }
    }

    pAudioElements->voice_data.flags |= 0x4000;
    pAudioElements->voice_data.modeFlags |= 0x40;
    return pScript;
}
*/

u8* SoundScriptSetVoiceFlags4000ClearMode(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->voice_data.flags |= 0x4000;
    pAudioElements->voice_data.modeFlags &= ~0x40;
    return pScript;
}

int SoundScriptSkip3(int arg0) {
    return arg0 + 3;
}

s32 SoundScriptNop(s32 arg0) {
    return arg0;
}

s32 SoundScriptNop2(s32 arg0) {
    return arg0;
}

// ADSR / Envelope Reset Handler?
u8* SoundScriptCallE5BC(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    func_8003E5BC(pAudioElements->unk_0x26, pAudioElements);
    return pScript;
}

u8* func_8003D60C(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->voice_data.unkAdsr1 = pScript[0];
    pAudioElements->voice_data.unkAdsr2 = pScript[1];
    pAudioElements->voice_data.unkAdsr3 = pScript[2];
    pAudioElements->voice_data.flags |= 0x1F0;
    return pScript + 3;
}

u8* SoundScriptSetAttackTime(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 attackTime = *pScript;
    pAudioElements->voice_data.flags |= 0x10;
    pAudioElements->voice_data.unkAdsr4 = attackTime;
    return pScript + 1;
}

u8* SoundScriptSetDecayTime(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 decayTime = *pScript;
    pAudioElements->voice_data.flags |= 0x20;
    pAudioElements->voice_data.adsrDR = decayTime;
    return pScript + 1;
}

u8* SoundScriptSetSustain(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 temp = *pScript;
    pAudioElements->voice_data.flags |= 0x40;
    pAudioElements->voice_data.unkAdsr5 = temp;
    return pScript + 1;
}

u8* SoundScriptSetRelease(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 temp = *pScript;
    pAudioElements->voice_data.flags |= 0x80;
    pAudioElements->unk_0x28 = temp;
    pAudioElements->voice_data.unkAdsr6 = temp;
    return pScript + 1;
}

u8* SoundScriptSetSustainLevel(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 temp = *pScript;
    pAudioElements->voice_data.flags |= 0x100;
    pAudioElements->voice_data.adsrSR = temp;
    return pScript + 1;
}

// Set decay and sustain?
u8* SoundScriptSetAdsrDRAndSR(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->voice_data.adsrDR = *pScript;
    pAudioElements->voice_data.adsrSR = *(pScript + 1);
    pAudioElements->voice_data.flags |= 0x120;
    return pScript + 2;
}

u8* SoundScriptSetAttackMode(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 attackMode = *pScript;
    pAudioElements->voice_data.flags |= 0x10;
    pAudioElements->voice_data.unkAdsr1 = attackMode;
    return pScript + 1;
}

u8* SoundScriptSetSustainMode(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 sustainMode = *pScript;
    pAudioElements->voice_data.flags |= 0x40;
    pAudioElements->voice_data.unkAdsr2 = sustainMode;
    return pScript + 1;
}

u8* SoundScriptSetReleaseMode(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 releaseMode = *pScript;
    pAudioElements->voice_data.flags |= 0x80;
    pAudioElements->voice_data.unkAdsr3 = releaseMode;
    return pScript + 1;
}

u8* SoundScriptSetUnk6E(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s16 temp = (s8)*pScript << 5;
    pAudioElements->unk_0x6E = temp;
    pAudioElements->status_flags |= 0x200;
    return pScript + 1;
}

u8* SoundScriptAddUnk6E(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->unk_0x6E += (s8)*pScript << 5;
    pAudioElements->status_flags |= 0x200;
    return pScript + 1;
}

u8* func_8003D79C(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->unk_0x6E += (s8)*pScript << 3;
    pAudioElements->status_flags |= SOUND_STATUS_PLAYING_NTOE;
    return pScript + 1;
}

u8* func_8003D7C8(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->unk_0x6E += pScript[1] + ((s8)pScript[0] << 8);
    pAudioElements->status_flags |= SOUND_STATUS_PLAYING_NTOE;
    return pScript + 2;
}

u8* func_8003D7FC(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u16 value = pScript[0];
    s32 delta = (s8)pScript[1] << 24;

    if (value != 0 && delta != 0) {
        pAudioElements->unk_0x94 = value;
        pAudioElements->unk_0x04 |= 0x1;
        pAudioElements->unk_0x84 = delta / value;
    } else {
        pAudioElements->unk_0x04 &= ~0x1;
    }
    return pScript + 2;
}

u8* SoundScriptToggleUnk04Bit2(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->unk_0x04 ^= 0x2;
    return pScript;
}

u8* SoundScriptClearUnk04Bit1(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->unk_0x04 &= ~0x1;
    return pScript;
}

u8* func_8003D884(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 value = *pScript++;

    pAudioElements->unk_0x70 = value;
    if (value) {
        pAudioElements->unk_0x04 |= 0x4;
    } else {
        pAudioElements->unk_0x04 &= ~0x4;
    }

    return pScript;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003D8B8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003D9A4);

u8* func_8003DAB0(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 divisor = (u8)(*pScript + 1);
    s16 step;

    pScript++;
    if (divisor) {
        divisor *= 4;
        step = 0x400 / divisor;
        pAudioElements->envelopes[0].unk_0x1A = step;
        pAudioElements->envelopes[0].unk_0x18 = step;
    }

    return pScript;
}

u8* SoundScriptSetUnkCEAndF6(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->unk_0xCE |= 0x1;
    pAudioElements->envelopes[0].flags |= 0x1;
    return pScript;
}

u8* SoundScriptClearUnkCEAndF6(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->unk_0xCE &= ~0x1;
    pAudioElements->envelopes[0].flags &= ~0x1;
    return pScript;
}

// Set volume handler / Dynamic?
u8* func_8003DB2C(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->unk_0x78 = *pScript << 24;
    pAudioElements->status_flags |= SOUND_STATUS_VOLUME_CHANGE;
    pAudioElements->unk_0x04 &= ~0x108;
    return pScript + 1;
}

// Crescendo?
u8* func_8003DB58(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->unk_0x78 = (pAudioElements->unk_0x78 + ((s8)*pScript << 24)) & 0x7FFFFFFF;
    pAudioElements->status_flags |= SOUND_STATUS_VOLUME_CHANGE;
    pAudioElements->unk_0x04 &= ~0x108;
    return pScript + 1;
}

u8* func_8003DB98(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u16 frames = *pScript;
    s32 delta = ((s8)pScript[1] << 24) - pAudioElements->unk_0x78;

    if (frames && delta) {
        pAudioElements->unk_0x96 = frames;
        pAudioElements->unk_0x04 = (pAudioElements->unk_0x04 | 0x8) & ~0x100;
        pAudioElements->unk_0x88 = delta / frames;
    }

    return pScript + 2;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003DBE4);
/*
Attempt (97.78%, correct size): the only difference is the original's
`move a1,t0` copy of the frame count, where gcc 2.6 emits `andi a1,t0,0xff`
(`s32 frames` instead makes gcc fold the copy away entirely, 97.41%).
Same gcc 2.6 byte->int copy quirk as func_8003D0E8 and func_8003D13C.
u8* func_8003DBE4(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 start = pScript[0] << 24;
    s32 target = pScript[2] << 24;
    u8 frames = pScript[1];
    s32 delta = target - start;

    if (delta != 0 && frames != 0) {
        pAudioElements->ramp.step = delta / frames;
        pAudioElements->ramp.target = start >> 16;
        pAudioElements->ramp.counter = frames;
        pAudioElements->unk_0x04 = (pAudioElements->unk_0x04 | 0x100) & ~0x8;
    } else {
        pAudioElements->unk_0x04 &= ~0x100;
    }

    return pScript + 3;
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003DC50);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003DD24);

u8* func_8003DE18(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 divisor = (u8)(*pScript + 1);
    s16 step;

    pScript++;
    if (divisor) {
        divisor *= 4;
        step = 0x400 / divisor;
        pAudioElements->envelopes[1].unk_0x1A = step;
        pAudioElements->envelopes[1].unk_0x18 = step;
    }

    return pScript;
}

u8* SoundScriptSetUnkCEAndUnk116(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->unk_0xCE |= 0x2;
    pAudioElements->envelopes[1].flags |= 0x1;
    return pScript;
}

u8* SoundScriptClearUnkCEAndUnk116(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->unk_0xCE &= ~0x2;
    pAudioElements->envelopes[1].flags &= ~0x1;
    return pScript;
}

// Pan / Balance handler?
u8* SoundScriptSetUnk74(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->unk_0x74 = *pScript << 8;
    pAudioElements->status_flags |= 0x100;
    return pScript + 1;
}

u8* func_8003DEB4(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->unk_0x74 += (s8)*pScript << 8;
    pAudioElements->unk_0x74 &= 0x7FFF;
    pAudioElements->status_flags |= SOUND_STATUS_VOLUME_CHANGE;
    return pScript + 1;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003DEE4);

/* Best attempt (76.59%): every instruction matches except the entry `move a1, a0`
   (the original keeps the script base in a1) and the `subu` computing delta, which
   the original places in the delay slot of the frames test instead of before it.
u8* func_8003DEE4(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 target = (s8)pScript[1];
    s32 current = (s8)(pAudioElements->unk_0x74 >> 8);
    u16 frames = pScript[0];
    s32 delta;

    if (frames && (delta = target - current)) {
        delta <<= 8;
        pAudioElements->unk_0x92 = delta;
        pAudioElements->unk_0x98 = frames;
        pAudioElements->unk_0x04 |= 0x10;
        pAudioElements->unk_0x90 = delta / frames;
    }

    return pScript + 2;
}
*/

u8* func_8003DF3C(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 divisor = (u8)(*pScript + 1);
    s16 step;

    pScript++;
    if (divisor) {
        divisor *= 4;
        step = 0x400 / divisor;
        pAudioElements->envelopes[2].unk_0x1A = step;
        pAudioElements->envelopes[2].unk_0x18 = step;
    }

    return pScript;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003DF78);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003E04C);

u8* SoundScriptSetUnkCEAndUnk136(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->unk_0xCE |= 0x4;
    pAudioElements->envelopes[2].flags |= 0x1;
    return pScript;
}

u8* func_8003E160(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->unk_0xCE &= ~0x4;
    pAudioElements->envelopes[2].flags &= ~0x1;
    return pScript;
}

u8* func_8003E180(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    AudioEnvelope* pEnvelope;
    u8 value;
    u8 last;

    pAudioElements->unk_0xCC = pScript[0];
    pEnvelope = &pAudioElements->envelopes[pAudioElements->unk_0xCC];
    value = pScript[1];
    pEnvelope->unk_0x1D = value & 0xF;
    pEnvelope->unk_0x0 = D_800508A4[pEnvelope->unk_0x1D];

    if ((value & 0x10) == 0) {
        pEnvelope->flags = 2;
    } else {
        pEnvelope->flags = 0;
    }

    last = pScript[2];
    pEnvelope->unk_0x1A = 0x400;
    pEnvelope->unk_0x16 = 0;
    pEnvelope->unk_0x1C = last;
    return pScript + 3;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003E1F8);

s32 func_8003E290(s32 value, s16 divisor, s16 mode) {
    if (value != 0 && divisor != 0) {
        switch (mode) {
            case 2:
            case 3:
                value /= divisor;
                break;
            case 4:
                if (divisor != 1) {
                    value /= divisor - 1;
                }
                break;
        }
    }

    return value;
}

u8* func_8003E308(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 divisor = pScript[1] + 1;
    AudioEnvelope* pEnvelope = &pAudioElements->envelopes[pAudioElements->unk_0xCC];
    s16 step;

    if (divisor) {
        s32 scaledDivisor = divisor * 4;

        step = 0x400 / scaledDivisor;
        pEnvelope->unk_0x16 = *pScript << 2;
        pEnvelope->unk_0x1A = step;
        pEnvelope->unk_0x18 = step;
    }

    return pScript + 2;
}

u8* SoundScriptNop5(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    return pScript;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003E360);
/*
Attempt (93.38%, same size): only the s-register assignment is swapped (original
keeps pScript in s1 / pAudioElements in s2, gcc 2.6 picks the opposite) and the
original returns pScript+1 from s1 in v0 instead of computing it early.
u8* func_8003E360(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 index = pScript[0];
    AudioEnvelope* pEnvelope = &pAudioElements->envelopes[index];

    func_8003E3E0(pEnvelope);
    pEnvelope->flags |= 0x1;
    pAudioElements->unk_0xCE |= 1 << index;
    return pScript + 1;
}
*/

void func_8003E3E0(AudioEnvelope* pEnvelope) {
    pEnvelope->count = 1;
    pEnvelope->value = 0;
    pEnvelope->flags &= ~0xC;
    pEnvelope->unk_0x14 = pEnvelope->unk_0x16;
    pEnvelope->unk_0x18 = pEnvelope->unk_0x1A;
}

u8* func_8003E40C(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 index = *pScript;

    pAudioElements->envelopes[index].flags &= ~0x1;
    pAudioElements->unk_0xCE &= ~(1 << index);
    return pScript + 1;
}

u8* func_8003E44C(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 instrument = *pScript;
    u8 value = pScript[1];
    SoundWDSEntry* pWds;

    pAudioElements->unk_0x25 = instrument;
    pWds = SoundFindWdsEntry(instrument);
    if (!pWds) {
        pWds = g_SoundWdsLinkedList;
    }
    pAudioElements->unk2C = (u32)pWds;
    func_8003E5BC(value, pAudioElements);
    return pScript + 2;
}

u8* func_8003E4BC(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 value = *pScript++;

    if (value != 0) {
        pAudioManager->unk_Interpolator_0x64.currentValue = value << 24;
        pAudioManager->unk_0x54 = (pAudioManager->unk_0x58 >> 16) * (value << 8);
    }
    return pScript;
}

u8* func_8003E4F0(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 instrument = *pScript++;
    SoundWDSEntry* pWds;

    pAudioElements->unk_0x25 = instrument;
    pWds = SoundFindWdsEntry(instrument);
    if (!pWds) {
        pWds = g_SoundWdsLinkedList;
    }
    pAudioElements->unk2C = (u32)pWds;
    return pScript;
}

u8* func_8003E54C(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    long keyStatus;
    short envx;

    SpuGetVoiceEnvelopeAttr(pAudioElements->voice_number, &keyStatus, &envx);

    if (envx == 0) {
        pAudioElements->status_flags &= ~0x3;
        SoundReleaseVoiceFromChannel(&pAudioElements->voice_data, pAudioElements->voice_number);
        pAudioElements->active_flag = 0;
    }

    return pScript;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003E5BC);

//----------------------------------------------------------------------------------------------------------------------
void unk_SoundSetFlagsOnActiveVoices(u16 flags, AudioManager* manager) {
    AudioElement* pElement;
    u32 cnt;

    pElement = &manager->elements[0];
    cnt = manager->elementCount;

    do {
        if (pElement->active_flag) {
            pElement->status_flags = flags | pElement->status_flags;
        }
        pElement++;
        cnt--;
    } while (cnt);
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSetFlagsOnActiveVoices(AudioManager* manager, s32 flags) {
    AudioElement* pElement;
    u32 cnt;

    pElement = &manager->elements[0];
    cnt = manager->elementCount;

    do {

        if (pElement->active_flag) {
            // this doesn't feel right, considering SoundVoiceData::flags is a 16-bit flag
            // I know that SoundVoiceData::volume is 16-bits from SoundVoiceData::flags from func_8003E900
            // but I get a reg swap if flags is 16 bits
            pElement->voice_data.flags |= (u16)flags;
        }

        pElement++;
        cnt--;
    } while (cnt);
}

//----------------------------------------------------------------------------------------------------------------------
void SoundClearVoiceDataPointers(void) {
    s32 offset = sizeof(SoundVoiceData*) * (NUM_VOICES - 1);
    while( offset >= 0 ) {
        *(u32*)((u8*)&g_SoundChannels + offset) = NULL;
        offset -= sizeof(SoundVoiceData*);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SoundAssignVoiceToChannelAndStop(SoundVoiceData* voiceData, u32 channelIndex) {

    SoundVoiceData* currentVoice;
    SoundVoiceData** pChannel;

    pChannel = &g_SoundChannels[channelIndex];
    if (channelIndex < NUM_VOICES) {
        currentVoice = *pChannel;

        // Mark current voice as needing update
        if (currentVoice == voiceData) {
            g_unk_VoicesNeedingProcessing = (1 << channelIndex) | g_unk_VoicesNeedingProcessing;
            return;
        }

        // Do not steal a higher priority voices
        if (currentVoice && currentVoice->priority > voiceData->priority) {
            return;
        }

        // Assign voice to channel
        voiceData->flags = 0xFFFF;
        voiceData->assignedVoice = channelIndex;
        g_SoundChannels[channelIndex] = voiceData;

        // Mark for voice processing
        g_unk_VoicesNeedingProcessing = (1 << channelIndex) | g_unk_VoicesNeedingProcessing;

        // Stop any current key ons for this channel
        g_SoundKeyOnFlags = ~(1 << channelIndex) & g_SoundKeyOnFlags;
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SoundAssignVoiceToChannel(SoundVoiceData* voiceData, u32 channelIndex) {
    SoundVoiceData* currentVoice;
    SoundVoiceData** channelPtr;

    channelPtr = &g_SoundChannels[channelIndex];
    if (channelIndex < NUM_VOICES) {
        currentVoice = *channelPtr;

        if (currentVoice == voiceData || (currentVoice && currentVoice->priority > voiceData->priority)) {
            return;
        }

        voiceData->assignedVoice = channelIndex;
        *channelPtr = voiceData;
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SoundReleaseVoiceFromChannel(SoundVoiceData* voiceData, uint channelIndex)
{
    SoundVoiceData** channelPtr;
    uint channelBitMask;

    channelPtr = &g_SoundChannels[channelIndex];
    if (channelIndex < NUM_VOICES) {
        if (*channelPtr == voiceData) {
            *channelPtr = NULL;
            channelBitMask = 1 << channelIndex;
            g_unk_VoicesNeedingProcessing = channelBitMask | g_unk_VoicesNeedingProcessing;

            // Clear key-on flag to prevent playback
            g_SoundKeyOnFlags = ~channelBitMask & g_SoundKeyOnFlags;
        }
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SoundAbortVoiceOnChannel(SoundVoiceData* voiceData, u32 channelIndex) {

    if ((channelIndex < NUM_VOICES) && (g_SoundChannels[channelIndex] == voiceData)) {
        g_unk_VoicesNeedingProcessing = (1 << channelIndex) | g_unk_VoicesNeedingProcessing;
        g_SoundKeyOnFlags = ~(1 << channelIndex) & g_SoundKeyOnFlags;
    }
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003E900);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003EB5C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003EBF0);

// .sdata tables used by the sample-rate helper below (splat data segment).
extern u8 D_80050B78[];
extern s16 D_80050BF0[];

s16 func_8003EEA0(u32 tone) {
    s32 entry = D_80050B78[(tone & 0x7FFF) >> 8];
    s32 shift = 6 - (entry >> 4);
    s32 value = D_80050BF0[(tone & 0xFF) + ((entry & 0xF) << 8)];

    if (shift < 0) {
        value <<= -shift;
    } else {
        value >>= shift;
    }
    return value;
}

//----------------------------------------------------------------------------------------------------------------------
void SoundAssignVoiceToChannelAndPlay(SoundVoiceData* voiceData, u32 channelIndex)
{
    SoundVoiceData* currentVoice;
    SoundVoiceData** pChannel;

    pChannel = &g_SoundChannels[channelIndex];
    if (channelIndex < NUM_VOICES) {
        currentVoice = *pChannel;

        // Skip assignment if voice already assigned
        if (currentVoice != voiceData) {
            if (currentVoice && currentVoice->priority > voiceData->priority) {
                return;
            }

            // Assign voice to channel
            voiceData->flags = 0xFFFF;
            voiceData->assignedVoice = channelIndex;
            g_SoundChannels[channelIndex] = voiceData;

            // Mark for voice processing
            g_unk_VoicesNeedingProcessing = (1 << channelIndex) | g_unk_VoicesNeedingProcessing;
        }

        // Always trigger playback on this channel
        g_SoundKeyOnFlags = (1 << channelIndex) | g_SoundKeyOnFlags;
    }
}

void SoundStopVoiceOnChannel(SoundVoiceData* voiceData, u32 channelIndex) {

    if( channelIndex < NUM_VOICES && g_SoundChannels[channelIndex] == voiceData ) {

        g_SoundKeyOffFlags = (1 << channelIndex) | g_SoundKeyOffFlags;
    }
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003EFE4);

void func_8003F190(AudioEnvelope* pEnvelope) {
    pEnvelope->flags &= ~0x1;
}

s32 func_8003F1A4(AudioEnvelope* pEnvelope) {
    if (--pEnvelope->count == 0) {
        pEnvelope->count = pEnvelope->reload;
        pEnvelope->value = pEnvelope->value ? 0 : pEnvelope->delta;
    }

    return pEnvelope->value;
}

s32 func_8003F1EC(AudioEnvelope* pEnvelope) {
    if (--pEnvelope->count == 0) {
        s32 delta = pEnvelope->delta;

        pEnvelope->count = pEnvelope->reload;
        if (pEnvelope->flags & 0x8) {
            delta = -delta;
        }
        pEnvelope->value = delta;
        pEnvelope->flags ^= 0x8;
    }

    return pEnvelope->value;
}

s32 func_8003F240(AudioEnvelope* pEnvelope) {
    if (--pEnvelope->count == 0) {
        s32 delta = pEnvelope->delta;

        pEnvelope->count = pEnvelope->reload;
        if (pEnvelope->flags & 0x8) {
            delta = -delta;
        }
        pEnvelope->unk_0x8 = delta;
        pEnvelope->flags ^= 0x8;
    }

    pEnvelope->value += pEnvelope->unk_0x8;
    return pEnvelope->value;
}

s32 func_8003F2A0(AudioEnvelope* pEnvelope) {
    s32 count = pEnvelope->count;

    count--;
    if (count == 0) {
        u16 flags = pEnvelope->flags;
        s32 delta;

        count = pEnvelope->reload;
        if (flags & 0x4) {
            count <<= 1;
        }
        delta = pEnvelope->delta;
        pEnvelope->unk_0x8 = delta;
        if (flags & 0x8) {
            pEnvelope->unk_0x8 = -delta;
        }
        flags = (flags | 0x4) ^ 0x8;
        pEnvelope->flags = flags;
    }

    pEnvelope->count = count;
    pEnvelope->value += pEnvelope->unk_0x8;
    return pEnvelope->value;
}

s32 func_8003F308(AudioEnvelope* pEnvelope) {
    if (--pEnvelope->count == 0) {
        pEnvelope->count = pEnvelope->reload;
        pEnvelope->value = 0;
    } else {
        pEnvelope->value += pEnvelope->delta;
    }

    return pEnvelope->value;
}

s32 func_8003F354(AudioEnvelope* pEnvelope) {
    s32 random = func_8003F43C();

    if (--pEnvelope->count == 0) {
        pEnvelope->count = pEnvelope->reload;
        random = func_8003F43C();
        pEnvelope->value = (pEnvelope->delta >> 15) * random;
    }

    return pEnvelope->value;
}

s32 func_8003F3C0(AudioEnvelope* pEnvelope) {
    if (--pEnvelope->count == 0) {
        s32 delta;
        s32 random;

        pEnvelope->count = pEnvelope->reload;
        random = func_8003F43C();
        delta = pEnvelope->delta;
        pEnvelope->value = (delta >> 14) * random - delta;
    }

    return pEnvelope->value;
}

void func_8003F42C(s32 seed) {
    D_800594E4 = seed;
}

s32 func_8003F43C(void) {
    s32 seed = D_800594E4;

    seed ^= seed << 17;
    seed ^= seed >> 15;
    D_800594E4 = seed;

    return seed & 0x7FFF;
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSetVoiceKeyOn(u32 voiceFlags) {
    g_pSoundSpuRegisters->_rxx.key_on[0] = voiceFlags;
    g_pSoundSpuRegisters->_rxx.key_on[1] = (u16)(voiceFlags >> 0x10);
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSetVoiceKeyOff(u32 voiceFlags) {
    g_pSoundSpuRegisters->_rxx.key_off[0] = voiceFlags;
    g_pSoundSpuRegisters->_rxx.key_off[1] = (u16)(voiceFlags >> 0x10);
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSetReverbVoices(u32 voiceFlags) {
    g_pSoundSpuRegisters->_rxx.rev_mode[0] = voiceFlags;
    g_pSoundSpuRegisters->_rxx.rev_mode[1] = (u16)(voiceFlags >> 0x10);
}

//----------------------------------------------------------------------------------------------------------------------
void SoundUnkDebugNoReturn_8003F4BC(void) {}

//----------------------------------------------------------------------------------------------------------------------
void SoundSetVoiceStartAddress(s32 voiceIndex, s32 addr) {
    SPU_VOICE_REG* voice = &g_pSoundSpuRegisters->_rxx.voice[voiceIndex];
    voice->addr = addr >> 3;
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSetVoiceLoopAddress(s32 voiceIndex, s32 addr) {
    SPU_VOICE_REG* voice = &g_pSoundSpuRegisters->_rxx.voice[voiceIndex];
    voice->loop_addr = addr >> 3;
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSetVoiceVolume(s32 voiceIndex, s32 volL, s32 volR) {
    SPU_VOICE_REG* voice = &g_pSoundSpuRegisters->_rxx.voice[voiceIndex];
    voice->volume.left = volL;
    voice->volume.right = volR;
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSetVoicePitch(s32 voiceIndex, s32 pitch) {
    SPU_VOICE_REG* voice = &g_pSoundSpuRegisters->_rxx.voice[voiceIndex];
    voice->pitch = pitch;
}

//----------------------------------------------------------------------------------------------------------------------
// ADSR Functions
void SoundSetVoiceAdsrAttackModeAndRate(s32 voiceIndex, s32 attackRate, s32 attackModeBit2) {
    SPU_VOICE_REG* voice = &g_pSoundSpuRegisters->_rxx.voice[voiceIndex];
    voice->adsr[0] = (voice->adsr[0] & 0x00FF) +
        (attackRate << 8) +
        ((attackModeBit2 >> 2) << 15);
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSetVoiceAdsrDecayShift(s32 voiceIndex, s32 decayShift) {
    SPU_VOICE_REG* voice = &g_pSoundSpuRegisters->_rxx.voice[voiceIndex];
    voice->adsr[0] = (voice->adsr[0] & 0xFF0F) + (decayShift << 4);
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSetVoiceAdsrSustainRateAndDirection(s32 voiceIndex, s32 sustainRate, s32 sustainDirBit1) {
    SPU_VOICE_REG* voice = &g_pSoundSpuRegisters->_rxx.voice[voiceIndex];
    voice->adsr[1] = (voice->adsr[1] & 0x003F) + (sustainRate << 6) + ((sustainDirBit1 >> 1) << 14);
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSetVoiceAdsrReleaseShiftAndMode(s32 voiceIndex, s32 releaseRate, s32 releaseModeBit2) {
    SPU_VOICE_REG* voice = &g_pSoundSpuRegisters->_rxx.voice[voiceIndex];
    s32 combinedValue = releaseRate + ((releaseModeBit2 >> 2) << 5);
    voice->adsr[1] = (voice->adsr[1] & 0xFFC0) + combinedValue;
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSetVoiceAdsrSustainLevel(s32 voiceIndex, s32 sustainLevel) {
    SPU_VOICE_REG* voice = &g_pSoundSpuRegisters->_rxx.voice[voiceIndex];
    voice->adsr[0] = (voice->adsr[0] & 0xFFF0) + sustainLevel;
}
// End ADSR functions

//----------------------------------------------------------------------------------------------------------------------
int SoundValidateFile(SoundFile* pSoundFile, u32 magicBytes, unsigned short targetValue) {
    unsigned char bIsError;
    
    if (pSoundFile->magic != magicBytes) {
        return SOUND_ERR_INVALID_SIGNATURE;
    }
    
    if (SoundFileComputeChecksum(pSoundFile) == 0) {
        // Version check?
        bIsError = (pSoundFile->unkC != targetValue);
        return bIsError * SOUND_ERR_UNK_0X4;
    }
    
    return SOUND_ERR_INVALID_CHECKSUM;
}

//----------------------------------------------------------------------------------------------------------------------
s32 SoundUnkDebug0(void* p) {
#if 0
    // Secrets of the universe
#endif

    return 0;
}

//----------------------------------------------------------------------------------------------------------------------
int SoundFileComputeChecksum(SoundFile* pSoundFile) {
    int nResult;
    int* pCurrent;
    unsigned int nCount;

    pCurrent = pSoundFile;
    nCount = (pSoundFile->unk8 + 3) / 4; // Align to 4-byte boundary
    nResult = 0;
    do {
        nResult += *pCurrent++;
    } while (--nCount);
    
    return nResult;
}


INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", SoundHandleError);
