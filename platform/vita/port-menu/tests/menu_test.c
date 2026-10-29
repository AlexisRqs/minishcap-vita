#include "port_menu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
static int actions;
static bool action(void *ctx){(*(int*)ctx)++;return true;}
static void tap(PM_Menu *m,unsigned key){PM_Update(m,0);PM_Update(m,key);PM_Update(m,0);}
int main(int argc,char **argv){
 assert(argc==2);char path[256];snprintf(path,sizeof(path),"%s/settings.ini",argv[1]);
 const char *const choices[]={"3x","Max","2x"};
 PM_Option options[]={
 {"fps","Afficher les FPS",PM_BOOL,0,0,1,1},
 {"volume","Volume (%)",PM_RANGE,100,0,100,10},
 {"size","Taille image",PM_CHOICE,0,0,2,1,choices},
 {"restore","Restaurer",PM_ACTION,0,0,0,1,NULL,"Confirmer restauration?",NULL,action},
 {"a","Option A",PM_BOOL,0,0,1,1},{"b","Option B",PM_BOOL,0,0,1,1},
 {"c","Option C",PM_BOOL,0,0,1,1},{"d","Option D",PM_BOOL,0,0,1,1}
 };
 PM_Menu m;assert(PM_Init(&m,"Jeu A",path,options,8,&actions));
 PM_Open(&m,PM_TOGGLE);PM_Update(&m,PM_TOGGLE);assert(m.open);
 tap(&m,PM_ACCEPT);assert(options[0].value==1);
 PM_Update(&m,PM_ACCEPT);PM_Update(&m,PM_ACCEPT);assert(options[0].value==0);PM_Update(&m,0);
 tap(&m,PM_DOWN);tap(&m,PM_RIGHT);assert(options[1].value==100);
 tap(&m,PM_LEFT);assert(options[1].value==90);
 tap(&m,PM_DOWN);tap(&m,PM_LEFT);assert(options[2].value==2);
 tap(&m,PM_DOWN);tap(&m,PM_ACCEPT);assert(m.confirming&&actions==0);
 tap(&m,PM_BACK);assert(m.open&&!m.confirming&&actions==0);
 tap(&m,PM_ACCEPT);tap(&m,PM_ACCEPT);assert(actions==1);
 tap(&m,PM_BACK);assert(!m.open);
 options[1].value=20;assert(PM_Load(&m));assert(options[1].value==90);
 /* Stable identifiers survive reordering and additions; bad values ignored. */
 FILE *f=fopen(path,"wb");assert(f);fputs("port_menu=1\nsize=1\nvolume=999\nunknown=4\nfps=abc\n",f);fclose(f);
 assert(PM_Load(&m)&&options[1].value==90&&options[2].value==1);
 PM_Open(&m,0);m.selected=7;
 uint32_t pixels[240*160+2];pixels[0]=pixels[240*160+1]=0x12345678;
 PM_Draw(&m,pixels+1,240);assert(pixels[0]==0x12345678&&pixels[240*160+1]==0x12345678);
 tap(&m,PM_DOWN);assert(m.selected==0);tap(&m,PM_UP);assert(m.selected==7);
 const char *good=m.settings_path;m.settings_path="/nonexistent-port-menu/settings.ini";
 tap(&m,PM_ACCEPT);tap(&m,PM_BACK);assert(m.open&&m.save_failed);
 tap(&m,PM_START);assert(!m.open&&m.dirty);m.settings_path=good;
 PM_Open(&m,0);tap(&m,PM_BACK);assert(!m.open&&!m.dirty);
 /* A missing primary after interrupted rotation recovers .bak. */
 char backup[264];snprintf(backup,sizeof(backup),"%s.bak",path);remove(backup);assert(rename(path,backup)==0);
 options[1].value=0;assert(PM_Load(&m)&&options[1].value==90);
 PM_Option duplicate[2]={options[0],options[0]};assert(!PM_Init(&m,"Bad",path,duplicate,2,0));
 for(int i=0;i<8;i++){options[i].category=i%2?"Audio":"Image";options[i].help="Aide de cette option.";}
 assert(PM_Init(&m,"Grouped",path,options,8,&actions));PM_Open(&m,0);assert(m.root&&m.category_count==2);
 tap(&m,PM_DOWN);tap(&m,PM_ACCEPT);assert(!m.root&&m.selected==1);
 tap(&m,PM_DOWN);assert(m.selected==3);tap(&m,PM_HELP);assert(m.help_open);
 PM_Draw(&m,pixels+1,240);tap(&m,PM_BACK);assert(!m.help_open&&!m.root);
 tap(&m,PM_BACK);assert(m.root);tap(&m,PM_ACCEPT);assert(m.selected==1);
 int initial=options[1].value;tap(&m,PM_LEFT);PM_Reset(&m);assert(options[1].value==initial);
 tap(&m,PM_TOGGLE);assert(!m.open);options[0].category=NULL;assert(!PM_Init(&m,"Mixed",path,options,8,0));
 puts("PASS: edges, ranges, choices, actions, pagination, persistence, recovery, write errors");
}
