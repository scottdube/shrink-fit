// r48_can.cpp — Vertiv R48-3000e3 CAN supervisory link over the ESP32 TWAI
// peripheral. Set bus voltage and current limit ONLINE; read back the rectifier's
// own V/I as an INDEPENDENT cross-check against the INA228 (CR-11).
//
// NOT in scope: enabling the output. CR-05 needs a hard voltage STEP into the ZVS,
// which only the SSR delivers. Commanding the rectifier on over CAN is a ramp —
// the exact condition that drives both MOSFETs into conduction. Nothing here
// touches the enable path.
//
// PROTOCOL VERIFIED ON THE RECTIFIER 2026-09-07 (requirements §6.14.0a–e).
// Ported from ina228_test.cpp, which is the code that actually held a session,
// decoded telemetry against a meter, and commanded voltage and current under load.
//   - IDs are EXTENDED (29-bit).
//   - Session is a conversation: handshake (two syncs + gimme5) every 10 s until
//     the rectifier answers on 0x060F8003 (data[3] 0x58 first contact, 0x5D
//     recovery), then keep-alive (control + settings query) every 6 s.
//   - Telemetry: any frame with data[0]==0x11. Vout = (d[1]<<8|d[2])/1024 (whole
//     volts), Iout = (d[6]<<8|d[7])/512. Both verified against a clamp meter.
//   - Set voltage cmd 0x21, set current cmd 0x22, value = IEEE-754 big-endian
//     float. Current is a FRACTION of R48_I_BASE_A.
//   - ONLINE only. OFFLINE (0x24/0x19) is deliberately absent: it persists in the
//     rectifier across power cycles.
#include "r48_can.h"
#include "config.h"
#include <driver/twai.h>
#include <string.h>
#include <math.h>

// Extended arbitration IDs.
static const uint32_t ID_SET      = 0x0607FF83;  // set voltage / current
static const uint32_t ID_SETTINGS = 0x0647FF83;  // settings query (keep-alive)
static const uint32_t ID_CONTROL  = 0x06080783;  // control byte / gimme5
static const uint32_t ID_SYNC1    = 0x0707FF83;
static const uint32_t ID_SYNC2    = 0x0717FF83;
static const uint32_t ID_HELLO    = 0x060F8003;  // handshake reply, data[3]==0x58/0x5D
static const uint32_t ID_STATUS   = 0x060F8007;  // telemetry / info frames

static void encFloatBE(float v, uint8_t *out) {
  uint32_t u; memcpy(&u, &v, 4);
  out[0] = u >> 24; out[1] = u >> 16; out[2] = u >> 8; out[3] = u;
}

bool R48::begin(gpio_num_t txPin, gpio_num_t rxPin, uint16_t bitrateKbps) {
  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(txPin, rxPin, TWAI_MODE_NORMAL);
  g.rx_queue_len = 32;

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

  _up   = true;
  _mode = INIT;                 // start the conversation immediately
  _nextTx = 0; _lastRx = 0;
  return true;
}

void R48::_send(uint32_t id, const uint8_t d[8]) {
  twai_message_t m = {};
  m.extd = 1; m.identifier = id; m.data_length_code = 8;
  memcpy(m.data, d, 8);
  twai_transmit(&m, pdMS_TO_TICKS(20));
}

// The clamp is HERE and is not a parameter. 48.0 V is the IRFP260N ceiling.
bool R48::setVoltage(float volts) {
  if (!_up) return false;
  if (volts > BUS_VOLTAGE_CEIL) volts = BUS_VOLTAGE_CEIL;   // clamp, don't reject —
  if (volts < BUS_VOLTAGE_MIN)  volts = BUS_VOLTAGE_MIN;    // the caller wants an output
  _vHold = volts;                                           // service() re-sends it
  uint8_t d[8] = {0x03, 0xF0, 0x00, 0x21, 0, 0, 0, 0};
  encFloatBE(volts, &d[4]);
  _send(ID_SET, d);
  return true;
}

// Current as a fraction of R48_I_BASE_A (~56 A). Clamped to the rectifier's
// 0.10–1.21 range (§6.15). This is the rectifier's own CC loop.
bool R48::setCurrentLimit(float amps) {
  if (!_up) return false;
  float frac = amps / R48_I_BASE_A;
  if (frac > 1.21f) frac = 1.21f;
  if (frac < 0.10f) frac = 0.10f;
  _iHold = frac;
  uint8_t d[8] = {0x03, 0xF0, 0x00, 0x22, 0, 0, 0, 0};
  encFloatBE(frac, &d[4]);
  _send(ID_SET, d);
  return true;
}

bool R48::setFan(bool full) {
  if (!_up) return false;
  uint8_t d[8] = {0x03, 0xF0, 0x00, 0x33, 0, 0, 0, 0};
  if (full) d[5] = 0x01;                 // 00 01 00 00 = full, 00 00 00 00 = auto
  _send(ID_SET, d);
  return true;
}

void R48::forEachInfo(void (*cb)(uint8_t sub, const uint8_t d[4], uint32_t ageMs)) {
  uint32_t now = millis();
  for (int i = 0; i < 256; i++)
    if (_infoSeen[i]) cb((uint8_t)i, _info[i], now - _infoSeen[i]);
}

void R48::service() {
  if (!_up || _mode == LISTEN) return;
  uint32_t now = millis();

  // If the rectifier has gone quiet long enough that it will want the handshake
  // again, drop back to INIT so we re-establish before commanding anything.
  if (_mode == RUN && _lastRx && now - _lastRx > 25000) { _mode = INIT; _nextTx = 0; }

  if (now < _nextTx) return;

  if (_mode == INIT) {
    static const uint8_t sync1[8]  = {0x04, 0xF0, 0x01, 0x5A, 0, 0, 0, 0};
    static const uint8_t sync2[8]  = {0x04, 0xF0, 0x5A, 0x00, 0, 0, 0, 0};
    static const uint8_t gimme5[8] = {0x20, 0xF0, 0x00, 0x80, 0, 0, 0, 0};
    _send(ID_SYNC1, sync1);
    _send(ID_SYNC2, sync2);
    _send(ID_CONTROL, gimme5);
    _nextTx = now + 10000;
  } else {  // RUN — keep-alive, then refresh any held setpoints
    static const uint8_t ctrl[8]     = {0x00, 0xF0, 0x01, 0x80, 0, 0, 0, 0};
    static const uint8_t settings[8] = {0x03, 0xF0, 0x00, 0x08, 0x3F, 0x94, 0x62, 0x9B};
    _send(ID_CONTROL, ctrl);
    _send(ID_SETTINGS, settings);
    // The setpoint survives on session traffic alone (measured), so re-sending is
    // belt-and-braces — cheap insurance against a firmware version that ever loses
    // the keep-alive path.
    if (!isnan(_vHold)) { uint8_t d[8]={0x03,0xF0,0x00,0x21,0,0,0,0}; encFloatBE(_vHold,&d[4]); _send(ID_SET,d); }
    if (!isnan(_iHold)) { uint8_t d[8]={0x03,0xF0,0x00,0x22,0,0,0,0}; encFloatBE(_iHold,&d[4]); _send(ID_SET,d); }
    _nextTx = now + CAN_KEEPALIVE_MS;
  }
}

void R48::poll(Telem& out) {
  if (!_up) { out.online = false; return; }

  twai_message_t m;
  while (twai_receive(&m, 0) == ESP_OK) {
    if (!m.extd) continue;
    _lastRx = out.lastRxMs = millis();

    if (m.identifier == ID_HELLO && (m.data[3] == 0x58 || m.data[3] == 0x5D)) {
      if (_mode == INIT) { _mode = RUN; _nextTx = 0; }        // handshake acknowledged
    } else if (m.data[3] == 0x23 && _mode != LISTEN) {
      _mode = INIT; _nextTx = 0;                               // rectifier asked to re-sync
    }
    if (m.data[0] == 0x41 && m.data_length_code == 8) {        // info frame
      uint8_t sub = m.data[3];
      memcpy(_info[sub], &m.data[4], 4);
      _infoSeen[sub] = millis();
      uint32_t raw = ((uint32_t)m.data[4] << 24) | ((uint32_t)m.data[5] << 16) |
                     ((uint32_t)m.data[6] << 8)  |  m.data[7];
      float f; memcpy(&f, &raw, 4);
      switch (sub) {
        case 0x01: out.vSet   = f; break;
        case 0x03: out.iLimit = f; break;
        case 0x04: out.tempC  = f; break;
        case 0x05: out.acInV  = f; break;
      }
    }
    if (m.data[0] == 0x11 && m.data_length_code == 8) {        // status frame
      out.vOut = (float)(((uint16_t)m.data[1] << 8) | m.data[2]) / 1024.0f;
      out.iOut = (float)(((uint16_t)m.data[6] << 8) | m.data[7]) / 512.0f;
    }
  }

  // Stale telemetry is a fault, not silent degradation: CR-11 makes this the
  // independent channel for the plausibility cross-check (CR-03).
  out.online = (out.lastRxMs != 0) && (millis() - out.lastRxMs < CAN_SESSION_LOST_MS);
}
