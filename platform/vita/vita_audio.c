#include "port_audio.h"
#include "port_m4a_backend.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>
static atomic_uint paused;
#ifdef __vita__
static atomic_flag mix_lock = ATOMIC_FLAG_INIT;
#endif
static FILE *capture;
#ifdef __vita__
#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>
static int audioPort=-1,thread=-1;
static atomic_bool playing;
static atomic_bool reopen_output;
static int audioThread(unsigned n,void*p){
 (void)n;(void)p; int16_t buf[512*2];
 while(playing){
  if(atomic_exchange(&reopen_output,false)){if(audioPort>=0)sceAudioOutReleasePort(audioPort);audioPort=-1;}
  if(audioPort<0){audioPort=sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM,512,48000,SCE_AUDIO_OUT_MODE_STEREO);if(audioPort<0){sceKernelDelayThread(10000);continue;}}
  while(atomic_flag_test_and_set(&mix_lock))sceKernelDelayThread(100);
  if(paused)memset(buf,0,sizeof(buf));else Port_M4A_Backend_Render(buf,512,false);
  atomic_flag_clear(&mix_lock);if(sceAudioOutOutput(audioPort,buf)<0){reopen_output=true;sceKernelDelayThread(10000);}
 }
 return 0;
}
#else
#include <SDL.h>
static SDL_AudioDeviceID device;
static void callback(void*u,Uint8*b,int n){(void)u;if(paused)memset(b,0,n);else Port_M4A_Backend_Render((int16_t*)b,n/4,false);}
#endif
bool Port_Audio_Init(void){
 if(!Port_M4A_Backend_Init(48000))return false;
 const char *path=getenv("TMC_AUDIO_CAPTURE"); if(path){capture=fopen(path,"wb");return capture!=NULL;}
#ifdef __vita__
 audioPort=sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM,512,48000,SCE_AUDIO_OUT_MODE_STEREO);
 if(audioPort<0){Port_M4A_Backend_Shutdown();return false;}
 playing=1;thread=sceKernelCreateThread("tmc-audio",audioThread,0x10000100,64*1024,0,0,NULL);
 if(thread<0){playing=0;sceAudioOutReleasePort(audioPort);audioPort=-1;Port_M4A_Backend_Shutdown();return false;}
 if(sceKernelStartThread(thread,0,NULL)<0){playing=0;sceKernelDeleteThread(thread);thread=-1;sceAudioOutReleasePort(audioPort);audioPort=-1;Port_M4A_Backend_Shutdown();return false;}return true;
#else
 SDL_AudioSpec want={0},got; want.freq=48000;want.format=AUDIO_S16SYS;want.channels=2;want.samples=512;want.callback=callback;
 device=SDL_OpenAudioDevice(NULL,0,&want,&got,0);if(!device)return false;SDL_PauseAudioDevice(device,0);return true;
#endif
}
void Port_Audio_Shutdown(void){
 if(capture){fclose(capture);capture=NULL;}
#ifdef __vita__
 playing=0;if(thread>=0){sceKernelWaitThreadEnd(thread,NULL,NULL);sceKernelDeleteThread(thread);}if(audioPort>=0)sceAudioOutReleasePort(audioPort);
#else
 if(device)SDL_CloseAudioDevice(device);
#endif
 Port_M4A_Backend_Shutdown();
}
void Port_Audio_Reset(void){Port_M4A_Backend_Reset();}
static void pauseReason(unsigned reason,bool p){
#ifdef __vita__
 while(atomic_flag_test_and_set(&mix_lock))sceKernelDelayThread(100);
 if(p)atomic_fetch_or(&paused,reason);else atomic_fetch_and(&paused,~reason);atomic_flag_clear(&mix_lock);
#else
 if(device)SDL_LockAudioDevice(device);
 if(p)atomic_fetch_or(&paused,reason);else atomic_fetch_and(&paused,~reason);
 if(device)SDL_UnlockAudioDevice(device);
#endif
}
void Port_Audio_3DSSetPaused(bool p){pauseReason(1,p);}
void Vita_AudioSystemPaused(bool p){
 pauseReason(2,p);
#ifdef __vita__
 if(!p)reopen_output=true;
#endif
}
void Port_Audio_SetMasterVolume(float v){Port_M4A_Backend_SetMasterVolume(v);}
float Port_Audio_GetMasterVolume(void){return Port_M4A_Backend_GetMasterVolume();}

void Vita_AudioTick(void){if(capture&&!paused){int16_t b[804*2];Port_M4A_Backend_Render(b,804,false);fwrite(b,4,804,capture);}}
