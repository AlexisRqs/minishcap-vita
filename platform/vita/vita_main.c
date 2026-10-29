#include "platform_3ds.h"
#include "port_rom.h"
#include "port_audio.h"
#include "port_ppu.h"
#include "vita_menu.h"
#include "vita_livearea.h"
#include "port_runtime_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef __vita__
#include <psp2/kernel/processmgr.h>
unsigned int _sceUserMainThreadStackSize = 512 * 1024;
unsigned int _newlib_heap_size_user = 160 * 1024 * 1024;
#endif
extern void AgbMain(void);
int main(int argc, char **argv) {
#ifdef __vita__
    mkdir("ux0:data/minishcap",0777);
    chdir("ux0:data/minishcap");
    freopen("vita.log","w",stderr);
    freopen("stdout.log","w",stdout);
#endif
    if(!Platform3DS_Init()) return 1;
#ifdef __vita__
    const char *rom="baserom_eu.gba";
#else
    const char *rom=argc>1?argv[1]:"baserom_eu.gba";
#endif
    FILE *f=fopen(rom,"rb");
    if(!f){Platform3DS_ShowFatal("ROM missing","Copy baserom_eu.gba to ux0:data/minishcap/"); return 1;}
    unsigned char header[0xb0];
    int valid=fread(header,1,sizeof(header),f)==sizeof(header);
    if(valid){fseek(f,0,SEEK_END);valid=ftell(f)==16*1024*1024 && memcmp(header+0xac,"BZMP",4)==0;}
    fclose(f);
    if(!valid){Platform3DS_ShowFatal("Unsupported ROM","This build requires the clean European Minish Cap ROM.");return 1;}
    fprintf(stderr,"Minish Cap Vita 0.06: loading %s\n",rom); fflush(stderr);
    Port_Config_Load("minishcap.ini");
    Port_Config_SetWidescreenEnabled(false);
    Port_LoadRom(rom);
    if(!Vita_BackupRecover()){Platform3DS_ShowFatal("Save recovery failed","Existing save files have been preserved.");return 1;}
    Port_PPU_Init(NULL);
    if(!getenv("TMC_NO_AUDIO")){
        if(Port_Audio_Init()){Port_Audio_SetMasterVolume(Port_Config_GetMasterVolume());atexit(Port_Audio_Shutdown);}
        else fprintf(stderr,"WARNING: audio initialization failed\n");
    }
    Vita_MenuInit();
    Vita_LiveAreaRefresh("livearea-revision.txt",5);
    if(Vita_LiveAreaSettingsRequested())Vita_OpenLaunchSettings();
    fprintf(stderr,"Entering AgbMain\n"); fflush(stderr);
    AgbMain();
    Port_PPU_Shutdown();
    Platform3DS_Shutdown();
    return 0;
}
