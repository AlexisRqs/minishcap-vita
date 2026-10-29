"""Run each storage test in a fresh temporary directory, including repeated CTest runs."""
import subprocess
import sys
import tempfile

with tempfile.TemporaryDirectory(prefix="port-menu-test-") as directory:
    args = [] if sys.argv[2] == "model" else [directory]
    subprocess.run([sys.argv[1], *args], check=True)
