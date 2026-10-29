#!/usr/bin/env python3
"""Synthetic boundary checks; no real ROM or save file is accessed."""
import importlib.util
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('rebuild',ROOT/'tools/rebuild_from_rom.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
external=getattr(module,'external',getattr(module,'outside_git',None))

class BoundaryTests(unittest.TestCase):
    def test_reject_checkout_and_symlink(self):
        with self.assertRaises(ValueError):external(ROOT/'private-output')
        with tempfile.TemporaryDirectory() as temp:
            link=Path(temp)/'checkout';link.symlink_to(ROOT,target_is_directory=True)
            with self.assertRaises(ValueError):external(link/'private-output')
    def test_reject_other_git_repository(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);(root/'.git').mkdir()
            with self.assertRaises(ValueError):external(root/'output')
    def test_accept_external_directory(self):
        with tempfile.TemporaryDirectory() as temp:
            path=Path(temp)/'output';self.assertEqual(external(path),path.resolve())
    def test_wrong_rom_writes_nothing(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);rom=root/'invalid.gba';rom.write_bytes(b'not a ROM')
            output=root/'output'
            command=[sys.executable,str(ROOT/'tools/rebuild_from_rom.py'),'--rom-us',str(rom),'--sdk',str(root/'sdk'),'--workspace',str(output)]
            if (ROOT/'engine/full').is_dir():command+=['--rom-eu',str(rom)]
            result=subprocess.run(command,capture_output=True,text=True)
            self.assertNotEqual(result.returncode,0);self.assertFalse(output.exists())
            self.assertIn('Unsupported',result.stderr)

if __name__=='__main__':unittest.main()
