"""End-to-end checks for adventure-input.txt on the unchanged Vita 0.02 ELF."""
import json, sys
from pathlib import Path
r = json.loads(Path(sys.argv[1]).read_text())
states = r['states']
sequence = []
for s in states:
    if not sequence or s['room'] != sequence[-1]:
        sequence.append(s['room'])
required = [[34,21],[34,16],[34,17],[34,16],[3,1],[21,0],[3,6],
            [7,0],[128,4],[128,2],[128,1],[128,0],[7,0],[3,6]]
pos = 0
for room in sequence:
    if pos < len(required) and room == required[pos]:
        pos += 1
assert pos == len(required), (pos, sequence)
scrub = json.loads(Path(sys.argv[2]).read_text()) if len(sys.argv)>2 else r
if scrub is not r:
    assert scrub['states'] == states[:len(scrub['states'])]
reflections = [c for c in scrub['combat'] if c['handler'] == 'BusinessScrubPrologue_OnCollision'
               and c['hit_type'] == 1 and c['contact'] & 0x7f == 0x42]
assert reflections, 'No reflected seed hit the Business Scrub'
deaths = {c['entity'] for c in r['combat'] if c['handler'] == 'Octorok_OnDeath'
          and c['health'] == 0 and c['frame'] > 160000}
assert len(deaths) >= 2, deaths
for entity in deaths:
    assert any(c['entity'] == entity and c['handler'] == 'Octorok_OnCollision'
               and c['health'] == 0 and c['contact'] & 0x7f == 4 for c in r['combat']), entity
last = states[-1]
assert last['room'] == [3,6] and last['action'] == 1
before = next(s for s in states if s['frame'] == 169600)
assert abs(last['x']-before['x']) > 20 and abs(last['y']-before['y']) > 40
print('PASS: village, shield reflection, castle, two Octorok deaths, post-combat movement')
