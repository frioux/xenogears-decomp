#include "common.h"
#include "system/sound.h"
#include "psyq/kernel.h"
#include "psyq/libspu.h"

s32 SoundCalculateEnvelopeDelta(s32 value, s16 divisor, s16 mode);
s32 SoundEnvelopeStepTriangle(AudioEnvelope* pEnvelope);
s32 SoundEnvelopeStepTriangleHalfRate(AudioEnvelope* pEnvelope);
//----------------------------------------------------------------------------------------------------------------------
// SOUND SYSTEM LIMITS, IDS AND FLAGS
//----------------------------------------------------------------------------------------------------------------------
#define SOUND_EFFECT_CHANNEL_COUNT_MIN      4
#define SOUND_EFFECT_CHANNEL_COUNT_MAX      16
#define SOUND_SPU_MEMORY_SIZE               0x80000 // end of SPU RAM, the address space the block list tiles
#define SOUND_VOLUME_MAX                    0x7FFF  // 16-bit signed volume ceiling
#define SOUND_PAN_CENTER                    0x4000
#define SOUND_PAN_MAX                       0x7F00
#define SOUND_FILE_VERSION                  0x101   // .sed file format version SoundValidateFile expects
#define SOUND_ERR_HEAP_ALLOC_FAILED         0x1E    // SoundHandleError id for a failed sound-heap allocation
// SoundHandleError ids, each one naming the failure its call site has just detected
#define SOUND_ERR_INVALID_MANAGER            0x5     // no usable AudioManager: NULL argument or not in g_SoundAudioManagerListHead
#define SOUND_ERR_INVALID_SEQ_HEADER         0xA     // SoundUnkDebug0 rejected pManager->pSeqHeader
#define SOUND_ERR_INVALID_SEDS               0xB     // SoundValidateFile rejected the file SoundRemoveSedsEntry is removing
#define SOUND_ERR_SEDS_NOT_FOUND             0x10    // the SoundFile is not in g_SoundSedsLinkedList
#define SOUND_ERR_WDS_NOT_FOUND              0x11    // the SoundWDSEntry is not in g_SoundWdsLinkedList
#define SOUND_ERR_WDS_ALREADY_LOADED         0x16    // SoundFindWdsEntry already finds this WDS id
#define SOUND_ERR_SPU_MEMORY_ALLOC_FAILED    0x1F    // SoundSpuMemoryAllocateWDS returned 0
#define SOUND_ERR_REVERB_ALLOC_FAILED        0x20    // the reverb work area block could not be allocated
#define SOUND_ERR_SPU_MEMORY_FREE_FAILED     0x24    // SoundSpuMemoryFreeBlock did not return the address it was handed
#define SOUND_ERR_ALREADY_INITIALIZED        0x28    // bit 15 is already set, as the 0xB801 SoundInitialize writes leaves it
#define SOUND_ERR_NOT_INITIALIZED            0x29    // g_SoundControlFlags is 0, so there is nothing to shut down
#define SOUND_REVERB_MEMORY_NONE            (-1)    // g_SoundReverbMemoryHandle before the work area is allocated
#define SOUND_REVERB_TYPE_NONE              0xFF    // g_SoundReverbType before any reverb type is set
#define SOUND_REVERB_TYPE_NO_CHANGE         (-2)    // SoundSetReverbModeWithAllocation leaves the SPU alone
// SoundSetAudioManagerBackupState modes
#define SOUND_MANAGER_BACKUP_DISCARD        0
#define SOUND_MANAGER_BACKUP_SAVE           1
#define SOUND_MANAGER_BACKUP_RESTORE        2
// g_SoundControlFlags bits (the handler bit 2 is SOUND_CTL_FLAG_IRQ_HANDLER in sound.h)
#define SOUND_CTL_FLAG_EVENT_ENABLED        (1 << 0)  // g_unk_SoundEvent is enabled
#define SOUND_CTL_FLAG_SKIP_DUPLICATE_CHECK (1 << 7)  // SoundAddSedsEntry adds files whose sedId is already in the list
#define SOUND_CTL_FLAG_REVERB_TRANSFER      (1 << 5)  // the reverb work area upload is in progress
#define SOUND_CTL_FLAG_MUTED                (1 << 6)  // every voice silenced, the timer handler does nothing
#define SOUND_CTL_FLAG_SOUND_EFFECT_ENABLED (1 << 11) // gate the SoundPlaySoundEffect* family tests
#define SOUND_CTL_FLAG_REVERB_ENABLED       (1 << 12) // managers apply their reverb settings to the SPU
#define SOUND_CTL_MASK_VOLUME_MODE          ((1 << 8) | (1 << 9) | (1 << 10)) // SoundGetVolumeMode decodes it and reports 0 while it is clear
// AudioManager.flags bits (bit 15 is what SoundAudioManagerIsActive reports)
#define SOUND_MANAGER_FLAG_IN_PLACE         (1 << 14) // manager lives in caller-provided memory, do not free it
#define SOUND_MANAGER_FLAG_PLAYING          (1 << 15)
#define SOUND_MANAGER_FLAG_SEQUENCE_LOADED  (1 << 0)  // pSeqHeader has been read into the manager's fields
#define SOUND_MANAGER_FLAG_BACKUP_SAVED     (1 << 4)  // pBackup holds a snapshot of the manager
#define SOUND_MANAGER_FLAG_STOP_PENDING     (1 << 5)  // stop at stopBar, the elements are not mixed meanwhile
#define SOUND_MANAGER_FLAG_NEEDS_ACTIVATION (1 << 8)  // playback resumes once a non-zero volume arrives
// SoundTransferCommand.commandType, the rest of the family is SOUND_SPU_COMMAND_WRITE/READ in sound.h
#define SOUND_SPU_COMMAND_READ_DECODED_ALL      0x3
#define SOUND_SPU_COMMAND_READ_DECODED_CDONLY   0x4
// AudioElement.active_flag bits (the percussion, hold, rest and instrument bits are SOUND_CHANNEL_* in sound.h)
#define SOUND_CHANNEL_ACTIVE                0x1     // element is playing and gets stepped
#define SOUND_CHANNEL_MUTED                 0x20    // silenced by SoundMuteElementsByFlags
// SoundVoiceData.flags: one bit per SPU voice register group the element scripts rewrite
#define SOUND_VOICE_FLAG_VOLUME             0x1
#define SOUND_VOICE_FLAG_PITCH              0x4
#define SOUND_VOICE_FLAG_ATTACK             0x10
#define SOUND_VOICE_FLAG_DECAY              0x20
#define SOUND_VOICE_FLAG_SUSTAIN            0x40
#define SOUND_VOICE_FLAG_RELEASE            0x80
#define SOUND_VOICE_FLAG_SUSTAIN_LEVEL      0x100
#define SOUND_VOICE_FLAG_ALL                (SOUND_VOICE_FLAG_VOLUME | SOUND_VOICE_FLAG_PITCH | SOUND_VOICE_FLAG_ATTACK | SOUND_VOICE_FLAG_DECAY | SOUND_VOICE_FLAG_SUSTAIN | SOUND_VOICE_FLAG_RELEASE | SOUND_VOICE_FLAG_SUSTAIN_LEVEL)
// AudioEnvelope.flags bits
#define SOUND_ENVELOPE_FLAG_ENABLED         0x1
#define SOUND_ENVELOPE_FLAG_HALF_RATE       0x4     // the reloaded count doubles, the envelope steps every other tick
#define SOUND_ENVELOPE_FLAG_DESCENDING      0x8     // the step handlers negate delta while this is set
#define SOUND_EFFECT_ELEMENT_COUNT          16      // elements of the sound-effect manager SoundInitialize creates
#define SOUND_EFFECT_ELEMENT_PAIR_COUNT     2       // elements one sound-effect pair spans, what g_SoundEffectElementPairCount reports
#define SOUND_PERCUSSION_TABLE_SIZE         0x180   // tail added to a manager's allocation when it carries a percussion map
#define SOUND_TICKS_PER_WHOLE_NOTE          192     // a beat divisor of 4 splits this into 48 ticks per beat
#define SOUND_LOOP_COUNT_NONE               0xFFFF  // "no active element seen": any real loopCount compares lower

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

// SPU / sequencer setup entry point. Callees live further down this unit.
extern char g_SoundSpuMemoryTableStart[];
extern u8 g_SoundHeapStorage[];
extern s32 g_SoundTimerTickCounter;
extern s16 g_SoundCdVolumeFadeActive;
extern u16 g_SoundPendingSpuIrqEnable;
extern s32 g_SoundTimerTicksTotal;
extern s32 g_SoundTimerCyclesElapsed;
// State block reached through g_SoundCdSequencerState (only reader in the binary).
typedef struct {
    u16 active;      // 0x00, bit 0 = sequencer active
    u8 unk2[0x10];   // 0x02
    u16 unk12;       // 0x12
    u8 unk14[0x22];  // 0x14
    s16 unk36;       // 0x36
    s16 unk38;       // 0x38
    s16 unk3A;       // 0x3A
    u8 unk3C[0x26];  // 0x3C
    s16 unk62;       // 0x62
    s16 unk64;       // 0x64
    s16 unk66;       // 0x66
} SoundCdSeqState; // Size: 0x68
extern SoundCdSeqState* g_SoundCdSequencerState;
extern AudioManager* g_SoundEffectAudioManager;
// Script interpreter tables (the bytes live in the exe's .sdata, one entry per command).
typedef u8* (*SoundScriptHandler)(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements);
extern SoundScriptHandler g_SoundScriptHandlers[];
extern u8 g_SoundScriptCommandLengths[]; // bytes of arguments the command carries, indexed by command - 0x80
extern u8 g_SoundNoteDefaultDurations[]; // default duration of a note command in ticks, 0 = the script carries it
extern u8 g_SoundNoteTypePitchOffsets[]; // offset of the note command's pitch from the element's octave

s32 SoundTimerInterruptHandler(void);
void SoundSpuIRQHandler(void);
void SoundOnTransferCallback(void);
void func_800386C4(s32 mode);
void SoundHeapInitialize(void* startAddress, unsigned int size);
void SoundSpuMemoryInitialize(void);
void* SoundHeapAllocate(unsigned int allocSize);
void SoundClearVoiceDataPointers(void);
void SoundSetCdAttr(long reverb, long mix);
void SoundSetMasterVolumeWithFade(s32 volume, s32 frames);
void SoundSetCdVolumeWithFade(s32 targetVolume, s32 fadeFrames);
void SoundSetupCdMix(s32 vol);
int SoundSpuMemoryAllocateBlockAtAddress(int size, int address, int unkArg);
AudioManager* SoundCreateAudioManagerForElementCount(s32 elementCount);
void SoundSetReverbModeWithAllocation(s32 reverbType, s32 reverbDepth, s32 reverbDelay, s32 reverbFeedback);

void SoundInitialize(s32 flags) {
    if (g_SoundControlFlags < 0) {
        SoundHandleError(SOUND_ERR_ALREADY_INITIALIZED);
        return;
    }

    g_SoundControlFlags = flags | 0xB801;
    SpuInitMalloc(4, g_SoundSpuMemoryTableStart);
    SoundHeapInitialize(g_SoundHeapStorage, 0x6300);
    SoundSpuMemoryInitialize();
    g_SoundTransferQueue = SoundHeapAllocate(SOUND_TRANSFER_QUEUE_SIZE * sizeof(SoundTransferCommand));
    SoundClearVoiceDataPointers();
    g_SoundRandomSeed = 0x12345678;
    g_SoundAudioManagerListHead = NULL;
    g_SoundEffectAudioManager = NULL;
    g_SoundSedsLinkedList = NULL;
    g_SoundWdsLinkedList = NULL;
    g_SoundCdSequencerState = NULL;
    g_SoundKeyOnFlags = 0;
    g_SoundKeyOffFlags = 0;
    g_unk_VoicesNeedingProcessing = 0;
    g_SoundVolumeController.commonAttr.mvolmode.left = 0;
    g_SoundVolumeController.commonAttr.mvolmode.right = 0;
    g_SoundVolumeController.commonAttr.mask = SPU_COMMON_MVOLMODEL | SPU_COMMON_MVOLMODER;

    EnterCriticalSection();
    g_unk_SoundEvent = OpenEvent(RCntCNT2, EvSpINT, RCntMdINTR, SoundTimerInterruptHandler);
    SetRCnt(RCntCNT2, 0x44E8, RCntMdINTR);
    StartRCnt(RCntCNT2);
    SpuSetTransferCallback(&SoundOnTransferCallback);
    SpuSetIRQCallback(&SoundSpuIRQHandler);
    SpuSetIRQ(0);
    g_SoundTimerTickCounter = 0;
    g_SoundSpuIRQCount = 0;
    ExitCriticalSection();

    SoundSpuMemoryAllocateBlockAtAddress(0x2000, 0x10000, 4);
    func_800386C4(1);
    SoundSetCdAttr(0, 1);
    SoundSetMasterVolumeWithFade(0x3FFF, 0);
    SoundSetCdVolumeWithFade(0x7FFF, 0);
    if (g_SoundControlFlags & 0x4000) {
        SoundSetupCdMix(0x80);
    }

    g_SoundEffectAudioManager = SoundCreateAudioManagerForElementCount(SOUND_EFFECT_ELEMENT_COUNT);
    g_SoundSoundEffectChannelCount = 8;
    g_SoundReverbMemoryHandle = SOUND_REVERB_MEMORY_NONE;
    g_SoundUploadDestBuffer = 0;
    g_SoundReverbType = SOUND_REVERB_TYPE_NONE;
    SoundSetReverbModeWithAllocation(SPU_REV_MODE_STUDIO_C, 0, 0, 0);
    SpuSetReverb(1);
    g_SoundSpuErrorId = 0;
}

void SoundReset(void) {
    int i;

    if (g_SoundControlFlags == 0x0) {
        SoundHandleError(SOUND_ERR_NOT_INITIALIZED);
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
            (*pChannel)->flags |= SOUND_VOICE_FLAG_ALL;
        }
        i++;
        pChannel++;
    } while (i < NUM_VOICES);

    g_SoundControlFlags &= ~SOUND_CTL_FLAG_MUTED;
}

void SoundMuteAllSpuChannels(void) {
    SPU_VOICE_REG* pVoice = g_pSoundSpuRegisters->_rxx.voice;
    s32 i;

    g_SoundControlFlags |= SOUND_CTL_FLAG_MUTED;
    for (i = 0; i < NUM_VOICES; pVoice++, i++) {
        pVoice->volume.left = 0;
        pVoice->volume.right = 0;
        pVoice->pitch = 0;
        pVoice->adsr[0] = (pVoice->adsr[0] & 0xFF) + 0x7F00;
        pVoice->adsr[1] = 0x1FDF;
    }
}

void SoundEnableEvent(void) {
    if (!(g_SoundControlFlags & SOUND_CTL_FLAG_EVENT_ENABLED)) {
        g_SoundControlFlags |= SOUND_CTL_FLAG_EVENT_ENABLED;
        EnableEvent(g_unk_SoundEvent);
    }
}

void SoundDisableEvent(void) {
    if (g_SoundControlFlags & SOUND_CTL_FLAG_EVENT_ENABLED) {
        DisableEvent(g_unk_SoundEvent);
        g_SoundControlFlags &= ~SOUND_CTL_FLAG_EVENT_ENABLED;
    }
}



SoundWDSEntry* SoundLoadWdsFile(SoundWDSEntry* pWdsFile, void* pUnk) {
    SoundWDSEntry* pEntry;
    SoundWDSEntry** ppLink;
    int spuAddress;

    spuAddress = SoundSpuMemoryAllocateWDS(pWdsFile);
    if (spuAddress == 0) {
        SoundHandleError(SOUND_ERR_SPU_MEMORY_ALLOC_FAILED);
        return NULL;
    }

    // the adpcm payload sits at a runtime byte offset in the file image, so it cannot be a named field
    SoundQueueSpuWriteCommand(spuAddress, (u8*)pWdsFile + pWdsFile->adpcmDataOffset, pWdsFile->adpcmDataSize, NULL);

    pEntry = (SoundWDSEntry*)SoundHeapAllocateFromEnd(pWdsFile->headerSizeMby);
    if (pEntry == NULL) {
        SoundSpuMemoryFreeBlock(spuAddress);
        SoundHandleError(SOUND_ERR_HEAP_ALLOC_FAILED);
        return NULL;
    }

    SoundHeapSetBlockMemory(pEntry, pWdsFile, pWdsFile->headerSizeMby);
    pEntry->spuMemoryAddress = spuAddress;

    DisableEvent(g_unk_SoundEvent);
    ppLink = &g_SoundWdsLinkedList;
    while (*ppLink != NULL) {
        ppLink = &(*ppLink)->pNext;
    }
    *ppLink = pEntry;
    pEntry->pNext = NULL;
    EnableEvent(g_unk_SoundEvent);

    return pEntry;
}

// Loads part of a WDS file, basically a sized SoundLoadWdsFile?
SoundWDSEntry* SoundLoadPartialWdsFile(SoundWDSEntry* pWdsFile, int size, int mode) {
    SoundWDSEntry* pEntry;
    SoundWDSEntry** ppLink;
    int spuAddress;

    if (SoundFindWdsEntry(pWdsFile->id) != 0) {
        SoundHandleError(SOUND_ERR_WDS_ALREADY_LOADED);
        return NULL;
    }

    spuAddress = SoundSpuMemoryAllocateWDS(pWdsFile, mode);
    if (spuAddress == 0) {
        SoundHandleError(SOUND_ERR_SPU_MEMORY_ALLOC_FAILED);
        return NULL;
    }

    SoundWdsSetTransferParamters(spuAddress, pWdsFile->adpcmDataSize);
    // same runtime payload offset as SoundLoadWdsFile
    SoundTransferWdsPart((u8*)pWdsFile + pWdsFile->adpcmDataOffset, size - pWdsFile->headerSizeMby);

    pEntry = (SoundWDSEntry*)SoundHeapAllocateFromEnd(pWdsFile->headerSizeMby);
    if (pEntry == NULL) {
        SoundSpuMemoryFreeBlock(spuAddress);
        SoundHandleError(SOUND_ERR_HEAP_ALLOC_FAILED);
        return NULL;
    }

    SoundHeapSetBlockMemory(pEntry, pWdsFile, pWdsFile->headerSizeMby);
    pEntry->spuMemoryAddress = spuAddress;

    DisableEvent(g_unk_SoundEvent);
    ppLink = &g_SoundWdsLinkedList;
    while (*ppLink != NULL) {
        ppLink = &(*ppLink)->pNext;
    }
    *ppLink = pEntry;
    pEntry->pNext = NULL;
    EnableEvent(g_unk_SoundEvent);

    return pEntry;
}

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

int SoundTransferWdsPart(void* pBuffer, int size) {
    int nRemaining;
    int nBytes;
    u32 spuAddress;

    nRemaining = g_SoundWdsRemainingBytes;
    if (nRemaining == 0) {
        return 0;
    }

    nBytes = nRemaining;
    if (size < nRemaining) {
        nBytes = size;
    }
    size = nBytes;
    spuAddress = g_SoundWdsCurSpuAddress;
    SoundQueueSpuWriteCommand(spuAddress, pBuffer, size, NULL);
    g_SoundWdsCurSpuAddress = spuAddress + size;
    g_SoundWdsRemainingBytes = nRemaining - size;

    return nRemaining - size;
}

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
        SoundHandleError(SOUND_ERR_WDS_NOT_FOUND);
        return;
    }

    DisableEvent(g_unk_SoundEvent);
    if (pPrev != NULL)
        pPrev->pNext = pTargetEntry->pNext;
    else 
        g_SoundWdsLinkedList = pTargetEntry->pNext;
    EnableEvent(g_unk_SoundEvent);
    
    if (pTargetEntry->spuMemoryAddress != SoundSpuMemoryFreeBlock(pTargetEntry->spuMemoryAddress)) {
        SoundHandleError(SOUND_ERR_SPU_MEMORY_FREE_FAILED);
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
    if (!(g_SoundControlFlags & SOUND_CTL_FLAG_SKIP_DUPLICATE_CHECK)) {
        for (pEntry = g_SoundSedsLinkedList; pEntry != NULL; pEntry = pEntry->pNext) {
            if (pSoundFile->sedId == pEntry->sedId) {
                SoundHandleError(SOUND_ERR_ENTRY_ALREADY_EXISTS);
                return;
            }
        }
    }

    // Validate SEDS File
    nSedsStatus = SoundValidateFile(pSoundFileToVerify, FILE_SIGNATURE('s','e','d','s'), SOUND_FILE_VERSION);
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


void SoundRemoveSedsEntry(SoundFile* pSoundFile) {
    SoundFile* pCurrent = g_SoundSedsLinkedList;
    SoundFile* pPrev = NULL;
    SoundFile* pTarget = pSoundFile;
    short nSedsStatus;

    for (; pCurrent != NULL; pCurrent = pCurrent->pNext) {
        if (pCurrent == pTarget) break;
        pPrev = pCurrent;
    }

    if (pCurrent == NULL) {
        SoundHandleError(SOUND_ERR_SEDS_NOT_FOUND);
        return;
    }

    SoundStopElementsForSoundFile(pSoundFile);
    DisableEvent(g_unk_SoundEvent);

    if (pPrev != NULL) {
        pPrev->pNext = pTarget->pNext;
    } else {
        g_SoundSedsLinkedList = pTarget->pNext;
    }
    pTarget->pNext = NULL;

    nSedsStatus = SoundValidateFile(pSoundFile, FILE_SIGNATURE('s', 'e', 'd', 's'), SOUND_FILE_VERSION);
    if (nSedsStatus != SOUND_STATUS_OK) {
        SoundHandleError(SOUND_ERR_INVALID_SEDS);
        return;
    }

    EnableEvent(g_unk_SoundEvent);
}

//----------------------------------------------------------------------------------------------------------------------
void SoundClearSedsList(void) {
    SoundStopAllElements();
    g_SoundSedsLinkedList = 0;
}

//----------------------------------------------------------------------------------------------------------------------
SoundFile* SoundFindSedsEntry(SoundFile* pSoundFile, s16 sedId) {
    SoundFile* pEntry;

    if (pSoundFile != NULL) {
        sedId = pSoundFile->sedId;
    }

    pEntry = g_SoundSedsLinkedList;
    while (pEntry != NULL && sedId != pEntry->sedId) {
        pEntry = pEntry->pNext;
    }

    return pEntry;
}

//----------------------------------------------------------------------------------------------------------------------
void SoundStopAllPlayback(void) {
    SoundStopAllAudioManagers();
    SoundStopAllElements();
}

//----------------------------------------------------------------------------------------------------------------------
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_800386C4);
/*
Attempt (97.73%, retail's exact 352 bytes). Everything matches except ONE instruction's position:
retail is `sh s0,0x66(s1); li v0,1; sh v0,0x36(s1); sh v0,0x62(s1)`, ours puts the `li` first (the
constant is materialised at the head of the block). Screened: gcc-2.7.2-cdk / gcc-2.7.2 / this
unit's gcc-2.6.0 = 93.18 / 97.67 / 97.73, so the preset is not the answer either.
Measured dead on the `li`: source statement order in both directions (0x36/0x62 last is 97.58),
`unk62 = unk36 = 1;`, a `u16` local assigned 1 AFTER the 0x66 store, a `do { } while (0);` around
the flag pair, around the 0x66 store alone and around all three stores, and a volatile store on
unk66 (97.73 all). Duplicating the three stores in BOTH volume arms (so cross-jumping forms the
tail) does give retail's store order but moves the `li` into the beq delay slot instead. A 4-minute
PERMUTER_NONE run (1811 iterations, base=60 best=60) found nothing, and cc1_probe shows cc1 itself
hoists the constant, so this is the STORE_HOIST class. The switch tree needs `case 0: break;` (with
only cases 1-3 gcc pivots on 2), and volume must be shifted into itself (`volume <<= 7/6`) so only
one sll is emitted.
void func_800386C4(s32 mode) {
    AudioManager* pManager;
    SoundCdSeqState* pState;
    u16 volume;

    g_SoundControlFlags &= ~0x700;

    switch (mode) {
        case 0:
            break;
        case 1:
            g_SoundControlFlags |= 0x100;
            break;
        case 2:
            g_SoundControlFlags |= 0x300;
            break;
        case 3:
            g_SoundControlFlags |= 0x500;
            break;
    }

    SoundApplyVolumeSettings();
    SpuSetReverbModeDepth(g_SoundReverbDepth.left, g_SoundReverbDepth.right);

    pManager = g_SoundAudioManagerListHead;
    while (pManager != NULL) {
        unk_SoundSetFlagsOnActiveVoices(0x100, pManager);
        pManager = pManager->next;
    }

    if (g_SoundControlFlags & 0x4000) {
        SoundSetupCdMix(g_SoundVolumeController.cdMixVolume);
    }

    pState = g_SoundCdSequencerState;
    if (pState != NULL) {
        if (pState->active & 0x1) {
            volume = pState->unk12;
            if (SoundGetVolumeMode() != 0) {
                volume <<= 7;
                pState->unk38 = volume;
                pState->unk3A = 0;
                pState->unk64 = 0;
            } else {
                volume <<= 6;
                pState->unk38 = volume;
                pState->unk3A = volume;
                pState->unk64 = volume;
            }
            pState->unk66 = volume;
            pState->unk62 = pState->unk36 = 1;
        }
    }
}
*/

//----------------------------------------------------------------------------------------------------------------------
s32 SoundGetVolumeMode(void) {
    s32 out;

    if (g_SoundControlFlags & SOUND_CTL_MASK_VOLUME_MODE) {
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

    g_SoundVolumeController.cdMixVolume = vol;

    if (g_SoundControlFlags & SOUND_CTL_MASK_VOLUME_MODE) {
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
void SoundSetReverbEnabled(s32 arg0) {

    if (arg0 != 0) {
        g_SoundControlFlags |= SOUND_CTL_FLAG_REVERB_ENABLED;
    } else {
        g_SoundControlFlags &= ~SOUND_CTL_FLAG_REVERB_ENABLED;
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSetAudioManagerFlag1(AudioManager* manager, s32 bIn) {
    if (bIn != 0) {
        manager->flags &= ~(1 << 0);
    } else {
        manager->flags |= (1 << 0);
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

    if (reverbType == SOUND_REVERB_TYPE_NO_CHANGE) {
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

        if (g_SoundReverbMemoryHandle != SOUND_REVERB_MEMORY_NONE) {
            SoundSpuMemoryFreeBlock(g_SoundReverbMemoryHandle);
        }

        workAreaSize = g_ReverbWorkAreaSizes[reverbType];
        workAreaStartAddr = workAreaSize;
        allocationSize = SOUND_SPU_MEMORY_SIZE - workAreaSize;

        memoryHandle = SoundSpuMemoryAllocateBlockAtAddress(workAreaStartAddr, allocationSize, 5);
        g_SoundReverbMemoryHandle = memoryHandle;

        if (memoryHandle == 0) {
            SoundHandleError(SOUND_ERR_REVERB_ALLOC_FAILED);
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
            SoundHandleError(SOUND_ERR_HEAP_ALLOC_FAILED);
        }
    }
    g_SoundControlFlags |= SOUND_CTL_FLAG_REVERB_TRANSFER;
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

        g_SoundControlFlags &= ~SOUND_CTL_FLAG_REVERB_TRANSFER;

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
// SoundHeapAllocate, but slightly different
void* SoundHeapAllocateFromEnd(int size) {
    SoundHeapBlockHeader* pHeapBlock;
    SoundHeapBlockHeader* pTargetBlock;
    SoundHeapBlockHeader* pNewBlock;
    SoundHeapBlockHeader* pNext;
    void* pMemory;
    int nAvailable;
    int nTotalSize;
    int nSize;

    DisableEvent(g_unk_SoundEvent);
    nTotalSize = ((size + 0xF) & ~0xF) + sizeof(SoundHeapBlockHeader);
    pTargetBlock = NULL;
    nAvailable = 0;
    pHeapBlock = g_SoundHeapHead;

    while (1) {
        if (pHeapBlock->pNext == NULL) {
            nSize = g_SoundHeapEnd - (u32)pHeapBlock->pPrev;
            if (nSize < nTotalSize) {
                break;
            }
            pTargetBlock = pHeapBlock;
            nAvailable = g_SoundHeapEnd;
            break;
        }

        pNext = pHeapBlock->pNext;
        nSize = (u32)pHeapBlock->pNext - (u32)pHeapBlock->pPrev;
        if (nSize >= nTotalSize) {
            pTargetBlock = pHeapBlock;
            nAvailable = (u32)pNext;
        }

        pHeapBlock = pHeapBlock->pNext;
    }

    if (pTargetBlock == NULL) {
        return NULL;
    }

    nAvailable -= nTotalSize;
    pNewBlock = (SoundHeapBlockHeader*)((nAvailable + 0xF) & ~0xF);
    pMemory = pNewBlock + 1;
    pNewBlock->pPrev = (SoundHeapBlockHeader*)((u32)pMemory + size);
    pNewBlock->pNext = NULL;
    pNewBlock->unk4 = 0;
    pNewBlock->unk0 = 2;
    pNewBlock->unk2 = 0;
    pNewBlock->pNext = pTargetBlock->pNext;
    pTargetBlock->pNext = pNewBlock;
    EnableEvent(g_unk_SoundEvent);
    SoundHeapClearBlockMemory(pMemory, size);

    return pMemory;
}

void SoundHeapFree(void* pMemory) {
    SoundHeapBlockHeader* pBlock;
    SoundHeapBlockHeader* pCurrent = g_SoundHeapHead;
    SoundHeapBlockHeader* pPrev;
    SoundHeapBlockHeader* pEntry;

    DisableEvent(g_unk_SoundEvent);
    pBlock = (SoundHeapBlockHeader*)pMemory - 1;
    pEntry = pCurrent;
    pPrev = NULL;
    for (; pEntry != pBlock; pEntry = pEntry->pNext) {
        pPrev = pEntry;
    }
    pCurrent = pPrev;
    if (pCurrent != NULL) {
        pPrev->pNext = pBlock->pNext;
    }
    EnableEvent(g_unk_SoundEvent);
}

u32 SoundHeapGetLargestFreeBlockSize(void) {
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
    g_SoundSpuMemoryBlocks[0].type = 5;
    g_SoundSpuMemoryBlocks[0].spuAddress = 0;
    g_SoundSpuMemoryBlocks[0].size = 0x1010;
    g_SoundSpuMemoryBlocks[0].nextBlockIndex = 0;
}


INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", SoundSpuMemoryAllocateBlock);
/*
Attempt (94.58%, 256 bytes = retail's size). Residual, in the loop head only: retail
REMATERIALISES the array base every iteration (`lh v0,2(s1); lui %hi; addiu %lo; sll; addu`)
while gcc 2.6 hoists one copy into a2 in the preheader (`move a2,s0` plus a nop in the lh's
load-delay slot), and therefore also swaps the two callee-saved pointers (here address s1,
pPrevBlock s0; retail address s0, pPrevBlock s1). Tried, all 93-94.6%: an index local,
`(u8*)base + (i<<4)`, `new_var = pPrevBlock` used as the index (that one fixes the registers
but folds the base into s1, 244 bytes), a do/while round the loop body (93.25), declaration
order, and -O1/-O3/-G0/cdk screens (all worse). loop.c hoists the base here; the sibling
SoundSpuMemoryAllocateBlockFit shows the same hoist and matches, so the hoist itself is not
the blocker - retail's source must defeat it somehow.
Round 315 added, all still 94.58 or worse: `(u8*)g_SoundSpuMemoryBlocks + (idx << 4)` cast
form (identical to the subscript), local declaration order both ways (no change), the
while (1) { if (nextBlockIndex == 0) break; } form (84.06), a `short nextIndex` assigned in
the condition (82.58, kills retail's duplicated lh), and a 4-minute permuter run
(486 -> 355) whose only improvement is WRONG CODE - it hoists `short new_var =
pPrevBlock->nextBlockIndex;` above the loop and indexes with the fixed value, so the loop
would never advance. That candidate does say what gcc wants: a live short in a register.
ROUND 316 PROVED THE HOIST IS NOT SOURCE-CONTROLLABLE (nine spellings, cc1_probe.sh, every
one hoists the base into the preheader as `la $16` plus `move $6,$16` with a nop in the lh's
load-delay slot): `&Arr[i]`, `Arr + i`, `(u8*)Arr + (i << 4)`, `(u8*)Arr + i * sizeof(T)`,
`&Arr[(u8)i]`, a base local assigned inside the body, the same local live across the whole
function (that one only moves the copy into a callee-saved $19 and turns the addu
base-first, so it is worse), the unbounded-array extern vs the header's array declaration,
and gcc-2.7.2-cdk (`move $7,$16`, same hoist). Padding the loop body with 1/3/6/12 extra
global increments did not stop it either, so round 113's move_movables cost threshold is not
what we hit here: for a body this small the invariant is always moved. Searching every
matched function in all eleven binaries for an address formed inside a loop found exactly
one family where retail re-forms it (field/misc9 FieldInitScreenStripSprites), and there the
loop contains jal calls and the re-formed `la` sits in a block AFTER them, where cse2 cannot
forward the callee-saved copy; this loop has no call, so that route does not exist. Treat the
remaining delta as a compiler heuristic, not a missing spelling: the sibling
SoundSpuMemoryAllocateBlockFit hoists the same way and matches because retail does too, so
move_movables is running normally here and retail's own Fit object shows the identical
hoisted-copy shape (`addu t0,s0,zero` in its preheader). Whatever defeated the hoist in
Block's original source is therefore not how the base is spelled (nine measured above), so
it is the loop or its exits - that is the next lead, not another address spelling.
    // Allocates a block of SPU memory of the given size, returns the SPU address or 0 on failure.
    int SoundSpuMemoryAllocateBlock(int size, int unkArg) {
        SoundSpuMemoryBlock* pPrevBlock;
        int address;
        SoundSpuMemoryBlock* pBlock;
        int index;
    
        pPrevBlock = &g_SoundSpuMemoryBlocks[0];
        address = pPrevBlock->spuAddress + pPrevBlock->size;
    
        while (pPrevBlock->nextBlockIndex != 0) {
            pBlock = (SoundSpuMemoryBlock *)((u8 *)g_SoundSpuMemoryBlocks + (pPrevBlock->nextBlockIndex << 4));
            if (pBlock->spuAddress - address >= size) {
                goto allocate;
            }
            pPrevBlock = pBlock;
            address = pBlock->spuAddress + pBlock->size;
        }
    
        if (SOUND_SPU_MEMORY_SIZE - address < size) {
            return 0;
        }
    
    allocate:
        index = SoundSpuMemoryGetFreeBlock();
        if (index < 0) {
            return 0;
        }
    
        pBlock = &g_SoundSpuMemoryBlocks[index];
        pBlock->flags = SPU_MEMORY_IN_USE;
        pBlock->type = 0;
        pBlock->spuAddress = address;
        pBlock->size = size;
        pBlock->nextBlockIndex = pPrevBlock->nextBlockIndex;
        pPrevBlock->nextBlockIndex = index;
    
        return address;
    }
*/


int SoundSpuMemoryAllocateBlockFit(int size) {
    SoundSpuMemoryBlock* pBlock;
    SoundSpuMemoryBlock* pNextBlock;
    SoundSpuMemoryBlock* pFitBlock;
    int address;
    int allocAddress;
    int index;

    pBlock = &g_SoundSpuMemoryBlocks[0];
    pFitBlock = NULL;
    while (1) {
        address = pBlock->spuAddress + pBlock->size;
        if (pBlock->nextBlockIndex == 0) {
            if (SOUND_SPU_MEMORY_SIZE - address >= size) {
                pFitBlock = pBlock;
                allocAddress = SOUND_SPU_MEMORY_SIZE - size;
            }
            break;
        }
        pNextBlock = &g_SoundSpuMemoryBlocks[pBlock->nextBlockIndex];
        if (pNextBlock->spuAddress - address >= size) {
            pFitBlock = pBlock;
            allocAddress = pNextBlock->spuAddress - size;
        }
        pBlock = pNextBlock;
    }

    if (pFitBlock == NULL) {
        return 0;
    }

    index = SoundSpuMemoryGetFreeBlock();
    if (index < 0) {
        return 0;
    }

    // Reuses pNextBlock: a second pointer local there puts the stores through lui/addiu.
    pNextBlock = &g_SoundSpuMemoryBlocks[index];
    pNextBlock->flags = SPU_MEMORY_IN_USE;
    pNextBlock->type = 0;
    pNextBlock->spuAddress = allocAddress;
    pNextBlock->size = size;
    pNextBlock->nextBlockIndex = pBlock->nextBlockIndex;
    pBlock->nextBlockIndex = index;

    return allocAddress;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", SoundSpuMemoryAllocateBlockAtAddress);
/*
ROUND 333: 88.92% at retail's exact 292 bytes with the body below. The loop head, the back
edge (`slt v0,v1,s1; bnez v0,head; move s0,a0` in the delay slot) and the alloc tail are now
instruction for instruction identical to retail, and the two `break` arms sit where retail
puts them: a plain `while (pPrevBlock->spuAddress < address)` (which gcc rotates into the top
guard plus the bottom test) is what produces that, where the old while(1)+continue and the
do-while both moved the arms. Residual: only the two top `return 0` blocks. Retail branches
when the test is true (`slt v0,a2,s2; bnez v0,epilogue; move v0,zero`) and jumps for the
second (`beqz v0,alloc; move v0,zero; j epilogue; nop`) - both returns cross-jumped into one
out-of-line v0=0 plus the epilogue; gcc 2.6 lays the first return out as the fall-through
(`beqz` + `j`) and moves the second test below the two break arms, same instruction count and
the same 292 bytes. Tried this round: nested ifs with one trailing `return 0` (80.14, it also
moves the break arms to the end), `goto fail` + one `fail: return 0;` (same), an empty
do{}while(0) after each return (no change). Next lead is the failure-block layout, not the
tests. Earlier rounds:
Attempt (87.00%, 296 bytes against retail's 292). The whole preheader matches retail
instruction for instruction (base la in s0, size s2, address s1, nSpace a2=0,
freeAddress a1, endAddress a3) and so does the alloc tail; the residual is the loop:
retail is a do-while whose bottom test is `slt v0,v1,s1; bnez v0,head; addu s0,a0` (the
pPrevBlock = pBlock assignment in the back edge's delay slot, so pPrevBlock is s0 and
address s1), while every spelling here either rotates the loop (do-while: gcc hoists
the top `nextBlockIndex == 0` test into the preheader and runs the body bottom-tested,
72-73%) or needs an extra `j` back to the head (while(1)+continue, 87.00%, and the two
callee-saved pointers come out address s0 / pPrevBlock s1 - the reverse of retail).
Hand shapes tried: do-while, do-while with an index local, while(1)+continue (87.00),
the nested if/while of the previous note (87.00); permuter 4 min: 1079 -> 651, best
edit only wrapped a `return 0` in do{}while(0), so no.
int SoundSpuMemoryAllocateBlockAtAddress(int size, int address, int unkArg) {
    int nSpace = 0;
    SoundSpuMemoryBlock* pPrevBlock = g_SoundSpuMemoryBlocks;
    SoundSpuMemoryBlock* pBlock;
    int freeAddress = pPrevBlock->spuAddress + pPrevBlock->size;
    int endAddress = address + size;
    int index;

    while (pPrevBlock->spuAddress < address) {
        if (pPrevBlock->nextBlockIndex == 0) {
            nSpace = SOUND_SPU_MEMORY_SIZE - freeAddress;
            break;
        }
        pBlock = &g_SoundSpuMemoryBlocks[pPrevBlock->nextBlockIndex];
        if (pBlock->spuAddress >= endAddress) {
            nSpace = pBlock->spuAddress - freeAddress;
            break;
        }
        freeAddress = pBlock->spuAddress + pBlock->size;
        pPrevBlock = pBlock;
    }

    if (nSpace < size) {
        return 0;
    }
    if (address < freeAddress) {
        return 0;
    }

    index = SoundSpuMemoryGetFreeBlock();
    if (index < 0) {
        return 0;
    }

    pBlock = &g_SoundSpuMemoryBlocks[index];
    pBlock->flags = SPU_MEMORY_IN_USE;
    pBlock->type = 0;
    pBlock->spuAddress = address;
    pBlock->size = size;
    pBlock->nextBlockIndex = pPrevBlock->nextBlockIndex;
    pPrevBlock->nextBlockIndex = index;

    return address;
}
*/


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
            pCurBlock->type = 0;
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

void SoundSpuMemorySetBlockType(int targetAddress, u_char value) {
    SoundSpuMemoryBlock *pBlock;

    pBlock = SoundSpuMemoryFindBlock(targetAddress);
    if (pBlock != NULL) {
        pBlock->type = value;
    }
}

s32 SoundUnkDebug1(void) { return 0; }

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

AudioManager* SoundCreateAndPlayAudioManager(SoundFile* pFile, s32 arg1, s32 arg2) {
    AudioManager* pManager = SoundCreateAudioManagerOnHeap(pFile);

    SoundPlayAudioManager(pManager, arg1, arg2);
    return pManager;
}

AudioManager* SoundCreateAudioManagerOnHeap(AudioManager* pManager) {
    AudioManager* pSource = pManager;
    AudioManager* pNew;
    s16 status;
    s32 size;

    status = (s16)SoundUnkDebug0(pManager);
    if (status != 0) {
        SoundHandleError(status);
        return NULL;
    }

    size = SoundCalculateAudioManagerSize(pManager->elementCount);
    if (pManager->percussionCount != 0) {
        size += SOUND_PERCUSSION_TABLE_SIZE;
    }

    pNew = SoundHeapAllocate(size);
    if (pNew == NULL) {
        SoundHandleError(SOUND_ERR_HEAP_ALLOC_FAILED);
        return NULL;
    }

    pNew->pSeqHeader = (s32)pSource;
    if (pSource->percussionCount != 0) {
        SoundCopyPercussionTable(pNew, pSource);
    }
    SoundLoadAudioManagerSequenceHeader(pNew);
    func_8003B424(pNew);
    pNew->muteFlags = 0;
    SoundAddAudioManagerToList(pNew);

    return pNew;
}

AudioManager* SoundCreateAudioManagerInPlace(AudioManager* pManager, AudioManager* pNew) {
    s16 status;
    s32 size;

    status = (s16)SoundUnkDebug0(pManager);
    if (status != 0) {
        SoundHandleError(status);
        return NULL;
    }

    size = SoundCalculateAudioManagerSize(pManager->elementCount);
    if (pManager->percussionCount != 0) {
        size += SOUND_PERCUSSION_TABLE_SIZE;
    }
    SoundHeapClearBlockMemory(pNew, size);

    pNew->pSeqHeader = (s32)pManager;
    if (pManager->percussionCount != 0) {
        SoundCopyPercussionTable(pNew, pManager);
    }
    SoundLoadAudioManagerSequenceHeader(pNew);
    func_8003B424(pNew);
    pNew->muteFlags = 0;
    SoundAddAudioManagerToList(pNew);
    pNew->flags |= SOUND_MANAGER_FLAG_IN_PLACE;

    return pNew;
}

void SoundDestroyAudioManager(AudioManager* pManager) {
    if (pManager->flags & SOUND_MANAGER_FLAG_PLAYING) {
        SoundStopAudioManager(pManager);
    }

    if ((s16)SoundUnkDebug0(pManager->pSeqHeader) != 0) {
        SoundHandleError(SOUND_ERR_INVALID_SEQ_HEADER);
        return;
    }

    if (SoundRemoveAudioManagerFromList(pManager) != 0) {
        SoundHandleError(SOUND_ERR_INVALID_MANAGER);
        return;
    }

    SoundFreeAudioManagerBackups(pManager);

    if (!(pManager->flags & SOUND_MANAGER_FLAG_IN_PLACE)) {
        SoundHeapFree(pManager);
    }
}

void SoundPlayAudioManager(AudioManager* pManager, s32 volume, s32 frames) {
    s16 status;

    if (pManager == NULL) {
        SoundHandleError(SOUND_ERR_INVALID_MANAGER);
        return;
    }

    pManager->flags &= ~SOUND_MANAGER_FLAG_PLAYING;
    status = (s16)SoundUnkDebug0(pManager->pSeqHeader);
    if (status != 0) {
        SoundHandleError(SOUND_ERR_INVALID_SEQ_HEADER);
        return;
    }

    if (pManager->flags & SOUND_MANAGER_FLAG_PLAYING) {
        SoundStopAudioManager(pManager);
    }

    DisableEvent(g_unk_SoundEvent);
    SoundLoadAudioManagerSequenceHeader(pManager);
    func_8003B424(pManager);
    pManager->volumeInterpolator.currentValue = 0;
    SoundSetAudioManagerVolumeWithFade(pManager, volume, frames);

    pManager->flags |= SOUND_MANAGER_FLAG_PLAYING;
    EnableEvent(g_unk_SoundEvent);
}

void SoundReloadAudioManagerSamples(AudioManager* pManager, s32 volume, s32 frames) {
    AudioElement* pElement;
    SoundWDSEntry* pWds;
    SoundWdsTone* pTone;
    u32 sampleStart;
    s32 cnt;

    if (pManager == NULL) {
        SoundHandleError(SOUND_ERR_INVALID_MANAGER);
        return;
    }

    pElement = &pManager->elements[0];
    cnt = pManager->elementCount;
    do {
        cnt--;
        pWds = SoundFindWdsEntry(pElement->sndId);
        pElement->pWds = (u32)pWds;
        pTone = &pWds->tones[pElement->toneIndex];
        sampleStart = pTone->sampleOffset << 3;
        pElement->voice_data.startAddress = sampleStart + pWds->spuMemoryAddress;
        pElement->voice_data.loopAddress = sampleStart + (pTone->loopOffset << 3);
        pElement->voice_data.flags = 0xFFFF;
        pElement++;
    } while (cnt);

    pManager->volumeInterpolator.currentValue = 0;
    pManager->flags |= SOUND_MANAGER_FLAG_NEEDS_ACTIVATION;
    SoundSetAudioManagerVolumeWithFade(pManager, volume, frames);
}

//----------------------------------------------------------------------------------------------------------------------
void SoundStopAudioManager(AudioManager* manager) {
    if (manager == NULL) {
        SoundHandleError(SOUND_ERR_INVALID_MANAGER);
        return;
    }
    manager->flags &= ~SOUND_MANAGER_FLAG_PLAYING;
    SoundReleaseAllVoices();
}

//----------------------------------------------------------------------------------------------------------------------
void SoundFadeOutAudioManager(AudioManager* manager, s32 arg1) {
    if (manager == NULL) {
        SoundHandleError(SOUND_ERR_INVALID_MANAGER);
        return;
    }
    SoundSetAudioManagerVolumeWithFade(manager, 0, arg1);
}

//----------------------------------------------------------------------------------------------------------------------
void SoundStopAllAudioManagers(void) {
    AudioManager* pManager;

    pManager = g_SoundAudioManagerListHead;
    if (pManager != NULL) {
        do {
            if (pManager->flags & 1) {
                pManager->flags &= ~SOUND_MANAGER_FLAG_PLAYING;
                SoundReleaseAllVoices(pManager);
            }
            pManager = pManager->next;
        } while (pManager != NULL);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SoundNop(void) {}

//----------------------------------------------------------------------------------------------------------------------
void SoundSetControlFlag800(s32 bIn) {
    if (bIn != 0) {
        g_SoundControlFlags |= SOUND_CTL_FLAG_SOUND_EFFECT_ENABLED;
    } else {
        SoundStopAllElements();
        g_SoundControlFlags &= ~SOUND_CTL_FLAG_SOUND_EFFECT_ENABLED;
    }
}

s32 SoundSetSoundEffectChannelCount(s32 count) {
    if (count) {
        if (count > SOUND_EFFECT_CHANNEL_COUNT_MAX) {
            count = SOUND_EFFECT_CHANNEL_COUNT_MAX;
        }
        if (count < SOUND_EFFECT_CHANNEL_COUNT_MIN) {
            count = SOUND_EFFECT_CHANNEL_COUNT_MIN;
        }
        g_SoundSoundEffectChannelCount = count & 0xFE;
    }

    return g_SoundSoundEffectChannelCount;
}

void SoundPlaySoundEffectOnLastPair(s32 arg0) {
    if (g_SoundControlFlags & SOUND_CTL_FLAG_SOUND_EFFECT_ENABLED) {
        s32 value = g_SoundEffectElementCount;

        value -= 2;
        g_SoundEffectElementPairCount = SOUND_EFFECT_ELEMENT_PAIR_COUNT;
        func_8003B644((s16)(value | (s16)0x8000), arg0, 0x6000, 0x4000);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Wowoweewa flag central
void SoundSendCommandConstant(s32 arg0) {
    if (g_SoundControlFlags & SOUND_CTL_FLAG_SOUND_EFFECT_ENABLED) {
        g_SoundEffectElementPairCount = SOUND_EFFECT_ELEMENT_PAIR_COUNT;
        func_8003B644((1 << 2) | (1 << 3) | (1 << 13) | (1 << 14), arg0, (1 << 13) | ( 1 << 14), (1 << 14));
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SoundPlaySoundEffectOnFreePair(s32 arg0) {
    if (g_SoundControlFlags & SOUND_CTL_FLAG_SOUND_EFFECT_ENABLED) {
        s32 value = SoundAllocateSoundEffectElements(arg0, 2);

        g_SoundEffectElementPairCount = SOUND_EFFECT_ELEMENT_PAIR_COUNT;
        func_8003B644((s16)(value | 0x2000), arg0, 0x6000, 0x4000);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SoundPlaySoundEffectOnPair(s32 arg0, s32 arg1) {
    if (g_SoundControlFlags & SOUND_CTL_FLAG_SOUND_EFFECT_ENABLED) {
        g_SoundEffectElementPairCount = SOUND_EFFECT_ELEMENT_PAIR_COUNT;
        func_8003B644(
            ((arg1 & 0xFE) ^ (1 << 3)) | (1 << 13), // wtf is this... we really need to figure out some of these macros
            arg0,
            (1 << 13) | (1 << 14),
            (1 << 14)
        );
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSendCommandByIdWithLevels(s32 arg0, s32 arg1, s32 arg2) {
    if (g_SoundControlFlags & SOUND_CTL_FLAG_SOUND_EFFECT_ENABLED) {
        s32 value = SoundAllocateSoundEffectElements(arg0, 2);

        g_SoundEffectElementPairCount = SOUND_EFFECT_ELEMENT_PAIR_COUNT;
        func_8003B644((s16)(value | 0x2000), arg0, (s16)(arg1 << 8), (s16)(arg2 << 8));
    }
}

void SoundPlaySoundEffectOnPairWithLevels(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    if (g_SoundControlFlags & SOUND_CTL_FLAG_SOUND_EFFECT_ENABLED) {
        g_SoundEffectElementPairCount = SOUND_EFFECT_ELEMENT_PAIR_COUNT;
        func_8003B644(((arg1 & 0xFE) ^ 0x8) | 0x2000, arg0, (s16)(arg2 << 8), (s16)(arg3 << 8));
    }
}

void SoundStopAllElements(void) {
    AudioManager* pManager = g_SoundEffectAudioManager;
    AudioElement* pElement = &pManager->elements[0];
    u32 cnt = g_SoundEffectElementCount;

    DisableEvent(g_unk_SoundEvent);
    do {
        if (pElement->active_flag & SOUND_CHANNEL_ACTIVE) {
            pElement->active_flag = 0;
            SoundReleaseVoiceFromChannel(&pElement->voice_data, pElement->voice_number);
        }
        pElement++;
        cnt--;
    } while (cnt);
    pManager->activeElementMask = 0;
    EnableEvent(g_unk_SoundEvent);
}

void SoundStopElementsForSoundFile(SoundFile* pSoundFile) {
    AudioManager* pManager = g_SoundEffectAudioManager;
    AudioElement* pElement = &pManager->elements[0];
    u32 cnt = g_SoundEffectElementCount;
    s16 sedId = pSoundFile->sedId;

    do {
        if (pElement->active_flag & SOUND_CHANNEL_ACTIVE) {
            // soundId holds two 16-bit ids; the second one is the owning file's sedId
            if ((pElement->soundId >> 16) == sedId) {
                pElement->active_flag = 0;
                pManager->activeElementMask &= ~(1 << (u8)pElement->elementIndex);
                SoundReleaseVoiceFromChannel(&pElement->voice_data, pElement->voice_number);
            }
        }
        pElement++;
        cnt--;
    } while (cnt);
}

void SoundStopElementsById(s32 sedId) {
    AudioManager* pManager = g_SoundEffectAudioManager;
    AudioElement* pElement = &pManager->elements[0];
    u32 cnt = g_SoundEffectElementCount;

    do {
        if (pElement->active_flag & SOUND_CHANNEL_ACTIVE) {
            if (pElement->soundId == sedId) {
                pElement->active_flag = 0;
                pManager->activeElementMask &= ~(1 << (u8)pElement->elementIndex);
                SoundReleaseVoiceFromChannel(&pElement->voice_data, pElement->voice_number);
            }
        }
        pElement++;
        cnt--;
    } while (cnt);
}

void SoundStopElementsByIndex(s32 elementIndex) {
    AudioManager* pManager;
    AudioElement* pElement;
    u32 cnt;

    elementIndex &= 0xFE;
    elementIndex ^= 8;
    cnt = 2;
    pManager = g_SoundEffectAudioManager;
    pElement = &pManager->elements[elementIndex];

    do {
        if (pElement->active_flag & SOUND_CHANNEL_ACTIVE) {
            pElement->active_flag = 0;
            pManager->activeElementMask &= ~(1 << (u8)pElement->elementIndex);
            SoundReleaseVoiceFromChannel(&pElement->voice_data, pElement->voice_number);
        }
        pElement++;
        cnt--;
    } while (cnt);
}

void SoundNop2(void) {}

void SoundNop3(void) {}

void SoundSetElementVolumeById(s32 id, s32 value) {
    AudioManager* pManager = g_SoundEffectAudioManager;
    AudioElement* pElement = &pManager->elements[0];
    u32 cnt = g_SoundEffectElementCount;

    do {
        if (pElement->active_flag & SOUND_CHANNEL_ACTIVE) {
            if (pElement->soundId == id) {
                pElement->volume = value << 8;
                pElement->status_flags = SOUND_STATUS_VOLUME_CHANGE;
            }
        }
        pElement++;
        cnt--;
    } while (cnt);
}

void SoundSetElementVolumeByIndex(s32 elementIndex, s32 value) {
    AudioElement* pElement;
    u32 cnt;

    elementIndex &= 0xFE;
    elementIndex ^= 8;
    cnt = 2;
    pElement = &g_SoundEffectAudioManager->elements[elementIndex];

    do {
        if (pElement->active_flag & SOUND_CHANNEL_ACTIVE) {
            pElement->volume = value << 8;
            pElement->status_flags = SOUND_STATUS_VOLUME_CHANGE;
        }
        pElement++;
        cnt--;
    } while (cnt);
}

void SoundFadeElementVolumeById(s32 id, s32 value, s32 frames) {
    AudioManager* pManager = g_SoundEffectAudioManager;
    AudioElement* pElement = &pManager->elements[0];
    u32 cnt = g_SoundEffectElementCount;
    s32 volume = value << 8;
    s32 delta;

    do {
        if (pElement->active_flag & SOUND_CHANNEL_ACTIVE) {
            if (pElement->soundId == id) {
                delta = volume - pElement->volume;
                if (delta) {
                    if (frames == 0) {
                        frames = 1;
                    }
                    pElement->volumeStep = delta / frames;
                    pElement->volumeTarget = volume;
                    pElement->volumeFrames = frames;
                    pElement->rampFlags |= 0x20;
                }
            }
        }
        pElement++;
        cnt--;
    } while (cnt);
}

void SoundFadeElementVolumeByIndex(s32 elementIndex, s32 value, s32 frames) {
    AudioElement* pElement;
    u32 cnt;
    s32 volume;
    s32 delta;

    elementIndex &= 0xFE;
    elementIndex ^= 8;
    cnt = 2;
    volume = value << 8;
    pElement = &g_SoundEffectAudioManager->elements[elementIndex];

    do {
        if (pElement->active_flag & SOUND_CHANNEL_ACTIVE) {
            delta = volume - pElement->volume;
            if (delta) {
                if (frames == 0) {
                    frames = 1;
                }
                pElement->volumeStep = delta / frames;
                pElement->volumeTarget = volume;
                pElement->volumeFrames = frames;
                pElement->rampFlags |= 0x20;
            }
        }
        pElement++;
        cnt--;
    } while (cnt);
}

void SoundSetElementPanById(s32 id, s32 value) {
    AudioManager* pManager = g_SoundEffectAudioManager;
    AudioElement* pElement = &pManager->elements[0];
    u32 cnt = g_SoundEffectElementCount;

    do {
        if (pElement->active_flag & SOUND_CHANNEL_ACTIVE) {
            if (pElement->soundId == id) {
                pElement->pan = value << 8;
                pElement->status_flags = SOUND_STATUS_VOLUME_CHANGE;
            }
        }
        pElement++;
        cnt--;
    } while (cnt);
}

void SoundSetElementPanByIndex(s32 elementIndex, s32 value) {
    AudioElement* pElement;
    u32 cnt;
    s32 volume;

    elementIndex &= 0xFE;
    elementIndex ^= 8;
    cnt = 2;
    volume = value << 8;
    pElement = &g_SoundEffectAudioManager->elements[elementIndex];

    do {
        if (pElement->active_flag & SOUND_CHANNEL_ACTIVE) {
            pElement->pan = volume;
            pElement->status_flags = SOUND_STATUS_VOLUME_CHANGE;
        }
        pElement++;
        cnt--;
    } while (cnt);
}

s32 SoundGetActiveElementMask(s32 id) {
    AudioElement* pElement = &g_SoundEffectAudioManager->elements[0];
    u32 bit = 1;
    s32 count = g_SoundEffectElementCount;
    s32 result = 0;

    if (id == -1) {
        do {
            if (pElement->active_flag & SOUND_CHANNEL_ACTIVE) {
                result |= bit;
            }
            pElement++;
            bit <<= 1;
        } while (--count);
    } else {
        do {
            if ((pElement->active_flag & SOUND_CHANNEL_ACTIVE) && pElement->soundId == id) {
                result |= bit;
            }
            pElement++;
            bit <<= 1;
        } while (--count);
    }

    return result;
}

s32 SoundAllocateSoundEffectElements(s32 soundId, s32 count) {
    AudioManager* pManager = g_SoundEffectAudioManager;
    AudioElement* pElement = &pManager->elements[0];
    u32 cnt = g_SoundEffectElementCount;
    s32 index;
    s32 bestIndex;
    u32 bestTick;
    u32 lowMask;
    u32 blockMask;
    s32 limit;
    u32 exclude = 0; // must stay a variable: retail's mask test builds ~0 in a register, not a literal
    u32 activeMask;
    s32 span;

    do {
        if (pElement->active_flag & SOUND_CHANNEL_ACTIVE) {
            if (pElement->soundId == soundId) {
                pElement->active_flag = 0;
                pManager->activeElementMask &= ~(1 << (u8)pElement->elementIndex);
                SoundReleaseVoiceFromChannel(&pElement->voice_data, pElement->voice_number);
            }
        }
        pElement++;
        cnt--;
    } while (cnt);

    limit = pManager->elementCount - g_SoundSoundEffectChannelCount;
    pManager = g_SoundEffectAudioManager;
    span = count + 2;
    index = g_SoundEffectElementCount - span;
    lowMask = ~0u >> (32 - count);
    bestTick = -1;
    blockMask = lowMask << index;
    activeMask = ~exclude & pManager->activeElementMask;
    pElement = &pManager->elements[index];
    if (activeMask & blockMask) {
        do {
            if (pElement->startTick < bestTick && (u8)pElement->unk_0x07 < 0x21) {
                bestTick = pElement->startTick;
                bestIndex = index;
            }
            blockMask >>= count;
            if (blockMask < lowMask || (u32)index <= (u32)limit) {
                index = bestIndex;
                break;
            }
            pElement -= count;
            index -= count;
        } while (activeMask & blockMask);
    }

    return index;
}

s32 SoundAudioManagerIsActive(AudioManager* pManager) {
    return (u16)pManager->flags >> 15;
}

void SoundSetAudioManagerMasterVolumeWithFade(AudioManager* pManager, s32 volume, s32 frames) {
    s32 delta;

    if (volume == 0) {
        volume = 0x100;
    }

    pManager->tempoScaleInterpolator.targetValue = volume;

    if (frames == 0) {
        s32 product = (s16)(pManager->tempo >> 16) * volume;

        pManager->tempoScaleInterpolator.counter = 0;
        pManager->tempoScaleInterpolator.currentValue = volume << 16;
        pManager->tickRate = product;
    } else {
        delta = (volume << 16) - pManager->tempoScaleInterpolator.currentValue;
        if (delta != 0) {
            pManager->tempoScaleInterpolator.counter = frames;
            pManager->tempoScaleInterpolator.stepIncrement = delta / frames;
        }
    }
}

void SoundSetAudioManagerVolumeWithFade(AudioManager* pManager, s32 volume, s32 frames) {
    s32 delta;

    pManager->volumeInterpolator.targetValue = volume << 8;

    if (frames == 0) {
        pManager->volumeInterpolator.currentValue = volume << 24;
        pManager->volumeInterpolator.counter = 0;
        unk_SoundSetFlagsOnActiveVoices(SOUND_STATUS_VOLUME_CHANGE, pManager);
    } else {
        delta = (volume << 16) - (pManager->volumeInterpolator.currentValue >> 8);
        if (delta == 0) {
            return;
        }
        pManager->volumeInterpolator.counter = frames;
        pManager->volumeInterpolator.stepIncrement = (delta / frames) << 8;
    }

    if ((pManager->flags & SOUND_MANAGER_FLAG_NEEDS_ACTIVATION) && volume != 0) {
        SoundActivateAudioManager(pManager);
    }
}

void SoundFadeAudioManagerInterpolator7c(AudioManager* pManager, s16 volume, s32 frames) {
    s32 delta;

    pManager->noteInterpolator.targetValue = volume << 8;

    if (frames == 0) {
        pManager->noteInterpolator.currentValue = volume << 24;
        pManager->noteInterpolator.counter = 0;
        unk_SoundSetFlagsOnActiveVoices(SOUND_STATUS_PLAYING_NTOE, pManager);
    } else {
        delta = (volume << 16) - (pManager->noteInterpolator.currentValue >> 8);
        if (delta != 0) {
            pManager->noteInterpolator.counter = frames;
            pManager->noteInterpolator.stepIncrement = (delta / frames) << 8;
        }
    }
}

void SoundFadeAudioManagerInterpolator88(AudioManager* pManager, s16 volume, s32 frames) {
    s32 delta;

    pManager->panInterpolator.targetValue = volume << 8;

    if (frames == 0) {
        pManager->panInterpolator.currentValue = volume << 24;
        pManager->panInterpolator.counter = 0;
        unk_SoundSetFlagsOnActiveVoices(SOUND_STATUS_VOLUME_CHANGE, pManager);
    } else {
        delta = (volume << 16) - (pManager->panInterpolator.currentValue >> 8);
        if (delta != 0) {
            pManager->panInterpolator.counter = frames;
            pManager->panInterpolator.stepIncrement = (delta / frames) << 8;
        }
    }
}

void SoundActivateAudioManager(AudioManager* pManager) {
    DisableEvent(g_unk_SoundEvent);

    if (g_SoundControlFlags & SOUND_CTL_FLAG_REVERB_ENABLED) {
        SoundSetReverbModeWithAllocation(pManager->reverbType, pManager->reverbDepth, pManager->reverbDelay, pManager->reverbFeedback);
    }

    SoundSetFlagsOnActiveVoices(pManager, 0xFFFF);
    SoundSetElementsStatusFlag1(pManager);
    pManager->flags = (pManager->flags & ~SOUND_MANAGER_FLAG_NEEDS_ACTIVATION) | SOUND_MANAGER_FLAG_PLAYING;
    EnableEvent(g_unk_SoundEvent);
}

// Mutes or unmutes the voices selected by `flags`, one bit per element.
void SoundMuteElementsByFlags(AudioManager* pManager, u32 flags) {
    AudioElement* pElement;
    s32 count;

    if (pManager == NULL) {
        return;
    }

    pManager->muteFlags = flags;
    pElement = pManager->elements;
    count = pManager->elementCount;
    do {
        if (pElement->active_flag != 0) {
            if (flags & 1) {
                if (!(pElement->active_flag & SOUND_CHANNEL_MUTED)) {
                    pElement->active_flag |= SOUND_CHANNEL_MUTED;
                    if (pManager->flags & SOUND_MANAGER_FLAG_PLAYING) {
                        SoundStopVoiceOnChannel(&pElement->voice_data, pElement->voice_number);
                    }
                }
            } else {
                if (pElement->active_flag & SOUND_CHANNEL_MUTED) {
                    pElement->active_flag &= ~SOUND_CHANNEL_MUTED;
                    if ((*(u32*)&pElement->active_flag & (SOUND_CHANNEL_PERCUSSION_ACTIVE | SOUND_CHANNEL_HOLD_NOTE)) == SOUND_CHANNEL_HOLD_NOTE) {
                        if (pManager->flags & SOUND_MANAGER_FLAG_PLAYING) {
                            SoundAssignVoiceToChannelAndPlay(&pElement->voice_data, pElement->voice_number);
                        }
                    }
                }
            }
        }
        pElement++;
        flags >>= 1;
    } while (--count != 0);
}

void SoundSetAudioManagerUnk1B(AudioManager* pManager, u8 value) {
    pManager->unk_0x1b = value;
}

// The play-time record SoundGetAudioManagerPlayTime fills.
typedef struct {
    /* 0x0 */ u32 playTicks;
    /* 0x4 */ u16 unk4; // total - field * 240
    /* 0x6 */ u16 unk6; // field - index * 60
    /* 0x8 */ u16 unk8; // index
} SoundPlayTime; // Size: 0xA

void SoundGetAudioManagerPlayTime(AudioManager* pManager, SoundPlayTime* pOut) {
    u32 total = (u32)pManager->playTime >> 8;
    u32 field = total / 240;
    u32 index = field / 60;

    pOut->playTicks = pManager->playTicks;
    pOut->unk4 = total - field * 240;
    pOut->unk6 = field - index * 60;
    pOut->unk8 = index;
}

u16* SoundGetLowestActiveElementCounter(AudioManager* pManager) {
    AudioElement* pElement = pManager->elements;
    u16 count = pManager->elementCount;
    u16* pResult = &pManager->lowestLoopCount;
    u16 best = SOUND_LOOP_COUNT_NONE;

    do {
        if (pElement->active_flag != 0 && pElement->loopCount < best) {
            best = pElement->loopCount;
        }
        pElement++;
    } while (--count);

    if (best == SOUND_LOOP_COUNT_NONE) {
        best = 0;
    }
    *pResult = best;
    return pResult;
}

// Header of an in-memory sequence file, reached through AudioManager::pSeqHeader.
typedef struct {
    /* 0x00 */ u8 unk00[0x10];
    /* 0x10 */ u16 unk10;
    /* 0x12 */ u16 unk12;
    /* 0x14 */ u8 elementCount;
    /* 0x15 */ u8 unk15;
    /* 0x16 */ u16 unk16;
    /* 0x18 */ u16 unk18;
    /* 0x1A */ u8 unk1A;
    /* 0x1B */ u8 unk1B;
    /* 0x1C */ u8 unk1C;
    /* 0x1D */ u8 unk1D;
    /* 0x1E */ u16 unk1E;
} SoundSeqHeader; // Size: 0x20

s32 SoundGetAudioManagerSeqOffsetPointer(AudioManager* pManager) {
    AudioManager* pCurrent = g_SoundAudioManagerListHead;
    s32 result;

    if (pManager != NULL) {
        while (pCurrent != NULL && pCurrent != pManager) {
            pCurrent = pCurrent->next;
        }
    }

    if (pCurrent != NULL) {
        SoundSeqHeader* pSeq = (SoundSeqHeader*)pCurrent->pSeqHeader;

        result = pSeq->unk1E + (s32)pSeq;
    } else {
        result = 0;
    }

    return result;
}

void SoundSetAudioManagerBackupState(AudioManager* pManager, s32 mode) {
    switch (mode) {
    case SOUND_MANAGER_BACKUP_DISCARD:
        SoundDiscardAudioManagerBackup(pManager);
        break;
    case SOUND_MANAGER_BACKUP_SAVE:
        SoundSaveAudioManagerBackup(pManager);
        break;
    case SOUND_MANAGER_BACKUP_RESTORE:
        SoundRestoreAudioManagerBackup(pManager);
        break;
    }
}

void SoundDiscardAudioManagerBackup(AudioManager* pManager) {
    if (pManager->flags & SOUND_MANAGER_FLAG_BACKUP_SAVED) {
        pManager->flags &= ~SOUND_MANAGER_FLAG_BACKUP_SAVED;
        SoundFreeAudioManagerBackups(pManager);
    }
}

void SoundSaveAudioManagerBackup(AudioManager* pManager) {
    AudioManager* pCopy;
    s32 size;

    DisableEvent(g_unk_SoundEvent);
    size = SoundCalculateAudioManagerSize(pManager->elementCount);
    if (pManager->pBackup == NULL) {
        pCopy = SoundHeapAllocate(size);
    }
    if (pCopy == NULL) {
        EnableEvent(g_unk_SoundEvent);
        return;
    }

    pManager->flags |= SOUND_MANAGER_FLAG_BACKUP_SAVED;
    pManager->pBackup = pCopy;
    SoundHeapSetBlockMemory(pCopy, pManager, size);
    pCopy->next = NULL;
    pCopy->pBackup = NULL;
    pManager->restoreTicks = 0;
    EnableEvent(g_unk_SoundEvent);
}

void SoundRestoreAudioManagerBackup(AudioManager* pManager) {
    s32 savedValue;

    if (pManager->pBackup != NULL && (pManager->flags & SOUND_MANAGER_FLAG_BACKUP_SAVED)) {
        DisableEvent(g_unk_SoundEvent);
        SoundReleaseAllVoices(pManager);
        savedValue = pManager->playTicks;
        SoundCopyAudioManagerData(pManager, pManager->pBackup);
        pManager->restoreTicks = savedValue;
        SoundSetFlagsOnActiveVoices(pManager, 0xFFFF);
        SoundSetElementsStatusFlag1(pManager);
        EnableEvent(g_unk_SoundEvent);
    }
}

void SoundSetAudioManagerMaxVolume(AudioManager* pManager, u16 volume) {
    s32 newVolume = 0x7F00;
    u_long event = g_unk_SoundEvent;

    pManager->flags |= SOUND_MANAGER_FLAG_STOP_PENDING;
    pManager->stopBar = volume;
    DisableEvent(event);
    SoundReleaseAllVoices(pManager);
    EnableEvent(g_unk_SoundEvent);
    pManager->tempoScaleInterpolator.targetValue = newVolume;
    pManager->tempoScaleInterpolator.counter = 0;
    pManager->tempoScaleInterpolator.currentValue = newVolume << 16;
    pManager->tickRate = (pManager->tempo >> 16) * newVolume;
}
/*
Attempt (87.10%, same size): every instruction is right; only the first eight are
ordered differently. The original schedules `lui/lw a0` (DisableEvent's argument)
before `sw ra`/`lhu`/`sh a1,30(pManager)`, gcc puts them after. Statement order of the
two field updates does not change it.
void SoundSetAudioManagerMaxVolume(AudioManager* pManager, u16 volume) {
    s32 newVolume = 0x7F00;

    pManager->flags |= 0x20;
    pManager->stopBar = volume;
    DisableEvent(g_unk_SoundEvent);
    SoundReleaseAllVoices(pManager);
    EnableEvent(g_unk_SoundEvent);
    pManager->tempoScaleInterpolator.targetValue = newVolume;
    pManager->tempoScaleInterpolator.counter = 0;
    pManager->tempoScaleInterpolator.currentValue = newVolume << 16;
    pManager->tickRate = (pManager->tempo >> 16) * newVolume;
}
*/

//----------------------------------------------------------------------------------------------------------------------
void SoundSetElementsStatusFlag1(AudioManager* manager) {
    AudioElement* pElement;
    u32 cnt;

    cnt = manager->elementCount;
    pElement = &manager->elements[0];

    do {
        // What's going on here? Are the first two fields just a u32?
        if ((*(u32*)pElement & (SOUND_CHANNEL_ACTIVE | SOUND_CHANNEL_HOLD_NOTE)) == (SOUND_CHANNEL_ACTIVE | SOUND_CHANNEL_HOLD_NOTE)) {
            // I'm beginning to think that this isn't just a flag for activity considering this mask
            if ((pElement->active_flag & (SOUND_CHANNEL_PERCUSSION_ACTIVE | SOUND_CHANNEL_MUTED)) == 0) {
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

// One 5-byte record of the percussion map: the slot it fills and the word stored there.
typedef struct {
    /* 0x0 */ u8 index;
    /* 0x1 */ u8 value[4];
} SoundPercussionEntry; // Size: 0x5

// One 4-byte slot of the percussion table the manager's heap block ends with.
typedef struct {
    /* 0x0 */ u8 tone; // indexes the WDS tone table
    /* 0x1 */ u8 tune;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 pan;
} SoundPercussionTone; // Size: 0x4

void SoundCopyPercussionTable(AudioManager* pManager, AudioManager* pSrc) {
    u32* pTable;
    u32* pSlot;
    SoundPercussionEntry* pEntry;
    u16 mapOffset;
    u32 cnt;

    // the percussion map is the tail of the manager's heap block
    pTable = (u32*)((u8*)pManager + SoundCalculateAudioManagerSize(pSrc->elementCount));
    pManager->pPercussionData = (s32)pTable;
    // only the low 16 bits of unk_0x20 are the map's byte offset; the same field is a
    // 32-bit tick counter in SoundTimerInterruptHandler, so the width lives in this local
    mapOffset = pSrc->unk_0x20;
    pEntry = (SoundPercussionEntry*)((u8*)pSrc + mapOffset);
    cnt = pSrc->percussionCount;

    do {
        u32 value = pEntry->value[0] | (pEntry->value[1] << 8) | (pEntry->value[2] << 16) | (pEntry->value[3] << 24);

        pSlot = pTable + pEntry->index;
        *pSlot = value;
        pEntry++;
        cnt--;
    } while (cnt);
}

AudioManager* SoundCreateAudioManagerForElementCount(s32 elementCount) {
    s32 count = elementCount & ~1;
    AudioManager* pManager;
    AudioElement* pElement;
    s32 i;
    s32 voiceNumber;
    s32 nRemaining;

    g_SoundEffectElementCount = count;
    pManager = SoundHeapAllocate(SoundCalculateAudioManagerSize(count));
    if (pManager == NULL) {
        SoundHandleError(SOUND_ERR_HEAP_ALLOC_FAILED);
        return NULL;
    }

    SoundResetAudioManager(pManager);

    pElement = &pManager->elements[0];
    voiceNumber = NUM_VOICES - count;
    nRemaining = count;
    i = 0;
    do {
        pElement->active_flag = 0;
        pElement->elementIndex = i++;
        pElement->voice_number = voiceNumber;
        pElement++;
        voiceNumber++;
        nRemaining--;
    } while (nRemaining);

    SoundAddAudioManagerToList(pManager);
    return pManager;
}

void SoundFreeAudioManager(AudioManager* pManager) {
    SoundRemoveAudioManagerFromList(pManager);
    SoundHeapFree(pManager);
}

void SoundLoadAudioManagerSequenceHeader(AudioManager* pManager) {
    SoundSeqHeader* pSeq = (SoundSeqHeader*)pManager->pSeqHeader;
    s16 status;

    status = (s16)SoundUnkDebug0(pManager->pSeqHeader);
    if (status != 0) {
        SoundHandleError(SOUND_ERR_INVALID_SEQ_HEADER);
        return;
    }

    pManager->flags |= SOUND_MANAGER_FLAG_SEQUENCE_LOADED;
    pManager->unk_0x12 = pSeq->unk10;
    pManager->elementCount = pSeq->elementCount;
    pManager->sndId = pSeq->unk16;
    pManager->unk_0x18 = pSeq->unk18;
    pManager->reverbType = pSeq->unk1A;
    pManager->reverbDepth = pSeq->unk1B << 8;
    pManager->reverbDelay = pSeq->unk1C;
    pManager->reverbFeedback = pSeq->unk1D;

    if (g_SoundControlFlags & SOUND_CTL_FLAG_REVERB_ENABLED) {
        SoundSetReverbModeWithAllocation((s8)pManager->reverbType, pManager->reverbDepth, pManager->reverbDelay, pManager->reverbFeedback);
    }

    SoundInitializeAudioManager(pManager);
}

void SoundResetAudioManager(AudioManager* pManager) {
    pManager->flags = 2;
    pManager->unk_0x12 = 0x7FFF;
    pManager->sndId = 0;
    pManager->unk_0x18 = 0x7F;
    pManager->elementCount = g_SoundEffectElementCount;
    SoundInitializeAudioManager(pManager);
}

//----------------------------------------------------------------------------------------------------------------------
// TODO(jperos): Boy, we really need some names of this stuff
void SoundInitializeAudioManager(AudioManager* manager) {
    SoundFreeAudioManagerBackups(manager);

    manager->barCount = 1;
    manager->unk_0x1a = 0;
    manager->unk_0x1b = 0;
    manager->lowestLoopCount = 0;
    manager->beatCount = 0;
    manager->beatsPerBar = 4;
    manager->tickCount = 1;
    manager->ticksPerBeat = 48;
    manager->beatDivisor = 4;
    manager->unk_0x3e = 4;

    manager->tempoScaleInterpolator.currentValue = 0x01000000;
    manager->volumeInterpolator.currentValue = 0x7F000000;

    manager->tempo = 0x00660000;
    manager->tickRate = 0x6600;
    manager->playTime = 0;
    manager->playTicks = 0;
    manager->unk_0x20 = 0;
    manager->activeElementMask = 0;
    manager->noteInterpolator.currentValue = 0;
    manager->panInterpolator.currentValue = 0;
    manager->tempoScaleInterpolator.counter = 0;
    manager->volumeInterpolator.counter = 0;
    manager->noteInterpolator.counter = 0;
    manager->panInterpolator.counter = 0;
    manager->tempoStep = 0;
    manager->tempoFrames = 0;
    manager->tickAccumulator = 0x00010000;
}

//----------------------------------------------------------------------------------------------------------------------
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003B424);

/* 84.32% - prologue and the element-field store block match; left over: the
   active_flag ternary (retail keeps a j + two li where gcc cross-jumps), the
   |= 4 reload (gcc reuses the stored value), and the envelope-zeroing base
   (retail walks from a0 = element + 0x60).
void func_8003B424(AudioManager* pManager) {
    u8* pSeqData;
    AudioElement* pElement;
    u16* pScriptOffset;
    SoundWDSEntry* pWds;
    s32 count = pManager->elementCount;
    u32 value;
    u32 aliveBits;
    s32 bit;
    s32 i;
    s32 voice;
    s32 j;

    if (count == 0) {
        return;
    }

    pElement = pManager->elements;
    i = 0;
    voice = -1;
    aliveBits = 0;
    pSeqData = (u8*)pManager->pSeqHeader;
    pScriptOffset = (u16*)(pSeqData + 0x22);
    pWds = SoundFindWdsEntry(pManager->sndId);
    if (pWds == NULL) {
        do {
            pWds = g_SoundWdsLinkedList;
        } while (0);
    }

    do {
        if (*pScriptOffset != 0) {
            bit = 1 << i;
            aliveBits |= bit;
            pElement->active_flag = (pManager->muteFlags & bit) ? 0x421 : 0x401;
            if (pManager->flags & 4) {
                pElement->active_flag |= 4;
            }

            pElement->status_flags = 0x170;
            pElement->rampFlags = 0;
            value = *(u16*)(pSeqData + 0x10);
            pElement->unk_0x07 = 0x10;
            pElement->elementIndex = i;
            pElement->soundId = value;
            pElement->octave = 0x3C;
            pElement->gateRatio = 0xF;
            pElement->loopIndex = 0xFFFF;
            pElement->volume = 0x6000;
            pElement->elementVolume = 0x7F000000;
            pElement->savedScriptIP = NULL;
            pElement->unk_0x1C = 0;
            pElement->loopCount = 0;
            pElement->unk_0x22 = 0;
            pElement->fermataDuration = 0;
            pElement->durationBias = 0;
            pElement->detune = 0;
            pElement->currentNote = 0;
            pElement->pan = 0x4000;
            pElement->slideFrames = 0;
            pElement->noteOffset = 0;
            pElement->volumeOffset = 0;
            pElement->panOffset = 0;
            pElement->voice_data.unkC = 0;
            pElement->voice_data.unkE = 0;
            pElement->activeEnvelopeMask = 0;
            value = (u32)pSeqData + *pScriptOffset;
            pElement->scriptStartIP = value;
            pElement->pScriptIP = value;

            for (j = NUM_VOICE_ENVELOPES - 1; j >= 0; j--) {
                pElement->envelopes[j].flags = 0;
            }

            pElement->pWds = (u32)pWds;
            pElement->sndId = (u8)pManager->sndId;
            if (pWds != NULL) {
                SoundApplyToneToElement(0, pElement);
            }
            pElement->voice_number = voice;
            pElement->voice_data.modeFlags = 0;
            pElement->voice_data.priority = 0x100;
            SoundAssignVoiceToChannelAndStop(&pElement->voice_data, voice);
        } else {
            pElement->active_flag = 0;
        }

        pScriptOffset++;
        pElement++;
        i++;
        voice++;
    } while (--count != 0);

    pManager->activeElementMask = aliveBits;
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003B644);
/*
Attempt (96.74%, 748 bytes = the original's size): the ONLY residual is ONE instruction. Retail
shifts the volume VARIABLE (`srl v0,s8,0xf` then `andi v0,v0,1`); this compile folds the scaling
shift into it (`srl v0,v0,0x16` on the product's register), so the clamp test reads the product
instead of volume. Everything else is identical - the sed search loop, the WDS fallback, the whole
element store block, the envelope loop, the count/pWds registers and the 0x50 frame.
What got it here from the previous 96.32 park, do not regress:
(1) the scaling must be ONE assignment, `volume = (u32)((s16)volume * tablebyte) >> 7;`. Two
    statements give `mflo sN / srl sN,sN,7` (retail's shift shape, no fold) but the WRONG volume
    register ($s6 instead of $s8) - the merged form has half the references, which is what makes
    gcc hand volume the last callee-saved register.
(2) the `if (pWds == NULL)` fallback must NOT be wrapped in `do { } while (0);` any more: with the
    merged assignment that wrapper rotates pWds to $s6 and count to $s7 (retail: pWds $s7, count
    $s6). It was only needed by the old two-statement body.
Measured dead: the declaration order of `count`/`volume` (any order; the tie does not follow the
pseudo number), `s32` vs `u32 count`, and `value = volume >> 15;` as its own statement (96.66).
Two 3-minute permuter runs on this body found nothing (base 190 = this score, best 190).
Reached from an older body, worth keeping: `level` is s32 (so `(s16)volume` really narrows -
retail has the sll 16/sra 16 pair), `u32 volume = level;` makes volume reuse level's register
(retail's `move s8,a2` at the entry), and the `elementFlags = element >> 8;` assignment has to sit
just before `pScriptOffset = ...`.
The tone-table byte needs the LEFT-ASSOCIATIVE pointer sum below - `- (-off)` is the permuter
spelling: retail emits `addu v0,s2,v0` and then adds the masked index, and this form also gets the
`lhu 24(s2)` table-offset load emitted before the `lw 16(sp)` soundId reload. The realistic
`+ pSoundFile->volumeTableOffset` form scores 95.35 (one instruction order swap) - retry it only
if the fold above is ever solved.
All six callers of this function stay at 100.00% with the real four-argument prototype in scope.
Round 322, measured on the fold: splitting the scaling as `value = (u32)((s16)volume * tablebyte);`
then `volume = value >> 7;` DOES give retail's exact register shape (`srl s8,v0,0x7` - product left
in $v0, volume in $s8) and still folds the test shift into the product's register: 96.66. So the
register question is closed and the residual is purely cse's equivalence note on the shift's
destination letting combine rewrite `(lshltr (lshltr product 7) 15)` as `product >> 22`; the two
statements `volume = volume >> 7` variant coalesces the product into volume's $s6 (96.16), and
`if (volume & 0x8000)` loses the `srl 15` instruction entirely (one short of retail's size - cannot
link). Nothing in source order, declaration order or a do/while block moved the fold.
*/
/*
void func_8003B644(s16 element, s32 soundId, s32 level, s16 pan) {
    SoundFile* pSoundFile = g_SoundSedsLinkedList;
    AudioManager* pManager = g_SoundEffectAudioManager;
    AudioElement* pElement;
    SoundWDSEntry* pWds;
    u16* pScriptOffset;
    u8 elementFlags;
    s16 sedId = soundId >> 16;
    u32 value;
    s32 count;
    s32 i;
    u32 volume = level;

    while (pSoundFile->sedId != sedId) {
        pSoundFile = pSoundFile->pNext;
        if (pSoundFile == NULL) {
            return;
        }
    }

    pWds = SoundFindWdsEntry(pSoundFile->sndId);
    if (pWds == NULL) {
        pWds = g_SoundWdsLinkedList;
    }

    volume = (u32)((s16)volume * *(u8*)((u8*)pSoundFile - (-pSoundFile->volumeTableOffset) + (soundId & 0xFFFF))) >> 7;
    if ((volume >> 15) & 1) {
        volume = 0x7FFF;
    }

    elementFlags = element >> 8;
    pScriptOffset = &pSoundFile->scriptOffsets[(soundId & 0xFFFF) * 2];
    pElement = &pManager->elements[element & 0xFF];
    count = g_SoundEffectElementPairCount;

    DisableEvent(g_unk_SoundEvent);

    do {
        pElement->soundId = soundId;
        pElement->startTick = g_SoundTimerTickCounter;
        pElement->unk_0x07 = elementFlags;

        if (*pScriptOffset != 0) {
            pManager->activeElementMask |= 1 << (u8)pElement->elementIndex;
            pElement->active_flag = 0x409;
            if (pSoundFile->flags & 0x1) {
                pElement->active_flag = 0x40B;
            }

            pElement->status_flags = 0x170;
            pElement->rampFlags = 0;
            value = (u32)pSoundFile + *pScriptOffset;
            pElement->octave = 0x3C;
            pElement->gateRatio = 0xF;
            pElement->loopIndex = 0xFFFF;
            pElement->savedScriptIP = NULL;
            pElement->unk_0x1C = 0;
            pElement->loopCount = 0;
            pElement->unk_0x22 = 0;
            pElement->fermataDuration = 0;
            pElement->durationBias = 0;
            pElement->detune = 0;
            pElement->currentNote = 0;
            pElement->volume = volume;
            pElement->elementVolume = 0x7F000000;
            pElement->slideFrames = 0;
            pElement->noteOffset = 0;
            pElement->volumeOffset = 0;
            pElement->panOffset = 0;
            pElement->voice_data.unkC = 0;
            pElement->voice_data.unkE = 0;
            pElement->activeEnvelopeMask = 0;
            pElement->pan = pan;
            pElement->scriptStartIP = value;
            pElement->pScriptIP = value;

            for (i = NUM_VOICE_ENVELOPES - 1; i >= 0; i--) {
                pElement->envelopes[i].flags = 0;
            }

            pElement->pWds = (u32)pWds;
            pElement->sndId = pSoundFile->sndId;
            if (pWds != NULL) {
                SoundApplyToneToElement(0, pElement);
            }
            pElement->voice_data.modeFlags = 0;
            pElement->voice_data.priority = 0x200;
            SoundAssignVoiceToChannelAndStop(&pElement->voice_data, pElement->voice_number);
        } else {
            pManager->activeElementMask &= ~(1 << (u8)pElement->elementIndex);
            pElement->active_flag = 0;
            SoundReleaseVoiceFromChannel(&pElement->voice_data, pElement->voice_number);
        }

        pScriptOffset++;
        pElement++;
    } while (--count != 0);

    pManager->flags |= 0x8000;
    EnableEvent(g_unk_SoundEvent);
}
*/
void SoundFreeAudioManagerBackups(AudioManager* pManager) {
    AudioManager* pEntry = pManager->pBackup;

    if (pEntry != NULL) {
        pManager->pBackup = NULL;
        do {
            pManager = pEntry->pBackup;
            SoundHeapFree(pEntry);
            pEntry = pManager;
        } while (pManager != NULL);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SoundCopyAudioManagerData(AudioManager* pDest, AudioManager* pSrc) {
    AudioManager* savedNext;
    AudioManager* savedUnk;

    savedNext = pDest->next;
    savedUnk = pDest->pBackup;
    SoundHeapSetBlockMemory(pDest, pSrc, SoundCalculateAudioManagerSize(pDest->elementCount));
    pDest->next = savedNext;
    pDest->pBackup = savedUnk;
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

    // Handle cleanup if needed (playing flag set)
    if (manager->flags & SOUND_MANAGER_FLAG_PLAYING) {
        if (manager == NULL) {
            SoundHandleError(SOUND_ERR_INVALID_MANAGER);  // Invalid cleanup state
        } else {
            // Clear cleanup flag and release resources
            manager->flags &= ~SOUND_MANAGER_FLAG_PLAYING;
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
void SoundSetElementActiveFlags(u16 flags, AudioManager* pManager) {
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
    g_SoundControlFlags |= SOUND_CTL_FLAG_IRQ_HANDLER;
    if (pCallback) {
        pCallback();
    }
    
    g_SoundControlFlags &= ~SOUND_CTL_FLAG_TRANSFER_BUSY;
    if (g_SoundTransferQueueReadIndex != g_SoundTransferQueueWriteIndex) {
        SoundProcessTransferCommand();
    }
    g_SoundControlFlags &= ~SOUND_CTL_FLAG_IRQ_HANDLER;
}

void SoundQueueSpuWriteCommand(u32 transferAddress, void* pData, u_long dataSize, SoundCommandCallback_t pCallback) {
    SoundQueueTransferCommand(transferAddress, pData, dataSize, pCallback, SOUND_SPU_COMMAND_WRITE);
}

void SoundQueueSpuReadCommand(u32 transferAddress, void* pData, u_long dataSize, SoundCommandCallback_t pCallback) {
    SoundQueueTransferCommand(transferAddress, pData, dataSize, pCallback, SOUND_SPU_COMMAND_READ);
}

// Queue SPU ReadDecodedData SPU_ALL Command
void SoundQueueSpuReadDecodedDataAll(u32 transferAddress, void* pData, u_long dataSize, SoundCommandCallback_t pCallback) {
    SoundQueueTransferCommand(transferAddress, pData, dataSize, pCallback, SOUND_SPU_COMMAND_READ_DECODED_ALL);
}

// Queue SPU ReadDecodedData SPU_CDONLY Command
void SoundQueueSpuReadDecodedDataCdOnly(u32 transferAddress, void* pData, u_long dataSize, SoundCommandCallback_t pCallback) {
    SoundQueueTransferCommand(transferAddress, pData, dataSize, pCallback, SOUND_SPU_COMMAND_READ_DECODED_CDONLY);
}

void SoundQueueTransferCommand(u32 transferAddress, void* pData, u_long dataSize, SoundCommandCallback_t pCallback, unsigned short commandType) {
    SoundTransferCommand* pCmd;
    unsigned short nControlFlags;
    unsigned short nNextIndex;

    nControlFlags = g_SoundControlFlags;
    if (!(nControlFlags & SOUND_CTL_FLAG_IRQ_HANDLER)) {
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
    
    if (!(g_SoundControlFlags & SOUND_CTL_FLAG_TRANSFER_BUSY)) {
        SoundProcessTransferCommand();
    }
    
    if (!(nControlFlags & SOUND_CTL_FLAG_IRQ_HANDLER)) {
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

void SoundNop4(void) {}

// Waits for the transfer queue to finish (arg & 0x10) and returns the mode of the
// command at the read index, or 0 when the queue is idle.
s32 SoundGetPendingSpuCommandType(u32 flags) {
    if (flags & SOUND_CTL_FLAG_TRANSFER_BUSY) {
        while (*(volatile u16*)&g_SoundControlFlags & SOUND_CTL_FLAG_TRANSFER_BUSY) {
        }
    }

    if (!(g_SoundControlFlags & SOUND_CTL_FLAG_TRANSFER_BUSY)) {
        return 0;
    }

    return (s16)g_SoundTransferQueue[g_SoundTransferQueueReadIndex].commandType;
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
    
    g_SoundControlFlags |= SOUND_CTL_FLAG_TRANSFER_BUSY;

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
        case SOUND_SPU_COMMAND_READ_DECODED_ALL:
            g_SoundDecodedReadResult = SpuReadDecodedData(pCmd->pSpuData, SPU_ALL);
            break;
        case SOUND_SPU_COMMAND_READ_DECODED_CDONLY:
            g_SoundDecodedReadResult = SpuReadDecodedData(pCmd->pSpuData, SPU_CDONLY);
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
s32 SoundTimerInterruptHandler(void) {
    AudioManager* pManager;
    s16 volume;
    u16 cdVolume;
    u32 start;
    u32 end;
    AudioElement* pElements;
    u8 count;

    if (g_SoundControlFlags & SOUND_CTL_FLAG_MUTED) {
        return 0;
    }

    start = GetRCnt(0xF2000002);

    if (g_SoundTimerTickCounter++ & 1) {
        if (g_SoundVolumeController.masterInterpolator.counter != 0) {
            SoundTickInterpolator(&g_SoundVolumeController.masterInterpolator);
            volume = (s16)(g_SoundVolumeController.masterInterpolator.currentValue >> 16);
            g_SoundVolumeController.currentMasterVolume = volume;
            SoundSetVolumeWithPhase(volume, &g_SoundVolumeController.commonAttr.mvol, 0);
            g_SoundVolumeController.commonAttr.mask |= SPU_COMMON_MVOLL | SPU_COMMON_MVOLR;
        }

        if (g_SoundCdVolumeFadeActive != 0) {
            SoundTickInterpolator(&g_SoundVolumeController.cdInterpolator);
            cdVolume = (u16)(g_SoundVolumeController.cdInterpolator.currentValue >> 16);
            g_SoundVolumeController.currentCdVolume = cdVolume;
            g_SoundVolumeController.commonAttr.cd.volume.right = cdVolume;
            g_SoundVolumeController.commonAttr.cd.volume.left = cdVolume;
            g_SoundVolumeController.commonAttr.mask |= SPU_COMMON_CDVOLL | SPU_COMMON_CDVOLR;
        }

        if (g_SoundVolumeController.commonAttr.mask != 0) {
            SpuSetCommonAttr(&g_SoundVolumeController.commonAttr);
            g_SoundVolumeController.commonAttr.mask = 0;
        }
    }

    func_8003E900();

    for (pManager = g_SoundAudioManagerListHead; pManager != NULL; pManager = pManager->next) {
        if (pManager->flags >= 0) {
            continue;
        }

        if (pManager->restoreTicks != 0 && (u32)pManager->playTicks >= (u32)pManager->restoreTicks) {
            SoundRestoreAudioManagerBackup(pManager);
        }

        if (pManager->tempoScaleInterpolator.counter != 0) {
            SoundTickInterpolator(&pManager->tempoScaleInterpolator);
            pManager->tickRate = (s16)(pManager->tempo >> 16) * (s16)(pManager->tempoScaleInterpolator.currentValue >> 16);
        }

        if (pManager->volumeInterpolator.counter != 0) {
            SoundTickInterpolator(&pManager->volumeInterpolator);
            unk_SoundSetFlagsOnActiveVoices(SOUND_STATUS_VOLUME_CHANGE, pManager);
        }

        if (pManager->noteInterpolator.counter != 0) {
            SoundTickInterpolator(&pManager->noteInterpolator);
            unk_SoundSetFlagsOnActiveVoices(SOUND_STATUS_PLAYING_NTOE, pManager);
        }

        if (pManager->panInterpolator.counter != 0) {
            SoundTickInterpolator(&pManager->panInterpolator);
            unk_SoundSetFlagsOnActiveVoices(SOUND_STATUS_VOLUME_CHANGE, pManager);
        }

        pManager->unk_0x20++;
        pManager->playTime += (s16)(pManager->tempoScaleInterpolator.currentValue >> 16);

        pManager->tickAccumulator -= pManager->tickRate;
        if (pManager->tickAccumulator < 0) {
            do {
                pManager->tickAccumulator += 0x10000;
                if (--pManager->tickCount == 0) {
                    pManager->tickCount = pManager->ticksPerBeat;
                    if ((u16)++pManager->beatCount > (u16)pManager->beatsPerBar) {
                        pManager->beatCount = 1;
                        pManager->barCount++;
                    }
                }

                count = pManager->elementCount;
                if (count != 0) {
                    pElements = pManager->elements;
                    func_8003C4C4(pManager, pElements, count);
                    func_8003C6E8(pManager, pElements, count);
                }

                if (cdVolume = pManager->activeElementMask != 0) {
                    pManager->playTicks++;
                    if (pManager->volumeInterpolator.currentValue == 0) {
                        SoundStopAudioManager(pManager);
                        pManager->flags |= SOUND_MANAGER_FLAG_NEEDS_ACTIVATION;
                    }

                    if ((u16)pManager->barCount == pManager->stopBar) {
                        pManager->flags &= ~SOUND_MANAGER_FLAG_STOP_PENDING;
                        SoundSetAudioManagerMasterVolumeWithFade(pManager, 0, 0);
                        pManager->stopBar = 0;
                    }
                } else {
                    pManager->flags &= ~SOUND_MANAGER_FLAG_PLAYING;
                    break;
                }
            } while (pManager->tickAccumulator < 0);
        }
    }

    for (pManager = g_SoundAudioManagerListHead; pManager != NULL; pManager = pManager->next) {
        if (pManager->flags >= 0) {
            continue;
        }

        count = pManager->elementCount;
        if (count != 0) {
            pElements = pManager->elements;
            SoundStepElementEnvelopes(pManager, pElements, count);
            SoundApplyElementStatusFlags(pManager, pElements, count);
        }
    }

    SoundApplyPendingKeyOff();

    if (g_SoundPendingSpuIrqEnable & 1) {
        g_SoundPendingSpuIrqEnable &= ~1;
        SpuSetIRQ(1);
    }

    end = GetRCnt(0xF2000002);
    if (end >= start) {
        g_SoundTimerCyclesElapsed = g_SoundTimerCyclesElapsed + (end - start);
        g_SoundTimerTicksTotal++;
    }

    return 0;
}


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
/*
Attempt (99.92%, retail's exact 548 bytes; 2 instructions differ, so it cannot link yet). The element
walk, the flag masks, the two 16-bit counters packed in the word at 0x5C, the ramp accumulators and
the key-off tail are all instruction for instruction right. What is left:
(1) the loop tail's last shift: retail `sll v0,v0,0x10` reads the fresh `next` ($v0), ours reads the
    loop-carried `count` ($a2) - gcc knows `next == count` after the copy. Needs the TWO-variable tail
    `next = count - 1; count = next;` with `while ((s16)next != 0)`. Measured dead on it: a `u16`
    count with `count--` (combine folds it to `addu a2,a2,t3` off a hoisted 0xFFFF, 96.45),
    `while ((u16)count != 0)` (andi, 96.90), `count = count - 1; next = count;` (97.30), `s16 next`
    (98.46, and it adds an `sra`).
(2) the base register of the rampFlags store: retail `sh a3,0x2($a0)` with $a0 = &status_flags (the
    whole function reaches the element at -2 from that base), ours `sh a3,0x4($a1)` with $a1 = the
    element base. Same address, different encoding.
The VOLATILE STORE below is load-bearing: without it reorg steals that store into the `bne`
`fermataTicks` delay slot and the restTicks `addiu` stays in the block (98.07, one swapped pair).
`*(volatile u16*)(&pElement->status_flags + 1)` and the plain `pElement->rampFlags` are the same
address; both spellings of the volatile lvalue give 99.92, and the element-base spelling
(`*(volatile u16*)&pElement->rampFlags`) does too. A 5-minute permuter run (output-66) offered moving
this store into the `elementFlags & 0x20` arm (66 vs base 127) - that is WRONG CODE (retail's single
`sh a3,0x2(a0)` sits in the shared tail and is reached from all four arms), so it was not taken.
The header's AudioManager tempoFrames/tempoTarget and AudioElement volumeStep/volumeFrames are u16
(the original loads them with lhu, not lh).
void func_8003C4C4(AudioManager* pManager, AudioElement* pElement, u32 elementCount) {
    u16 activeFlags;
    s32 count = elementCount;
    u16 tickCounter = pManager->tempoFrames;
    u32 fermataTimer;
    s32 fermataTicks;
    s32 restTicks;
    u16 statusFlags;
    u16 elementFlags;
    s32 next;

    if (tickCounter != 0) {
        tickCounter--;
        if (tickCounter != 0) {
            pManager->tempo += pManager->tempoStep;
        } else {
            pManager->tempo = pManager->tempoTarget << 16;
        }
        pManager->tickRate = (s16)(pManager->tempo >> 16) * (s16)(pManager->tempoScaleInterpolator.currentValue >> 16);
        pManager->tempoFrames = tickCounter;
    }

    do {
        activeFlags = pElement->active_flag;
        if (activeFlags != 0) {
            fermataTimer = *(u32*)&pElement->fermataDuration;
            fermataTicks = fermataTimer & 0xFFFF;
            restTicks = fermataTimer >> 16;
            statusFlags = pElement->status_flags;

            if (fermataTicks != 0) {
                elementFlags = pElement->rampFlags;

                if (elementFlags & 0x8) {
                    statusFlags |= SOUND_STATUS_VOLUME_CHANGE;
                    if (--pElement->elementVolumeFrames == 0) {
                        elementFlags &= ~0x8;
                    }
                    pElement->elementVolume += pElement->elementVolumeStep;
                }
                if (elementFlags & 0x1) {
                    statusFlags |= SOUND_STATUS_PLAYING_NTOE;
                    if (!(elementFlags & 0x2)) {
                        if (--pElement->noteFrames == 0) {
                            elementFlags &= ~0x1;
                        }
                    }
                    pElement->note += pElement->noteStep;
                }
                if (elementFlags & 0x10) {
                    if (--pElement->panFrames == 0) {
                        pElement->pan = pElement->panTarget;
                        elementFlags &= ~0x10;
                    } else {
                        pElement->pan += pElement->panStep;
                    }
                    statusFlags |= SOUND_STATUS_VOLUME_CHANGE;
                }
                if (elementFlags & 0x20) {
                    if (--pElement->volumeFrames == 0) {
                        pElement->volume = pElement->volumeTarget;
                        elementFlags &= ~0x20;
                    } else {
                        pElement->volume += pElement->volumeStep;
                    }
                    statusFlags |= SOUND_STATUS_VOLUME_CHANGE;
                }
                *(volatile u16*)(&pElement->status_flags + 1) = elementFlags;
                if (--fermataTicks == 1) {
                    if (activeFlags & 0x1000) {
                        pElement->voice_data.releaseRate = 6;
                        pElement->voice_data.flags |= 0x80;
                    }
                }
                if (--restTicks == 0) {
                    pElement->active_flag |= SOUND_CHANNEL_REST_NOTE;
                    statusFlags |= SOUND_STATUS_REST_NOTE;
                }
                *(u32*)&pElement->fermataDuration = fermataTicks + (restTicks << 16);
            }
            pElement->status_flags = statusFlags;
        }
        pElement++;
        next = count - 1;
        count = next;
    } while ((s16)next != 0);
}
*/

// 69.00%, retail's 1436 bytes against this body's 1448. The whole script walk is here - the note
// command with its two selector tables, the handler dispatch, the loop-marker scan, the gate/duration
// arithmetic and the envelope reset - and it is within 3 instructions of retail's size, but the
// register allocation cascades: retail defines $s6 (count) before $s2 (the element) in the prologue,
// hangs the two constants in $s7 = 1 / $s8 = 0x90 where this body gets them the other way round, and
// reloads active_flag a second time for the 0xF8FF mask where gcc cse keeps the first load. Retail's
// frame is 0x48 (32 bytes of locals), this body's 0x40, so the original source had two more spilled
// values than this spelling. Written from the assembly in one pass; nothing here has been tuned.
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003C6E8);
/*
// Steps one element's script until the next note starts: a note command carries the pitch and
// its duration selectors, everything else is a control command handled above.
void func_8003C6E8(AudioManager* pManager, AudioElement* pElement, s32 count) {
    u8* pScript;
    u32 activeFlags;
    u32 noteStarted;
    s32 cmd;
    u8 selector;
    u8 note;
    u8 length;
    s32 ticks;
    s32 delta;
    u16 gateTicks;
    AudioLoop* pLoop;
    AudioEnvelope* pEnvelope;
    s32 i;
    s32 next;

    do {
        if (pElement->active_flag != 0) {
            if (pElement->fermataDuration == 0) {
                noteStarted = 0;
                activeFlags = pElement->active_flag;
                pScript = (u8*)pElement->pScriptIP;
                pElement->active_flag = activeFlags & ~0x700;

                for (;;) {
                    cmd = *pScript++;
                    if (cmd < 0x80) {
                        // A note: bit 3 of the active flags keeps the running volume for a tie.
                        if (!(pElement->active_flag & 0x8)) {
                            pElement->volume = cmd << 8;
                        }
                        pElement->status_flags |= SOUND_STATUS_VOLUME_CHANGE;

                        selector = *pScript;
                        note = pElement->octave + g_SoundNoteTypePitchOffsets[selector];
                        pElement->nextNote = note;
                        pScript++;

                        length = g_SoundNoteDefaultDurations[selector];
                        if (length == 0) {
                            length = *pScript++;
                        }

                        pElement->fermataDuration = length;
                        pElement->voice_data.releaseRate = pElement->releaseRate;
                        pElement->voice_data.flags |= SOUND_VOICE_FLAG_RELEASE;

                        if (pElement->active_flag & SOUND_CHANNEL_PERCUSSION_ACTIVE) {
                            SoundSetElementPercussionTone(pManager, pElement, note);
                        } else {
                            pElement->note = ((note << 8) + pElement->detune + pElement->toneTune) << 16;
                        }

                        pElement->status_flags |= SOUND_STATUS_PLAYING_NTOE;
                        pElement->active_flag |= SOUND_CHANNEL_NOTE_ACTIVE | SOUND_CHANNEL_HOLD_NOTE;
                        if (activeFlags & SOUND_CHANNEL_REST_NOTE) {
                            pElement->status_flags |= 1;
                        }
                        noteStarted = 1;

                        if (pElement->active_flag & SOUND_CHANNEL_CHANGE_INSTRUMENT) {
                            pElement->active_flag &= ~SOUND_CHANNEL_CHANGE_INSTRUMENT;
                            pElement->voice_data.flags = 0xFFFF;
                            pElement->status_flags |= SOUND_STATUS_VOLUME_CHANGE | SOUND_STATUS_PLAYING_NTOE;
                        }
                    } else {
                        pScript = g_SoundScriptHandlers[cmd - 0x80](pScript, pManager, pElement);
                        if (pElement->active_flag == 0) {
                            pManager->activeElementMask &= ~(1 << pElement->elementIndex);
                            break;
                        }
                    }

                    if (pElement->status_flags & (SOUND_STATUS_VOLUME_CHANGE | 0x400)) {
                        break;
                    }
                }

                pElement->pScriptIP = (u32)pScript;

                if (pElement->active_flag != 0) {
                    if (pElement->active_flag & 0x800) {
                        pElement->active_flag |= SOUND_STATUS_PLAYING_NTOE;
                    }

                    pLoop = &pElement->loops[pElement->loopIndex];
                    cmd = *pScript;
                    if (cmd >= 0x80) {
                        do {
                            if (cmd == 0x90) {
                                pScript = pElement->savedScriptIP;
                                if (pScript == NULL) {
                                    break;
                                }
                            } else if (cmd == 0x80 || (cmd >= 0xB0 && cmd <= 0xB1)) {
                                pElement->status_flags &= ~SOUND_STATUS_PLAYING_NTOE;
                                break;
                            } else if (cmd == 0x81) {
                                pElement->status_flags |= SOUND_STATUS_PLAYING_NTOE;
                                break;
                            } else {
                                if (cmd == 0x99) {
                                    if (pLoop->count != 0) {
                                        pScript = pLoop->start;
                                        continue;
                                    }
                                    pLoop--;
                                }

                                if (cmd == 0x9A && pLoop->count == 0) {
                                    pScript = pLoop->end;
                                    pLoop--;
                                } else {
                                    pScript += g_SoundScriptCommandLengths[cmd - 0x80];
                                }
                            }
                        } while ((cmd = *pScript) >= 0x80);
                    }

                    if (cmd < 0x80) {
                        pElement->status_flags |= 0x1000;
                    } else {
                        pElement->status_flags &= ~0x1000;
                    }

                    ticks = pElement->durationBias + pElement->fermataDuration;
                    if ((s16)ticks <= 0) {
                        ticks = pElement->fermataDuration + ticks;
                        pElement->durationBias = pElement->durationBias + (u8)pElement->fermataDuration;
                    }

                    if (pElement->active_flag & 0x600) {
                        gateTicks = 0x7FFF;
                    } else if (pElement->gateRatio == 0x10) {
                        gateTicks = ticks;
                    } else {
                        if (pElement->gateRatio == 0xF) {
                            gateTicks = ticks - 1;
                        } else {
                            gateTicks = (u32)((s16)ticks * pElement->gateRatio) >> 4;
                        }

                        if (gateTicks == 0) {
                            gateTicks = 1;
                        }
                    }

                    *(u32*)&pElement->fermataDuration = (s16)ticks + (gateTicks << 16);

                    if (noteStarted != 0) {
                        if (pElement->rampFlags & 0x4) {
                            delta = (pElement->nextNote - pElement->currentNote) << 24;
                            if (delta != 0) {
                                pElement->rampFlags |= 0x1;
                                pElement->noteFrames = pElement->slideFrames;
                                pElement->note = ((pElement->currentNote << 8) + pElement->detune + pElement->toneTune) << 16;
                                pElement->noteStep = delta / pElement->slideFrames;
                            }
                        }

                        pElement->currentNote = pElement->nextNote;

                        if (pElement->rampFlags & 0x100) {
                            pElement->elementVolumeFrames = pElement->ramp.counter;
                            pElement->elementVolumeStep = pElement->ramp.step;
                            pElement->elementVolume = pElement->ramp.target << 16;
                            pElement->rampFlags |= 0x8;
                        }

                        pEnvelope = pElement->envelopes;
                        for (i = NUM_VOICE_ENVELOPES; i != 0; i--) {
                            if ((pEnvelope->flags & 3) == 3) {
                                pEnvelope->value = 0;
                                pEnvelope->count = 1;
                                pEnvelope->delayCount = pEnvelope->delayReload;
                                pEnvelope->rampLevel = pEnvelope->rampStep;
                                pElement->status_flags |= SOUND_STATUS_VOLUME_CHANGE;
                                pEnvelope->flags &= ~0xC;
                            }
                            pEnvelope++;
                        }
                    }
                }
            }
        }

        pElement++;
        next = count - 1;
        count = next;
    } while ((s16)next != 0);
}
*/

void SoundSetElementPercussionTone(AudioManager* pAudioManager, AudioElement* pElement, u8 percussionIndex) {
    SoundPercussionTone* pTone;
    s32 note;
    u16 statusFlags;

    pTone = (SoundPercussionTone*)((percussionIndex * 4) + pAudioManager->pPercussionData);

    SoundApplyToneToElement(pTone->tone, pElement);

    note = (pTone->tune << 8) + pElement->detune + pElement->toneTune;
    pElement->note = note << 16;
    statusFlags = pElement->status_flags;
    pElement->pan = pTone->pan << 8;
    pElement->status_flags = statusFlags | SOUND_STATUS_VOLUME_CHANGE;
}

u8* SoundScriptDefaultHandler(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    return pScript;
}

// Rest note handler?
u8* SoundScriptRestNote(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
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

u8* SoundScriptConditionalSaveIP(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
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

u8* SoundScriptRepeatOrEnd(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8* pSavedScript = pAudioElements->savedScriptIP;

    if (pSavedScript) {
        pScript = pSavedScript;
        pAudioElements->loopCount++;
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
u8* SoundScriptSetTimeSignature(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 divisor = pScript[1];
    u16 step = SOUND_TICKS_PER_WHOLE_NOTE / divisor;
    u16 value = *pScript;

    pAudioManager->ticksPerBeat = step;
    pAudioManager->beatDivisor = divisor;
    pAudioManager->beatsPerBar = value;
    pAudioManager->unk_0x3e = value;
    pAudioManager->tickCount = pAudioManager->ticksPerBeat;
    return pScript + 2;
}

u8* SoundScriptSetManagerUnk32And34(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioManager->barCount = pScript[0];
    pAudioManager->beatCount = pScript[1];
    pAudioManager->tickCount = pAudioManager->ticksPerBeat;
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
u8* SoundScriptLoopStart(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    AudioLoop* pLoop;

    pAudioElements->loopIndex++;
    pLoop = &pAudioElements->loops[pAudioElements->loopIndex];
    pLoop->count = *pScript++ - 1;
    pLoop->start = pScript;
    pLoop->savedOctave = pAudioElements->octave;
    return pScript;
}

// Loop end handler?
u8* SoundScriptLoopEnd(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    AudioLoop* pLoop = &pAudioElements->loops[pAudioElements->loopIndex];
    u8* pResult = pScript;

    if (--pLoop->count != 0xFF) {
        pLoop->end = pScript;
        pLoop->endOctave = pAudioElements->octave;
        pResult = pLoop->start;
        pAudioElements->octave = pLoop->savedOctave;
    } else {
        pAudioElements->loopIndex--;
    }

    return pResult;
}

u8* SoundScriptLoopExitIfDone(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    AudioLoop* pLoop = &pAudioElements->loops[pAudioElements->loopIndex];
    u8* pResult = pScript;

    if (pLoop->count == 0) {
        pResult = pLoop->end;
        pAudioElements->octave = pLoop->endOctave;
        pAudioElements->loopIndex--;
    }

    return pResult;
}

u8* SoundScriptSendCommandByIdWithLevels(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    SoundSendCommandByIdWithLevels(pScript[0] | (pScript[1] << 8), 0x7F, 0x40);
    return pScript + 3;
}

u8* SoundScriptStopElementsById(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    SoundStopElementsById(pScript[0] | (pScript[1] << 8));
    return pScript + 2;
}

u8* SoundScriptJumpToInstrumentScript(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    SoundFile* pSoundFile = g_SoundSedsLinkedList;
    s16 sedId = pAudioElements->soundId >> 16;
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

    // scriptOffsets holds byte offsets from the start of the file image, so the target is base + offset
    pScript = (u8*)pSoundFile + pSoundFile->scriptOffsets[instrument * 2 + channel];
    return pScript + 3;
}

// Set tempo handler?
u8* SoundScriptSetTempo(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 value = *pScript++;

    pAudioManager->tempo = value << 16;
    pAudioManager->tickRate = (value & 0xFF) * (s16)(pAudioManager->tempoScaleInterpolator.currentValue >> 16);
    return pScript;
}

u8* SoundScriptAddTempo(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 pad[2]; // unused: reproduces the original's 0x8-byte frame
    s8 value = *pScript;

    pAudioManager->tempo += value << 16;
    pAudioManager->tickRate = 0;
    return pScript + 1;
}
/*
Attempt (67.27%): the original has an unused 0x8-byte stack frame
(`addiu sp,sp,-0x8` ... `addiu sp,sp,0x8`) and computes the return value before the
epilogue, so its source had something that forces a frame this form does not.
u8* SoundScriptAddTempo(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s8 value = *pScript;

    pAudioManager->tempo += value << 16;
    pAudioManager->tickRate = 0;
    return pScript + 1;
}
*/

u8* SoundScriptRampTempo(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s16 unk0 = pScript[0];
    u8 unk1 = pScript[1];
    s32 delta;

    pAudioManager->tempoTarget = unk1;
    delta = (unk1 << 16) - pAudioManager->tempo;
    if (unk0 != 0 && delta != 0) {
        pAudioManager->tempoFrames = unk0;
        pAudioManager->tempoStep = delta / unk0;
    }
    return pScript + 2;
}

u8* SoundScriptSetVolume(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioManager->volumeInterpolator.currentValue = *pScript++ << 24;
    unk_SoundSetFlagsOnActiveVoices(SOUND_STATUS_VOLUME_CHANGE, pAudioManager);
    return pScript;
}

u8* SoundScriptRampVolume(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 step = pScript[0];
    u8 target = pScript[1];
    u16 counter = step << 5;
    s32 delta = (target << 24) - pAudioManager->volumeInterpolator.currentValue;

    if ((step << 5) != 0 && delta != 0) {
        pAudioManager->volumeInterpolator.counter = counter;
        pAudioManager->volumeInterpolator.targetValue = target << 8;
        pAudioManager->volumeInterpolator.stepIncrement = delta / (step << 5);
    }
    return pScript + 2;
}

u8* SoundScriptSetUnk62(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->gateRatio = *pScript;
    return pScript + 1;
}

u8* SoundScriptSetVoice(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 note;

    note = *pScript++;

    if (note < 25) {
        SoundReleaseVoiceFromChannel(&pAudioElements->voice_data, pAudioElements->voice_number);
        SoundAssignVoiceToChannelAndStop(&pAudioElements->voice_data, pAudioElements->voice_number = note);
    }

    return pScript;
}

u8* SoundScriptSetTone(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    SoundApplyToneToElement(*pScript++, pAudioElements);
    return pScript;
}

u8* SoundScriptAddUnk60(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 value = *pScript++;

    if (value) {
        pAudioElements->durationBias += value;
    } else {
        pAudioElements->durationBias = 0;
    }

    return pScript;
}

// Percussion On handler?
u8* SoundScriptPercussionOn(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    if (pAudioManager->pPercussionData) {
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

u8* SoundScriptSetVoiceFlags1000AndMode(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    if (pAudioElements->voice_number & 0x1) {
        pAudioElements->voice_data.flags |= 0x1000;
        pAudioElements->voice_data.modeFlags |= 0x10;
    }

    return pScript;
}

u8* SoundScriptSetVoiceFlags1000ClearMode(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    if (pAudioElements->voice_number & 0x1) {
        pAudioElements->voice_data.flags |= 0x1000;
        pAudioElements->voice_data.modeFlags &= ~0x10;
    }

    return pScript;
}

u8* SoundScriptSetNoiseClock(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioManager->noiseClock = *pScript++;
    SpuSetNoiseClock(pAudioManager->noiseClock);
    pAudioElements->voice_data.flags |= 0x2000;
    pAudioElements->voice_data.modeFlags |= 0x20;
    return pScript;
}

u8* SoundScriptAddNoiseClock(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioManager->noiseClock = (pAudioManager->noiseClock + *pScript++) & 0x3F;
    SpuSetNoiseClock(pAudioManager->noiseClock);
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

u8* SoundScriptSetReverb(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
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

u8* SoundScriptSetVoiceFlags4000AndMode(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    if (pAudioManager->flags & 0x6) {
        if ((g_SoundControlFlags & 0x2000) == 0) {
            return pScript;
        }
        if (pAudioElements->active_flag & 0x2) {
            // redundant test: the two returns are what puts the return value in this branch's delay slot
            if (pAudioManager != NULL) {
                return pScript;
            }
            return pScript;
        }
    }

    pAudioElements->voice_data.flags |= 0x4000;
    pAudioElements->voice_data.modeFlags |= 0x40;
    return pScript;
}

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
    SoundApplyToneToElement(pAudioElements->toneIndex, pAudioElements);
    return pScript;
}

u8* SoundScriptSetAdsrModes(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->voice_data.attackMode = pScript[0];
    pAudioElements->voice_data.sustainMode = pScript[1];
    pAudioElements->voice_data.releaseMode = pScript[2];
    pAudioElements->voice_data.flags |=
        SOUND_VOICE_FLAG_ATTACK | SOUND_VOICE_FLAG_DECAY | SOUND_VOICE_FLAG_SUSTAIN |
        SOUND_VOICE_FLAG_RELEASE | SOUND_VOICE_FLAG_SUSTAIN_LEVEL;
    return pScript + 3;
}

u8* SoundScriptSetAttackTime(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 attackTime = *pScript;
    pAudioElements->voice_data.flags |= SOUND_VOICE_FLAG_ATTACK;
    pAudioElements->voice_data.attackRate = attackTime;
    return pScript + 1;
}

u8* SoundScriptSetDecayTime(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 decayTime = *pScript;
    pAudioElements->voice_data.flags |= SOUND_VOICE_FLAG_DECAY;
    pAudioElements->voice_data.adsrDR = decayTime;
    return pScript + 1;
}

u8* SoundScriptSetSustain(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 temp = *pScript;
    pAudioElements->voice_data.flags |= SOUND_VOICE_FLAG_SUSTAIN;
    pAudioElements->voice_data.sustainRate = temp;
    return pScript + 1;
}

u8* SoundScriptSetRelease(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 temp = *pScript;
    pAudioElements->voice_data.flags |= SOUND_VOICE_FLAG_RELEASE;
    pAudioElements->releaseRate = temp;
    pAudioElements->voice_data.releaseRate = temp;
    return pScript + 1;
}

u8* SoundScriptSetSustainLevel(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 temp = *pScript;
    pAudioElements->voice_data.flags |= SOUND_VOICE_FLAG_SUSTAIN_LEVEL;
    pAudioElements->voice_data.adsrSR = temp;
    return pScript + 1;
}

// Set decay and sustain?
u8* SoundScriptSetAdsrDRAndSR(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->voice_data.adsrDR = *pScript;
    pAudioElements->voice_data.adsrSR = *(pScript + 1);
    pAudioElements->voice_data.flags |= SOUND_VOICE_FLAG_DECAY | SOUND_VOICE_FLAG_SUSTAIN_LEVEL;
    return pScript + 2;
}

u8* SoundScriptSetAttackMode(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 attackMode = *pScript;
    pAudioElements->voice_data.flags |= SOUND_VOICE_FLAG_ATTACK;
    pAudioElements->voice_data.attackMode = attackMode;
    return pScript + 1;
}

u8* SoundScriptSetSustainMode(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 sustainMode = *pScript;
    pAudioElements->voice_data.flags |= SOUND_VOICE_FLAG_SUSTAIN;
    pAudioElements->voice_data.sustainMode = sustainMode;
    return pScript + 1;
}

u8* SoundScriptSetReleaseMode(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 releaseMode = *pScript;
    pAudioElements->voice_data.flags |= SOUND_VOICE_FLAG_RELEASE;
    pAudioElements->voice_data.releaseMode = releaseMode;
    return pScript + 1;
}

u8* SoundScriptSetUnk6E(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s16 temp = (s8)*pScript << 5;
    pAudioElements->detune = temp;
    pAudioElements->status_flags |= SOUND_STATUS_PLAYING_NTOE;
    return pScript + 1;
}

u8* SoundScriptAddUnk6E(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->detune += (s8)*pScript << 5;
    pAudioElements->status_flags |= SOUND_STATUS_PLAYING_NTOE;
    return pScript + 1;
}

u8* SoundScriptAddUnk6EFine(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->detune += (s8)*pScript << 3;
    pAudioElements->status_flags |= SOUND_STATUS_PLAYING_NTOE;
    return pScript + 1;
}

u8* SoundScriptAddUnk6EWord(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->detune += pScript[1] + ((s8)pScript[0] << 8);
    pAudioElements->status_flags |= SOUND_STATUS_PLAYING_NTOE;
    return pScript + 2;
}

u8* SoundScriptSetUnk84Ramp(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u16 value = pScript[0];
    s32 delta = (s8)pScript[1] << 24;

    if (value != 0 && delta != 0) {
        pAudioElements->noteFrames = value;
        pAudioElements->rampFlags |= 0x1;
        pAudioElements->noteStep = delta / value;
    } else {
        pAudioElements->rampFlags &= ~0x1;
    }
    return pScript + 2;
}

u8* SoundScriptToggleUnk04Bit2(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->rampFlags ^= 0x2;
    return pScript;
}

u8* SoundScriptClearUnk04Bit1(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->rampFlags &= ~0x1;
    return pScript;
}

u8* SoundScriptSetUnk70(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 value = *pScript++;

    pAudioElements->slideFrames = value;
    if (value) {
        pAudioElements->rampFlags |= 0x4;
    } else {
        pAudioElements->rampFlags &= ~0x4;
    }

    return pScript;
}

u8* SoundScriptSetTriangleEnvelope0(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 note = (s8)pScript[1];
    s16 velocity = pScript[0];
    s32 unk16;

    if (note != 0) {
        if (velocity != 0) {
            // the note offset squared with its sign kept: the pitch curve
            if (note < 0) {
                note = note * -note;
            } else {
                note = note * note;
            }
            velocity = velocity + ((s16)velocity * (s16)velocity) / 64;
            pAudioElements->envelopes[0].delta = SoundCalculateEnvelopeDelta(note << 14, velocity, 3);
            pAudioElements->envelopes[0].reload = velocity;
            unk16 = pScript[2] << 2;
            pAudioElements->envelopes[0].rampStep = 0x400;
            pAudioElements->envelopes[0].pStepHandler = (undefined32)SoundEnvelopeStepTriangleHalfRate;
            pAudioElements->envelopes[0].handlerIndex = 3;
            pAudioElements->envelopes[0].flags = 3;
            pAudioElements->envelopes[0].target = 0;
            pAudioElements->envelopes[0].delayReload = unk16;
            pAudioElements->activeEnvelopeMask |= 0x1;
            SoundResetEnvelope(&pAudioElements->envelopes[0]);
        }
    }

    return pScript + 3;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003D9A4);
/*
Screened under gcc-2.7.2-cdk / gcc-2.7.2 / this unit's gcc-2.6.0: 61.87 / 65.37 / 63.84 - not
a compiler-only match; the current preset is the best of the three.
Attempt (63.84%, 268 bytes vs the original's 280): the shape is right (pitch = note * abs(note)
via the if/else, volume = velocity + velocity^2/64, the call and the envelopes[0] store order all
line up) but gcc mirrors the callee-saved allocation: it puts pAudioElements in s2 and the
velocity in a2/v1, where the original has pAudioElements s1 (copied in the first beqz delay slot),
velocity s2, index s3, flags s0. `s32 velocity`/`s32 note` (in either declaration order) and
u8/s16 local types all give the same mirror. A second miss: `((mode & 0x10) == 0) * 2 + 1` becomes
a two-way branch here instead of the original's sltiu/sll/addiu, and `(s16)velocity * (s16)velocity`
is elided unless the local is widened to s32. Compare the parked SoundScriptSetTriangleEnvelope0, the same function
with a constant mode, which is stuck at 92.34% on the same allocation mirror.
u8* func_8003D9A4(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s8 note = pScript[1];
    s16 velocity = pScript[0];
    u8 mode = pScript[2];
    s32 pitch;
    s32 volume;

    if (note != 0 && velocity != 0) {
        if (note < 0) {
            pitch = note * -note;
        } else {
            pitch = note * note;
        }
        volume = velocity + (velocity * velocity) / 64;
        pAudioElements->envelopes[0].delta = SoundCalculateEnvelopeDelta(pitch << 14, (s16)volume, mode & 0xF);
        pAudioElements->envelopes[0].rampStep = 0x400;
        pAudioElements->envelopes[0].reload = volume;
        pAudioElements->envelopes[0].delayReload = 0;
        pAudioElements->envelopes[0].handlerIndex = mode & 0xF;
        pAudioElements->envelopes[0].target = 0;
        pAudioElements->envelopes[0].flags = ((mode & 0x10) == 0) * 2 + 1;
        pAudioElements->envelopes[0].pStepHandler = g_SoundEnvelopeStepHandlers[mode & 0xF];
        pAudioElements->activeEnvelopeMask |= 0x1;
        SoundResetEnvelope(&pAudioElements->envelopes[0]);
    }

    return pScript + 3;
}
*/

u8* SoundScriptSetEnvelope0Rate(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 divisor = (u8)(*pScript + 1);
    s16 step;

    pScript++;
    if (divisor) {
        divisor *= 4;
        step = 0x400 / divisor;
        pAudioElements->envelopes[0].rampStep = step;
        pAudioElements->envelopes[0].rampLevel = step;
    }

    return pScript;
}

u8* SoundScriptSetUnkCEAndF6(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->activeEnvelopeMask |= 0x1;
    pAudioElements->envelopes[0].flags |= SOUND_ENVELOPE_FLAG_ENABLED;
    return pScript;
}

u8* SoundScriptClearUnkCEAndF6(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->activeEnvelopeMask &= ~0x1;
    pAudioElements->envelopes[0].flags &= ~SOUND_ENVELOPE_FLAG_ENABLED;
    return pScript;
}

// Set volume handler / Dynamic?
u8* SoundScriptSetElementVolume(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->elementVolume = *pScript << 24;
    pAudioElements->status_flags |= SOUND_STATUS_VOLUME_CHANGE;
    pAudioElements->rampFlags &= ~0x108;
    return pScript + 1;
}

// Crescendo?
u8* SoundScriptAddElementVolume(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->elementVolume = (pAudioElements->elementVolume + ((s8)*pScript << 24)) & 0x7FFFFFFF;
    pAudioElements->status_flags |= SOUND_STATUS_VOLUME_CHANGE;
    pAudioElements->rampFlags &= ~0x108;
    return pScript + 1;
}

u8* SoundScriptSetElementVolumeRamp(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u16 frames = *pScript;
    s32 delta = ((s8)pScript[1] << 24) - pAudioElements->elementVolume;

    if (frames && delta) {
        pAudioElements->elementVolumeFrames = frames;
        pAudioElements->rampFlags = (pAudioElements->rampFlags | 0x8) & ~0x100;
        pAudioElements->elementVolumeStep = delta / frames;
    }

    return pScript + 2;
}

u8* SoundScriptSetElementRamp(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 start = pScript[0] << 24;
    s32 target = pScript[2] << 24;
    s16 frames = pScript[1];
    s32 delta = target - start;

    if (delta != 0 && frames != 0) {
        pAudioElements->ramp.step = delta / frames;
        pAudioElements->ramp.target = start >> 16;
        pAudioElements->ramp.counter = frames;
        pAudioElements->rampFlags = (pAudioElements->rampFlags | 0x100) & ~0x8;
    } else {
        pAudioElements->rampFlags &= ~0x100;
    }

    return pScript + 3;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003DC50);
/*
Screened under gcc-2.7.2-cdk / gcc-2.7.2 / this unit's gcc-2.6.0: 88.64 / 88.64 / 92.60 - not
a compiler-only match; the current preset is the best of the three.
Attempt (93.36%, 204 bytes, same size; ideally the register roles swap). gcc puts the running
volume in s0 and pAudioElements in s1; the original has volume s1, pAudioElements s0 (the
prologue save order follows). It also elides the `sll a1,16` / `sra a1,16` that converts the
divisor argument, because gcc proves from the lbu that velocity, velocity*velocity/64 and
their sum fit in HImode - the original could not prove it. `s32 velocity = pScript[0]` with
`velocity + (velocity * velocity) / 64` (no s16 casts) removes the spurious `andi 0xff` and
the extra `move v0,s1` (92.60%, 200 bytes: it drops the sll/sra pair) - that shape is the
closest to the original's instruction sequence. `s16 volume`, `(s16)velocity` in the product
and u8 velocity all land lower (90.34%).
u8* func_8003DC50(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 note = (s8)pScript[1];
    s32 velocity = pScript[0];
    s32 volume;
    s32 unk16;

    if (note != 0) {
        if (velocity != 0) {
            volume = velocity + (velocity * velocity) / 64;
            pAudioElements->envelopes[1].delta = SoundCalculateEnvelopeDelta(note << 24, (s16)volume, 2);
            pAudioElements->envelopes[1].reload = volume;
            unk16 = pScript[2] << 2;
            pAudioElements->envelopes[1].rampStep = 0x400;
            pAudioElements->envelopes[1].pStepHandler = (undefined32)SoundEnvelopeStepTriangle;
            pAudioElements->envelopes[1].handlerIndex = 2;
            pAudioElements->envelopes[1].target = 1;
            pAudioElements->envelopes[1].flags = 3;
            pAudioElements->envelopes[1].delayReload = unk16;
            pAudioElements->activeEnvelopeMask |= 0x2;
            SoundResetEnvelope(&pAudioElements->envelopes[1]);
        }
    }

    return pScript + 3;
}
*/


INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003DD24);
/*
Attempt (78.85%, 232 bytes against retail's 244; the separate velocity/volume version) and
(78.28%, 240 bytes; the single running local below). Round 333: the whole tail now matches
retail's store set and order (rampStep, reload, delayReload, target, handlerIndex,
`flags + 1`, mask |= 2, the call in the last sh's slot) and the sltiu/sll flag computation
comes out right once `flags` is its own local computed BEFORE the call (`flags + 1` at the
store site is what puts retail's late addiu s0,s0,1 there). Residual, in the head only:
retail loads velocity straight into s2 and copies it to v0 for the test and the multiply
(`move v0,s2; beqz v0; mult v0,v0`), then re-uses s2 for the volume; gcc puts the lbu in a1,
keeps it there, and moves the sum into s2. That single missing copy plus the callee-saved
roles (retail mode/value s3 and pAudioElements s1; ours value s1 and pAudioElements s3, i.e.
a mirror) is the whole delta. Tried and measured this round: declaration order both ways,
`flags` before `value` (72.89), and the parked variants below (69.46 / 71.39).
u8* func_8003DD24(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s8 note = pScript[1];
    u8 mode = pScript[2];
    s32 value;
    s32 flags;
    s32 volume = pScript[0];

    if (note != 0) {
        if (volume != 0) {
            volume += (volume * volume) / 64;
            value = mode & 0xF;
            flags = ((mode & 0x10) == 0) * 2;
            pAudioElements->envelopes[1].delta = SoundCalculateEnvelopeDelta(note << 24, (s16)volume, value);
            pAudioElements->envelopes[1].rampStep = 0x400;
            pAudioElements->envelopes[1].reload = volume;
            pAudioElements->envelopes[1].delayReload = 0;
            pAudioElements->envelopes[1].target = 1;
            pAudioElements->envelopes[1].pStepHandler = g_SoundEnvelopeStepHandlers[value];
            pAudioElements->envelopes[1].handlerIndex = value;
            pAudioElements->envelopes[1].flags = flags + 1;
            pAudioElements->activeEnvelopeMask |= 0x2;
            SoundResetEnvelope(&pAudioElements->envelopes[1]);
        }
    }

    return pScript + 3;
}
*/
/*
Attempt (69.46%, 244 bytes vs the original's 236). pScript reaches s4, volume s2 and the step
handler table load v1, so the head and the tail order are right. Still different: (1) the flags
test - the original is andi s0,s3,0x10 + sltiu s0,s0,1 + sll + addiu 1, but gcc 2.6 turns every
spelling tried (an inline == 0 or != 0 comparison, or a `== 0` / `< 1` / `!x` local multiplied by
2) into either a bnez pair or srl/xori/andi bit extraction; (2) the third call argument reaches a2
through a move in the jal delay slot where the original reuses the andi a2,s3,0xF computed before
the division; (3) with the flags local present pAudioElements moves from s1 to s3. Reusing one
variable for the velocity and the volume (`volume += volume * volume / 64`) is what puts them in a
single register; separate velocity/volume locals come out at 59.39% in exactly 236 bytes.
// u8* func_8003DD24(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
//     s32 note = (s8)pScript[1];
//     u8 unk2 = pScript[2];
//     s32 value;
//     s32 unk10;
//     s32 volume = pScript[0];

//     if (note != 0) {
//         if (volume != 0) {
//             volume += (volume * volume) / 64;
//             value = unk2 & 0xF;
//             unk10 = !(unk2 & 0x10);
//             pAudioElements->envelopes[1].delta = SoundCalculateEnvelopeDelta(note << 24, (s16)volume, value);
//             pAudioElements->envelopes[1].rampStep = 0x400;
//             pAudioElements->envelopes[1].reload = volume;
//             pAudioElements->envelopes[1].delayReload = 0;
//             pAudioElements->envelopes[1].target = 1;
//             pAudioElements->envelopes[1].flags = unk10 * 2 + 1;
//             pAudioElements->envelopes[1].pStepHandler = g_SoundEnvelopeStepHandlers[value];
//             pAudioElements->envelopes[1].handlerIndex = value;
//             pAudioElements->activeEnvelopeMask |= 0x2;
//             SoundResetEnvelope(&pAudioElements->envelopes[1]);
//         }
//     }

//     return pScript + 3;
// }
*/
/*
Attempt (71.39%, same size as the original's 236 bytes): the tail is present in the
original's order (delta, 0x400, reload, 0, step-handler table, target, handlerIndex,
flags, activeEnvelopeMask, call), but gcc loads the table entry into a1 instead of v1, drops the
`lhu v0,0xCE` / `ori` from before the table load, and forms the envelope call base a0
much earlier. `flags = ((value & 0x10) == 0) * 2 + 1` does produce the original's
branchless sltiu/sll/addiu, and `(s16)volume` the sll/sra pair.
*/

u8* SoundScriptSetEnvelope1Rate(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 divisor = (u8)(*pScript + 1);
    s16 step;

    pScript++;
    if (divisor) {
        divisor *= 4;
        step = 0x400 / divisor;
        pAudioElements->envelopes[1].rampStep = step;
        pAudioElements->envelopes[1].rampLevel = step;
    }

    return pScript;
}

u8* SoundScriptSetUnkCEAndUnk116(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->activeEnvelopeMask |= 0x2;
    pAudioElements->envelopes[1].flags |= SOUND_ENVELOPE_FLAG_ENABLED;
    return pScript;
}

u8* SoundScriptClearUnkCEAndUnk116(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->activeEnvelopeMask &= ~0x2;
    pAudioElements->envelopes[1].flags &= ~SOUND_ENVELOPE_FLAG_ENABLED;
    return pScript;
}

// Pan / Balance handler?
u8* SoundScriptSetUnk74(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->pan = *pScript << 8;
    pAudioElements->status_flags |= SOUND_STATUS_VOLUME_CHANGE;
    return pScript + 1;
}

u8* SoundScriptAddElementPan(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->pan += (s8)*pScript << 8;
    pAudioElements->pan &= 0x7FFF;
    pAudioElements->status_flags |= SOUND_STATUS_VOLUME_CHANGE;
    return pScript + 1;
}

u8* SoundScriptStartPanRamp(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 volume = pAudioElements->pan;
    s32 target = (s8)pScript[1];
    u16 frames = pScript[0];
    s32 current = (s8)(volume >> 8);
    s32 delta;
    s32 step;
    s32 panDistance;

    panDistance = target - current;
    if (frames) {
        delta = panDistance;
        if (delta != 0) {
            delta <<= 8;
            step = delta / frames;
            pAudioElements->panTarget = delta;
            pAudioElements->panFrames = frames;
            pAudioElements->rampFlags |= 0x10;
            pAudioElements->panStep = step;
        }
    }

    return pScript + 2;
}

u8* SoundScriptSetEnvelope2Rate(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 divisor = (u8)(*pScript + 1);
    s16 step;

    pScript++;
    if (divisor) {
        divisor *= 4;
        step = 0x400 / divisor;
        pAudioElements->envelopes[2].rampStep = step;
        pAudioElements->envelopes[2].rampLevel = step;
    }

    return pScript;
}

u8* SoundScriptSetTriangleEnvelope2(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s8 note = pScript[1];
    s16 velocity = pScript[0];
    s32 unk16;

    if (note != 0) {
        if (velocity != 0) {
            velocity = velocity + ((s16)velocity * (s16)velocity) / 64;
            pAudioElements->envelopes[2].delta = SoundCalculateEnvelopeDelta(note << 24, (s16)velocity, 3);
            pAudioElements->envelopes[2].reload = velocity;
            unk16 = pScript[2] << 2;
            pAudioElements->envelopes[2].rampStep = 0x400;
            pAudioElements->envelopes[2].pStepHandler = (undefined32)SoundEnvelopeStepTriangleHalfRate;
            pAudioElements->envelopes[2].handlerIndex = 3;
            pAudioElements->envelopes[2].target = 2;
            pAudioElements->envelopes[2].flags = 3;
            pAudioElements->envelopes[2].delayReload = unk16;
            pAudioElements->activeEnvelopeMask |= 0x4;
            SoundResetEnvelope(&pAudioElements->envelopes[2]);
        }
    }

    return pScript + 3;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003E04C);
/*
Screened under gcc-2.7.2-cdk / gcc-2.7.2 / this unit's gcc-2.6.0: 57.67 / 67.11 / 67.52 - not
a compiler-only match; the current preset is the best of the three.
Attempt (67.52%, 232 bytes vs the original's 240): every store and the sltiu/sll
flag computation are right; gcc gives the AudioElement s2 and volume s1 (original:
element s1, volume s2), re-uses the velocity copy for the `addu` that forms volume so
the original's extra `move s2,a1` is missing, and moves the `addiu s0,s0,1` and the
`lhu 0xCE` earlier. u8/s32 velocity, value &= 0xF vs a separate index local, note/
velocity/value declaration order, a pEnvelope local and store order all land between
59.31% and 67.52%. type of pScript[2] is u8.
u8* func_8003E04C(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 velocity = pScript[0];
    s32 note = (s8)pScript[1];
    s32 value = pScript[2];
    s32 volume;
    s32 flags;

    if (note != 0) {
        if (velocity != 0) {
            volume = velocity + (velocity * velocity) / 64;
            flags = ((value & 0x10) == 0) * 2;
            value &= 0xF;
            pAudioElements->envelopes[2].delta = SoundCalculateEnvelopeDelta(note << 24, (s16)volume, value);
            pAudioElements->envelopes[2].rampStep = 0x400;
            pAudioElements->envelopes[2].reload = volume;
            pAudioElements->envelopes[2].delayReload = 0;
            pAudioElements->envelopes[2].target = 2;
            pAudioElements->envelopes[2].handlerIndex = value;
            pAudioElements->envelopes[2].flags = flags + 1;
            pAudioElements->envelopes[2].pStepHandler = g_SoundEnvelopeStepHandlers[value];
            pAudioElements->activeEnvelopeMask |= 0x4;
            SoundResetEnvelope(&pAudioElements->envelopes[2]);
        }
    }

    return pScript + 3;
}
*/

u8* SoundScriptSetUnkCEAndUnk136(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->activeEnvelopeMask |= 0x4;
    pAudioElements->envelopes[2].flags |= SOUND_ENVELOPE_FLAG_ENABLED;
    return pScript;
}

u8* SoundScriptClearUnkCEAndUnk136(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->activeEnvelopeMask &= ~0x4;
    pAudioElements->envelopes[2].flags &= ~SOUND_ENVELOPE_FLAG_ENABLED;
    return pScript;
}

u8* SoundScriptSelectEnvelope(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    AudioEnvelope* pEnvelope;
    u8 value;
    u8 last;

    pAudioElements->envelopeIndex = pScript[0];
    pEnvelope = &pAudioElements->envelopes[pAudioElements->envelopeIndex];
    value = pScript[1];
    pEnvelope->handlerIndex = value & 0xF;
    pEnvelope->pStepHandler = g_SoundEnvelopeStepHandlers[pEnvelope->handlerIndex];

    if ((value & 0x10) == 0) {
        pEnvelope->flags = 2;
    } else {
        pEnvelope->flags = 0;
    }

    last = pScript[2];
    pEnvelope->rampStep = 0x400;
    pEnvelope->delayReload = 0;
    pEnvelope->target = last;
    return pScript + 3;
}

// Round 66: 85.66% (148 bytes, right size). Two permuter runs (630 -> 305 -> 265) plus hand
// shapes; the body below is the best found. Split the multiply from the divide (`pitch = x*x;`
// then `pitch / 64` at the use) - that is what puts the `mult` early and `mflo` after the
// pAudioElements->envelopeIndex scale in retail. `note` must be a u8 COPY of pScript[0] taken after
// the multiply, otherwise gcc reuses the single load for the sum (retail reloads it into a1).
// The `do { } while (0)` is load-bearing: it is the only shape found that gives retail's
// prologue (pScript s1, envelope s2; without it the two are mirrored and the score is 83.37) -
// it is also why this is parked and not shipped. THE REMAINING 2-INSTRUCTION DELTA: retail
// loads `lbu a0,2(s1)` and `lbu a2,29(s2)` BEFORE the volume `addu a1,a1,v0`; ours emits both
// after it. Everything else, including the call and the two envelope stores, is exact.
// Round 157: 86.18%, from the permuter sweep (265 -> 245). The edit that gained: two statements
// into one `int` temp (`new_var = pScript[2] << 16;` then `new_var = (s8)pScript[1] << 24 | new_var;`)
// instead of one nested expression - that also turns retail's `or a0,v0,a0` into ours.
// A further permuter run from this body found nothing (245 -> 245). Computing the value BEFORE
// the volume line to hoist its loads collapses the shape to 62.34% (wrong size), do not retry.
// Still 2 loads late: retail loads pScript[2] and handlerIndex BEFORE `addu a1,a1,v0` because it
// keeps the first pScript[0] load in v0 (ours keeps it in a1 and skips the reload); allocator tie.
/*
u8* func_8003E1F8(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 pitch;
    AudioEnvelope* pEnvelope;
    s32 volume;
    u8 note;
    int new_var;

    volume = pScript[0];
    pitch = volume * volume;
    note = volume;
    pEnvelope = &pAudioElements->envelopes[pAudioElements->envelopeIndex];
    do {
        volume = note + pitch / 64;
        new_var = pScript[2] << 16;
        new_var = ((s8)pScript[1] << 24) | new_var;
        pEnvelope->delta = SoundCalculateEnvelopeDelta(new_var, volume, pEnvelope->handlerIndex);
        pEnvelope->reload = volume;
        return pScript + 3;
    } while (0);
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003E1F8);


s32 SoundCalculateEnvelopeDelta(s32 value, s16 divisor, s16 mode) {
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

u8* SoundScriptSetCurrentEnvelopeRate(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 divisor = pScript[1] + 1;
    AudioEnvelope* pEnvelope = &pAudioElements->envelopes[pAudioElements->envelopeIndex];
    s16 step;

    if (divisor) {
        s32 scaledDivisor = divisor * 4;

        step = 0x400 / scaledDivisor;
        pEnvelope->delayReload = *pScript << 2;
        pEnvelope->rampStep = step;
        pEnvelope->rampLevel = step;
    }

    return pScript + 2;
}

u8* SoundScriptNop5(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    return pScript;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003E360);
/*
Round 190 attempt: 95.31% (124 bytes, the original's size). The prologue, the
s-register assignment (pScript s1, pAudioElements s2, envelope s0, index s3) and the
statement order now all match. Two levers got there: the do-while wrapping the two
closing statements (it looks gratuitous and is not - it is what fixes the
s-registers; the old 94.84% note already had it as `pScript += 1; return pScript;`),
and the u16 mask local instead of the shift inline (94.84 -> 95.31).
What is left is one coalescing: gcc computes the increment into the return register
(`addiu v0,s1,1`) and never writes s1, which then pushes the post-call temps into
v1/a0, where retail increments in place (`addiu s1,s1,1`), keeps v0 for the flags
temp and the mask base and closes with `move v0,s1`. Tried: return pScript + 1,
pScript += 1 with return pScript, ++pScript, pNext = pScript + 1 returned, the
increment moved before and after the mask store, and permute.sh twice from this body
(best 150, reached only with new_var/new_var2 debris). Structurally stuck: stop
permuting.
u8* func_8003E360(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 index = *pScript;
    AudioEnvelope* pEnvelope = &pAudioElements->envelopes[index];
    u16 mask;

    SoundResetEnvelope(pEnvelope);
    pEnvelope->flags |= 0x1;
    do {
        mask = 1 << index;
        pAudioElements->activeEnvelopeMask |= mask;
        pScript += 1;
        return pScript;
    } while (0);
}
*/

void SoundResetEnvelope(AudioEnvelope* pEnvelope) {
    pEnvelope->count = 1;
    pEnvelope->value = 0;
    pEnvelope->flags &= ~(SOUND_ENVELOPE_FLAG_HALF_RATE | SOUND_ENVELOPE_FLAG_DESCENDING);
    pEnvelope->delayCount = pEnvelope->delayReload;
    pEnvelope->rampLevel = pEnvelope->rampStep;
}

u8* SoundScriptDisableEnvelope(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 index = *pScript;

    pAudioElements->envelopes[index].flags &= ~SOUND_ENVELOPE_FLAG_ENABLED;
    pAudioElements->activeEnvelopeMask &= ~(1 << index);
    return pScript + 1;
}

u8* SoundScriptSetInstrumentAndTone(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 instrument = *pScript;
    u8 value = pScript[1];
    SoundWDSEntry* pWds;

    pAudioElements->sndId = instrument;
    pWds = SoundFindWdsEntry(instrument);
    if (!pWds) {
        pWds = g_SoundWdsLinkedList;
    }
    pAudioElements->pWds = (u32)pWds;
    SoundApplyToneToElement(value, pAudioElements);
    return pScript + 2;
}

u8* SoundScriptSetExpression(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 value = *pScript++;

    if (value != 0) {
        pAudioManager->tempoScaleInterpolator.currentValue = value << 24;
        pAudioManager->tickRate = (pAudioManager->tempo >> 16) * (value << 8);
    }
    return pScript;
}

u8* SoundScriptSetInstrument(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 instrument = *pScript++;
    SoundWDSEntry* pWds;

    pAudioElements->sndId = instrument;
    pWds = SoundFindWdsEntry(instrument);
    if (!pWds) {
        pWds = g_SoundWdsLinkedList;
    }
    pAudioElements->pWds = (u32)pWds;
    return pScript;
}

u8* SoundScriptReleaseVoiceIfEnvelopeEnded(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
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

void SoundApplyToneToElement(s16 value, AudioElement* pElement) {
    SoundWDSEntry* pWds;
    SoundWdsTone* pTone;
    u32 offset;
    u32 spuAddress;
    u32 adsr;
    u32 releaseShift;
    u16 flags;
    u16 toneTune;

    pElement->toneIndex = value;
    pWds = (SoundWDSEntry*)pElement->pWds;
    pTone = &pWds->tones[value];
    spuAddress = pWds->spuMemoryAddress;
    adsr = spuAddress; // the address goes through `adsr`: retail reaches it in $v0 and the two ADSR words in $a2
    offset = pTone->sampleOffset * 8;
    pElement->voice_data.startAddress = offset + adsr;
    pElement->voice_data.loopAddress = offset + pTone->loopOffset * 8;

    adsr = pTone->adsrModes;
    pElement->voice_data.attackMode = adsr & 0x7;
    pElement->voice_data.sustainMode = (adsr >> 4) & 0x7;
    pElement->voice_data.releaseMode = (adsr >> 8) & 0x7;

    adsr = pTone->adsr;
    releaseShift = adsr >> 24;
    pElement->voice_data.attackRate = adsr & 0x7F;
    pElement->voice_data.adsrDR = (adsr >> 8) & 0xF;
    pElement->voice_data.sustainRate = (adsr >> 16) & 0x7F;
    pElement->voice_data.adsrSR = (adsr >> 12) & 0xF;

    flags = pElement->active_flag;
    // inner store first: retail writes the element's own releaseRate field before the voice one
    pElement->voice_data.releaseRate = (pElement->releaseRate = releaseShift & 0x1F);
    toneTune = pTone->toneTune;
    pElement->active_flag = flags | SOUND_CHANNEL_CHANGE_INSTRUMENT;
    pElement->toneTune = toneTune;
}

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
    // a byte offset, not an index: an indexed loop makes gcc keep the table base in a register
    s32 offset = sizeof(SoundVoiceData*) * (NUM_VOICES - 1);
    while( offset >= 0 ) {
        *(SoundVoiceData**)((u8*)&g_SoundChannels + offset) = NULL;
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
/*
Attempt (98.51%, 604 bytes = the original's size; the instruction multiset and the order of
every instruction agree): the only difference is one register pair - retail keeps the SPU
voice-register base in t0 and the loop index in t1, gcc gives t0 to the index and t1 to the
base. Every other register agrees (updateMask t2, the channel array t3, the three masks
t4/t5/t6, the adsr biv a2, flags a3). Tried: for(;;) vs do/while, both pointer increments in
every position, the local declaration order, the statement order of the initialisations (this
one fixed the prologue and the loop latch exactly), a modeFlags local, and 4 minutes of the
permuter (base=225 best=225, 1172 iterations). gcc 2.6.0's local-alloc hands t0 to the index
inside the mask-update block, where the index is referenced five times and the base not at
all; retail's allocator made the opposite call.
*/
/*
// Voice register update requested by the channel scripts: each SoundVoiceData's
// flags say which SPU registers to refresh, then the voice's flags are cleared.
void func_8003E900(void) {
    SpuUnion* pSpu;
    SPU_VOICE_REG* pVoice;
    SoundVoiceData* pChannel;
    SoundVoiceData** pChannels;
    u16 flags;
    u32 revModeMask;
    u32 noiseModeMask;
    u32 chanFmMask;
    u32 updateMask;
    u32 keyOnFlags;
    s32 i;

    pChannels = g_SoundChannels;
    revModeMask = 0;
    noiseModeMask = 0;
    chanFmMask = 0;
    updateMask = 0;
    pVoice = g_pSoundSpuRegisters->_rxx.voice;
    i = 0;
    do {
        pChannel = *pChannels;

        if (pChannel != NULL) {
            flags = pChannel->flags;
            if (flags != 0) {
                if (flags & 0x1) {
                    pVoice->volume.left = pChannel->volume.left;
                    pVoice->volume.right = pChannel->volume.right;
                }
                if (flags & 0x4) {
                    pVoice->pitch = pChannel->pitch;
                }
                if (flags & 0x8) {
                    pVoice->addr = pChannel->startAddress >> 3;
                    pVoice->loop_addr = pChannel->loopAddress >> 3;
                }
                if (flags & 0x10) {
                    pVoice->adsr[0] = (pVoice->adsr[0] & 0xFF) + (pChannel->attackRate << 8) +
                        ((pChannel->attackMode >> 2) << 15);
                }
                if (flags & 0x20) {
                    pVoice->adsr[0] = (pVoice->adsr[0] & 0xFF0F) + (pChannel->adsrDR << 4);
                }
                if (flags & 0x40) {
                    pVoice->adsr[1] = (pVoice->adsr[1] & 0x3F) + (pChannel->sustainRate << 6) +
                        ((pChannel->sustainMode >> 1) << 14);
                }
                if (flags & 0x80) {
                    pVoice->adsr[1] = (pVoice->adsr[1] & 0xFFC0) +
                        (pChannel->releaseRate + ((pChannel->releaseMode >> 2) << 5));
                }
                if (flags & 0x100) {
                    pVoice->adsr[0] = (pVoice->adsr[0] & 0xFFF0) + pChannel->adsrSR;
                }
                updateMask |= flags & 0x7000;
                pChannel->flags = 0;
            }

            // Voice 0..15 live in the low half of the SPU's 24-bit masks, 16..23 in the high half.
            chanFmMask |= ((pChannel->modeFlags >> 4) & 1) << i;
            noiseModeMask |= ((pChannel->modeFlags >> 5) & 1) << i;
            revModeMask |= ((pChannel->modeFlags >> 6) & 1) << i;
        }

        pVoice++;
        pChannels++;
        i++;
    } while (i < NUM_VOICES);

    pSpu = g_pSoundSpuRegisters;
    if (updateMask & 0xFFFF) {
        if (updateMask & 0x1000) {
            pSpu->_rxx.chan_fm[0] = chanFmMask;
            pSpu->_rxx.chan_fm[1] = chanFmMask >> 16;
        }
        if (updateMask & 0x2000) {
            pSpu->_rxx.noise_mode[0] = noiseModeMask;
            pSpu->_rxx.noise_mode[1] = noiseModeMask >> 16;
        }
        if (updateMask & 0x4000) {
            pSpu->_rxx.rev_mode[0] = revModeMask;
            pSpu->_rxx.rev_mode[1] = revModeMask >> 16;
        }
    }

    keyOnFlags = g_SoundKeyOnFlags;
    if (keyOnFlags != 0) {
        pSpu->_rxx.key_on[0] = keyOnFlags;
        pSpu->_rxx.key_on[1] = keyOnFlags >> 16;
        g_SoundKeyOnFlags = 0;
    }
}
*/

void SoundApplyPendingKeyOff(void) {
    u32 flags = g_unk_VoicesNeedingProcessing;
    SpuUnion* pSpu = g_pSoundSpuRegisters;
    SPU_VOICE_REG* pVoice = pSpu->_rxx.voice;
    u32 keyOffHi;
    s32 i;

    if (flags != 0) {
        i = 0;
        do {
            if (flags & (1 << i)) {
                pVoice->adsr[1] = (pVoice->adsr[1] & 0xFFC0) | 6;
            }
            i++;
            pVoice++;
        } while (i < NUM_VOICES);
    }

    flags = g_SoundKeyOffFlags | g_unk_VoicesNeedingProcessing;
    if (flags != 0) {
        keyOffHi = flags >> 0x10;
        pSpu->_rxx.key_off[0] = flags;
        pSpu->_rxx.key_off[1] = keyOffHi;
        g_unk_VoicesNeedingProcessing = 0;
        g_SoundKeyOffFlags = 0;
    }
}

// Recomputes the SPU volume/pan/pitch of every element whose status flags ask for it. The
// manager's interpolators and an element's note are 16.16 fixed point; the mixer reads the
// integer part on its own, hence the (s16*) casts.
void SoundApplyElementStatusFlags(AudioManager* pManager, AudioElement* pElements, s32 count) {
    AudioElement* pElement;
    s16 numElements = count;
    u16 statusFlags;
    s32 currentVolume;
    s32 scale;
    s32 pan;
    s32 left;
    s32 right;

    if (pManager->flags & SOUND_MANAGER_FLAG_STOP_PENDING) {
        return;
    }

    pElement = pElements;
    do {
        if (pElement->active_flag != 0) {
            statusFlags = pElement->status_flags;

            if (statusFlags & SOUND_STATUS_VOLUME_CHANGE) {
                currentVolume = (s16)(pElement->elementVolume >> 16);
                scale = currentVolume - ((currentVolume * pElement->volumeOffset) >> 15);
                if (scale > SOUND_VOLUME_MAX) {
                    scale = SOUND_VOLUME_MAX;
                }
                if (scale < 0) {
                    scale = 0;
                }
                scale = (pElement->volume * scale) >> 15;
                scale = ((pManager->volumeInterpolator.currentValue >> 16) * scale) >> 16;

                pan = (s16)pElement->pan + pElement->panOffset +
                    (pManager->panInterpolator.currentValue >> 16);
                if (pan > SOUND_PAN_MAX) {
                    pan = SOUND_PAN_MAX;
                }
                if (pan < 0) {
                    pan = 0;
                }

                if (g_SoundControlFlags & 0x100) {
                    if (pan < SOUND_PAN_CENTER) {
                        right = (pan * 0x5A00) >> 14;
                        left = SOUND_PAN_MAX - ((pan * 0x2500) >> 14);
                    } else {
                        pan = 0x8000 - pan;
                        left = (pan * 0x5A00) >> 14;
                        right = SOUND_PAN_MAX - ((pan * 0x2500) >> 14);
                    }
                    left = (left * scale) >> 15;
                    right = (right * scale) >> 15;
                } else {
                    left = right = (scale * 0x5A00) >> 15;
                }

                pElement->voice_data.volume.left = left;
                pElement->voice_data.volume.right = right;
                pElement->voice_data.flags |= SOUND_VOICE_FLAG_VOLUME;
            }

            if (statusFlags & SOUND_STATUS_PLAYING_NTOE) {
                s32 note = (s16)(pElement->note >> 16) + pElement->noteOffset +
                    (pManager->noteInterpolator.currentValue >> 16);

                pElement->voice_data.pitch = SoundGetTonePitch((s16)note) & 0x3FFF;
                pElement->voice_data.flags |= SOUND_VOICE_FLAG_PITCH;
            }

            if (statusFlags & 0x1) {
                if ((pElement->active_flag & SOUND_CHANNEL_MUTED) == 0) {
                    SoundAssignVoiceToChannelAndPlay(&pElement->voice_data, pElement->voice_number);
                }
            }

            if (statusFlags & SOUND_STATUS_REST_NOTE) {
                SoundStopVoiceOnChannel(&pElement->voice_data, pElement->voice_number);
            }

            pElement->status_flags = 0;
        }
        pElement++;
    } while (--numElements != 0);
}

// .sdata tables used by the sample-rate helper below (splat data segment).
extern u8 g_SoundNoteOctaveTable[];
extern s16 g_SoundSemitonePitchTable[];

s16 SoundGetTonePitch(u32 tone) {
    s32 entry = g_SoundNoteOctaveTable[(tone & 0x7FFF) >> 8];
    s32 shift = 6 - (entry >> 4);
    s32 value = g_SoundSemitonePitchTable[(tone & 0xFF) + ((entry & 0xF) << 8)];

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

void SoundStepElementEnvelopes(AudioManager* pAudioManager, AudioElement* pElements, s32 count) {
    AudioElement* pElement = pElements;
    AudioEnvelope* pEnvelope;
    u16 statusFlags;
    s16 numElements = count;
    s32 value;
    s32 i;

    do {
        if (pElement->active_flag != 0) {
            pElement->panOffset = 0;
            pElement->volumeOffset = 0;
            pElement->noteOffset = 0;
            if (pElement->activeEnvelopeMask != 0) {
                i = NUM_VOICE_ENVELOPES;
                pEnvelope = pElement->envelopes;
                statusFlags = pElement->status_flags;
                for (; i != 0; i--) {
                    if ((pEnvelope->flags & SOUND_ENVELOPE_FLAG_ENABLED) != 0) {
                        if (pEnvelope->delayCount != 0) {
                            pEnvelope->delayCount--;
                        } else {
                            value = ((s32 (*)(AudioEnvelope*))pEnvelope->pStepHandler)(pEnvelope);
                            if ((s16)pEnvelope->rampLevel < 0x400) {
                                value = (value >> 10) * (s16)pEnvelope->rampLevel;
                                pEnvelope->rampLevel += pEnvelope->rampStep;
                            }
                            value >>= 16;
                            switch (pEnvelope->target) {
                            case 0:
                                pElement->noteOffset += value;
                                statusFlags |= SOUND_STATUS_PLAYING_NTOE;
                                break;
                            case 1:
                                pElement->volumeOffset += value;
                                statusFlags |= SOUND_STATUS_VOLUME_CHANGE;
                                break;
                            case 2:
                                pElement->panOffset += value;
                                statusFlags |= SOUND_STATUS_VOLUME_CHANGE;
                                break;
                            }
                        }
                    }
                    pEnvelope++;
                }
                pElement->status_flags = statusFlags;
            }
        }
        pElement++;
    } while (--numElements != 0);
}

void SoundClearEnvelopeFlags(AudioEnvelope* pEnvelope) {
    pEnvelope->flags &= ~SOUND_ENVELOPE_FLAG_ENABLED;
}

s32 SoundEnvelopeStepPulse(AudioEnvelope* pEnvelope) {
    if (--pEnvelope->count == 0) {
        pEnvelope->count = pEnvelope->reload;
        pEnvelope->value = pEnvelope->value ? 0 : pEnvelope->delta;
    }

    return pEnvelope->value;
}

s32 SoundEnvelopeStepSquare(AudioEnvelope* pEnvelope) {
    if (--pEnvelope->count == 0) {
        s32 delta = pEnvelope->delta;

        pEnvelope->count = pEnvelope->reload;
        if (pEnvelope->flags & SOUND_ENVELOPE_FLAG_DESCENDING) {
            delta = -delta;
        }
        pEnvelope->value = delta;
        pEnvelope->flags ^= SOUND_ENVELOPE_FLAG_DESCENDING;
    }

    return pEnvelope->value;
}

s32 SoundEnvelopeStepTriangle(AudioEnvelope* pEnvelope) {
    if (--pEnvelope->count == 0) {
        s32 delta = pEnvelope->delta;

        pEnvelope->count = pEnvelope->reload;
        if (pEnvelope->flags & SOUND_ENVELOPE_FLAG_DESCENDING) {
            delta = -delta;
        }
        pEnvelope->stepDelta = delta;
        pEnvelope->flags ^= SOUND_ENVELOPE_FLAG_DESCENDING;
    }

    pEnvelope->value += pEnvelope->stepDelta;
    return pEnvelope->value;
}

s32 SoundEnvelopeStepTriangleHalfRate(AudioEnvelope* pEnvelope) {
    s32 count = pEnvelope->count;

    count--;
    if (count == 0) {
        u16 flags = pEnvelope->flags;
        s32 delta;

        count = pEnvelope->reload;
        if (flags & SOUND_ENVELOPE_FLAG_HALF_RATE) {
            count <<= 1;
        }
        delta = pEnvelope->delta;
        pEnvelope->stepDelta = delta;
        if (flags & SOUND_ENVELOPE_FLAG_DESCENDING) {
            pEnvelope->stepDelta = -delta;
        }
        flags = (flags | SOUND_ENVELOPE_FLAG_HALF_RATE) ^ SOUND_ENVELOPE_FLAG_DESCENDING;
        pEnvelope->flags = flags;
    }

    pEnvelope->count = count;
    pEnvelope->value += pEnvelope->stepDelta;
    return pEnvelope->value;
}

s32 SoundEnvelopeStepSawtooth(AudioEnvelope* pEnvelope) {
    if (--pEnvelope->count == 0) {
        pEnvelope->count = pEnvelope->reload;
        pEnvelope->value = 0;
    } else {
        pEnvelope->value += pEnvelope->delta;
    }

    return pEnvelope->value;
}

s32 SoundEnvelopeStepRandom(AudioEnvelope* pEnvelope) {
    s32 random = SoundRandomNext();

    if (--pEnvelope->count == 0) {
        pEnvelope->count = pEnvelope->reload;
        random = SoundRandomNext();
        pEnvelope->value = (pEnvelope->delta >> 15) * random;
    }

    return pEnvelope->value;
}

s32 SoundEnvelopeStepRandomBipolar(AudioEnvelope* pEnvelope) {
    if (--pEnvelope->count == 0) {
        s32 delta;
        s32 random;

        pEnvelope->count = pEnvelope->reload;
        random = SoundRandomNext();
        delta = pEnvelope->delta;
        pEnvelope->value = (delta >> 14) * random - delta;
    }

    return pEnvelope->value;
}

void SoundSetRandomSeed(s32 seed) {
    g_SoundRandomSeed = seed;
}

s32 SoundRandomNext(void) {
    s32 seed = g_SoundRandomSeed;

    seed ^= seed << 17;
    seed ^= seed >> 15;
    g_SoundRandomSeed = seed;

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
        bIsError = (pSoundFile->version != targetValue);
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
    nCount = (pSoundFile->fileSize + 3) / 4; // Align to 4-byte boundary
    nResult = 0;
    do {
        nResult += *pCurrent++;
    } while (--nCount);
    
    return nResult;
}


extern u8 g_SoundErrorSedsFile[];
extern u16 D_80050924;
extern u8 g_SoundErrorWdsFile[];

void SoundHandleError(s16 errorId) {
    if (!(g_SoundControlFlags & 0x88)) {
        g_SoundControlFlags |= 0x8;
        g_SoundSpuErrorId = errorId;
        SoundSpuMemoryFreeBlock(0x10000);
        SoundLoadWdsFile(g_SoundErrorWdsFile, NULL);
        SoundAddSedsEntry((SoundFile*)g_SoundErrorSedsFile);
        SoundGetPendingSpuCommandType(SOUND_CTL_FLAG_TRANSFER_BUSY);
        SoundPlaySoundEffectOnFreePair(((u32)D_80050924 << 16) | 1);
    }
}
