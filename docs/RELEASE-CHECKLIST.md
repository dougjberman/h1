# Release checklist

What has to be true before a `vX.Y.Z` tag is pushed and h1 is submitted to
[The Firmware Hub](https://thefirmwarehub.com/submit/). The Hub lists a firmware only when it has
been installed on a real FM-1 from V15 and from another firmware, and left again.

Tick a box only after doing it on hardware.

Done so far (2026-11-18, one unit bought November 2026): section 1, with the development build of
commit `037ed8a` installed from official V15 by `fm1_install.py`. The FM-1 answered `FM-1_015`
before and `FM-1_48100` after, restarted by itself, the port is named `h1 FM-1`, and the
screen reader's channel reported COZY and ORBIT. No crash was recorded.

Reported by a tester (2026-11-19, [#25](https://github.com/jakehandy/h1/issues/25)), not ticked
below because the route differs:

- v0.2.1 installed over stock **V16** (not V15) on a unit with the older Puya P25Q80 (`0x856014`)
  and m-bootloader 2.4 in place of the stock bootloader, with the tester's own USB MIDI installer
  (the same update protocol as the web installers). It wrote to 100%, restarted by itself, answers
  `FM-1_481` and shows on USB as `h1 FM-1`. The package matched `SHA256SUMS`.
- Leaving h1 worked by restoring a full flash backup with M-Flasher in download mode: it verified,
  restarted, and the saved data came back byte for byte. That route skips h1's update loader, so it
  is not section 3's **Back to official V15**.
- Not reported yet: settings kept across a power cycle on that chip (section 5's real test).

## 0. Before touching an FM-1

- [ ] The design is signed off in the simulator.
- [ ] CI is green on the commit to be released: the host tests, and the firmware job's build with
      JieLi's toolchain. Read the firmware job's log for the sizes: the app has to fit its flash
      slot, and the pool and `.data`/`.bss` have to fit RAM (see "RAM on the device" in
      [ARCHITECTURE.md](ARCHITECTURE.md)). The build refuses when they don't.
- [ ] Get the package to test: Actions, **Build and attest firmware**, **Run workflow** on that
      commit, then download the run's `packages` artifact. That is `h1.fwsc`, a development build
      with identity `FM-1_48100`.
- [ ] Type `FM-1_481` into the identity checker on the
      [Hub's submit page](https://thefirmwarehub.com/submit/) and confirm no other firmware uses
      it. If it is taken, change `id_prefix` in `app/fm1.toml` and build again.
- [ ] Back up the test FM-1 with whatever it runs now. On Windows,
      [MvaveFM1Unbricker](https://github.com/Quixotic7/MvaveFM1Unbricker) can save the whole flash.
- [ ] Know the way out before going in: read the Hub's
      [rescue page](https://thefirmwarehub.com/tools/rescue/) once.

## 1. Install from stock V15

- [x] Start from official V15 (the Hub's **Back to official V15** puts it there).
- [x] Install `h1.fwsc`: `python3 base/tools/fm1_install.py h1.fwsc`. It finishes without an error
      and the FM-1 restarts into h1 (COZY comes up).
- [x] `python3 base/tools/fm1_install.py --info` answers `FM-1_48100`.
- [x] The computer shows the USB device as `h1 FM-1`.

## 2. It works

- [ ] Every one of the 27 keys sounds, and the right key lights.
- [ ] All seven encoders and fourteen buttons do what the README's hotkeys table says.
- [ ] PRESETS goes through all sixteen instruments. Each one draws, animates smoothly and
      sounds. Note any that stutter (KOI, URCHIN and BONK send the most pixels).
- [ ] No crackle or dropouts with eight notes held on the heavy instruments (TWANG, BARD, KOI),
      with ECHO and SPACE turned up.
- [ ] MASTER goes from silent to full without clicks. Headphones and the speaker both play. The
      ring in the middle of the screen, marked GLOBAL, follows the knob and is gone a second after
      it stops; it does not come up by itself with the knob at rest.
- [ ] Hold EDIT: the ring comes up marked INSTRUMENT and stays while EDIT is down. Turn SELECT
      with it held: that instrument gets quieter without clicks, down to silence, and MASTER
      still works on top. Let go: the knob strip is not pinned. A quick tap of EDIT still pins
      it. With LAYERS on, a loop recorded on one instrument follows that instrument's volume
      while another is in hand. The volume is still there after a restart, and hold SAVE for two
      seconds puts it back to full.
- [ ] Hold EDIT and turn MASTER: the ring is marked INSTRUMENT, the instrument follows the knob
      from silence to full, and everything else stays as loud as it was. Let go and turn MASTER
      away from where it was: nothing gets louder or quieter, and the ring shows the held volume
      with a dot for the knob. Turn it back through that volume: it follows the knob again,
      without a jump.
- [ ] Loop: REC, play a take, REC again, it loops, overdub, hold REC clears. The white keys show
      the pattern and the place runs across.
- [ ] Step: hold SEQ, REC, enter notes and a rest, PLAY. The white keys show the steps.
- [ ] LAYERS: SEQ and REC together says LAYERS ON. Record a loop, turn PRESETS, play over it and
      record again: both instruments play. Turn ALGORITHM: the recorded part does not change.
      Hold REC clears only the instrument in hand; held three seconds, everything. The same in
      STEP. SEQ and REC again: LAYERS OFF, and it all plays on the instrument in hand.
- [ ] The SEQ page's top line scrolls through HOLD SEQ, SEQ + REC and HOLD REC, and reads still
      for a moment when the page opens.
- [ ] Metronome: hold GLO cycles CLICK REC, CLICK ON, CLICK OFF, and holding never opens the page.
- [ ] MIDI in over USB plays notes, and CC 20 to 23 move the four knobs.
- [ ] Change the instrument, an algorithm and a knob, wait ten seconds, switch off and on: they
      are kept.
- [ ] Turn a knob, turn ALGORITHM and back: each algorithm has its knobs where they were left.
      Hold SAVE says the algorithm's name and RESET, and its four knobs are as new. Held two
      seconds it says the instrument's name and RESET. Held four seconds, ALL RESET.
- [ ] Writing steps, eight keys held together land on one step, and it plays all eight.
- [ ] Leave it running for thirty minutes. No freeze, no SAFE MODE.

## 3. Leave h1 again

- [ ] From h1, install official V15 (the Hub's **Back to official V15**). The FM-1 runs stock
      again and still works.
- [ ] From h1, install another custom firmware from the Hub. It starts.

## 4. Install from another firmware

- [ ] From that other custom firmware, install `h1.fwsc` again. It starts, and `--info` answers
      `FM-1_48100`.
- [ ] Install `h1.fwsc` over h1 itself (an update). Settings are kept.

## 5. A newer-flash unit

The FM-1s sold since autumn 2026 carry a UC25HQ80 flash (JEDEC `0xB36014`) in place of the Puya
P25Q80 (`0x856014`). A firmware that only knows the Puya can't save or update on them.

- [ ] Do sections 1, 3 and 4 on a newer unit. h1 doesn't report the flash ID over USB, so the test
      is the next line: on a chip a firmware doesn't know, settings are gone after a power cycle.
- [ ] On that unit, settings survive a power cycle (this is the part that fails on a firmware that
      doesn't know the chip).
- [ ] If only one kind of unit is at hand, find a tester with the other kind before submitting,
      and say in the release notes which kind was tested by whom.

## 6. Recovery

- [ ] Hold REC while switching on: the FM-1 enters the chip's update mode, and switching off and
      on again starts h1 normally.

## 7. Release

- [ ] Remove the "Status: no release yet" note from the README's Install section.
- [ ] Add the repository topics: `m-vave`, `fm-1`, `firmware`, `synthesizer`.
- [ ] Before the tag, on the commit to be tagged: `tools/build_rfm1.sh X.Y.Z` (the browser simulator's
      `dist/h1.rfm1`), and `python3 tools/hub_card.py` when an instrument came or went. Commit both.
      The release workflow refuses a tag whose version differs from `CHANGELOG.md`'s top section or
      from `dist/h1.rfm1`'s (0.2.2 went out with the simulator still on 0.2.1, #46).
- [ ] Merge to `main`. `fmhub.json` has to be in the root of the default branch.
- [ ] Tag: `git tag vX.Y.Z && git push origin vX.Y.Z` (the `v` and three numbers: `v0.3.0`). The workflow builds `h1-X.Y.Z.fwsc`
      (identity `FM-1_481XXYYZZ`: v0.2.0 is `FM-1_481000200`), tests, attests and publishes the
      release.
- [ ] Install the released `h1-X.Y.Z.fwsc` once on the FM-1, as a last check of the exact file.
- [ ] Submit at [thefirmwarehub.com/submit](https://thefirmwarehub.com/submit/).
