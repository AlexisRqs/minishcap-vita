#ifndef VITA_LIVEAREA_H
#define VITA_LIVEAREA_H
#include <stdbool.h>
/* Same-app LiveArea launch, reusable by other Vita adapters. */
bool Vita_LiveAreaSettingsRequested(void);
bool Vita_IsSettingsLaunch(const char *parameters);
void Vita_LiveAreaRefresh(const char *stamp_path, unsigned revision);
void Vita_OpenLaunchSettings(void);
#endif
