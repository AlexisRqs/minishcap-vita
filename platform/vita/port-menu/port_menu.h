/* SPDX-License-Identifier: MIT */
#ifndef PORT_MENU_H
#define PORT_MENU_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define PM_API_VERSION 2
#define PM_MAX_OPTIONS 32
#define PM_PAGE_ROWS 7
/* Neutral input bits, independent of Vita, SDL and GBA. */
enum { PM_UP=1, PM_DOWN=2, PM_LEFT=4, PM_RIGHT=8, PM_ACCEPT=16,
       PM_BACK=32, PM_TOGGLE=64, PM_START=128, PM_HELP=256 };
typedef enum { PM_BOOL, PM_RANGE, PM_CHOICE, PM_ACTION } PM_Type;
typedef struct {
 const char *id, *label;
 PM_Type type;
 int value, minimum, maximum, step;
 const char *const *choices;
 const char *confirmation; /* Non-NULL actions require a second ACCEPT. */
 bool (*enabled)(void *context);
 bool (*action)(void *context);
 const char *category, *help;
} PM_Option;
typedef struct { uint32_t background, border, text, selected; } PM_Theme;
typedef struct {
 const char *game_name, *settings_path;
 PM_Option *options;
 size_t count, selected;
 void *context;
 PM_Theme theme;
 bool open, dirty, save_failed, confirming;
 unsigned previous;
 char notice[29];
 const char *categories[PM_MAX_OPTIONS];
 size_t category_count, category;
 bool root, help_open;
 int defaults[PM_MAX_OPTIONS];
} PM_Menu;
bool PM_Init(PM_Menu *, const char *, const char *, PM_Option *, size_t, void *);
bool PM_Load(PM_Menu *); /* Missing files retain adapter defaults. */
bool PM_Save(PM_Menu *); /* Temp + backup + rename; failure keeps menu open. */
void PM_Reset(PM_Menu *);
void PM_Open(PM_Menu *, unsigned held);
void PM_Update(PM_Menu *, unsigned held); /* Call regularly, edge driven. */
/* 240x160 RGBA8 framebuffer, stride in pixels. No platform/game dependencies. */
void PM_Draw(const PM_Menu *, uint32_t *, size_t stride);
void PM_Text(uint32_t *, size_t stride, unsigned x, unsigned y, const char *, uint32_t);
#endif
