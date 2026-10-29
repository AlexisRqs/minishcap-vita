/* SPDX-License-Identifier: MIT */
#include "port_backup.h"
#include "port_runtime.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static void save(unsigned char n){FILE*f=fopen("game.sav","wb");assert(f);for(int i=0;i<64;i++)fputc(n,f);assert(!fclose(f));}
static int first(const char*p){FILE*f=fopen(p,"rb");assert(f);int c=fgetc(f);fclose(f);return c;}
static bool valid(const char*p){FILE*f=fopen(p,"rb");if(!f)return false;int c=fgetc(f);fclose(f);return c>0&&c<100;}
int main(int argc,char**argv){assert(argc==2&&!chdir(argv[1]));PB_Store s={"game.sav","backup",64,valid};PB_Info info;
 assert(!PB_Capture(NULL)&&!PB_Recover(NULL)&&!PB_Restore(NULL,0));
 assert(!PB_Capture(&s));save(1);assert(PB_Capture(&s));assert(PB_InfoAt(&s,0,&info)&&info.generation==1);
 assert(PB_Capture(&s));assert(!PB_InfoAt(&s,1,&info));
 save(2);assert(PB_Capture(&s));save(3);assert(PB_Capture(&s));save(4);assert(PB_Capture(&s));
 assert(PB_InfoAt(&s,0,&info)&&info.generation==4);assert(PB_Restore(&s,1));assert(first("game.sav")==2&&first("game.sav.before-restore")==4);
 PB_Info ordered[PB_SLOTS];unsigned slots[PB_SLOTS];PB_List(&s,ordered,slots);assert(slots[0]==0&&slots[1]==2&&slots[2]==1);
 assert(PB_PreviousInfo(&s,&info)&&info.valid);char date[32];PB_Date(0,date,sizeof(date));assert(!strcmp(date,"date inconnue"));PB_Date(1792756800,date,sizeof(date));assert(strlen(date)==19);
 assert(!rename("game.sav.before-restore","game.sav.before-restore.old"));assert(PB_Recover(&s)&&PB_PreviousInfo(&s,NULL));
 assert(PB_RestorePrevious(&s)&&first("game.sav")==4&&first("game.sav.before-restore")==4);
 PB_List(&s,ordered,slots);assert(ordered[0].generation==5);assert(PB_Restore(&s,slots[0])&&first("game.sav")==2);
 assert(!rename("game.sav","game.sav.restore.rollback"));assert(PB_Recover(&s)&&first("game.sav")==2);
 assert(!rename("backup.2.pmb","backup.2.pmb.old"));assert(PB_Recover(&s)&&PB_InfoAt(&s,2,&info));
 FILE*f=fopen("backup.1.pmb","r+b");assert(f);fseek(f,40,SEEK_SET);fputc(99,f);fclose(f);assert(!PB_InfoAt(&s,1,&info)&&!PB_Restore(&s,1)&&first("game.sav")==2);
 PB_Store wrong=s;wrong.path="other.sav";assert(!PB_InfoAt(&wrong,0,&info));wrong.prefix=NULL;assert(!PB_Capture(&wrong));
 save(200);assert(!PB_Capture(&s));assert(!rename("game.sav","game.sav.restore.rollback"));save(201);assert(!PB_Recover(&s)&&first("game.sav.restore.rollback")==200);
 int map[]={1,0,9,-1};uint16_t targets[]={1,2};assert(PR_MapButtons(1,map,targets,4,2)==2);assert(PR_MapButtons(15,map,targets,4,2)==3);
 assert(PR_Stick(.2f,0,.3f,true)==0&&PR_Stick(.5f,-.5f,.3f,true)==80&&PR_Stick(1,1,.3f,false)==0);
 PR_Lifecycle l={0};assert(PR_SetPause(&l,PR_PAUSE_MENU,true));assert(!PR_ReleaseInputs(&l,true));PR_SetPause(&l,PR_PAUSE_SYSTEM,true);PR_SetPause(&l,PR_PAUSE_MENU,false);assert(l.pauses&&l.resumes==0);PR_SetPause(&l,PR_PAUSE_SYSTEM,false);assert(l.resumes==1&&!PR_ReleaseInputs(&l,false)&&!PR_ReleaseInputs(&l,true)&&PR_ReleaseInputs(&l,true));
 PR_Profiler p={0};for(unsigned i=0;i<=6100;i++)PR_ProfileFrame(&p,1+(uint64_t)i*16667,1000,2000,0x3410);assert(p.count==60);PR_Sample a=p.samples[(p.next+59)%60];assert(a.fps>59&&a.fps<61&&a.ppu_ms==1&&a.present_ms==2&&a.slow==0);PR_ProfileResetClock(&p);PR_ProfileFrame(&p,999999999,0,0,0);assert(p.count==60&&p.frames==0);assert(PR_ProfileExport(&p,"performance.csv"));
 puts("PASS: backup rotation, dedup, CRC, profile isolation, restore, crash recovery, invalid source; remapping, deadzone, nested pauses, neutral gate, profiler ring/export");
}
