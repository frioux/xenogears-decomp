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
extern void func_8008E3CC(void* pOt, s32 arg1, s32 arg2);
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
    // g_BattleRender* globals func_8007A958 feeds to rcos/rsin).
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
    /* 0x4 */ Misc10PartData* unk04;
} Misc10ModelRecord; // Size: 0x8

typedef struct {
    /* 0x0 */ u32 state; // which payload the node carries: 1 a model part, 2 an entry list, 5 a work block (misc13's Misc13Node, whose BattleSetNode* helpers set it)
    /* 0x4 */ Misc10ModelRecord* pModel;
} Misc10ModelNode; // Size: 0x8

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
    /* 0x284 */ u32* pDisplayWork;
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
    /* 0x90C */ s16 actionAngle; // latched when actionPhase 2 starts, copied into `angle` each frame (misc3's func_80078194)
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
    /* 0x0 */ u32 unk00;
    /* 0x4 */ u8* pMessageStrings;
    /* 0x8 */ Misc10ActionRow* pActionRows;
} Misc10ArchiveHeader; // Size: 0xC

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
extern u8 D_8005061D;
extern u8 D_8005061E;
extern u8 D_8005061F;
extern u8 D_80050620;
extern u8 D_80050621;
extern s32 D_80092798;
extern s32 D_8009279C;
extern void* D_800927B8;
extern s32 D_800928B0;
extern void* D_80097070;
extern void* D_8009878C;
extern s32 D_80097018;

extern void* BattleCreateOrderingTablePair(s32 count);
extern void* BattleCreateLightGroup(void* pParent);
extern void ModelPartResolvePointers(void* pModelPart);
extern void func_80029AFC(void* pQueue, s32 arg1, s32 arg2);
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
extern void func_8007B388(void* pArchive);
extern void BattleLoadTextTimImages(void* pArchive);
extern void func_800878DC(void* pArchive);
extern void func_800868E0(u8* pArchive);
extern void func_80071794(u8* pArchive);
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
extern void func_8007A344(void* pActor1, void* pActor0);
extern void func_8007AE10(void* pActor1, void* pActor0);
extern void func_800840CC(void* pScene);
extern void func_800846A0(void* pScene);
extern void func_80072858(void* pScene);
extern void func_80084A40(void* pScene);
extern void func_80071AD0(void);
extern void func_80083310(s32 arg);
extern void SetGeomScreen(long distance);
extern void FontSetColor(s32 red, s32 green, s32 blue);
extern void FontSetPositionsByOffsetClamped(s32 offsetX, s32 offsetY);
extern void BattleSetScreenTileBlack(void);
extern void BattleSetScreenTileGrey(void);
extern void BattleClearAndQueueActorPrims(void* pScene);
extern char BattleResetForNextRound(void);
extern void func_8007B270(u8* pColor0, u8* pColor1);
extern void func_80084C88(Misc10Actor* pActor, u8* pData, s32 arg2);

extern void BattleSetTpageAndClut(s16 x, s16 y, s16 clutX, s16 clutY);
extern void BattleClearTpageAndClut(void);
extern void* BattleRelocateBlockPointers(void* pData);
extern u8* func_8008B38C(u8* pArg);
extern u8* BattleSetActorSoundTable(Misc10Actor* pActor);

s32 BattleQueueActorPrims(Misc10Actor* pActor) {
    u32* pData = pActor->pDisplayWork;

    func_8008E3CC((u8*)pData[g_BattleDrawBufferIndex + 1] + 8, 0xC0, 0);
    BattleDrawTextPrims((u8*)pData[g_BattleDrawBufferIndex + 1] + 4);
    BattleAttachActorPrims(pData);
    BattleAddSharedPrim();
    return 0;
}

void BattleDrawEffectFrame(void) {
    if (g_BattleEffectFrameTimer != 0) {
        func_8008E120();
        func_8007F258(g_BattleRootOrderingTable, 0);
        BattleDrawTextPrims(g_BattleRootOrderingTable);
        func_8008E3CC(g_BattleRootOrderingTable, 0xC0, 1);
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
            func_8008E3CC(g_BattleRootOrderingTable, timer, 1);
        }
    } else {
        g_BattleEffectImageActive = 0;
    }
}

void BattleActorCreateRotatedPart(Misc10Actor* pActor) {
    void* pPartList;
    Misc10Draw* pDraw;
    u8* pBuf;

    pPartList = pActor->pModelLink->pModel->unk04->pPartList;
    pDraw = BattleCreateObject();
    pBuf = BattleCreateModelPart();
    BattleSetNodeModelPart(pDraw, pBuf);
    BattleAttachModelPart(pBuf, g_BattleRotatedPartModel);
    BattleAppendListEntry(pPartList, pDraw);
    pDraw->rotY = BATTLE_ANGLE_THREE_QUARTER_TURN;
    pDraw->rotX = 0;
    pDraw->rotZ = BATTLE_ANGLE_QUARTER_TURN;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80084C88);

/* 89.37% objdiff, 832 bytes against retail 840. Structure, call order, the
   prologue save order and every s-register except one pair match, and the
   jump table comes out at the unit rodata start where retail has it. Because
   the stub owns a jtbl_ table, this function can never print MATCH in
   diff_func.sh (the original dump stops at the first case label, which gas
   makes a symbol when .rodata references it) - objdiff% and check_bin.sh are
   the only judges.
   What is still wrong:
   1. Best body below keeps pActor->pModelLink = pNode AFTER the switch, which is
      what makes gcc hand pActor s0 / pNode s1 (global-alloc priority order
      pActor, pData, arg2, pNode, with s1 already reserved by the flip-loop
      buffer through local-alloc). Moving that store to its retail position
      (middle block, before the pActionRow store) makes pNode block-local, so
      local-alloc takes s0 for it and pActor falls to s1 - the whole function
      then reads 86% with s0/s1 swapped. That trade-off is the whole gap.
   2. pActor->typeFlags |= 1; pActor->typeFlags |= 2; (retail case 35/36) is cse-d
      into one |= 3 by gcc; retail has two lbu/ori/sb pairs. Untried:
      bitfields, or a union member for that byte.
   3. gcc cross-jumps the three case-body sb v0,0x90A into one shared tail;
      retail has one per case (2 instructions, the size difference).
   Key trick that fixed the prologue: assign pSrc = pData->pImage + 0x200
   BEFORE HeapAlloc in the arg2 == 0 branch - retail keeps it in s0 across
   the HeapAlloc call (its addiu lands in the call delay slot), which is what
   pushes the heap buffer to s1 and pData/arg2 to s2/s3.
   void func_80084C88(Misc10Actor* pActor, Misc10Block* pData, s32 arg2) {
       RECT rect;
       Misc10SpriteHeader* pHeader;
       Misc10ActionRow* pRow;
       u8* pNode;
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
       pNode = func_8008B38C((u8*)pData);
       pObj = (u8*)BattleCreateObject();
       pActor->pObject = pObj;
       BattleAppendListEntry(pObj, pNode);
       pActor->pActionRow = &D_80092874[pActor->typeId];
       pActor->typeFlags = 0;
       switch (pActor->typeId) {
       case 3:
       case 14:
       case 27:
       case 34:
       case 39:
       case 41:
       case 42:
           pActor->typeFlags |= 4;
           break;
       case 13:
           BattleClearTpageAndClut();
           BattleActorCreateRotatedPart(pActor);
           break;
       case 35:
       case 36:
           pActor->typeFlags |= 1;
           pActor->typeFlags |= 2;
           break;
       case 38:
           pActor->typeFlags |= 2;
           break;
       }
       pActor->pModelLink = pNode;
       BattleSetActorSoundTable(pActor);
       pHeader = pData->pHeader;
       pActor->pScript = (u8*)pHeader;
       pActor->pPartTimings = pData->pPartTimings;
       pActor->pModelScriptOffsets = (s16*)&pHeader[1];
       pActor->pActions = pData->pActions;
       pActor->pVisiblePartIndices = pActor->pScript + pHeader->visiblePartIndicesOffset;
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
   }
*/

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

/*
   88.42% objdiff, 2912 bytes against retail 2928 (four instructions short). This is
   the battle scene's init and main loop: it allocates the grid and the ordering
   table, loads the archive entries, then runs the frame loop whose switch on
   g_BattleSavedScreenMode is the jtbl_80070260 table this stub owns. The body below
   is a complete transcription.

   SOLVED: the frame and the register count are retail's now (0x28, five saved
   registers s0-s4 in retail's exact slots). The one live value that had to go was
   in the done block: `pActor1 = &g_BattleActor1;' and `pActor0 = &g_BattleActor0;'
   there made gcc hoist both la's to the head of the block, so they overlapped
   pBuf's live range and the function needed a sixth saved register (s5, frame
   0x30, every s-number one off). Passing &g_BattleActor1 / &g_BattleActor0
   directly to the two func_80084C88 calls and the &g_BattleActor1.effectColorR /
   &g_BattleActor0.effectColorR pair to func_8007B270 fixed the frame in one edit
   (2896 -> 2912). The actors are still assigned at newRound for the frame loop.
   Retail does the same: its case 1 and case 5 bodies pass pActor1/pActor0 (the
   live s1/s0) while cases 4 and 8 write &g_BattleActor1 / &g_BattleActor0 out in
   full, so the switch arms below spell those two cases with the globals, which is
   worth 4 instructions. FontLoadClut's second parameter is u_short, not short:
   retail passes 0x8000 as ori, a short declaration gives addiu -0x8000.

   RESIDUAL, in order of size:
   1. newRound's three pointer assignments are the missing four instructions.
   Retail forms them `lui v0,%hi(sym) / addiu v0,v0,%lo / addu s1,v0,zero' for
   g_BattleActor1 and g_BattleActor0 (a copy through v0) and only the
   g_BattleMatchCount one as the plain two-instruction la into s3; this body gets
   the plain form for all three, and gcc also emits matchCount first. Casts,
   declaration order and a separate local were measured, no change.
   2. The screenRelease block (15 instructions, `g_C1ButtonStateReleased = 0;' ...
   `goto reinit') sits at the very end of this body; retail places it at
   0x8008548C, between the audio-manager if's then-block jump and its ELSE block.
   Both paths jump over it, so no source statement order reproduces it: the label
   position it needs is one gcc does not emit for an if/else.
   3. Two block-placement differences of the same family (2 instructions each):
   retail emits `beqz $s0,A / beq $s0,1,B / j merge' for the state == 0 /
   state == 1 test where this body emits an inverted bne pair with both bodies
   inline, and retail's D_8005061D switch arms (beq a0,s3 / slti / beqz) are all
   out of line behind a j where ours fall through into case 0's body. Nesting the
   if as `if (state != 0) { if (state == 1) ... } else ...' was measured: 82.91%.
   4. Allocator tie, no bytes: retail has pScene in s2 and the ~1 mask / state
   constant / &g_BattleMatchCount chain in s3; this body has them the other way
   round. Declaration order, pScene's type and the seds/pBuf statement order were
   measured, no change.
   5. Init order: retail stores g_BattleEffectImageActive = NULL before the
   li a0,0x7FFF for FontLoadClut (ours hoists the li), and loads
   D_800917C0[0].buffer into s0, the register pRes just freed, where this body
   keeps it in s1, the register the done block's pBuf also uses. One pseudo with
   two defs in different blocks: retail behaves as if the two pBuf uses were
   separate locals.

   s32 func_800852C4(void) {
       Misc10ArchiveHeader* pArchive;
       Misc10Actor* pActor1;
       Misc10Actor* pActor0;
       Misc10Resource* pRes;
       u8* pMatchCount;
       u8* pBuf;
       void* pScene;
       s32 state;
       s32 idleTimer;
       s32 idleCount;
       s32 count;

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
       func_80029AFC(pRes, 0, 0);
       g_BattleSedsEntryId = D_800917C0[1].buffer;
       pBuf = (u8*)D_800917C0[0].buffer;
       BattleSetScreenResolution(0x140, 0xDA);
       BattleClearLineSlots();
       func_80030988(1, 1, 0x40, 0x40);
       ControllerSetFrameCallback(BattleUpdateEffectImage);
       g_BattleEffectImageActive = NULL;
       FontLoadClut(0x7FFF, 0x8000);
       SystemTransferPaletteToVRAM(0x140, 0xFF);
       BattleInitTextLists();
       BattleInitOptionDefaults();
       BattleEffectClearResourcePointers();
       D_800927B8 = NULL;
       g_BattleSlotEntryBuffers[0] = NULL;
       D_80097070 = NULL;
       D_8009878C = NULL;
       BattleFreeScreenBuffer(1);
       ArchiveCdDataSync(0);
       SoundAddSedsEntry(g_BattleSedsEntryId);
       if (g_BattleAudioManagerEnabled != 0) {
           g_BattlingAudioManager = SoundCreateAudioManagerOnHeap(pBuf);
           SoundPlayAudioManager(g_BattlingAudioManager, 0x7F, 0);
       } else {
           g_BattlingAudioManager = g_SoundAudioManager;
       }
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
       func_8007B388(pArchive);
       BattleLoadTextTimImages(pArchive);
       func_800878DC(pArchive);
       func_800868E0((u8*)pArchive);
       func_80071794((u8*)pArchive);
       HeapFree(pArchive);
       BattleInitSpriteRectTable();
       BattleInitMessageWindow();
       func_8008BC04();
       g_BattleMultiplyNodeMatrices = 0;

   reinit:
       HeapFreeAllDelayedBlocks();
       idleTimer = BATTLE_IDLE_TIMEOUT;
       BattleFreeNodeTree(D_8009878C);
       BattleFreeNodeTree(D_80097070);
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
       g_BattleDifficultyLevel = D_80050621;
       g_BattleMatchCount = D_80050620;
       g_BattleTransitionFlag = 0;
       BattleFreeTextArchive();
       D_80092798 = D_8005061E;
       D_8009279C = D_8005061F;
       g_BattlePlayer1ComControlled = 0;
       switch (D_8005061D) {
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
       case 0:
           g_BattlePlayer2ComControlled = 1;
           BattleSetScreenMode(1);
           g_BattleMode = 1;
           BattleLoadSelectionTextures(D_80092798, D_8009279C);
           if (D_8009279C == 5) {
               g_BattleStageLightIndex = 1;
           }
           break;
       }
       state = 1;
       BattleLoadSlotEntry(0, D_80092798);
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
           if (state == 0) {
               BattleUpdateTextWindow();
               if (g_BattleStateAdvanceRequested != 0) {
                   g_BattleFrameWaitCount = 2;
                   state = 1;
               }
               if ((g_BattleDrawFlags & 1) != 0) {
                   func_8008E120();
               }
           } else if (state == 1) {
               ControllerMergeStates();
               if (BattleCheckModeOver() == 0) {
                   func_8008E120();
                   func_8007F258(g_BattleRootOrderingTable, 0);
               }
               if (ArchiveCdDataSync(1) == 0) {
                   state = 10;
               }
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
       func_80084C88(&g_BattleActor1, g_BattleSlotEntryBuffers[0], 0);
       BattleDrawEffectFrame();
       func_80084C88(&g_BattleActor0, D_800927B8, 1);
       BattleDrawEffectFrame();
       func_8007B270(&g_BattleActor1.effectColorR, &g_BattleActor0.effectColorR);
       BattleSetScreenTileGrey();
       if (g_BattleEffectFrameTimer != 0) {
           g_BattleEffectImageActive = (u8*)1;
           SoundSetAudioManagerMasterVolumeWithFade(g_BattlingAudioManager, 0x158, 0);
           g_BattleMotionSpeedScale = 0x200;
       }
       g_BattleResultSide = 2;

   newRound:
       BattleResetForNextRound();
       pActor1 = &g_BattleActor1;
       pActor0 = &g_BattleActor0;
       pMatchCount = &g_BattleMatchCount;
       pActor1->position.x = 0x3E80;
       g_BattleSceneOriginYMirror = 0;
       g_BattleSceneOriginZ = 0x3F80;
       pActor0->position.x = 0x4080;
       g_BattleSceneOriginY = 0;
       D_80097018 = 0x3F80;
       for (;;) {
           FontSetColor(0, 0xFF, 0);
           g_BattleFrameWaitCount = 2;
           D_800928B0 = 0;
           g_BattleSavedScreenMode = g_BattleScreenMode;
           BattleUpdateEffectTimer();
           func_80034888(g_BattleMessageWindow, g_BattleRootOrderingTable, g_BattleDrawBufferIndex);
           switch (g_BattleSavedScreenMode) {
           case 1:
               func_80079DF0(pActor1, pActor0);
               SetGeomScreen(0xC0);
               if (g_BattleScreenMode == 1) {
                   func_800796B8(pActor1, pActor0);
               }
               goto commonDraw;
           case 6:
               func_80072858(pScene);
               break;
           case 7:
               func_80071AD0();
               SetGeomScreen(0xC0);
               func_800840CC(pScene);
               if (g_BattleScreenMode == 3) {
                   g_BattleFrameWaitCount = 0;
                   func_8008BC04();
                   goto case3;
               }
               break;
           case 4:
               SetGeomScreen(0x800);
               func_8007A344(&g_BattleActor1, &g_BattleActor0);
               if (g_BattleScreenMode != 8) {
                   func_80083310(1);
                   D_800928B0 = 1;
               }
               goto commonDraw;
           case 8:
               SetGeomScreen(0x200);
               func_8007AE10(&g_BattleActor1, &g_BattleActor0);
           commonDraw:
               func_800840CC(pScene);
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
               func_80079DF0(pActor1, pActor0);
               SetGeomScreen(0xC0);
               func_800796B8(pActor1, pActor0);
               func_800846A0(pScene);
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

   screenRelease:
       g_C1ButtonStateReleased = 0;
       g_C1ButtonStatePressedOnce = 0;
       BattleSetBothSidesUser();
       g_BattleFrameWaitCount = 0;
       BattleSetScreenTileBlack();
       func_8008BC04();
       goto reinit;
   }
*/

/*

/* 85.76% objdiff, 2896 bytes against retail 2928 (741 instructions). This is the
   battle scene's init and main loop: it allocates the grid and the ordering
   table, loads the archive entries, then runs the frame loop whose switch on
   g_BattleSavedScreenMode is the jtbl_80070260 table this stub owns. The body
   below is a full transcription, and the instruction stream is very close: the
   prologue, the five BattleAllocResourceBuffer calls, the two archive decompress
   blocks, the reinit block, the D_8005061D mode switch and the whole frame-loop
   head are instruction-identical apart from register choice and one jump-table
   operand. What is left, per phase (instructions, retail vs this body):

     phase                                  retail   ours
     init + resource alloc + audio           123*     125
     LZSSDecompress block (g_BattleGridCells)    9         9
     first LZSSHeapDecompress block             21        21
     reinit block (.L800855D8)                  30        30
     state init (.L8008564C)                    21        21
     D_8005061D mode switch + slot load        106       106
     main while loop (idle timer + dispatch)    99        97
     done block + two func_80084C88 calls       62        64
     BattleResetForNextRound + actor origins    36        34
     frame loop head (.L80085B48)               44        42
     the 9-case switch on g_BattleSavedScreenMode 157     142
     frame loop tail + screenRelease block      33        33
                                              ----      ----
                                              741       724

   * retail's 138 includes the 15-instruction screenRelease block, which retail
   places at 0x8008548C (between the audio-manager if and .L800854D4); this body
   puts it at the end, so it lands in the tail instead. That is the whole reason
   the first and last rows are 15 apart; excluding it, the init phase is 2 short.

   1. THE REGISTER COUNT. This body saves six callee-saved registers (s0-s5,
   frame 0x30) where retail saves five (s0-s4, frame 0x28, and retail's frame
   leaves 0x10 bytes of its local area unwritten). So one local here has a live
   range retail's source did not have, and every s-register number differs
   because of it. Retail's allocation, which is a clean disjoint-range chain:
   s2 = the scene object for the whole function; s0 = pRes, then the decompressed
   buffer, then &g_BattleActor0; s1 = the idle timer, then &g_BattleActor1 (their
   ranges are disjoint, the timer dies at the `done' block); s3 = the &~1 button
   mask, then &g_BattleMatchCount; s4 = the idle counter. Once the extra live
   value is gone the frame, the save order and all of these should fall into
   retail's slots.
   2. THE SWITCH (157 vs 142 instructions) is the biggest single gap and has not
   been analysed case by case. Retail's 15 extra instructions are in the arms,
   not in the dispatch: the dispatch, the sltiu bound check, the .rodata jtbl and
   the `li a0,0x9E` in every break's delay slot all reproduce exactly (the
   jtbl_80070260-vs-.rodata operand is expected in a unit that owns its tables).
   Note retail's case bodies are physically ordered 1, 6, 7, 4, 8, 0, 5, 3, 2,
   which is the order they are listed in here, and that case 1 jumps into the
   middle of case 8 (a shared `commonDraw' label), case 7 jumps to case 3's body
   and case 2 jumps there too - those three gotos are real, they are not
   cross-jumping the compiler did: no plain `break' can produce them.
   3. Two gcc-2.7.2 block-placement differences: this body hoists `move a0` for
   the first call of a case above the `jr v0' dispatch (retail sets it at the
   start of the arm), and it forms &g_BattleActor1 / &g_BattleActor0 at the top
   of the `done' block instead of immediately before each func_80084C88 call as
   retail does. Both are 1-2 instructions and neither has been tried.
   4. The five resource words at D_800917C0 are read as an array here. Retail
   takes the address of the D_800917DC word into a register and loads through it
   twice (LZSSDecompress, then HeapFree) while it forms D_800917D4 and D_800917E4
   as direct lui + lw pairs - a source pointer local for the first one only. That
   is why pRes is spelled D_800917C0[k] from the reads on: keeping pRes live past
   func_80029AFC is what forced the sixth saved register in the first place.
   The reading of D_8005061D: retail's compare tree is `beq 1 / slti 2 / beq 0'
   and its arms store s0, which holds 2 there and is overwritten with 1 in the
   case-0 path - that is the `state' local, whose values are 0 (title), 1
   (running), 2 (neither) and 10 (exit).
   The idle timer is 0x4650 frames (BATTLE_IDLE_TIMEOUT) reset on any button
   outside bit 0, and 0x78 frames (BATTLE_IDLE_HOLD_COUNT) of C1 held on the
   default text list zero it immediately, which is what hands both sides to the
   COM.
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

void BattleSetVertexXForFlipped(s32 flag, u16* pDst, s32 value) {
    if (flag != 0) {
        *pDst = 0xDA - value;
    } else {
        *pDst = 0x67 + value;
    }
}

void BattleSetVertexYForFlipped(s32 flag, u16* pDst, s32 value) {
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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80085EC8);

/* 95.39%, retail's exact 1516 bytes, 6 shapes. Everything in the body is
   instruction-for-instruction retail except register naming: the D_800917F4
   offset bases get s1/s2/s3 where retail has s3/s1/s2 (+0xC is the first
   defined and retail gives it s3), the meter[2]/meter[3] constant block is
   hoisted in retail's order but into different registers, and the gauge loop
   hoists the src.x0 load above src.y0/y1 where retail leaves it third. The
   loop shape that got the exact size was gauge[3] + gaugeMirror[3] (one biv,
   offsets 576/636 off pEnv); gauge[6] with gauge[i+3] makes a second giv
   (+4 bytes) and a walking pointer re-bases onto pEnv+0x284 (+0 bytes).
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
    BattleCopyVertexOffsetAdd(&D_800917F4[6], &pEnv->barPrim[0].x1);
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
        pEnv->gaugeMirror[i].x0 = 0x140 - pEnv->gauge[i].x0;
        pEnv->gaugeMirror[i].y0 = pEnv->gauge[i].y0;
        pEnv->gauge[i].len = 4;
        Misc10SetRGBC(&pEnv->gauge[i], 0, 0xFF, 0, 0x20);
        pEnv->gaugeMirror[i].len = 4;
        Misc10SetRGBC(&pEnv->gaugeMirror[i], 0, 0xFF, 0, 0x20);
        pEnv->gaugeMirror[i].y1 = pEnv->gauge[i].y1;
        pEnv->gaugeMirror[i].y2 = pEnv->gauge[i].y2;
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
}
*/

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

    ((u8*)pPoly)[3] = 9;
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

    ((u8*)pPoly)[3] = 9;
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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_800868E0);

/* 71.59% (1228 bytes against retail 1348), 3 attempts, parked for time.
   The unit's data at D_80095698 is POLY_FT4[16] (0x280 bytes, ending exactly at
   D_80095918) followed by DR_TPAGE[4]; every retail offset lands on a POLY_FT4
   field (+3 len, +4 r0/g0/b0/code, +8 x0/y0, +0xC u0/v0, +0xE clut, +0x10
   x1/y1), so the packets are POLY_FT4s. The word/halfword stores retail has
   (sw 0x090007 at +8, sh 0 at +0xC, sw 0x160016 at +0x10, sw 0x2C000080 at +4)
   are the setXY0Fast/setUV0Fast family from include/system/font.h: gcc 2.7.2
   does NOT merge adjacent short/byte stores, so they must be written as word
   stores (the macros above). This body has all the values, both TIM sections,
   the SetDrawTPage/DR_TPAGE pairs, the three GetClut calls, both blocks of
   sprite setups and all five 0x28-byte copies in retail order.
   What is left: (1) about 30 instructions of register/loop-setup difference -
   retail keeps the POLY base in s1 and forms the copy pointers base-relative
   (addiu s2,s1,0xA0 / addiu t0,s1,0xF0) while keeping the scalar field stores
   ABSOLUTE (lui $at,%hi/%lo), and its copies do not re-materialise the base per
   copy; (2) retail's frame is 0x50 with s3-s0 saved (pActor in s3), ours 0x48
   with s2-s0, i.e. we are one callee-saved register short; (3) the two
   g_BattleBarTexV/D_80092864 lbu's sit earlier in retail than the stores.
   Tried: all-array-symbol accesses (55.28%, copies also go absolute), all
   pointer-local accesses (64.58%, scalar stores then go base-relative).
   The form below is the mixed one.
   void func_800868E0(Misc10Actor* pActor) {
    TIM_IMAGE tim;
    RECT rect;
    POLY_FT4* p;
    u16* pClut;

    p = D_80095698;
    OpenTIM((u_long*)pActor->pTimData);
    ReadTIM(&tim);
    pClut = (u16*)tim.caddr;
    pClut[0] = 0;
    pClut[1] = 0x8000;
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    p[5] = p[4];
    FieldSoundResetActorEffects((Misc10TagSrc*)&tim, 6, 7, &p[0], 0);
    BattleBuildFlippedTexturedQuad((Misc10TagSrc*)&tim, 0x13A - tim.prect->w * 4, 7, &p[2], 0);
    SetDrawTPage(&D_80095918[0], 0, 1, GetTPage(1, 0, 0x380, 0x100));
    D_80095918[1] = D_80095918[0];
    SetDrawTPage(&D_80095918[2], 0, 1, GetTPage(0, 0, 0x380, 0x100));
    D_80095918[3] = D_80095918[2];
    setlen(&D_80095698[4], 4);
    D_80095698[4].code = 0x65;
    Misc10SetXY0(&D_80095698[4], 7, 9);
    Misc10SetUV0(&D_80095698[4], 0, 0);
    Misc10SetXY1(&D_80095698[4], 0x16, 0x16);
    D_80095698[4].clut = GetClut(0, 0x1F6);
    p[6] = p[5];
    Misc10SetXY0(&D_80095698[6], 0x123, 0);
    D_80095698[6].u0 = 0x20;
    D_80095698[6].clut = GetClut(0, 0x1F7);
    OpenTIM((u_long*)pActor->pModelLink);
    ReadTIM(&tim);
    LoadImage(tim.prect, tim.paddr);
    FieldSoundResetActorEffects((Misc10TagSrc*)&tim, 6, 0x20, &p[12], 0);
    BattleBuildFlippedTexturedQuad((Misc10TagSrc*)&tim, 0x13A - tim.prect->w * 4, 0x20, &p[14], 0);
    Misc10SetXY1(&D_80095698[8], 0x40, 8);
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
    p[10] = p[8];
    Misc10SetUV0(&D_80095698[10], 0x80, 8);
    Misc10SetXY0(&D_80095698[10], 0xE3, 0x15);
    p[9] = p[8];
    p[11] = p[10];
    setRECT(&rect, 0x3A0, 0x110, 0x10, 1);
    LoadImage(&rect, (u_long*)D_80091814);
}
*/
void BattleAddSharedPrim(void) {
    AddPrim(g_BattleRootOrderingTable, &g_BattleSharedPrimBuffer[g_BattleDrawBufferIndex]);
}

void BattleQueueGaugeBarPrims(u8* pOt, u8* pBase, s32 value, s32 flag) {
    u16* pTaper;
    s32 edge;

    if (value >= BATTLE_GAUGE_BAR_SEGMENT1_END) {
        BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0xC), BATTLE_GAUGE_BAR_SEGMENT1_END);
        BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x14), 0x34);
        AddPrim(pOt, pBase);
    } else {
        BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0xC), value);
        BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x14), value - 4);
        AddPrim(pOt, pBase);
        return;
    }
    if (value >= BATTLE_GAUGE_BAR_SEGMENT2_END) {
        BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x24), BATTLE_GAUGE_BAR_SEGMENT2_END);
        BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x2C), 0x40);
        BattleSetVertexYForFlipped(flag, (u16*)(pBase + 0x2E), 0x10);
        AddPrim(pOt, pBase + 0x18);
    } else {
        BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x24), value);
        pTaper = (u16*)(pBase + 0x2C);
        edge = value - 4;
        BattleSetVertexXForFlipped(flag, pTaper, edge - (value - BATTLE_GAUGE_BAR_SEGMENT1_END) / 4);
        BattleSetVertexYForFlipped(flag, (u16*)(pBase + 0x2E), (value - BATTLE_GAUGE_BAR_SEGMENT1_END) / 2 + 8);
        AddPrim(pOt, pBase + 0x18);
        return;
    }
    BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x3C), value);
    BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x44), value - 8);
    AddPrim(pOt, pBase + 0x30);
}

s32 BattleSetGaugeBarPrimColor(s32 arg0, u8* pOut) {
    s32 value;
    s32* pColor;

    if (BattleCheckActorBelowBreakThreshold(arg0, 0) != 0) {
        return 0;
    }
    if (BattleCheckActorBelowBreakThreshold(arg0, 1) != 0) {
        value = 0x2800FFFF;
        pOut[3] = 5;
    } else {
        value = 0x280000FF;
        pOut[3] = 5;
    }
    pColor = (s32*)(pOut + 4);
    *pColor = value;
    return 1;
}
