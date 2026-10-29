#include "vita_livearea.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __vita__
#include <psp2/appmgr.h>
#include <psp2/sysmodule.h>
/* This SDK exports the stub but has no public livearea.h. As in PMD Vita. */
extern int sceLiveAreaReplaceAllSync(const char *contents_path);
#endif
bool Vita_IsSettingsLaunch(const char *parameters){
 if(!parameters)return false;
 const char *at=parameters;
 while((at=strstr(at,"-settings"))){
  char after=at[9];
  bool before=at==parameters||strchr(" :=&",at[-1])!=NULL;
  if(before&&(!after||strchr(" &\r\n",after)))return true;
  at+=9;
 }
 return false;
}
bool Vita_LiveAreaSettingsRequested(void){
#ifdef __vita__
 char parameters[1024]={0};
 if(sceAppMgrGetAppParam(parameters)<0)return false;
 parameters[sizeof(parameters)-1]=0;
 return Vita_IsSettingsLaunch(parameters);
#else
 return Vita_IsSettingsLaunch(getenv("TMC_LAUNCH"));
#endif
}
void Vita_LiveAreaRefresh(const char *stamp,unsigned revision){
#ifdef __vita__
 unsigned saved=0;FILE*f=fopen(stamp,"r");
 if(f){if(fscanf(f,"%u",&saved)!=1)saved=0;fclose(f);}
 if(saved==revision)return;
 int result=sceSysmoduleLoadModule(SCE_SYSMODULE_LIVEAREA);
 if(result>=0){
  result=sceLiveAreaReplaceAllSync("app0:/sce_sys/livearea/contents");
  sceSysmoduleUnloadModule(SCE_SYSMODULE_LIVEAREA);
 }
 fprintf(stderr,"LiveArea revision %u refresh: 0x%08x\n",revision,(unsigned)result);
 /* Failed refresh is harmless and retried on next launch. */
 if(result>=0){f=fopen(stamp,"w");if(f){fprintf(f,"%u\n",revision);fclose(f);}}
#else
 (void)stamp;(void)revision;
#endif
}
