"""Verify the bundled, unchanged Port Menu source snapshot."""
import hashlib, json
from pathlib import Path

def verify(root):
    record = json.loads((root / 'tools/port-menu-source.json').read_text())
    module = root / record['path']
    for name, expected in record['files'].items():
        path = module / name
        if path.is_symlink() or not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            raise ValueError('Bundled Port Menu source mismatch: ' + name)
    return record['commit']

if __name__ == '__main__':
    print('PASS Port Menu ' + verify(Path(__file__).resolve().parents[1]))
