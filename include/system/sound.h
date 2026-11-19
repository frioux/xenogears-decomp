#ifndef _XENO_SOUND_INPUT_H
#define _XENO_SOUND_INPUT_H

#include "psyq/libspu.h"
#include "psyq/libcd.h"

#define NUM_VOICES 24

#define NUM_NOTES_PER_OCTAVE 12

#define CHANNEL_RIGHT 0
#define CHANNEL_LEFT  1

typedef struct {
    s16 assignedVoice;
    u16 modeFlags;
    s16 priority;
    u16 flags;
    SpuVolume volume;
    // neither is ever read: retail clears both as halfwords when an element starts
    u16 unkC;
    u16 unkE;
    u8 unk10[4];
    u16 pitch;
    u8 unk16[6];
    u32 startAddress;
    u32 loopAddress;
    u8 attackMode;
    u8 sustainMode;
    u8 releaseMode;
    u8 attackRate;
    u8 adsrDR;
    u8 sustainRate;
    u8 releaseRate;
    u8 adsrSR;
} SoundVoiceData;

#define SOUND_CTL_FLAG_IRQ_HANDLER (1 << 2)
// An SPU transfer command is in flight: the transfer busy-wait loop tests it.
#define SOUND_CTL_FLAG_TRANSFER_BUSY (1 << 4)

#define SOUND_STATUS_OK 0x0
#define SOUND_ERR_INVALID_SIGNATURE 0x1
#define SOUND_ERR_INVALID_CHECKSUM 0x2 // Maybe
#define SOUND_ERR_UNK_0X4 0x4
#define SOUND_ERR_MANAGER_NOT_IN_LIST 0xF
#define SOUND_ERR_ENTRY_ALREADY_EXISTS 0x15
#define SOUND_ERR_UNEXPECTED_CALLBACK 0x26

#define SOUND_TRANSFER_QUEUE_SIZE 8

#define SOUND_SPU_COMMAND_WRITE 0x1
#define SOUND_SPU_COMMAND_READ 0x2

#define SOUND_WDS_ALLOCATE_AT_ADDRESS 0
#define SOUND_WDS_ALLOCATE_AUTOMATIC -1

#define FILE_SIGNATURE(a, b, c, d) (d<<24)+(c<<16)+(b<<8)+a

typedef void (*SoundCommandCallback_t)(void);

typedef struct {
    /* 0x0 */ undefined16 unk0; // Flags?
    /* 0x2 */ undefined16 unk2;
    /* 0x4 */ undefined32 unk4;
    /* 0x8 */ void* pPrev;
    /* 0xC */ void* pNext;
} SoundHeapBlockHeader;


#define MAX_SPU_MEMORY_BLOCKS 0xC

#define SPU_MEMORY_FREE 0x0
#define SPU_MEMORY_RESERVED 0x1 // Or last block?
#define SPU_MEMORY_IN_USE 0x80

typedef struct {
    /* 0x0 */ u_char flags;
    /* 0x1 */ u_char type;
    /* 0x2 */ short nextBlockIndex;
    /* 0x4 */ int spuAddress;
    /* 0x8 */ int size;
    /* 0xC */ u_int pad0xC;
} SoundSpuMemoryBlock;

// Possible a more general queue to sending commands to the SPU,
// but all supported commands has to do with data transfer.
typedef struct {
    /* 0x0  */ u_short commandType;
    /* 0x2  */ short unk2;
    /* 0x4  */ void* pSpuData;
    /* 0x8  */ mem_addr pTransferAddress;
    /* 0xC  */ u_long dataSize;
    /* 0x10 */ SoundCommandCallback_t pCallbackFn;
} SoundTransferCommand;

// Possibly a struct which can either be a SMD (Background Music), SED (Sound Effect) or SND entry
struct SoundFile_t {
    /* 0x0  */ u_int magic;
    /* 0x4  */ undefined32 unk4;
    /* 0x8  */ undefined32 fileSize;
    /* 0xC  */ undefined16 version;
    /* 0xE  */ undefined16 unkE;
    /* 0x10 */ u16 flags; // bit 0 sets SOUND_CHANNEL_* bit 1 on every element the file starts
    /* 0x12 */ u16 unk12;
    /* 0x14 */ u_short sedId;
    /* 0x16 */ u_short sndId;
    /* 0x18 */ u16 volumeTableOffset; // pSoundFile + this + sound id reaches a per-effect volume byte
    /* 0x1A */ u16 unk1A;
    /* 0x1C */ struct SoundFile_t* pNext;
    /* 0x20 */ u_short scriptOffsets[1]; // Pair of scripts for each instrument. 1 script for 1 channel.
};
typedef struct SoundFile_t SoundFile;

// Tone table entry that follows the WDS header at offset 0x30 of a SoundWDSEntry.
typedef struct {
    /* 0x0 */ u32 sampleOffset;
    /* 0x4 */ u16 loopOffset;
    /* 0x6 */ u16 toneTune; // copied into AudioElement::toneTune
    /* 0x8 */ u32 adsr;
    /* 0xC */ u16 adsrModes; // attack/sustain/release mode, 3 bits each
    /* 0xE */ u16 unkE;
} SoundWdsTone; // Size: 0x10

struct SoundWDSEntry_t {
    /* 0x0  */ u_char _unk0[0x10];
    /* 0x10 */ u_int headerSizeMby;
    /* 0x14 */ u_int adpcmDataSize; // Sample size
    /* 0x18 */ u_int adpcmDataOffset; // Offset to data to write to SPU
    /* 0x1C */ u_short unk1C;
    /* 0x1E */ u_short unk1E;
    /* 0x20 */ u_short id;
    /* 0x22 */ u_short unk22;
    /* 0x24 */ u_int unk24;
    /* 0x28 */ int spuMemoryAddress; // Optional
    /* 0x2C */ struct SoundWDSEntry_t* pNext;
    /* 0x30 */ SoundWdsTone tones[1]; // tone table, one entry per tone in the file
};
typedef struct SoundWDSEntry_t SoundWDSEntry;

typedef struct {
    /* 0x0 */ int currentValue; // Current interpolated value
    /* 0x4 */ int stepIncrement; // Amount to add each step
    /* 0x8 */ short counter; // Steps remaining
    /* 0xA */ short targetValue; // Final value when counter reaches 0
} AudioInterpolator;

// AudioElement.active_flags
#define SOUND_CHANNEL_PERCUSSION_ACTIVE 0x10
#define SOUND_CHANNEL_NOTE_ACTIVE 0x80
#define SOUND_CHANNEL_HOLD_NOTE 0x100
#define SOUND_CHANNEL_REST_NOTE 0x400
#define SOUND_CHANNEL_CHANGE_INSTRUMENT 0x8000

// AudioElement.status_flags
#define SOUND_STATUS_REST_NOTE 0x2
#define SOUND_STATUS_VOLUME_CHANGE 0x100
#define SOUND_STATUS_PLAYING_NTOE 0x200

// Saved loop position for the script loop-start/loop-end handlers.
typedef struct {
    /* 0x0 */ u8 count;
    /* 0x1 */ u8 unk1;
    /* 0x2 */ u8 savedOctave;
    /* 0x3 */ u8 endOctave;
    /* 0x4 */ u8* start;
    /* 0x8 */ u8* end;
} AudioLoop; // Size: 0xC

#define NUM_SOUND_LOOPS 4

// Per-voice modulation state: a stepped value driven by one of the step handlers.
typedef struct {
    /* 0x0  */ undefined32 pStepHandler; // g_SoundEnvelopeStepHandlers entry, called with this envelope
    /* 0x4  */ s32 value;
    /* 0x8  */ undefined32 stepDelta; // signed delta the step handlers add to value
    /* 0xC  */ s32 delta;
    /* 0x10 */ u16 count;
    /* 0x12 */ u16 reload;
    /* 0x14 */ u16 delayCount;
    /* 0x16 */ u16 delayReload;
    /* 0x18 */ u16 rampLevel;
    /* 0x1A */ u16 rampStep;
    /* 0x1C */ u8 target; // 0 = pitch, 1 = volume, 2 = pan
    /* 0x1D */ u8 handlerIndex;
    /* 0x1E */ u16 flags;
} AudioEnvelope; // Size: 0x20

#define NUM_VOICE_ENVELOPES 4

// Per-voice linear ramp state: current step, frames remaining, final target.
typedef struct {
    /* 0x0 */ s32 step;
    /* 0x4 */ u16 counter;
    /* 0x6 */ u16 target;
} AudioElementRamp; // Size: 0x8

typedef struct {
    /* 0x0   */ u16 active_flag;
    /* 0x2   */ u16 status_flags;
    /* 0x4   */ u16 rampFlags; // one bit per running ramp
    /* 0x6   */ s8 elementIndex;
    /* 0x7   */ s8 unk_0x07;
    /* 0x8   */ s32 soundId;
    /* 0xC   */ u32 startTick; // g_SoundTimerTickCounter at element start; voice allocation picks the lowest
    /* 0x10  */ u32 scriptStartIP; // pScriptIP's initial value, never updated after the element starts
    /* 0x14  */ u32 pScriptIP; // next script byte to run
    /* 0x18  */ u8* savedScriptIP;
    /* 0x1C  */ s32 unk_0x1C;
    /* 0x20  */ u16 loopCount;
    /* 0x22  */ u8 unk_0x22;
    /* 0x23  */ u8 savedOctave;
    /* 0x24  */ u8 unk_0x24;
    /* 0x25  */ u8 sndId;
    /* 0x26  */ u8 toneIndex;
    /* 0x27  */ u8 voice_number;
    /* 0x28  */ u8 releaseRate;
    /* 0x29  */ s8 unk29;
    /* 0x2A  */ s16 unk2A;
    /* 0x2C  */ u32 pWds; // SoundWDSEntry*
    /* 0x30  */ SoundVoiceData voice_data;
    /* 0x5C  */ s16 fermataDuration;
    /* 0x5E  */ u16 restTicks; // ticks left before the note rests, counted down with fermataDuration
    /* 0x60  */ s8 durationBias; // added to fermataDuration to give the note's length in ticks
    /* 0x61  */ u8 unk_0x61;
    /* 0x62  */ u16 gateRatio; // gate length as n/16 of the note: 0x10 = whole note, 0xF = one tick short
    /* 0x64  */ u8 currentNote; // note number the pitch in note is built from
    /* 0x65  */ u8 nextNote; // note number parsed from the script, adopted into currentNote
    /* 0x66  */ u16 octave;
    /* 0x68  */ u32 note; // 16.16, ramped by noteStep
    /* 0x6C  */ s16 toneTune;
    /* 0x6E  */ s16 detune;
    /* 0x70  */ u16 slideFrames;
    /* 0x72  */ u16 loopIndex;
    /* 0x74  */ u16 pan;
    /* 0x76  */ s16 volume;
    /* 0x78  */ u32 elementVolume; // 16.16
    /* 0x7C  */ AudioElementRamp ramp;
    /* 0x84  */ s32 noteStep;
    /* 0x88  */ s32 elementVolumeStep;
    /* 0x8C  */ u16 volumeStep;
    /* 0x8E  */ u16 volumeTarget;
    /* 0x90  */ u16 panStep;
    /* 0x92  */ u16 panTarget;
    /* 0x94  */ u16 noteFrames;
    /* 0x96  */ u16 elementVolumeFrames;
    /* 0x98  */ u16 panFrames;
    /* 0x9A  */ u16 volumeFrames;
    /* 0x9C  */ AudioLoop loops[NUM_SOUND_LOOPS];
    /* 0xCC  */ u16 envelopeIndex;
    /* 0xCE  */ u16 activeEnvelopeMask;
    /* 0xD0  */ s16 noteOffset; // summed from the envelopes into the note
    /* 0xD2  */ s16 volumeOffset;
    /* 0xD4  */ s16 panOffset;
    /* 0xD6  */ u16 pad0xD6;
    /* 0xD8  */ AudioEnvelope envelopes[NUM_VOICE_ENVELOPES];
} AudioElement;

typedef struct AudioManager {
    /* 0x0  */ struct AudioManager* next;
    /* 0x4  */ struct AudioManager* pBackup;
    /* 0x8  */ s32 pSeqHeader; // sequence file image
    /* 0xC  */ s32 pPercussionData;
    /* 0x10 */ s16 flags;
    /* 0x12 */ s16 unk_0x12;
    /* 0x14 */ u8 elementCount;
    /* 0x15 */ u8 percussionCount;
    /* 0x16 */ s16 sndId;
    /* 0x18 */ s16 unk_0x18;
    /* 0x1A */ u8 unk_0x1a;
    /* 0x1B */ u8 unk_0x1b;
    /* 0x1C */ u16 noiseClock;
    /* 0x1E */ u16 stopBar; // barCount this sequence stops at
    /* 0x20 */ s32 unk_0x20;
    /* 0x24 */ s32 playTicks; // ticks driven while elements are active, the tick part of the play time
    /* 0x28 */ s32 playTime; // 1/256 s, seconds/minutes come out of it
    /* 0x2C */ s32 restoreTicks; // playTicks at which the saved backup is restored, 0 = not armed
    /* 0x30 */ s16 lowestLoopCount;
    /* 0x32 */ s16 barCount;
    /* 0x34 */ s16 beatCount;
    /* 0x36 */ u16 tickCount;
    /* 0x38 */ s16 beatsPerBar;
    /* 0x3A */ s16 ticksPerBeat;
    /* 0x3C */ s16 beatDivisor;
    /* 0x3E */ s16 unk_0x3e;
    /* 0x40 */ u8 unk_0x40;
    /* 0x41 */ u8 reverbType;
    /* 0x42 */ u8 reverbDelay;
    /* 0x43 */ u8 reverbFeedback;
    /* 0x44 */ s16 reverbDepth;
    /* 0x46 */ u8 unk_0x46[0x02];
    /* 0x48 */ s32 activeElementMask;
    /* 0x4C */ s32 muteFlags;
    /* 0x50 */ s32 tickAccumulator;
    /* 0x54 */ s32 tickRate;
    /* 0x58 */ s32 tempo;
    /* 0x5C */ s32 tempoStep;
    /* 0x60 */ u16 tempoFrames;
    /* 0x62 */ u16 tempoTarget;
    /* 0x64 */ AudioInterpolator tempoScaleInterpolator; // scales tempo into tickRate
    /* 0x70 */ AudioInterpolator volumeInterpolator;
    /* 0x7C */ AudioInterpolator noteInterpolator;
    /* 0x88 */ AudioInterpolator panInterpolator;
    /* 0x94 */ AudioElement elements[24];
} AudioManager;

typedef struct {
    /* 0x00 */ SpuCommonAttr commonAttr;

    // Volume state management
    /* 0x28 */ s16 currentMasterVolume;
    /* 0x2A */ s16 currentCdVolume;
    /* 0x2C */ s16 currentReverbDepth;
    /* 0x2E */ s16 cdMixVolume;

    /* 0x30 */ AudioInterpolator masterInterpolator;
    /* 0x3C */ AudioInterpolator cdInterpolator;
} SoundVolumeController;

extern SoundVolumeController g_SoundVolumeController;

extern s32 g_ReverbWorkAreaSizes[SPU_REV_MODE_MAX];
extern u8 g_SoundReverbType;
extern u8 g_SoundReverbDelay;
extern u8 g_SoundReverbFeedback;
extern s32 g_SoundReverbMemoryHandle;

extern s32 g_SoundEffectElementPairCount;
extern s32 g_SoundRandomSeed;
extern s32 g_SoundSoundEffectChannelCount;
extern AudioManager* g_SoundEffectAudioManager;
extern s32 g_SoundEffectElementCount;

// Heap
extern SoundHeapBlockHeader* g_SoundHeapHead;
extern u32 g_SoundHeapEnd;
extern u32 g_SoundHeapSize;

// SPU Transfer Command Queue
extern s16 g_SoundDecodedReadResult;
extern SoundTransferCommand* g_SoundTransferQueue;
extern u16 g_SoundTransferQueueReadIndex;
extern u16 g_SoundTransferQueueWriteIndex;

// SPU Memory Management
extern SoundSpuMemoryBlock g_SoundSpuMemoryBlocks[MAX_SPU_MEMORY_BLOCKS];

extern s32 g_SoundUploadDestBuffer;
extern s32 g_SoundUploadSourceAddr;
extern s32 g_SoundUploadBytesRemaining;
extern u_long g_unk_SoundEvent; // Event Descriptor

extern SoundFile* g_SoundSedsLinkedList;
extern s32 g_SoundEnvelopeStepHandlers[]; // AudioEnvelope step handler table
extern SoundWDSEntry* g_SoundWdsLinkedList;

extern int g_SoundWdsCurSpuAddress;
extern int g_SoundWdsRemainingBytes;

extern short g_SoundSpuErrorId;
extern long g_unk_VoicesNeedingProcessing;
extern AudioManager* g_SoundAudioManagerListHead;
extern short g_SoundControlFlags;
extern SpuIRQCallbackProc g_SoundSpuIrqCallbackFn;
extern int g_SoundSpuIRQCount;

extern u32 g_SoundKeyOnFlags;
extern u32 g_SoundKeyOffFlags;
extern SoundVoiceData* g_SoundChannels[24];

extern SpuVolume g_SoundReverbDepth;

extern CdlATV g_SoundCdRomAttenuation;

extern s32 SoundCalculateAudioManagerSize(s32 elementCount);
extern void SoundSetVolumeWithPhase(s32, SpuVolume*, s32);

#endif