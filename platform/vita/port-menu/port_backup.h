/* SPDX-License-Identifier: MIT */
#ifndef PORT_BACKUP_H
#define PORT_BACKUP_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define PB_SLOTS 3
typedef struct {const char *path,*prefix;size_t size;bool (*validate)(const char *path);} PB_Store;
typedef struct {bool valid;uint32_t generation;uint64_t timestamp;} PB_Info;
void PB_List(const PB_Store *,PB_Info entries[PB_SLOTS],unsigned slots[PB_SLOTS]);
bool PB_PreviousInfo(const PB_Store *,PB_Info *);
bool PB_RestorePrevious(const PB_Store *);
void PB_Date(uint64_t timestamp,char *out,size_t size);
bool PB_InfoAt(const PB_Store *,unsigned slot,PB_Info *);
bool PB_Capture(const PB_Store *); /* De-duplicates newest identical payload. */
bool PB_Restore(const PB_Store *,unsigned slot); /* Caller must exit/reload, no stale RAM writes. */
bool PB_Recover(const PB_Store *); /* Before opening the game's save subsystem. */
#endif
