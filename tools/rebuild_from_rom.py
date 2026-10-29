#!/usr/bin/env python3
"""Rebuild in an external, disposable workspace. Never uploads any input/output."""
import os
import importlib.util
import argparse
import hashlib
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PIN = 'b181eb6a427678b2d374c98432abf114b0330693'

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def outside_git(path):
    path = path.resolve()
    if any((parent / '.git').exists() for parent in (path, *path.parents)):
        raise ValueError('Private inputs/workspace must be outside every Git checkout: ' + str(path))
    if path == ROOT or ROOT in path.parents:
        raise ValueError('Workspace must be outside the source repository')
    return path

def checked_rom(path, expected):
    path = outside_git(path)
    data = path.read_bytes()
    if hashlib.sha1(data).hexdigest() != expected:
        raise ValueError('Unsupported ROM revision: ' + str(path))
    return data

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom-us', type=Path, required=True, help='Original USA Minish Cap ROM for native data tables')
    parser.add_argument('--rom-eu', type=Path, help='Required European ROM for local title rendering and runtime')
    parser.add_argument('--sdk', type=Path, required=True)
    parser.add_argument('--workspace', type=Path, required=True, help='New directory outside every Git checkout')
    args = parser.parse_args()
    layout = json.loads((ROOT / 'tools/rom-build/rom-layout.json').read_text())
    rom = checked_rom(args.rom_us, layout['rom_sha1'])
    if not args.rom_eu: parser.error('--rom-eu is required for the runtime and local title artwork')
    eu = checked_rom(args.rom_eu, 'cff199b36ff173fb6faf152653d1bccf87c26fb7')
    workspace = outside_git(args.workspace)
    if workspace.exists():
        parser.error('Workspace already exists; choose a new directory (nothing was overwritten)')
    sdk = args.sdk.resolve()
    for name in ['bin/arm-vita-eabi-gcc', 'bin/vita-elf-create', 'arm-vita-eabi/lib/libvita2d.a']:
        if not (sdk / name).is_file(): parser.error('Missing VitaSDK prerequisite: ' + name)
    from check_port_menu import verify
    if verify(ROOT) != PIN: parser.error('Unexpected bundled Port Menu revision')
    workspace.mkdir(parents=True)
    manifest = {'ok': False, 'rom_us_sha1': layout['rom_sha1'], 'module_commit': PIN, 'hardware_tested': False}
    report = workspace / 'build-report.json'
    report.write_text(json.dumps(manifest, indent=2))
    generated = workspace / 'generated'; generated.mkdir()
    def expand(match):
        record = layout['arrays'][match[1]]
        data = rom[record['offset']:record['offset'] + record['length']]
        if hashlib.sha256(data).hexdigest() != record['sha256']: raise ValueError('ROM extraction mismatch')
        return '\n'.join('    ' + ', '.join(f'0x{x:02X}' for x in data[i:i+16]) + ',' for i in range(0, len(data), 16))
    for template in (ROOT / 'tools/rom-build').glob('*.c.in'):
        text = re.sub(r'@@ROM:([^@]+)@@', expand, template.read_text())
        if '@@ROM:' in text: raise ValueError('Unresolved resource reference')
        (generated / template.name.removesuffix('.in')).write_text(text)
    sound = (ROOT / 'tools/rom-build/sound-layout.json').read_text().rstrip('\n').encode()
    # Layout metadata is regenerated as C; it contains no sound samples.
    body = '\n'.join('    ' + ', '.join(f'0x{x:02x}' for x in sound[i:i+16]) + ',' for i in range(0, len(sound), 16))
    (generated / 'generated_sounds_embed.cpp').write_text('#include "port_sounds_embed.hpp"\nnamespace PortSoundsEmbed {\nconst unsigned char kData[] = {\n' + body + '\n};\nconst std::size_t kSize = sizeof(kData);\n}\n')
    # Render original title pixels locally; do not distribute captured images.
    host=workspace/'host-build'
    with (workspace/'host-build.log').open('w') as log:
        subprocess.run(['cmake','-S',str(ROOT/'platform/vita'),'-B',str(host),
                        '-DTMC_GENERATED_DIR='+str(generated)],stdout=log,stderr=subprocess.STDOUT,check=True)
        subprocess.run(['cmake','--build',str(host),'--parallel','8'],stdout=log,stderr=subprocess.STDOUT,check=True)
    art=workspace/'art'; art.mkdir()
    local_rom=art/'baserom_eu.gba'; local_rom.write_bytes(eu)
    for name,background in [('title',False),('forest',True)]:
        capture=art/name; capture.mkdir()
        env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_AUDIODRIVER='dummy',
                 TMC_NO_AUDIO='1',TMC_UNPACED='1',TMC_FRAMES='2001',TMC_DUMP_AT='2000',TMC_AUTOLOAD='0')
        for key in list(env):
            if key.startswith('TMC_') and key not in {'TMC_NO_AUDIO','TMC_UNPACED','TMC_FRAMES','TMC_DUMP_AT','TMC_AUTOLOAD'}:del env[key]
        if background:env['TMC_TITLE_BACKGROUND']='1'
        with (capture/'render.log').open('w') as log:
            subprocess.run([str(host/'tmc-vita'),str(local_rom)],cwd=capture,env=env,
                           stdout=log,stderr=subprocess.STDOUT,check=True,timeout=120)
    assets=workspace/'livearea'
    spec=importlib.util.spec_from_file_location('livearea',ROOT/'tools/rom-build/livearea.py')
    live=importlib.util.module_from_spec(spec);spec.loader.exec_module(live)
    live.make_assets(art/'title/frame.ppm',art/'forest/frame.ppm',assets,module)
    shutil.copy2(ROOT/'platform/vita/assets/template.xml',assets/'template.xml')
    with (workspace / 'build.log').open('w') as log:
        subprocess.run([sys.executable, str(ROOT / 'platform/vita/build.py'), '--sdk', str(sdk), '--assets-dir', str(assets), '--generated-source-dir', str(generated), '--output', str(workspace / 'build')], stdout=log, stderr=subprocess.STDOUT, check=True)
    if eu is not None:
        target = workspace / 'install/ux0/data/minishcap/baserom_eu.gba'
        target.parent.mkdir(parents=True); target.write_bytes(eu)
    vpk = workspace / 'build/minishcap-vita-0.06.vpk'
    manifest.update(ok=True, vpk=str(vpk), vpk_sha256=digest(vpk), generated_inputs={p.name:digest(p) for p in generated.iterdir()})
    report.write_text(json.dumps(manifest, indent=2) + '\n')
    print('PASS: fresh ROM-based Vita build:', vpk)

if __name__ == '__main__':
    main()
