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
#define SOUND_CHANNEL_NTOE                  0x200   // the 0x81 script command sets it, 0x80 and the 0xB0/0xB1 pair clear it
// AudioElement.status_flags bit 0 (the rest of the field is SOUND_STATUS_* in sound.h)
#define SOUND_STATUS_KEY_ON                 0x1     // SoundSetElementsStatusFlag1 queues it, the element mixer assigns and keys the voice on
// AudioElement.rampFlags: one bit per interpolator the element runs (bit 1 has no known use)
#define SOUND_RAMP_FLAG_NOTE                0x1     // noteFrames/noteStep slide the pitch
#define SOUND_RAMP_FLAG_SLIDE               0x4     // slideFrames is set, the note glides from currentNote to nextNote
#define SOUND_RAMP_FLAG_ELEMENT_VOLUME      0x8     // elementVolumeFrames/elementVolumeStep
#define SOUND_RAMP_FLAG_PAN                 0x10    // panFrames/panStep
#define SOUND_RAMP_FLAG_VOLUME              0x20    // volumeFrames/volumeStep
#define SOUND_RAMP_FLAG_QUEUED              0x100   // a ramp waits in `ramp`, installed into the element-volume ramp
// AudioElement.voice_data.modeFlags: one bit per SPU voice flag register the voice is routed into
#define SOUND_VOICE_MODE_PITCH_MODULATION   0x10
#define SOUND_VOICE_MODE_NOISE              0x20
#define SOUND_VOICE_MODE_REVERB             0x40
// SoundVoiceData.flags: one bit per SPU voice register group the element scripts rewrite
#define SOUND_VOICE_FLAG_VOLUME             0x1
#define SOUND_VOICE_FLAG_PITCH              0x4
#define SOUND_VOICE_FLAG_ATTACK             0x10
#define SOUND_VOICE_FLAG_DECAY              0x20
#define SOUND_VOICE_FLAG_SUSTAIN            0x40
#define SOUND_VOICE_FLAG_RELEASE            0x80
#define SOUND_VOICE_FLAG_SUSTAIN_LEVEL      0x100
#define SOUND_VOICE_FLAG_NOISE              0x2000  // the noise scripts set it with SpuSetNoiseClock, the voice update rewrites noise_mode
#define SOUND_VOICE_FLAG_ALL                (SOUND_VOICE_FLAG_VOLUME | SOUND_VOICE_FLAG_PITCH | SOUND_VOICE_FLAG_ATTACK | SOUND_VOICE_FLAG_DECAY | SOUND_VOICE_FLAG_SUSTAIN | SOUND_VOICE_FLAG_RELEASE | SOUND_VOICE_FLAG_SUSTAIN_LEVEL)
// AudioEnvelope.flags bits
#define SOUND_ENVELOPE_FLAG_ENABLED         0x1
#define SOUND_ENVELOPE_FLAG_HALF_RATE       0x4     // the reloaded count doubles, the envelope steps every other tick
#define SOUND_ENVELOPE_FLAG_DESCENDING      0x8     // the step handlers negate delta while this is set
#define SOUND_ENVELOPE_RAMP_SCALE           0x400   // rampStep at full speed, and the rampLevel the step is scaled below
#define SOUND_EFFECT_ELEMENT_COUNT          16      // elements of the sound-effect manager SoundInitialize creates
#define SOUND_EFFECT_ELEMENT_PAIR_COUNT     2       // elements one sound-effect pair spans, what g_SoundEffectElementPairCount reports
#define SOUND_EFFECT_ELEMENT_PAIR_STRIDE    8       // the two elements of a pair sit this far apart in the table, so bit 3 picks the half
#define SOUND_EFFECT_PAIR_VOLUME            0x6000  // level the pair play helpers start a sound effect at
#define SOUND_EFFECT_CHANNEL_COUNT_DEFAULT  8       // g_SoundSoundEffectChannelCount until the game changes it
#define SOUND_PERCUSSION_TABLE_SIZE         0x180   // tail added to a manager's allocation when it carries a percussion map
#define SOUND_UPLOAD_BUFFER_SIZE            0x840   // SoundInitiateReverbWorkAreaTransfer's SPU write buffer, sized so a whole tail fits
#define SOUND_UPLOAD_CHUNK_SIZE             0x800   // largest SPU write SoundExecuteReverbWorkAreaTransfer queues at a time
#define SOUND_TICKS_PER_WHOLE_NOTE          192     // a beat divisor of 4 splits this into 48 ticks per beat
#define SOUND_DEFAULT_BEATS_PER_BAR         4       // beatsPerBar (and its mirror at 0x3E) before a time signature command
#define SOUND_DEFAULT_BEAT_DIVISOR          4       // beatDivisor that makes ticksPerBeat SOUND_TICKS_PER_WHOLE_NOTE / 4
#define SOUND_TIMER_RELOAD                  0x44E8  // RCnt2 reload count: one SoundTimerInterruptHandler per 1/240 s
#define SOUND_TIMER_TICKS_PER_SECOND        240     // what SoundGetAudioManagerPlayTime divides playTime by to reach seconds
#define SOUND_SECONDS_PER_MINUTE            60      // ... and the seconds in the minute it reports next
#define SOUND_MANAGER_VOLUME_MAX            0x7F00  // SoundSetAudioManagerMaxVolume scales the tempo by this (max volume << 8)
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
void SoundSetVolumeMode(s32 mode);
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
    SetRCnt(RCntCNT2, SOUND_TIMER_RELOAD, RCntMdINTR);
    StartRCnt(RCntCNT2);
    SpuSetTransferCallback(&SoundOnTransferCallback);
    SpuSetIRQCallback(&SoundSpuIRQHandler);
    SpuSetIRQ(SPU_OFF);
    g_SoundTimerTickCounter = 0;
    g_SoundSpuIRQCount = 0;
    ExitCriticalSection();

    SoundSpuMemoryAllocateBlockAtAddress(0x2000, 0x10000, 4);
    SoundSetVolumeMode(1);
    SoundSetCdAttr(0, 1);
    SoundSetMasterVolumeWithFade(0x3FFF, 0);
    SoundSetCdVolumeWithFade(0x7FFF, 0);
    if (g_SoundControlFlags & 0x4000) {
        SoundSetupCdMix(0x80);
    }

    g_SoundEffectAudioManager = SoundCreateAudioManagerForElementCount(SOUND_EFFECT_ELEMENT_COUNT);
    g_SoundSoundEffectChannelCount = SOUND_EFFECT_CHANNEL_COUNT_DEFAULT;
    g_SoundReverbMemoryHandle = SOUND_REVERB_MEMORY_NONE;
    g_SoundUploadDestBuffer = 0;
    g_SoundReverbType = SOUND_REVERB_TYPE_NONE;
    SoundSetReverbModeWithAllocation(SPU_REV_MODE_STUDIO_C, 0, 0, 0);
    SpuSetReverb(SPU_ON);
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
    SoundSetVoiceKeyOff(SPU_ALLCH); // Release all voices
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
void SoundSetVolumeMode(s32 mode) {
    AudioManager* pManager;
    SoundCdSeqState* pState;
    u16 volume;

    g_SoundControlFlags &= ~SOUND_CTL_MASK_VOLUME_MODE;

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
                pState->unk66 = volume;
            } else {
                volume <<= 6;
                pState->unk38 = volume;
                pState->unk3A = volume;
                pState->unk64 = volume;
                pState->unk66 = volume;
            }
            pState->unk36 = 1;
            pState->unk62 = 1;
        }
    }
}

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
        temp_v0 = SoundHeapAllocate(SOUND_UPLOAD_BUFFER_SIZE);
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
        chunkSize = (bytesRemaining <= SOUND_UPLOAD_BUFFER_SIZE) ? bytesRemaining : SOUND_UPLOAD_CHUNK_SIZE;

        currentSourceAddr = g_SoundUploadSourceAddr;

        g_SoundUploadBytesRemaining = bytesRemaining - chunkSize;
        g_SoundUploadSourceAddr = currentSourceAddr + chunkSize;

        SoundQueueSpuWriteCommand(currentSourceAddr, g_SoundUploadDestBuffer, chunkSize, SoundExecuteReverbWorkAreaTransfer);

        if ((g_SoundControlFlags & SOUND_CTL_FLAG_TRANSFER_BUSY) == 0) {
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


// Attempt (94.58%, 256 bytes = retail's size). Residual, in the loop head only: retail
// REMATERIALISES the array base every iteration (`lh v0,2(s1); lui %hi; addiu %lo; sll; addu`)
// while gcc 2.6 hoists one copy into a2 in the preheader (`move a2,s0` plus a nop in the lh's
// load-delay slot), and therefore also swaps the two callee-saved pointers (here address s1,
// pPrevBlock s0; retail address s0, pPrevBlock s1). Tried, all 93-94.6%: an index local,
// `(u8*)base + (i<<4)`, `new_var = pPrevBlock` used as the index (that one fixes the registers
// but folds the base into s1, 244 bytes), a do/while round the loop body (93.25), declaration
// order, and -O1/-O3/-G0/cdk screens (all worse). loop.c hoists the base here; the sibling
// SoundSpuMemoryAllocateBlockFit shows the same hoist and matches, so the hoist itself is not
// the blocker - retail's source must defeat it somehow.
// Round 315 added, all still 94.58 or worse: `(u8*)g_SoundSpuMemoryBlocks + (idx << 4)` cast
// form (identical to the subscript), local declaration order both ways (no change), the
// while (1) { if (nextBlockIndex == 0) break; } form (84.06), a `short nextIndex` assigned in
// the condition (82.58, kills retail's duplicated lh), and a 4-minute permuter run
// (486 -> 355) whose only improvement is WRONG CODE - it hoists `short new_var =
// pPrevBlock->nextBlockIndex;` above the loop and indexes with the fixed value, so the loop
// would never advance. That candidate does say what gcc wants: a live short in a register.
// ROUND 316 PROVED THE HOIST IS NOT SOURCE-CONTROLLABLE (nine spellings, cc1_probe.sh, every
// one hoists the base into the preheader as `la $16` plus `move $6,$16` with a nop in the lh's
// load-delay slot): `&Arr[i]`, `Arr + i`, `(u8*)Arr + (i << 4)`, `(u8*)Arr + i * sizeof(T)`,
// `&Arr[(u8)i]`, a base local assigned inside the body, the same local live across the whole
// function (that one only moves the copy into a callee-saved $19 and turns the addu
// base-first, so it is worse), the unbounded-array extern vs the header's array declaration,
// and gcc-2.7.2-cdk (`move $7,$16`, same hoist). Padding the loop body with 1/3/6/12 extra
// global increments did not stop it either, so round 113's move_movables cost threshold is not
// what we hit here: for a body this small the invariant is always moved. Searching every
// matched function in all eleven binaries for an address formed inside a loop found exactly
// one family where retail re-forms it (field/misc9 FieldInitScreenStripSprites), and there the
// loop contains jal calls and the re-formed `la` sits in a block AFTER them, where cse2 cannot
// forward the callee-saved copy; this loop has no call, so that route does not exist. Treat the
// remaining delta as a compiler heuristic, not a missing spelling: the sibling
// SoundSpuMemoryAllocateBlockFit hoists the same way and matches because retail does too, so
// move_movables is running normally here and retail's own Fit object shows the identical
// hoisted-copy shape (`addu t0,s0,zero` in its preheader). Whatever defeated the hoist in
// Block's original source is therefore not how the base is spelled (nine measured above), so
// it is the loop or its exits - that is the next lead, not another address spelling.
//     // Allocates a block of SPU memory of the given size, returns the SPU address or 0 on failure.
// Round 411: re-measured the un-parked body at 94.58% / 256 bytes with the residual below
// unchanged (loop head only: the hoisted base and the s0/s1 swap). No new spelling attempted;
// the nine measured in round 316 plus three permuter runs stand.
// Round 446 re-measured the un-parked body: 94.58% / retail's exact 256 bytes / 64 insns, 40 differing lines, all one cause. One more dead spelling measured: pointer arithmetic on the struct type (`g_SoundSpuMemoryBlocks + pPrevBlock->nextBlockIndex`, no u8 cast) is byte-identical to the cast form, 94.58 / 256 / 64 - the 15th spelling. NEW READ of the two objects (this is why the spellings cannot move it): retail has NO register holding &Arr inside the loop, because its `la s1` IS pPrevBlock and pPrevBlock is overwritten by `move s1,a0`, so the loop head materialises the base with two insns (lui+addiu, in the lh load-delay slot); our body keeps &Arr in s0 at the loop head and loop.c snapshots it there with `move a2,s0` + a nop filler, i.e. one insn SHORTER per iteration than retail - and because that register is then reused for pPrevBlock the two callee-saved pointers swap (address s1 / pPrevBlock s0 vs retail's s0/s1) and every later s0/s1 reference mirrors with them. So the residual is move_movables' choice of the invariant's rtx class (symbol constant vs the REG_EQUAL register the preheader `la` left behind), not an address spelling; that also explains why the loop is 1 insn long per iteration and still 0 bytes off. Do not re-enter without a way to stop the preheader la from being the invariant.
// Round 459: re-measured the un-parked body: 94.58% / retail's exact 256 bytes / 64 insns, 40 differing lines - the same one cause as round 446 (the invariant &g_SoundSpuMemoryBlocks is hoisted into a2 with a per-iteration 'move a2,s0' plus a nop in the lh's load-delay slot where retail re-forms it with lui+addiu, and because that copy references s0 the address/pPrevBlock pair mirrors: ours address s1 / pPrevBlock s0, retail s0/s1). One new spelling measured, byte-identical: reading the address back through pPrevBlock after the pointer update ('address = pPrevBlock->spuAddress + pPrevBlock->size;') is 94.58 / 256 / 64, the 16th. Do not re-enter without a way to stop the preheader la from becoming the loop's invariant: with the invariant hoisted, s0/s1 cannot mirror retail, so this is not a callee-saved quotient contest.
// Round 472: re-measured the un-parked body at 94.58% / retail's exact 256 bytes / 64 insns / 40 differing lines - unchanged, the residual is still the loop head only (retail re-forms the base per iteration with lui+addiu in the lh load-delay slot and keeps address in s0 / pPrevBlock in s1, while gcc 2.6 hoists &g_SoundSpuMemoryBlocks into the loop head as 'move a2,s0' plus a nop filler and swaps the two callee-saved pointers). One more spelling measured this round: computing 'address' from the array itself ('g_SoundSpuMemoryBlocks[0].spuAddress + g_SoundSpuMemoryBlocks[0].size') before forming pPrevBlock - it disarranges the preheader (the three %lo loads and the la reorder, +2 insns of shuffling) so it was abandoned unscored and reverted; the preheader in retail is load-load-load, la, beqz with the addu in the delay slot, which the parked body already reproduces exactly. Standing conclusion from rounds 316/446/459: every spelling hoists (nine probed with cc1_probe.sh), so this is move_movables' invariant choice, not source. Do not re-enter without a way to stop the preheader la from becoming the loop's invariant.
/*
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
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", SoundSpuMemoryAllocateBlock);





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

// Round 478: 98.82% (296 bytes = retail's 0x128 span, 74 insns both). The old note's
// '88.92% at retail's exact 292 bytes' was 4 bytes/1 insn SHORT - retail is 296 bytes, so every earlier
// attempt was one instruction short and could never link. Two things closed it: (1) writing the two
// guards as ONE condition, 'if (nSpace < size || address < freeAddress) return 0;' - as two separate
// if's gcc 2.6 lays the second slt out in the first test's beqz delay slot and drops retail's
// 'j fail / nop' pair (73 insns); with '||' both tests come out as retail's bnez-fail + beqz-alloc and
// the nop pair is there; (2) pPrevBlock/nSpace declared with initialisers in that order so the
// preheader emits 'lui/addiu s0' before 'move a2,zero' like retail. Residual = ONE mirrored
// callee-saved pair: retail keeps pPrevBlock in s0 and address in s1, this body has them swapped (30
// diff lines, every one of them that pair). -dl on the compile: address 72 'used 8 times across 43
// insns', pPrevBlock 74 '8 times across 44' -> quotients 2.233 vs 2.180 under
// floor_log2(refs)*refs*4/live, so address wins s0 by 2.4% and pPrevBlock is 2 insns of live range
// away. That margin is body-dependent: with 'int nSpace = 0;' declared FIRST the same body measures
// 97.30 (only 'move a2,zero' 3 insns early) and THERE pPrevBlock wins s0 (8/41 = 2.34) - so both
// halves of retail exist, in two bodies that cannot be combined by declaration order. Tried this
// round, all 296 bytes / 74 insns: park body 88.92, declarations-only+assignments 98.82 (swap),
// nSpace-first 97.30 (order), separate ifs in every order 87-90.51. Below 99, so no permuter run.
// Round 491: re-measured 98.82% (296 bytes = retail's 0x128 span, 74 insns both), unchanged from
// round 478's note. Residual is still ONE mirrored callee-saved pair: retail keeps pPrevBlock in s0 and
// address in s1, this body has them swapped (30 diff lines, every one of them that pair). -dl on THIS
// body: pseudo 72 = address 'used 8 times across 43 insns' (3*8*4/43 = 2.233) vs pseudo 75 = pPrevBlock
// 'used 8 times across 44 insns' (2.18), and -dg processes them adjacent in the order 84 94 76 79 72 75
// ... so the callee-saved hand-out is 72 -> s0, 75 -> s1, 71 (size) -> s2. Confirmed the round-478 split
// is a live-LENGTH question, not a quotient tie: with 'int nSpace = 0;' declared FIRST the whole body is
// 97.30% with exactly TWO differing lines - 'move a2,zero' 3 insns early - and there pPrevBlock takes s0
// because its init is expanded 3 insns later (live 41 -> 2.34). Emission order follows source order in
// this function (no preheader scheduling to exploit), so the [base, a2] order retail has and the live-41
// pPrevBlock that wins s0 cannot both be sourced. Tried this round, all 74 insns / 296 bytes: the
// nSpace-first initialiser body 97.30 (2 lines), do { ... } while (0) around the whole insert group
// 98.82 unchanged (label optimized away, allocation untouched - the a-one-iteration-do-while lever does
// not reach this pair), and the a-parameter-copy-signedness lever 'int startAddress = address;' used at
// all six sites 92.91 (nSpace displaced to a3, 44 lines). Below 99, so no permuter run. A future round
// should attack the live LENGTH: pPrevBlock needs <= 41 or address >= 45 without moving either end of
// the emitted preheader.
// PARKED round 510: 98.82% / 296 bytes / 74 insns = retail's exact size and instruction stream,
// residual is ONE mirrored callee-saved pair (retail pPrevBlock=s0, address=s1; this body has them swapped).
// -dl on this body: address 8 refs / 43 insns = 2.233, pPrevBlock 8 / 44 = 2.180, so pPrevBlock misses s0 by
// one instruction of live range; every term is pinned by retail's own stream (pPrevBlock's def is the
// prologue lui/addiu, its last use is the final 'sh a1,2(s0)' in the jump delay slot).
// Round 510 additions: preset screen - gcc-2.7.2 -O2 -G8 gives the IDENTICAL 98.82% mirrored map and
// gcc-2.7.2-cdk 91.08, so this is not a compiler-version question; and 'do { } while (0);' between the loop
// and the alloc test is byte-identical 98.82 (label optimised away, the empty-statement barrier does not
// reach this pair). Rounds 478/491 already closed: nSpace first (97.30, pair correct but move a2,zero 3
// insns early), do-while around the insert group, 'int startAddress = address;' (92.91), separate ifs
// (87-90.51). Only a block-structure change that re-ranks the pair is left; recommend a permuter run.
// PARKED round 510: 98.82% / 296 bytes / 74 insns = retail's exact size and stream; residual is ONE mirrored
// callee-saved pair (retail pPrevBlock=s0, address=s1; this body swaps them, 30 lines, all of them that pair).
// -dl: address 8 refs / 43 insns = 2.233, pPrevBlock 8 / 44 = 2.180, i.e. one instruction of live range.
// NEW round 510: the pair IS reachable from source and the handle is nSpace's DEF POSITION - with
// 'int nSpace;' (uninitialised) and 'nSpace = 0;' as the FIRST statement of the loop body the pair comes
// out retail's way and the whole body is 2 differing lines / 97.16% (the only residual is the preheader
// 'move a2,zero', which retail has BEFORE the freeAddress loads and gcc then puts at the loop preheader).
// That spelling is NOT shippable: on the path where the while-test is false immediately nSpace is read
// uninitialised (and retail's own stream proves retail zeroes it before the entry beqz: 'move a2,zero' sits
// at insn 8, four insns before 'beqz v0,.L80039644'). Adding the honest pre-loop 'int nSpace = 0;' back
// (declaration init + the in-loop zero) measures 98.22% / 42 lines with nSpace displaced to a1 and the
// mirror back, and the all-assignments spelling (pPrevBlock/nSpace/freeAddress/endAddress assigned as
// statements, retail's emission order) is 98.82% / 30 lines - so the pre-loop zero is what pushes address
// to s0. Preset screen this round: gcc-2.7.2 -O2 -G8 98.82 (identical), gcc-2.7.2-cdk 91.08 -> not a
// compiler question. Also measured: 'do { } while (0);' between the loop and the alloc test is byte
// identical (label optimised away). Rounds 478/491 closed: nSpace declared first (97.30, pair right but
// 'move a2,zero' 3 insns early), do-while around the insert group, 'int startAddress = address;' 92.91,
// separate ifs 87-90.51. Remaining lever: a block structure that keeps retail's pre-loop zero AND gives
// pPrevBlock a live range of <= 42 or address >= 45 - next agent should permute from THIS body (a 3-minute
// run reached base=85 best=60 only by the uninitialised nSpace edit above, i.e. wrong code).
// Round 523 re-measured the un-parked body: 98.82% / 296 bytes / 74 insns = retail's exact size and stream, residual unchanged (the $s0/$s1 pair: ours address $s0 / pPrevBlock $s1). -dl stays address 8 refs / 43 = 2.233 against pPrevBlock 8 / 44 = 2.180, so the pair needs ONE more pPrevBlock reference (3*9/44 = 0.614 > 0.558) or its def two insns later - and the def is pinned by retail's own stream, this round proves it: 'int freeAddress = g_SoundSpuMemoryBlocks[0].spuAddress + g_SoundSpuMemoryBlocks[0].size;' instead of the pPrevBlock deref is BYTE-IDENTICAL 98.82 / 296 / 74, because cc1 folds both spellings to the same four abs-symbol insns and the merged pseudo is still born at retail's lui/addiu s0. The ONE shape that does flip the pair is declaring pPrevBlock after that global read (also realistic: first block's end, then walk from the head): 87.91% / 300 bytes / 75 insns, pair retail's but the base then comes out as 'addiu s0,v0,-4' two insns later plus an extra 'addiu v0,v0,4'. So the flip costs 4 bytes at retail's instruction count, i.e. no 296-byte body reaches it. Do not re-enter without a no-cost extra reference to the walking pointer.
/*
int SoundSpuMemoryAllocateBlockAtAddress(int size, int address, int unkArg) {
    SoundSpuMemoryBlock* pPrevBlock = g_SoundSpuMemoryBlocks;
    int nSpace = 0;
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

    if (nSpace < size || address < freeAddress) {
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
}*/
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

        value -= SOUND_EFFECT_ELEMENT_PAIR_COUNT;
        g_SoundEffectElementPairCount = SOUND_EFFECT_ELEMENT_PAIR_COUNT;
        func_8003B644((s16)(value | (s16)0x8000), arg0, SOUND_EFFECT_PAIR_VOLUME, SOUND_PAN_CENTER);
    }
}

//----------------------------------------------------------------------------------------------------------------------
// Wowoweewa flag central
void SoundSendCommandConstant(s32 arg0) {
    if (g_SoundControlFlags & SOUND_CTL_FLAG_SOUND_EFFECT_ENABLED) {
        g_SoundEffectElementPairCount = SOUND_EFFECT_ELEMENT_PAIR_COUNT;
        func_8003B644((1 << 2) | (1 << 3) | (1 << 13) | (1 << 14), arg0, SOUND_EFFECT_PAIR_VOLUME, SOUND_PAN_CENTER);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SoundPlaySoundEffectOnFreePair(s32 arg0) {
    if (g_SoundControlFlags & SOUND_CTL_FLAG_SOUND_EFFECT_ENABLED) {
        s32 value = SoundAllocateSoundEffectElements(arg0, SOUND_EFFECT_ELEMENT_PAIR_COUNT);

        g_SoundEffectElementPairCount = SOUND_EFFECT_ELEMENT_PAIR_COUNT;
        func_8003B644((s16)(value | 0x2000), arg0, SOUND_EFFECT_PAIR_VOLUME, SOUND_PAN_CENTER);
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SoundPlaySoundEffectOnPair(s32 arg0, s32 arg1) {
    if (g_SoundControlFlags & SOUND_CTL_FLAG_SOUND_EFFECT_ENABLED) {
        g_SoundEffectElementPairCount = SOUND_EFFECT_ELEMENT_PAIR_COUNT;
        func_8003B644(
            ((arg1 & 0xFE) ^ SOUND_EFFECT_ELEMENT_PAIR_STRIDE) | (1 << 13), // wtf is this... we really need to figure out some of these macros
            arg0,
            SOUND_EFFECT_PAIR_VOLUME,
            SOUND_PAN_CENTER
        );
    }
}

//----------------------------------------------------------------------------------------------------------------------
void SoundSendCommandByIdWithLevels(s32 arg0, s32 arg1, s32 arg2) {
    if (g_SoundControlFlags & SOUND_CTL_FLAG_SOUND_EFFECT_ENABLED) {
        s32 value = SoundAllocateSoundEffectElements(arg0, SOUND_EFFECT_ELEMENT_PAIR_COUNT);

        g_SoundEffectElementPairCount = SOUND_EFFECT_ELEMENT_PAIR_COUNT;
        func_8003B644((s16)(value | 0x2000), arg0, (s16)(arg1 << 8), (s16)(arg2 << 8));
    }
}

void SoundPlaySoundEffectOnPairWithLevels(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    if (g_SoundControlFlags & SOUND_CTL_FLAG_SOUND_EFFECT_ENABLED) {
        g_SoundEffectElementPairCount = SOUND_EFFECT_ELEMENT_PAIR_COUNT;
        func_8003B644(((arg1 & 0xFE) ^ SOUND_EFFECT_ELEMENT_PAIR_STRIDE) | 0x2000, arg0, (s16)(arg2 << 8), (s16)(arg3 << 8));
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
    elementIndex ^= SOUND_EFFECT_ELEMENT_PAIR_STRIDE;
    cnt = SOUND_EFFECT_ELEMENT_PAIR_COUNT;
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
    elementIndex ^= SOUND_EFFECT_ELEMENT_PAIR_STRIDE;
    cnt = SOUND_EFFECT_ELEMENT_PAIR_COUNT;
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
                    pElement->rampFlags |= SOUND_RAMP_FLAG_VOLUME;
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
    elementIndex ^= SOUND_EFFECT_ELEMENT_PAIR_STRIDE;
    cnt = SOUND_EFFECT_ELEMENT_PAIR_COUNT;
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
                pElement->rampFlags |= SOUND_RAMP_FLAG_VOLUME;
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
    elementIndex ^= SOUND_EFFECT_ELEMENT_PAIR_STRIDE;
    cnt = SOUND_EFFECT_ELEMENT_PAIR_COUNT;
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
    u32 field = total / SOUND_TIMER_TICKS_PER_SECOND;
    u32 index = field / SOUND_SECONDS_PER_MINUTE;

    pOut->playTicks = pManager->playTicks;
    pOut->unk4 = total - field * SOUND_TIMER_TICKS_PER_SECOND;
    pOut->unk6 = field - index * SOUND_SECONDS_PER_MINUTE;
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
    s32 newVolume = SOUND_MANAGER_VOLUME_MAX;
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
                pElement->status_flags |= SOUND_STATUS_KEY_ON;
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
    manager->beatsPerBar = SOUND_DEFAULT_BEATS_PER_BAR;
    manager->tickCount = 1;
    manager->ticksPerBeat = SOUND_TICKS_PER_WHOLE_NOTE / SOUND_DEFAULT_BEAT_DIVISOR;
    manager->beatDivisor = SOUND_DEFAULT_BEAT_DIVISOR;
    manager->unk_0x3e = SOUND_DEFAULT_BEATS_PER_BAR;

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
// Round 496: 83.91% (536 bytes vs retail 544, 134 insns vs 136), the park re-measured against a fresh
// object (the old 84.32 note was inside a block that also carried the func_8003B644 notes and is gone).
// TWO instructions short, both in the flag-store cluster, and the whole rest of the 71 differing lines
// is downstream register naming (s6/s7 exchange: retail keeps elementCount in s6 and the WDS entry in
// s7, ours has them the other way round - same statement set, same size for the rest of the body):
//  (1) the active_flag ternary: retail emits beqz / sw-aliveBits-in-slot / j / li 0x421 / li 0x401 /
//      sh common, i.e. it did NOT invert the branch; every spelling here (ternary inside the store, and
//      now also if/else into a u16 activeFlag local followed by the store - measured byte-identical)
//      lets gcc 2.6.0 cross-jump the two single-set arms and drop the j, 1 insn;
//  (2) `if (pManager->flags & 4) pElement->active_flag |= 4;`: retail RELOADS the halfword
//      (lhu v0,0(s1); ori; sh), ours forwards the value it just stored (ori v0,a0,4; sh), 1 insn. Both
//      accesses are through the same s1 and no store sits between them, so cse forwarding is unavoidable
//      in source; a reload would need a call or an aliasing store retail does not have.
// Also still off: retail hoists `sllv v0,v0,s5` (bit = 1 << i) before the two lw of aliveBits/muteFlags,
// ours loads aliveBits first - a scheduler order in the same block, harmless while the count is short.
// Round 506: 94.99% (544 bytes / 136 insns = retail's EXACT size), up from 83.91 (536/134). Four
// source changes, each measured on its own, and the first one names the lever:
//  1. `u32 mute = pManager->muteFlags;` before `bit = 1 << i;` (the old body read muteFlags inline in
//     the ternary) - 91.32 / 544 / 136. Naming the mask moves the aliveBits store after the `and`, so
//     reorg fills the beqz slot with it (retail's `sw a2,16(sp)` in the slot), the arms stop sharing the
//     slot's `li` and retail's `j` reappears, and the stored value then dies in v0 which the following
//     `lhu v0,16(s2)` clobbers - which is why retail RELOADS the halfword in `|= 4`. Both of the two
//     gap-causes in the round-496 note closed with that one line.
//  2. the `do { pWds = g_SoundWdsLinkedList; } while (0);` wrapper dropped for a plain if - 91.72, and
//     the s6/s7 exchange (elementCount vs the WDS entry) disappeared with the allocno.
//  3. `sndId` stored before `pWds` - 93.19 (retail: `lbu v0,22(s2)` then `sw s7,-4(s0)`).
//  4. the `u32 value` local dropped for the soundId load (`pElement->soundId = *(u16*)(pSeqData+0x10);`)
//     and only the script IP named (`u32 ip`) - 94.99: retail reuses the SHORT-LIVED v1 for both the
//     soundId load and the IP sum, one local kept both in a0 across the whole store block.
// NAMED RESIDUAL: one cluster, 28 differing lines, all in the store block after the flags walk, and no
// byte in it (size already retail's):
//  (a) the envelope loop's `j` and its strength-reduced pointer are MIRRORED: retail keeps j in a1
//      (`li a1,0x3` in the flags-test delay slot, `addiu a1,a1,-1`/`bgez a1`, `addiu a0,a0,-32` in the
//      bgez slot) with the pointer in a0; ours has j in a0 and the pointer in v1. A two-pseudo
//      caller-saved hand-out, the class lever a-dead-if-else-block-flips-a-two-way-callee-saved-hand-out
//      describes for callee-saved pairs - untried for caller-saved pairs.
//  (b) sched2 sinks ours' `sb s5,-42(s0)` (elementIndex) and `sw ...,-40(s0)` (soundId) to the end of
//      the fourteen zero stores; retail emits them early, right after `sb v0,-41(s0)`.
//  (c) `and v0,v0,v1` (retail) vs `and v1,v1,v0` - commutative operand order, no byte cost.
// MEASURED DEAD on this body: preset screen (gcc-2.7.2-cdk 81.24, -O3 compile-failed, -G0 83.91, -O1
// 72.29 - all on the 83.91 body); `u16 activeFlag` + separate store (byte-identical to the ternary);
// declaration order of `count` (byte-identical); aliveBits after the ternary 76.92 / 528 / 132; the
// u16-local and `aliveBits |= bit;` last both worse. Below 99%, so no permuter run.
/*
// Round 523 re-measured the un-parked body: 94.99% / 544 bytes / 136 insns (retail's exact size), 28 differing lines. ONE cluster CLOSED this round: the aliveBits ternary - 'pElement->active_flag = (bit & mute) ? 0x421 : 0x401;' instead of '(mute & bit)' gives 'and v0,v0,v1' (retail) for our 'and v1,v1,v0', 95.14% / 544 / 136 at 24 differing lines. That is the shipped body. Also measured and REVERTED: a 'do { } while (0);' block boundary around the whole store cluster (status_flags .. pScriptIP) - unlike the empty statements tried in rounds 496/506 this label DOES survive in 2.6.0 and it does move the caller-saved a0/a1/v1 hand-out, but to 94.22% / 62 differing lines, so the lever a-dead-if-else-block-flips-a-two-way-callee-saved-hand-out reaches this caller-saved pair in the wrong direction. NAMED RESIDUAL now: clusters (a) and (b) are ONE cause, retail's schedule of the store block - retail places 'lhu v1,16(s8)' (soundId) and 'addiu a0,s1,0x60' (the envelopes pointer) at the TOP of the block and stores the soundId 3 insns after its load, so v1 dies before the ip sum ('lhu v1,0(s3); addu v1,s8,v1') and the early envelopes pointer takes a0, pushing the loop counter to a1; ours sinks 'sw v1,-40(s0)' to the END of the fourteen zero stores, so v1 stays live across the cluster (the ip sum then lands in v0), the pointer is born 20 insns later and takes v1, and j takes a0. Nothing in the source order moves it (the store order already matches retail's and the load is already early), so this is sched2 - do not re-enter without a way to shorten the soundId value's live range.
/*
void func_8003B424(AudioManager* pManager) {
    s32 count = pManager->elementCount;
    u8* pSeqData;
    AudioElement* pElement;
    u16* pScriptOffset;
    SoundWDSEntry* pWds;
    u32 ip;
    u32 aliveBits;
    s32 bit;
    s32 i;
    s32 voice;
    s32 j;
    u32 mute;

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
        pWds = g_SoundWdsLinkedList;
    }

    do {
        if (*pScriptOffset != 0) {
            mute = pManager->muteFlags;
            bit = 1 << i;
            aliveBits |= bit;
            pElement->active_flag = (bit & mute) ? 0x421 : 0x401;
            if (pManager->flags & 4) {
                pElement->active_flag |= 4;
            }

            pElement->status_flags = 0x170;
            pElement->rampFlags = 0;
            pElement->unk_0x07 = 0x10;
            pElement->elementIndex = i;
            pElement->soundId = *(u16*)(pSeqData + 0x10);
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
            ip = (u32)pSeqData + *pScriptOffset;
            pElement->scriptStartIP = ip;
            pElement->pScriptIP = ip;

            for (j = NUM_VOICE_ENVELOPES - 1; j >= 0; j--) {
                pElement->envelopes[j].flags = 0;
            }

            pElement->sndId = (u8)pManager->sndId;
            pElement->pWds = (u32)pWds;
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
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003B424);




// Round 463 re-measure: 96.74%, retail's exact 748 bytes / 187 insns, the same 4 clusters and the
// same single cause (srl v0,s8,0xf against srl v0,v0,0x16). No new spelling attempted: lever
// d-combine-folds-through-any-intervening-statement measures that the fold survives copies,
// reorders and declaration changes, and its two named doors are both shut here - a different basic
// block would need a branch retail does not have (any branch between the two shifts changes bytes,
// so it is unspellable by construction, not by measurement), and an untracked value would need the
// volume to come from memory, i.e. a stack slot retail does not have. Clusters 2-4 stay downstream
// as round 450 proved.
// Round 450 re-measure: 96.74%, retail's exact 748 bytes / 187 insns, 8 differing
// lines in 4 clusters - unchanged, and the body is real (not a stale object).
// Cluster 1 is the wall (lever d-combine-folds-through-any-intervening-statement):
// retail srl v0,s8,0xf then andi v0,v0,1 reads the volume VARIABLE, every spelling
// here folds to srl v0,v0,0x16 off the mflo result.
// NEW this round, and the reason clusters 2-4 are not worth reordering: the fold is
// not register-neutral. Retail's first srl reads v0 and kills it; the folded form
// drags v0 (the mflo result) four bytes further, and that is exactly the live range
// competing for the later lhu t0,24(sp) (pan) / addiu a0,s1,96
// (&pElement->voice_data) placements and for the lbu v0,22(s2) slot near the loop
// latch. So clusters 2-4 are DOWNSTREAM of cluster 1, not independent statement-order
// differences: no reorder can close them while the fold stands.
// Also measured this round: -O1 67.76, -O3 compile-fails (cc1 rejects), so the unit's
// -O2 is the only level. Preset already ruled out in round 437 (cdk 94.57, 2.7.2
// 94.72 vs 2.6.0's 96.74) and two permuter runs are PERMUTER_NONE (base 190 best 190).
// Do not reopen without a new compiler.
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
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003B644);


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

    start = GetRCnt(RCntCNT2);

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
                    SoundTickTempoAndElementRamps(pManager, pElements, count);
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

    end = GetRCnt(RCntCNT2);
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
// `pFlags` holds &status_flags (the loop's offset-2 giv, $a0 in retail) and the volatile cast on the
// rampFlags store keeps reorg from sinking that store into the `bne` delay slot.
void SoundTickTempoAndElementRamps(AudioManager* pManager, AudioElement* pElement, u32 elementCount) {
    u16 activeFlags;
    s16 count = elementCount;
    u16 tickCounter = pManager->tempoFrames;
    u32 fermataTimer;
    s32 fermataTicks;
    s32 restTicks;
    u16 statusFlags;
    u16 elementFlags;
    u16* pFlags;

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
        pFlags = &pElement->status_flags;
        activeFlags = pElement->active_flag;
        if (activeFlags != 0) {
            fermataTimer = *(u32*)&pElement->fermataDuration;
            fermataTicks = fermataTimer & 0xFFFF;
            restTicks = fermataTimer >> 16;
            statusFlags = *pFlags;

            if (fermataTicks != 0) {
                elementFlags = pElement->rampFlags;

                if (elementFlags & SOUND_RAMP_FLAG_ELEMENT_VOLUME) {
                    statusFlags |= SOUND_STATUS_VOLUME_CHANGE;
                    if (--pElement->elementVolumeFrames == 0) {
                        elementFlags &= ~SOUND_RAMP_FLAG_ELEMENT_VOLUME;
                    }
                    pElement->elementVolume += pElement->elementVolumeStep;
                }
                if (elementFlags & SOUND_RAMP_FLAG_NOTE) {
                    statusFlags |= SOUND_STATUS_PLAYING_NTOE;
                    if (!(elementFlags & 0x2)) {
                        if (--pElement->noteFrames == 0) {
                            elementFlags &= ~SOUND_RAMP_FLAG_NOTE;
                        }
                    }
                    pElement->note += pElement->noteStep;
                }
                if (elementFlags & SOUND_RAMP_FLAG_PAN) {
                    if (--pElement->panFrames == 0) {
                        pElement->pan = pElement->panTarget;
                        elementFlags &= ~SOUND_RAMP_FLAG_PAN;
                    } else {
                        pElement->pan += pElement->panStep;
                    }
                    statusFlags |= SOUND_STATUS_VOLUME_CHANGE;
                }
                if (elementFlags & SOUND_RAMP_FLAG_VOLUME) {
                    if (--pElement->volumeFrames == 0) {
                        pElement->volume = pElement->volumeTarget;
                        elementFlags &= ~SOUND_RAMP_FLAG_VOLUME;
                    } else {
                        pElement->volume += pElement->volumeStep;
                    }
                    statusFlags |= SOUND_STATUS_VOLUME_CHANGE;
                }
                *(volatile u16*)(pFlags + 1) = elementFlags;
                if (--fermataTicks == 1) {
                    if (activeFlags & 0x1000) {
                        pElement->voice_data.releaseRate = 6;
                        pElement->voice_data.flags |= SOUND_VOICE_FLAG_RELEASE;
                    }
                }
                if (--restTicks == 0) {
                    pElement->active_flag |= SOUND_CHANNEL_REST_NOTE;
                    statusFlags |= SOUND_STATUS_REST_NOTE;
                }
                *(u32*)&pElement->fermataDuration = fermataTicks + (restTicks << 16);
            }
            *pFlags = statusFlags;
        }
        pElement++;
    } while (--count != 0);
}
// 75.84% / 1440 bytes against retail's 1436 (ONE instruction long). Round 442 un-parked and
// re-measured this body unchanged, so the number is live, and cc1_probe was used to settle the
// two type questions below instead of guessing at them.
// WHAT IS RIGHT: the element loop, the note path, the gate/duration arithmetic, the envelope reset
// and the frame (0x48, s0-s8+ra, 32 bytes of locals) are instruction for instruction right.
// (1) THE SCAN LOOP'S COMMAND VALUE IS TWO DIFFERENT TYPES IN RETAIL, and no one declaration of
//     `cmd` gives both. Retail sign-extends once per iteration (`sll v1,a2,0x10` in the preheader
//     .L8003C98C and the latch .L8003CA58 delay slots, `sra v1,v1,0x10` at the loop top) and runs
//     the 0x90/0x80/0x81/0x99 equality chain on v1, while the `< 0x80` tests (`slti v0,a2,0x80`,
//     SIGNED, on the bare lbu register) and the length-table index (`addiu v0,a2,-0x80 / sll v0,0x10
//     / sra v0,0xe`) both work off that raw register. cc1_probe, four spellings, round 442:
//       u8 cmd + `(s16)(cmd-0x80)` index: retail's 3-instruction index, but `< 0x80` becomes
//         `andi 0xff` + `sltu` (retail has no andi and a signed compare); 75.02% in place.
//       s16 cmd: signed compare plus the HImode value, but the index collapses to `addu / sll 2`;
//         74.51% in place.
//       int cmd, and int cmd with `(s16)cmd` in the chain: gcc folds the cast and compares the raw
//         byte, so the `sll 16/sra 16` pair CANNOT come from a cast on a byte-range-known value -
//         retail's dispatch value had an unknown range, i.e. it is probably not the lbu at all.
// (2) THE +1 INSTRUCTION IS THE INLINED SCAN-LOOP BOTTOM, in the 0x90 arm: retail ends it
//     `bnez s1,.L8003CA48` (one jump to the SHARED latch); ours inverts the test and inlines
//     `lbu a2,0(s1) / slti / bnez` plus a copy of the 0x80 test behind it. `while (1) { if (cmd <
//     0x80) break; ... }` removes the duplication but reorders every block (48.89%, 1396 bytes);
//     the do-while keeps the layout. An explicit `continue` in the 0x90 arm compiles identically.
// (3) BLOCK ORDER is the dominant residual (51 of the 82 differing mnemonic lines are pure register
//     renames). Retail puts the handler block (.L8003C8A0), the clear-channel-bit block
//     (.L8003C90C) and the 0x81/0x80 arms (.L8003C928/C93C) OUT OF LINE between the note path and
//     .L8003C950, and points nothing at the epilogue at `sw s1,0x12(s0)` (it emits bnez + j +
//     `addiu s0,s0,0x158` in the delay slot); ours inlines the arms into the scan loop and cse's
//     the second active_flag load (`move s3,v1` where retail re-loads). No source shape tried
//     reproduces retail's layout.
// OTHER MEASURED-AND-REVERTED SPELLINGS (do not repeat): `u8 note` 75.02; selector advance moved
// before the note add 75.53; `cmd == 0x80 || (cmd >= 0xB0 && cmd <= 0xB1)` merged 71.94 (gcc had
// already merged the tails); `*(volatile u16 *)&pElement->active_flag` at the second read 75.67 /
// 1444; `s16 handlerIndex = cmd - 0x80` local inside the else arm is identical to the cast.
// NOT THE PRESET: gcc-2.7.2 scores 74.47 and breaks neighbours that already match under 2.6.0
// (SoundEnableAllSpuChannels 92.73, SoundSetReverbModeWithAllocation 99.04, SoundSetCdVolumeWithFade
// 98.46); gcc-2.7.2-cdk rejects the unit (cc1 exit 33). XenoLibrary/2.6.0 is right.
// HEADER EVIDENCE, shared header so not applied: retail reads durationBias (0x60) with `lbu` then
// `sll 24 / sra 24`, i.e. the source sign-extends an UNSIGNED byte; the header declares it s8 and we
// emit a bare `lb`. Same shape at octave (0x66): retail `lbu 0x64(s0)`, header u16, ours `lhu`.
// SEMANTIC NOTES on the body, read off the .s: the 0x80 / 0x81 / 0xB0-0xB1 arms set and clear 0x200
// on ACTIVE_FLAG (0(s2), SOUND_CHANNEL_NTOE), not on status_flags; the script loop's continue test
// is `active_flag & 0x500`; the `cmd < 0x80` mark at the end sets/clears 0x1000 on active_flag;
// gateRatio is tested 0xF first and 0x10 second; the envelope loop reads flags once into a local.
// twins: none (twin_for.py). m2c re-drafts the same block structure with raw types (round 429).
// lever.py "fold the register names out of the two listings" reaches the same verdict: this is a
// block-order wall, not a per-instruction one - the permuter cannot fix it either (75.84%).
/*
void func_8003C6E8(AudioManager* pManager, AudioElement* pElement, s32 count) {
    u8* pScript;
    AudioLoop* pLoop;
    AudioEnvelope* pEnvelope;
    u32 activeFlags;
    s32 noteStarted;
    s32 cmd;
    u8 selector;
    s32 note;
    u8 length;
    u16 envelopeFlags;
    s32 ticks;
    s32 delta;
    u16 gateTicks;
    s32 i;
    s32 next;
    u32 spare[2];

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
                        pScript = g_SoundScriptHandlers[(s16)(cmd - 0x80)](pScript, pManager, pElement);
                        if (pElement->active_flag == 0) {
                            pManager->activeElementMask &= ~(1 << pElement->elementIndex);
                            break;
                        }
                    }

                    if (pElement->active_flag & (SOUND_CHANNEL_HOLD_NOTE | SOUND_CHANNEL_REST_NOTE)) {
                        break;
                    }
                }

                pElement->pScriptIP = (u32)pScript;

                if (pElement->active_flag != 0) {
                    if (pElement->active_flag & 0x800) {
                        pElement->active_flag |= 0x200;
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
                            } else if (cmd == 0x80) {
                                pElement->active_flag &= ~SOUND_CHANNEL_NTOE;
                                break;
                            } else if (cmd == 0x81) {
                                pElement->active_flag |= SOUND_CHANNEL_NTOE;
                                break;
                            } else if (cmd >= 0xB0 && cmd <= 0xB1) {
                                pElement->active_flag &= ~SOUND_CHANNEL_NTOE;
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
                                    pScript += g_SoundScriptCommandLengths[(s16)(cmd - 0x80)];
                                }
                            }
                        } while ((cmd = *pScript) >= 0x80);
                    }
                    if (cmd < 0x80) {
                        pElement->active_flag |= 0x1000;
                    } else {
                        pElement->active_flag &= ~0x1000;
                    }

                    ticks = pElement->durationBias + pElement->fermataDuration;
                    if ((s16)ticks <= 0) {
                        ticks = pElement->fermataDuration + ticks;
                        pElement->durationBias = pElement->durationBias + (u8)pElement->fermataDuration;
                    }

                    if (pElement->active_flag & 0x600) {
                        gateTicks = 0x7FFF;
                    } else if (pElement->gateRatio == 0xF) {
                        gateTicks = ticks - 1;
                    } else if (pElement->gateRatio == 0x10) {
                        gateTicks = ticks;
                    } else {
                        gateTicks = (u32)((s16)ticks * pElement->gateRatio) >> 4;
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
                            envelopeFlags = pEnvelope->flags;
                            if ((envelopeFlags & 3) == 3) {
                                pEnvelope->value = 0;
                                pEnvelope->count = 1;
                                pEnvelope->delayCount = pEnvelope->delayReload;
                                pEnvelope->rampLevel = pEnvelope->rampStep;
                                pElement->status_flags |= SOUND_STATUS_VOLUME_CHANGE;
                                pEnvelope->flags = envelopeFlags & ~0xC;
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

*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003C6E8);



void SoundSetElementPercussionTone(AudioManager* pAudioManager, AudioElement* pElement, u8 percussionIndex) {
    SoundPercussionTone* pTone;
    s32 note;
    u16 statusFlags;

    pTone = &((SoundPercussionTone*)pAudioManager->pPercussionData)[percussionIndex];

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
        pAudioElements->status_flags &= ~(SOUND_STATUS_KEY_ON | SOUND_STATUS_REST_NOTE);
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
        pAudioElements->voice_data.modeFlags |= SOUND_VOICE_MODE_PITCH_MODULATION;
    }

    return pScript;
}

u8* SoundScriptSetVoiceFlags1000ClearMode(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    if (pAudioElements->voice_number & 0x1) {
        pAudioElements->voice_data.flags |= 0x1000;
        pAudioElements->voice_data.modeFlags &= ~SOUND_VOICE_MODE_PITCH_MODULATION;
    }

    return pScript;
}

u8* SoundScriptSetNoiseClock(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioManager->noiseClock = *pScript++;
    SpuSetNoiseClock(pAudioManager->noiseClock);
    pAudioElements->voice_data.flags |= SOUND_VOICE_FLAG_NOISE;
    pAudioElements->voice_data.modeFlags |= SOUND_VOICE_MODE_NOISE;
    return pScript;
}

u8* SoundScriptAddNoiseClock(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioManager->noiseClock = (pAudioManager->noiseClock + *pScript++) & 0x3F;
    SpuSetNoiseClock(pAudioManager->noiseClock);
    pAudioElements->voice_data.flags |= SOUND_VOICE_FLAG_NOISE;
    pAudioElements->voice_data.modeFlags |= SOUND_VOICE_MODE_NOISE;
    return pScript;
}

u8* SoundScriptSetVoiceFlags2000AndMode(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->voice_data.flags |= SOUND_VOICE_FLAG_NOISE;
    pAudioElements->voice_data.modeFlags |= SOUND_VOICE_MODE_NOISE;
    return pScript;
}

u8* SoundScriptSetVoiceFlags2000ClearMode(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->voice_data.flags |= SOUND_VOICE_FLAG_NOISE;
    pAudioElements->voice_data.modeFlags &= ~SOUND_VOICE_MODE_NOISE;
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
    pAudioElements->voice_data.modeFlags |= SOUND_VOICE_MODE_REVERB;
    return pScript;
}

u8* SoundScriptSetVoiceFlags4000ClearMode(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->voice_data.flags |= 0x4000;
    pAudioElements->voice_data.modeFlags &= ~SOUND_VOICE_MODE_REVERB;
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
        pAudioElements->rampFlags |= SOUND_RAMP_FLAG_NOTE;
        pAudioElements->noteStep = delta / value;
    } else {
        pAudioElements->rampFlags &= ~SOUND_RAMP_FLAG_NOTE;
    }
    return pScript + 2;
}

u8* SoundScriptToggleUnk04Bit2(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->rampFlags ^= 0x2;
    return pScript;
}

u8* SoundScriptClearUnk04Bit1(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->rampFlags &= ~SOUND_RAMP_FLAG_NOTE;
    return pScript;
}

u8* SoundScriptSetUnk70(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 value = *pScript++;

    pAudioElements->slideFrames = value;
    if (value) {
        pAudioElements->rampFlags |= SOUND_RAMP_FLAG_SLIDE;
    } else {
        pAudioElements->rampFlags &= ~SOUND_RAMP_FLAG_SLIDE;
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
            pAudioElements->envelopes[0].rampStep = SOUND_ENVELOPE_RAMP_SCALE;
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

// Round 478: re-measured 91.13% (268 bytes = retail's size, 67 insns both) -- twin port of
// SoundScriptSetTriangleEnvelope0 unchanged; the whole body including every store order and the
// sll/sra mult chain is retail's. Residual = ONE allocator decision: retail holds element in s1 /
// velocity s2 / index s3 (its ndi writes a2 and a 'move s3,a2' relocates index), this body gets
// index s1 / element s2 / velocity s3, which also pushes the arg copy into the jal delay slot
// (retail's slot holds the sra). -dl/-dg on the compiled unit: pScript 71 used 5/48, element 73
// 12/46, velocity 74 6/34 (2 bytes), mode 78 3/22 (1 byte), index 79 4/17, flags 97 2/2; local-alloc
// hands index s1 in this body and s3 in retail's. Tried this round, all 67 insns / 268 bytes:
// u8 index 87.51 (the andi wanders), flags-before-index 88.40 (confirmed the old measurement).
// Below 99, so no permuter run.
// Round 491: re-measured 91.13% (268 bytes = retail's size, 67 insns both) -- the twin port of
// SoundScriptSetTriangleEnvelope0 below; the whole body including every store order and the
// sll/sra mult chain is retail's. Residual = an allocator ROTATION of the five callee-saved values.
// Retail: pScript s4, flags s0, element s1, velocity s2, mode s3 (index reuses s3 after mode dies).
// This body: pScript s4, mode s0, index s1, element s2, velocity s3. -dl on it: 73 element 12 refs/46
// insns (3.13), 74 velocity 6/34 2 bytes (0.70), 78 mode 3/22 1 byte (0.27), 79 index 4/17 BLOCK-LOCAL
// (so local-alloc, not global-alloc, hands it s1); -dg order 90 75 73 74 71 78 with dispositions
// 71 in 20, 73 in 18, 74 in 19, 78 in 16, 79 in 17. Retail's map needs flags (2 refs in 2 insns,
// quotient ~4) to be the first callee-saved candidate and index NOT to be a pseudo of its own.
// Measured this round, all 268 bytes / 67 insns: masking mode IN PLACE ('mode &= 0xF;' at the index
// site, no index variable) gives 90.52% with retail's EXACT prologue save order and element in s1 --
// residual is then velocity/mode swapped (s3/s2), the flags 'andi' scheduled one step earlier and
// 'move a2,s2' still in the jal slot where retail has the sra; 'flags' computed before a separate
// index re-confirmed at 88.40 (50 diff lines). So the next round should start from the in-place
// 'mode &= 0xF' body and attack the last mirrored pair, not from this one. Below 99, no permuter run.
// Round 512: the in-place 'mode &= 0xF' body (flags computed before the mask) measures 90.52% at
// retail's exact 268 bytes / 67 instructions with 30 differing lines and retail's EXACT prologue
// save order (sw s4, sw s1, sw ra, sw s3, sw s2, sw s0), element in s1, flags in s0 and pScript in
// s4 all retail's. Judge this family by the differing-line count, not the percentage: the older
// 's32 index = mode & 0xF' body reads 91.13% with 50 differing lines and element in s2.
// Residual = ONE adjacent pair in the global-alloc priority list. Retail keeps velocity in s2 and
// mode in s3; this body mirrors them (velocity s3, mode s2), and that also pushes the flags 'andi'
// one slot early (retail's bgez-a1 slot holds 'sll a0,a0,0xe') and puts the call-arg copy in the
// jal slot where retail has the sra. Retail's 'andi a2,s3,0xf' + 'move s3,a2' is index's own copy
// into mode's dead register, i.e. retail keeps index a SEPARATE pseudo (what the 91.13% body does,
// and that body then coalesces the two back into one register), so no single body is both.
// cc1 -da -dl -dg on this body: '6 regs to allocate: 89 77 73 75 76 71', dispositions
// 71 in 20 (s4), 73 in 17 (s1), 75 in 18 (s2), 76 in 19 (s3), and 94/113/119/122 all in 16 -- s0 is
// taken by LOCAL-alloc (the flags chain) exactly as in retail, so only the callee-saved hand-out
// matters. Pseudos: 73 element 12 refs / 46 insns, 76 velocity 6 / 33 (2 bytes), 75 masked mode
// 7 / 25 (1 byte). The hand-out follows the printed list order (73->17, 75->18, 76->19, 71->20), so
// the fix is velocity ahead of masked mode, i.e. velocity's allocno quotient above mode's.
// Measured this round, all 268 bytes / 67 insns: 's32 mode' byte-identical, 'velocity += ...'
// byte-identical, mode declared first and assigned late 87.66% / 32 lines. Below 99, no permuter run.
/*
u8* func_8003D9A4(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s16 velocity = pScript[0];
    s32 note = (s8)pScript[1];
    u8 mode = pScript[2];
    s32 flags;

    if (note != 0) {
        if (velocity != 0) {
            if (note < 0) {
                note = note * -note;
            } else {
                note = note * note;
            }
            velocity = velocity + ((s16)velocity * (s16)velocity) / 64;
            flags = ((mode & 0x10) == 0) * 2;
            mode &= 0xF;
            pAudioElements->envelopes[0].delta = SoundCalculateEnvelopeDelta(note << 14, velocity, mode);
            pAudioElements->envelopes[0].rampStep = SOUND_ENVELOPE_RAMP_SCALE;
            pAudioElements->envelopes[0].reload = velocity;
            pAudioElements->envelopes[0].delayReload = 0;
            pAudioElements->envelopes[0].pStepHandler = g_SoundEnvelopeStepHandlers[mode];
            pAudioElements->envelopes[0].handlerIndex = mode;
            pAudioElements->envelopes[0].target = 0;
            pAudioElements->envelopes[0].flags = flags + 1;
            pAudioElements->activeEnvelopeMask |= 0x1;
            SoundResetEnvelope(&pAudioElements->envelopes[0]);
        }
    }

    return pScript + 3;
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003D9A4);



u8* SoundScriptSetEnvelope0Rate(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 divisor = (u8)(*pScript + 1);
    s16 step;

    pScript++;
    if (divisor) {
        divisor *= 4;
        step = SOUND_ENVELOPE_RAMP_SCALE / divisor;
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
    pAudioElements->rampFlags &= ~(SOUND_RAMP_FLAG_ELEMENT_VOLUME | SOUND_RAMP_FLAG_QUEUED);
    return pScript + 1;
}

// Crescendo?
u8* SoundScriptAddElementVolume(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    pAudioElements->elementVolume = (pAudioElements->elementVolume + ((s8)*pScript << 24)) & 0x7FFFFFFF;
    pAudioElements->status_flags |= SOUND_STATUS_VOLUME_CHANGE;
    pAudioElements->rampFlags &= ~(SOUND_RAMP_FLAG_ELEMENT_VOLUME | SOUND_RAMP_FLAG_QUEUED);
    return pScript + 1;
}

u8* SoundScriptSetElementVolumeRamp(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u16 frames = *pScript;
    s32 delta = ((s8)pScript[1] << 24) - pAudioElements->elementVolume;

    if (frames && delta) {
        pAudioElements->elementVolumeFrames = frames;
        pAudioElements->rampFlags = (pAudioElements->rampFlags | SOUND_RAMP_FLAG_ELEMENT_VOLUME) & ~SOUND_RAMP_FLAG_QUEUED;
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
        pAudioElements->rampFlags = (pAudioElements->rampFlags | SOUND_RAMP_FLAG_QUEUED) & ~SOUND_RAMP_FLAG_ELEMENT_VOLUME;
    } else {
        pAudioElements->rampFlags &= ~SOUND_RAMP_FLAG_QUEUED;
    }

    return pScript + 3;
}

u8* SoundScriptSetTriangleEnvelope1(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s8 note = pScript[1];
    s16 velocity = pScript[0];
    s32 delayReload;

    if (note != 0) {
        if (velocity != 0) {
            velocity = velocity + ((s16)velocity * (s16)velocity) / 64;
            pAudioElements->envelopes[1].delta = SoundCalculateEnvelopeDelta(note << 24, (s16)velocity, 2);
            pAudioElements->envelopes[1].reload = velocity;
            delayReload = pScript[2] << 2;
            pAudioElements->envelopes[1].rampStep = SOUND_ENVELOPE_RAMP_SCALE;
            pAudioElements->envelopes[1].pStepHandler = (undefined32)SoundEnvelopeStepTriangle;
            pAudioElements->envelopes[1].handlerIndex = 2;
            pAudioElements->envelopes[1].target = 1;
            pAudioElements->envelopes[1].flags = 3;
            pAudioElements->envelopes[1].delayReload = delayReload;
            pAudioElements->activeEnvelopeMask |= 0x2;
            SoundResetEnvelope(&pAudioElements->envelopes[1]);
        }
    }

    return pScript + 3;
}


// Round 496: 93.66% (244 bytes / 61 insns = retail's exact size), big jump from the 84.75 park by
// PORTING the twin func_8003E04C body (that function is instruction-identical apart from envelope
// index 1 vs 2, target 1 vs 2, mask 0x2 vs 0x4) - the short volume, the split rateMask (test emitted
// before the index) and the pEnvelope pointer local are what the shape needs. Residual is the same
// one func_8003E04C has been stuck on since round 411: callee-saved identity. Retail s1=pAudioElements
// s2=volume s3=mode/index with the index made into the call-arg register then copied to its home
// (`andi a2,s3,0xf; move s3,a2`) so the jal slot holds `sra a1,a1,0x10`; here s1=mode/index,
// s2=pAudioElements, s3=volume, `andi s1,s1,0xf` in place and `move a2,s1` in the jal slot - same
// instruction COUNT, 48 differing lines, all register names. Also tried here: `s32 index = mode & 0xF;`
// as an initialiser inside a bare block after the rateMask statement (the round-485 lead, scoped so it
// cannot hoist) - byte-identical 93.66/61, so the tie is not about where index is born.
// Round 506: 92.95% (244 bytes / 61 insns = retail's exact size), 26 differing lines, and retail's
// EXACT prologue save order (`sw s1 / sw ra / sw s3 / sw s2 / sw s0`) - the in-place masked form
// (lever an-in-place-mask-moves-the-hand-out-and-the-prologue-save-order). The rival body, the twin
// func_8003E04C shape with a separate `s32 index = mode & 0xF;` and the split rateMask, measures
// 93.66% but at 48 differing lines and rotates the whole callee-saved triple (element s2, volume s3,
// masked s1, saves s2 before s1). Judge by lines/prologue: lever round 497 measured both on E04C and
// round 512 re-measured them on D9A4.
// NAMED RESIDUAL: `volume` and `mode` mirrored - retail lbu s2 (volume) / lbu s3 (mode), ours s3/s2 -
// and everything downstream of it (addu 266-sh base, `sb s3,277` handlerIndex, the bgez slot: retail
// keeps `sll a0,a0,0x18` there, ours schedules `andi s0,s2,0x10` and loses retail's
// `andi a2,s3,0xf` + `move s3,a2` index copy). Round 512's -dl/-dg read on D9A4 (the envelopes[0]
// copy) shows `mode` and `volume` as an ADJACENT pair in the printed hand-out list (`6 regs to
// allocate: 89 77 73 75 76 71`, 75 in 18 / 76 in 19), which is why no spelling of either moves it;
// retail needs volume's quotient ABOVE mode's, and round 497 already measured dead on these bodies:
// every declaration order, both store orders (reload before rampStep), an empty do{}while(0), and
// `s32 volume` (60 insns, loses the write-back `move`). Below 99%, so no permuter run.
// Round 526 read this body's OWN -da -dl -dg (gcc-2.6.0, `7 regs to allocate: 82 90 73 78 74 71 75`,
// dispositions 71 in 20 / 73 in 17 / 74 in 19 / 75 in 4 / 78 in 18) and the sort key is
// `floor_log2(n_refs) * n_refs / live_length`, with NO size term: 2.67, 2.50, 0.900 (element 12 refs
// across 40), 0.467 (mode 7 / 30), 0.385 (volume 5 / 26), 0.238 (pScript 5 / 42), 0.200 - monotone,
// so the earlier "volume's quotient exceeds mode's" reading came from a size factor the sort does not
// use. `mode` here is ONE merged pseudo carrying 7 references across the mask and the call, so it
// outranks volume and holds s2 where retail holds s3. Volume can only clear 0.467 with 7 references
// (0.538) or a live range of 21 insns (it is 26), and retail's own 244-byte stream pins both: its
// five mentions ARE lbu / move v0 / addu / move back / sh 266, and the reload store sits after the
// call by retail's own order. Measured dead this round, all byte-identical at 92.95 / 244 / 61: the
// statement order rateMask, mode mask, flags (identical to rateMask, flags, mode mask), a
// one-iteration do{}while(0) around the post-call store cluster, and splitting the ramp into
// `s32 square = volume * volume;` plus `volume += square / 64;` (cse adds no reference to volume).
// A separate masked value (`s32 index = mode & 0xF;`) buys the RIGHT quota order - its -da list is
// `83 91 73 74 71 75 78`, element 0.900 then volume 0.385 then raw mode 0.214, which is retail's
// order - but pseudo 78 then prints `preferences: 17`, so mode takes s1 BY PREFERENCE (from its
// coalescing with index) and pushes element to s2: 93.66% / 244 bytes / 61 insns but 48 differing
// lines and a mirrored prologue. That preference, not the quota, is the remaining lead. `u8 index`
// instead of `s32 index` moves mode to s0 (worse).
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003DD24);
/*  92.95% / 244 bytes / 61 insns / 26 differing lines, retail's exact prologue save order:
u8* func_8003DD24(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    short volume = pScript[0];
    s32 note = (s8)pScript[1];
    u8 mode = pScript[2];
    AudioEnvelope* pEnvelope;
    s32 flags;
    s32 rateMask;

    if (note != 0) {
        if (volume != 0) {
            volume += (volume * volume) / 64;
            rateMask = mode & 0x10;
            mode &= 0xF;
            flags = (rateMask == 0) * 2;
            pEnvelope = &pAudioElements->envelopes[1];
            pEnvelope->delta = SoundCalculateEnvelopeDelta(note << 24, (s16)volume, mode);
            pEnvelope->rampStep = SOUND_ENVELOPE_RAMP_SCALE;
            pEnvelope->reload = volume;
            pEnvelope->delayReload = 0;
            pEnvelope->pStepHandler = g_SoundEnvelopeStepHandlers[mode];
            pEnvelope->handlerIndex = mode;
            pEnvelope->target = 1;
            pEnvelope->flags = flags + 1;
            pAudioElements->activeEnvelopeMask |= 0x2;
            SoundResetEnvelope(pEnvelope);
        }
    }

    return pScript + 3;
}
*/


u8* SoundScriptSetEnvelope1Rate(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    s32 divisor = (u8)(*pScript + 1);
    s16 step;

    pScript++;
    if (divisor) {
        divisor *= 4;
        step = SOUND_ENVELOPE_RAMP_SCALE / divisor;
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
            pAudioElements->rampFlags |= SOUND_RAMP_FLAG_PAN;
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
        step = SOUND_ENVELOPE_RAMP_SCALE / divisor;
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
            pAudioElements->envelopes[2].rampStep = SOUND_ENVELOPE_RAMP_SCALE;
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

// Round 411: 92.30% (244 bytes = retail's size, 61 insns = retail's, 12 differing lines,
// two hunks). Body is the parked one: envelope pointer local used for every store and the
// reset, and `short volume` - that type flip alone took the park from 84.75 to 90.34 and is
// what produces retail's `move v0,s2` volume copy and the callee-saved assignment element s1
// / volume s2 / index s3. Hoisting `index = mode & 0xF` into the outer if block before the
// volume test scored 89.02 with only 14 differing lines (the best instruction distance seen).
// Residual: (1) retail keeps `mode` in s3 from the lbu and forms the index in the call's a2,
// copying it back (`andi a2,s3,0xf; move s3,a2`), while here mode lands in s0 and the index is
// computed early into s3, so the jal delay slot holds `move a2,s3` instead of retail's
// `sra a1,a1,0x10`; (2) the bgez target is 4 bytes later because of that.
// Also measured: index first inside the inner if (90.34, 48 lines), flags before index inside
// the inner if (87.34), `mode & 0xF` inline in all three uses with flags first (80.20, 256
// bytes) and with the index assigned just after the call (90.54), s32 volume for each (67-84).
// Three permuter runs: 798 -> 595 -> 531 -> 471 (the last found the envelope pointer local).
// Next lead: retail's `move s3,a2` only appears when `mode` keeps s3 and dies at the index, so
// the source needs the `& 0x10` test emitted before the index and the `== 0` after it.
// Round 472: 93.66% (244 bytes = retail's size, 61 insns = retail's, 48 differing lines) with the body below - new best, up from the 92.30 park. The instruction STREAM is now identical instruction-for-instruction to retail; every remaining difference is a callee-saved REGISTER IDENTITY. Retail: s1=pAudioElements s2=volume s3=mode/index, and the index is made by 'andi a2,s3,0xf' + a hoisted copy 'move s3,a2' (the index home) before the call, so the jal delay slot holds 'sra a1,a1,0x10'. Ours: s1=mode/index s2=pAudioElements s3=volume, 'andi s1,s1,0xf' in place and the arg copy deferred to the delay slot ('move a2,s1') - same 61 insns, both spend 2 insns on the index.
// What got here: the flags test must be emitted BEFORE the index ('rateMask = mode & 0x10;' then 'index = mode & 0xF;' then 'flags = (rateMask == 0) * 2;' - that split alone is 93.66 from 92.30, and it is the round-471 note's next lead, now measured). What did not move the register order (all measured this round, all worse or equal): the direct 'pAudioElements->envelopes[2].field' spelling for every store and '&pAudioElements->envelopes[2]' at the reset (90.46 - a sub-word index makes all stores 'sw rt,0x124(s1)' but the allocno order is unchanged, so the pEnvelope local is kept), 'mode & 0xF' inline at the call with 'index = mode & 0xF;' after it (90.54 - cse2 merges to ONE andi that lands just before the jal instead of after the flags test), and 's32 mode' instead of 'u8 mode' (93.66, byte-identical). Parked because the residual is the global allocator's ORDER of the four callee-saved allocnos (ours mode-first, retail pAudioElements-first), not a spelling: reference counts and live ranges of the same three pseudos differ nowhere, so no source edit that keeps the instruction stream intact has shifted it yet.
// Round 485: 93.66% re-measured (244 bytes / 61 insns = retail's, 48 differing lines), still the
// best. 15 more spellings, none above it: all six declaration orders of volume/note/mode, s16 vs
// short volume, u32 vs s32 rateMask, index-then-rateMask, the one-line flags form (93.54), the sum
// split into `quotient` + `volume + quotient` (byte-identical), and an alias for pAudioElements -
// declaration order is not the handle here (the round-419 lever: a hand-out is insensitive to it
// once the machine code is fixed). The one shape that DOES move the hand-out is
// `s32 index = mode & 0xF;` as an initialiser next to the other locals: it gives retail's
// element s1 / volume s2 / index s3 (19 differing lines, the closest yet) but hoists the andi out
// of the `if`, so the body is 62 insns / 248 bytes (88.77) - i.e. only mode's home (theirs s3,
// that shape's s0) is still wrong, and the hoist is what costs the byte. Give the index an
// initialiser that stays inside the block and it collapses back to s1=mode/index.
// SoundScriptSetCurrentEnvelopeDelta, the same script family, DID reach 100% this round with a u8 copy of pScript[0] +
// a separate quotient statement + s16 volume; that trick does not transfer - retail keeps a single
// pScript[0] load here (`addu a1,s2,a1`), so there is no second load to force.
// SAME RETAIL BODY: func_8003DD24 is instruction-for-instruction identical apart from envelopes[0]/target 1/mask 0x2, so this body should be ported there (its park is 84.75).
// Round 497: 92.95% (244 bytes = retail's exact size, 61 insns = retail's, 26 differing
// lines - the closest body this family has had; the previous best was Round 472's 93.66% at
// 48 differing lines, which is a WORSE instruction diff, so this body is the one to keep).
// NEW and the reason to keep it: masking mode IN PLACE (`mode &= 0xF;`, no `index` variable)
// gives retail's EXACT prologue save order (sw s1,20 / sw ra,36 / sw s3,28 / sw s2,24 /
// sw s0,16) and puts the element base in $s1, both retail's - the 93.66% body saved s2 before
// s1 and had element $s2 / volume $s3 / index $s1.
// Residual, two connected shapes: (1) volume and mode are MIRRORED (this body volume $s3 /
// mode $s2; retail volume $s2 / mode $s3); (2) retail forms the masked value in the CALL ARG
// register and copies it to its home ('andi a2,s3,0xf' then 'move s3,a2' before the jal, so the
// jal slot holds 'sra a1,a1,0x10'), where this body masks in place and copies into a2 in the
// slot ('andi s2,s2,0xf', 'sra a1,a1,0x10' before the jal, 'move a2,s2' in the slot). Same 61
// insns / 244 bytes either way, so retail's shape needs the masked value to be a SEPARATE pseudo
// from mode (mode dying at the mask, the masked value live across the call), which is what the
// `index = mode & 0xF;` body does - and it collapses back to one register ($s1) instead.
// -da/-dl/-dg, gcc-2.6.0-psx -O2 -G8: "7 regs to allocate: 82 90 73 75 76 71 74", dispositions
// 71 in 20 (pScript $s4), 73 in 17 (element $s1), 78 in 16 (rateMask/flags $s0), 75 in 18
// (mode $s2), 76 in 19 (volume $s3); lreg 73 element 12 refs / 40 insns pointer, 71 pScript
// 5/42, 75 mode 7/30 "1 bytes", 76 volume 5/26 "2 bytes; dies in 2 places", 78 2/19. Note mode
// is processed BEFORE volume although volume's quotient (2*5*10000*2/26 = 7692) exceeds mode's
// (2*7*10000/30 = 4666), so this pair is not the plain descending-quotient order - before calling
// it closed, find which allocno between them takes the register first.
// Tried this round, all semantics-preserving, none better: s32 volume (60 insns / 240 bytes /
// 86.51, loses the 'move' that writes the sum back); declaration order with separate assignments
// (byte-identical to the initialised form - the pair does not move with declaration order);
// an empty `do { } while (0);` before the pEnvelope store group (optimised away, byte-identical);
// the reload store written before rampStep (byte-identical, sched2 restores retail's order);
// the early-return duplicate-tail form of the two tests (62 insns / 248 bytes / 90.41, gcc did
// not cross-jump the two 'addiu v0,s4,3' tails).
// FAMILY: func_8003D9A4 (envelopes[0], mask 0x1, target 0) and func_8003DD24 (envelopes[1], mask
// 0x2, target 1) are the same retail body; both parks predate this in-place-mask form, so port
// this one to them first - it is byte-identical in shape apart from the index/target/mask.
/*
u8* func_8003E04C(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    short volume = pScript[0];
    s32 note = (s8)pScript[1];
    u8 mode = pScript[2];
    AudioEnvelope* pEnvelope;
    s32 flags;
    s32 rateMask;

    if (note != 0) {
        if (volume != 0) {
            volume += (volume * volume) / 64;
            rateMask = mode & 0x10;
            mode &= 0xF;
            flags = (rateMask == 0) * 2;
            pEnvelope = &pAudioElements->envelopes[2];
            pEnvelope->delta = SoundCalculateEnvelopeDelta(note << 24, (s16)volume, mode);
            pEnvelope->rampStep = SOUND_ENVELOPE_RAMP_SCALE;
            pEnvelope->reload = volume;
            pEnvelope->delayReload = 0;
            pEnvelope->pStepHandler = g_SoundEnvelopeStepHandlers[mode];
            pEnvelope->handlerIndex = mode;
            pEnvelope->target = 2;
            pEnvelope->flags = flags + 1;
            pAudioElements->activeEnvelopeMask |= 0x4;
            SoundResetEnvelope(pEnvelope);
        }
    }

    return pScript + 3;
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003E04C);


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
    pEnvelope->rampStep = SOUND_ENVELOPE_RAMP_SCALE;
    pEnvelope->delayReload = 0;
    pEnvelope->target = last;
    return pScript + 3;
}

// `level` is a u8 copy of the script byte so gcc reloads pScript[0] for the sum (retail's second
// lbu); the divide is its own statement so the mflo lands before that reload.
u8* SoundScriptSetCurrentEnvelopeDelta(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 level = pScript[0];
    s32 pitch = level * level;
    AudioEnvelope* pEnvelope = &pAudioElements->envelopes[pAudioElements->envelopeIndex];
    s32 quotient = pitch / 64;
    s16 volume = pScript[0] + quotient;

    pEnvelope->delta = SoundCalculateEnvelopeDelta(((s8)pScript[1] << 24) | (pScript[2] << 16), volume, pEnvelope->handlerIndex);
    pEnvelope->reload = volume;
    return pScript + 3;
}


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

        step = SOUND_ENVELOPE_RAMP_SCALE / scaledDivisor;
        pEnvelope->delayReload = *pScript << 2;
        pEnvelope->rampStep = step;
        pEnvelope->rampLevel = step;
    }

    return pScript + 2;
}

u8* SoundScriptNop5(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    return pScript;
}

// Round 437 re-measure: 95.31% (124 bytes = the original's size, but MINE IS 31 INSN AND RETAIL 32 - one short, so this body cannot link yet). Everything in the old note still holds: prologue, s-register block (pScript s1, pAudioElements s2, envelope s0, index s3) and statement order are retail's. The residual is exactly retail's addiu s1,s1,1 (in place, scheduled in the 0x1E load-delay slot) followed by addu v0,s1,zero against mine's single addiu v0,s1,1; the flags temp roles (retail v0, mine v1) are downstream of that missing copy. Measured dead this round, all 95.31% / 31 insns, i.e. flow copy-propagates every one of them: bare pScript++; return pScript; (in and out of the do-while wrapper), return ++pScript;, pScript += 1; return pScript;, return pScript += 1;. The one spelling that DOES keep two instructions is moving the increment up so its OLD value is needed - index = *pScript++;, the trick SoundScriptSetVolume in this file uses for the same tail - but here it then lives across the SoundResetEnvelope call and costs 33 insns / 132 bytes / 84.16%, rotating the whole s-block (envelope s3, elements s2). So the open question is a use of pScript's pre-increment value that costs nothing; retail's 32 instructions contain none. Structurally stuck: two permuter runs already found nothing (best 150), do not re-permute.
/* Round 450: 95.31%, 31 insns vs retail 32 (124 bytes, so it cannot link).
   Residual is one shape only: retail keeps the increment in place (addiu s1,s1,1,
   reorg'd into the lhu delay slot) and copies it out at the end (move v0,s1);
   every spelling here gives a single addiu v0,s1,1 and the flags temp moves to
   v1.  New this round: the increment statement placed between the flags write and
   the mask (pScript += 1; mask = ...; return pScript;) is 31 insns, same 95.31.
   Round 437 already measured dead: pScript++/return pScript (bare and in a
   do-while), return ++pScript, pScript += 1; return pScript, return pScript += 1,
   and index = *pScript++ (33 insns, rotates the s-block).  Two permuter runs
   found nothing; the source shape is not reachable with 2.6.0 here.
   Round 463 new axes, all dead: a distinct local returned at the end
   (pReturn = pScript + 1; ... return pReturn;) is 93.38% / 31 insns - gcc folds the
   local into v0 as well - and presets are ruled out too (2.7.2 94.84, cdk 94.84,
   2.6.0 -O3 rejects the file).  Cause named: with one use of the incremented
   pointer, 2.6.0's allocator always coalesces the add into $v0. */
/*
u8* func_8003E360(u8* pScript, AudioManager* pAudioManager, AudioElement* pAudioElements) {
    u8 index = *pScript;
    AudioEnvelope* pEnvelope = &pAudioElements->envelopes[index];
    u16 mask;

    SoundResetEnvelope(pEnvelope);
    pEnvelope->flags |= 0x1;
    pScript += 1;
    mask = 1 << index;
    pAudioElements->activeEnvelopeMask |= mask;
    return pScript;
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003E360);


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
        pAudioElements->status_flags &= ~(SOUND_STATUS_KEY_ON | SOUND_STATUS_REST_NOTE);
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

/* Attempt (98.51%, retail's exact 604 bytes / 151 insns; the instruction multiset, the order of every
instruction, the frame and the saved set all agree): ONE register pair is mirrored. Retail keeps the
SPU voice base in t0 (so the post-loop pSpu pseudo reuses t0) and the loop counter in t1; gcc 2.6.0
gives t0 to the counter and t1 to the base, which also costs the post-loop base its t0 (it lands in
v1) and turns every adsr temp from a0 into v1 - so all 72 differing lines are this one cause.
Tried: for(;;) vs do/while, both pointer increments in every position, local declaration order,
initialisation statement order (fixed the prologue and the loop latch exactly), a modeFlags local,
and a 4-minute permuter run (base=225 best=225, 1172 iterations). Measured this round: the explicit
derived adsr pointer (`u16* pAdsr = (u16*)pVoice + 4; ... pAdsr += 8;`) makes the biv set gcc's own
and scores 93.75% / 620 bytes (155 insns, worse); gcc-2.7.2 is 98.51 and gcc-2.7.2-cdk 98.31, so the
preset is not the answer. This is a local-alloc qty tie: the counter has 5 refs against the base's
4 over the same live range, and no spelling moves either count without changing an instruction.
Re-measured (round 420): the live body still reads 98.51% at retail's 604 bytes / 151 insns, 72
differing lines, that one mirror and nothing else. New spellings tried this round: a u_char counter
(QI's allocno size of 1 cuts its priority 4x, the one term that WOULD flip the pair) - gcc pays an
andi to widen it back for the shift and the compare, 96.36% / 608 bytes / 152 insns, rejected;
-G0 measures the same 98.51 and gcc-2.6.0 -O3 refuses the file. Note retail's t0=base / t1=counter
contradicts the -dg priority formula (base 4 refs, counter 5) rather than just losing a tie, so the
mirror came from a body this C cannot spell: do not re-enter without a new allocator axis.
Round 433 read the dumps instead of guessing (cc1 -dl -dg on build/src/slus_006.64/system/sound.c.i):
both values are GLOBAL allocnos, so global-alloc's refs/live quotient decides them, not -dl's insn
counts - 'Register dispositions: 71 in 3 72 in 9 73 in 5 74 in 11 75 in 7 81 in 8 ...' gives pVoice
= 72 -> t1 and the counter = 81 -> t0, and the allocation order is '73 80 203 75 71 81 72 79 74 78 77
76'. Their numbers: pVoice 8 refs over 96 insns (0.083), counter 13 over 95 (0.137). Both counts are
PINNED by the instruction stream, which is already retail's: pVoice appears only in the preheader
def, the a2 giv, the single 'sh 0(t0)' and its own 'addiu' increment, the counter in the three
sllv, its addiu and the slti. Flipping the pair needs pVoice at 14 refs or the counter at 8, i.e.
two instructions retail does not have, so this is a proof and not a tie - do not re-enter it.
*/
// Round 446 re-measured the un-parked body: 98.51% at retail's exact 604 bytes / 151 insns, 72 differing lines, unchanged. Nothing new attempted: round 433's -dg/-dl dumps prove the t0/t1 mirror is not a local-alloc tie but a global-alloc refs/live quotient with both counts pinned by the instruction stream (pVoice 8 refs/96 insns, counter 13/95; flipping needs 14 or 8, i.e. two instructions retail does not have). Do not re-enter without a new allocator axis.
// Round 459: re-measured 98.51% / retail's exact 604 bytes / 151 insns (72 differing lines, unchanged). Two NEW axes measured, both worse: (1) the counter declared u16 instead of s32 does NOT shrink its allocno mode size, because i is only ever used in promoted SI contexts, so the pseudo stays SI and pays an extra extension insn: 96.36% / 608 bytes / 152 insns, same t0/t1 mirror - this is the exception to a-qiparam-size-one-pins-the-saved-pair, a promoted local cannot be given a smaller allocno_size. (2) sharing one pSpu variable pre- and post-loop (pSpu = g_pSoundSpuRegisters; pVoice = pSpu->_rxx.voice; ... pSpu = g_pSoundSpuRegisters;) costs the coalesce: 97.85% / 608 bytes / 152 insns. So round 446's arithmetic stands: pVoice needs 14 refs (or the counter <= 7) with both pinned by the instruction stream.
/*
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
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/sound", func_8003E900);


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

            if (statusFlags & SOUND_STATUS_KEY_ON) {
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
                            if ((s16)pEnvelope->rampLevel < SOUND_ENVELOPE_RAMP_SCALE) {
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
