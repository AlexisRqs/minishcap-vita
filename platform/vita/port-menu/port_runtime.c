/* SPDX-License-Identifier: MIT */
#include "port_runtime.h"
#include <stdio.h>
#include <string.h>
uint16_t PR_MapButtons(unsigned physical,const int *map,const uint16_t *targets,size_t count,size_t target_count){uint16_t out=0;for(size_t i=0;i<count&&i<32;i++)if((physical&(1u<<i))&&map[i]>=0&&(size_t)map[i]<target_count)out|=targets[map[i]];return out;}
uint16_t PR_Stick(float x,float y,float d,bool on){if(!on)return 0;if(d<.1f)d=.1f;if(d>.8f)d=.8f;return (x>d?16:0)|(x<-d?32:0)|(y<-d?64:0)|(y>d?128:0);}
bool PR_SetPause(PR_Lifecycle *s,unsigned reason,bool active){unsigned old=s->pauses;if(active)s->pauses|=reason;else s->pauses&=~reason;if(old&&!s->pauses){s->release=true;s->resumes++;}return old!=s->pauses;}
bool PR_ReleaseInputs(PR_Lifecycle *s,bool neutral){if(s->pauses)return false;if(s->release){if(neutral)s->release=false;return false;}return true;}
void PR_ProfileResetClock(PR_Profiler *p){p->started=p->last=p->ppu=p->present=p->max_frame=0;p->frames=p->slow=0;}
void PR_ProfileFrame(PR_Profiler *p,uint64_t now,uint64_t ppu,uint64_t present,unsigned scene){
 if(!p->started){p->started=p->last=now;return;}
 uint64_t dt=now-p->last;p->last=now;p->frames++;p->ppu+=ppu;p->present+=present;if(dt>p->max_frame)p->max_frame=dt;if(dt>20000)p->slow++;
 if(now-p->started>=1000000){p->samples[p->next]=(PR_Sample){p->frames*1000000.0/(now-p->started),p->ppu/(p->frames*1000.0),p->present/(p->frames*1000.0),p->max_frame,p->slow,scene};p->next=(p->next+1)%PR_HISTORY;if(p->count<PR_HISTORY)p->count++;PR_ProfileResetClock(p);p->started=p->last=now;}
}
bool PR_ProfileExport(const PR_Profiler *p,const char *path){FILE*f=fopen(path,"wb");if(!f)return false;bool ok=fprintf(f,"sample,fps,ppu_ms,present_ms,max_frame_us,over_20ms,scene\n")>0;for(unsigned i=0;i<p->count;i++){unsigned at=(p->next+PR_HISTORY-p->count+i)%PR_HISTORY;const PR_Sample*s=&p->samples[at];if(fprintf(f,"%u,%.3f,%.3f,%.3f,%llu,%u,%u\n",i,s->fps,s->ppu_ms,s->present_ms,(unsigned long long)s->max_frame_us,s->slow,s->scene)<0)ok=false;}if(fclose(f))ok=false;return ok;}
