import struct,sys,io,json,os
from pathlib import Path
from elftools.elf.elffile import ELFFile
from unicorn import *
from unicorn.arm_const import *
elfpath=sys.argv[1]
out=Path(sys.argv[3]) if len(sys.argv)>3 else Path('work/arm-test');out.mkdir(parents=True,exist_ok=True)
inputs=[tuple(int(x,16 if i==2 else 10) for i,x in enumerate(line.split())) for line in Path(sys.argv[4]).read_text().splitlines()] if len(sys.argv)>4 else []
with open(elfpath,'rb') as f:
 e=ELFFile(f); sy={s.name:int(s['st_value']) for s in e.get_section_by_name('.symtab').iter_symbols()}; sizes={s.name:int(s['st_size']) for s in e.get_section_by_name('.symtab').iter_symbols()}; seg=[(int(s['p_vaddr']),int(s['p_memsz']),s.data()) for s in e.iter_segments() if s['p_type']=='PT_LOAD']
u=Uc(UC_ARCH_ARM,UC_MODE_ARM);u.reg_write(UC_ARM_REG_C1_C0_2,0xf00000);u.reg_write(UC_ARM_REG_FPEXC,0x40000000)
for a,n,d in seg:
 lo=a&~4095;u.mem_map(lo,((a+n+4095)&~4095)-lo);u.mem_write(a,d)
u.mem_map(0x90000000,0x400000);u.mem_map(0xa0000000,0x8000000)
stop=0x90000000;stack=0x903ff000;scratch=0x90001000;alloc=0xa0000000
regs=[UC_ARM_REG_R0,UC_ARM_REG_R1,UC_ARM_REG_R2,UC_ARM_REG_R3]
def r(i):return u.reg_read(regs[i])
def ret(n=0):u.reg_write(UC_ARM_REG_R0,n&0xffffffff);u.reg_write(UC_ARM_REG_PC,u.reg_read(UC_ARM_REG_LR))
def hook(n,fn):
 if n not in sy:return
 u.hook_add(UC_HOOK_CODE,lambda *args:fn(),begin=sy[n]&~1,end=sy[n]&~1)
def malloc(n):
 global alloc
 p=alloc;alloc+=(n+15)&~15;assert alloc<0xa8000000;return p
for n in ['malloc','linearAlloc']:hook(n,lambda:ret(malloc(r(0))))
hook('calloc',lambda:ret(malloc(r(0)*r(1))))
for n in ['free','linearFree','__aeabi_atexit','fflush','sceKernelLockLwMutex','sceKernelUnlockLwMutex']:hook(n,lambda:ret())
def cp():
 p,q,n=r(0),r(1),r(2);u.mem_write(p,bytes(u.mem_read(q,n)));ret(p)
def ms():
 p,v,n=r(0),r(1),r(2);u.mem_write(p,bytes([v&255])*n);ret(p)
for n in ['memcpy','memmove','sceClibMemcpy','sceClibMemmove']:hook(n,cp)
for n in ['memset','sceClibMemset']:hook(n,ms)
hook('__getreent',lambda:ret(struct.unpack('<I',u.mem_read(sy['_impure_ptr'],4))[0]));hook('__errno',lambda:ret(scratch+1024))
def cs(p):
 b=bytearray()
 while p:
  v=u.mem_read(p,1)[0]
  if not v:return b.decode(errors='replace')
  b.append(v);p+=1
 return ''
for n in ['fprintf','printf','puts','fputs','fputc']:hook(n,lambda:ret())
def missing():
 u.mem_write(scratch+1024,struct.pack('<I',2));ret(-1)
hook('stat',lambda:ret(0) if cs(r(0)) in files else missing());hook('getenv',lambda:ret(scratch+2048) if cs(r(0))=='TMC_AUTOLOAD' and inputs else ret(0));hook('access',lambda:ret(-1));hook('mkdir',lambda:ret(0))
rom=Path(os.environ['TMC_TEST_ROM']).read_bytes()
fds={};files={};paths={}
if os.environ.get('TMC_TEST_SAVE'):
 raw=Path(os.environ['TMC_TEST_SAVE']).read_bytes()
 assert len(raw)==8192
 files['tmc_eu.sav']=io.BytesIO(raw)
def op():
 path=cs(r(0));mode=cs(r(1))
 if path.endswith('.gba'):h=malloc(256);fds[h]=io.BytesIO(rom);ret(h)
 elif 'w' in mode:
  h=malloc(256);files[path]=io.BytesIO();fds[h]=files[path];paths[h]=path;ret(h)
 elif path in files:
  h=malloc(256);fds[h]=io.BytesIO(files[path].getvalue());paths[h]=path;ret(h)
 else:u.mem_write(scratch+1024,struct.pack('<I',2));ret(0)
def read():
 p,s,n,h=r(0),r(1),r(2),r(3);b=fds[h].read(s*n);u.mem_write(p,b);ret(len(b)//s if s else 0)
hook('fopen',op);hook('fread',read);hook('fclose',lambda:ret());hook('fseek',lambda:(fds[r(0)].seek(struct.unpack('<i',struct.pack('<I',r(1)))[0],r(2)),ret()));hook('ftell',lambda:ret(fds[r(0)].tell()))
def write():
 h=r(3)
 if h in fds:fds[h].write(bytes(u.mem_read(r(0),r(1)*r(2))))
 ret(r(2))
hook('fwrite',write)
hook('fileno',lambda:ret(r(0)));hook('fsync',lambda:ret());hook('ferror',lambda:ret());
def rename():
 files[cs(r(1))]=files.pop(cs(r(0)));ret()
hook('rename',rename)
def remove():
 files.pop(cs(r(0)),None);ret()
hook('unlink',remove);hook('remove',remove)
def getc():
 b=fds[r(0)].read(1);ret(b[0] if b else -1)
hook('fgetc',getc)
hook('Platform3DS_IsNativeAddress',lambda:ret(r(0)>0x10000000));hook('Platform3DS_IsActiveStackAddress',lambda:ret(0))
for n in ['Port_Audio_Update','Port_M4A_Backend_OnVBlank','Platform3DS_WaitForVBlank','Platform3DS_EndFrameBoundary','Platform3DS_MarkFrameDiscontinuity']:hook(n,lambda:ret())
states=[]
combat=[]
frames=0
# Observation only: retain the original compiled handler and all of its effects.
def observe_combat(name):
 p=r(0); e=bytes(u.mem_read(p,0x88))
 combat.append({'frame':frames,'handler':name,'entity':p,'kind':e[8],'id':e[9],
                'health':e[0x45],'hit_type':e[0x3f],'contact':e[0x41],
                'x':struct.unpack_from('<i',e,0x2c)[0]/65536,
                'y':struct.unpack_from('<i',e,0x30)[0]/65536})
for observer in ['BusinessScrubPrologue_OnCollision','BusinessScrub_OnCollision','Octorok_OnCollision','Octorok_OnDeath']:
 hook(observer,lambda name=observer:observe_combat(name))
limit=int(sys.argv[2]) if len(sys.argv)>2 else 1000
def boundary():
 global frames,limit
 frames+=1
 if frames%100==0:
  p=sy['gPlayerEntity'];x,y=struct.unpack('<ii',u.mem_read(p+0x2c,8));state={'frame':frames,'room':list(u.mem_read(sy['gRoomControls']+4,2)),'origin':list(struct.unpack('<HH',u.mem_read(sy['gRoomControls']+6,4))),'main':list(u.mem_read(sy['gMain'],12)),'input':list(u.mem_read(sy['gInput'],8)),'priority':list(u.mem_read(sy['gPriorityHandler'],8)),'task':u.mem_read(sy['gMain']+2,1)[0],'action':u.mem_read(p+12,1)[0],'x':x/65536,'y':y/65536};states.append(state);print(state,flush=True)
 if frames>=limit:
  if os.environ.get('TMC_TEST_INTERACTIVE'):
   for name,size in [('gPlayerEntity',0x88),('gPlayerState',0xb0),('gRoomControls',0x38),('gArea',0x894),('gEntities',72*0x88)]:
    (out/(name+'.bin')).write_bytes(bytes(u.mem_read(sy[name],size)))
   for name in ['gRoomVars','gMapBottom','gMapTop','gSave']:
    if name in sy and sizes[name]: (out/(name+'.bin')).write_bytes(bytes(u.mem_read(sy[name],sizes[name])))
   print('AWAIT_INPUT',frames,flush=True)
   command=json.loads(input())
   if command.get('stop'):u.emu_stop();return
   inputs.extend(tuple(row) for row in command.get('events',[]))
   limit=int(command['until']);assert limit>frames
  else:u.emu_stop();return
 ret(1)
hook('Platform3DS_BeginFrameBoundary',boundary);hook('Platform3DS_ReadKeyInput',lambda:ret(0x3ff ^ sum(mask for lo,hi,mask in inputs if lo<=frames<hi)));hook('Platform3DS_ReadKeyDownInput',lambda:ret(0x3ff))
def present():
 raw=bytes(u.mem_read(r(0),240*160*4))
 if frames%100==0:
  rgb=b''.join(raw[i:i+3] for i in range(0,len(raw),4));(out/f'frame-{frames:05}.ppm').write_bytes(b'P6\n240 160\n255\n'+rgb)
 ret()
hook('Vita_Present',present)
# Platform clock services only; game and backup logic execute unchanged.
def clock64():
 u.reg_write(UC_ARM_REG_R1,0);ret(frames*16743)
hook('Platform3DS_SystemTick',clock64)
def walltime():
 if r(0):u.mem_write(r(0),struct.pack('<q',1792756800))
 u.reg_write(UC_ARM_REG_R1,0);ret(1792756800)
hook('time',walltime)
# Render sampled frames only; all game updates, DMA, input run every tick.
hook('Port_PPU_PresentFrame',lambda:ret() if frames%100 else None)
def die():raise RuntimeError('abort '+hex(u.reg_read(UC_ARM_REG_LR)))
hook('abort',die);hook('exit',die)
def call(n,*args):
 u.reg_write(UC_ARM_REG_SP,stack);u.reg_write(UC_ARM_REG_LR,stop)
 for i,v in enumerate(args):u.reg_write(regs[i],v)
 print('call',n,flush=True)
 try:u.emu_start(sy[n],stop)
 except Exception:
  print('PC',hex(u.reg_read(UC_ARM_REG_PC)), 'LR',hex(u.reg_read(UC_ARM_REG_LR)),flush=True);raise
if os.environ.get('TMC_SENTINEL_TEST'):
 live=sy['gPlayerEntity'];dead=sy['gEntities'];prev=sy['gEntityLists'];assert prev>=0x80000000
 u.mem_write(live,struct.pack('<II',prev,prev));u.mem_write(dead,struct.pack('<I',0xffffffff))
 call('ClearAllDeletedEntities')
 result={'live_preserved':bytes(u.mem_read(live,4))==struct.pack('<I',prev),'deleted_cleared':bytes(u.mem_read(dead,4))==bytes(4)}
 print(result);(out/'sentinel.json').write_text(json.dumps(result,indent=2));sys.exit(0 if all(result.values()) else 1)
u.mem_write(scratch+2048,b'0\0');u.mem_write(scratch,b'baserom_eu.gba\0');call('Port_LoadRom',scratch);call('Port_PPU_Init',0);call('AgbMain')
(out/'oam.bin').write_bytes(bytes(u.mem_read(sy['gOamMem'],1024)))
for name,data in files.items():
 if name.endswith('.sav'):
  assert '/' not in name and ':' not in name
  (out/('test-'+name)).write_bytes(data.getvalue())
print('done',frames)
(out/'result.json').write_text(json.dumps({'frames':frames,'states':states,'combat':combat,'elf':elfpath,'input':inputs,'rom_sha1':__import__('hashlib').sha1(rom).hexdigest()},indent=2))
