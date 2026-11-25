# FlareBuddy

**A landing height caller for light aircraft.** A small LiDAR looks down
from the airframe and FlareBuddy speaks your height above the ground —
"forty… thirty-five… thirty… twenty-five… twenty… nineteen…" — through
Bluetooth earbuds, a headset, or the intercom, in the last seconds of the
landing flare. It's the callouts an airliner's radio altimeter makes,
for an airplane that will never have one.

Built for an AirCam (twin-engine open-cockpit backcountry plane), but it
will ride on anything with a strut or a belly to strap it to. It is
completely self-contained — its own battery, its own sensor, Bluetooth
audio — and needs nothing from the aircraft. Strap it on, fly, take it
off.

**Hear it:** [the Full Range ladder, 40 ft down to 1](flarebuddy_callouts_40_to_1.mp3)
— a 26-second MP3 assembled from the actual firmware voice clips,
with a deliberate pause between the 5-foot steps.

> **Advisory aid only.** FlareBuddy is an experimental, uncertified
> homebuilt project. It is not a substitute for pilot judgment, a
> calibrated instrument, or your own eyes. Fly the airplane.

## What it does

- **Two callout ladders**, selectable on the device:
  - **Original** — every foot from 20 ft down to 1 ft.
  - **Full Range** — arms at 50 ft and calls 40, 35, 30, 25, then every
    foot from 20 down.
- **Bidirectional** — if you balloon back up, it calls the heights again
  on the way up.
- **Spoken voice, not beeps** — real synthesized speech clips (a
  generated voice, baked into the firmware as PCM).
- **Callout plausibility gate** — a step the airplane cannot physically
  have made (a reflection off a hand, clothing, the gear leg) is
  squelched instead of announced. If the new height holds for ¾ second
  it was real after all, and is announced once.
- **Median-filtered sensor** readings, with a signal-strength gate on
  the LiDAR.
- **On-device settings menu** (hold both buttons 2 s) — Bluetooth
  device, wheel offset (0.0–4.0 ft in 0.5 ft steps, so callouts read
  *wheel* height, not sensor height), sensor select, display flip for
  upside-down mounting, ladder select. Everything persists across
  power cycles.
- **Battery gauge** on screen, colour-coded — and it knows when it's on
  USB power and says so instead of pegging at a misleading 100%.
- **Any Bluetooth A2DP target** — earbuds (Galaxy Buds Pro proven), or a
  small Bluetooth receiver plugged into the audio panel's aux input so
  callouts come through the intercom.

## Hardware in one paragraph

A LilyGo **TTGO T-Display** (classic ESP32 — Bluetooth Classic A2DP is
the whole point; ESP32-S3 boards are BLE-only and *cannot* stream this
audio) reads a **Benewake TFmini Plus** LiDAR over UART and streams the
callouts with the ESP32-A2DP library. A boost regulator gives the sensor
a clean 5 V from the 3.7 V LiPo flight battery regardless of charge
state. The whole unit — board, battery, sensor — lives in one small
3D-printed case on a GoPro-pattern fork, strapped to the strut with a
cushioned strut mount. Full parts list, wiring, firmware setup, and
install notes are in **[HOWTO.md](HOWTO.md)**.

## Status

Bench-complete and bench-verified at firmware **v21** (the sketch's
header comment carries the full version history, v1–v21; the bench
record is in [BENCH_LOG.md](BENCH_LOG.md)). The prototype is built and
printed; first flight test is pending the strut mount's arrival.

Known honest limits:

- The TFmini Plus is rated to 12 m (39.4 ft). It read solidly to 37 ft
  on the bench *through its protective lens film*; the Full Range
  ladder arms at 50 ft, but returns past the rated range are gravy,
  not a promise. The first callout you actually hear may be 40 — or 35.
- The battery percentage is a voltage-based gauge: honest as a
  low-battery warning, not a precision instrument.

## Similar systems

FlareBuddy is not the first landing height caller — the idea descends
from the airliner radio altimeter, and commercial GA versions exist:

- **MicroKit Landing Height System** — laser-based, announces
  200 ft down to 1 ft, NORSEE-approved for any GA airplane as a minor
  alteration. $1,065 ($1,165 for the 200 series with gear warning).
- **Holy Micro! SkyVoice Alert 500** — LiDAR, takeoff and landing
  callouts from 500 ft, NORSEE-approved. $1,450.
- **EngineBridge** also sells an installed landing height system.

All of them are *installed* equipment — wired into aircraft power and
the intercom — and their lasers reach 200 ft. FlareBuddy is the
opposite trade: nothing installed, nothing wired, its own battery and
Bluetooth audio, **about $85 in electronic parts — under $100 all-in**
with the printed case and hardware — at the cost of a ~40 ft sensor
ceiling. If your airplane is certified and you want callouts from
200 ft, buy one of theirs. If you want to strap something on for the
weekend, build one of these.

## Repository layout

- `firmware/height_caller_v21.ino` — the sketch (Arduino / ESP32 core
  3.3.x, **Huge APP** partition scheme — see the HOWTO).
- `firmware/voice_clips.h` — the spoken callouts as PCM arrays.
  Generated, do not edit by hand.
- `voice/make_clips_v20.py` — regenerates `voice_clips.h` from the
  per-word MP3s (44100 Hz mono 16-bit, trimmed, peak-normalized).
- `HOWTO.md` — build it, wire it, flash it, set it up, install it.
- `BENCH_LOG.md` — the running bench record, dead ends included.

## Credit

Designed and built by Mark Harris with Buster (his Muse assistant).
The callout voice is a synthesized voice ("Helio"), generated for this
project.
