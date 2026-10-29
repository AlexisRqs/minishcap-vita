#!/usr/bin/env python3
"""Compare legacy copies; Git checkouts must be updated using pinned commits."""
import argparse
import hashlib
import json
import shutil
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('source', type=Path)
parser.add_argument('destination', type=Path)
parser.add_argument('--write', action='store_true')
args = parser.parse_args()
if args.source.resolve() == args.destination.resolve():
    parser.error('Source and destination must differ')
if args.write and (args.destination / '.git').exists():
    parser.error('Refusing to overwrite a Git checkout; update its pinned commit instead')

def files(root):
    return {str(p.relative_to(root)): p for p in root.rglob('*')
            if p.is_file() and not any(part in {'.git', '__pycache__', '.DS_Store', 'test-output'}
                or part.startswith('build') for part in p.relative_to(root).parts)}

source = files(args.source)
if not source:
    parser.error('Source contains no files')
if args.write:
    for relative, path in source.items():
        target = args.destination / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, target)
destination = files(args.destination)
if set(source) != set(destination):
    parser.error('Different file lists (including obsolete destination files)')
manifest = {}
for relative, path in sorted(source.items()):
    if path.read_bytes() != destination[relative].read_bytes():
        parser.error('Different contents: ' + relative)
    manifest[relative] = hashlib.sha256(path.read_bytes()).hexdigest()
print(json.dumps({'identical': True, 'files': manifest}, indent=2))
