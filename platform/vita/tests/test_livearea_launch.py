"""Host integration: -settings must open once before frame 1 and preserve play.
Usage: python test_livearea_launch.py HOST_EXECUTABLE ROM OUTPUT_DIRECTORY
"""
import os,sys,subprocess,hashlib,json
from pathlib import Path
exe,rom,out=map(lambda s:Path(s).resolve(),sys.argv[1:]);out.mkdir(parents=True,exist_ok=True)
reports={}
for name,launch in [('normal',''),('settings','type=LAUNCH_APP_BY_URI&uri=psla:-settings')]:
 d=out/name;d.mkdir(exist_ok=True);(d/'menu.txt').write_text('2 3 40\n')
 env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',TMC_UNPACED='1',TMC_FRAMES='2010',TMC_DUMP_AT='2000',TMC_TRACE='1',TMC_AUDIO_CAPTURE='audio.raw',TMC_LAUNCH=launch,TMC_MENU_SCRIPT=str(d/'menu.txt'))
 for key in ['TMC_AUTOLOAD','TMC_TITLE_BACKGROUND','TMC_MENU_AT','TMC_INPUT_SCRIPT']:env.pop(key,None)
 with (d/'run.log').open('w') as f:subprocess.run([str(exe),str(rom)],cwd=d,env=env,stdout=f,stderr=f,check=True,timeout=30)
 log=(d/'run.log').read_text()
 if launch:
  assert log.count('MENU open game_frame=0')==1
  assert log.index('MENU close game_frame=0')<log.index('Entering AgbMain')
 else:assert 'MENU open' not in log
 reports[name]={n:hashlib.sha256((d/n).read_bytes()).hexdigest() for n in ['frame.ppm','audio.raw']}
 reports[name]['states']=[line for line in log.splitlines() if line.startswith('STATE')]
assert reports['normal']==reports['settings']
(out/'result.json').write_text(json.dumps(reports,indent=2)+'\n')
print('PASS: Settings opens once before game, closes cleanly, same image/audio/game states as normal launch')
