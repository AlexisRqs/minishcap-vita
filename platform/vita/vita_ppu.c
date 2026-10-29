#include "port_ppu.h"
#include "platform_3ds.h"
#include "port_gba_mem.h"
#include "port_hdma.h"
#include "port_widescreen.h"
#include "virtuappu.h"
#include "cpu/mode1.h"
#include "main.h"
#include "player.h"
#include "save.h"
#include <stdio.h>
#include <stdlib.h>
extern void Vita_Present(const uint32_t *pixels);
void Port_PPU_Init(SDL_Window*w){
 (void)w;
 VirtuaPPUMode1GbaMemory memory={gIoMem,gVram,gBgPltt,gObjPltt,gOamMem};
 virtuappu_mode1_bind_gba_memory(&memory);
 extern bool Port_Config_GetColorCorrection(void);
 virtuappu_mode1_set_color_correction(Port_Config_GetColorCorrection());
 virtuappu_registers.frame_width=240;virtuappu_registers.frame_height=160;virtuappu_registers.frame_pitch=240;
 Port_Widescreen_SetWindowPixels(240,160);
}
void Port_PPU_PresentFrame(void){
 virtuappu_registers.mode=(*(uint16_t*)gIoMem&7)?2:1;
 virtuappu_mode1_pre_line_callback=port_hdma_has_active_channels()?port_hdma_step_line:NULL;
 static unsigned frame;frame++;
#ifdef __vita__
 /* Explicit file handle: retain a small, periodically refreshed state report
  * even when the Vita C runtime does not redirect stderr successfully. */
 if(frame == 1 || frame%600 == 0){
  FILE *diagnostic=fopen("ux0:data/minishcap/diagnostic.txt","wb");
  if(diagnostic){
   char line[256];
   int n=snprintf(line,sizeof(line),"Minish Cap Vita 0.06\nframe=%u task=%u state=%u area=%u room=%u\nplayer x=%d y=%d action=%u language=%u\n",
    frame,gMain.task,gMain.state,gRoomControls.area,gRoomControls.room,
    gPlayerEntity.base.x.HALF.HI,gPlayerEntity.base.y.HALF.HI,gPlayerEntity.base.action,gSaveHeader->language);
   if(n>0 && (size_t)n<sizeof(line))fwrite(line,1,(size_t)n,diagnostic);
   fclose(diagnostic);
  }
 }
#endif

 if(getenv("TMC_TRACE") && frame%500==0)fprintf(stderr,"STATE frame=%u task=%u area=%u room=%u x=%d y=%d action=%u lang=%u\n",frame,gMain.task,gRoomControls.area,gRoomControls.room,gPlayerEntity.base.x.HALF.HI,gPlayerEntity.base.y.HALF.HI,gPlayerEntity.base.action,gSaveHeader->language);
 uint64_t renderStart=Platform3DS_SystemTick();
#ifndef __vita__
 /* Asset export: render the original forest layers without title/logo sprites.
  * Host only; restore the display register immediately after rendering. */
 uint16_t assetDisplay=*(uint16_t*)gIoMem;
 if(getenv("TMC_TITLE_BACKGROUND"))*(uint16_t*)gIoMem=assetDisplay&~(0x0400|0x1000);
#endif
 virtuappu_render_frame();
#ifndef __vita__
 *(uint16_t*)gIoMem=assetDisplay;
#endif
 extern void Vita_ProfileRender(uint64_t us);
 Vita_ProfileRender(Platform3DS_SystemTick()-renderStart);
 Vita_Present(virtuappu_frame_buffer);
}
void Port_PPU_SetPresentIsFirstOfTick(bool f){(void)f;}
void Port_PPU_Shutdown(void){}

unsigned Vita_ProfileScene(void){return ((unsigned)gRoomControls.area<<8)|gRoomControls.room;}
