// r48_can.cpp — R48-3000e3 CAN supervisory link over the ESP32-S3 TWAI peripheral.
//
// FRAME FORMAT IS NOT IMPLEMENTED. The R48 protocol is undocumented by Vertiv and
// was reverse-engineered from the Emerson M820B shelf controller. Port the
// arbitration IDs, scaling, and byte order from:
//   https://github.com/anikrooz/Emerson-Vertiv-R48
//   https://github.com/PurpleAlien/R48_Rectifier
// Those use the mcp_can API; only the transport differs — the frame definitions
// are the part worth copying. Do not guess them: the rectifier retains the last
// set current across power cycles, so a malformed write leaves a setpoint you
// will not see until it bites.
//
// Bring-up order: LISTEN FIRST. Most R48 units broadcast status unprompted, so
// traffic should appear without transmitting anything. That alone confirms the
// JST is CAN, the wiring is right, and the bit rate is correct.
#include "r48_can.h"
#include "config.h"
#include <driver/twai.h>

static bool s_installed = false;

bool R48::begin(gpio_num_t txPin, gpio_num_t rxPin, uint16_t bitrateKbps) {
  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(txPin, rxPin, TWAI_MODE_NORMAL);
  g.rx_queue_len = 16;

  // Only the rates worth supporting here. Verify against the rectifier by
  // measuring the narrowest bit time on the scope — 1/t_bit — rather than
  // trusting either library.
  twai_timing_config_t t;
  switch (bitrateKbps) {
    case 125: t = TWAI_TIMING_CONFIG_125KBITS(); break;
    case 250: t = TWAI_TIMING_CONFIG_250KBITS(); break;
    case 500: t = TWAI_TIMING_CONFIG_500KBITS(); break;
    default:  return false;
  }

  twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g, &t, &f) != ESP_OK) return false;
  if (twai_start() != ESP_OK) { twai_driver_uninstall(); return false; }

  s_installed = true;
  _up = true;
  return true;
}

bool R48::setVoltage(float volts) {
  if (!_up) return false;
  if (volts < 41.5f || volts > 58.5f) return false;   // hard reject, no clamping
  if (volts > BUS_VOLTAGE_CEIL) return false;         // IRFP260N-derived ceiling
  // TODO(port): build and transmit the set-voltage frame via twai_transmit().
  return false;                                        // false until implemented
}

bool R48::setCurrentLimit(float amps) {
  if (!_up) return false;
  if (amps < 1.0f || amps > 50.0f) return false;
  // TODO(port): build and transmit the set-current frame.
  return false;
}

void R48::poll(Telem& out) {
  if (!_up) { out.online = false; return; }

  twai_message_t msg;
  while (twai_receive(&msg, 0) == ESP_OK) {
    // TODO(port): decode status frames into out.vOut / out.iOut.
    out.lastRxMs = millis();
  }

  // Stale telemetry is a fault condition, not silent degradation: CR-11 makes
  // this the independent channel for the plausibility check (CR-03), so losing
  // it means losing the cross-check that catches a miscalibrated shunt.
  out.online = (out.lastRxMs != 0) && (millis() - out.lastRxMs < 3000);
}
