#ifndef VITA_MENU_H
#define VITA_MENU_H
#include "port-menu/port_menu.h"
enum { VM_FPS, VM_STICK, VM_VOLUME, VM_SCALE, VM_FILTER, VM_COLOR, VM_DEADZONE, VM_FAST, VM_PERF, VM_MAP_X, VM_MAP_O, VM_MAP_SQUARE, VM_MAP_L, VM_MAP_R, VM_MAP_START, VM_MAP_SELECT, VM_COUNT };
void Vita_MenuInit(void);
bool Vita_BackupRecover(void);
void Vita_BackupOnSave(const char *path);
uint16_t Vita_MapButtons(unsigned physical);
bool Vita_FastBoot(void);
int Vita_MenuValue(unsigned option);
void Vita_MenuOpen(unsigned held);
bool Vita_MenuActive(void);
void Vita_MenuUpdate(unsigned held);
void Vita_MenuDraw(uint32_t *pixels);
#endif
