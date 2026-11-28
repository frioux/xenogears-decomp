#ifndef _XENO_FIELD_ACTOR_H
#define _XENO_FIELD_ACTOR_H

// ActorData::soundState once the handler released the actor's sound slot: no proximity sound
#define FIELD_ACTOR_SOUND_STATE_NONE 0xFF

// ActorData::scriptFlags bits (FieldScriptSetPlayerActor, FieldScriptSetActorMovementFlag)
#define FIELD_SCRIPT_FLAG_PLAYER_ACTOR 0x4000
#define FIELD_SCRIPT_FLAG_ROTATION_LOCKED 0x8000

// Set while the player is being walked to another map; cleared again when the walk did not
// get the player there (FieldPlayerSetMapTransitionFlag, FieldScriptVMHandlerMapTransition)
#define FIELD_SCRIPT_FLAG_MAP_TRANSITION 0x80

#include "psyq/libgte.h"
#include "system/model.h"

#define ACTOR_ID_INVALID 0xFF

#define ACTOR_STATUS_INVISIBLE 0x20

#define NUM_8DIR_MOVEMENT_DIRECTIONS 0x8
#define MASK_8DIR_MOVEMENT_NUM_DIRECTIONS 0x7

#define SCRIPT_MAX_STACK_SIZE 0x4

#define SCRIPT_STATE_IDLE 0x0

#define ACTOR_SCRIPT_EXISTS -1

#define ACTOR_MAX_NUM_SCRIPTS 0x8


#define ACTOR_TYPE_UNKNOWN 0x0
#define ACTOR_TYPE_PEOPLE 0x1
#define ACTOR_TYPE_ROBOT 0x2
#define ACTOR_TYPE_PLAYER 0x3


typedef struct {
    u_short currentIP; // Instruction Pointer
    u_char waitTimer; // Used for sleep() logic in scripts
    u_char scriptId;
    u_int flags_0: 16;
    u_int state: 2; // Execution state
    u_int flags_0x12: 4;
    u_int isInUse: 1;
    u_int flags_0x17: 2;
    u_int flags_0x19: 7;
} ActorScriptSlot;

typedef struct {
    short x, y;
} SVEC2;

typedef struct {
    short x, y, z;
} SVEC;

typedef struct {
    int x;
    int y;
    int z;
} IVEC3;

// The file format containing sprite animations for an actor
typedef struct {
    /* 0x0 */ u32 field_0x0;
    /* 0x4 */ u32 animationsOffset;
    /* 0x8 */ u32 framesOffset;
    /* 0xC */ u32 paletteOffset;
    // ...
} SpriteAnimPackageFileHeader;

typedef struct {
    /* 0x0 */ u16 flags; // Mask 0x3F = num animations
    /* 0x2 */ //u16 animationOffsets[num animations];
    /* 0xN */ // AnimationData[]
} SpriteAnimPackageFileAnimationsData;

typedef struct {
    /* 0x0 */ u16 oneDirectional: 1;
    /* 0x0 */ u16 fourDirectional: 1;
    /* 0x0 */ u16 eightDirectional: 1;
    /* 0x0 */ u16 flags: 3;
    /* 0x0 */ u16 numAnimations: 6;
    /* 0x0 */ u16 flags_2: 4;
    /* 0x2 */ u16 scriptOffset;
    /* 0x4 */ u16 unkOffset;
} SpriteAnimPackageFileAnimation;

// An animation package which has been parsed and possibly loaded in memory
typedef struct {
    /* 0x0  */ void* pFrames;
    /* 0x4  */ SVEC2 tex;
    /* 0x8  */ SVEC2 clut;
    /* 0xC  */ void* pPalette;
    /* 0x10 */ void* pAnimations;
} SpriteAnimPackage;

typedef struct {
    /* 0x0 */ u8 translationX;
    /* 0x1 */ u8 translationY;
    /* 0x2 */ s16 rotationX;
    /* 0x4 */ s16 rotationY;
    /* 0x6 */ s16 rotationZ;
} SpriteDirectionTransform;

typedef struct {
    /* 0x0 */ SpriteDirectionTransform dirTransforms[8];
} SpriteDirectionTransforms;

// Three angles with no pad byte: the script VM keeps a u16 right after it (0x10A).
typedef struct {
    short vx, vy, vz;
} ActorRotation;

typedef struct {
    /* 0x0  */ SVEC rotation;
    /* 0x6  */ SVEC scale;
    /* 0xC  */ MATRIX transformMatrix;
    /* 0x2C */ void* pFramesData;
    /* 0x30 */ void* pCurRenderFramesData;
    /* 0x34 */ SpriteDirectionTransforms* pDirTransforms;
    /* 0x38 */ void* pNextSpriteData;
    /* 0x3C */ u8 offsetX;
    /* 0x3D */ u8 offsetY;
    /* 0x3E */ s16 field_0x3E;
} SpriteDataI1;

typedef struct {
    /* 0x0  */ s32 field_0x0;
    /* 0x4  */ s32 field_0x4;
    /* 0x8  */ s32 field_0x8;
    /* 0xC  */ s16 field_0xC;
    /* 0xE  */ s16 texX;
    /* 0x10 */ s16 texY;
    /* 0x12 */ s16 field_0x12;
    /* 0x14 */ s16 entityId;
    /* 0x16 */ s16 field_0x16;
    /* 0x18 */ void* field_0x18;
} SpriteDataI3;

typedef struct {
    /* 0x0  */ s16 tileX;
    /* 0x2  */ s16 tileY;
    /* 0x4  */ u8 texX;
    /* 0x5  */ u8 texY;
    /* 0x6  */ u8 width;
    /* 0x7  */ u8 height;
    /* 0x8  */ u8 deltaWidth;
    /* 0x9  */ u8 deltaHeight;
    /* 0xA  */ s16 texPage;
    /* 0xC  */ s16 clutID;
    /* 0xE  */ s16 field_0xE;
    /* 0x10 */ u32 colorAndCode;
    /* 0x14 */ s32 flags;
} SpriteFrameData;

// Flags (Field 0xB0):
// 0x400: Current animation is using non-default animation package

// SpriteData's two animation flag words are read and written whole by most
// callers, but a few set a single byte of one: the animation id is byte 3 of
// flags_0xAC and the animation flag byte is byte 0 of flags_0xB0.
typedef union {
    /* 0x0 */ s32 flags;
    /* 0x0 */ u8 bytes[4];
} SpriteFlagWord;

typedef struct {
    /* 0x0  */ IVEC3 position;
    /* 0xC  */ IVEC3 step;
    /* 0x18 */ int moveSpeed;
    /* 0x1C */ u32 gravity;
    /* 0x20 */ SpriteDataI1* pBase;
    /* 0x24 */ void* pVramData;
    /* 0x28 */ u8 red;
    /* 0x29 */ u8 green;
    /* 0x2A */ u8 blue;
    /* 0x2B */ u8 prim;
    /* 0x2C */ s16 scale;
    /* 0x2E */ s16 field_0x2e;
    /* 0x30 */ s16 field_0x30;
    /* 0x32 */ s16 direction; // Angle
    /* 0x34 */ s16 curSpriteFrame;
    /* 0x36 */ s16 field_0x36;
    /* 0x38 */ s16 field_0x38;
    /* 0x3A */ u16 field_0x3A;
    /* 0x3C */ u32 field_0x3C_0: 20;
    /* 0x3C */ u32 field_0x3C_1: 4;
    /* 0x3C */ u32 field_0x3C_2: 4;
    /* 0x3C */ u32 field_0x3C_6: 1; // Needs to recompute transform matrix?
    /* 0x3C */ u32 field_0x3C_5: 1;
    /* 0x3C */ u32 field_0x3C_3: 1;
    /* 0x3C */ u32 field_0x3C_4: 1;
    /* 0x40 */ u32 field_0x40;
    /* 0x44 */ void* pCurAnimFile;
    /* 0x48 */ void* pDefaultAnimFile;
    /* 0x4C */ void* pSpecialAnimFile; // used when animation id is negative
    /* 0x50 */ u32 field_0x50;
    /* 0x54 */ u32 field_0x54;
    /* 0x58 */ void* pCurAnimation; // Points to animation entry in SpriteAnimPackageAnimationsData
    /* 0x5C */ u32 field_0x5C;
    /* 0x60 */ u32 field_0x60;
    /* 0x64 */ void* pSpriteBytecode; // IP
    /* 0x68 */ void* field_0x68; // Callback
    /* 0x6C */ void* field_0x6C; // Pointer to self?
    /* 0x70 */ void* field_0x70;
    /* 0x74 */ void* pTargetEntitySprite;
    /* 0x78 */ u32 field_0x78;
    /* 0x7C */ void* field_0x7C;
    /* 0x80 */ s16 field_0x80;
    /* 0x82 */ s16 field_0x82;
    /* 0x84 */ s16 field_0x84;
    /* 0x86 */ s16 allocatedDataSize;
    /* 0x88 */ void* field_0x88;
    /* 0x8C */ u8 stackIndex;
    /* 0x8D */ u8 field_0x8D;
    /* 0x8E */ u8 stack[16];
    /* 0x9E */ s16 animScriptWaitTimer;
    /* 0xA0 */ s16 field_0xA0;
    /* 0xA2 */ s16 field_0xA2;
    /* 0xA4 */ s16 field_0xA4;
    /* 0xA6 */ s16 field_0xA6;
    /* 0xA8 */ s32 flags_0xA8;
    /* 0xAC */ SpriteFlagWord flags_0xAC;
    /* 0xB0 */ SpriteFlagWord flags_0xB0;
    /* 0xB4 */ SpriteDataI1 inner1;
    /* 0xF4 */ SpriteDataI3 inner3;
    /* 0x110 */ SpriteAnimPackage i2CurPackage;
    /* 0x124 */ SpriteDirectionTransforms directionTransforms;
} SpriteData;

// TODO: Fix the horrible naming of these bitfields.
typedef union {
    u_int flags;
    struct {
        u_int scriptFlags_0x0: 1; // isDisabled?
        u_int scriptFlags_0x1: 7;
        u_int scriptFlags_0x8: 1;
        u_int type: 7;
        u_int scriptFlags_0xX: 1;
        u_int scriptFlags_0xA: 1; // isDialogActivationDisabled?
        u_int scriptFlags_0x10: 1;
        u_int scriptFlags_0x11: 1;
        u_int scriptFlags_0x13: 1;
        u_int scriptFlags_0x14: 1;
        u_int scriptFlags_0x15: 1;
        u_int scriptFlags_0x16: 1;
        u_int scriptFlags_0x17: 1;
        u_int scriptFlags_0x18: 1;
        u_int scriptFlags_0x19: 6;
    } fields;
} ActorScriptFlags;

// Snapshot of the script arguments, allocated on the heap behind
// ActorData::pScriptArgs by the indirect-argument handlers.
typedef struct {
    /* 0x0 */ u16 arg1;
    /* 0x2 */ u16 arg2;
    /* 0x4 */ u16 arg3;
    /* 0x6 */ u16 arg4;
    /* 0x8 */ u16 arg5;
    /* 0xA */ u16 arg6;
    /* 0xC */ u16 arg7;
    /* 0xE */ u16 arg8;
} FieldScriptArgs; // Size: 0x10

typedef struct {
    /* 0x0 */ ActorScriptFlags scriptFlags;

    // MFlag2
    u_int flags;

    /* 0x8  */ short walkmeshTriIds[4]; // Pri 0 - 3
    /* 0x10 */ short walkmeshId; // pol
    /* 0x12 */ short pad_0x12;
    /* 0x14 */ u_int curWalkmeshTriMaterial; // ID
        // 0x20: P
        // 0x40: G
        // 0x80: C
    /* 0x18 */ u_short width; //xWidth
    /* 0x1A */ u_short height;
    /* 0x1C */ u_short zWidth;
    /* 0x1E */ u_short solidRange;
    /* 0x20 */ VECTOR position;
    /* 0x30 */ VECTOR moveModified; // Step / delta?
    /* 0x40 */ VECTOR move;
    /* 0x50 */ VECTOR curTriNormal; // Surface normal
    /* 0x60 */ SVECTOR positionOffset; // Added to position to build a target point
    /* 0x68 */ SVEC prevPosition;
    /* 0x6E  */ u_short stuckFrameCount; // Frames run without moving; gives up at 0x41
    /* 0x70  */ u_short hideModeParam; // Argument of the set-hide-mode script instruction
    /* 0x72  */ u_short curYPos;
    /* 0x74  */ u_char canInteract;
    /* 0x75  */ u_char parentActorId;
    /* 0x76  */ u_short moveSpeed;
    /* 0x78  */ short scriptPointersStack[SCRIPT_MAX_STACK_SIZE];
    /* 0x80  */ u_char faceId;
    /* 0x81  */ u_char dialogCursorPosition; // Column chosen in the text box, 0xFF = none
    /* 0x82  */ u_char dialogWidth;
    /* 0x83  */ u_char dialogHeight;
    /* 0x84  */ u_int dialogFlags;
    /* 0x88  */ short dialogPixelWidth;
    /* 0x8A  */ short dialogPixelHeight;
    /* 0x8C  */ ActorScriptSlot scripts[ACTOR_MAX_NUM_SCRIPTS];
    /* 0xCC  */ u_short scriptInstructionPointer;
    /* 0xCE  */ u_char curScriptIndex;
    /* 0xCF  */ u_char waitScriptIndex; // Slot of the script this actor started and waits for
    /* 0xD0  */ VECTOR moveStep;
    /* 0xE0  */ short moveFrameTotal;
    /* 0xE2  */ u_char curDoorStep;
    /* 0xE3  */ u_char hitTimer; // Hit reaction ramp: +2 per frame while pushed, -1 per frame after
    /* 0xE4  */ short characterId;
    /* 0xE6  */ short defaultAnimationId;
    /* 0xE8  */ short curAnimationId;
    /* 0xEA  */ short animationOverride; // 0xFF = none; replaces the computed animation id
    /* 0xEC  */ short scriptYPos; // Y the script set; overrides the walkmesh height while scriptFlags_0x10 is set
    /* 0xEE  */ short spriteClipY; // Y (in the frame's own space) the sprite's frames are cut off at
    /* 0xF0  */ int fallSpeed; // 16.16, accumulated by gravity and clamped to the material's limit
    /* 0xF4  */ u_short scaleX;
    /* 0xF6  */ u_short scaleY;
    /* 0xF8  */ u_short scaleZ;
    /* 0xFA  */ short unkFA;
    /* 0xFC  */ u_char red;
    /* 0xFD  */ u_char green;
    /* 0xFE  */ u_char blue;
    /* 0xFF  */ u_char red2;
    /* 0x100 */ u_char green2;
    /* 0x101 */ u_char blue2;
    /* 0x102 */ short frameCounter;
    /* 0x104 */ ActorRotation rotation; // 0xFFF mask: rotation
    /* 0x10A */ u_short soundId;
    /* 0x10C */ u_char soundVolume;
    /* 0x10D */ u_char soundState; // FIELD_ACTOR_SOUND_STATE_NONE = no proximity sound for this actor
    /* 0x110 */ void* pTargetTrack; // FieldActorTargetTrack, 0xC bytes on the heap
    /* 0x114 */ FieldScriptArgs* pScriptArgs;
    /* 0x118 */ void* pScriptWorkWords; // One u_int work word per model script
    /* 0x11C */ short lockedRotationY; // rotation.vy frozen when movement is disabled
    /* 0x11E */ short updateAngle; // Per-frame angle step of FieldMathUpdateAngle
    /* 0x120 */ void* pSpecialAnimFile;
    /* 0x124 */ short specialAnimArchiveIndex; // -1 when no special anim file is loaded
    /* 0x126 */ u_char spriteSource; // Bit 0x80: index into g_FieldSpriteData, else a party index into g_PartyDataBuffers
    /* 0x127 */ u_char spriteId;
    /* 0x128 */ short modelAnimation;
    /* 0x12A */ u_short unk12A;


    // 0x1C0 => Stack index, bit 0x100 is not used since there's only four elements in the stack
    u_int flags12C_0: 2;
    u_int flags12C_0x2: 3; // Tentative
    u_int flags12C_0x5: 1;
    u_int flags12C_0x6: 3; // Keeps track of recursion level in scripts - inc on call, dec on ret
    /* flags12C_0x9 */ u_int direction: 3;
    u_int flags12C_0xD: 1;
    u_int flags12C_0xE: 3;
    u_int flags12C_0x10: 2;
    u_int flags12C_0x12: 10; // Model type << 6
    u_int flags12C_0x1C: 4;


    int flags130;
    int flags134;
} ActorData;

typedef struct {
    /* 0x0  */ void* pModelData; // 0x24 size, model related data
    /* 0x4  */ void* pSpriteData; // 0x164 size
    /* 0x8  */ void* pShadow; // 0x70 size
    /* 0xC  */ MATRIX transformMatrix;
    /* 0x2C */ MATRIX childMatrix;
    /* 0x4C */ ActorData* pActorData;
    /* 0x50 */ SVEC rotation;
    /* 0x56 */ short flags;
    /* 0x58 */ short status;
    /* 0x5A */ u_short spriteFlags; // Bit 0: a sprite model is attached and must be destroyed first
} FieldActor;

// Model space bounding box the actor's model data keeps a pointer to; the same
// block holds the model's render command list below offset 0x20
typedef struct {
    /* 0x00 */ undefined8 unk0[0x6];
    /* 0x06 */ u16 numCommands;
    /* 0x08 */ undefined8 unk8[0x8];
    /* 0x10 */ u32* pCommands; // each word is (primitiveCount << 16 | opcode), then 2 words per primitive
    /* 0x14 */ undefined8 unk14[0xC];
    /* 0x20 */ SVECTOR min;
    /* 0x28 */ SVECTOR max;
} FieldActorModelBounds; // Size: 0x30

// One entry of FieldActorModelScriptTable: the callback plus the value it ramps.
// ModelStepRampValue moves value toward target by step, clamped to target.
typedef struct {
    /* 0x0  */ u_int (*pCallback)(void);
    /* 0x4  */ s32 target;
    /* 0x8  */ s32 value;
    /* 0xC  */ s32 step;
    /* 0x10 */ undefined8 unk10[0x10];
} FieldActorModelScript; // Size: 0x20

// Built by func_800303C8, which saves the part's arrays before repacking its
// vertices and normals; the field side reads pCallback's return value through
// ActorData::pScriptWorkWords (FieldPopActorScriptWorkWord).
typedef struct {
    /* 0x0  */ ModelPart* pModelPart; // the part the table was built from
    /* 0x4  */ SVECTOR* pVertices; // the part's vertex array, repacked
    /* 0x8  */ SVECTOR* pNormals; // the part's normal array, repacked
    /* 0xC  */ s32 numScripts;
    /* 0x10 */ FieldActorModelScript* pScripts;
} FieldActorModelScriptTable;

// FieldActor::pModelData
typedef struct {
    /* 0x00 */ undefined8 unk0[0x4];
    /* 0x04 */ FieldActorModelBounds* pBounds;
    /* 0x08 */ u8* pPrimitiveBuffers[2]; // one built-primitive buffer per g_FieldCurRenderContextIndex
    /* 0x10 */ undefined8 unk10[2];
    /* 0x12 */ s16 renderMode; // Passed to ModelRenderPacket as its 4th argument
    /* 0x14 */ FieldActorModelScriptTable* pScripts;
    /* 0x18 */ s16 centerX;
    /* 0x1A */ s16 centerY;
    /* 0x1C */ s16 centerZ;
    /* 0x1E */ s16 pad1E; // centerX is read as one SVECTOR, so this is its pad
    /* 0x20 */ s16 size;
    /* 0x22 */ undefined16 unk22;
} FieldActorModelData; // Size: 0x24

typedef struct {
    short L11;
    short L12;
    short L13;
    short LR1;
    short LR2;
    short LR3;
    short L21;
    short L22;
    short L23;
    short LG1;
    short LG2;
    short LG3;
    short L31;
    short L32;
    short L33;
    short LB1;
    short LB2;
    short LB3;
    short RBK;
    short GBK;
    short BBK;
} FieldLightFileData;

typedef struct {
    u_short flags;
    u_short rotationX;
    u_short rotationY;
    u_short rotationZ;
    u_short positionX;
    u_short positionY;
    u_short positionZ;
    u_short modelID;
} FieldActorFileData; // Size: 0x10

typedef struct {

    // Decompressed sizes
    /* 0x10C */ u_int timPackageSize;
    /* 0x110 */ u_int walkmeshSize;
    /* 0x114 */ u_int modelDataSize;
    /* 0x118 */ u_int spriteDataSize;
    /* 0x11C */ u_int clutDataSize;
    /* 0x120 */ u_int scriptsSize;
    /* 0x124 */ u_int unk3Size;
    /* 0x128 */ u_int dialogsSize;
    /* 0x12C */ u_int triggersSize;

    // Offsets to compressed data
    /* 0x130 */ u_int timPackageOffset;
    /* 0x134 */ u_int walkmeshDataOffset;
    /* 0x138 */ u_int modelDataOffset;
    /* 0x13C */ u_int spriteDataOffset;
    /* 0x140 */ u_int clutDataOffset;
    /* 0x144 */ u_int scriptsOffset;
    /* 0x148 */ u_int pUnk3Data;
    /* 0x14C */ u_int dialogsOffset;
    /* 0x150 */ u_int triggersOffset;
    /* 0x154 */ FieldLightFileData lightData;
    /* 0x17E */ u_char pad17E[0xE];
    /* 0x18C */ u_short numEntitites;
    /* 0x18E */ undefined16 pad18E;
    /* 0x190 */ u_short actorData[0]; // Start of entity data, FieldActorFileData sized
} ActorFile;


#define SCRIPT_OFFSET_TABLE_SIZE 0x40

typedef struct {
    u_int signBits[0x20]; // Sign bits for variables
    /* 0x80 */ u_int numScripts;
    /* 0x84 */ void* metadata; // numScripts * 0x40 size of script metadata
    // script instructions
} ScriptsFile;

extern int g_FieldNumActors;
extern FieldActor* volatile g_FieldActors;

extern ActorData* g_FieldScriptVMCurActor;
extern void* g_FieldScriptVMCurScriptData;
extern int g_FieldScriptMaxInstructionCount;
extern void* g_FieldScriptMemory;
extern ScriptsFile* g_FieldCurScriptFile;
extern int g_FieldScriptVMStopFlag; // Stop script VM exection?

extern void* g_FieldSpriteData;

#define SCRIPT_READ_U8(idx) *(u8*)&g_FieldScriptVMCurScriptData[idx]
#define SCRIPT_READ_U8_REL(idx) *(u8*)&g_FieldScriptVMCurScriptData[g_FieldScriptVMCurActor->scriptInstructionPointer + idx]

#define ARG(index) ((index * 2) - 1)
#define SCRIPT_IMM_ARG(index) (FieldScriptVMGetInstructionArgument(ARG(index)) & 0xFFFF)
#define SCRIPT_IMM_ARG_ALIGNED(index) (FieldScriptVMGetInstructionArgument(index*2) & 0xFFFF)

#endif
