"""Check sampled ARM game states after stairs-roundtrip-input.txt (18000 frames)."""
import json,sys
from pathlib import Path
result=json.loads(Path(sys.argv[1]).read_text())
sequence=[]
for state in result['states']:
 room=state['room']
 if room[0]==34 and room[1] in (16,21) and (not sequence or sequence[-1]!=room):
  sequence.append(room)
assert sequence==[[34,21],[34,16],[34,21]],sequence
assert result['states'][-1]['action']==1
print('PASS: bedroom -> entrance -> bedroom; player NORMAL')
