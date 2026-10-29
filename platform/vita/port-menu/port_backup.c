/* SPDX-License-Identifier: MIT */
#include "port_backup.h"
#include "port_model.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
static bool validstore(const PB_Store *s){return s&&s->path&&s->prefix&&s->path[0]&&s->prefix[0]&&strlen(s->path)<=280&&strlen(s->prefix)<=280&&s->size&&s->size<=16*1024*1024;}
static uint32_t crc(const unsigned char *b,size_t n){uint32_t c=~0u;while(n--){c^=*b++;for(int k=0;k<8;k++)c=(c>>1)^((0u-(c&1))&0xedb88320u);}return ~c;}
static void put(unsigned char *p,uint64_t v,int n){while(n--){*p++=(unsigned char)v;v>>=8;}}
static uint64_t get(const unsigned char *p,int n){uint64_t v=0;for(int i=n-1;i>=0;i--)v=(v<<8)|p[i];return v;}
static bool name(char *out,size_t cap,const char *base,const char *suffix){int n=snprintf(out,cap,"%s%s",base,suffix);return n>0&&(size_t)n<cap;}
static bool slotname(const PB_Store *s,unsigned slot,char *out){int n=snprintf(out,320,"%s.%u.pmb",s->prefix,slot);return slot<PB_SLOTS&&n>0&&n<320;}
static bool readraw(const char *p,unsigned char *b,size_t n){FILE*f=fopen(p,"rb");if(!f)return false;bool ok=fread(b,1,n,f)==n&&fgetc(f)==EOF&&!ferror(f);fclose(f);return ok;}
static bool writefile(const char *p,const void *a,size_t na,const void *b,size_t nb){FILE*f=fopen(p,"wb");if(!f)return false;bool ok=fwrite(a,1,na,f)==na&&(!nb||fwrite(b,1,nb,f)==nb);if(fflush(f)||fsync(fileno(f)))ok=false;if(fclose(f))ok=false;return ok;}
static bool readslot(const PB_Store*s,unsigned slot,PB_Info *info,unsigned char *data){
 char p[320];unsigned char h[32];if(!slotname(s,slot,p))return false;FILE*f=fopen(p,"rb");if(!f)return false;
 bool ok=fread(h,1,32,f)==32&&!memcmp(h,"PMBACK1",8)&&get(h+8,4)==s->size&&get(h+28,4)==crc((const unsigned char*)s->path,strlen(s->path));
 if(ok)ok=fread(data,1,s->size,f)==s->size&&fgetc(f)==EOF&&!ferror(f)&&get(h+12,4)==crc(data,s->size);
 fclose(f);if(ok&&info)*info=(PB_Info){true,(uint32_t)get(h+16,4),get(h+20,8)};return ok;
}
bool PB_InfoAt(const PB_Store*s,unsigned slot,PB_Info *info){
 if(info)memset(info,0,sizeof(*info));if(!validstore(s))return false;
 unsigned char*b=malloc(s->size);if(!b)return false;bool ok=readslot(s,slot,info,b);free(b);return ok;
}
static bool install(const char *temp,const char *path){
 char old[352];if(!name(old,sizeof(old),path,".old"))return false;
 bool exists=access(path,F_OK)==0;
 if(exists){if(remove(old)&&errno!=ENOENT)return false;if(rename(path,old))return false;}
 if(rename(temp,path)){if(exists)rename(old,path);return false;}remove(old);return true;
}
bool PB_Capture(const PB_Store*s){
 if(!validstore(s))return false;
 unsigned char *data=malloc(s->size),*other=malloc(s->size);if(!data||!other){free(data);free(other);return false;}
 bool ok=false;if(!readraw(s->path,data,s->size)||(s->validate&&!s->validate(s->path)))goto done;
 PB_Info info[PB_SLOTS];unsigned target=0;uint32_t max=0,min=~0u;int newest=-1,empty=-1;
 for(unsigned i=0;i<PB_SLOTS;i++){
  info[i]=(PB_Info){0};
  if(!readslot(s,i,&info[i],other)){if(empty<0)empty=(int)i;continue;}
  if(info[i].generation>=max){max=info[i].generation;newest=(int)i;}
  if(info[i].generation<min){min=info[i].generation;target=i;}
 }
 if(newest>=0&&readslot(s,(unsigned)newest,NULL,other)&&!memcmp(data,other,s->size)){ok=true;goto done;}
 if(max==UINT32_MAX)goto done;if(empty>=0)target=(unsigned)empty;
 char path[320],temp[352];unsigned char h[32]={0};slotname(s,target,path);name(temp,sizeof(temp),path,".tmp");
 memcpy(h,"PMBACK1",8);put(h+8,s->size,4);put(h+12,crc(data,s->size),4);put(h+16,max+1,4);put(h+20,(uint64_t)time(NULL),8);put(h+28,crc((const unsigned char*)s->path,strlen(s->path)),4);
 ok=writefile(temp,h,32,data,s->size)&&install(temp,path);if(!ok)remove(temp);
 done:free(data);free(other);return ok;
}
bool PB_Recover(const PB_Store*s){
 if(!validstore(s))return false;
 char rollback[352],temp[352];if(!name(rollback,sizeof(rollback),s->path,".restore.rollback")||!name(temp,sizeof(temp),s->path,".restore.tmp"))return false;
 if(access(rollback,F_OK)==0){
  bool valid=access(s->path,F_OK)==0&&(!s->validate||s->validate(s->path));
  if(!valid){if(access(s->path,F_OK)==0)return false;if(rename(rollback,s->path))return false;}
  else remove(rollback);
 }
 char before[352],beforeOld[384];name(before,sizeof(before),s->path,".before-restore");name(beforeOld,sizeof(beforeOld),before,".old");
 if(access(before,F_OK)!=0&&access(beforeOld,F_OK)==0&&rename(beforeOld,before))return false;
 remove(temp);
 /* Recover interrupted replacement of a rotating slot before it is reused. */
 for(unsigned i=0;i<PB_SLOTS;i++){char p[320],old[352];slotname(s,i,p);name(old,sizeof(old),p,".old");if(access(p,F_OK)!=0&&access(old,F_OK)==0&&rename(old,p))return false;}
 return true;
}
bool PB_Restore(const PB_Store*s,unsigned slot){
 if(!validstore(s)||!PB_Recover(s))return false;
 unsigned char *data=malloc(s->size),*current=malloc(s->size);if(!data||!current){free(data);free(current);return false;}
 bool ok=false;char temp[352],old[352],before[352],bt[384];
 if(!name(temp,sizeof(temp),s->path,".restore.tmp")||!name(old,sizeof(old),s->path,".restore.rollback")||!name(before,sizeof(before),s->path,".before-restore")||!name(bt,sizeof(bt),before,".tmp"))goto done;
 if(!readslot(s,slot,NULL,data)||!readraw(s->path,current,s->size))goto done;
 if(!writefile(temp,data,s->size,NULL,0)||(s->validate&&!s->validate(temp)))goto done;
 /* Keep a separate durable copy of current progress, outside the rotating slots. */
 if(!writefile(bt,current,s->size,NULL,0)||!install(bt,before))goto done;
 if(rename(s->path,old))goto done;
 if(rename(temp,s->path)){rename(old,s->path);goto done;}
 ok=true;remove(old);
 done:free(data);free(current);return ok;
}

void PB_List(const PB_Store*s,PB_Info entries[PB_SLOTS],unsigned slots[PB_SLOTS]){
 PB_Info raw[PB_SLOTS];uint64_t gen[PB_SLOTS];bool valid[PB_SLOTS];
 for(unsigned i=0;i<PB_SLOTS;i++){PB_InfoAt(s,i,&raw[i]);gen[i]=raw[i].generation;valid[i]=raw[i].valid;}
 PM_SortNewest(slots,gen,valid,PB_SLOTS);for(unsigned i=0;i<PB_SLOTS;i++)entries[i]=raw[slots[i]];
}
void PB_Date(uint64_t timestamp,char*out,size_t size){if(!PM_FormatDate(timestamp,out,size)&&size)snprintf(out,size,"date inconnue");}
bool PB_PreviousInfo(const PB_Store*s,PB_Info*info){
 if(info)memset(info,0,sizeof(*info));if(!validstore(s))return false;
 char path[352];name(path,sizeof(path),s->path,".before-restore");struct stat st;
 if(stat(path,&st)||st.st_size!=(off_t)s->size||(s->validate&&!s->validate(path)))return false;
 if(info)*info=(PB_Info){true,0,st.st_mtime>0?(uint64_t)st.st_mtime:0};return true;
}
bool PB_RestorePrevious(const PB_Store*s){
 /* Keep the previous file untouched; preserve current progress in the rotation. */
 if(!validstore(s)||!PB_Recover(s)||!PB_PreviousInfo(s,NULL))return false;
 char before[352],temp[352],rollback[352];name(before,sizeof(before),s->path,".before-restore");name(temp,sizeof(temp),s->path,".restore.tmp");name(rollback,sizeof(rollback),s->path,".restore.rollback");
 unsigned char*data=malloc(s->size);if(!data)return false;
 bool ok=readraw(before,data,s->size)&&writefile(temp,data,s->size,NULL,0)&&(!s->validate||s->validate(temp));free(data);
 if(!ok||!PB_Capture(s))return false;
 if(rename(s->path,rollback))return false;
 if(rename(temp,s->path)){rename(rollback,s->path);return false;}
 remove(rollback);return true;
}
