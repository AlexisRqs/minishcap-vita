/* SPDX-License-Identifier: MIT */
#ifndef PORT_RUNTIME_H
#define PORT_RUNTIME_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
uint16_t PR_MapButtons(unsigned physical,const int *mapping,const uint16_t *targets,size_t count,size_t target_count);
uint16_t PR_Stick(float x,float y,float deadzone,bool enabled);
enum { PR_PAUSE_MENU=1, PR_PAUSE_SYSTEM=2 };
typedef struct {unsigned pauses,resumes;bool release;} PR_Lifecycle;
bool PR_SetPause(PR_Lifecycle *,unsigned reason,bool active);
bool PR_ReleaseInputs(PR_Lifecycle *,bool neutral);
#define PR_HISTORY 60
typedef struct {double fps,ppu_ms,present_ms;uint64_t max_frame_us;unsigned slow,scene;} PR_Sample;
typedef struct {uint64_t started,last,ppu,present,max_frame;unsigned frames,slow,next,count;PR_Sample samples[PR_HISTORY];} PR_Profiler;
void PR_ProfileResetClock(PR_Profiler *);
void PR_ProfileFrame(PR_Profiler *,uint64_t now,uint64_t ppu_us,uint64_t present_us,unsigned scene);
bool PR_ProfileExport(const PR_Profiler *,const char *path);
#endif
