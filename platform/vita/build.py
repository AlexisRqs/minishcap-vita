#!/usr/bin/env python3
"""Build/package the Vita prototype. SDK may be supplied by --sdk or VITASDK."""
import argparse,os,subprocess,zipfile,shutil,struct
import xml.etree.ElementTree as ET
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--assets-dir',type=Path,required=True,help='Locally generated LiveArea files');p.add_argument('--sdk',default=os.getenv('VITASDK'));p.add_argument('--output',type=Path,default=Path('build-vita'));p.add_argument('--generated-source-dir',type=Path,required=True,help='Local generated game-data sources, excluded from Git');a=p.parse_args()
if not a.sdk:p.error('Provide --sdk /path/to/vitasdk or set VITASDK')
sdk=Path(a.sdk).resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
platform=Path(__file__).resolve().parent;root=platform.parents[1]
env=dict(os.environ,VITASDK=str(sdk));env['PATH']=str(sdk/'bin')+os.pathsep+env['PATH']
def run(*args):subprocess.run([str(x) for x in args],env=env,check=True)
run('cmake','-S',platform,'-B',out,'-DCMAKE_TOOLCHAIN_FILE='+str(sdk/'share/vita.toolchain.cmake'),'-DTMC_GENERATED_DIR='+str(a.generated_source_dir.resolve()))
run('cmake','--build',out,'-j','8')
run(sdk/'bin/vita-elf-create',out/'tmc-vita',out/'tmc-vita.velf')
run(sdk/'bin/vita-make-fself','-s',out/'tmc-vita.velf',out/'eboot.bin')
run(sdk/'bin/vita-mksfoex','-s','TITLE_ID=MINISH001','-s','APP_VER=00.06','Minish Cap Vita',out/'param.sfo')
# Reject broken shell assets before packaging; Vita expects indexed 8-bit PNGs.
asset_sizes={'icon0.png':(128,128),'bg.png':(840,500),'startup.png':(280,158),'settings.png':(280,124)}
for name,size in asset_sizes.items():
 raw=(a.assets_dir.resolve()/name).read_bytes()
 assert raw[:8]==b'\x89PNG\r\n\x1a\n' and struct.unpack('>II',raw[16:24])==size,name
 assert raw[24:26]==bytes([8,3]),(name,'expected 8-bit indexed PNG')
xml=ET.parse(a.assets_dir.resolve()/'template.xml').getroot()
assert xml.attrib.get('content-rev')=='5'
assert xml.find("./frame[@id='frame3']/liveitem/target").text=='psla:-settings'
for node in xml.iter():
 if node.tag in ('image','startup-image'):assert node.text in asset_sizes,node.text
with zipfile.ZipFile(out/'minishcap-vita-0.06.vpk','w',zipfile.ZIP_DEFLATED) as z:
 z.write(out/'eboot.bin','eboot.bin');z.write(out/'param.sfo','sce_sys/param.sfo')
 for name in [*asset_sizes, 'template.xml']:
  f=a.assets_dir.resolve()/name
  z.write(f,'sce_sys/'+('' if name=='icon0.png' else 'livearea/contents/')+name)
 for f in [root/'LICENSE',root/'docs/legal/COPYRIGHT.md',root/'docs/legal/THIRD-PARTY-LICENSES.md',root/'docs/legal/UPSTREAM-THIRD-PARTY-LICENSES.md',root/'src/third_party/agbplay_core/COPYING.LESSER',platform/'PROVENANCE.txt',platform/'VITA2D-LICENSE.txt',platform/'port-menu/FONT-LICENSE.txt']:
  z.write(f,'licenses/'+f.name)
 z.write(platform/'port-menu/LICENSE','licenses/PORT-MENU-LICENSE.txt')
print(out/'minishcap-vita-0.06.vpk')
