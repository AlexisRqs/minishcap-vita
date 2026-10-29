#include "port_model.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
static bool hidden;
static bool visible(unsigned id,void*ctx){(void)ctx;return !hidden||id!=3;}
int main(void){
 const unsigned a[]={0,3},b[]={1,2};const PM_Group groups[]={{a,2},{b,2},{NULL,0}};PM_Nav n={.groups=groups,.count=3,.visible=visible};
 assert(PM_NavMove(&n,0,-1,false)==3);assert(PM_NavMove(&n,3,1,true)==1);assert(PM_NavMove(&n,1,1,false)==2);assert(PM_NavMove(&n,2,1,true)==3);
 hidden=true;assert(PM_NavRecall(&n,0)==0&&PM_NavCount(&n,0)==1);assert(PM_NavMove(&n,0,1,true)==2);assert(PM_NavAt(&n,2,0)==UINT_MAX);
 PM_Option o={.type=PM_CHOICE,.value=0,.minimum=0,.maximum=2,.step=1};assert(PM_AdjustOption(&o,-1)&&o.value==2);assert(PM_AdjustOption(&o,1)&&o.value==0);
 o.type=PM_RANGE;o.value=INT_MAX;o.maximum=INT_MAX;assert(!PM_AdjustOption(&o,1)&&o.value==INT_MAX);
 o.type=PM_BOOL;o.value=1;o.maximum=1;assert(PM_AdjustOption(&o,-1)&&o.value==0);o.type=PM_ACTION;assert(!PM_AdjustOption(&o,1));
 puts("PASS: shared navigation remembers IDs, skips empty groups and hidden choices; option rules preserve bounds and wrap choices");
}
