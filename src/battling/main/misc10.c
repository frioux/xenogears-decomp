#include "common.h"
#include "psyq/libgpu.h"
#include "system/memory.h"
#include "system/archive.h"

// The pixel depth of the TIM a battle quad samples: GetTPage's tp argument, which
// also says how many texels one halfword of the image holds.
#define BATTLE_TEX_DEPTH_4BIT 0
#define BATTLE_TEX_DEPTH_8BIT 1
#define BATTLE_TEX_DEPTH_16BIT 2

// g_BattleControlFlags bit 0: set while the player, not the COM, drives both sides.
#define BATTLE_CONTROL_FLAG_USER 0x1

// The size HeapAlloc gives the 128x128 battle grid the archive decompresses into,
// and the ordering table BattleCreateOrderingTablePair splits in two.
#define BATTLE_GRID_CELLS_SIZE 0x10010
#define BATTLE_ORDERING_TABLE_SIZE 0x1000

// Frames of no button input before the battle hands both sides to the COM, and the
// number of frames C1 may be held on the default text list to skip that wait.
#define BATTLE_IDLE_TIMEOUT 0x4650
#define BATTLE_IDLE_HOLD_COUNT 0x78

// The archive directory an actor's entry is loaded from below, the value
// src/battling/main/misc12.c's BattleInitGraphics selects too;
// include/system/archive.h's ARCHIVE_DIR_* list stops at 0x2C.

// Quarter and three-quarter turn of the 12-bit angle rcos/rsin work in, the space
// src/battling/main/misc4.c and misc3.c name (BATTLE_ANGLE_QUARTER_TURN,
// BATTLE_ANGLE_HALF_TURN).
#define BATTLE_ANGLE_QUARTER_TURN 0x400
#define BATTLE_ANGLE_THREE_QUARTER_TURN 0xC00

// The fill lengths, in bar pixels, at which the gauge bar's first segment
// (barPrim[0]) and its tapered second segment (barPrim[1] at pBase+0x18) are full.
#define BATTLE_GAUGE_BAR_SEGMENT1_END 56
#define BATTLE_GAUGE_BAR_SEGMENT2_END 72

// How far, in bar pixels, the far x of a filled segment sits behind the fill
// value: the first segment's by 4 (the second segment's far x starts from the
// same offset), the third segment's (barPrim[2] at pBase+0x30) by 8.
#define BATTLE_GAUGE_BAR_SEGMENT1_TAPER 4
#define BATTLE_GAUGE_BAR_SEGMENT3_TAPER 8

// The two colour words BattleSetGaugeBarPrimColor stamps into the side meter
// quad: POLY_F4's primitive code 0x28 in the top byte, then the RGB (white, or
// red with only its first channel set).
#define BATTLE_GAUGE_METER_COLOR_WHITE 0x2800FFFF
#define BATTLE_GAUGE_METER_COLOR_RED 0x280000FF

// .sdata/.bss of the overlay (supplied by splat's asm segments)
extern u8 g_BattleDrawBufferIndex;
extern s16 g_BattleEffectFrameTimer;
extern s32 g_BattleScreenMode;
extern s32 g_BattleMode;
extern u8 g_BattleTransitionFlag;
extern u8* g_BattleSlotEntryBuffers[];
extern s32 g_BattleSedsEntryId;
extern s32 g_BattleAudioManagerEnabled;
extern s32 g_BattlingAudioManager;
extern u8 g_BattleSlot1EntryId;
extern u8 g_BattleSlot0EntryId;
extern u8 g_BattleRotatedPartModel[];
extern u8 g_BattleControlFlags;
extern u8 g_BattlePlayer1ComControlled;
extern u8 g_BattlePlayer2ComControlled;
extern s32 g_SpuChannelMask;
extern s32 g_BattleMotionSpeedScale;
extern u8* g_BattleRootOrderingTable;
extern u8* g_BattleEffectImageActive;


extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern s32 ArchiveCdDataSync(s32 mode);
extern void func_8008E120(void);
extern void func_8007F258(void* pOt, s32 arg);
extern void BattleDrawTextPrims(void* pOt);
extern void BattleDrawEffectImage(void* pOt, s32 arg1, s32 arg2);
extern void func_8008BC04(void);
extern void SoundSetAudioManagerMasterVolumeWithFade(s32 arg0, s32 arg1, s32 arg2);
extern void BattleEffectFreeResources(void);
extern void BattleClearScreen(s32 arg);
extern void BattleSetScreenMode(s32 arg);
extern void SoundRemoveSedsEntry(s32 arg);
extern void SoundStopAudioManager(s32 arg);
extern void SoundDestroyAudioManager(s32 arg);
extern void func_80088A40(void);
extern void ChangeGameState(s32 state);
extern void Vsync(s32 mode);
extern void MainLoop(s32 arg);
extern void* BattleCreateObject(void);
extern void* BattleCreateModelPart(void);
extern void BattleSetNodeModelPart(void* pA, void* pB);
extern void BattleAttachModelPart(void* pA, void* pB);
extern void BattleAppendListEntry(void* pList, void* pNode);
extern void* BattleLoadArchiveEntry(s32 arg);
extern void BattleAttachActorPrims(void* pData);
extern s32 BattleCheckActorBelowBreakThreshold(s32 arg0, s32 arg1);
void BattleAddSharedPrim(void);

// The header at the start of the block pData->pHeader points at, once
// BattleRelocateBlockPointers has fixed up its +0x30 offset. It is the block the
// actor's script pointer (+0x8FC) addresses, so the fields the units that read
// the script name carry their names: src/battling/main/misc4.c's ActorScript (the
// part indices, the two scales, the facing) and misc1.c's BattleScriptBlock (the
// render pair).
typedef struct {
    /* 0x00 */ u8 basePartIndex;
    /* 0x01 */ u8 partIndex0;
    /* 0x02 */ u8 partIndex1;
    /* 0x03 */ u8 radius;
    /* 0x04 */ s16 vertexIndex0;
    /* 0x06 */ s16 vertexIndex1;
    /* 0x08 */ s16 baseVertexIndex;
    /* 0x0A */ u8 pad0A[0x2];
    /* 0x0C */ u16 speedScale; // 8.8 speed, copied into the actor's moveSpeed/moveSpeedBase
    /* 0x0E */ u8 visiblePartCount;
    /* 0x0F */ u8 stepScale; // copied into the actor's stepScale by misc4's func_80078F00
    /* 0x10 */ u8 effectColorR;
    /* 0x11 */ u8 effectColorG;
    /* 0x12 */ u8 effectColorB;
    /* 0x13 */ u8 pad13;
    /* 0x14 */ s16 angleX;
    /* 0x16 */ s16 angleYStep;
    /* 0x18 */ s16 offsetY;
    /* 0x1A */ s16 offsetZ;
    /* 0x1C */ s16 modelColor;
    /* 0x1E */ u16 angleY;
    /* 0x20 */ u16 facing;
    /* 0x22 */ u8 pad22[0x2];
    // The actor's camera rig (misc4's func_8007A768 copies these into the
    // g_BattleRender* globals BattleUpdateCameraPosition feeds to rcos/rsin).
    /* 0x24 */ s16 cameraAngleOffset;
    /* 0x26 */ s16 cameraYOffset;
    /* 0x28 */ s16 cameraTargetYOffset;
    /* 0x2A */ s16 cameraRadius;
    /* 0x2C */ u8 pad2C[0x4];
    /* 0x30 */ s32 visiblePartIndicesOffset; // byte offset of the sprite table from the block start
} Misc10SpriteHeader; // Size: 0x34
STATIC_ASSERT_SIZEOF(Misc10SpriteHeader, 0x34);

// One 4-byte action slot of the block's +0x18 array; only the flag byte is read.
typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x1 */ u8 unk1;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 flags;
} Misc10ActionSlot; // Size: 0x4

// One 0x20-byte entry of the D_80092874 table (the same table src/battling/
// main/misc6.c calls BattleFilterEntry); +0xA holds one flag byte per action
// slot and this unit counts 14 of them. The actor's pActionRow (+0x1600) is
// &D_80092874[typeId] (retail asm, func_80084C88), so the fields misc2/misc3/
// misc18 read through their own pActionData at that same offset are this one.
typedef struct {
    /* 0x00 */ s16 scale; // the multiplier misc3's BattleUpdateSlotGaugeDisplay applies to an action's points byte
    /* 0x02 */ u8 unk02[0x2];
    /* 0x04 */ s16 requiredProgress; // the word src/battling/main/misc6.c reads as the entry's progress gate
    /* 0x06 */ u8 actionChances[0x4]; // one probability byte per action, copied out by misc18's BattleLoadWorkActionChances
    /* 0x0A */ u8 slotFlags[0xE];
    /* 0x18 */ u8 percent; // stored verbatim into the slot gauge's percent byte, and message 3 only swaps in its model while it is non-zero (misc3's BattleActionData)
    /* 0x19 */ u8 unk19[0x7];
} Misc10ActionRow; // Size: 0x20

// The relocated resource block pData points at: the layout
// BattleRelocateBlockPointers in src/battling/main/misc14.c fixes up, so each
// pointer below is a byte offset from the block start before that call. The
// three pointers at 0x00-0x08 are what that file's func_8008B38C walks to
// build the actor's nodes: the part descriptors, the model data they index and
// the per-part animation list.
typedef struct {
    /* 0x00 */ u8* pPartList; // count word followed by 0x10-byte part descriptors
    /* 0x04 */ u8* pModelData; // base of the 0x38-byte model part records
    /* 0x08 */ u8* pAnimList; // count word followed by 0x14-byte entries
    /* 0x0C */ u8* pTimImage; // TIM uploaded with the battle-wide tpage/clut (misc14's BattleRelocateBlockPointers)
    /* 0x10 */ Misc10SpriteHeader* pHeader;
    /* 0x14 */ u8* pPartTimings;
    /* 0x18 */ Misc10ActionSlot* pActions;
    /* 0x1C */ u32 pSelf; // the block's own address, the base every pointer above is rebased against
    /* 0x20 */ u8* pImage;
} Misc10Block; // Size: 0x24

// A 32-bit component vector; the actor's position blocks are this wide.
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 w;
} Vec4; // Size: 0x10
STATIC_ASSERT_SIZEOF(Vec4, 0x10);

// One of the actor's eight 0x44-byte effect slots; the layout and the names are
// src/battling/main/misc2.c's, the unit that drives its fields.
typedef struct {
    /* 0x00 */ VECTOR pos;
    /* 0x10 */ VECTOR prevPos;
    /* 0x20 */ SVECTOR velocity;
    /* 0x28 */ SVECTOR direction;
    /* 0x30 */ u8 active;
    /* 0x31 */ u8 scale;
    /* 0x32 */ u8 drawKind;
    /* 0x33 */ u8 mode;
    /* 0x34 */ u8 pad34[0x4];
    /* 0x38 */ s32 key;
    /* 0x3C */ s16 percent;
    /* 0x3E */ s16 dirWeight;
    /* 0x40 */ s16 life;
    /* 0x42 */ u8 pad42[0x2];
} BattleEffectSlot; // Size: 0x44
STATIC_ASSERT_SIZEOF(BattleEffectSlot, 0x44);

// The model chain the actor's +0x5C pointer heads. That node's +0x4 is the
// model record (src/battling/main/misc2.c reads the same field as the model),
// the record's own +0x4 leads to the part data, and the part data's +0x30 is the
// list of part nodes a created draw object joins.
typedef struct {
    /* 0x00 */ u8 unk00[0x30];
    /* 0x30 */ void* pPartList;
} Misc10PartData; // Size: 0x34

typedef struct {
    /* 0x0 */ u32 unk00;
    /* 0x4 */ Misc10PartData* pPartData;
} Misc10ModelRecord; // Size: 0x8

typedef struct {
    /* 0x0 */ u32 state; // which payload the node carries: 1 a model part, 2 an entry list, 5 a work block (misc13's Misc13Node, whose BattleSetNode* helpers set it)
    /* 0x4 */ Misc10ModelRecord* pModel;
} Misc10ModelNode; // Size: 0x8

// The per-frame display work the actor group owns (the pointer it carries at
// +0x284): one ordering-table pointer per draw buffer. src/battling/main/misc9.c
// declares the same block as Misc9DisplayWork.
typedef struct {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ u_long* pOt[2];
} Misc10DisplayWork; // Size: 0xC

// A battle participant as this unit drives it. The offsets are the full actor
// src/battling/main/misc4.c's Actor, misc2.c's Actor and misc3.c's BattleActor
// declare, and the fields below carry their names.
typedef struct {
    /* 0x000 */ Vec4 position;
    /* 0x010 */ s32 vx;
    /* 0x014 */ s32 vy;
    /* 0x018 */ s32 vz;
    /* 0x01C */ u8 pad1C[0x4];
    /* 0x020 */ s32 pushVx;
    /* 0x024 */ s32 pushVy;
    /* 0x028 */ s32 pushVz;
    /* 0x02C */ u8 pad2C[0x4];
    /* 0x030 */ s32 steerVx;
    /* 0x034 */ s32 steerVy;
    /* 0x038 */ s32 steerVz;
    /* 0x03C */ u8 pad3C[0x4];
    /* 0x040 */ s32 stepDistance;
    /* 0x044 */ s32 steerSpeed;
    /* 0x048 */ s32 distance;
    /* 0x04C */ u8 modelIndex;
    /* 0x04D */ u8 pad4D;
    /* 0x04E */ u8 prevPartIndex;
    /* 0x04F */ u8 animSpeed;
    /* 0x050 */ s16 lastFrame;
    /* 0x052 */ u8 animSpeedBonus;
    /* 0x053 */ u8 pad53;
    /* 0x054 */ s32 angle; // the actor's facing, added to the script's camera angle before rcos/rsin (misc4)
    /* 0x058 */ u8* pTimData;
    /* 0x05C */ Misc10ModelNode* pModelLink;
    /* 0x060 */ u8* pObject;
    /* 0x064 */ u8 pad64[0x8];
    /* 0x06C */ Vec4 prevPosition; // the position x,y,z,pad copied here before the frame's movement
    /* 0x07C */ u8* pPartTimings;
    /* 0x080 */ Misc10ActionSlot* pActions;
    /* 0x084 */ u8* pActionEntry; // the entry the move in progress selected (src/battling/main/misc4.c)
    // Ramped 0..0x100 deflection of one input direction each: misc3's func_80076884
    // charges the direction the input selects and decays the other three.
    /* 0x088 */ s32 inputLeft;
    /* 0x08C */ s32 inputUp;
    /* 0x090 */ s32 inputRight;
    /* 0x094 */ s32 inputDown;
    /* 0x098 */ s32 inputRampStep;
    /* 0x09C */ s32 inputDecayStep;
    /* 0x0A0 */ u8 padA0[0x8];
    /* 0x0A8 */ s32 unkA8;
    /* 0x0AC */ u8 padAC[0x4];
    /* 0x0B0 */ s32 floorY;
    /* 0x0B4 */ s16 gaugeValue;
    /* 0x0B6 */ s16 gaugePoints;
    /* 0x0B8 */ s16 prevGaugeValue;
    /* 0x0BA */ s16 gaugeLevel;
    /* 0x0BC */ s16 gaugeMax;
    /* 0x0BE */ s16 breakThresholdOffset;
    /* 0x0C0 */ u8 gaugeWidth;
    /* 0x0C1 */ u8 prevGaugeWidth;
    /* 0x0C2 */ u8 flashTimer;
    /* 0x0C3 */ u8 actionPhaseTimer;
    /* 0x0C4 */ u8 state;
    /* 0x0C5 */ u8 actionPhase;
    /* 0x0C6 */ u8 prevState;
    /* 0x0C7 */ u8 padC7;
    /* 0x0C8 */ s16 moveEndTimer;
    /* 0x0CA */ s16 moveTimer;
    /* 0x0CC */ u16 sideBaseAngle;
    /* 0x0CE */ s16 facingOffset;
    /* 0x0D0 */ u32 flags;
    /* 0x0D4 */ u32 stateFlags; // masked and tested by the movement code (src/battling/main/misc4.c)
    /* 0x0D8 */ u8* pTarget;
    /* 0x0DC */ u8 unk0DC[0x1A8];
    /* 0x284 */ Misc10DisplayWork* pDisplayWork;
    /* 0x288 */ u8 pad288[0x3BC];
    /* 0x644 */ s32 gaugePercent;
    /* 0x648 */ s32 facingAngle;
    /* 0x64C */ BattleEffectSlot effectSlots[8];
    /* 0x86C */ u8 pad86C[0x90];
    /* 0x8FC */ u8* pScript;
    /* 0x900 */ s16* pModelScriptOffsets; // one script offset per model index, the table right after the header
    /* 0x904 */ u8* pVisiblePartIndices;
    /* 0x908 */ u8 visiblePartCount;
    /* 0x909 */ u8 typeId;
    /* 0x90A */ u8 typeFlags;
    /* 0x90B */ u8 timer; // counted down by the ground-effect pass (src/battling/main/misc4.c)
    /* 0x90C */ s16 actionAngle; // latched when actionPhase 2 starts, copied into `angle` each frame (misc3's BattleActorUpdateVelocity)
    /* 0x90E */ u8 savedModelIndex;
    /* 0x90F */ u8 unk90F;
    /* 0x910 */ u8 soundTimer; // frames left before the gauge sound repeats (src/battling/main/misc3.c)
    /* 0x911 */ u8 flaggedActionCount;
    /* 0x912 */ u8 unflaggedActionCount;
    // 0x914..0x15D0 hold the action timers, the four bound/mid points, the part
    // animation state, the message queue and the 0xC00 state record misc3's
    // BattleActor names; this unit's own code stops at 0x912.
    /* 0x913 */ u8 unk913[0xCC1];
    /* 0x15D4 */ u8 effectColorR;
    /* 0x15D5 */ u8 effectColorG;
    /* 0x15D6 */ u8 effectColorB;
    /* 0x15D7 */ u8 pad15D7;
    /* 0x15D8 */ u8 pad15D8[0x10];
    /* 0x15E8 */ s16 unk15E8;
    /* 0x15EA */ s16 unk15EA;
    /* 0x15EC */ s16 unk15EC; // read as two 16-bit words by misc2's BattleComputeScaledFieldDelta
    /* 0x15EE */ s16 unk15EE;
    /* 0x15F0 */ s16 stepScale; // from the script header's stepScale; multiplies the distance into stepDistance
    /* 0x15F2 */ s16 moveSpeed;
    /* 0x15F4 */ s16 moveSpeedBase;
    /* 0x15F6 */ s16 unk15F6; // the round reset sets 0x100 here and 0 at the next word (misc4)
    /* 0x15F8 */ s16 unk15F8;
    /* 0x15FA */ u8 pad15FA[0x2];
    /* 0x15FC */ void* pWork; // the per-slot work record the state handlers run on (misc17/misc18's pWork)
    /* 0x1600 */ Misc10ActionRow* pActionRow;
} Misc10Actor; // Size: 0x1604
STATIC_ASSERT_SIZEOF(Misc10Actor, 0x1604);

typedef struct {
    /* 0x00 */ u8 unk00[0x44];
    /* 0x44 */ s16 rotX;
    /* 0x46 */ s16 rotY;
    /* 0x48 */ s16 rotZ;
} Misc10Draw; // Size: unknown

typedef struct {
    /* 0x0 */ u16 entryIndex;
    /* 0x2 */ u16 pad2;
    /* 0x4 */ s32 buffer;
} Misc10Resource; // Size: 0x8

typedef struct {
    /* 0x00 */ u8 unk00[0x310];
} Misc10Prim; // Size: 0x310

typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
} Misc10Point; // Size: 0x4

typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
    /* 0x4 */ s16 w;
    /* 0x6 */ u16 h;
} Misc10TexRect; // Size: 0x8

typedef struct {
    /* 0x00 */ u8 unk00[0x04];
    /* 0x04 */ Misc10Point* crect;
    /* 0x08 */ u8 unk08[0x04];
    /* 0x0C */ Misc10TexRect* prect;
} Misc10TagSrc; // Size: 0x10

// The archive header ResolveArchiveEntryPointers relocates before func_80082C4C
// and the rest read it: the message strings and the action-row table are the only
// two pointers this unit takes out of it.
typedef struct {
    /* 0x0 */ u32 pointerCount; // ResolveArchiveEntryPointers fixes up this many relative pointers, the two below them
    /* 0x4 */ u8* pMessageStrings;
    /* 0x8 */ Misc10ActionRow* pActionRows;
} Misc10ArchiveHeader; // Size: 0xC

// The battle texture archive func_800852C4 decompresses just before it calls the
// TIM loaders; src/battling/main/misc11.c declares the whole 0x94-byte record as
// BattlingTextureArchive and names the entries it loads, and this function takes
// two of the ones it leaves unnamed.
typedef struct {
    /* 0x00 */ s32 pointerCount;
    /* 0x04 */ u_long* unk04[0x15];
    /* 0x58 */ u_long* unk58[6];
} Misc10TextureArchive; // Size: 0x70, the head of misc11's 0x94-byte archive

extern Misc10ActionRow* D_80092874;
extern Misc10Actor g_BattleActor1;
extern Misc10Actor g_BattleActor0;
extern Misc10Resource D_800917C0[];
extern Misc10Prim g_BattleSharedPrimBuffer[];

extern s16 g_BattleDebugFlags;
extern s32* g_BattleGridCells;
extern u8* g_BattleMessageStringTable;
extern s32 g_BattleMultiplyNodeMatrices;
extern u16 g_BattleFrameWaitCount;
extern u8 g_BattleFacingUpdateEnabled;
extern u8 g_BattleStageLightIndex;
extern u8 g_BattleOptionOn;
extern u8 g_BattleDifficultyLevel;
extern u8 g_BattleMatchCount;
extern u8 g_BattlePlayer1ComControlled;
extern u8 g_BattlePlayer2ComControlled;
extern s32 g_BattleStateAdvanceRequested;
extern s32 g_BattleSavedScreenMode;
extern s32 g_BattleDrawFlags;
extern s32 g_BattleSceneOriginYMirror;
extern s32 g_BattleSceneOriginY;
extern s32 g_BattleSceneOriginZ;
extern s32 g_BattleResultSide;
extern s16 g_BattleCombatant0DownCount;
extern s16 g_BattleCombatant1DownCount;
extern u8 g_BattleMessageWindow[];
extern u16 g_C1ButtonState;
extern u16 g_C2ButtonState;
extern u16 g_C1ButtonStateReleased;
extern u16 g_C1ButtonStatePressedOnce;
extern s32 g_SoundAudioManager;
extern u8 g_BattleRequestedMode;
extern u8 D_8005061E;
extern u8 D_8005061F;
extern u8 g_BattleRequestedMatchCount;
extern u8 g_BattleRequestedDifficultyLevel;
extern s32 D_80092798;
extern s32 D_8009279C;
extern void* D_800927B8;
extern s32 g_BattleFloorOutlineArc;
extern void* g_BattleActor0Node;
extern void* g_BattleActor1Node;
extern s32 D_80097018;

extern void* BattleCreateOrderingTablePair(s32 count);
extern void* BattleCreateLightGroup(void* pParent);
extern void ModelPartResolvePointers(void* pModelPart);
extern void ArchiveStartStreamQueue(void* pQueue, s32 arg1, s32 arg2);
extern void BattleSetScreenResolution(s32 width, s32 height);
extern void BattleClearLineSlots(void);
extern void func_80030988(s16 a0, s16 a1, s32 a2, s32 a3);
extern void ControllerSetFrameCallback(void (*pCallback)(void));
extern void FontLoadClut(short xDest, u_short yDest);
extern void SystemTransferPaletteToVRAM(short xDest, short yDest);
extern void BattleInitTextLists(void);
extern void BattleInitOptionDefaults(void);
extern void BattleEffectClearResourcePointers(void);
extern void BattleFreeScreenBuffer(s32 arg);
extern void SoundAddSedsEntry(s32 entryId);
extern s32* SoundCreateAudioManagerOnHeap(void* pData);
extern void SoundPlayAudioManager(s32* pManager, s32 volume, s32 frames);
extern void LZSSDecompress(void* pSrc, void* pDst);
extern void* LZSSHeapDecompress(void* pCompressed, s32 flags);
extern void func_80081ECC(void);
extern void ResolveArchiveEntryPointers(void* pHeader);
extern void func_8007EEE8(s32 arg);
extern void func_80082C4C(u8* pArchive);
extern void BattleEffectInitTextures(void* pArchive);
extern void BattleLoadTextTimImages(void* pArchive);
extern void BattlingLoadTextureArchive(void* pArchive);
extern void func_800868E0(Misc10TextureArchive* pArchive);
extern void BattleLoadTextureTimAndPrims(u8* pArchive);
extern void BattleInitSpriteRectTable(void);
extern void BattleInitMessageWindow(void);
extern void BattleFreeNodeTree(void* pNode);
extern void BattleInitTextWindow(void);
extern void BattleEffectInitResources(void);
extern void BattleResetState(void);
extern void SystemStringReset(void* pString);
extern void BattleFreeTextArchive(void);
extern void BattleLoadSelectionTextures(s32 entry0, s32 entry1);
extern void BattleInitTutorialMessage(void);
extern s32 BattleIsDefaultTextList(void);
extern void BattleReloadSlotEntries(void);
extern void BattleUpdateTextWindow(void);
extern void ControllerMergeStates(void);
extern void func_8008BC04(void);
extern void func_80034888(void* pString, void* pOt, s32 arg);
extern void func_80079DF0(void* pActor1, void* pActor0);
extern void func_800796B8(void* pActor1, void* pActor0);
extern void BattleUpdateCombatantPair(void* pActor1, void* pActor0);
extern void BattleUpdateCombatantPairTransition(void* pActor1, void* pActor0);
extern void BattleRenderCameraFrame(void* pScene);
extern void BattleRenderLightGroupFrame(void* pScene);
extern void func_80072858(void* pScene);
extern void func_80084A40(void* pScene);
extern void BattleUpdateTutorialScreen(void);
extern void func_80083310(s32 arg);
extern void SetGeomScreen(long distance);
extern void FontSetColor(s32 red, s32 green, s32 blue);
extern void FontSetPositionsByOffsetClamped(s32 offsetX, s32 offsetY);
extern void BattleSetScreenTileBlack(void);
extern void BattleSetScreenTileGrey(void);
extern void BattleClearAndQueueActorPrims(void* pScene);
extern char BattleResetForNextRound(void);
extern void BattleEffectUploadClut(u8* pColor0, u8* pColor1);
extern void func_80084C88(Misc10Actor* pActor, Misc10Block* pData, s32 arg2);

extern void BattleSetTpageAndClut(s16 x, s16 y, s16 clutX, s16 clutY);
extern void BattleClearTpageAndClut(void);
extern void* BattleRelocateBlockPointers(void* pData);
extern u8* func_8008B38C(u8* pArg);
extern u8* BattleSetActorSoundTable(Misc10Actor* pActor);

s32 BattleQueueActorPrims(Misc10Actor* pActor) {
    Misc10DisplayWork* pWork = pActor->pDisplayWork;

    BattleDrawEffectImage((u8*)pWork->pOt[g_BattleDrawBufferIndex] + 8, 0xC0, 0);
    BattleDrawTextPrims((u8*)pWork->pOt[g_BattleDrawBufferIndex] + 4);
    BattleAttachActorPrims(pWork);
    BattleAddSharedPrim();
    return 0;
}

void BattleDrawEffectFrame(void) {
    if (g_BattleEffectFrameTimer != 0) {
        func_8008E120();
        func_8007F258(g_BattleRootOrderingTable, 0);
        BattleDrawTextPrims(g_BattleRootOrderingTable);
        BattleDrawEffectImage(g_BattleRootOrderingTable, 0xC0, 1);
        func_8008BC04();
    }
}

void BattleUpdateEffectTimer(void) {
    s16 timer;

    timer = g_BattleEffectFrameTimer;
    if (timer != 0) {
        timer = timer - 4;
        g_BattleEffectFrameTimer = timer;
        if (timer <= 0) {
            g_BattleEffectFrameTimer = 0;
            g_BattleEffectImageActive = 0;
            SoundSetAudioManagerMasterVolumeWithFade(g_BattlingAudioManager, 0x100, 0);
            g_BattleMotionSpeedScale = 0x100;
            BattleEffectFreeResources();
        } else {
            BattleDrawEffectImage(g_BattleRootOrderingTable, timer, 1);
        }
    } else {
        g_BattleEffectImageActive = 0;
    }
}

void BattleActorCreateRotatedPart(Misc10Actor* pActor) {
    void* pPartList;
    Misc10Draw* pDraw;
    u8* pBuf;

    pPartList = pActor->pModelLink->pModel->pPartData->pPartList;
    pDraw = BattleCreateObject();
    pBuf = BattleCreateModelPart();
    BattleSetNodeModelPart(pDraw, pBuf);
    BattleAttachModelPart(pBuf, g_BattleRotatedPartModel);
    BattleAppendListEntry(pPartList, pDraw);
    pDraw->rotY = BATTLE_ANGLE_THREE_QUARTER_TURN;
    pDraw->rotX = 0;
    pDraw->rotZ = BATTLE_ANGLE_QUARTER_TURN;
}

// 97.37% objdiff, retail's exact 840 bytes / 210 insns. The body is
// instruction-identical except one callee-saved pair, one addu operand order and
// one caller-saved rotation in the copy loop:
//   1. pActor is in $s1 and pNode in $s0 where retail has pActor $s0 / pNode $s1;
//      every $s0/$s1 store then differs, the loop's action-row temp is $a1 where
//      retail has $a0, the 0x100 rect width reuses the node register and the
//      prologue save order moves. The swap is LOCAL-ALLOC's, not global-alloc's: the
//      -dl dump has pNode (register 77, "used 3 times across 10 insns, crosses 2
//      calls") take $16 in block 3 before global-alloc reaches pActor (register 72,
//      41 refs, processed 4th), so only a competing block-3 quantity that crosses a
//      call could move it, and neither body has one. The one shape that adds it
//      (pObj live across the append) measures 95.10% / 852 bytes because it moves
//      the pObject store out of the jal delay slot. An empty do-while around the
//      whole body: 96.42%. Preset screen cdk 94.04, 2.6.0 96.13, Default 97.37.
//   2. One addu operand order: retail addu v0,v0,v1 (destination = the
//      visiblePartIndicesOffset temp) where this body emits addu v0,v1,v0
//      (pHeader + offset). SEVEN spellings now measure identical under 2.7.2: the
//      plain sum, the offset written first, an s32 cast, the subscript form
//      &((u8*)pHeader)[off], a named s32 offset declared before and after pHeader,
//      and the double-negated offset pHeader - -(offset). gcc 2.7.2 canonicalises a
//      commutative PLUS by PSEUDO NUMBER (pHeader's is the older one), so no C
//      reaches retail's order here.
//   3. In the 0x16 x 0x16 byte-flip loop retail keeps dst $a2 / read $v1 / col $a0
//      and this body keeps dst $a1 / read $a0 / col $v1 (a 3-cycle of caller-saved
//      temps; pSrc $s0 and pBuf $s1 are retail's already). Not attacked yet.
// CASE VALUES FIXED THIS PASS - the old body had "case 35: case 36:" for the |=1
// arm, so the emitted 40-word table sent idx32 to the |=1 arm and idx34 to the
// default where retail (jtbl_800701C0) has typeId 35 in the |=4 group and 36/37 in
// |=1. objdiff diffs only the function's text, so it scored 97.37 either way while
// check_bin would have failed on .rodata. Correct groups: |=1 = 36, 37 (falling
// through to |=2); |=2 = 38; |=4 = 3, 14, 27, 34, 35, 39, 41, 42; rotated part = 13.
// MEASURED, same size 840 with the earlier bodies: 88.91 = |=1/|=2 duplicated, |=3
// fused, pVisiblePartIndices re-read from pActor->pScript; 94.57 = the |=1 arm
// falling through into case 38 (one shared |=2 body); 95.02 = the old store order
// (pModelLink stored after the switch, 12 insns later) - that DOES give pActor $s0 /
// pNode $s1 but costs the store position; 95.78 = pModelLink store moved to its
// retail position (before typeFlags = 0); 97.37 = pActions read into a local before
// the pModelScriptOffsets store (permuter find), which makes the pPartTimings
// load/store adjacent as retail has it.
/*
void func_80084C88(Misc10Actor* pActor, Misc10Block* pData, s32 arg2) {
    RECT rect;
    Misc10SpriteHeader* pHeader;
    Misc10ActionRow* pRow;
    Misc10ModelNode* pNode;
    Misc10ActionSlot* pActions;
    u8* pObj;
    u8* pBuf;
    u8* pSrc;
    u8* pDst;
    u8* pRead;
    s32 row;
    s32 col;
    s32 i;

    pActor->flags = (pActor->flags & 0xF7FFFFFF) | ((arg2 & 1) << 27);
    if (arg2 != 0) {
        BattleSetTpageAndClut(0x380, 0, 0, 0x1FE);
        pActor->pTarget = (u8*)&g_BattleActor1;
    } else {
        BattleSetTpageAndClut(0x3C0, 0, 0, 0x1FF);
        pActor->pTarget = (u8*)&g_BattleActor0;
    }
    BattleRelocateBlockPointers(pData);
    pNode = (Misc10ModelNode*)func_8008B38C((u8*)pData);
    pObj = (u8*)BattleCreateObject();
    pActor->pObject = pObj;
    BattleAppendListEntry(pObj, pNode);
    pActor->pModelLink = pNode;
    pActor->pActionRow = &D_80092874[pActor->typeId];
    pActor->typeFlags = 0;
    switch (pActor->typeId) {
    case 36:
    case 37:
        pActor->typeFlags |= 1;
    case 38:
        pActor->typeFlags |= 2;
        break;
    case 3:
    case 14:
    case 27:
    case 34:
    case 35:
    case 39:
    case 41:
    case 42:
        pActor->typeFlags |= 4;
        break;
    case 13:
        BattleClearTpageAndClut();
        BattleActorCreateRotatedPart(pActor);
        break;
    }
    BattleSetActorSoundTable(pActor);
    pHeader = pData->pHeader;
    pActor->pScript = (u8*)pHeader;
    pActor->pPartTimings = pData->pPartTimings;
    pActions = pData->pActions;
    pActor->pModelScriptOffsets = (s16*)&pHeader[1];
    pActor->pActions = pActions;
    pActor->pVisiblePartIndices = (u8*)pHeader + pHeader->visiblePartIndicesOffset;
    pActor->visiblePartCount = pHeader->visiblePartCount;
    pActor->effectColorR = pHeader->effectColorR;
    pActor->effectColorG = pHeader->effectColorG;
    pActor->effectColorB = pHeader->effectColorB;
    pActor->flaggedActionCount = 0;
    pActor->unflaggedActionCount = 0;
    pRow = pActor->pActionRow;
    for (i = 0; i < 0xE; i++) {
        if (pRow->slotFlags[i] != 0) {
            if (pActor->pActions[i].flags != 0) {
                pActor->flaggedActionCount++;
            } else {
                pActor->unflaggedActionCount++;
            }
        }
    }
    rect.x = 0;
    rect.y = arg2 + 0x1F6;
    rect.w = 0x100;
    rect.h = 1;
    LoadImage(&rect, (u_long*)pData->pImage);
    rect.x = arg2 * 0x10 + 0x380;
    rect.y = 0x100;
    rect.w = 0xB;
    rect.h = 0x16;
    if (arg2 != 0) {
        LoadImage(&rect, (u_long*)(pData->pImage + 0x200));
    } else {
        pSrc = pData->pImage + 0x200;
        pBuf = (u8*)HeapAlloc(0x1E4, 0);
        pDst = pBuf;
        for (row = 0; row < 0x16; row++) {
            pRead = pSrc + 0x15;
            for (col = 0; col < 0x16; col++) {
                *pDst++ = *pRead--;
            }
            pSrc += 0x16;
        }
        LoadImage(&rect, (u_long*)pBuf);
        HeapDelayedFree(pBuf, 1);
    }
    rect.x = 0x3A0;
    rect.y = arg2 * 8 + 0x100;
    rect.w = 0x10;
    rect.h = 8;
    LoadImage(&rect, (u_long*)(pData->pImage + 0x3E4));
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80084C88);



void BattleUpdateEffectImage(void) {
    if (g_BattleEffectImageActive != NULL && (g_SpuChannelMask & 0x1)) {
        func_8008E120();
    }
}

void BattleSetBothSidesCom(void) {
    g_BattleControlFlags &= ~BATTLE_CONTROL_FLAG_USER;
    BattleClearScreen(0);
    BattleSetScreenMode(5);
    g_BattlePlayer2ComControlled = 1;
    g_BattlePlayer1ComControlled = 1;
    g_BattleMode = 6;
}

void BattleSetBothSidesUser(void) {
    u8 flags = g_BattleControlFlags;

    g_BattlePlayer2ComControlled = 0;
    g_BattlePlayer1ComControlled = 0;
    g_BattleControlFlags = flags | BATTLE_CONTROL_FLAG_USER;
}

void BattleLoadSlotEntry(s32 index, s32 value) {
    if (index != 0) {
        g_BattleSlot1EntryId = value;
    } else {
        g_BattleSlot0EntryId = value;
    }
    if (g_BattleSlotEntryBuffers[index] != NULL) {
        HeapFree(g_BattleSlotEntryBuffers[index]);
        g_BattleSlotEntryBuffers[index] = NULL;
    }
    ArchiveSetIndex(ARCHIVE_DIR_BATTLE_ENTRIES, 1);
    g_BattleSlotEntryBuffers[index] = BattleLoadArchiveEntry(value + 2);
    ArchiveSetIndex(ARCHIVE_DIR_BATTLE_ENTRIES, 0);
}

void BattleFreeSlotEntry(s32 index) {
    ArchiveCdDataSync(0);
    if (g_BattleSlotEntryBuffers[index] != NULL) {
        HeapFree(g_BattleSlotEntryBuffers[index]);
        g_BattleSlotEntryBuffers[index] = NULL;
    }
}

void BattleAllocResourceBuffer(Misc10Resource* pResource, u_int allocFlags) {
    pResource->buffer = (s32)HeapAlloc(ArchiveDecodeAlignedSize(pResource->entryIndex), allocFlags);
}

void BattleReturnToMainLoop(void) {
    SoundRemoveSedsEntry(g_BattleSedsEntryId);
    if (g_BattleAudioManagerEnabled != 0) {
        SoundStopAudioManager(g_BattlingAudioManager);
        SoundDestroyAudioManager(g_BattlingAudioManager);
    }
    func_80088A40();
    ChangeGameState(1);
    DrawSync(0);
    Vsync(2);
    g_BattleTransitionFlag = 1;
    MainLoop(0);
}

s32 BattleCheckModeOver(void) {
    s32 result = 0;

    if (g_BattleScreenMode == 5 || g_BattleScreenMode == 7) {
        result = 1;
    } else if (g_BattleScreenMode == 1 && g_BattleMode == 4 && g_BattleTransitionFlag == 0) {
        result = 1;
    }
    return result;
}



INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_800852C4);

/* 99.24% objdiff, retail's exact 2928 bytes. This is the battle scene's init and
   frame loop; its jtbl_80070260 switch on g_BattleSavedScreenMode is the table this
   stub owns. The body below is a complete transcription, instruction-identical to
   retail except for ONE register pair (RESIDUAL below).

   FIXED THIS ROUND: the block the audio manager is created on
   (D_800917C0[0].buffer) is its own variable (pAudioBlock), not the pBuf the two
   LZSS results are assigned to. With one pBuf covering both lifetimes gcc gave the
   early block s1 and pushed the D_800917C0 base out of s0; as two variables the
   early one takes s0 exactly as retail does (retail: lui s0,%hi(D_800917C4) ...
   addu s1,v0 for the LZSS result), and the body went 99.19 -> 99.24. Splitting
   either LZSS result off, or the archive pointer, is worse (99.13 / 99.14), so
   those are one variable each in the original too.

   FIXED EARLIER, each one a source shape:
   - The frame loop's `if (state == 0) ... else if (state == 1) ...` is a
     `switch (state)` with case 0 and case 1. gcc 2.7.2 puts both tests up front
     (beqz / li v0,1 / beq / nop / j / nop) and both arms out of line; that is
     retail's 6-instruction dispatch and the two instructions the if/else-if body
     was short.
   - The g_BattleRequestedMode switch lists its cases in ASCENDING order (0, 1, 2). Case
     bodies come out in source order and retail's are at +0x464 (0), +0x4b8 (1) and
     +0x4e0 (2); listing 1, 2, 0 puts case 0 out of line and costs 4 instructions.
   - `state = 1;` sits BETWEEN the two BattleLoadSlotEntry calls. With it before
     both, reorg duplicates `li s0,1` into the four jump delay slots and leaves
     `addu a0,zero,zero` for the jal; retail has the argument in the jump slots and
     `ori s0,zero,1` in the jal's slot, so the state store is the first instruction
     after the first call.
   - `g_BattleEffectImageActive = NULL;` comes BEFORE
     ControllerSetFrameCallback(BattleUpdateEffectImage), not after.
   - `count` is u8, not s32 (it is loaded through a u8 pointer).
   - pActor1/pActor0 are NOT locals: the two actor addresses appear inline
     (`g_BattleActor1.position.x = 0x3E80;`, `func_80079DF0(&g_BattleActor1,
     &g_BattleActor0)`), and gcc then keeps them in s1/s0 across the whole loop.

   RESIDUAL, one allocno pair and the li order it implies, no size difference:
   pScene is s3 here and s2 in retail, and the chain that holds the constant 1, the
   ~1 mask and &g_BattleMatchCount is s2 here and s3 in retail - the same two
   allocnos swapped. Retail's `ori s3,zero,1` also lands at the top of the reinit
   block (0x855EC, in BattleFreeNodeTree's delay slot) where this body emits
   `ori s0,zero,2` (state = 2) and keeps the 1 until two calls later; the two `li`
   positions are tied to the same pair.
   WHY NO SPELLING REACHES IT (round 434: -da/-dg dump of THIS body, cc1 2.7.2-psx
   with the unit's flags). The mirror is not decided by the two allocnos' own
   priorities - the contest is won by a THIRD, short-lived one. Pseudo 72 = pScene,
   15 refs / 463 insns (quotient 0.39); pseudo 122 = the const_int 1 that feeds the
   `g_BattleTransitionFlag == 1` bne, 4 refs / 58 insns (0.55); pseudo 75 =
   pMatchCount, 7 refs / 240 insns (0.23). The dump's processing order is "18 regs to
   allocate: 81 230 240 73 76 143 121 78 79 122 132 208 207 72 155 77 75 80" and its
   dispositions read "122 in 18 ... 72 in 19 ... 75 in 18", i.e. 122 takes $s2 before
   72's turn and pushes pScene to $s3, and 75 then takes the $s2 that 122 released -
   retail's pair exactly, mirrored. Nothing in the source moves 122's quotient: its
   four refs are the def, the bne, the g_BattleTransitionFlag = 1 store and the
   REG_EQUAL. So only a different allocno set flips it (the pAudioBlock split above
   did move a pair, which is the one handle that has worked here).
   MEASURED, all at 2928 bytes unless noted. Round 434 on this body: state = 2 before
   g_BattleScreenMode = -1 99.24; state = 2 after the facing/stage stores 99.24; the
   facing/stage stores swapped 99.21; a u8 `transition` local feeding both
   g_BattleTransitionFlag tests 99.24 (identical code). On the 99.19 body (one pBuf):
   pMatchCount inlined as &g_BattleMatchCount 98.87 (2932); typed Misc10Actor*
   pScene 99.19; pScene declared last 99.19; state = 2 moved after the facing/stage
   stores 99.19; pMatchCount born early 98.70 (2932); `& 0xFFFE` instead of `& ~1`
   98.36 (2916); a temp for BattleCreateOrderingTablePair's result - same code. Two
   permuter runs (10 min: 3929 -> 665; 7 min: 665 -> 580) found nothing usable, so
   do not spend another one; round 434 ran a third on this body (3 min, base 620
   best 535) and its best candidate merged `state` into `idleTimer`, i.e. wrong code.
   On THIS body: pRes folded into D_800917C0[i] 99.24 (neutral, so pRes is the
   original's variable), the second LZSS result as its own variable 99.13, the
   first LZSS result as its own variable 99.13 (so both are pBuf), the second
   archive as its own variable 99.19 (so both are pArchive), idleCount += 1 99.24,
   a second pScene alias for the draw arms 98.80 (2936). Nothing tried moves the
   pScene/constant pair; the one thing that did move a pair was the pAudioBlock
   split above.

   s32 func_800852C4(void) {
       void* pScene;
       Misc10ArchiveHeader* pArchive;
       Misc10Resource* pRes;
       u8* pMatchCount;
       u8* pBuf;
       u8* pAudioBlock;
       s32 state;
       s32 idleTimer;
       s32 idleCount;
       u8 count;

       g_BattleDebugFlags = 0;
       g_BattleControlFlags |= BATTLE_CONTROL_FLAG_USER;
       g_BattleGridCells = HeapAlloc(BATTLE_GRID_CELLS_SIZE, 0);
       pScene = BattleCreateLightGroup(BattleCreateOrderingTablePair(BATTLE_ORDERING_TABLE_SIZE));
       ModelPartResolvePointers(g_BattleRotatedPartModel);
       pRes = D_800917C0;
       BattleAllocResourceBuffer(&pRes[0], 0);
       BattleAllocResourceBuffer(&pRes[1], 0);
       BattleAllocResourceBuffer(&pRes[2], 1);
       BattleAllocResourceBuffer(&pRes[3], 1);
       BattleAllocResourceBuffer(&pRes[4], 1);
       ArchiveStartStreamQueue(pRes, 0, 0);
       g_BattleSedsEntryId = D_800917C0[1].buffer;
       pAudioBlock = (u8*)D_800917C0[0].buffer;
       BattleSetScreenResolution(0x140, 0xDA);
       BattleClearLineSlots();
       func_80030988(1, 1, 0x40, 0x40);
       g_BattleEffectImageActive = NULL;
       ControllerSetFrameCallback(BattleUpdateEffectImage);
       FontLoadClut(0x7FFF, 0x8000);
       SystemTransferPaletteToVRAM(0x140, 0xFF);
       BattleInitTextLists();
       BattleInitOptionDefaults();
       BattleEffectClearResourcePointers();
       D_800927B8 = NULL;
       g_BattleSlotEntryBuffers[0] = NULL;
       g_BattleActor0Node = NULL;
       g_BattleActor1Node = NULL;
       BattleFreeScreenBuffer(1);
       ArchiveCdDataSync(0);
       SoundAddSedsEntry(g_BattleSedsEntryId);
       if (g_BattleAudioManagerEnabled == 0) {
           goto sharedAudioManager;
       }
       g_BattlingAudioManager = SoundCreateAudioManagerOnHeap(pAudioBlock);
       SoundPlayAudioManager(g_BattlingAudioManager, 0x7F, 0);
       goto audioManagerReady;

   screenRelease:
       g_C1ButtonStateReleased = 0;
       g_C1ButtonStatePressedOnce = 0;
       BattleSetBothSidesUser();
       g_BattleFrameWaitCount = 0;
       BattleSetScreenTileBlack();
       func_8008BC04();
       goto reinit;

   sharedAudioManager:
       g_BattlingAudioManager = g_SoundAudioManager;

   audioManagerReady:
       LZSSDecompress((void*)D_800917C0[3].buffer, g_BattleGridCells);
       HeapFree((void*)D_800917C0[3].buffer);
       func_80081ECC();
       pArchive = LZSSHeapDecompress((void*)D_800917C0[2].buffer, 0);
       HeapFree((void*)D_800917C0[2].buffer);
       ResolveArchiveEntryPointers(pArchive);
       g_BattleMessageStringTable = pArchive->pMessageStrings;
       D_80092874 = pArchive->pActionRows;
       func_8007EEE8(g_BattleTransitionFlag == 1);
       pArchive = LZSSHeapDecompress((void*)D_800917C0[4].buffer, 1);
       HeapFree((void*)D_800917C0[4].buffer);
       ResolveArchiveEntryPointers(pArchive);
       func_80082C4C((u8*)pArchive);
       BattleEffectInitTextures(pArchive);
       BattleLoadTextTimImages(pArchive);
       BattlingLoadTextureArchive(pArchive);
       func_800868E0((Misc10TextureArchive*)pArchive);
       BattleLoadTextureTimAndPrims((u8*)pArchive);
       HeapFree(pArchive);
       BattleInitSpriteRectTable();
       BattleInitMessageWindow();
       func_8008BC04();
       g_BattleMultiplyNodeMatrices = 0;

   reinit:
       HeapFreeAllDelayedBlocks();
       idleTimer = BATTLE_IDLE_TIMEOUT;
       BattleFreeNodeTree(g_BattleActor1Node);
       BattleFreeNodeTree(g_BattleActor0Node);
       HeapFreeAllDelayedBlocks();
       BattleInitTextWindow();
       g_BattleEffectFrameTimer = 0xFF;
       g_BattleEffectImageActive = NULL;
       BattleEffectInitResources();
       BattleResetState();
       SystemStringReset(g_BattleMessageWindow);
       g_BattleScreenMode = -1;
       state = 2;
       g_BattleFacingUpdateEnabled = 0;
       g_BattleStageLightIndex = 0;
       if (g_BattleTransitionFlag == 1) {
           g_BattleFrameWaitCount = 0;
           state = 0;
           goto runLoop;
       }
       if (g_BattleTransitionFlag == state) {
           g_BattleFacingUpdateEnabled = 1;
           g_BattleOptionOn = 0;
       }
       g_BattleDifficultyLevel = g_BattleRequestedDifficultyLevel;
       g_BattleMatchCount = g_BattleRequestedMatchCount;
       g_BattleTransitionFlag = 0;
       BattleFreeTextArchive();
       D_80092798 = D_8005061E;
       D_8009279C = D_8005061F;
       g_BattlePlayer1ComControlled = 0;
       switch (g_BattleRequestedMode) {
       case 0:
           g_BattlePlayer2ComControlled = 1;
           BattleSetScreenMode(1);
           g_BattleMode = 1;
           BattleLoadSelectionTextures(D_80092798, D_8009279C);
           if (D_8009279C == 5) {
               g_BattleStageLightIndex = 1;
           }
           break;
       case 1:
           g_BattlePlayer2ComControlled = 1;
           BattleSetScreenMode(1);
           g_BattleMode = 4;
           break;
       case 2:
           g_BattlePlayer2ComControlled = 0;
           BattleSetScreenMode(7);
           g_BattleMode = 5;
           BattleInitTutorialMessage();
           break;
       }
       BattleLoadSlotEntry(0, D_80092798);
       state = 1;
       BattleLoadSlotEntry(1, D_8009279C);
       g_BattleStateAdvanceRequested = 0;
       g_BattleFrameWaitCount = 2;

   runLoop:
       if (BattleCheckModeOver()) {
           ArchiveCdDataSync(0);
           state = 10;
       }
       while (state != 10) {
           if ((g_C1ButtonState & ~1) != 0 || (g_C2ButtonState & ~1) != 0) {
               idleTimer = BATTLE_IDLE_TIMEOUT;
           }
           if ((g_C1ButtonState & 1) != 0 && BattleIsDefaultTextList() != 0) {
               idleCount++;
               if (idleCount == BATTLE_IDLE_HOLD_COUNT) {
                   idleTimer = 0;
               }
           } else {
               idleCount = 0;
           }
           if (idleTimer != 0) {
               idleTimer--;
           } else if (g_BattleEffectFrameTimer != 0) {
               BattleReloadSlotEntries();
               g_BattleEffectFrameTimer = 0;
               ArchiveCdDataSync(0);
               BattleSetBothSidesCom();
               goto done;
           }
           func_80084A40(pScene);
           switch (state) {
           case 0:
               BattleUpdateTextWindow();
               if (g_BattleStateAdvanceRequested != 0) {
                   g_BattleFrameWaitCount = 2;
                   state = 1;
               }
               if ((g_BattleDrawFlags & 1) != 0) {
                   func_8008E120();
               }
               break;
           case 1:
               ControllerMergeStates();
               if (BattleCheckModeOver() == 0) {
                   func_8008E120();
                   func_8007F258(g_BattleRootOrderingTable, 0);
               }
               if (ArchiveCdDataSync(1) == 0) {
                   state = 10;
               }
               break;
           }
           BattleQueueActorPrims((Misc10Actor*)pScene);
           func_8008BC04();
       }

   done:
       g_BattleFrameWaitCount = 0;
       if (BattleCheckModeOver()) {
           g_BattleEffectFrameTimer = 0;
           g_BattleEffectImageActive = NULL;
           BattleEffectFreeResources();
           BattleFreeTextArchive();
       }
       pBuf = LZSSHeapDecompress(g_BattleSlotEntryBuffers[0], 0);
       HeapFree(g_BattleSlotEntryBuffers[0]);
       g_BattleSlotEntryBuffers[0] = pBuf;
       BattleDrawEffectFrame();
       pBuf = LZSSHeapDecompress(D_800927B8, 0);
       HeapFree(D_800927B8);
       D_800927B8 = pBuf;
       BattleDrawEffectFrame();
       func_80084C88(&g_BattleActor1, (Misc10Block*)g_BattleSlotEntryBuffers[0], 0);
       BattleDrawEffectFrame();
       func_80084C88(&g_BattleActor0, (Misc10Block*)D_800927B8, 1);
       BattleDrawEffectFrame();
       BattleEffectUploadClut(&g_BattleActor1.effectColorR, &g_BattleActor0.effectColorR);
       BattleSetScreenTileGrey();
       if (g_BattleEffectFrameTimer != 0) {
           g_BattleEffectImageActive = (u8*)1;
           SoundSetAudioManagerMasterVolumeWithFade(g_BattlingAudioManager, 0x158, 0);
           g_BattleMotionSpeedScale = 0x200;
       }
       g_BattleResultSide = 2;

   newRound:
       BattleResetForNextRound();
       g_BattleActor1.position.x = 0x3E80;
       g_BattleSceneOriginYMirror = 0;
       g_BattleSceneOriginZ = 0x3F80;
       g_BattleActor0.position.x = 0x4080;
       g_BattleSceneOriginY = 0;
       D_80097018 = 0x3F80;
       pMatchCount = &g_BattleMatchCount;
       for (;;) {
           FontSetColor(0, 0xFF, 0);
           g_BattleFrameWaitCount = 2;
           g_BattleFloorOutlineArc = 0;
           g_BattleSavedScreenMode = g_BattleScreenMode;
           BattleUpdateEffectTimer();
           func_80034888(g_BattleMessageWindow, g_BattleRootOrderingTable, g_BattleDrawBufferIndex);
           switch (g_BattleSavedScreenMode) {
           case 1:
               func_80079DF0(&g_BattleActor1, &g_BattleActor0);
               SetGeomScreen(0xC0);
               if (g_BattleScreenMode == 1) {
                   func_800796B8(&g_BattleActor1, &g_BattleActor0);
               }
               goto commonDraw;
           case 6:
               func_80072858(pScene);
               break;
           case 7:
               BattleUpdateTutorialScreen();
               SetGeomScreen(0xC0);
               BattleRenderCameraFrame(pScene);
               if (g_BattleScreenMode == 3) {
                   g_BattleFrameWaitCount = 0;
                   func_8008BC04();
                   goto case3;
               }
               break;
           case 4:
               SetGeomScreen(0x800);
               BattleUpdateCombatantPair(&g_BattleActor1, &g_BattleActor0);
               if (g_BattleScreenMode != 8) {
                   func_80083310(1);
                   g_BattleFloorOutlineArc = 1;
               }
               goto commonDraw;
           case 8:
               SetGeomScreen(0x200);
               BattleUpdateCombatantPairTransition(&g_BattleActor1, &g_BattleActor0);
           commonDraw:
               BattleRenderCameraFrame(pScene);
               break;
           case 0:
               BattleUpdateTextWindow();
               BattleClearAndQueueActorPrims(pScene);
               g_BattleFrameWaitCount = 0;
               break;
           case 5:
               ControllerMergeStates();
               if (g_C1ButtonStateReleased != 0) {
                   goto screenRelease;
               }
               func_80079DF0(&g_BattleActor1, &g_BattleActor0);
               SetGeomScreen(0xC0);
               func_800796B8(&g_BattleActor1, &g_BattleActor0);
               BattleRenderLightGroupFrame(pScene);
               break;
           case3:
           case 3:
               if (g_BattleTransitionFlag == 0) {
                   BattleReturnToMainLoop();
               }
               BattleSetScreenTileBlack();
               goto reinit;
           case 2:
               count = *pMatchCount;
               if (count == 0) {
                   goto romMatchOver;
               }
               if (g_BattleCombatant1DownCount == count) {
                   count = pMatchCount[-2];
               } else if (g_BattleCombatant0DownCount == count) {
                   count = pMatchCount[-1];
               } else {
                   goto romMatchOver;
               }
               if (count != 0) {
                   goto case3;
               }
               BattleSetScreenMode(6);
               break;
           romMatchOver:
               BattleSetScreenMode(1);
               goto newRound;
           }
           FontSetPositionsByOffsetClamped(0x9E, 0);
           FontSetColor(0xFF, 0xFF, 0);
           func_8008BC04();
       }

   }
   ROUND 447 (redo lane, no change to the body): re-measured 99.24% / 2928 bytes
   with the body live, and CLOSED THE PRESET AXIS, which the old note never did:
   try_flags.sh gives gcc-2.6.0 99.24 (identical allocation), gcc-2.7.2-cdk 91.22,
   plain -G0 99.24, and -O3 / cdk with -G0 do not compile. So the mirrored pair is
   not a compiler-version question and no preset switch fixes it; every already
   matched function in the unit is Default 100.00, so there is nothing to report.
   THE MARGIN IS NOW ARITHMETIC rather than a guess: the -da numbers fit
   floor_log2(refs) * refs * size / live_length exactly (pScene 3*15*4/463 = 0.389,
   the const_int 1 2*4*4/58 = 0.552, pMatchCount 2*7*4/240 = 0.233), so pScene
   overtakes 122 only at refs >= 16 (floor_log2 jumps to 4, giving 0.553) or at
   live_length <= 326 insns, and 122 falls behind only past a live range of 82
   insns. pScene is created at the top of the function and last used in the case 5
   arm, and every term on 122 is a def, a branch, a store and a REG_EQUAL, so all
   three are pinned by retail's own instruction stream: one more reference to
   pScene would be one instruction more than retail's 2928 bytes. This is a
   reachability proof, not a spelling gap - do not permute this function again.
   ONE HONEST CAVEAT, found this round: idleCount is read UNINITIALISED (idleCount++
   with no prior store on the runLoop path) and retail does the same thing - $s4 is
   saved in the frame and never defined anywhere before the addiu s4,s4,1 at
   0x85884 - so the UB read is what the original has; initialising the counter costs
   the extra instruction and 2932 bytes.
   ROUND 460 (redo lane): re-measured 99.24% / 2928 bytes unparked, and it is still
   the same mirror. A fourth permuter run (102 iterations in 3 min, base 620) came
   back best 535 - the same wrong-code merge of `state` into `idleTimer` that round
   434 rejected - so the permuter is now exhausted on this body too, not just the
   spellings. tools/scripts/lever.py's a-global-allocno-priority-is and
   a-a-near-miss-allocno-quotient both read this as the arithmetic class: pScene
   needs live <= 326 insns or 16 refs (0.553 vs 0.552 is the actual margin, so ONE
   ref would do it) and both are pinned by retail's 732-instruction stream.
*/

void BattleCopyVertexOffsetAdd(u16* pSrc, u16* pDst) {
    pDst[0] = pSrc[0] + 0x18;
    pDst[1] = pSrc[1] + 0x6;
    pDst[0] += 0x4F;
}

void BattleCopyVertexOffsetFlip(u16* pSrc, u16* pDst) {
    pDst[0] = 0x8B - pSrc[0];
    pDst[1] = 0x20 - pSrc[1];
    pDst[0] += 0x4F;
}

void BattleSetVertexXForFlipped(s32 flag, s16* pDst, s32 value) {
    if (flag != 0) {
        *pDst = 0xDA - value;
    } else {
        *pDst = 0x67 + value;
    }
}

void BattleSetVertexYForFlipped(s32 flag, s16* pDst, s32 value) {
    if (flag != 0) {
        *pDst = 0x20 - value;
    } else {
        *pDst = 0x6 + value;
    }
}

// Packet word store, as include/system/font.h's setXY0Fast / setUV0Fast family.
#define Misc10SetXY0(p, _x0, _y0) *(s32*)&(p)->x0 = (_x0) | ((_y0) << 0x10)
#define Misc10SetXY1(p, _x1, _y1) *(s32*)&(p)->x1 = (_x1) | ((_y1) << 0x10)
#define Misc10SetXY2(p, _x2, _y2) *(s32*)&(p)->x2 = (_x2) | ((_y2) << 0x10)
#define Misc10SetXY3(p, _x3, _y3) *(s32*)&(p)->x3 = (_x3) | ((_y3) << 0x10)
#define Misc10SetUV0(p, _u0, _v0) *(u16*)&(p)->u0 = (_u0) | ((_v0) << 0x8)
#define Misc10SetRGBC(p, _r0, _g0, _b0, _code) \
    *(s32*)&(p)->r0 = (_r0) | ((_g0) << 0x8) | ((_b0) << 0x10) | ((_code) << 0x18)

// The two 0x310-byte gauge draw environments that src/battling/main/misc11.c's
// BattleInitDrawEnvPair fills through func_80085EC8 and then swaps between. Every
// entry is a GPU packet template: the DMA word count in tag byte 3, the colour and
// primitive code word, then the vertex pairs the offset table D_800917F4 fills in.
typedef struct {
    /* 0x00 */ u8 pad00[0x3];
    /* 0x03 */ u8 len;
    /* 0x04 */ u8 r0, g0, b0, code;
    /* 0x08 */ u16 x0, y0;
    /* 0x0C */ u16 x1, y1;
    /* 0x10 */ u16 x2, y2;
    /* 0x14 */ u16 x3, y3;
} Misc10Quad; // Size: 0x18

// A gauge segment: the same packet with three vertex pairs.
typedef struct {
    /* 0x00 */ u8 pad00[0x3];
    /* 0x03 */ u8 len;
    /* 0x04 */ u8 r0, g0, b0, code;
    /* 0x08 */ u16 x0, y0;
    /* 0x0C */ u16 x1, y1;
    /* 0x10 */ u16 x2, y2;
} Misc10Triangle; // Size: 0x14

// A background quad followed by the dummy word setLineF4 stamps: the init code
// gives each one len 6 and code 0x4C (a four-point line), then MargePrim joins
// them in pairs.
typedef struct {
    /* 0x00 */ Misc10Quad quad;
    /* 0x18 */ u32 pad;
} Misc10BackgroundQuad; // Size: 0x1C

typedef struct {
    /* 0x000 */ DR_TPAGE tpage[2];
    /* 0x010 */ Misc10BackgroundQuad background[4];
    /* 0x080 */ Misc10Quad barPrim[6];
    /* 0x110 */ DR_TPAGE barTpage;
    /* 0x118 */ Misc10Quad barFront[6];
    /* 0x1A8 */ Misc10Quad barBack[6];
    /* 0x238 */ Misc10Triangle gauge[3];
    /* 0x274 */ Misc10Triangle gaugeMirror[3];
    /* 0x2B0 */ Misc10Quad meter[4];
} Misc10GaugeEnv; // Size: 0x310

// The vertex offsets the packets above are filled from: eight halfword pairs.
extern u16 D_800917F4[];

// PARKED 96.78%, retail EXACT 1516 bytes, 379 vs 379 instructions. Every remaining
// difference is inside ONE basic block: the meter[2]/meter[3] packet stores after
// the gauge loop (72 diff lines, nothing else in the function differs).
//
// FIXED THIS ROUND, 96.16 -> 96.78: THE GAUGE LOOP'S STATEMENT ORDER. The previous
// note claimed the loop body's statement order was proven right by its store order,
// but sched2 is on in this preset (levers.md h-sched2-is-on-so-order-is-the-
// schedulers-not-the-sources), so the store order is the SCHEDULER'S output and
// proves nothing about the source. Reordering the body to y0 first, then the
// gauge len/SetRGBC pair, then the gaugeMirror len/SetRGBC pair, then y1, y2, then
// the three `0x140 - x` statements reproduced retail's loop exactly: the preheader
// hoist order (`li t7,0x4`, `lui/ori t5,0x2000FF00`, `li t6,0x140` - each
// loop-invariant constant materialises in order of FIRST USE, so 0x140 belongs
// last, which is where retail has it) and the load interleave (y0, y1, x0, y2, x1,
// x2), while the stores still land in retail's order.
//
// REMAINING, one residual, a register hand-out: retail gives the nine hoisted meter
// constant words a1,t2,t1,a3,a2,t0,t5,t4,t3 and keeps v0 for the hoisted
// gaugeMirror[1].y0 load, v1 for 0x1E and a0 for the len 5; our compile takes v1 for
// the 0x280000FF word and never touches t4/t5 (the registers the gauge loop's dead
// constants held). Because our v0 is busy with the constant chain, the `lhu 0x292`
// cannot be hoisted above the 0x1E group, so the whole block rotates - the
// block-local find_free_reg scan-start class.
// MEASURED THIS ROUND, all 1516 bytes / 379 insns:
//  - 96.59: the gaugeMirror[1].y0 read put in a u16 local before the 0x1E group and
//    stored after it, to lengthen its live range - cse propagated the load to its use
//    and the hand-out did not move.
//  - preset screens (2.6.0, cdk) and a 4-minute permuter, on the previous body: the
//    permuter's one edit scored LOWER when ported, so it was reverted.
//  - loop shape (settled, do not re-derive): gauge[3] plus gaugeMirror[3] (one biv,
//    offsets 576/636 off pEnv); gauge[6] with gauge[i + 3] makes a second giv (+4
//    bytes) and a walking pointer re-bases onto pEnv + 0x284 (+0 bytes).
//  - the earlier index-0xE fix (barPrim[0].x1 wants D_800917F4[0xE]) still holds: all
//    forty BattleCopyVertexOffsetAdd/Flip calls are register-identical to retail.
// ROUND RE-MEASURE, still 96.78% / retail EXACT 1516 bytes / 379 insns: the residual is
// now PROVEN to be sched1 freedom plus the block-local hand-out, not the position of
// the two `--` statements. This preset runs BOTH schedulers (-fschedule-insns is on:
// with `-fno-schedule-insns` alone the nine meter constants stop being hoisted to the
// top of the block and are re-materialised inline one at a time). Retail its shape -
// the gaugeMirror[1].y0 load at 0x292 sitting between `sh 0x722` and the 0x1E group,
// the .x2 load at 0x298 after it, and both `addiu -1` / `sh` at the tail - is what
// retail sched1 produced, i.e. its two load VALUES are live across the nine hoisted
// constants and therefore own v0/v1, which is what pushes the constants to
// a1,t2,t1,a3,a2,t0,t5,t4,t3. Source position cannot pin those loads: every
// intervening store is a constant offset off the same base (s8), which 2.7.2
// memrefs_conflict_p proves disjoint, so sched1 sinks or hoists them freely.
// MEASURED THIS ROUND (all with the unit preset; nothing else in the function moved):
//  - 95.95%, 380 insns / 1520 bytes: the two `--` moved to the END of the function.
//    sched1 hoisted the 0x292 load to the top of the block (v1) but left the 0x298
//    one as an inline load/addiu/store trio, and the load-delay slot became a real
//    `nop`, hence +1 instruction. Constants then a0,t4,t3,t1,a3,t2,t0,a2,a1.
//  - 96.77%, 379 insns / 1516 bytes: both reads into u16 locals at their source
//    positions, both stores at the end. sched1 hoisted BOTH loads to the top of the
//    block (above `lui v0,0x280000`) into v1 and a0; the constants then start at a1
//    (retail) but run t5,t4,t2,t0,t3,t1,a3,a2, not retail t2,t1,a3,a2,t0,t5,t4,t3.
//  - still settled: the body below is the best of the three (96.78); the 96.59
//    gaugeMirror-u16-local and the preset screens from the earlier round stand.
/*
void func_80085EC8(Misc10GaugeEnv* pEnv) {
    s32 i;

    SetDrawTPage(&pEnv->tpage[0], 0, 1, (u16)GetTPage(0, 2, 0, 0));
    SetDrawTPage(&pEnv->tpage[1], 0, 0, (u16)GetTPage(0, 1, 0, 0));

    pEnv->background[0].quad.len = 6;
    Misc10SetRGBC(&pEnv->background[0].quad, 0, 0, 0, 0x4C);
    pEnv->background[0].pad = 0x55555555;
    pEnv->background[1].quad.len = 6;
    Misc10SetRGBC(&pEnv->background[1].quad, 0, 0, 0, 0x4C);
    pEnv->background[1].pad = 0x55555555;
    pEnv->background[2].quad.len = 6;
    Misc10SetRGBC(&pEnv->background[2].quad, 0, 0, 0, 0x4C);
    pEnv->background[2].pad = 0x55555555;
    pEnv->background[3].quad.len = 6;
    Misc10SetRGBC(&pEnv->background[3].quad, 0, 0, 0, 0x4C);
    pEnv->background[3].pad = 0x55555555;

    BattleCopyVertexOffsetAdd(&D_800917F4[0], &pEnv->background[0].quad.x0);
    BattleCopyVertexOffsetAdd(&D_800917F4[2], &pEnv->background[0].quad.x1);
    BattleCopyVertexOffsetAdd(&D_800917F4[4], &pEnv->background[0].quad.x2);
    BattleCopyVertexOffsetAdd(&D_800917F4[6], &pEnv->background[0].quad.x3);
    BattleCopyVertexOffsetAdd(&D_800917F4[6], &pEnv->background[1].quad.x0);
    BattleCopyVertexOffsetAdd(&D_800917F4[8], &pEnv->background[1].quad.x1);
    BattleCopyVertexOffsetAdd(&D_800917F4[0xA], &pEnv->background[1].quad.x2);
    BattleCopyVertexOffsetAdd(&D_800917F4[0], &pEnv->background[1].quad.x3);
    BattleCopyVertexOffsetFlip(&D_800917F4[0], &pEnv->background[2].quad.x0);
    BattleCopyVertexOffsetFlip(&D_800917F4[2], &pEnv->background[2].quad.x1);
    BattleCopyVertexOffsetFlip(&D_800917F4[4], &pEnv->background[2].quad.x2);
    BattleCopyVertexOffsetFlip(&D_800917F4[6], &pEnv->background[2].quad.x3);
    BattleCopyVertexOffsetFlip(&D_800917F4[6], &pEnv->background[3].quad.x0);
    BattleCopyVertexOffsetFlip(&D_800917F4[8], &pEnv->background[3].quad.x1);
    BattleCopyVertexOffsetFlip(&D_800917F4[0xA], &pEnv->background[3].quad.x2);
    BattleCopyVertexOffsetFlip(&D_800917F4[0], &pEnv->background[3].quad.x3);

    MargePrim(&pEnv->background[0], &pEnv->background[1]);
    MargePrim(&pEnv->background[2], &pEnv->background[3]);

    pEnv->background[0].quad.x0 = 0x1D;
    pEnv->background[2].quad.x0 = 0x121;

    pEnv->barPrim[0].len = 5;
    Misc10SetRGBC(&pEnv->barPrim[0], 0xFF, 0, 0, 0x28);
    pEnv->barPrim[1].len = 5;
    Misc10SetRGBC(&pEnv->barPrim[1], 0xFF, 0, 0, 0x28);
    pEnv->barPrim[2].len = 5;
    Misc10SetRGBC(&pEnv->barPrim[2], 0xFF, 0, 0, 0x28);
    pEnv->barPrim[3].len = 5;
    Misc10SetRGBC(&pEnv->barPrim[3], 0xFF, 0, 0, 0x28);
    pEnv->barPrim[4].len = 5;
    Misc10SetRGBC(&pEnv->barPrim[4], 0xFF, 0, 0, 0x28);
    pEnv->barPrim[5].len = 5;
    Misc10SetRGBC(&pEnv->barPrim[5], 0xFF, 0, 0, 0x28);

    BattleCopyVertexOffsetAdd(&D_800917F4[0], &pEnv->barPrim[0].x0);
    BattleCopyVertexOffsetAdd(&D_800917F4[0xE], &pEnv->barPrim[0].x1);
    BattleCopyVertexOffsetAdd(&D_800917F4[0xA], &pEnv->barPrim[0].x2);
    BattleCopyVertexOffsetAdd(&D_800917F4[8], &pEnv->barPrim[0].x3);
    BattleCopyVertexOffsetAdd(&D_800917F4[0xE], &pEnv->barPrim[1].x0);
    BattleCopyVertexOffsetAdd(&D_800917F4[0xC], &pEnv->barPrim[1].x1);
    BattleCopyVertexOffsetAdd(&D_800917F4[8], &pEnv->barPrim[1].x2);
    BattleCopyVertexOffsetAdd(&D_800917F4[6], &pEnv->barPrim[1].x3);
    BattleCopyVertexOffsetAdd(&D_800917F4[0xC], &pEnv->barPrim[2].x0);
    BattleCopyVertexOffsetAdd(&D_800917F4[2], &pEnv->barPrim[2].x1);
    BattleCopyVertexOffsetAdd(&D_800917F4[6], &pEnv->barPrim[2].x2);
    BattleCopyVertexOffsetAdd(&D_800917F4[4], &pEnv->barPrim[2].x3);
    BattleCopyVertexOffsetFlip(&D_800917F4[0], &pEnv->barPrim[3].x0);
    BattleCopyVertexOffsetFlip(&D_800917F4[0xE], &pEnv->barPrim[3].x1);
    BattleCopyVertexOffsetFlip(&D_800917F4[0xA], &pEnv->barPrim[3].x2);
    BattleCopyVertexOffsetFlip(&D_800917F4[8], &pEnv->barPrim[3].x3);
    BattleCopyVertexOffsetFlip(&D_800917F4[0xE], &pEnv->barPrim[4].x0);
    BattleCopyVertexOffsetFlip(&D_800917F4[0xC], &pEnv->barPrim[4].x1);
    BattleCopyVertexOffsetFlip(&D_800917F4[8], &pEnv->barPrim[4].x2);
    BattleCopyVertexOffsetFlip(&D_800917F4[6], &pEnv->barPrim[4].x3);
    BattleCopyVertexOffsetFlip(&D_800917F4[0xC], &pEnv->barPrim[5].x0);
    BattleCopyVertexOffsetFlip(&D_800917F4[2], &pEnv->barPrim[5].x1);
    BattleCopyVertexOffsetFlip(&D_800917F4[6], &pEnv->barPrim[5].x2);
    BattleCopyVertexOffsetFlip(&D_800917F4[4], &pEnv->barPrim[5].x3);

    SetDrawTPage(&pEnv->barTpage, 0, 1, (u16)GetTPage(0, 1, 0, 0));
    func_800732AC(&pEnv->barFront[0], &pEnv->barPrim[0], 0x90);
    func_800732AC(&pEnv->barBack[0], &pEnv->barPrim[0], 0x90);

    for (i = 0; i < 6; i++) {
        pEnv->barFront[i].len = 5;
        Misc10SetRGBC(&pEnv->barFront[i], 0x60, 0x60, 0x80, 0x28);
    }
    for (i = 0; i < 6; i++) {
        pEnv->barBack[i].len = 5;
        Misc10SetRGBC(&pEnv->barBack[i], 0xFF, 0, 0, 0x28);
    }

    Misc10SetXY0(&pEnv->gauge[0], 0x14, 0x20);
    Misc10SetXY1(&pEnv->gauge[0], 0x1C, 0x20);
    Misc10SetXY2(&pEnv->gauge[0], 0x1C, 0x28);
    Misc10SetXY0(&pEnv->gauge[1], 0x13, 0x20);
    Misc10SetXY1(&pEnv->gauge[1], 0x13, 0x29);
    Misc10SetXY2(&pEnv->gauge[1], 0x1B, 0x29);
    Misc10SetXY0(&pEnv->gauge[2], 0x13, 0x32);
    Misc10SetXY1(&pEnv->gauge[2], 0x13, 0x2A);
    Misc10SetXY2(&pEnv->gauge[2], 0x1B, 0x2A);

    // Six iterations: retail's own loop writes three mirrored segments past the
    // end of the array, into the empty meter slots below it.
    for (i = 0; i < 6; i++) {
        pEnv->gaugeMirror[i].y0 = pEnv->gauge[i].y0;
        pEnv->gauge[i].len = 4;
        Misc10SetRGBC(&pEnv->gauge[i], 0, 0xFF, 0, 0x20);
        pEnv->gaugeMirror[i].len = 4;
        Misc10SetRGBC(&pEnv->gaugeMirror[i], 0, 0xFF, 0, 0x20);
        pEnv->gaugeMirror[i].y1 = pEnv->gauge[i].y1;
        pEnv->gaugeMirror[i].y2 = pEnv->gauge[i].y2;
        pEnv->gaugeMirror[i].x0 = 0x140 - pEnv->gauge[i].x0;
        pEnv->gaugeMirror[i].x1 = 0x140 - pEnv->gauge[i].x1;
        pEnv->gaugeMirror[i].x2 = 0x140 - pEnv->gauge[i].x2;
    }

    Misc10SetRGBC(&pEnv->meter[0], 0, 0, 0, 0x28);
    Misc10SetRGBC(&pEnv->meter[1], 0, 0, 0, 0x28);
    pEnv->meter[0].x1 = 0x63;
    pEnv->meter[0].x3 = 0x5E;
    pEnv->meter[0].y1 = 9;
    pEnv->meter[0].y0 = 9;
    pEnv->meter[0].y3 = 0x14;
    pEnv->meter[0].y2 = 0x14;
    pEnv->meter[1].x2 = 0x122;
    pEnv->meter[1].x0 = 0x122;
    pEnv->meter[1].x1 = 0xE3;
    pEnv->meter[1].x3 = 0xDE;
    pEnv->meter[1].y1 = 0x13;
    pEnv->meter[1].y0 = 0x13;
    pEnv->gaugeMirror[1].y0--;
    pEnv->meter[0].x2 = 0x1E;
    pEnv->meter[0].x0 = 0x1E;
    pEnv->meter[1].y3 = 0x1E;
    pEnv->meter[1].y2 = 0x1E;
    pEnv->gaugeMirror[1].x2--;
    pEnv->meter[0].len = 5;
    pEnv->meter[1].len = 5;
    pEnv->meter[2].len = 5;
    Misc10SetRGBC(&pEnv->meter[2], 0xFF, 0, 0, 0x28);
    Misc10SetXY0(&pEnv->meter[2], 6, 0x32);
    Misc10SetXY1(&pEnv->meter[2], 0xA, 0x36);
    Misc10SetXY2(&pEnv->meter[2], 6, 0x4C);
    Misc10SetXY3(&pEnv->meter[2], 0xA, 0x48);
    pEnv->meter[3].len = 5;
    Misc10SetRGBC(&pEnv->meter[3], 0xFF, 0, 0, 0x28);
    Misc10SetXY0(&pEnv->meter[3], 0x13A, 0x32);
    Misc10SetXY1(&pEnv->meter[3], 0x136, 0x36);
    Misc10SetXY2(&pEnv->meter[3], 0x13A, 0x4C);
    Misc10SetXY3(&pEnv->meter[3], 0x136, 0x48);
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80085EC8);





void FieldSoundResetActorEffects(Misc10TagSrc* p, s32 x, s32 y, POLY_FT4* pPoly, s32 mode) {
    s32 scale;
    s32 right;

    switch (mode) {
    case BATTLE_TEX_DEPTH_4BIT:
        scale = 4;
        break;
    case BATTLE_TEX_DEPTH_8BIT:
        scale = 2;
        break;
    case BATTLE_TEX_DEPTH_16BIT:
        scale = 1;
        break;
    }

    setlen(pPoly, 9);
    pPoly->code = 0x2D;
    pPoly->clut = GetClut(p->crect->x, p->crect->y);
    pPoly->tpage = GetTPage(mode, 0, p->prect->x, p->prect->y);
    pPoly->x0 = pPoly->x2 = x;
    right = x + p->prect->w * scale;
    if (scale == 2) {
        pPoly->x3 = right + 1;
    } else {
        pPoly->x3 = right;
    }
    pPoly->y0 = pPoly->y1 = y;
    pPoly->x1 = pPoly->x3 = pPoly->x3;
    pPoly->y2 = pPoly->y3 = y + p->prect->h;
    pPoly->u0 = pPoly->u2 = p->prect->x * scale;
    pPoly->u1 = pPoly->u3 = (p->prect->x + p->prect->w) * scale;
    pPoly->v0 = pPoly->v1 = p->prect->y;
    pPoly->v2 = pPoly->v3 = p->prect->y + p->prect->h;
    pPoly[1] = pPoly[0];
}


void BattleBuildFlippedTexturedQuad(Misc10TagSrc* p, s32 x, s32 y, POLY_FT4* pPoly, s32 mode) {
    s32 scale;

    switch (mode) {
    case BATTLE_TEX_DEPTH_4BIT:
        scale = 4;
        break;
    case BATTLE_TEX_DEPTH_8BIT:
        scale = 2;
        break;
    case BATTLE_TEX_DEPTH_16BIT:
        scale = 1;
        break;
    }

    setlen(pPoly, 9);
    pPoly->code = 0x2D;
    pPoly->clut = GetClut(p->crect->x, p->crect->y);
    pPoly->tpage = GetTPage(mode, 0, p->prect->x, p->prect->y);
    pPoly->x0 = pPoly->x2 = x + p->prect->w * scale;
    pPoly->x1 = pPoly->x3 = x;
    pPoly->u0 = pPoly->u2 = p->prect->x * scale - 1;
    pPoly->u1 = pPoly->u3 = (p->prect->x + p->prect->w) * scale - 1;
    pPoly->y0 = pPoly->y1 = y;
    pPoly->y2 = pPoly->y3 = y + p->prect->h;
    pPoly->v0 = pPoly->v1 = p->prect->y;
    pPoly->v2 = pPoly->v3 = p->prect->y + p->prect->h;
    pPoly[1] = pPoly[0];
}

extern POLY_FT4 D_80095698[16];
extern DR_TPAGE D_80095918[4];
extern u8 D_80091814[];
extern u8 g_BattleBarTexV;
extern u8 D_80092864;

// PARKED 86.50% (1344 bytes, 336 insns against retail's 1348/337). Frame and saved
//   set are retail's exactly (0x50, s0-s3); the residual is the callee-saved hand-out
//   plus ~17 scheduling positions (18 deletes, 17 inserts).
//
// THE -da/-dl DUMP ASKED FOR LAST ROUND IS DONE (gcc-2.7.2-psx cc1 on
//   build/src/battling/main/misc10.c.i with the unit's cc_flags plus -O2 -G8 -da -dl
//   -dg). func_800868E0 has 20 allocnos, processed `118 201 ... 189 198 72 74`, and
//   the dispositions are `72 in 18  74 in 19  76 in 17  88 in 16`: pseudo 72 is the
//   pArchive parameter (3 refs / 106 insns / 12 calls = floor_log2(3)*3*4*10000/106 =
//   1132) and takes $s2, pseudo 74 is the pBar base &D_80095698[8] (2 refs / 244
//   insns / 14 calls = 327) and takes $s3, where retail has the parameter in $s3 and
//   the pointer in $s2. The quotients are 3.5x apart, so retail's map needs 74
//   processed FIRST, i.e. 74's live range under 141 insns - and 74 runs 244 insns
//   only because the tail's `D_80095698[8]` subscript is CSE'd with the one `pBar`
//   use across 14 calls.
//
// MEASURED THIS ROUND, the pair flipped WITHOUT moving the absolute stores: spelling
//   the three tail copies off the pointer (`pBar[2] = pBar[0]; pBar[1] = pBar[0];
//   pBar[3] = pBar[2];`) while the elem10 field stores stay `&D_80095698[10]` gives
//   74 four refs (quotient 1311 > 1132), so 74 takes $s2 and pArchive $s3 - retail's
//   map exactly, 0x58(s3)/0x5C(s3)/0x60(s3) for the archive entries and every elem10
//   store still lui-at plus sh-abs - for 86.35% / 1328 bytes / 332 insns. Five
//   instructions short, because each copy then derives BOTH addresses from the single
//   $s2 base where retail re-forms a separate base per copy (`lui s2,D_80095788` for
//   the elem6 pair, `lui v0,D_80095850` for the elem10 pair) hoisted across the
//   preceding GetClut. Giving every copy its own pointer local is the untried half,
//   and such a local has to be assigned BEFORE the GetClut to be live across it -
//   which is the shape that produced the 244-insn 74 in the first place.
//   Also measured: the tail's elem10 field stores off the pointer too - 85.49% /
//   1316 bytes / 329 insns, retail's pair again but eight short, since every tail
//   address then becomes register-relative and the lui-at forms disappear.
//
// THE ELEM10 POINTER LOCAL IS NOW MEASURED (round 530), and it does buy retail's
//   register map at a byte price. `pCopy = &D_80095698[10]; *pCopy = *pBar;` with the
//   two field stores ALSO off pCopy gives 85.46% / 1328 bytes / 332 insns: the whole
//   prologue comes out retail's (`sw s3 / move s3,a0 / sw ra / sw s2 / sw s1 / sw s0`,
//   pArchive in $s3, &D[8] in $s2, every 0x58/0x5C/0x60 access off $s3), and the two
//   elem10 field stores recover retail's lui-at plus sh/sw-abs form - but the three
//   copy loops then come out 5 instructions short, because $s0/$a2/$a3 get re-used
//   in a different rotation (retail's `addiu a2,s0,-0x50` dest / `move s1,s2` source
//   pair becomes `move a3,s0` / `move a2,s2`). Keeping the field stores as element
//   subscripts (`Misc10SetUV0(&D_80095698[10], ...)`) is 86.00% / 1336 / 334 insns,
//   i.e. the pointer local alone is worth 8 bytes of the 4 the old body is short of.
//   So the residual is now ONE named thing: the dest/source/base register rotation of
//   the three tail copies, at retail's exact prologue and saved set. Best is still the
//   subscript body below at 86.50% / 1344 / 336 (retail 1348 / 337).
//
// Earlier rounds (all re-parked, do not repeat): all-array 55.28, all-pointer 64.58,
//   mixed 71.59, pBar formed late 84.62 (four insns short), a pointer local for the
//   first copy 85.46 (three short), two permuter runs (base 3559 -> 2619) found only
//   more address hoists, and try_flags.sh --sweep is 86.50 under gcc 2.6.0 and 2.7.2
//   at -O2/-O3 in -G4/-G8 with -mcpu=3000/r8000, 53.07 at -O1 and 69.64 under cdk, so
//   the preset is right.
//   Idem facts worth keeping: D_80095698 is POLY_FT4[16] (0x280 bytes, ending exactly
//   at D_80095918, DR_TPAGE[4] of 8 each), every retail offset lands on a POLY_FT4
//   field, the 0x28-byte copies are plain struct assignments (gcc 2.7.2 expands a
//   block move as two 16-byte iterations plus an 8-byte tail), `-0x8000` into the
//   CLUT's STP bit is signed because retail is addiu, and retail forms every copy's
//   addresses in a callee-saved register hoisted across the preceding call.
/*
void func_800868E0(Misc10TextureArchive* pArchive) {
    TIM_IMAGE tim;
    RECT rect;
    s16* pClut;
    POLY_FT4* pBar;

    OpenTIM(pArchive->unk58[0]);
    ReadTIM(&tim);
    pClut = (s16*)tim.caddr;
    pClut[0] = 0;
    pClut[1] = -0x8000;
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    FieldSoundResetActorEffects((Misc10TagSrc*)&tim, 6, 7, &D_80095698[0], 0);
    BattleBuildFlippedTexturedQuad((Misc10TagSrc*)&tim, 0x13A - tim.prect->w * 4, 7, &D_80095698[2], 0);
    SetDrawTPage(&D_80095918[0], 0, 1, GetTPage(1, 0, 0x380, 0x100));
    D_80095918[1] = D_80095918[0];
    SetDrawTPage(&D_80095918[2], 0, 1, GetTPage(0, 0, 0x380, 0x100));
    D_80095918[3] = D_80095918[2];

    pBar = &D_80095698[8];
    setlen(&D_80095698[4], 4);
    D_80095698[4].code = 0x65;
    Misc10SetXY0(&D_80095698[4], 7, 9);
    Misc10SetUV0(&D_80095698[4], 0, 0);
    Misc10SetXY1(&D_80095698[4], 0x16, 0x16);
    D_80095698[4].clut = GetClut(0, 0x1F6);
    D_80095698[5] = D_80095698[4];
    D_80095698[6] = D_80095698[5];
    Misc10SetXY0(&D_80095698[6], 0x123, 0);
    D_80095698[6].u0 = 0x20;
    D_80095698[6].clut = GetClut(0, 0x1F7);
    D_80095698[7] = D_80095698[6];

    OpenTIM(pArchive->unk58[1]);
    ReadTIM(&tim);
    LoadImage(tim.prect, tim.paddr);
    FieldSoundResetActorEffects((Misc10TagSrc*)&tim, 6, 0x20, &D_80095698[12], 0);
    BattleBuildFlippedTexturedQuad((Misc10TagSrc*)&tim, 0x13A - tim.prect->w * 4, 0x20, &D_80095698[14], 0);

    Misc10SetXY1(pBar, 0x40, 8);
    Misc10SetRGBC(&D_80095698[12], 0x80, 0, 0, 0x2C);
    Misc10SetRGBC(&D_80095698[13], 0x80, 0, 0, 0x2C);
    Misc10SetRGBC(&D_80095698[14], 0x80, 0, 0, 0x2C);
    Misc10SetRGBC(&D_80095698[15], 0x80, 0, 0, 0x2C);
    setlen(&D_80095698[12], 9);
    setlen(&D_80095698[13], 9);
    setlen(&D_80095698[14], 9);
    setlen(&D_80095698[15], 9);
    setlen(&D_80095698[8], 4);
    D_80095698[8].code = 0x65;
    Misc10SetUV0(&D_80095698[8], 0x80, 0);
    g_BattleBarTexV = D_80095698[12].v0;
    D_80092864 = D_80095698[14].v0;
    D_80095698[13].clut = D_80095698[0].clut;
    D_80095698[12].clut = D_80095698[0].clut;
    D_80095698[15].clut = D_80095698[0].clut;
    D_80095698[14].clut = D_80095698[0].clut;
    D_80095698[8].clut = GetClut(0x3A0, 0x110);
    Misc10SetXY0(&D_80095698[8], 0x1E, 0xB);
    D_80095698[10] = D_80095698[8];
    Misc10SetUV0(&D_80095698[10], 0x80, 8);
    Misc10SetXY0(&D_80095698[10], 0xE3, 0x15);
    D_80095698[9] = D_80095698[8];
    D_80095698[11] = D_80095698[10];

    setRECT(&rect, 0x3A0, 0x110, 0x10, 1);
    LoadImage(&rect, (u_long*)D_80091814);
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_800868E0);



void BattleAddSharedPrim(void) {
    AddPrim(g_BattleRootOrderingTable, &g_BattleSharedPrimBuffer[g_BattleDrawBufferIndex]);
}

void BattleQueueGaugeBarPrims(u8* pOt, POLY_F4* pBase, s32 value, s32 flag) {
    s16* pTaper;
    s32 edge;

    if (value >= BATTLE_GAUGE_BAR_SEGMENT1_END) {
        BattleSetVertexXForFlipped(flag, &pBase[0].x1, BATTLE_GAUGE_BAR_SEGMENT1_END);
        BattleSetVertexXForFlipped(flag, &pBase[0].x3, BATTLE_GAUGE_BAR_SEGMENT1_END - BATTLE_GAUGE_BAR_SEGMENT1_TAPER);
        AddPrim(pOt, pBase);
    } else {
        BattleSetVertexXForFlipped(flag, &pBase[0].x1, value);
        BattleSetVertexXForFlipped(flag, &pBase[0].x3, value - BATTLE_GAUGE_BAR_SEGMENT1_TAPER);
        AddPrim(pOt, pBase);
        return;
    }
    if (value >= BATTLE_GAUGE_BAR_SEGMENT2_END) {
        BattleSetVertexXForFlipped(flag, &pBase[1].x1, BATTLE_GAUGE_BAR_SEGMENT2_END);
        BattleSetVertexXForFlipped(flag, &pBase[1].x3, 0x40);
        BattleSetVertexYForFlipped(flag, &pBase[1].y3, 0x10);
        AddPrim(pOt, &pBase[1]);
    } else {
        BattleSetVertexXForFlipped(flag, &pBase[1].x1, value);
        pTaper = &pBase[1].x3;
        edge = value - BATTLE_GAUGE_BAR_SEGMENT1_TAPER;
        BattleSetVertexXForFlipped(flag, pTaper, edge - (value - BATTLE_GAUGE_BAR_SEGMENT1_END) / 4);
        BattleSetVertexYForFlipped(flag, &pBase[1].y3, (value - BATTLE_GAUGE_BAR_SEGMENT1_END) / 2 + 8);
        AddPrim(pOt, &pBase[1]);
        return;
    }
    BattleSetVertexXForFlipped(flag, &pBase[2].x1, value);
    BattleSetVertexXForFlipped(flag, &pBase[2].x3, value - BATTLE_GAUGE_BAR_SEGMENT3_TAPER);
    AddPrim(pOt, &pBase[2]);
}

s32 BattleSetGaugeBarPrimColor(s32 arg0, POLY_F4* pPrim) {
    u32 color;

    if (BattleCheckActorBelowBreakThreshold(arg0, 0) != 0) {
        return 0;
    }
    if (BattleCheckActorBelowBreakThreshold(arg0, 1) != 0) {
        color = BATTLE_GAUGE_METER_COLOR_WHITE;
        setlen(pPrim, 5);
    } else {
        color = BATTLE_GAUGE_METER_COLOR_RED;
        setlen(pPrim, 5);
    }
    *(u32*)&pPrim->r0 = color;
    return 1;
}
