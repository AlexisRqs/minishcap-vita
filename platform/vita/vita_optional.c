#include <stdbool.h>
/* No update overlay or second screen on Vita. */
bool Port_SecondScreen_3DS_UpdateOpen(void){return false;}
#include <stddef.h>
#include <stdlib.h>
void* linearAlloc(size_t n){return malloc(n);}
void linearFree(void*p){free(p);}
void Port_SecondScreenState_Publish(void){}
void Port_PPU_CyclePresentationMode(int n){(void)n;}
void Port_PPU_CycleWindowScale(int n){(void)n;}
bool Port_PPU_IsFullscreen(void){return true;}
const char* Port_PPU_PresentationModeName(void){return "Nearest";}
void Port_PPU_ToggleFullscreen(void){}
unsigned char Port_PPU_WindowScale(void){return 3;}
