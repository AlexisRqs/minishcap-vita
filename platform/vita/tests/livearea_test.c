#include "vita_livearea.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 assert(Vita_IsSettingsLaunch("-settings"));
 assert(Vita_IsSettingsLaunch("type=LAUNCH_APP_BY_URI&uri=psla:-settings"));
 assert(Vita_IsSettingsLaunch("argument=-settings&other=1"));
 assert(!Vita_IsSettingsLaunch(NULL));assert(!Vita_IsSettingsLaunch(""));
 assert(!Vita_IsSettingsLaunch("-settings-other"));assert(!Vita_IsSettingsLaunch("no-settings"));
 puts("PASS: LiveArea same-app launch parameter parsing");
}
