#include <SDL.h>
#include <stdatomic.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "port_audio.h"
static atomic_uint mixed;
bool Port_M4A_Backend_Init(uint32_t rate){return rate==48000;}
void Port_M4A_Backend_Shutdown(void){}
void Port_M4A_Backend_Reset(void){}
void Port_M4A_Backend_SetMasterVolume(float v){(void)v;}
float Port_M4A_Backend_GetMasterVolume(void){return 1;}
void Port_M4A_Backend_Render(int16_t *out,uint32_t frames,bool mute){(void)mute;memset(out,0,frames*4);mixed+=frames;}
extern void Port_Audio_3DSSetPaused(bool);
extern void Vita_AudioSystemPaused(bool);
int main(void){
 assert(SDL_Init(SDL_INIT_AUDIO)==0);assert(Port_Audio_Init());SDL_Delay(80);
 assert(mixed>0);
 for(int i=0;i<3;i++){
  Port_Audio_3DSSetPaused(true);unsigned before=mixed;SDL_Delay(80);assert(mixed==before);
  Port_Audio_3DSSetPaused(false);SDL_Delay(80);assert(mixed>before);
 }
 Port_Audio_3DSSetPaused(true);Vita_AudioSystemPaused(true);unsigned before=mixed;
 Port_Audio_3DSSetPaused(false);SDL_Delay(80);assert(mixed==before);
 Vita_AudioSystemPaused(false);SDL_Delay(80);assert(mixed>before);
 Vita_AudioSystemPaused(true);Port_Audio_3DSSetPaused(true);before=mixed;
 Vita_AudioSystemPaused(false);SDL_Delay(80);assert(mixed==before);
 Port_Audio_3DSSetPaused(false);SDL_Delay(80);assert(mixed>before);
 Port_Audio_Shutdown();SDL_Quit();puts("PASS: mixer frozen while paused, resumes without reset (3 cycles and overlapping system/menu pauses)");
}
