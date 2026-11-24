// height_caller_v21.ino - AirCam landing height caller.
// v21 changes from v20 (Mark's request, Nov 24): the battery line
// no longer cries wolf on USB. The board has no USB-detect pin, so
// USB is inferred from the battery voltage: with USB plugged in,
// the onboard charger holds the battery node at ~4.2 V and the old
// gauge pegged at 100% whatever the battery was really doing. Now
// a sustained >= 4.18 V (latch releases below 4.12 V) shows
// "USB POWER" in green instead of a percentage; on battery the
// percentage shows as before. A freshly charged battery just
// unplugged can read as USB for a short while until its voltage
// settles under load - the only false positive, and a brief one.
// v20 changes from v19 (Mark's requests, Nov 24): a LADDER item in
// the SETTINGS menu - ORIGINAL (the 20 ft ladder, every foot) or
// FULL RANGE. Full range arms the zone at 50 ft and calls 40, 35,
// 30 and 25 on the way down (new voice clips, same Helio voice),
// then every foot from 20 as before - a callout is spoken as the
// height descends through its number, the same threshold rule the
// 1 ft ladder always used. Saved to NVS ("ladder"); the original
// ladder is the default. The TFmini validity caps rise to 1550 cm
// so genuine long returns are not discarded (the sensor is rated
// to 12 m - anything past that is gravy, and still strength-gated).
// v19 changes from v18 (Mark's request, Nov 22): a DISPLAY item in
// the SETTINGS menu - UPRIGHT or FLIPPED (screen rotated 180
// degrees) - for boxes that mount the board USB-end down. Saved to
// NVS ("rot"), applied at boot before anything draws, and applied
// the moment it is chosen so the menu itself flips for confirmation.
// The button helpers swap roles when flipped: the physical buttons
// trade places, so VOL- stays on the user's left and VOL+ on the
// right in either mounting. Set once at install; survives power
// cycles and firmware updates.
// v18 changes from v17 (Mark's request, Nov 22): a callout quality
// check. The sensor can hand the ladder a wildly wrong distance for
// a few frames - a reflection off a hand, clothing, the gear leg -
// and two bad frames in a row defeat the 3-reading median, so the
// caller would announce heights the plane cannot have reached:
// 10, then 2, then 11, then 5. A real AirCam cannot teleport, so
// every ladder announcement now passes through speakBand(): a step
// is spoken only if it is physically plausible - one foot, or a
// bigger step that the time since the last spoken callout makes
// possible at a generous 10 ft/s. An implausible step is squelched
// and watched: if the readings hold at the new level for 750 ms it
// was real after all (sensor re-aimed, genuine step) and the
// current height is spoken once; if they wander off, nothing is
// ever said. In smooth flight every step is one foot, so the gate
// never engages.
// v17 changes from v16 (Nov 22 bench session + Mark's requests):
// (1) The TFmini Plus proved itself on the bench - rock solid at
// every taped distance - so the callout ladder stretches to 20 ft:
// new voice clips for eleven through twenty (same Helio voice,
// avocado_v2:MAI_03), zone ceiling raised to 21 ft, callouts for
// bands 1..20. (2) The main loop's validity cap is now sensor-aware:
// v16 discarded every reading at or beyond 600 cm - a leftover from
// the ultrasonic, whose no-echo default is ~604 cm - which made the
// LiDAR dash out at 19.6 ft even though its parser accepts frames to
// ~39 ft. TFmini readings are now valid to 1250 cm; the ultrasonic
// keeps its 600 cm guard. (3) Battery monitor (Mark's request): the
// T-Display senses its battery on GPIO34 through the onboard divider
// (enabled by GPIO14); the screen shows BAT: percent, averaged over
// 16 samples every 2 s, colour-coded (green >= 50, yellow 25-49,
// red < 25). Voltage-based LiPo percent is approximate - a low-
// battery gauge, not a precision instrument. Below 2.5 V it shows
// BAT: --- (no battery fitted, e.g. USB bench power). (4) Screen:
// VOL, OFF and BAT lines are now text size 2, the speaker icon is
// drawn ~1.5x larger, and the LAST CALL and IN ZONE / ABOVE ZONE
// lines are removed to make room (Mark's request).
// v16 changes from v15 (Mark's requests, Nov 20): (1) A speaker icon
// now appears at the right end of the BT line only while audio is
// actually flowing (the v6 audioReady test: connected AND the audio
// callback has run within the last second). "BT: CONNECTED" alone
// can precede usable audio by several seconds; the icon is the
// unambiguous "callouts are live" signal for a new user. (2) The
// volume step is saved to NVS ("vol") on every button change and
// restored at boot - it no longer resets to maximum on a power
// cycle. Default on a fresh board is still maximum.
// v15 changes from v14 (Mark's bench report, Nov 20): the earbuds had
// to be put into pairing mode before EVERY connect, even after a
// reset. Root cause, verified in the ESP32-A2DP 1.8.12 source: the
// library's reconnect_status defaults to NoReconnect, and our sketch
// never enabled it - so the library never saved the connected
// device's address, and every boot fell back to name discovery,
// which only sees devices in pairing mode. Fix: set_auto_reconnect
// (true) on the normal start paths, so the first successful connect
// stores the device address (the library keeps it in its own NVS)
// and later boots PAGE that address directly - no discovery, no
// pairing mode, the buds just have to be on. Selection boots keep it
// OFF: selection must run a real discovery scan, not page the old
// device. If the paged address never answers, the library falls back
// to the name scan on its own.
// v14 changes from v13 (Mark's request, Nov 20): the weatherproof
// TFmini Plus LiDAR is coming, so the SETTINGS menu gains a SENSOR
// entry - ULTRASONIC or TFMINI, saved in NVS ("sensor"). Both sensors
// share the same physical port (GPIO 25/26 + 5 V); the menu picks
// which protocol the firmware speaks on those pins. The LiDAR talks
// UART at 115200 (Serial2), 9-byte 0x59 0x59 frames; a reading counts
// only with signal strength >= 30 and a frame fresher than 300 ms.
// The callout ladder is unchanged (10 ft top) so the two sensors can
// be compared apples-to-apples on the bench; the 20 ft ladder is the
// follow-up build once the LiDAR proves itself.
// v13 changes from v12 (Mark's photo, Nov 20): leaving the menu via
// DONE left the SETTINGS menu and the normal screen painted on top of
// each other. Cause: the loop called the menu's draw function in the
// same pass that processed the DONE press, and that draw consumed the
// forceRedraw flag meant for the normal screen - so the normal screen
// never did its one full clear-and-paint, and its incremental patches
// landed on top of the menu. (The offset -> menu transition had the
// same flaw.) Fix: after polling buttons, a screen is drawn ONLY if
// the UI is still on that screen; state transitions now hand the
// forceRedraw to the incoming screen untouched.
// v12 changes from v11 (Mark's bench report, Nov 20): after setting an
// offset in the menu, his earbuds stayed disconnected. Nothing was
// wrong with the saved device - entering the menu restarts the board
// (dropping Bluetooth), Bluetooth stays off while in settings, and in
// v11 the only way back was DONE -> another restart -> reconnect.
// v12 smooths the exit: DONE now starts Bluetooth to the saved device
// directly and drops into normal operation with no second restart
// (the menu boot never started the stack, so the one-start()-per-boot
// rule still holds). The saved BT device is never written by the menu
// or offset flows - only the "offset" key is.
// v11 changes from v10 (Mark's request, Nov 20): the wheel offset list
// now runs 0.0 to 4.0 ft in 0.5 ft steps (was 2.0-4.0). The 0.0 entry
// makes bench testing read true probe distance again. (The v10 scare -
// screen showing 5.0 and only "Four" announced - was the 2.5 ft offset
// working as designed against a ~7.5 ft bench distance, confirmed by
// Mark.)
// v10 changes from v9 (Mark's bench report, Nov 20): in the menu, VOL-
// did nothing at all. Suspected cause: the VOL+ button's resting level
// was LEARNED in the first instant after boot - and the both-hold
// restart that opens the menu fires while the user is still holding
// the buttons, so the board could learn "pressed" as the resting
// level. The button then read backwards all boot, and the v9 safety
// gate (wait for both buttons released) could never stand down,
// leaving the menu deaf. Fixes: the resting level is now hard-coded
// HIGH (both T-Display buttons are active-low with board pull-ups),
// and the gate also stands down on a 2.5 s timeout so it can never
// stay deaf even if a button read sticks.
// v9 changes from v8 (Mark's bench report, Nov 20): the SETTINGS menu
// flashed up and instantly jumped into BT selection. Cause: the menu
// opens via a 2 s both-button hold + restart, and the still-held VOL+
// button counted as a fresh "choose" press on the menu's first poll,
// selecting BT DEVICE before the user ever saw the menu. (This also
// explains the v7 "no wheel offset" confusion.) Fix: every settings
// screen now ignores the buttons until BOTH have been released once -
// the opening gesture has to end before any input counts.
// v8 changes from v7 (Mark's bench report, Nov 20): the screen strobed -
// every screen was fully erased and repainted many times a second. Now
// nothing is repainted unless what it shows actually changed: the
// normal screen updates field by field (only the patch that changed is
// erased and redrawn, the panel is never blanked), and the settings
// screens redraw only on a button press or a scan update. Also fixed:
// the SETTINGS menu's WHEEL OFFSET label was too wide for the screen
// at that font size and wrapped - it is now a deliberate two-line entry.
// Everything else is v7: on-device settings menu (hold both buttons
// 2 s) with Bluetooth device selection and the wheel offset list
// (2.0-4.0 ft in 0.5 ft steps), both saved in NVS; stale-callout
// protection; all-spoken callouts 10..1 counting up and down;
// median-filtered sensor; volume buttons with confirmation beep.
// Board: LilyGo T-Display (classic ESP32), ESP32 core 3.3.13,
// Partition Scheme: Huge APP. Libraries: ESP32-A2DP 1.8.12, Adafruit GFX,
// Adafruit ST7735 and ST7789. voice_clips.h must sit in this sketch folder.

#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Preferences.h>
#include "BluetoothA2DPSource.h"
#include "voice_clips.h"

#define TFT_CS 5
#define TFT_DC 16
#define TFT_RST 23
#define TFT_BL 4

#define TRIG_PIN 25
#define ECHO_PIN 26
#define LIDAR_RX_PIN 26   // TFmini TX -> GPIO26 (the Echo pin)
#define LIDAR_TX_PIN 25   // TFmini RX -> GPIO25 (the Trig pin)

#define BATT_ADC_PIN 34   // battery voltage via the onboard 1:2 divider
#define BATT_EN_PIN 14    // ADC power enable for the divider (LilyGo)

#define BTN_A 0     // front button on GPIO0 (pressed = LOW)   - VOL- / NEXT
#define BTN_B 35    // front button on GPIO35 (learned idle)   - VOL+ / CHOOSE

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
BluetoothA2DPSource a2dp_source;
Preferences prefs;

// Set whenever a different screen (or a fresh boot) takes over the
// display; the draw functions do one full paint, then go incremental.
bool forceRedraw = true;

// ---------------- Settings state ----------------
// Wheel offset: the probe's height above the ground when the plane is
// parked on its gear, in feet. Callouts report WHEEL height =
// sensor reading minus this. Set from the on-device list (NVS "offset").
float wheelOffsetFt = 0.0;
int sensorSel = 0;             // 0 = ultrasonic, 1 = TFmini Plus (NVS)
int sensorIdx = 0;             // cursor in the sensor list
bool flipDisplay = false;      // screen rotated 180 degrees (NVS "rot")
int dispIdx = 0;               // cursor in the display list
int ladderSel = 0;             // 0 = original 20 ft, 1 = full range (NVS "ladder")
int ladderIdx = 0;             // cursor in the ladder list
// TFmini Plus UART parser state (Serial2 on the ultrasonic's pins).
float lastLidarCm = -1;
unsigned long lastLidarMs = 0;
uint8_t lidarBuf[9];
int lidarIdx = 0;
const float OFFSET_VALUES[9] = {0.0, 0.5, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0};
const int OFFSET_COUNT = 9;
int offsetIdx = 0;

// UI states for the settings flow (normal running is all zero/false).
bool selectMode = false;    // collecting + choosing a Bluetooth device
int settingsState = 0;      // 1 = menu, 2 = offset, 3 = sensor, 4 = display, 5 = ladder
int menuIdx = 0;   // 0 = BT DEVICE, 1 = WHEEL OFFSET, 2 = SENSOR, 3 = DISPLAY, 4 = LADDER, 5 = DONE
bool buttonsArmed = true;   // settings screens: see buttonsGate()

// ---------------- Bluetooth target + selection ----------------
char savedName[64] = "";    // device chosen previously (NVS "btname")
char targetName[64] = "";   // device to connect to this boot

struct FoundDev {
  char name[48];
  int rssi;
};
FoundDev foundList[8];
volatile int foundCount = 0;
volatile bool discoveryStopped = false;
int selIdx = 0;             // entry currently shown in the selection list

// Called by the library for every device its scan finds. In select mode
// we record the device and decline to connect; in connect mode we accept
// only the saved target name.
bool ssidCallback(const char *ssid, esp_bd_addr_t address, int rssi) {
  if (selectMode) {
    if (ssid != nullptr && ssid[0] != '\0' && foundCount < 8) {
      for (int i = 0; i < foundCount; i++) {
        if (strcmp(foundList[i].name, ssid) == 0) return false;  // dupe
      }
      strncpy(foundList[foundCount].name, ssid, sizeof(foundList[0].name) - 1);
      foundList[foundCount].name[sizeof(foundList[0].name) - 1] = '\0';
      foundList[foundCount].rssi = rssi;
      foundCount++;
      Serial.print("Found: ");
      Serial.println(ssid);
    }
    return false;
  }
  return strcmp(ssid, targetName) == 0;
}

void discoveryModeCallback(esp_bt_gap_discovery_state_t state) {
  if (state == ESP_BT_GAP_DISCOVERY_STOPPED) discoveryStopped = true;
}

// Selection list = found devices, then RESCAN, then KEEP OLD (if any).
int selEntryCount() {
  int n = (int)foundCount + 1;              // + RESCAN
  if (savedName[0] != '\0') n++;            // + KEEP OLD
  return n;
}

void activateSelection() {
  int fc = (int)foundCount;
  if (selIdx < fc) {
    prefs.begin("hcaller", false);
    prefs.putString("btname", foundList[selIdx].name);
    prefs.end();
    Serial.print("Chosen: ");
    Serial.println(foundList[selIdx].name);
  } else if (selIdx == fc) {
    prefs.begin("hcaller", false);          // RESCAN
    prefs.putBool("btselect", true);
    prefs.end();
  }
  // KEEP OLD (last entry): nothing to save, savedName is still in NVS.
  ESP.restart();
}

void restartIntoSelection() {
  prefs.begin("hcaller", false);
  prefs.putBool("btselect", true);
  prefs.end();
  ESP.restart();
}

void restartIntoMenu() {
  prefs.begin("hcaller", false);
  prefs.putBool("menu", true);
  prefs.end();
  ESP.restart();
}

// Enter Bluetooth device selection from the settings menu. The A2DP
// stack has not been started yet this boot, so this is still the one
// and only start() call.
void enterBtSelect() {
  settingsState = 0;
  selectMode = true;
  forceRedraw = true;
  buttonsArmed = false;
  a2dp_source.set_auto_reconnect(false);   // selection must discover
  a2dp_source.start();
  Serial.println("Settings: Bluetooth device selection");
}

// ---------------- Audio segment queue ----------------
// The A2DP callback (Bluetooth task) consumes segments; loop() produces
// them. A segment is either a voice clip (PCM) or a generated tone
// (toneHz 0 = silence, used as a spacer). Segments carry the time they
// were queued and are dropped if they wait more than 1.5 s.
struct Segment {
  const int16_t *pcm;  // nullptr means "generate from toneHz"
  uint32_t len;        // length in samples
  float toneHz;
  uint32_t queuedMs;
};
static Segment segQueue[6];
static volatile uint8_t segHead = 0;  // next to play
static volatile uint8_t segTail = 0;  // next free slot
volatile uint32_t lastDataCbMs = 0;   // last time the audio callback ran

bool queueSegment(const int16_t *pcm, uint32_t len, float toneHz) {
  uint8_t next = (segTail + 1) % 6;
  if (next == segHead) return false;  // queue full: drop
  segQueue[segTail].pcm = pcm;
  segQueue[segTail].len = len;
  segQueue[segTail].toneHz = toneHz;
  segQueue[segTail].queuedMs = millis();
  segTail = next;
  return true;
}

// True only while sound is genuinely flowing to the device: connected
// AND the audio callback has run within the last second.
bool audioReady() {
  return a2dp_source.is_connected() && lastDataCbMs != 0 &&
         (millis() - lastDataCbMs) < 1000;
}

int32_t get_sound_data(Frame *data, int32_t frameCount) {
  lastDataCbMs = millis();
  static Segment cur = {nullptr, 0, 0, 0};
  static uint32_t curPos = 0;
  static bool curActive = false;
  static float tonePhase = 0.0;
  for (int32_t i = 0; i < frameCount; i++) {
    int16_t sample = 0;
    if (!curActive) {
      // Pop the next segment, discarding any that waited too long.
      while (segHead != segTail) {
        if (millis() - segQueue[segHead].queuedMs > 1500) {
          segHead = (segHead + 1) % 6;  // stale: drop it
          continue;
        }
        cur = segQueue[segHead];
        segHead = (segHead + 1) % 6;
        curPos = 0;
        tonePhase = 0.0;
        curActive = true;
        break;
      }
    }
    if (curActive) {
      if (cur.pcm != nullptr) {
        sample = cur.pcm[curPos];
      } else {
        float env = 1.0;
        if (curPos < 220) env = curPos / 220.0;                 // 5 ms fade in
        if (cur.len - curPos < 220) env = (cur.len - curPos) / 220.0;  // out
        sample = (int16_t)(16000.0 * env * sin(tonePhase));
        tonePhase += 2.0 * PI * cur.toneHz / 44100.0;
        if (tonePhase >= 2.0 * PI) tonePhase -= 2.0 * PI;
      }
      curPos++;
      if (curPos >= cur.len) curActive = false;
    }
    data[i].channel1 = sample;
    data[i].channel2 = sample;
  }
  return frameCount;
}

bool liveAnnounced = false;  // current height announced since audio came up

void on_connection_state_changed(esp_a2d_connection_state_t state, void *ptr) {
  Serial.print("Bluetooth connection state: ");
  Serial.println(a2dp_source.to_str(state));
  if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
    segHead = segTail;        // flush anything left over
    liveAnnounced = false;
  }
}

// ---------------- Buttons ----------------
int btn35Idle = HIGH;
bool btnAWasPressed = false;
bool btnBWasPressed = false;
unsigned long bothHeldSince = 0;

// Logical buttons. With the display flipped 180 degrees the
// physical buttons trade places, so the helpers swap them: VOL-
// (this helper) is always the button on the user's left, VOL+ the
// right, whichever way the board is mounted. Every screen and the
// buttons gate read buttons through these two functions.
bool btnAPressed() {
  return flipDisplay ? (digitalRead(BTN_B) != btn35Idle)
                     : (digitalRead(BTN_A) == LOW);
}
bool btnBPressed() {
  return flipDisplay ? (digitalRead(BTN_A) == LOW)
                     : (digitalRead(BTN_B) != btn35Idle);
}

// Settings screens arm their buttons only after both have been
// released once, so the gesture that opened the screen (a long hold,
// a choose press on the previous screen) cannot leak into it as a
// phantom press. Set buttonsArmed = false on every screen entry.
bool buttonsGate() {
  static unsigned long gateStartMs = 0;
  bool a = btnAPressed();
  bool b = btnBPressed();
  if (!buttonsArmed) {
    if (gateStartMs == 0) gateStartMs = millis();
    if ((!a && !b) || millis() - gateStartMs > 2500) {
      buttonsArmed = true;
      gateStartMs = 0;
      Serial.println("Settings buttons armed");
    }
    btnAWasPressed = a;
    btnBWasPressed = b;
    return true;   // swallow this poll
  }
  gateStartMs = 0;
  return false;
}

// ---------------- Volume ----------------
const int VOL_STEPS[5] = {40, 60, 80, 100, 127};
int volIdx = 4;  // restored from NVS in setup; 4 = maximum (127)
unsigned long lastVolChangeMs = 0;

void setVolumeIdx(int idx) {
  volIdx = idx;
  prefs.begin("hcaller", false);
  prefs.putUChar("vol", (uint8_t)volIdx);   // survives power cycles
  prefs.end();
  a2dp_source.set_volume(VOL_STEPS[volIdx]);
  lastVolChangeMs = millis();
  Serial.print("Volume: ");
  Serial.println(VOL_STEPS[volIdx]);
  if (audioReady()) {
    queueSegment(nullptr, 4410, 0.0);       // 0.1 s pause for the buds
    queueSegment(nullptr, 6615, 1500.0);    // 0.15 s beep at the new level
  }
}

void pollButtonsNormal() {
  bool a = btnAPressed();
  bool b = btnBPressed();
  // Both buttons held 2 s: open the settings menu.
  if (a && b) {
    if (bothHeldSince == 0) bothHeldSince = millis();
    if (millis() - bothHeldSince >= 2000) restartIntoMenu();
  } else {
    bothHeldSince = 0;
  }
  if (millis() - lastVolChangeMs < 250) { btnAWasPressed = a; btnBWasPressed = b; return; }
  if (a && !btnAWasPressed && volIdx > 0) setVolumeIdx(volIdx - 1);
  if (b && !btnBWasPressed && volIdx < 4) setVolumeIdx(volIdx + 1);
  btnAWasPressed = a;
  btnBWasPressed = b;
}

void pollButtonsSelect() {
  if (buttonsGate()) return;
  bool a = btnAPressed();
  bool b = btnBPressed();
  if (!discoveryStopped) { btnAWasPressed = a; btnBWasPressed = b; return; }
  if ((int)foundCount == 0) {
    if ((a && !btnAWasPressed) || (b && !btnBWasPressed)) restartIntoSelection();
  } else {
    if (a && !btnAWasPressed) selIdx = (selIdx + 1) % selEntryCount();
    if (b && !btnBWasPressed) activateSelection();
  }
  btnAWasPressed = a;
  btnBWasPressed = b;
}

void pollButtonsMenu() {
  if (buttonsGate()) return;
  bool a = btnAPressed();
  bool b = btnBPressed();
  if (a && !btnAWasPressed) {
    menuIdx = (menuIdx + 1) % 6;
    Serial.print("Menu step: ");
    Serial.println(menuIdx);
  }
  if (b && !btnBWasPressed) {
    if (menuIdx == 0) {
      enterBtSelect();
    } else if (menuIdx == 1) {
      // Start the offset list at the current value if it is in the list.
      offsetIdx = 0;
      for (int i = 0; i < OFFSET_COUNT; i++) {
        if (OFFSET_VALUES[i] == wheelOffsetFt) offsetIdx = i;
      }
      settingsState = 2;
      forceRedraw = true;
      buttonsArmed = false;
    } else if (menuIdx == 2) {
      sensorIdx = sensorSel;   // open the list on the current sensor
      settingsState = 3;
      forceRedraw = true;
      buttonsArmed = false;
    } else if (menuIdx == 3) {
      dispIdx = flipDisplay ? 1 : 0;   // open the list on the current choice
      settingsState = 4;
      forceRedraw = true;
      buttonsArmed = false;
    } else if (menuIdx == 4) {
      ladderIdx = ladderSel;   // open the list on the current ladder
      settingsState = 5;
      forceRedraw = true;
      buttonsArmed = false;
    } else {
      // DONE: Bluetooth was never started this boot, so start it now
      // and drop straight into normal operation - no restart needed.
      settingsState = 0;
      forceRedraw = true;
      applySensor();   // the sensor choice may have changed in the menu
      strncpy(targetName, savedName, sizeof(targetName) - 1);
      Serial.print("Settings done, Bluetooth target: ");
      Serial.println(targetName);
      a2dp_source.set_auto_reconnect(true);   // page the saved address
      a2dp_source.start(targetName);
    }
  }
  btnAWasPressed = a;
  btnBWasPressed = b;
}

void pollButtonsOffset() {
  if (buttonsGate()) return;
  bool a = btnAPressed();
  bool b = btnBPressed();
  if (a && !btnAWasPressed) offsetIdx = (offsetIdx + 1) % OFFSET_COUNT;
  if (b && !btnBWasPressed) {
    wheelOffsetFt = OFFSET_VALUES[offsetIdx];
    prefs.begin("hcaller", false);
    prefs.putFloat("offset", wheelOffsetFt);
    prefs.end();
    Serial.print("Wheel offset set: ");
    Serial.println(wheelOffsetFt, 1);
    settingsState = 1;   // back to the settings menu
    forceRedraw = true;
    buttonsArmed = false;
  }
  btnAWasPressed = a;
  btnBWasPressed = b;
}

void pollButtonsSensor() {
  if (buttonsGate()) return;
  bool a = btnAPressed();
  bool b = btnBPressed();
  if (a && !btnAWasPressed) sensorIdx = (sensorIdx + 1) % 2;
  if (b && !btnBWasPressed) {
    sensorSel = sensorIdx;
    prefs.begin("hcaller", false);
    prefs.putUChar("sensor", (uint8_t)sensorSel);
    prefs.end();
    Serial.print("Sensor set: ");
    Serial.println(sensorSel == 1 ? "TFmini Plus" : "ultrasonic");
    settingsState = 1;   // back to the settings menu
    forceRedraw = true;
    buttonsArmed = false;
  }
  btnAWasPressed = a;
  btnBWasPressed = b;
}

void pollButtonsDisplay() {
  if (buttonsGate()) return;
  bool a = btnAPressed();
  bool b = btnBPressed();
  if (a && !btnAWasPressed) dispIdx = (dispIdx + 1) % 2;
  if (b && !btnBWasPressed) {
    flipDisplay = (dispIdx == 1);
    prefs.begin("hcaller", false);
    prefs.putUChar("rot", flipDisplay ? 1 : 0);
    prefs.end();
    Serial.print("Display set: ");
    Serial.println(flipDisplay ? "FLIPPED" : "UPRIGHT");
    // Apply at once: the menu repaints in the new orientation, and
    // the button helpers follow flipDisplay from this moment.
    tft.setRotation(flipDisplay ? 0 : 2);
    settingsState = 1;   // back to the settings menu
    forceRedraw = true;
    buttonsArmed = false;
  }
  btnAWasPressed = a;
  btnBWasPressed = b;
}

void pollButtonsLadder() {
  if (buttonsGate()) return;
  bool a = btnAPressed();
  bool b = btnBPressed();
  if (a && !btnAWasPressed) ladderIdx = (ladderIdx + 1) % 2;
  if (b && !btnBWasPressed) {
    ladderSel = ladderIdx;
    prefs.begin("hcaller", false);
    prefs.putUChar("ladder", (uint8_t)ladderSel);
    prefs.end();
    Serial.print("Ladder set: ");
    Serial.println(ladderSel == 1 ? "FULL RANGE" : "ORIGINAL 20 FT");
    settingsState = 1;   // back to the settings menu
    forceRedraw = true;
    buttonsArmed = false;
  }
  btnAWasPressed = a;
  btnBWasPressed = b;
}

// ---------------- Callout engine ----------------
// announcedBand is the whole foot last announced (-1 = none / out of the
// zone). A new band is accepted only after the height moves 0.2 ft past
// the boundary, so hovering on a boundary stays quiet.
bool inZone = false;
int announcedBand = -1;
char lastCallText[20] = "--";     // for the Serial log, e.g. "10 - VOICE"

void fireCallout(int feet) {
  const int16_t *clip = nullptr;
  uint32_t clipLen = 0;
  switch (feet) {
    case 40: clip = CLIP_FORTY;       clipLen = CLIP_FORTY_LEN;       break;
    case 35: clip = CLIP_THIRTYFIVE;  clipLen = CLIP_THIRTYFIVE_LEN;  break;
    case 30: clip = CLIP_THIRTY;      clipLen = CLIP_THIRTY_LEN;      break;
    case 25: clip = CLIP_TWENTYFIVE;  clipLen = CLIP_TWENTYFIVE_LEN;  break;
    case 20: clip = CLIP_TWENTY;   clipLen = CLIP_TWENTY_LEN;   break;
    case 19: clip = CLIP_NINETEEN; clipLen = CLIP_NINETEEN_LEN; break;
    case 18: clip = CLIP_EIGHTEEN; clipLen = CLIP_EIGHTEEN_LEN; break;
    case 17: clip = CLIP_SEVENTEEN; clipLen = CLIP_SEVENTEEN_LEN; break;
    case 16: clip = CLIP_SIXTEEN;  clipLen = CLIP_SIXTEEN_LEN;  break;
    case 15: clip = CLIP_FIFTEEN;  clipLen = CLIP_FIFTEEN_LEN;  break;
    case 14: clip = CLIP_FOURTEEN; clipLen = CLIP_FOURTEEN_LEN; break;
    case 13: clip = CLIP_THIRTEEN; clipLen = CLIP_THIRTEEN_LEN; break;
    case 12: clip = CLIP_TWELVE;   clipLen = CLIP_TWELVE_LEN;   break;
    case 11: clip = CLIP_ELEVEN;   clipLen = CLIP_ELEVEN_LEN;   break;
    case 10: clip = CLIP_TEN;   clipLen = CLIP_TEN_LEN;   break;
    case 9:  clip = CLIP_NINE;  clipLen = CLIP_NINE_LEN;  break;
    case 8:  clip = CLIP_EIGHT; clipLen = CLIP_EIGHT_LEN; break;
    case 7:  clip = CLIP_SEVEN; clipLen = CLIP_SEVEN_LEN; break;
    case 6:  clip = CLIP_SIX;   clipLen = CLIP_SIX_LEN;   break;
    case 5:  clip = CLIP_FIVE;  clipLen = CLIP_FIVE_LEN;  break;
    case 4:  clip = CLIP_FOUR;  clipLen = CLIP_FOUR_LEN;  break;
    case 3:  clip = CLIP_THREE; clipLen = CLIP_THREE_LEN; break;
    case 2:  clip = CLIP_TWO;   clipLen = CLIP_TWO_LEN;   break;
    case 1:  clip = CLIP_ONE;   clipLen = CLIP_ONE_LEN;   break;
    default: return;
  }
  snprintf(lastCallText, sizeof(lastCallText), "%d - VOICE", feet);
  if (audioReady()) queueSegment(clip, clipLen, 0);
  Serial.print("CALLOUT: ");
  Serial.println(lastCallText);
}

// ---------------- Ladder shape (v20) ----------------
// The ladder is a list of spoken heights. Original: every foot from
// 20 down. Full range: 40, 35, 30, 25, then every foot from 20.
// ladderBand(h) maps a height to the callout whose band it is in;
// a callout is spoken as the height descends through its number
// (the transition rules in loop() carry a small hysteresis).
float zoneEnterFt() { return ladderSel == 1 ? 50.0 : 21.0; }
float zoneExitFt()  { return ladderSel == 1 ? 50.5 : 21.5; }

int ladderBand(float h) {
  if (ladderSel == 1) {
    if (h >= 40) return 40;
    if (h >= 35) return 35;
    if (h >= 30) return 30;
    if (h >= 25) return 25;
    if (h >= 20) return 20;
  }
  return (int)h;
}

bool isCallout(int band) {
  if (band >= 1 && band <= 20) return true;
  return ladderSel == 1 && (band == 25 || band == 30 || band == 35 || band == 40);
}

// ---------------- Callout plausibility gate (v18) ----------------
// speakBand() is the only path from the ladder to fireCallout().
// lastSpokenBand / lastSpokenMs track what the pilot last heard;
// suspectBand / suspectSinceMs track a squelched step under watch
// (the watchdog itself lives in loop(), next to the ladder).
int lastSpokenBand = -1;
unsigned long lastSpokenMs = 0;
int suspectBand = -1;
unsigned long suspectSinceMs = 0;

void speakBand(int band, unsigned long now) {
  if (lastSpokenBand < 0) {
    fireCallout(band);
    lastSpokenBand = band;
    lastSpokenMs = now;
    return;
  }
  int step = abs(band - lastSpokenBand);
  float maxStep = 1.0 + (now - lastSpokenMs) / 1000.0 * 10.0;
  if (step <= maxStep) {
    fireCallout(band);
    lastSpokenBand = band;
    lastSpokenMs = now;
    suspectBand = -1;
  } else {
    suspectBand = band;      // squelched - the watchdog decides
    suspectSinceMs = now;
    Serial.print("Callout squelched: ");
    Serial.print(band);
    Serial.print(" right after ");
    Serial.println(lastSpokenBand);
  }
}

// ---------------- Battery monitor ----------------
// The T-Display reads its battery through a 1:2 divider on GPIO34
// (ADC1, so it works while Bluetooth runs). analogReadMilliVolts
// applies the chip's factory calibration; doubling gives battery
// volts. Percent comes from a LiPo discharge-curve lookup - honest
// as a low-battery gauge, not a precision instrument.
float readBatteryVolts() {
  uint32_t mv = 0;
  for (int i = 0; i < 16; i++) mv += analogReadMilliVolts(BATT_ADC_PIN);
  return (mv / 16.0) * 2.0 / 1000.0;
}

// Percent for a battery voltage; -1 when no battery is fitted
// (the divider node sits near 0 V, e.g. USB bench power).
int batteryPct(float volts) {
  if (volts < 2.5) return -1;
  static const float pts[][2] = {
    {3.30, 0}, {3.50, 10}, {3.60, 20}, {3.70, 35}, {3.80, 50},
    {3.90, 65}, {4.00, 80}, {4.10, 90}, {4.20, 100}
  };
  if (volts <= pts[0][0]) return 0;
  for (int i = 1; i < 9; i++) {
    if (volts <= pts[i][0]) {
      float f = (volts - pts[i - 1][0]) / (pts[i][0] - pts[i - 1][0]);
      return (int)(pts[i - 1][1] + f * (pts[i][1] - pts[i - 1][1]) + 0.5);
    }
  }
  return 100;
}

// ---------------- Sensor + median filter ----------------
float readUsCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long us = pulseIn(ECHO_PIN, HIGH, 60000UL);
  if (us == 0) return -1.0;          // no echo within timeout
  return us / 58.0;
}

// TFmini Plus: 9-byte UART frames (0x59 0x59, distance cm little-endian,
// strength, temperature, checksum). Drain the serial buffer through
// the frame parser; the freshest plausible reading wins.
void drainLidar() {
  while (Serial2.available()) {
    uint8_t b = (uint8_t)Serial2.read();
    if (lidarIdx == 0) {
      if (b == 0x59) lidarBuf[lidarIdx++] = b;
    } else if (lidarIdx == 1) {
      if (b == 0x59) lidarBuf[lidarIdx++] = b;
      else lidarIdx = 0;
    } else {
      lidarBuf[lidarIdx++] = b;
      if (lidarIdx == 9) {
        lidarIdx = 0;
        uint8_t sum = 0;
        for (int i = 0; i < 8; i++) sum += lidarBuf[i];
        if (sum == lidarBuf[8]) {
          int dist = lidarBuf[2] | (lidarBuf[3] << 8);
          int strength = lidarBuf[4] | (lidarBuf[5] << 8);
          if (dist > 0 && dist < 1550 && strength >= 30) {
            lastLidarCm = dist;
            lastLidarMs = millis();
          }
        }
      }
    }
  }
}

float readLidarCm() {
  drainLidar();
  if (millis() - lastLidarMs < 300) return lastLidarCm;
  return -1;
}

float readCm() {
  return sensorSel == 1 ? readLidarCm() : readUsCm();
}

// Point the shared sensor port (GPIO 25/26 + 5 V) at the selected
// sensor: UART for the LiDAR, GPIO pulse timing for the ultrasonic.
void applySensor() {
  if (sensorSel == 1) {
    lidarIdx = 0;
    lastLidarMs = 0;
    Serial2.begin(115200, SERIAL_8N1, LIDAR_RX_PIN, LIDAR_TX_PIN);
  } else {
    Serial2.end();
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
  }
}

// Last 3 valid readings (cm). Invalid readings never enter the window;
// the window is cleared whenever the caller leaves the zone.
float winCm[3] = {0, 0, 0};
int winCount = 0;

void winPush(float cm) {
  if (winCount < 3) {
    winCm[winCount++] = cm;
  } else {
    winCm[0] = winCm[1];
    winCm[1] = winCm[2];
    winCm[2] = cm;
  }
}

float winMedian() {
  if (winCount == 1) return winCm[0];
  if (winCount == 2) return (winCm[0] + winCm[1]) / 2.0;
  float a = winCm[0], b = winCm[1], c = winCm[2];
  if (a > b) { float t = a; a = b; b = t; }
  if (b > c) { float t = b; b = c; c = t; }
  if (a > b) { float t = a; a = b; b = t; }
  return b;
}

float lastGoodFt = -1.0;      // filtered wheel height for the screen
unsigned long lastValidMs = 0;

// ---------------- Screens ----------------
// All screens are change-driven: a draw function returns immediately
// when nothing it shows has changed, so the panel holds a steady image
// instead of strobing through erase-and-repaint cycles.
void printWrapped(const char *text, int x, int y, int maxChars, int lines) {
  int len = strlen(text);
  for (int line = 0; line < lines && line * maxChars < len; line++) {
    char buf[32];
    int n = len - line * maxChars;
    if (n > maxChars) n = maxChars;
    strncpy(buf, text + line * maxChars, n);
    buf[n] = '\0';
    tft.setCursor(x, y + line * 12);
    tft.print(buf);
  }
}

int lastSelCount = -1;
bool lastSelStopped = false;
int lastSelIdxShown = -1;

void drawSelectScreen() {
  int fc = (int)foundCount;
  if (!forceRedraw && discoveryStopped == lastSelStopped &&
      fc == lastSelCount && selIdx == lastSelIdxShown) return;
  lastSelStopped = discoveryStopped;
  lastSelCount = fc;
  lastSelIdxShown = selIdx;
  forceRedraw = false;

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(8, 10);
  tft.print("BT DEVICE SELECT");
  if (!discoveryStopped) {
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(8, 50);
    tft.print("Scanning...");
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(8, 70);
    tft.print("Found: ");
    tft.print(fc);
    tft.setCursor(8, 110);
    tft.print("Put the earbuds /");
    tft.setCursor(8, 122);
    tft.print("headset in pairing");
    tft.setCursor(8, 134);
    tft.print("mode now.");
    return;
  }
  if (fc == 0) {
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(8, 50);
    tft.print("No devices found");
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(8, 80);
    tft.print("Press any button");
    tft.setCursor(8, 92);
    tft.print("to scan again.");
    return;
  }
  tft.setCursor(8, 34);
  tft.print("Entry ");
  tft.print(selIdx + 1);
  tft.print(" of ");
  tft.print(selEntryCount());
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  if (selIdx < fc) {
    printWrapped(foundList[selIdx].name, 8, 60, 10, 3);
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(8, 140);
    tft.print("Signal: ");
    tft.print(foundList[selIdx].rssi);
  } else if (selIdx == fc) {
    tft.setCursor(8, 60);
    tft.print("RESCAN");
  } else {
    tft.setCursor(8, 60);
    tft.print("KEEP OLD");
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_WHITE);
    printWrapped(savedName, 8, 90, 21, 2);
  }
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(8, 190);
  tft.print("VOL- : next");
  tft.setCursor(8, 202);
  tft.print("VOL+ : choose");
}

int lastMenuIdx = -1;
float lastMenuOff = -1.0;
int lastMenuSensor = -1;
int lastMenuFlip = -1;
int lastMenuLadder = -1;

void drawMenuScreen() {
  if (!forceRedraw && menuIdx == lastMenuIdx &&
      wheelOffsetFt == lastMenuOff && sensorSel == lastMenuSensor &&
      (int)flipDisplay == lastMenuFlip && ladderSel == lastMenuLadder) return;
  lastMenuIdx = menuIdx;
  lastMenuOff = wheelOffsetFt;
  lastMenuSensor = sensorSel;
  lastMenuFlip = flipDisplay;
  lastMenuLadder = ladderSel;
  forceRedraw = false;

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(8, 10);
  tft.print("SETTINGS");
  // Entry rows; WHEEL OFFSET is a deliberate two-line entry (the label
  // does not fit on one line at this size - v7 wrapped it by accident).
  struct { const char *l1; const char *l2; int y; } items[6] = {
    {"BT DEVICE", nullptr, 26},
    {"WHEEL", "OFFSET", 46},
    {"SENSOR", nullptr, 86},
    {"DISPLAY", nullptr, 106},
    {"LADDER", nullptr, 126},
    {"DONE", nullptr, 146},
  };
  for (int i = 0; i < 6; i++) {
    tft.setTextSize(2);
    if (i == menuIdx) {
      tft.setTextColor(ST77XX_GREEN);
      tft.setCursor(2, items[i].y);
      tft.print(">");
    } else {
      tft.setTextColor(ST77XX_WHITE);
    }
    tft.setCursor(20, items[i].y);
    tft.print(items[i].l1);
    if (items[i].l2 != nullptr) {
      tft.setCursor(20, items[i].y + 18);
      tft.print(items[i].l2);
    }
  }
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(8, 170);
  tft.print("BT: ");
  printWrapped(savedName, 32, 170, 17, 2);
  tft.setCursor(8, 188);
  tft.print("Offset now: ");
  tft.print(wheelOffsetFt, 1);
  tft.print(" ft");
  tft.setCursor(8, 197);
  tft.print("Sensor: ");
  tft.print(sensorSel == 1 ? "TFmini Plus" : "Ultrasonic");
  tft.setCursor(8, 206);
  tft.print("Display: ");
  tft.print(flipDisplay ? "Flipped" : "Upright");
  tft.setCursor(8, 215);
  tft.print("Ladder: ");
  tft.print(ladderSel == 1 ? "Full range" : "Original 20 ft");
  tft.setCursor(8, 226);
  tft.print("VOL- next  VOL+ choose");
}

int lastSensorIdx = -1;

void drawSensorScreen() {
  if (!forceRedraw && sensorIdx == lastSensorIdx) return;
  lastSensorIdx = sensorIdx;
  forceRedraw = false;

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(8, 10);
  tft.print("SENSOR");
  tft.setCursor(8, 30);
  tft.print("Which sensor is wired up:");
  const char *names[2] = {"ULTRASONIC", "TFMINI"};
  for (int i = 0; i < 2; i++) {
    tft.setTextSize(2);
    if (i == sensorIdx) {
      tft.setTextColor(ST77XX_GREEN);
      tft.setCursor(2, 62 + i * 36);
      tft.print(">");
    } else {
      tft.setTextColor(ST77XX_WHITE);
    }
    tft.setCursor(20, 62 + i * 36);
    tft.print(names[i]);
  }
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(8, 150);
  tft.print("TFMINI = TFmini Plus LiDAR.");
  tft.setCursor(8, 162);
  tft.print("Both use pins 25/26 + 5V.");
  tft.setCursor(8, 190);
  tft.print("VOL- : next");
  tft.setCursor(8, 202);
  tft.print("VOL+ : set it");
}

int lastDispIdx = -1;

void drawDisplayScreen() {
  if (!forceRedraw && dispIdx == lastDispIdx) return;
  lastDispIdx = dispIdx;
  forceRedraw = false;

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(8, 10);
  tft.print("DISPLAY");
  tft.setCursor(8, 30);
  tft.print("Screen orientation:");
  const char *names[2] = {"UPRIGHT", "FLIPPED"};
  for (int i = 0; i < 2; i++) {
    tft.setTextSize(2);
    if (i == dispIdx) {
      tft.setTextColor(ST77XX_GREEN);
      tft.setCursor(2, 62 + i * 36);
      tft.print(">");
    } else {
      tft.setTextColor(ST77XX_WHITE);
    }
    tft.setCursor(20, 62 + i * 36);
    tft.print(names[i]);
  }
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(8, 150);
  tft.print("FLIPPED turns the screen");
  tft.setCursor(8, 162);
  tft.print("180 degrees. The buttons");
  tft.setCursor(8, 174);
  tft.print("swap sides to match.");
  tft.setCursor(8, 196);
  tft.print("VOL- : next");
  tft.setCursor(8, 208);
  tft.print("VOL+ : set it");
}

int lastLadderIdx = -1;

void drawLadderScreen() {
  if (!forceRedraw && ladderIdx == lastLadderIdx) return;
  lastLadderIdx = ladderIdx;
  forceRedraw = false;

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(8, 10);
  tft.print("LADDER");
  tft.setCursor(8, 30);
  tft.print("Callout ladder:");
  const char *names[2] = {"20 FT", "FULL"};
  for (int i = 0; i < 2; i++) {
    tft.setTextSize(2);
    if (i == ladderIdx) {
      tft.setTextColor(ST77XX_GREEN);
      tft.setCursor(2, 62 + i * 36);
      tft.print(">");
    } else {
      tft.setTextColor(ST77XX_WHITE);
    }
    tft.setCursor(20, 62 + i * 36);
    tft.print(names[i]);
  }
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(8, 150);
  tft.print("20 FT = the original:");
  tft.setCursor(8, 162);
  tft.print("every foot from 20 down.");
  tft.setCursor(8, 178);
  tft.print("FULL = 40/35/30/25,");
  tft.setCursor(8, 190);
  tft.print("then every foot from 20.");
  tft.setCursor(8, 206);
  tft.print("VOL- : next");
  tft.setCursor(8, 218);
  tft.print("VOL+ : set it");
}

int lastOffIdx = -1;

void drawOffsetScreen() {
  if (!forceRedraw && offsetIdx == lastOffIdx) return;
  lastOffIdx = offsetIdx;
  forceRedraw = false;

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(8, 10);
  tft.print("WHEEL OFFSET");
  tft.setCursor(8, 30);
  tft.print("Probe height on gear:");
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(5);
  tft.setCursor(10, 70);
  tft.print(OFFSET_VALUES[offsetIdx], 1);
  tft.setTextSize(2);
  tft.setCursor(105, 95);
  tft.print("ft");
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(8, 150);
  tft.print("0.0 - 4.0 ft, 0.5 steps");
  tft.setCursor(8, 190);
  tft.print("VOL- : next value");
  tft.setCursor(8, 202);
  tft.print("VOL+ : set it");
}

// Normal screen caches: what each field currently shows.
char lastHeightShown[12] = "";
uint16_t lastHeightColor = 0;
int lastBtShown = -1;
int lastVolShown = -1;
float lastOffShown = -1.0;
int lastBatShown = -2;
bool lastAudioShown = false;

// Speaker icon (body + cone + two sound chevrons), drawn ~1.5x
// larger than v16 in a 35 x 20 box at x, y (Mark's request, v17).
void drawSpeakerIcon(int x, int y, uint16_t color) {
  tft.fillRect(x, y + 7, 6, 7, color);
  tft.fillTriangle(x + 6, y + 2, x + 6, y + 19, x + 18, y + 10, color);
  tft.drawLine(x + 22, y + 5, x + 26, y + 10, color);
  tft.drawLine(x + 26, y + 10, x + 22, y + 16, color);
  tft.drawLine(x + 28, y + 2, x + 34, y + 10, color);
  tft.drawLine(x + 34, y + 10, x + 28, y + 19, color);
}

void drawScreen() {
  unsigned long now = millis();
  bool noReturn = (lastGoodFt < 0 || now - lastValidMs >= 3000);
  bool fresh = (lastGoodFt > 0 && now - lastValidMs < 1000);
  char hbuf[12];
  uint16_t hcol;
  if (noReturn) {
    strcpy(hbuf, "----");
    hcol = ST77XX_RED;
  } else {
    snprintf(hbuf, sizeof(hbuf), "%.1f", lastGoodFt);
    hcol = fresh ? ST77XX_GREEN : ST77XX_YELLOW;
  }

  if (forceRedraw) {
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(1);
    tft.setCursor(20, 10);
    tft.print("HEIGHT - FEET");
    // Reset the field caches so every field paints below.
    lastHeightShown[0] = '\0';
    lastHeightColor = 0;
    lastBtShown = -1;
    lastVolShown = -1;
    lastOffShown = -1.0;
    lastBatShown = -2;
    forceRedraw = false;
  }

  // Height digits (+ NO RETURN caption) share one patch.
  if (strcmp(hbuf, lastHeightShown) != 0 || hcol != lastHeightColor) {
    tft.fillRect(0, 55, 135, 80, ST77XX_BLACK);
    tft.setTextColor(hcol);
    tft.setTextSize(5);
    tft.setCursor(4, 60);
    tft.print(hbuf);
    if (noReturn) {
      tft.setTextColor(ST77XX_RED);
      tft.setTextSize(1);
      tft.setCursor(32, 120);
      tft.print("NO RETURN");
    }
    strcpy(lastHeightShown, hbuf);
    lastHeightColor = hcol;
  }

  int bt = a2dp_source.is_connected() ? 1 : 0;
  if (bt != lastBtShown) {
    tft.fillRect(0, 144, 94, 12, ST77XX_BLACK);
    tft.setTextColor(bt ? ST77XX_GREEN : ST77XX_YELLOW);
    tft.setTextSize(1);
    tft.setCursor(8, 148);
    tft.print(bt ? "BT: CONNECTED" : "BT: SEARCHING");
    lastBtShown = bt;
    // Restore the icon in step if this repaint touched its patch.
    lastAudioShown = audioReady();
    if (lastAudioShown) drawSpeakerIcon(97, 140, ST77XX_GREEN);
  }

  // Speaker icon at the right of the BT line: present only while
  // audio is actually flowing, so a new user can tell "connected"
  // from "callouts are live".
  bool ready = audioReady();
  if (ready != lastAudioShown) {
    lastAudioShown = ready;
    tft.fillRect(95, 138, 40, 26, ST77XX_BLACK);
    if (ready) drawSpeakerIcon(97, 140, ST77XX_GREEN);
  }

  // Status lines at text size 2 (v17, Mark's request). The LAST
  // CALL and zone lines that used to live below these are gone.
  if (volIdx != lastVolShown) {
    tft.fillRect(0, 164, 135, 20, ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.setCursor(4, 166);
    tft.print("VOL: ");
    tft.print(VOL_STEPS[volIdx]);
    lastVolShown = volIdx;
  }

  if (wheelOffsetFt != lastOffShown) {
    tft.fillRect(0, 186, 135, 20, ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.setCursor(4, 188);
    tft.print("OFF: ");
    tft.print(wheelOffsetFt, 1);
    lastOffShown = wheelOffsetFt;
  }

  // Battery line: sampled every 2 s, repainted only on change.
  // States: -1 = no battery ("BAT: ---"), 0..100 = percent on
  // battery, 101 = on USB ("USB POWER"). USB is inferred from the
  // voltage (v21): the charger holds the node at ~4.2 V, so the
  // latch takes USB at >= 4.18 V and releases below 4.12 V.
  static unsigned long lastBattMs = 0;
  static bool onUsb = false;
  if (now - lastBattMs >= 2000) {
    lastBattMs = now;
    float volts = readBatteryVolts();
    if (!onUsb && volts >= 4.18) onUsb = true;
    else if (onUsb && volts < 4.12) onUsb = false;
    int state = volts < 2.5 ? -1 : (onUsb ? 101 : batteryPct(volts));
    if (state != lastBatShown) {
      lastBatShown = state;
      tft.fillRect(0, 208, 135, 20, ST77XX_BLACK);
      tft.setTextSize(2);
      tft.setCursor(4, 210);
      if (state < 0) {
        tft.setTextColor(ST77XX_WHITE);
        tft.print("BAT: ---");
      } else if (state == 101) {
        tft.setTextColor(ST77XX_GREEN);
        tft.print("USB POWER");
      } else {
        tft.setTextColor(state >= 50 ? ST77XX_GREEN
                         : (state >= 25 ? ST77XX_YELLOW : ST77XX_RED));
        tft.print("BAT: ");
        tft.print(state);
        tft.print("%");
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  // T-Display screen wiring: data on GPIO 19, remap SPI before tft.init.
  SPI.begin(18, -1, 19, 5);   // SCK 18, no MISO, MOSI 19, SS 5
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BATT_EN_PIN, OUTPUT);
  digitalWrite(BATT_EN_PIN, HIGH);   // power the battery divider
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT);
  // Both buttons are active-low with board pull-ups, so the resting
  // level is HIGH (btn35Idle's initial value). It is NOT learned at
  // boot: the both-hold restart can leave a button held during the
  // learn window, which taught "pressed" as resting and inverted the
  // button for the whole boot (the v9 deaf-menu bug).
  tft.init(135, 240);
  tft.setRotation(2);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(8, 100);

  // Load saved settings and decide this boot's mode.
  prefs.begin("hcaller", true);
  String saved = prefs.getString("btname", "");
  bool forceSelect = prefs.getBool("btselect", false);
  bool wantMenu = prefs.getBool("menu", false);
  wheelOffsetFt = prefs.getFloat("offset", 0.0);
  sensorSel = prefs.getUChar("sensor", 0);
  volIdx = prefs.getUChar("vol", 4);
  if (volIdx > 4) volIdx = 4;
  flipDisplay = prefs.getUChar("rot", 0) != 0;
  ladderSel = prefs.getUChar("ladder", 0);
  if (ladderSel > 1) ladderSel = 0;
  prefs.end();
  if (flipDisplay) tft.setRotation(0);   // before anything is printed
  strncpy(savedName, saved.c_str(), sizeof(savedName) - 1);
  if (forceSelect || wantMenu) {
    prefs.begin("hcaller", false);
    prefs.putBool("btselect", false);   // consume the requests
    prefs.putBool("menu", false);
    prefs.end();
  }
  selectMode = forceSelect || savedName[0] == '\0';
  if (!selectMode && wantMenu) settingsState = 1;
  applySensor();

  a2dp_source.set_on_connection_state_changed(on_connection_state_changed);
  a2dp_source.set_data_callback_in_frames(get_sound_data);
  a2dp_source.set_volume(VOL_STEPS[volIdx]);
  a2dp_source.set_ssid_callback(ssidCallback);
  a2dp_source.set_discovery_mode_callback(discoveryModeCallback);

  if (selectMode) {
    buttonsArmed = false;
    tft.print("BT SETUP...");
    Serial.println("Height caller v16: device selection mode");
    a2dp_source.set_auto_reconnect(false);   // selection must discover
    a2dp_source.start();   // discovery only; ssidCallback records devices
  } else if (settingsState == 1) {
    buttonsArmed = false;
    tft.print("SETTINGS...");
    Serial.println("Height caller v16: settings menu");
    // Bluetooth is NOT started in the menu; it starts only if the user
    // picks BT DEVICE (enterBtSelect) - still one start() per boot.
  } else {
    strncpy(targetName, savedName, sizeof(targetName) - 1);
    tft.print("STARTING BT...");
    Serial.print("Height caller v16, Bluetooth target: ");
    Serial.print(targetName);
    Serial.print(", wheel offset ");
    Serial.print(wheelOffsetFt, 1);
    Serial.print(" ft, sensor ");
    Serial.print(sensorSel == 1 ? "TFmini Plus" : "ultrasonic");
    Serial.print(", ladder ");
    Serial.println(ladderSel == 1 ? "full range" : "original 20 ft");
    a2dp_source.set_auto_reconnect(true);   // page the saved address
    a2dp_source.start(targetName);
  }
  Serial.println("Setup done");
}

void loop() {
  // After polling, draw a settings screen ONLY if the UI is still on
  // it - a button press may have changed state (DONE, choosing an
  // entry), and the incoming screen must get the forceRedraw.
  if (selectMode) {
    pollButtonsSelect();
    if (selectMode) drawSelectScreen();
    delay(80);
    return;
  }
  if (settingsState == 1) {
    pollButtonsMenu();
    if (settingsState == 1 && !selectMode) drawMenuScreen();
    delay(80);
    return;
  }
  if (settingsState == 2) {
    pollButtonsOffset();
    if (settingsState == 2) drawOffsetScreen();
    delay(80);
    return;
  }
  if (settingsState == 3) {
    pollButtonsSensor();
    if (settingsState == 3) drawSensorScreen();
    delay(80);
    return;
  }
  if (settingsState == 4) {
    pollButtonsDisplay();
    if (settingsState == 4) drawDisplayScreen();
    delay(80);
    return;
  }
  if (settingsState == 5) {
    pollButtonsLadder();
    if (settingsState == 5) drawLadderScreen();
    delay(80);
    return;
  }
  pollButtonsNormal();
  float d = readCm();
  unsigned long now = millis();
  // Validity cap is sensor-aware (v17): the ultrasonic's no-echo
  // default sits at ~604 cm, so its readings cap at 600; the TFmini
  // cap is 1550 cm (v20 - past its 12 m rating; the strength gate
  // still decides which frames count).
  bool valid = (d > 0 && d < (sensorSel == 1 ? 1550 : 600));
  if (valid) {
    lastValidMs = now;
    winPush(d);
    float h = winMedian() / 30.48 - wheelOffsetFt;
    lastGoodFt = h;
    if (h > zoneExitFt()) {
      if (inZone) { inZone = false; announcedBand = -1; winCount = 0; lastSpokenBand = -1; suspectBand = -1; }
    } else if (h <= zoneEnterFt()) {
      int band = ladderBand(h);       // the callout whose band h is in
      if (!inZone) {
        inZone = true;
        if (h - band > 1.0) {
          // Entered well above the band's number (full range arms
          // at 50 but first calls 40): stay silent now and let the
          // descent rule speak the band as its number is crossed.
          announcedBand = band + (band >= 20 ? 5 : 1);
        } else {
          announcedBand = band;
          if (isCallout(band)) speakBand(band, now);
        }
      } else if (band < announcedBand && h <= band + 0.8) {
        announcedBand = band;         // descended to a new callout
        if (isCallout(band)) speakBand(band, now);
      } else if (band > announcedBand && h >= band + 0.2) {
        announcedBand = band;         // floated back up to a callout
        if (isCallout(band)) speakBand(band, now);
      }
    }
    // enterFt < h <= exitFt is a dead band so the zone edge does not flap.
    // Squelch watchdog (v18): a step speakBand() suppressed is
    // watched here. If the readings hold at that level for 750 ms
    // it was real after all - speak the current band once and
    // re-sync. If they return near the last spoken band, forget
    // the episode silently; if they settle somewhere else
    // implausible, watch that level instead.
    if (suspectBand >= 0) {
      if (announcedBand == suspectBand) {
        if (now - suspectSinceMs >= 750) {
          fireCallout(suspectBand);
          lastSpokenBand = suspectBand;
          lastSpokenMs = now;
          suspectBand = -1;
        }
      } else if (abs(announcedBand - lastSpokenBand) <= 1) {
        suspectBand = -1;
      } else {
        suspectBand = announcedBand;
        suspectSinceMs = now;
      }
    }
  } else {
    if (inZone && now - lastValidMs > 3000) {
      inZone = false;
      announcedBand = -1;
      winCount = 0;
      lastSpokenBand = -1;
      suspectBand = -1;
    }
  }
  // The moment audio is genuinely live, announce the current height
  // once - the pilot should not have to wait for the next foot to know
  // the caller is with him.
  if (!liveAnnounced && audioReady()) {
    if (inZone && isCallout(announcedBand) &&
        lastGoodFt - announcedBand >= 0 && lastGoodFt - announcedBand <= 1.0) {
      fireCallout(announcedBand);
      lastSpokenBand = announcedBand;
      lastSpokenMs = now;
      suspectBand = -1;
    }
    liveAnnounced = true;
  }
  drawScreen();
  delay(50);
}
