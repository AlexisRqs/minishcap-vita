/* SPDX-License-Identifier: MIT */
#include "port_model.h"
#include <limits.h>
#include <time.h>
bool PM_AdjustOption(PM_Option *o,int direction){
 if(!o||o->type==PM_ACTION||o->minimum>o->maximum||o->step<1)return false;
 int old=o->value;long long v=(long long)old+(direction<0?-o->step:o->step);
 if(o->type==PM_BOOL)v=!old;
 if(o->type==PM_CHOICE){if(v>o->maximum)v=o->minimum;if(v<o->minimum)v=o->maximum;}
 if(v<o->minimum)v=o->minimum;if(v>o->maximum)v=o->maximum;
 o->value=(int)v;return old!=o->value;
}
static bool valid(const PM_Nav*n,unsigned g){return n&&n->groups&&n->count<=PM_NAV_GROUPS&&g<n->count;}
static bool shown(const PM_Nav*n,unsigned id){return !n->visible||n->visible(id,n->context);}
unsigned PM_NavCount(const PM_Nav*n,unsigned g){unsigned count=0;if(valid(n,g))for(unsigned i=0;i<n->groups[g].count;i++)count+=shown(n,n->groups[g].ids[i]);return count;}
unsigned PM_NavAt(const PM_Nav*n,unsigned g,unsigned row){if(valid(n,g))for(unsigned i=0;i<n->groups[g].count;i++)if(shown(n,n->groups[g].ids[i])){if(!row--)return n->groups[g].ids[i];}return UINT_MAX;}
unsigned PM_NavGroup(const PM_Nav*n,unsigned id){if(n&&n->groups&&n->count<=PM_NAV_GROUPS)for(unsigned g=0;g<n->count;g++)for(unsigned i=0;i<n->groups[g].count;i++)if(n->groups[g].ids[i]==id)return g;return 0;}
unsigned PM_NavRecall(const PM_Nav*n,unsigned g){if(!valid(n,g))return UINT_MAX;unsigned id=n->remembered[g];for(unsigned r=0;r<PM_NavCount(n,g);r++)if(PM_NavAt(n,g,r)==id)return id;return PM_NavAt(n,g,0);}
unsigned PM_NavMove(PM_Nav*n,unsigned selected,int direction,bool tab){
 if(!n||!n->count||n->count>PM_NAV_GROUPS)return selected;
 unsigned g=PM_NavGroup(n,selected),count=PM_NavCount(n,g),row=0;
 if(!count)return selected;
 for(unsigned r=0;r<count;r++)if(PM_NavAt(n,g,r)==selected)row=r;
 n->remembered[g]=PM_NavAt(n,g,row);
 if(tab){for(unsigned tries=0;tries<n->count;tries++){g=(g+n->count+(direction<0?-1:1))%n->count;if(PM_NavCount(n,g))return PM_NavRecall(n,g);}return selected;}
 row=(row+count+(direction<0?-1:1))%count;n->remembered[g]=PM_NavAt(n,g,row);return n->remembered[g];
}

void PM_SortNewest(unsigned*order,const uint64_t*generation,const bool*valid,unsigned count){
 for(unsigned i=0;i<count;i++){unsigned at=i;order[i]=i;while(at&&valid[i]&&(!valid[order[at-1]]||generation[i]>generation[order[at-1]])){order[at]=order[at-1];at--;}order[at]=i;}
}
bool PM_FormatDate(uint64_t timestamp,char*out,size_t size){
 if(!out||!size)return false;out[0]=0;time_t t=(time_t)timestamp;
 if(!timestamp||t<=0||(uint64_t)t!=timestamp)return false;
 struct tm *date=localtime(&t);return date&&strftime(out,size,"%d/%m/%Y %H:%M:%S",date)>0;
}
