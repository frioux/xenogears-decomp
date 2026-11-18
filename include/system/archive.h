#ifndef _XENO_ARCHIVE_H
#define _XENO_ARCHIVE_H

#include "psyq/libcd.h"

#define ARCHIVE_DIR_CORE 0x0
#define ARCHIVE_DIR_FIELD 0x4
#define ARCHIVE_DIR_MENUS 0x10
#define ARCHIVE_DIR_MOVIES 0x18
#define ARCHIVE_DIR_FIELD_WDS 0x1C
#define ARCHIVE_DIR_BATTLE_SOUND 0x2C

#define CD_SECTOR_SIZE 0x800
#define RETRY_COUNT 0x4

#define STREAM_FILE_HEADER_SIZE 0x24

#define ARCHIVE_HEADER_ENTRY_SIZE 0x7
#define ARCHIVE_ENTRY_NAME_BUFFER_SIZE 0x40
#define ARCHIVE_MAX_SECTIONS 0x40

#define ARCHIVE_CD_DRIVE_IDLE 0x0
#define ARCHIVE_CD_DRIVE_READ_SECTOR 0x1
#define ARCHIVE_CD_DRIVE_READ_DONE 0x2
#define ARCHIVE_CD_DRIVE_SEEK 0x3
#define ARCHIVE_CD_DRIVE_SEEK_DONE 0x4
#define ARCHIVE_CD_DRIVE_DONE 0x5
#define ARCHIVE_CD_DRIVE_READ_DATA_SECTOR 0x6
#define ARCHIVE_CD_DRIVE_HALT 0x7
#define ARCHIVE_CD_DRIVE_READ_ADPCM_SECTOR 0x8
#define ARCHIVE_CD_DRIVE_RESET_MODE 0x9
#define ARCHIVE_CD_DRIVE_ERROR 0xA
#define ARCHIVE_CD_DRIVE_HANDLE_ERROR 0xB
#define ARCHIVE_CD_DRIVE_SET_ADPCM_PLAY_CHANNEL 0xC

#define ARCHIVE_CD_DRIVE_ERR_SEEK 0x1
#define ARCHIVE_CD_DRIVE_ERR_GENERIC 0x2
#define ARCHIVE_CD_DRIVE_ERR_READ 0x3
#define ARCHIVE_CD_DRIVE_ERR_HALT 0x4
#define ARCHIVE_CD_DRIVE_ERR_ADPCM 0x5
#define ARCHIVE_CD_DRIVE_ERR_RESET_MODE 0x6

#define ARCHIVE_STREAM_FILE_NOT_LOADED 0x0

typedef struct {
    u_short state;
    u_short id;
    u_short size; // In # file sectors
    u_short unk2;
} ArchiveStreamFileSectionHeader;

typedef struct {
    /* 0x0 */ short archiveIndex;
    /* 0x2 */ u16 _pad;
    /* 0x4 */ void* pData;
} StreamDataQueueEntry;

typedef struct {
    // the length of this array is unknown because we've lost the context giving the number of entries
    u8 *entries[1];
} SystemDataEntries;

// indices into SystemDataEntries.entries[]: which string/name table a lookup reads
#define SYSTEM_STRING_BANK_ACCESSORY 0x11
#define SYSTEM_STRING_BANK_ITEM 0x16
#define SYSTEM_STRING_BANK_WEAPON 0x17
#define SYSTEM_STRING_BANK_GLYPH 0x1B
#define SYSTEM_STRING_BANK_GEAR_ACCESSORY 0x32
#define SYSTEM_STRING_BANK_GEAR_WEAPON 51

extern ArchiveStreamFileSectionHeader* g_ArchiveStreamSections;
extern int g_ArchiveStreamSectionCount;

extern u32 g_ArchiveDebugTable;
extern u32 g_ArchiveHeader;
extern u32 g_ArchiveTable;
extern u32 g_CurArchiveOffset;
extern int g_ArchiveHeaderOffsetBase;
extern u_int g_ArchiveCdDriveState;
extern u_int g_ArchiveCdDriveError;
extern void* g_ArchiveCurStreamFile;
extern CdlLOC g_ArchiveCdCurLocation;
extern int g_ArchiveCurFileSize;

extern s32 g_ArchivePendingReadCount;
extern s32 g_ArchiveDebugFileHandle;
extern u_char* D_80059F1C;
extern s32 D_8005A49C;
extern s32 g_ArchiveCdReadRetryExhaustedCount;

extern u32 g_ArchiveCdReadStartCount;
extern u32 g_ArchiveCdReadCompleteCount;
extern u32 g_ArchiveCdReadStatusErrorCount;
extern u32 g_ArchiveCdDataSectorCompleteCount;
extern u32 g_ArchiveCdDataSectorErrorCount;
extern u32 g_ArchiveCdHaltCompleteCount;
extern u32 g_ArchiveCdHaltErrorCount;
extern void* g_ArchiveSavedReadyCallback;
extern char g_ArchiveCdFilter[]; // CdlFilter
extern u_char g_ArchiveCdMode[]; // u_char* - mode
extern u8* D_80059F1B;
extern int g_ArchiveStreamStopFlag;
extern int g_ArchiveCdChannel;
extern char D_80059F15;

int ArchiveSetIndex(int directoryIndex, int entryIndex);
int ArchiveGetArchiveOffsetIndices(int* alignedIndex, int* remainder);
int ArchiveDecodeSize(int entryIndex);
char* ArchiveGetFilePath(int entryIndex);
int ArchiveDecodeSector(int entryIndex);
void ArchiveCdSeekToFile(int entryIndex);
void ArchiveCdDriveCommandHandler(u_char status, u_char* pResult);

#endif