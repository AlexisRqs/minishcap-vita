/* SPDX-License-Identifier: MIT */
#include "port_menu.h"
#include "port_model.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include "font8x8.inc"
static bool enabled(const PM_Menu *m,size_t i){return !m->options[i].enabled||m->options[i].enabled(m->context);}
bool PM_Init(PM_Menu *m,const char *game,const char *path,PM_Option *options,size_t count,void *context){
 if(!m||!game||!path||!options||!count||count>PM_MAX_OPTIONS||strlen(path)>240)return false;
 for(size_t i=0;i<count;i++){
  PM_Option *o=&options[i];
  if(!o->id||!o->label||!o->id[0]||strlen(o->id)>63||strspn(o->id,"abcdefghijklmnopqrstuvwxyz0123456789_")!=strlen(o->id))return false;
  if(o->category&&!o->category[0])return false;
  if(o->type<PM_BOOL||o->type>PM_ACTION)return false;
  if(o->type!=PM_ACTION&&(o->minimum>o->maximum||o->value<o->minimum||o->value>o->maximum||o->step<1))return false;
  if(o->type==PM_BOOL&&(o->minimum!=0||o->maximum!=1))return false;
  if(o->type==PM_CHOICE&&(!o->choices||o->minimum!=0))return false;
  for(size_t j=0;j<i;j++)if(!strcmp(o->id,options[j].id))return false;
 }
 /* Categorized profiles must categorize every option; flat profiles remain compatible. */
 bool grouped=false,flat=false;for(size_t i=0;i<count;i++){if(options[i].category&&options[i].category[0])grouped=true;else flat=true;}
 if(grouped&&flat)return false;
 *m=(PM_Menu){.game_name=game,.settings_path=path,.options=options,.count=count,.context=context,
 .theme={0xff28384c,0xff669acd,0xffffffff,0xffb6e4ff}};
 for(size_t i=0;i<count;i++){
  m->defaults[i]=options[i].value;
  if(options[i].category){size_t g=0;for(;g<m->category_count;g++)if(!strcmp(options[i].category,m->categories[g]))break;
   if(g==m->category_count)m->categories[m->category_count++]=options[i].category;}
 }
 return true;
}
static bool load_file(PM_Menu *m,const char *path){
 FILE *f=fopen(path,"rb");if(!f)return false;
 char line[192],key[64],val[64],extra; bool valid=false;
 if(fgets(line,sizeof(line),f)&&!strcmp(line,"port_menu=1\n"))valid=true;
 if(valid)while(fgets(line,sizeof(line),f)){
  if(sscanf(line,"%63[^=]=%63s %c",key,val,&extra)!=2)continue;
  char *end;errno=0;long n=strtol(val,&end,10);if(errno||*end||n<INT_MIN||n>INT_MAX)continue;
  for(size_t i=0;i<m->count;i++){
   PM_Option *o=&m->options[i];
   if(o->type!=PM_ACTION&&!strcmp(key,o->id)&&n>=o->minimum&&n<=o->maximum)o->value=(int)n;
  }
 }
 fclose(f);return valid;
}
bool PM_Load(PM_Menu *m){
 if(load_file(m,m->settings_path))return true;
 char backup[256];snprintf(backup,sizeof(backup),"%s.bak",m->settings_path);
 return load_file(m,backup);
}
bool PM_Save(PM_Menu *m){
 char temp[256],backup[256];snprintf(temp,sizeof(temp),"%s.tmp",m->settings_path);snprintf(backup,sizeof(backup),"%s.bak",m->settings_path);
 FILE *f=fopen(temp,"wb");if(!f)return false;
 bool ok=fprintf(f,"port_menu=1\n")>0;
 for(size_t i=0;i<m->count;i++)if(m->options[i].type!=PM_ACTION)
  if(fprintf(f,"%s=%d\n",m->options[i].id,m->options[i].value)<0)ok=false;
 if(fflush(f)||fsync(fileno(f)))ok=false;
 if(fclose(f))ok=false;
 if(!ok){remove(temp);return false;}
 bool current=access(m->settings_path,F_OK)==0;
 if(current){
  if(remove(backup)&&errno!=ENOENT){remove(temp);return false;}
  if(rename(m->settings_path,backup)){remove(temp);return false;}
 }
 if(rename(temp,m->settings_path)){if(current)rename(backup,m->settings_path);remove(temp);return false;}
 m->dirty=false;return true;
}
void PM_Reset(PM_Menu *m){for(size_t i=0;i<m->count;i++)if(m->options[i].type!=PM_ACTION)m->options[i].value=m->defaults[i];m->dirty=true;}
static size_t visible(const PM_Menu *m,size_t *indices){
 size_t n=0;for(size_t i=0;i<m->count;i++)if(!m->category_count||(m->options[i].category&&!strcmp(m->options[i].category,m->categories[m->category])))indices[n++]=i;return n;
}
void PM_Open(PM_Menu *m,unsigned held){m->open=true;m->previous=held;m->confirming=false;m->save_failed=false;m->notice[0]=0;m->root=m->category_count>0;m->help_open=false;}
void PM_Update(PM_Menu *m,unsigned held){
 unsigned pressed=held&~m->previous;m->previous=held;if(!m->open)return;
 if(m->help_open){if(pressed&(PM_BACK|PM_TOGGLE|PM_START|PM_HELP|PM_ACCEPT))m->help_open=false;return;}
 if(m->confirming){
  if(pressed&(PM_BACK|PM_TOGGLE|PM_START)){m->confirming=false;return;}
  if(pressed&PM_ACCEPT){PM_Option *o=&m->options[m->selected];bool ok=enabled(m,m->selected)&&o->action&&o->action(m->context);snprintf(m->notice,sizeof(m->notice),"%s",ok?"Action terminee":"Action impossible");m->confirming=false;}
  return;
 }
 if((pressed&PM_BACK)&&m->category_count&&!m->root&&!m->save_failed){m->root=true;return;}
 if(pressed&(PM_BACK|PM_TOGGLE|PM_START)){
  if((m->save_failed&&(pressed&PM_START))||(!m->dirty)||PM_Save(m)){m->open=false;return;}
  m->save_failed=true;snprintf(m->notice,sizeof(m->notice),"Non sauve: START pour sortir");return;
 }
 if(m->root){
  if(pressed&PM_UP)m->category=(m->category+m->category_count-1)%m->category_count;
  else if(pressed&PM_DOWN)m->category=(m->category+1)%m->category_count;
  if(pressed&PM_ACCEPT){size_t indices[PM_MAX_OPTIONS];if(visible(m,indices)){m->selected=indices[0];m->root=false;}}
  return;
 }
 size_t indices[PM_MAX_OPTIONS],n=visible(m,indices),at=0;if(!n)return;
 for(size_t i=0;i<n;i++)if(indices[i]==m->selected)at=i;
 if(pressed&PM_UP)at=(at+n-1)%n;else if(pressed&PM_DOWN)at=(at+1)%n;
 m->selected=indices[at];
 if(pressed&PM_HELP){m->help_open=true;return;}
 if(!(pressed&(PM_ACCEPT|PM_LEFT|PM_RIGHT))||!enabled(m,m->selected))return;
 PM_Option *o=&m->options[m->selected];
 if(o->type==PM_ACTION){
  if(!(pressed&PM_ACCEPT))return;
  if(o->confirmation)m->confirming=true;
  else snprintf(m->notice,sizeof(m->notice),"%s",o->action&&o->action(m->context)?"Action terminee":"Action impossible");
  return;
 }
 if(PM_AdjustOption(o,(pressed&PM_LEFT)?-1:1)){m->dirty=true;m->save_failed=false;m->notice[0]=0;}
}
void PM_Text(uint32_t *pixels,size_t stride,unsigned x,unsigned y,const char *text,uint32_t color){
 if(!pixels||stride<240||!text)return;
 for(;*text&&x+8<=240;text++,x+=8){
  unsigned ch=(unsigned char)*text;if(ch<32||ch>126)ch='?';
  const unsigned char *glyph=pm_font+ch*8;
  for(unsigned r=0;r<8&&y+r<160;r++)for(unsigned b=0;b<8;b++)if(glyph[r]&(0x80u>>b))pixels[(y+r)*stride+x+b]=color;
 }
}
static void paragraph(uint32_t *pixels,size_t stride,unsigned y,const char *s,uint32_t color){
 if(!s)s="Aucune aide disponible.";
 for(unsigned line=0;*s&&line<8;line++){
  size_t n=strlen(s);if(n>28)n=28;
  if(s[n]&&n==28){size_t split=n;while(split&&s[split]!=' ')split--;if(split)n=split;}
  char text[29];memcpy(text,s,n);text[n]=0;PM_Text(pixels,stride,8,y+line*12,text,color);s+=n;while(*s==' ')s++;
 }
}
void PM_Draw(const PM_Menu *m,uint32_t *pixels,size_t stride){
 if(!pixels||stride<240)return;
 for(unsigned y=0;y<160;y++)for(unsigned x=0;x<240;x++)pixels[y*stride+x]=(x<3||x>=237||y<3||y>=157)?m->theme.border:m->theme.background;
 PM_Text(pixels,stride,8,10,m->category_count&&!m->root?m->categories[m->category]:"PARAMETRES",m->theme.selected);PM_Text(pixels,stride,8,24,m->game_name,m->theme.text);
 if(m->confirming||m->help_open){paragraph(pixels,stride,44,m->confirming?m->options[m->selected].confirmation:m->options[m->selected].help,m->theme.text);PM_Text(pixels,stride,8,144,m->confirming?"X : confirmer  O : annuler":"O : retour",m->theme.selected);return;}
 size_t indices[PM_MAX_OPTIONS],n=m->root?m->category_count:visible(m,indices),at=m->root?m->category:0;
 if(!m->root)for(size_t i=0;i<n;i++)if(indices[i]==m->selected)at=i;
 size_t first=(at/PM_PAGE_ROWS)*PM_PAGE_ROWS;
 for(size_t row=first;row<n&&row<first+PM_PAGE_ROWS;row++){
  char line[32];
  if(m->root)snprintf(line,sizeof(line),"%c %s",row==at?'>':' ',m->categories[row]);
  else{
   size_t i=indices[row];const PM_Option *o=&m->options[i];char value[16]="";
   if(!enabled(m,i))snprintf(value,sizeof(value),"--");
   else if(o->type==PM_BOOL)snprintf(value,sizeof(value),"%s",o->value?"OUI":"NON");
   else if(o->type==PM_CHOICE)snprintf(value,sizeof(value),"%s",o->choices[o->value]);
   else if(o->type==PM_RANGE)snprintf(value,sizeof(value),"%d",o->value);
   if(o->type==PM_ACTION)snprintf(line,sizeof(line),"%c %.24s%s",row==at?'>':' ',o->label,enabled(m,i)?"":" --");
   else snprintf(line,sizeof(line),"%c %-16.16s %.8s",row==at?'>':' ',o->label,value);
  }
  PM_Text(pixels,stride,8,40+(unsigned)(row-first)*12,line,row==at?m->theme.selected:m->theme.text);
 }
 if(m->notice[0])PM_Text(pixels,stride,8,128,m->notice,m->theme.selected);
 else if(n>PM_PAGE_ROWS){char page[24];snprintf(page,sizeof(page),"Page %u/%u",(unsigned)(first/PM_PAGE_ROWS+1),(unsigned)((n+PM_PAGE_ROWS-1)/PM_PAGE_ROWS));PM_Text(pixels,stride,8,128,page,m->theme.selected);}
 PM_Text(pixels,stride,8,144,m->root?"X : ouvrir  O : reprendre":"X/G-D : changer  Carre:aide",m->theme.text);
}
