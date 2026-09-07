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
    float    vOut = NAN;      // rectifier-reported output voltage
    float    iOut = NAN;      // rectifier-reported output current
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

private:
  enum Mode { LISTEN, INIT, RUN };
  void _send(uint32_t id, const uint8_t d[8]);
  bool _up = false;
  Mode _mode = LISTEN;
  uint32_t _nextTx = 0, _lastRx = 0;
  float _vHold = NAN, _iHold = NAN;   // held ONLINE setpoints, re-sent by service()
};
