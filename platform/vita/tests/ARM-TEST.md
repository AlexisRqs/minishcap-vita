Requires Python packages unicorn and pyelftools. Set TMC_TEST_ROM to your clean European ROM.
Run: python test_vita_arm.py <Vita ELF> 13500 <output directory> new-game-input.txt
Without an input fixture it waits at the title. With a fixture, TMC_AUTOLOAD=0 selects slot zero.
Set TMC_SENTINEL_TEST=1 for the focused deletion-marker regression (fails on 0.01).
Only platform/libc services are doubled. Game, DMA, scripts and entity logic run as compiled ARM code. Rendering is sampled every 100 frame boundaries; audio output is not tested.
No real saves are read or written: the test has an isolated in-memory filesystem.

For persistence validation, each run exports the isolated EEPROM as test-tmc_eu.sav
inside its output directory. Set TMC_TEST_SAVE to that file on a fresh invocation
to test the engine's normal startup read path. This never imports the console save.

Stair regression: use stairs-roundtrip-input.txt and 18000 boundaries. Then run
python check_stairs_roundtrip.py <output directory>/result.json
It requires the observed room sequence 34:21 -> 34:16 -> 34:21, with normal player action at the end.
The exit is at bedroom-local (88,24), world (1384,456), in the upper part of the room.
The earlier extended-input.txt targeted furniture/walls at the bottom and is not a transition regression.

Adventure regression (ARM harness): use adventure-input.txt and 170500 boundaries,
then python check_adventure.py <output directory>/result.json. The route includes
exploratory detours and idle periods. It goes from a new game through the festival,
shield reflection, Vaati ceremony, sword acquisition, and two Octorok kills.
Combat hooks only observe entry to the original handlers; they never return early
or modify registers, health, position, flags or inventory. Room sequence and
post-combat movement are asserted too. This fixture exceeds the Mac runner's
512-input limit and is intended for test_vita_arm.py.
TMC_TEST_INTERACTIVE=1 enables JSON input at the requested frame limit:
{"until":18000,"events":[[17000,17020,64]]} extends execution; masks in JSON are decimal.
{"stop":true} finishes and exports the complete input sequence and observations.
Keep interactive commands below the terminal's line limit (about 1 KB on macOS).

Packaged evidence: adventure-result.json is the complete 170500-boundary replay.
scrub-result.json is a focused 70000-boundary replay using the prologue-specific
BusinessScrubPrologue_OnCollision observer (the normal BusinessScrub handler is
not used by this enemy). Check the supplied pair with:
python check_adventure.py adventure-result.json scrub-result.json
The checker also requires the focused replay's sampled states to match the full
replay. A fresh full run using the current harness supplies all observations in
one result.json and needs no second argument.
