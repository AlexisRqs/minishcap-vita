/* Vita implementation of the inherited compact-console platform interface.
 * Its legacy Platform3DS names are retained to minimize engine changes. */
#include "platform_3ds.h"
#include "vita_menu.h"
#include "vita_livearea.h"
#include "port-menu/port_runtime.h"
#include "port_audio.h"
extern void Port_Audio_3DSSetPaused(bool paused);
extern void Vita_AudioSystemPaused(bool paused);
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef __vita__
#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/power.h>
#include <vita2d.h>
#else
#include <SDL.h>
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
#endif
static uint16_t keys=0x3ff,downkeys=0x3ff;
static unsigned frames;
static bool running=true;
static uint64_t nextFrame;
static float stickx,sticky;
static uint32_t lastPixels[240*160], menuPixels[240*160], hudPixels[240*160];
static unsigned uiKeys,previousUi;
static bool releaseKeys,menuRendering;
static unsigned physicalButtons;
static uint64_t fpsStarted;
static unsigned fpsFrames;
static PR_Lifecycle lifecycle;
static PR_Profiler profiler;
static uint64_t ppuTime;
void Vita_ProfileRender(uint64_t us){ppuTime=us;}
bool Vita_ProfileExport(void){return profiler.count&&PR_ProfileExport(&profiler,"performance.csv");}
static void SystemPause(bool active){
 if(PR_SetPause(&lifecycle,PR_PAUSE_SYSTEM,active)){
  Vita_AudioSystemPaused(active);releaseKeys=true;nextFrame=0;fpsStarted=0;fpsFrames=0;PR_ProfileResetClock(&profiler);
  fprintf(stderr,"LIFECYCLE %s frame=%u\n",active?"suspend":"resume",frames);
 }
}
static char fpsLabel[28]="FPS : --";
#ifdef __vita__
static vita2d_texture *texture;
static int powerCallback=-1;
static int PowerEvent(int id,int count,int info,void *user){
 (void)id;(void)count;(void)user;
 if(info&(SCE_POWER_CB_SYSTEM_SUSPEND|SCE_POWER_CB_APP_SUSPEND))SystemPause(true);
 if(info&(SCE_POWER_CB_SYSTEM_RESUME|SCE_POWER_CB_APP_RESUME|SCE_POWER_CB_AFTER_SYSTEM_RESUME))SystemPause(false);
 return 0;
}
#endif
int Platform3DS_Init(void){
#ifdef __vita__
 powerCallback=sceKernelCreateCallback("port-power",0,PowerEvent,NULL);
 if(powerCallback>=0&&scePowerRegisterCallback(powerCallback)<0){sceKernelDeleteCallback(powerCallback);powerCallback=-1;}
 if(powerCallback<0)fprintf(stderr,"WARNING: power callback unavailable\n");
 sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
 scePowerSetArmClockFrequency(444);
 if(!vita2d_init()) return 0;
 vita2d_set_vblank_wait(0);vita2d_set_clear_color(0xff000000);
 texture=vita2d_create_empty_texture(240,160);
 if(texture)vita2d_texture_set_filters(texture,SCE_GXM_TEXTURE_FILTER_POINT,SCE_GXM_TEXTURE_FILTER_POINT);
 return texture!=NULL;
#else
 if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)) return 0;
 window=SDL_CreateWindow("Minish Cap Vita — host validation",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,960,544,0);
 renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
 texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ABGR8888,SDL_TEXTUREACCESS_STREAMING,240,160);
 return texture!=NULL;
#endif
}
static void PresentPixels(const uint32_t *pixels,bool menu){
 float scale=3.0f;
 if(!menu){int mode=Vita_MenuValue(VM_SCALE);if(mode==1)scale=544.0f/160.0f;else if(mode==2)scale=2.0f;}
 float x=(960-240*scale)/2,y=(544-160*scale)/2;
#ifdef __vita__
 unsigned stride=vita2d_texture_get_stride(texture)/4;
 uint32_t *dst=vita2d_texture_get_datap(texture);
 for(unsigned row=0;row<160;row++)memcpy(dst+row*stride,pixels+row*240,240*4);
 int filter=!menu&&Vita_MenuValue(VM_FILTER)?SCE_GXM_TEXTURE_FILTER_LINEAR:SCE_GXM_TEXTURE_FILTER_POINT;
 vita2d_texture_set_filters(texture,filter,filter);
 vita2d_start_drawing();vita2d_clear_screen();
 vita2d_draw_texture_scale(texture,x,y,scale,scale);
 vita2d_end_drawing();vita2d_swap_buffers();vita2d_wait_rendering_done();
#else
 SDL_SetTextureScaleMode(texture,!menu&&Vita_MenuValue(VM_FILTER)?SDL_ScaleModeLinear:SDL_ScaleModeNearest);
 SDL_UpdateTexture(texture,NULL,pixels,240*4);SDL_SetRenderDrawColor(renderer,0,0,0,255);SDL_RenderClear(renderer);
 SDL_Rect rect={(int)x,(int)y,(int)(240*scale),(int)(160*scale)};
 SDL_RenderCopy(renderer,texture,NULL,&rect);SDL_RenderPresent(renderer);
#endif
}
static void DumpPixels(const char *path,const uint32_t *pixels){
 FILE *f=fopen(path,"wb");if(!f)return;
 fprintf(f,"P6\n240 160\n255\n");
 for(int i=0;i<240*160;i++){unsigned char p[3]={pixels[i],pixels[i]>>8,pixels[i]>>16};fwrite(p,1,3,f);}fclose(f);
}
void Vita_Present(const uint32_t *pixels){
 uint64_t presentStart=Platform3DS_SystemTick();
 memcpy(lastPixels,pixels,sizeof(lastPixels));
 uint64_t now=Platform3DS_SystemTick();if(!fpsStarted)fpsStarted=now;fpsFrames++;
 if(now-fpsStarted>=1000000){snprintf(fpsLabel,sizeof(fpsLabel),"FPS : %.1f",fpsFrames*1000000.0/(now-fpsStarted));fpsFrames=0;fpsStarted=now;}
 if(Vita_MenuValue(VM_FPS)||Vita_MenuValue(VM_PERF)){
  memcpy(hudPixels,pixels,sizeof(hudPixels));
  for(unsigned y=0;y<10;y++)for(unsigned x=0;x<104;x++)hudPixels[y*240+x]=0xff101010;
  if(Vita_MenuValue(VM_FPS))PM_Text(hudPixels,240,0,1,fpsLabel,0xffffffff);
  if(Vita_MenuValue(VM_PERF)&&profiler.count){PR_Sample *sample=&profiler.samples[(profiler.next+PR_HISTORY-1)%PR_HISTORY];char text[30];snprintf(text,sizeof(text),"PPU %.1f Pres %.1f ms",sample->ppu_ms,sample->present_ms);PM_Text(hudPixels,240,0,12,text,0xffffffff);}
  PresentPixels(hudPixels,false);
 }else PresentPixels(pixels,false);
 extern unsigned Vita_ProfileScene(void);
 PR_ProfileFrame(&profiler,Platform3DS_SystemTick(),ppuTime,Platform3DS_SystemTick()-presentStart,Vita_ProfileScene());
 const char *dump=getenv("TMC_DUMP_AT");if(dump&&frames==(unsigned)atoi(dump))DumpPixels("frame.ppm",pixels);
}
uint64_t Platform3DS_Milliseconds(void){return Platform3DS_SystemTick()/1000;}
uint64_t Platform3DS_SystemTick(void){
#ifdef __vita__
 return sceKernelGetProcessTimeWide();
#else
 return SDL_GetPerformanceCounter()*1000000/SDL_GetPerformanceFrequency();
#endif
}
uint64_t Platform3DS_TicksPerSecond(void){return 1000000;}
static void ReadControls(void){
 keys=0x3ff;uiKeys=0;stickx=sticky=0;physicalButtons=0;
#ifdef __vita__
 SceCtrlData p={0}; sceCtrlPeekBufferPositive(0,&p,1);
 const unsigned vita[7]={SCE_CTRL_CROSS,SCE_CTRL_CIRCLE,SCE_CTRL_SQUARE,SCE_CTRL_LTRIGGER,SCE_CTRL_RTRIGGER,SCE_CTRL_START,SCE_CTRL_SELECT};
 for(int i=0;i<7;i++)if(p.buttons&vita[i])physicalButtons|=1u<<i;
 keys&=~Vita_MapButtons(physicalButtons);
 if(p.buttons&SCE_CTRL_UP)keys&=~64;if(p.buttons&SCE_CTRL_DOWN)keys&=~128;
 if(p.buttons&SCE_CTRL_LEFT)keys&=~32;if(p.buttons&SCE_CTRL_RIGHT)keys&=~16;
 stickx=((int)p.lx-128)/128.f; sticky=((int)p.ly-128)/128.f;
 if(p.buttons&SCE_CTRL_UP)uiKeys|=PM_UP;if(p.buttons&SCE_CTRL_DOWN)uiKeys|=PM_DOWN;
 if(p.buttons&SCE_CTRL_LEFT)uiKeys|=PM_LEFT;if(p.buttons&SCE_CTRL_RIGHT)uiKeys|=PM_RIGHT;
 if(p.buttons&SCE_CTRL_CROSS)uiKeys|=PM_ACCEPT;if(p.buttons&SCE_CTRL_CIRCLE)uiKeys|=PM_BACK;
 if(p.buttons&SCE_CTRL_TRIANGLE)uiKeys|=PM_TOGGLE;if(p.buttons&SCE_CTRL_START)uiKeys|=PM_START;
 if(p.buttons&SCE_CTRL_SQUARE)uiKeys|=PM_HELP;
#else
 SDL_Event e;while(SDL_PollEvent(&e)){
  if(e.type==SDL_QUIT)running=false;
  if(e.type==SDL_APP_WILLENTERBACKGROUND||(e.type==SDL_WINDOWEVENT&&e.window.event==SDL_WINDOWEVENT_FOCUS_LOST))SystemPause(true);
  if(e.type==SDL_APP_DIDENTERFOREGROUND||(e.type==SDL_WINDOWEVENT&&e.window.event==SDL_WINDOWEVENT_FOCUS_GAINED))SystemPause(false);
 }
 const Uint8 *k=SDL_GetKeyboardState(NULL);
 const int sc[7]={SDL_SCANCODE_X,SDL_SCANCODE_Z,SDL_SCANCODE_C,SDL_SCANCODE_A,SDL_SCANCODE_S,SDL_SCANCODE_RETURN,SDL_SCANCODE_BACKSPACE};
 for(int i=0;i<7;i++)if(k[sc[i]])physicalButtons|=1u<<i;
 keys&=~Vita_MapButtons(physicalButtons);
 if(k[SDL_SCANCODE_UP])keys&=~64;if(k[SDL_SCANCODE_DOWN])keys&=~128;if(k[SDL_SCANCODE_LEFT])keys&=~32;if(k[SDL_SCANCODE_RIGHT])keys&=~16;
 if(k[SDL_SCANCODE_C])uiKeys|=PM_HELP;
 if(k[SDL_SCANCODE_UP])uiKeys|=PM_UP;if(k[SDL_SCANCODE_DOWN])uiKeys|=PM_DOWN;
 if(k[SDL_SCANCODE_LEFT])uiKeys|=PM_LEFT;if(k[SDL_SCANCODE_RIGHT])uiKeys|=PM_RIGHT;
 if(k[SDL_SCANCODE_X])uiKeys|=PM_ACCEPT;if(k[SDL_SCANCODE_Z])uiKeys|=PM_BACK;
 if(k[SDL_SCANCODE_T])uiKeys|=PM_TOGGLE;if(k[SDL_SCANCODE_RETURN])uiKeys|=PM_START;
 static unsigned inputStart[512], inputEnd[512],inputMask[512],inputCount;
 static bool scriptLoaded;
 if(!scriptLoaded){scriptLoaded=true;const char*path=getenv("TMC_INPUT_SCRIPT");FILE*f=path?fopen(path,"r"):NULL;if(f){while(inputCount<512 && fscanf(f,"%u %u %x",&inputStart[inputCount],&inputEnd[inputCount],&inputMask[inputCount])==3)inputCount++;fclose(f);}}
 for(unsigned i=0;i<inputCount;i++)if(!menuRendering&&frames>=inputStart[i]&&frames<inputEnd[i])keys&=~inputMask[i];
 const char *autoInput=getenv("TMC_AUTO_INPUT");
 if(autoInput){if(frames>700 && frames<710)keys&=~8; if(frames>850 && frames<860)keys&=~1;}
#endif
 if(!Vita_MenuValue(VM_STICK))stickx=sticky=0;
 keys&=~PR_Stick(stickx,sticky,Vita_MenuValue(VM_DEADZONE)/100.0f,Vita_MenuValue(VM_STICK));

}
#ifndef __vita__
/* Separate modal clock: host regression inputs must not advance game frames. */
static unsigned ScriptedMenuInput(unsigned tick){
 const char *path=getenv("TMC_MENU_SCRIPT");if(!path)return uiKeys;
 FILE *f=fopen(path,"r");if(!f)return PM_BACK;
 unsigned lo,hi,mask,held=0;while(fscanf(f,"%u %u %x",&lo,&hi,&mask)==3)if(tick>=lo&&tick<hi)held|=mask;
 fclose(f);return held;
}
#endif
static void ServicePower(void){
#ifdef __vita__
 sceKernelCheckCallback();
#endif
#ifndef __vita__
 static bool testDone;unsigned ticks=0;const char *test=getenv("TMC_SUSPEND_AT");
 bool inject=test&&!testDone&&frames>=(unsigned)atoi(test);if(inject){testDone=true;SystemPause(true);}
#endif
 while((lifecycle.pauses&PR_PAUSE_SYSTEM)&&running){
#ifdef __vita__
  sceKernelDelayThreadCB(10000);
#else
  SDL_Delay(10);if(inject&&++ticks==10)SystemPause(false);
#endif
  ReadControls();
 }
}
static void OpenMenu(void){
 PR_SetPause(&lifecycle,PR_PAUSE_MENU,true);Port_Audio_3DSSetPaused(true);menuRendering=true;
 Vita_MenuOpen(uiKeys);unsigned tick=0;
 fprintf(stderr,"MENU open game_frame=%u\n",frames);
 while(Vita_MenuActive()&&running){
  ServicePower();
  Vita_MenuDraw(menuPixels);PresentPixels(menuPixels,true);
#ifdef __vita__
  sceDisplayWaitVblankStart();
#else
  if(tick==(getenv("TMC_MENU_DUMP_AT")?(unsigned)atoi(getenv("TMC_MENU_DUMP_AT")):0)&&getenv("TMC_MENU_DUMP"))DumpPixels(getenv("TMC_MENU_DUMP"),menuPixels);
  SDL_Delay(16);
#endif
  ReadControls();
#ifndef __vita__
  uiKeys=ScriptedMenuInput(++tick);
#endif
  Vita_MenuUpdate(uiKeys);
 }
 menuRendering=false;releaseKeys=true;previousUi=uiKeys;keys=downkeys=0x3ff;stickx=sticky=0;
 nextFrame=0;fpsStarted=0;fpsFrames=0;
 PR_SetPause(&lifecycle,PR_PAUSE_MENU,false);PR_ProfileResetClock(&profiler);
 PresentPixels(lastPixels,false);Port_Audio_3DSSetPaused(false);
 fprintf(stderr,"MENU close game_frame=%u\n",frames);
}
void Vita_OpenLaunchSettings(void){
 ReadControls();OpenMenu();
}
static void Poll(void){
 ServicePower();uint16_t prev=keys;ReadControls();ServicePower();unsigned pressed=uiKeys&~previousUi;previousUi=uiKeys;
#ifndef __vita__
 static bool testOpened;
 const char *testAt=getenv("TMC_MENU_AT");if(testAt&&!testOpened&&frames>=(unsigned)atoi(testAt)){pressed|=PM_TOGGLE;testOpened=true;}
#endif
 if(releaseKeys){
  float deadzone=Vita_MenuValue(VM_DEADZONE)/100.0f;
  if(keys==0x3ff&&!uiKeys&&!physicalButtons&&stickx>-deadzone&&stickx<deadzone&&sticky>-deadzone&&sticky<deadzone){releaseKeys=false;PR_ReleaseInputs(&lifecycle,true);}
  keys=0x3ff;stickx=sticky=0;
 }else if(pressed&PM_TOGGLE)OpenMenu();
 downkeys=~((~keys)&prev)&0x3ff;
 const char *limit=getenv("TMC_FRAMES");if(limit&&frames>=(unsigned)atoi(limit))running=false;
 if(!running)exit(0);
}
bool Platform3DS_BeginFrameBoundary(void){frames++;Poll();return true;}
extern void Vita_AudioTick(void);
void Platform3DS_EndFrameBoundary(void){Vita_AudioTick();}
void Platform3DS_WaitForVBlank(void){
 if(getenv("TMC_UNPACED"))return;
 uint64_t now=Platform3DS_SystemTick(); if(nextFrame<now-20000)nextFrame=now;
 nextFrame+=16743;
 if(nextFrame>now){
#ifdef __vita__
 sceKernelDelayThread(nextFrame-now);
#else
 SDL_Delay((nextFrame-now)/1000);
#endif
 }
}
void Platform3DS_PumpWithoutVBlank(void){Poll();}
uint16_t Platform3DS_ReadKeyInput(void){return keys;}
uint16_t Platform3DS_ReadKeyDownInput(void){return downkeys;}
uint32_t Platform3DS_KeysHeld(void){return 0;}
void Platform3DS_ReadCircle(float*x,float*y){*x=stickx;*y=sticky;}
bool Platform3DS_IsRunning(void){return running;}
bool Platform3DS_IsNew3DS(void){return false;}
bool Platform3DS_CanUseCore1(void){return false;}
bool Platform3DS_TurboHeld(void){return false;}
unsigned Platform3DS_TurboMultiplier(void){return 1;}
void Platform3DS_SetTurboMultiplier(unsigned n){(void)n;}
void Platform3DS_MarkFrameDiscontinuity(Old3DSFramePacerDiscontinuity r){(void)r;nextFrame=0;}
int Platform3DS_IsNativeAddress(uintptr_t p){return p>0x10000000u;}
int Platform3DS_IsActiveStackAddress(uintptr_t p){uintptr_t local=(uintptr_t)&p;return p>local-4*1024*1024 && p<local+4*1024*1024;}
void Platform3DS_Debug(const char*m){fputs(m,stderr);}
void Platform3DS_ShowFatal(const char*t,const char*m){fprintf(stderr,"%s: %s\n",t,m);fflush(stderr);
#ifdef __vita__
 vita2d_pgf *font=vita2d_load_default_pgf();
 for(int i=0;i<600;i++){vita2d_start_drawing();vita2d_clear_screen();vita2d_pgf_draw_text(font,30,100,0xffffffff,1,t);vita2d_pgf_draw_text(font,30,150,0xffffffff,.8,m);vita2d_end_drawing();vita2d_swap_buffers();sceDisplayWaitVblankStart();}vita2d_free_pgf(font);
#endif
}
void Platform3DS_Shutdown(void){
#ifdef __vita__
 if(powerCallback>=0){scePowerUnregisterCallback(powerCallback);sceKernelDeleteCallback(powerCallback);powerCallback=-1;}
 vita2d_free_texture(texture);vita2d_fini();
#else
 SDL_DestroyTexture(texture);SDL_DestroyRenderer(renderer);SDL_DestroyWindow(window);SDL_Quit();
#endif
}
