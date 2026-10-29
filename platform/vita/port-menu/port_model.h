/* SPDX-License-Identifier: MIT */
#ifndef PORT_MODEL_H
#define PORT_MODEL_H
#include "port_menu.h"
/* Shared state rules; renderers and storage formats remain adapter-owned. */
bool PM_AdjustOption(PM_Option *option,int direction);
#define PM_NAV_GROUPS 16
typedef struct {const unsigned *ids;unsigned count;} PM_Group;
typedef struct {const PM_Group *groups;unsigned count;unsigned remembered[PM_NAV_GROUPS];bool (*visible)(unsigned,void*);void *context;} PM_Nav;
unsigned PM_NavCount(const PM_Nav*,unsigned group);
unsigned PM_NavAt(const PM_Nav*,unsigned group,unsigned row);
unsigned PM_NavGroup(const PM_Nav*,unsigned id);
unsigned PM_NavRecall(const PM_Nav*,unsigned group);
unsigned PM_NavMove(PM_Nav*,unsigned selected,int direction,bool tab);
/* Stable catalog ordering; clock changes do not reorder save generations. */
void PM_SortNewest(unsigned *order,const uint64_t *generation,const bool *valid,unsigned count);
bool PM_FormatDate(uint64_t timestamp,char *out,size_t size);
#endif
