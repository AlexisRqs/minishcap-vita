#include "vita_menu.h"
#include "port-menu/port_backup.h"
#include "port-menu/port_runtime.h"
#include "port_runtime_config.h"
#include "port_save.h"
#include "port_audio.h"
#include "cpu/mode1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
static const char *const scales[]={"3x","Max","2x"};
static const char *const filters[]={"Net","Lisse"};
static const char *const buttons[]={"A","B","L","R","Start","Select","Aucun"};
static const uint16_t targets[]={1,2,512,256,8,4,0};
static PB_Store backups={"tmc_eu.sav","tmc-backup",8192,Port_Save_ValidateBackup};
static PB_Info copies[PB_SLOTS],previousCopy;
static unsigned copySlots[PB_SLOTS];
static char copyLabels[PB_SLOTS][32],copyHelp[PB_SLOTS][256],copyConfirm[PB_SLOTS][256],previousHelp[256],previousConfirm[256];
static PM_Menu menu;
static bool resetSettings(void *ctx){PM_Reset(ctx);return true;}
extern bool Vita_ProfileExport(void);
static bool exportProfile(void *ctx){(void)ctx;return Vita_ProfileExport();}
static void refresh(void){
 PB_List(&backups,copies,copySlots);
 for(unsigned i=0;i<PB_SLOTS;i++){
  char date[32];PB_Date(copies[i].timestamp,date,sizeof(date));
  snprintf(copyLabels[i],sizeof(copyLabels[i]),"%u %s",i+1,copies[i].valid?date:"Indisponible");
  snprintf(copyHelp[i],sizeof(copyHelp[i]),"%s. %s. Les trois fichiers de partie sont inclus. Les copies sont triees de la plus recente a la plus ancienne.",date,i==0?"Copie la plus recente":"Copie precedente");
  snprintf(copyConfirm[i],sizeof(copyConfirm[i]),"Restaurer copie %u du %s ? Progres non sauvegarde perdu. La sauvegarde actuelle sera preservee. Le jeu sera ferme.",i+1,date);
 }
 PB_PreviousInfo(&backups,&previousCopy);char date[32];PB_Date(previousCopy.timestamp,date,sizeof(date));
 snprintf(previousHelp,sizeof(previousHelp),"Sauvegarde mise de cote avant la derniere restauration : %s. La restaurer conserve la sauvegarde actuelle dans les copies automatiques.",date);
 snprintf(previousConfirm,sizeof(previousConfirm),"Revenir avant restauration (%s) ? Progres non sauvegarde perdu. La sauvegarde actuelle sera copiee. Le jeu sera ferme.",date);
}
static bool saveReady(void *ctx){(void)ctx;PortSaveStats s;Port_Save_GetStats(&s);return s.initialized&&!s.dirty&&!s.transactionDepth&&!strcmp(s.activePath,backups.path);}
static bool ready0(void*c){return saveReady(c)&&copies[0].valid;}
static bool ready1(void*c){return saveReady(c)&&copies[1].valid;}
static bool ready2(void*c){return saveReady(c)&&copies[2].valid;}
static bool readyPrevious(void*c){return saveReady(c)&&previousCopy.valid;}
static bool restorePrevious(void*c){if(!saveReady(c)||!PB_RestorePrevious(&backups))return false;fprintf(stderr,"Previous backup restored; closing.\n");exit(0);}
static bool capture(void *ctx){(void)ctx;bool ok=PB_Capture(&backups);refresh();return ok;}
static bool restore(unsigned slot){if(!saveReady(NULL)||!PB_Restore(&backups,slot))return false;fprintf(stderr,"Backup restored; closing before stale game memory can write.\n");exit(0);}
static bool restore0(void*c){(void)c;return restore(copySlots[0]);}
static bool restore1(void*c){(void)c;return restore(copySlots[1]);}
static bool restore2(void*c){(void)c;return restore(copySlots[2]);}
#define OPT(id,label,type,v,min,max,step,choices,group,help) {id,label,type,v,min,max,step,choices,NULL,NULL,NULL,group,help}
#define MAP(id,label,v) OPT(id,label,PM_CHOICE,v,0,6,1,buttons,"Commandes","Action GBA de ce bouton. Triangle et les controles du menu restent fixes. Plusieurs boutons peuvent partager une action.")
static PM_Option options[]={
 OPT("show_fps","Afficher les FPS",PM_BOOL,0,0,1,1,NULL,"Affichage","Affiche le nombre d'images presentees par seconde."),
 OPT("left_stick","Stick gauche",PM_BOOL,1,0,1,1,NULL,"Commandes","Active le stick gauche. La croix directionnelle reste disponible."),
 OPT("volume","Volume (%)",PM_RANGE,100,0,100,10,NULL,"Audio","Volume du jeu. La musique reprend au meme endroit apres le menu."),
 OPT("display","Taille image",PM_CHOICE,0,0,2,1,scales,"Affichage","3x: pixels entiers. Max: hauteur de l'ecran sans deformation. 2x: image plus petite."),
 OPT("filter","Filtre",PM_CHOICE,0,0,1,1,filters,"Affichage","Net conserve les pixels. Lisse adoucit l'image, surtout en taille Max."),
 OPT("color","Couleurs GBA",PM_BOOL,0,0,1,1,NULL,"Affichage","Correction des couleurs pour se rapprocher de l'ecran GBA."),
 OPT("deadzone","Zone morte (%)",PM_RANGE,30,10,60,5,NULL,"Commandes","Augmenter cette valeur si Link se deplace seul. Le stick doit revenir dans cette zone apres le menu."),
 OPT("fast_boot","Demarrage rapide",PM_BOOL,0,0,1,1,NULL,"General","Au prochain lancement, ouvre la selection des sauvegardes sans les logos ni le titre. Ne charge aucune partie automatiquement."),
 OPT("performance","Mesures detail",PM_BOOL,0,0,1,1,NULL,"Performances","Affiche le cout moyen du rendu logiciel et de la presentation. Les mesures des 60 dernieres secondes sont exportables."),
 MAP("map_cross","Croix",0),MAP("map_circle","Rond",1),MAP("map_square","Carre",6),MAP("map_l","Gachette L",2),MAP("map_r","Gachette R",3),MAP("map_start","Start",4),MAP("map_select","Select",5),
 {"backup_now","Copier maintenant",PM_ACTION,0,0,0,1,NULL,NULL,saveReady,capture,"Sauvegardes","Copie la derniere sauvegarde ecrite par le jeu, pas la partie en memoire. Trois copies differentes sont conservees automatiquement."},
 {"restore0",copyLabels[0],PM_ACTION,0,0,0,1,NULL,copyConfirm[0],ready0,restore0,"Sauvegardes",copyHelp[0]},
 {"restore1",copyLabels[1],PM_ACTION,0,0,0,1,NULL,copyConfirm[1],ready1,restore1,"Sauvegardes",copyHelp[1]},
 {"restore2",copyLabels[2],PM_ACTION,0,0,0,1,NULL,copyConfirm[2],ready2,restore2,"Sauvegardes",copyHelp[2]},
 {"restore_previous","Avant restauration",PM_ACTION,0,0,0,1,NULL,previousConfirm,readyPrevious,restorePrevious,"Sauvegardes",previousHelp},
 {"export_perf","Exporter mesures",PM_ACTION,0,0,0,1,NULL,NULL,NULL,exportProfile,"Performances","Ecrit performance.csv dans le dossier du jeu. PPU: rendu logiciel. Presentation: copie et attente GPU. Les pauses sont exclues."},
 {"defaults","Reglages origine",PM_ACTION,0,0,0,1,NULL,"Retablir les reglages du portage? La sauvegarde du jeu et ses copies seront conservees.",NULL,resetSettings,"General","Retablit les commandes, l'affichage et le son par defaut. Ne supprime aucune sauvegarde."}
};
static void apply(void){Port_Audio_SetMasterVolume(options[VM_VOLUME].value/100.0f);virtuappu_mode1_set_color_correction(options[VM_COLOR].value!=0);}
bool Vita_BackupRecover(void){return PB_Recover(&backups);}
void Vita_BackupOnSave(const char *path){if(!strcmp(path,backups.path)&&!PB_Capture(&backups))fprintf(stderr,"WARNING: backup unavailable; primary save was committed.\n");}
void Vita_MenuInit(void){
 /* Preserve the 0.03 settings path and stable identifiers. */
 options[VM_VOLUME].value=(int)(Port_Config_GetMasterVolume()*100+.5f);
 options[VM_COLOR].value=Port_Config_GetColorCorrection();options[VM_FPS].value=Port_Config_GetShowFps();
 PM_Init(&menu,"Minish Cap - 0.06","vita-menu.ini",options,sizeof(options)/sizeof(options[0]),&menu);
 /* Factory reset is independent of existing legacy configuration. */
 menu.defaults[VM_VOLUME]=100;menu.defaults[VM_COLOR]=menu.defaults[VM_FPS]=0;
 PM_Load(&menu);refresh();apply();
}
int Vita_MenuValue(unsigned option){return option<VM_COUNT?options[option].value:0;}
uint16_t Vita_MapButtons(unsigned physical){int map[7];for(int i=0;i<7;i++)map[i]=options[VM_MAP_X+i].value;return PR_MapButtons(physical,map,targets,7,7);}
bool Vita_FastBoot(void){static bool consumed;bool fast=!consumed&&options[VM_FAST].value;consumed=true;return fast;}
void Vita_MenuOpen(unsigned held){refresh();PM_Open(&menu,held);}
bool Vita_MenuActive(void){return menu.open;}
void Vita_MenuUpdate(unsigned held){bool active=menu.open;PM_Update(&menu,held);if(active&&!menu.open)apply();}
void Vita_MenuDraw(uint32_t *pixels){PM_Draw(&menu,pixels,240);}
