// r48_can.h — supervisory CAN link to the Vertiv/Emerson R48-3000e3 (CR-11).
//
// Scope: set bus voltage, set a hardware current limit, read back the
// rectifier's own V/I as an INDEPENDENT cross-check against the INA228.
//
// Explicitly NOT in scope: enabling the output. CR-05 requires a hard voltage
// STEP into the ZVS stage, which only the contactor can deliver. Commanding the
// rectifier on over CAN produces a ramp — the exact condition that drives both
// MOSFETs into conduction. Nothing here may be wired to the enable path.
#pragma once
#include <Arduino.h>
#include <driver/gpio.h>

class R48 {
public:
  struct Telem {
    float    vOut = NAN;      // rectifier-reported output voltage (0x11 status frame)
    float    iOut = NAN;      // rectifier-reported output current
    // Decoded 2026-09-07 from the 0x41 info frames, sub-ID in data[3], payload a
    // big-endian IEEE-754 float in data[4..7]. Identified by commanding known
    // values and watching two of them come back: 0x01 mirrored our 41.5 V and
    // 0x03 our 0.982 current fraction, which validates the whole set.
    float    vSet   = NAN;    // 0x01 — commanded output voltage, read back
    float    iLimit = NAN;    // 0x03 — current limit as a fraction of rated
    float    tempC  = NAN;    // 0x04 — internal temperature, °C
    float    acInV  = NAN;    // 0x05 — AC input voltage
    uint32_t lastRxMs = 0;
    bool     online = false;
  };

  // ESP32-S3 built-in TWAI peripheral — no external CAN controller.
  bool begin(gpio_num_t txPin, gpio_num_t rxPin, uint16_t bitrateKbps);

  // Voltage 41.5–58.5 V, 0.1 V steps. Values outside range are rejected.
  bool setVoltage(float volts);

  // Current 1–50 A, 0.1 A steps. The rectifier appears to ignore settings
  // below ~5 A. This limit holds even if the ESP32 hangs — it is a protection
  // layer BELOW the firmware, which is the point.
  bool setCurrentLimit(float amps);

  // Non-blocking; drains the RX queue and decodes status frames. Call often.
  void poll(Telem& out);

  // Drive the session: handshake until the rectifier answers, then keep-alive
  // every CAN_KEEPALIVE_MS. MUST be called from the main loop unconditionally —
  // the rectifier drops its setpoints after ~37 s of silence (§6.14, measured),
  // which is the safety net, but it also means a stalled loop loses the bus.
  void service();

  // Fan mode, cmd 0x33 (requirements 6.15): true = force full, false = auto.
  // Never sent unless asked -- the keep-alive control byte leaves fanFull clear.
  bool setFan(bool full);

  // The rectifier streams 0x41 info frames on 0x060F8007 with a sub-ID in
  // data[3]. Sub-IDs 0x5A-0x5C are the model string; the rest are undecoded.
  // This keeps the last payload seen for each so they can be compared across
  // operating points -- one of them is plausibly temperature, which would
  // explain fan behaviour without guessing.
  // Callback rather than printing directly: the console's say() layer is static
  // to main.cpp, and reaching across translation units for it is a link error.
  void forEachInfo(void (*cb)(uint8_t sub, const uint8_t d[4], uint32_t ageMs));

private:
  enum Mode { LISTEN, INIT, RUN };
  void _send(uint32_t id, const uint8_t d[8]);
  bool _up = false;
  Mode _mode = LISTEN;
  uint32_t _nextTx = 0, _lastRx = 0;
  float _vHold = NAN, _iHold = NAN;   // held ONLINE setpoints, re-sent by service()
  uint8_t  _info[256][4] = {};        // last payload per 0x41 sub-ID
  uint32_t _infoSeen[256] = {};       // millis of last sighting, 0 = never
};
