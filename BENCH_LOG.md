# Height caller — bench log

## Nov 19, 2026 — sensor first light (AJ-SR04M at 3.3 V, T-Display, sensor_test.py)
- Readings track movement smoothly (54 cm out to 282 cm while moving).
- Held steady at ~88 cm: repeated 87-88 cm — rock stable, centimetre-class repeatability.
- One repeated constant: 604 cm (19.8 ft), nine times in a row at the end of the run.
  Ambiguous: either a genuine 6 m surface or the module's no-echo signature value.
  Discriminating test queued: point probe at open sky (nothing within 8 m) and see
  whether 604 persists; then a deliberate max-range walk-away test against a wall.

## Nov 19, 2026 — range limits at 3.3 V (Mark's tests)
- Minimum measured: 0.7 ft (~21 cm) — matches the 20 cm blind zone. Closer than that
  defaults to 604 cm.
- Maximum measured: 8.3 ft (~253 cm). Farther than that defaults to 604 cm.
- CONFIRMED: 604 cm (19.8 ft) is the module's no-echo default value, at both ends of
  the range. Caller logic treats >= ~600 cm as "no ground return".
- Next test: power the module at 5 V (T-Display 5V pin) with a 1k/2k divider on the
  Echo line, re-run the walk-away max-range test.

## Nov 19, 2026 — 5 V power (divider on Echo), Mark's re-test
- Maximum range: 15.5 ft (~472 cm) — up from 8.3 ft at 3.3 V. Slightly beyond the
  module's 4.5 m standard rating (hard-surface test). First callout at 15 ft is on.
- Minimum: ~6 in (~15 cm), slightly inside the rated 20 cm blind zone.
- Over grass/brush the max will be less than 15.5 ft — field test still to come.
- 5 V + divider wiring is the flight wiring.

## Nov 19, 2026 — display first light (T-Display ST7789)
- First run crashed: SPI baudrate 40 MHz exceeds the classic-ESP32 bus cap
  (~26.6 MHz) — invalid device handle, then Guru Meditation. Fixed at 20 MHz.
- Layout/offsets correct first try (135x240 portrait, XOFF 52, YOFF 40, INVON on).
- Colours were byte-swapped (red showed blue): MicroPython framebuf stores RGB565
  little-endian, panel wants big-endian. Fixed by pre-swapping the colour
  constants in st7789_min.py (GREEN 0xE007, RED 0x00F8, YELLOW 0xE0FF).
- Mark's reported "blank at times" was the NO RETURN screen, not a dropout
  (photo confirmed). v2 readout holds the last good value 1 s (shown yellow)
  before declaring NO RETURN. Mark: "working as described".
- Next: untethered field test (main.py + power bank) — max return height over
  pavement vs grass, the last unknown before callout design.

## Nov 19, 2026 — untethered field test (Mark, power bank, pole rig)
- Black asphalt: solid returns to 10.5 ft, worked properly throughout.
- Damp ground: solid to just over 9 ft, then NO RETURN.
- Real downward-looking envelope is ~10 ft hard surface / ~9 ft soft — below the
  15.5 ft horizontal wall figure (rough surfaces scatter the wide beam).
- Design consequence: callout ladder is built for a 10 ft ceiling. First call at
  10, per-foot below. The wish list's 15-20 ft start is not reachable with this
  sensor; the TFmini Plus LiDAR remains the drop-in upgrade if wanted.
- Install calibration: callouts must be WHEEL height = sensor reading minus the
  probe's parked height above ground on the gear.

## Nov 19, 2026 — callout pattern DECIDED (Mark)
- Voice at 10 / 5 / 3 / 2 / 1 ft; a short tone on the other feet (9/8/7/6/4).
- Rationale: flare sink ~3-5 ft/s, a spoken number takes ~0.5 s — voice on every
  foot would talk over itself.
- Next phase: Arduino build (A2DP Bluetooth audio is not possible in MicroPython).
  Voice clips to be generated as TTS, embedded as PCM in the sketch (16 MB flash
  is ample). Bench A2DP proof uses any BT speaker/earbuds; the AirCam panel/headset
  input question only matters at install time.

## Nov 19, 2026 — Arduino phase setup (Windows PC, Arduino IDE 2.3.11)
- CH341SER driver installed; T-Display on COM9. IDE board: LilyGo T-Display.
- Library Manager hiccup: duplicate Adafruit BusIO (stray arduino_777099 folder)
  had to be deleted before the ST7735/ST7789 library would install.
- Drawer LiPo #1: dead (0 V, protection never woke on charge) — to be recycled.
  Drawer LiPo #2: works, powers the board; charge LED not visibly confirmed,
  meter test pending. Flight pack (2000 mAh, JST-GH 1.25) ordered by Mark, plus
  generic 0.9-5V -> 5V boost boards (Amazon 10-pack) for the sensor rail.
- ESP32 core 4.0.0 CANNOT build on this PC: the link step launches
  tools\configure_link_libs.cmd and cmd.exe mangles the spaced profile path
  (C:\Users\Mark Harris) — "'C:\Users\Mark' is not recognized" at
  "Linking everything together", after a fully successful compile. The script's
  own quoting is fine; it never runs. FIX: downgrade the esp32 core to 3.3.x in
  Boards Manager (no such hook; also the proven base for ESP32-A2DP).
- RESOLVED same evening: core downgraded to 3.3.13; height_display sketch
  Verifies clean (318,093 bytes, 24% of program space).
- Upload initially failed: board was on battery (no COM9); on re-plugging USB
  Windows renumbered it to COM10. Upload then verified (hash OK).
- Blank-screen fault after first upload: backlight lit, no picture, program
  running (serial printing). Cause: Adafruit lib defaults to the ESP32's
  standard SPI data pin (GPIO 23) but the T-Display wires display MOSI to
  GPIO 19. Fix: SPI.begin(18, -1, 19, 5) before tft.init.
- Numbers upside down vs the MicroPython version: tft.setRotation(2) matches.
- MILESTONE (Nov 19 evening): Arduino readout at full parity with the
  MicroPython version — sensor + display + 1 s hold-last-good, confirmed by
  Mark ("Good to go"). Next session: Bluetooth A2DP proof.

## Nov 20, 2026 — A2DP session
- Mark clicked the IDE's "Update Available" and esp32 core 4.0.0 got installed
  again. Rolled back to 3.3.13 in Boards Manager; height_display Verifies
  clean again (318,105 bytes, 24%) — link step passes, rollback confirmed.
  Lesson: decline the IDE's core-update prompts for this project.
- Bluetooth proof target: Mark's earbuds (handiest at the bench).
  Beep-test sketch written: arduino/bt_beep_test/bt_beep_test.ino
  (ESP32-A2DP source, 1000 Hz tone 0.5 s on / 0.5 s off, connects to the
  earbuds by their exact Bluetooth name).
- Library Manager search would not surface ESP32-A2DP (author search showed
  4 other Schatzmann libraries, not this one). Installed v1.8.12 via
  Add .ZIP Library from the author's GitHub release clone. Compile fix:
  the Frame-based sound callback registers with set_data_callback_in_frames,
  NOT set_data_callback (raw bytes) — one-word fix in bt_beep_test v2.
- bt_beep_test v2 uploaded (hash verified, 1,088,885 bytes / 83%) but gave
  no visible result: Serial Monitor stayed stale/blank through resets and
  replugs (IDE monitor unreliability, same family as Nov 19), no beeps.
  Bisection: height_display re-uploaded and worked like before (board, port,
  serial all healthy); phone Bluetooth confirmed OFF, so the Buds were free.
- bt_beep_test v3 (on-screen BT status) uploaded Nov 20: screen shows
  SEARCHING in yellow — program alive, start() returned, loop polling.
  So v2 was very likely running too; the monitor was the blind spot and
  the earbuds simply never connected. Pairing step is next.
- PAIRING SOLVED + BEEPS HEARD (Nov 20, ~13:36 MST): with the board showing
  SEARCHING, Mark returned the Buds Pro to the case, closed it, and opened
  it again — that put the buds into pairing, the board connected, and he
  heard the 0.5 s on / 0.5 s off 1000 Hz beeps in the earbuds. The
  touchpad-hold method alone did not do it; case close/open is the recipe
  for these buds. BLUETOOTH A2DP AUDIO PIPELINE PROVEN END TO END:
  T-Display (core 3.3.13) + ESP32-A2DP 1.8.12 -> Galaxy Buds Pro (F488).
  Next phase: full caller sketch (sensor + callout ladder: voice at
  10/5/3/2/1, tones at 9/8/7/6/4, wheel-height offset, arm <= ~11 ft).
- FULL CALLER v1 BUILT (Nov 20): height_caller_v1.ino merges height_display
  (sensor + screen) with the A2DP audio engine. Callout ladder as decided:
  voice at 10/5/3/2/1, 1000 Hz 0.22 s tones at 9/8/7/6/4. Band logic: a foot
  fires once when the descent first reaches its whole-foot band; re-arms
  above 11.5 ft or after >1 s without ground return; 11.0-11.5 dead band.
  Callouts queue audio only while BT is connected (no stale blast on
  connect); screen always shows LAST CALL. WHEEL_OFFSET_FT = 0.0 for bench;
  set to the probe's parked height at install.
- Voice clips: five spoken numbers synthesized (system TTS voice), trimmed,
  normalized to 24k peak, 44.1 kHz mono PCM in voice_clips.h — 68,774
  samples total (137.5 KB flash), each word 0.28-0.35 s. Generator kept at
  voice/make_clips.py with the source MP3s alongside.
- Build needs Partition Scheme "Huge APP (3MB No OTA/1MB SPIFFS)": BT stack
  (1.09 MB) + display + clips outgrow the 1.25 MB default app partition.
  Awaiting Mark's compile/upload + ladder bench test.
- CALLER v1 BENCH VERDICT (Nov 20): "Very inconsistent." Serial log over a
  few minutes of testing showed only 10 callouts, out of order
  (1,3,2,1,9T,3,7T,5,3,1) with 10/8/6/4 never firing. Audio engine itself
  proven: both voice clips and tones played. Diagnosis: the logic fought
  the sensor's dropout character — every >1 s return gap re-armed the
  ladder, and the next valid reading then fired whatever band it landed
  in, hence out-of-order low shouts; fast hand movement skipped bands
  between 120 ms samples.
- CALLER v2 FIXES: (1) median-of-3 filter on valid readings for callout
  decisions (single bad echoes cannot fire or skip); (2) arm model with
  ceilingSeen/lowestFired — a foot fires only in descent context, and
  after a dropout the caller announces the re-acquired height once and
  continues down in order; (3) lost-return re-arm stretched 1 s -> 3 s;
  (4) loop delay 120 ms -> 50 ms so fast descents skip fewer feet.
  Firing the arming band is deliberate: "ten" on first ground return,
  "one" once at power-on near the ground acts as an audio self-check.
- CALLER v2 WORKING (Nov 20, photo from Mark): screen shows 7.5 ft live
  reading, BT: CONNECTED, LAST CALL: 3 - VOICE, CALLING OUT. The earlier
  "no callouts" report was timing — BT had just connected and no descent
  had run yet (callouts before a BT connection show on screen/serial only,
  by design). LAST CALL 3 with the height back at 7.5 also confirms the
  no-repeat rule: within one approach each foot fires once; the ladder
  re-arms only above 11.5 ft or after 3 s without return. Remaining
  verification: one slow controlled descent 11 -> 2 ft for full ladder
  order in the earbuds.
- Mark's v2 feedback (Nov 20): (1) callouts must count UP as well as down —
  a gust can float the plane back up a foot or two in the flare, and those
  feet must re-announce as it settles again; (2) volume too low in the buds.
- Board facts verified by web sources (Nov 20): T-Display front buttons are
  GPIO0 and GPIO35 (active-low, board pull-ups); RESET is a separate small
  switch — neither front button resets, which is why pressing one during
  the beep-test debugging did nothing. GPIO35 has no internal pull-up;
  sources disagree on which physical side is GPIO0 vs GPIO35.
- CALLER v3: announcedBand model with 0.2 ft hysteresis — each new whole
  foot announces once in either direction, boundary hover stays quiet.
  Volume: BT volume starts at 127 (was 80); clips re-rendered at 29k peak
  (was 24k); tone amplitude 9000 -> 16000; front buttons step volume
  40/60/80/100/127 (GPIO0 down with INPUT_PULLUP, GPIO35 up with its idle
  level learned at boot since its polarity is undocumented); screen shows
  VOL. Awaiting bench verdict.
- CALLER v3 BENCH VERDICT (Nov 20): "Working better. Volume better too."
  Bidirectional callouts and the louder volume both accepted. The caller
  is functionally bench-proven: sensor, ladder both directions, voice +
  tones, volume buttons, screen. Remaining before flight: set
  WHEEL_OFFSET_FT at install (probe parked height), 5 V boost for the
  sensor on battery power, battery % on screen, power-off strategy
  (soft sleep vs switch in the battery lead — buttons now do volume),
  enclosure/mount, and the flight audio target (earbuds vs panel/headset
  input, still open).
- Mark's v3 follow-ups (Nov 20): (1) volume button presses need audible
  feedback at the new level; (2) try ALL callouts as spoken numbers,
  no tones in the ladder (reverses the Nov 19 voice/tones split as a
  trial — the queue serializes clips, so a fast descent may run slightly
  behind; per-foot revert is a one-line change).
- CALLER v4: nine/eight/seven/six/four clips synthesized in the same
  voice (ten clips total, 302 KB flash, still fine in Huge APP).
  fireCallout is all-voice 10..1. Volume presses queue a 0.1 s pause
  (lets the buds apply the new AVRCP volume) then a 0.15 s 1500 Hz beep
  at the new level — the only tone left in the build, deliberately
  distinct. Awaiting bench verdict.
- CALLER v4 BENCH VERDICT (Nov 20): "That works quite well." All-spoken
  ladder 10-1 accepted (tones stay out unless a fast-descent trial in
  the plane says otherwise); volume confirmation beep accepted.
  BENCH PHASE COMPLETE. The caller as it stands: v4 sketch on the
  T-Display, core 3.3.13, Huge APP partition, ESP32-A2DP 1.8.12,
  Galaxy Buds Pro (F488) as the audio target. Everything left is
  install work, listed in the Nov 20 v3-verdict entry above.
- Mark's requirement (Nov 20): the caller must pair with ANY Bluetooth
  audio device, with a way to select which one. Verified against the
  ESP32-A2DP 1.8.12 headers first: start() with no name + set_ssid_callback
  is the library's designed selection hook; set_discovery_mode_callback
  reports when the scan stops.
- CALLER v5: chosen device name saved in NVS (Preferences "hcaller").
  Boot with no saved name (or a selection request) -> discovery scan,
  on-screen list (VOL- = next, VOL+ = choose, entries end with RESCAN and
  KEEP OLD), choice saved, board restarts and connects by name. Holding
  BOTH front buttons 2 s in normal use restarts into selection. Connect
  mode accepts only the saved name during any fallback scan. Known
  library limit: connection is by NAME (no start-by-MAC in the API), so
  it takes the first device with that name it finds. All selection runs
  at boot so the library only ever executes one start() per boot.
  Awaiting bench verdict.
- CALLER v5 VERDICT (Nov 20): selection "works very well." One hiccup:
  after selection, ~20 s of restart/scan/connect delay, during which
  callouts QUEUED and then shotgunned out all at once when audio came
  up. Root cause: is_connected() goes true when the A2DP signalling
  connects, well before the media stream flows and the library starts
  consuming the audio callback — segments piled up in the queue.
- CALLER v6 FIXES: audioReady() = connected AND the data callback has
  run within the last 1 s (proof sound is flowing); callouts and the
  volume beep queue only when audioReady; every segment is timestamped
  and dropped if it waits > 1.5 s; queue flushed on disconnect; when
  audio first goes live, the current height is announced once so the
  pilot is not left waiting for the next foot. The ~20 s connect delay
  itself is the BT scan/connect sequence and remains — it is now silent
  instead of backlogged. Awaiting bench verdict.
- CALLER v6 VERDICT (Nov 20): "Much better." The stale-callout shotgun
  is fixed. v6 is the current bench build. BENCH PHASE CLOSED.
  Remaining is install work only: WHEEL_OFFSET_FT calibration on the
  gear, 5 V boost module for the sensor rail on battery power, battery
  % on screen, power-off strategy, enclosure/mount, and the flight
  battery's arrival + polarity check.
- CALLER v7 (Nov 20): Mark asked for the wheel offset to be settable on
  the device itself (no web app; the caller has no Wi-Fi at all). The
  both-buttons 2 s hold now opens a SETTINGS menu instead of going
  straight to BT selection: BT DEVICE (the v5 selection flow),
  WHEEL OFFSET (list 2.0-4.0 ft in 0.5 ft steps, VOL- steps / VOL+
  sets, saved to NVS "offset"), DONE (restart to normal). The menu
  boot does not start Bluetooth unless BT DEVICE is picked, preserving
  the one-start()-per-boot rule. Normal screen now shows "OFF: x.x"
  next to the volume. wheelOffsetFt replaced the compile-time
  WHEEL_OFFSET_FT constant; default 0.0 until set. Awaiting bench
  verdict.
- V7 FLOW CONFUSION RESOLVED (Nov 20): Mark reported "No Wheel Offset
  presented" and BT DEVICE SELECT on both-hold. His IDE screenshot
  showed the truth in the Serial Monitor: "Height caller v7: settings
  menu" then "Settings: Bluetooth device selection" — v7 WAS running;
  the menu appeared and its first entry (BT DEVICE) was chosen, landing
  him in selection. The OFF: check misled because he was on menu
  screens, not the normal screen. Walked him out via KEEP OLD, then
  menu -> middle entry -> offset list. Also confirmed a real v7 defect:
  "WHEEL OFFSET" at text size 2 is 144 px wide on the 135 px screen and
  wrapped mid-word ("WHEEL OFF" / "SET").
- CALLER v8 (Nov 20): Mark asked for a smooth screen — v7 and earlier
  erased and repainted the whole panel every loop (~15x/s), a visible
  strobe. v8 is change-driven: draw functions return without touching
  the panel when nothing they show has changed; the normal screen
  repaints only the field patch that changed (height digits, BT line,
  VOL/OFF line, LAST CALL, zone line) and never blanks the panel;
  settings screens redraw only on button/scan changes. Menu WHEEL
  OFFSET is now a deliberate two-line entry. Saved BT device and
  offset carry over (same NVS keys). Awaiting bench verdict.
- V8/V7 MENU FLASH ROOT-CAUSED (Nov 20): Mark on v8 saw a screen pop
  up briefly, then the board went straight into BT selection. The flash
  WAS the SETTINGS menu. Cause: the menu opens via a 2 s both-button
  hold + ESP.restart(); the button edge detectors start un-pressed at
  boot, so the still-held VOL+ registered as a fresh "choose" on the
  menu's first poll and instantly selected BT DEVICE (entry 0). Same
  bug explains the v7 "No Wheel Offset presented" report and the
  Serial log sequence "settings menu" -> "Bluetooth device selection".
- CALLER v9 FIX: buttonsArmed gate — every settings screen ignores
  buttons until both have been released once after entry (the opening
  gesture must end before input counts). Applied to select/menu/offset
  screens and all transitions between them. Declaration-order checked
  (buttonsArmed declared with the UI state globals, before
  enterBtSelect). Awaiting bench verdict.
- CALLER v9 VERDICT (Nov 20): menu now stays on screen (gate worked),
  but VOL- did not scroll — the menu was deaf. Suspected root cause:
  btn35Idle was learned 50 ms after boot; the both-hold restart fires
  while the user is still holding the buttons, so the learn could
  capture the PRESSED level as idle, inverting GPIO35's read for the
  whole boot — then the gate's "both released" condition never became
  true and every settings screen ignored all input. (Same inversion
  may also explain the v8 instant-choose: a backwards B read looks
  like a held VOL+.)
- CALLER v10 FIXES: btn35Idle hard-coded HIGH (both T-Display buttons
  are active-low with board pull-ups, per the Nov 20 board research);
  the boot-time learn is deleted. Gate gains a 2.5 s timeout arm so it
  can never stay deaf. Serial prints added for gate-arm and menu steps
  to make any next round diagnosable from the monitor. Awaiting bench
  verdict.
- V10 "BROKEN DISPLAY/AUDIO" RESOLVED (Nov 20): Mark confirmed OFF was
  2.5 ft. Display 5.0 = ~7.5 ft probe distance minus 2.5 offset, and
  the lone "Four" was the only decisive band crossing at that geometry
  — the caller was working as designed. Mark accepted the diagnosis.
- CALLER v11 (Nov 20): at Mark's request the offset list is now
  0.0-4.0 ft in 0.5 ft steps (9 values, OFFSET_COUNT). 0.0 restores
  true probe-distance readings for bench work. His saved 2.5 remains
  valid and the list will open on it. Awaiting bench verdict.
- CALLER v11 MENU/OFFSET FLOW WORKS (Nov 20): Mark booted, buds
  connected, menu -> wheel offset -> selected a new value. His
  question: buds disconnected and never reconnected — is the menu
  resetting the BT device? ANSWER: no. Code audit: "btname" is written
  ONLY by activateSelection in select mode; the menu writes "menu",
  the offset flow writes "offset", setup consumes flags. The buds
  dropped because entering the menu restarts the board, and BT stays
  off for the whole settings session; v11 required DONE -> restart ->
  reconnect, and he was still sitting in the menu.
- CALLER v12: DONE in the menu now calls start(savedName) directly
  and enters normal operation with no second restart (menu boot never
  started the stack, so one-start-per-boot still holds). Awaiting
  bench verdict.
- CALLER v12 VERDICT (Nov 20, photo): leaving the menu via DONE left
  SETTINGS and the normal screen painted on top of each other
  ("SETTINGS" title, "BT DEVICE", "> DONE" ghosted under the normal
  fields; OFF: 0.5 visible — his new offset DID save; LAST CALL
  10 - VOICE, IN ZONE, BT: SEARCHING). Root cause: the loop drew the
  menu screen in the same pass that handled the DONE press, and that
  draw consumed the forceRedraw flag, so the normal screen never did
  its full clear and painted incrementally over the menu. Same flaw
  existed on the offset -> menu transition. Mark also reported BT did
  not reconnect after DONE (screen showed SEARCHING; possibly still
  inside the ~20 s connect window or the buds needed the case trick —
  the start() path itself is the same one that worked from the menu
  before).
- CALLER v13 FIX: loop dispatch guards — after polling, a settings
  screen draws only if the UI is still on that screen, so transitions
  hand forceRedraw to the incoming screen untouched. Awaiting bench
  verdict.
- TFMINI PLUS ORDERED (Nov 20): Mark ordered the weatherproof (IP65)
  TFmini Plus, arriving Sat Nov 21. He asked for a menu selection for
  the sensor used.
- CALLER v14: SETTINGS menu gains SENSOR (entries now BT DEVICE /
  WHEEL OFFSET / SENSOR / DONE). ULTRASONIC or TFMINI, saved in NVS
  "sensor" (getUChar, default ultrasonic). Both sensors share the
  same physical port: GPIO 25/26 + 5 V — the menu picks the protocol.
  LiDAR = Serial2 115200 on RX 26 / TX 25, 9-byte 0x59 0x59 frames,
  checksum-verified, strength >= 30 gate, frame must be < 300 ms old.
  applySensor() runs at boot and on menu DONE. Ladder deliberately
  unchanged (10 ft top) for apples-to-apples bench comparison; the
  20 ft ladder + clips for 11-20 is the follow-up once the LiDAR
  proves itself. Awaiting bench verdict.
- PAIRING-MODE-EVERY-TIME ROOT CAUSE + v15 FIX (Nov 20): Mark: buds
  must be put in pairing mode before every connect, even after reset.
  Verified in the ESP32-A2DP 1.8.12 source: reconnect_status defaults
  to NoReconnect (BluetoothA2DPCommon.h:381, despite a header comment
  claiming default-on), and set_last_connection() writes the peer
  address to the library's NVS ("connected_bda") ONLY when reconnect
  is enabled. Our sketch never called set_auto_reconnect, so no
  address was ever stored and every boot fell back to name discovery
  — which only sees discoverable (pairing-mode) devices. v15:
  set_auto_reconnect(true) on the normal boot start and the menu-DONE
  start; set_auto_reconnect(false) on both selection starts (a
  selection boot must run a real discovery). First good connect
  stores the address; later boots page it directly (buds just need
  to be on/connectable, e.g. out of the closed case). If the address
  never answers, the library falls back to the name scan by itself
  (AUTOCONNECT_TRY_NUM default 1000, heartbeat cycles). Awaiting
  bench verdict.
- CALLER v16 (Nov 20, Mark's request): speaker icon at the right end
  of the BT line, shown only while audioReady() (connected + audio
  callback ran within 1 s) — the unambiguous "callouts are live"
  signal, since BT: CONNECTED can precede flowing audio by seconds.
  Drawn/erased on state change only (its own patch); the BT line's
  repaint restores it in step so the two never desync. Awaiting
  bench verdict.
- CALLER v16 ADDENDUM (Nov 20, Mark's request while v16 was in
  flight): volume step now persists — saved to NVS "vol" in
  setVolumeIdx (the single choke point for button volume changes)
  and restored in setup before the initial set_volume. Fresh board
  still defaults to maximum (127). v16 ships with both the speaker
  icon and volume persistence.

## Queued for v17 (Mark, Nov 22, 2026)
- Battery monitor on screen: Mark asked that the next firmware tweak add a battery readout. The T-Display already senses battery voltage on GPIO34 (onboard divider) — no new parts. Show % (and/or icon) on the normal screen; voltage-based LiPo % is approximate (flat mid-curve) and dips under the 5 V boost load, so average the readings and treat it as a low-battery warning rather than a precision gauge. Rides with whatever change triggers v17.

## Nov 22, 2026 — TFmini bench session
- v16 uploaded and running (Mark confirmed).
- v15 auto-reconnect verdict: PASS — after a pairing-mode connect stored the address, a board reset with the buds simply out of the case connected on its own ("Yep, all good").
- v16 verdicts (Nov 22, 2026): PASS — speaker icon shows only while audio flows; volume set below max survives a power cycle; SETTINGS -> DONE returns a fully clean normal screen (v13 hand-off also PASS). Mark: "All verified."
- TFmini swap (Nov 22, 2026): wired via the Dupont adapter on GPIO 25/26 + 5 V (no divider, no blue board), SENSOR menu set to TFMINI, offset zeroed. Works on first wiring (no middle-wire swap needed). Mark's verdict: "much crisper" than the ultrasonic. No callouts above 10 ft — expected: v16's ladder tops at 10 ft by design.
- Range test (Nov 22, 2026): TFmini rock solid at every distance to 19.6 ft, then dashes (NO RETURN). ROOT CAUSE FOUND in v16 code, line 1075: the main loop's shared validity gate `valid = (d > 0 && d < 600)` — an ultrasonic-era cap (the ultrasonic's no-echo default is ~604 cm) — discards TFmini frames at >= 600 cm (19.7 ft), even though the TFmini parser itself accepts frames to 1190 cm at strength >= 30. The sensor never showed its real ceiling. v17 fix: make the cap sensor-aware (keep <600 for ultrasonic; ~1250 for TFmini).
- v17 scope is now: (1) sensor-aware distance cap, (2) 20 ft ladder + new voice clips 11-20 (Helio voice), (3) battery monitor on screen (Mark's request, Nov 22).
- v17 display scope added by Mark (Nov 22, 2026): enlarge the VOL readout, the wheel-offset (Off) readout, the speaker/audio icon, and the new battery reading — and the last-callout / zone message line can be eliminated to make room.
- v17 BUILT and delivered Nov 22, 2026 (~/workspace/height-caller/arduino/height_caller_v17/): (1) sensor-aware validity cap (TFmini 1250 cm, ultrasonic 600 cm); (2) 20 ft ladder — clips for eleven..twenty generated in the same Helio voice (avocado_v2:MAI_03) via make_clips_v17.py; voice_clips.h now 20 clips, 377,616 samples / 755,232 bytes (the regenerated 1-10 clips are sample-identical to v4's); zone ceiling 21.0/21.5, callouts 1..20; (3) battery monitor — GPIO34 divider (GPIO14 enable), analogReadMilliVolts x2, LiPo curve lookup, BAT line size 2, colour-coded, "BAT: ---" below 2.5 V (no battery); (4) VOL/OFF lines at text size 2, speaker icon ~1.5x, LAST CALL and IN ZONE/ABOVE ZONE lines removed. Diff-verified against v16; braces balance; no orphaned references. NOT compile-tested here (no Arduino toolchain in the VM) — Mark's IDE compile is the check, as with prior versions. Awaiting bench verdict.
- v17 verdict (Mark, Nov 22, 2026): "Good build." TFmini now reads to 37 ft — the limit of his test space, not the sensor (rated 12 m / ~39 ft). The 600 cm cap is confirmed gone and the 20 ft ladder build is accepted.
- ANOMALY reported by Mark Nov 22, 2026: callouts occasionally illogical (e.g. 8, 9, 10, 2, 11, 12, 12, 11, 5, 10, 9, 1, 8). Cause: a few wild frames (reflections) defeat the 3-reading median and the ladder announces them. v18 BUILT and delivered Nov 22, 2026 (~/workspace/height-caller/arduino/height_caller_v18/): callout plausibility gate — speakBand() is the only path from the ladder to fireCallout(); a step is spoken only if physically plausible (1 ft, or more if elapsed time allows at 10 ft/s); implausible steps are squelched and watched — if readings hold at the new level 750 ms it is spoken once (re-sync), if they wander off nothing is said. Zone exits and the audio-live announce reset/sync the gate. Squelches log to Serial ("Callout squelched: X right after Y"). Diff-verified vs v17 (85 changed lines), braces balance, not compile-tested here. Awaiting bench verdict.
- POWER observation (Mark, Nov 22, 2026): the entire v17 test session, including the 37-38 ft reading, ran on BATTERY power (USB disconnected) with the TFmini fed from the board's 5V pin. DVOM at that pin: 4.7 V on USB, 3.5 V on battery. So the pin back-feeds unregulated battery voltage (minus a diode drop) when USB is absent, and the TFmini Plus demonstrably runs at 3.5 V - below its 4.5 V spec minimum - at full range. Caveat: that voltage sags as the pack drains; behaviour near empty is untested. Buster's recommendation stands: fit the boost regulator he already has for a constant in-spec 5 V; alternative is a bench discharge test to find where (if anywhere) the sensor degrades.
- POWER SYSTEM COMPLETE (Mark, Nov 22, 2026): boost regulator wired in per schematic_v1.png (battery -> switch -> board + regulator V1; Vo -> sensor red; common ground). Vo measured 5.26 V - inside the TFmini Plus 4.5-5.5 V window (these modules sit slightly high to offset cable drop). All working on battery power with the sensor on regulated 5 V. Flight battery polarity confirmed good by operation.
- v19 BUILT and delivered Nov 22, 2026 (~/workspace/height-caller/arduino/height_caller_v19/): DISPLAY item added to the SETTINGS menu (5 items now: BT DEVICE / WHEEL OFFSET / SENSOR / DISPLAY / DONE) - UPRIGHT or FLIPPED (180-degree rotation), saved to NVS "rot", applied at boot before any text prints and applied live the moment it is chosen (menu repaints flipped for confirmation). Button swap: btnAPressed/btnBPressed are the only pin readers and they cross-map when flipped, so VOL- stays on the user's left in either mounting; the buttons gate reads through the helpers, so the swap cannot phantom-press. Menu screen gained a "Display:" status line (rows re-spaced). Diff-verified vs v18 (140 changed lines), braces balance, not compile-tested here. Awaiting bench verdict. NOTE: v18's squelch gate is in v19 too; its bench verdict is still outstanding.
- v19 verdict (Mark, Nov 22, 2026): "Works like a charm." DISPLAY flip + button swap verified on the bench. (The v18 squelch gate rides along in v19 but has no separate explicit verdict yet.)

## Idea noted Nov 24, 2026 — GPS speed gate for callouts
Mark is considering adding a small GPS module (e.g. Beitian BN-220, UART on free pins, 3.3 V) so groundspeed can gate the callouts — silent in cruise, armed on approach. GPS altitude is NOT for height (way too coarse); speed/logging only. Antenna needs sky view (mount antenna-up in the box). Mark will decide after flying v19, hopefully the weekend of Nov 28-29, 2026, if the strut mount arrives (offer of $89 placed Nov 24 on an open-box FlightFlix RS Slim Strut Mount 2, eBay).

## Nov 24, 2026 — Finished prototype
Mark shared a photo of the completed unit in his own redesigned printed box (grey): power slide switch on top, display window live with a height reading, USB-C connected, sensor end fitted. Box redesign and print are his; the caller is now a finished prototype awaiting the strut mount (eBay RS Slim, ETA Nov 27-30) for flight test.

## Nov 24, 2026 — v20 built (ladder selection + full range)
Mark's idea after the film discussion: with the lens film off, the TFmini may reach past 40 ft. v20 adds a SETTINGS > LADDER item (NVS "ladder", default ORIGINAL): ORIGINAL = the v19 20 ft ladder; FULL RANGE = zone arms at 50 ft, callouts 40/35/30/25 (new Helio clips, 24 clips total, 912,716 bytes flash; 1-20 sample-identical to v19), then every foot from 20. Callouts speak as the height descends through the number (band-relative hysteresis, exactly equivalent to v19's rules on the 1 ft ladder - verified by simulation: full descent gives 40@40.8, 35@35.8, 30, 25, 20@20.8, then 19..1; original descent unchanged). TFmini parser + validity caps raised 1190/1250 -> 1550 cm. Menu now 6 items, re-spaced; status block gained a "Ladder:" line. Not compile-tested in the VM (no toolchain) - Mark's IDE compile is the check, as always. Bench verdict pending. NOTE: sensor is rated 12 m (39.4 ft); returns past that are gravy.

## Nov 24, 2026 — v20 bench verdict
Mark, minutes after delivery: "Works like a charm." v20 (LADDER menu + full-range ladder) is bench-accepted. Flight test still awaits the strut mount (ETA Nov 27-30).

## Nov 24, 2026 — v21 built (USB POWER line)
Mark noticed the battery line pegs at 100% on USB (charger holds the battery node at ~4.2 V) and asked for a USB indication. The T-Display exposes no VBUS-detect pin, so v21 infers USB from voltage: readBatteryVolts() + batteryPct(volts) refactor; a latch takes "on USB" at >= 4.18 V and releases below 4.12 V; the BAT line then shows green "USB POWER" (his requested "USB Connected" is 13 chars = ~156 px at size 2, too wide for the 135 px screen). On battery the percent shows as before; no battery still shows "BAT: ---". Known brief false positive: a freshly charged battery just unplugged can read as USB until its voltage settles under load. voice_clips.h identical to v20. Bench verdict pending.

## Queued for the next iteration (noted Nov 24, 2026)
- "USB POWER" line: change colour from green to blue (ST77XX_BLUE or a lighter blue for readability). Mark: "Don't do it now" - fold into the next version whenever one happens.

## Nov 24, 2026 — Project named: FlareBuddy
Mark picked the brand name "FlareBuddy" for the height caller (from a spitball list; FlareCall and Pocket Radalt were the runners-up). Queued for the next iteration alongside the USB POWER colour change: put FLAREBUDDY on the screen header in place of / alongside "HEIGHT - FEET".

## Nov 24, 2026 — Design fact: lid screws are M3 × 6
Mark confirmed the lid screws in his box design are M3 × 6 (not the M3 × 10 in Buster's early drafts). Hardware README corrected accordingly.

## Nov 25, 2026 — battery gauge behaviour on v21 (Mark's report)
Charged overnight on USB, switch ON (correct charge configuration). On unplug the unit stayed on (battery took over seamlessly — charge path proven working). Gauge wandered: 35% at unplug, floating up to ~49%; after a 10 s power cycle it showed 70%, settling down to ~55%. Explained to Mark: the gauge is a voltage lookup, not a fuel gauge — readings sag under load (display + BT + boost + sensor), recover at rest, and sit high briefly after a charge (surface charge); the LiPo curve is flat mid-range so ~50 mV swings the percentage 15-20 points. All four numbers are the same nearly-full battery. Trustworthy check: DVOM on the pack at rest (>=4.15 V = full). A steadier gauge would need heavier averaging in firmware or a coulomb-counter IC; noted as a possible future tweak, not queued.

## Nov 25, 2026 — v22 built and delivered
Mark called the percentage gauge "very misleading" and asked to try showing actual voltage. v22 (diff vs v21 = header note, battery block, header text only; voice_clips.h identical): (1) BAT line shows volts to 2 decimals ("BAT: 4.05V"), repaint keyed on centivolt change; colour bands by volts — green >= 3.90, yellow 3.70-3.89, red < 3.70; batteryPct() lookup removed. (2) USB POWER now sky blue 0x867D (queued item from Nov 24). (3) Screen header now reads FLAREBUDDY (centred, cursor x=37) instead of HEIGHT - FEET (queued item from Nov 24). USB latch unchanged (>= 4.18 in, < 4.12 out); BAT: --- unchanged. Delivered as height_caller_v22.zip; only the .ino needs swapping into the standing height_caller folder. Bench verdict pending.
v22 verdict (Nov 25): "Yep, that's better. Not as misleading." Open concern from Mark: after a full night on USB (switch ON), the running unit shows 3.90 V (yellow band) — he expected ~4.2 V/full. Buster's diagnostic given: rested-voltage DVOM test decides it — unplug USB, switch off, wait ~10 min, measure the pack at its plug. >= 4.15 V rested = pack IS full and 3.90 is load sag at the running gauge; ~3.9 V rested = genuinely undercharged, next suspects are a weak USB source (charger + running load vs a 500 mA port) or a tired pack (compare drawer battery #2). Awaiting his measurement.
Mark measured the flight pack at rest, disconnected from everything: 3.877 VDC. Verdict: NOT fully charged — a full LiPo rests at 4.15-4.20 V; 3.877 is roughly 60%. So the overnight USB charge genuinely under-delivered; the v22 voltage display was telling the truth (running 3.90 V vs rested 3.877 — board ADC within ~0.02 V of his DVOM). Next diagnostic given: A/B test — same overnight charge for drawer battery #2 (known good). If #2 rests at 4.15+, the flight pack is tired; if it also lands ~3.9, the charging setup is at fault (weak USB source sharing with the running unit, or the board's modest charge current). Awaiting result.
v23 built and delivered Nov 25, 2026 (compile-verified on the rebuilt toolchain: arduino-cli 1.6.0 in ~/workspace/tools, esp32 core 3.3.13, board lilygo_t_display, Huge APP; 2,068,229 B / 65% flash). Two changes from v22, both Mark's requests: (1) battery colour bands lowered - green >= 3.75 V, yellow 3.55-3.74, red < 3.55 (was 3.90/3.70), because his pack rests at 3.877 V fully charged on the board's charger. (2) CHARGING display: on USB with 5 minutes of no activity (button helpers stamp lastActivityMs; audio flowing also counts), the battery line shows CHARGING and every other field turns off; any press/callout restores the full screen. Flashing does not count as activity (app not running during download; idle timer starts at boot). Awaiting bench verdict - he plans to flash the SPARE T-Display (original board lost a pushbutton; replacement switches are 3x4x2mm 4-pin SMD, on order, a few weeks out).
Nov 25, 2026: Mark flashed a board (COM10) and pasted the boot log. Clean boot, Bluetooth connected. Two things: (1) the serial banner still prints the stale literal "Height caller v16" - that string has been carried unchanged since v16 in every version including v23; cosmetic only, fix the banner to print the real version in the next iteration. (2) The board's saved NVS settings are NOT the flight board's Nov 24 state: BT target "Scosche BTAXS3" (and it connected - so a Scosche dongle is on hand and paired to this board), wheel offset 1.0 ft, ladder FULL RANGE, sensor TFmini Plus. So whichever board this is, it carries prior v20+ configuration. Flagged to Mark: if he wants flight-test parity with earlier bench tests, ladder back to ORIGINAL; offset is a placeholder until measured at install either way.
v24 built and delivered Nov 25, 2026 (compile-verified, 2,068,237 B / 65% flash). Bug fix: Mark bench-tested v23 with the Scosche BTAXS3 panel dongle connected (deliberate - testing the panel-audio path) and CHARGING never appeared after 5+ idle minutes. Cause: v23 treated audioReady() as activity, but audioReady() = connected + data callback within 1 s, and a connected A2DP stream runs its callback continuously even in silence - so activity was re-stamped every 2 s forever. v24 stamps activity only on button presses, sounds actually queued (queueSegment), and BT connect/disconnect. Also fixed the serial banner to print the real version (v24) instead of the stale "v16". Awaiting bench verdict.
Nov 25, 2026: Mark confirmed v24's CHARGING display works ("Nice, it worked") with the Scosche connected. v25 built and delivered same day (compile-verified): CHARGING word centred on the blank charging screen (cursor 19,112 - 96x16 px text on the 135x240 panel) instead of sitting on the bottom battery line. Banner bumped to v25. Awaiting bench verdict.
v26 built and delivered Nov 25, 2026 (compile-verified). Mark's request: bigger sub-menu fonts, BT screen specifically. BT DEVICE screen is now size 2 throughout (wording trimmed to the ~10-char size-2 line limit: "Entry 1/3", "VOL+ : pick"); all sub-screen headers and footers size 2; SETTINGS header size 2; offset range note size 2. Explanatory multi-line notes (sensor/display/ladder) and the menu status block stay size 1 - size 2 would crowd out the footers. printWrapped gained a pitch parameter (20 px at size 2); the device name previously wrapped at size 2 with a 12 px pitch. Awaiting bench verdict.
v27 built and delivered Nov 25, 2026 (compile-verified). Trigger: the Scosche BTAXS3 (received today) paired fine with FlareBuddy earlier in the day, then would not finalize pairing after Mark paired it with his phone - the classic stale-bond failure (phone pairing changed the Scosche's key; FlareBuddy kept the old one). v27: choosing a device in the BT screen first calls esp_bt_gap_remove_bond_device for it (discovery callback now stores addresses); KEEP OLD clears all stored bonds; 500 ms delay lets the stack commit before the selection restart. Also confirmed for Mark: the phone's "Pair" tap is Android's pairing confirmation, which the ESP32 library auto-answers - there is no missing step on the FlareBuddy. Awaiting bench verdict: re-select the Scosche (phone BT off) and see if it pairs clean.
v28 built and delivered Nov 25, 2026 (compile-verified). ROOT CAUSE FOUND for the Scosche pairing saga, in the ESP32-A2DP library source: the library stores the last SUCCESSFULLY connected device's address in its own NVS (set_last_connection on every connect) and start() with auto-reconnect pages ONLY that address; the saved name is ignored until retries run out. Mark's bench evidence nailed it: Bose A20 (paired earlier, last success) connected instantly with no menu while the target was set to the buds/Scosche; buds and Scosche, though picked in the menu and sitting in pairing mode (Scosche LED flashing = never contacted), looped Connecting/Disconnected. So the fault was never pairing keys - v27's bond-clearing addressed a non-cause (kept; harmless and occasionally useful). v28: selection saves the picked device's address to our NVS ("btaddr"); both connect paths call set_auto_reconnect(savedAddr) before start(), which plants that address in the library (start() only loads its NVS address when none is planted). Serial prints "Auto-reconnect armed for picked address XX:...". Test protocol: pick Scosche -> should connect; pick buds -> should connect to buds; A20 nearby should no longer hijack.
Nov 25, 2026 (evening): Mark confirms v28 works - the Scosche BTAXS3 connected after being picked in the menu. The address-paging diagnosis is proven on the device that failed all day. Earbuds test to follow.
Nov 25, 2026 (evening): v28 FULLY VERIFIED by Mark - "we have a working system." Round-robin test on one board: Scosche BTAXS3, Galaxy Buds, Bose A20, buds again, BTAX again - each time the device had to be picked in the BT menu again (expected: the board pages only the one picked address, and v27's bond-clear makes each pick a fresh pairing), and every one connected properly. Device switching on FlareBuddy is therefore: hold both buttons -> SETTINGS -> BT DEVICE -> pick -> restart -> connected. The A20 also works with FlareBuddy (first mention: Bose A20 headset paired earlier today).
