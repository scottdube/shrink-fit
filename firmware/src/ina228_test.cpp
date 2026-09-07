// INA228 bring-up on a 38-pin ESP32 devkit.
//
// DELIBERATELY A SEPARATE BOARD from the test rig. The rig works -- OTA, TWAI,
// buttons, duration ladder -- and must not be disturbed to try a new peripheral.
// This also starts the ESP32 port incrementally instead of as one migration
// later, and tests I2C on the silicon that will actually carry it (see
// requirements.md 6.12).
//
// Raw register access on purpose: no library between you and the chip during
// bring-up, so a wiring fault reads as a wiring fault rather than as a library
// that "doesn't work".
//
// WIRING (ESP32 default I2C):
//   INA228 VCC -> 3V3        SDA -> GPIO21
//   INA228 GND -> GND        SCL -> GPIO22
//
// Nothing else. No current through the board for this test -- the point is only
// to prove the chip is alive BEFORE R002 is desoldered. Desolder first and a
// dead board is ambiguous: arrived dead, or killed in the rework?

#include <Arduino.h>
#include <string.h>
#include <Wire.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <driver/twai.h>

// Hostname deliberately DIFFERENT from the test rig's "shrinkfit-testrig" --
// both are on the same network and an mDNS collision would send a flash to the
// wrong board. No OTA password, consistent with the rig (Scott, 2026-08-27).
static const char *OTA_HOST = "shrinkfit-ina228";
static Preferences prefs;
static bool otaReady = false;
static void otaBegin();   // defined below, after the register helpers
static bool     streaming = false;
static uint32_t nextRead  = 0;

// --- telnet console ----------------------------------------------------------
// OTA uploads firmware; it does not carry the console. This does. It also works
// where espota does not: telnet is an ordinary OUTBOUND connection from the
// laptop to the board, so it crosses the VLAN that espota's reverse connection
// cannot (see TRAPS). That means the stream is readable from the house network
// without switching WiFi.
static WiFiServer telnetSrv(23);
static WiFiClient telnetCli;

// Everything printed goes to BOTH the USB console and the telnet client, so
// there is one output path and no "which window is this in?" confusion.
static void say(const char *s) {
  Serial.print(s);
  if (telnetCli && telnetCli.connected()) telnetCli.print(s);
}
static void sayfln()                 { say("\n"); }
static void sayfln(const char *s)    { say(s); say("\n"); }
static void sayfln(const String &s)  { say(s.c_str()); say("\n"); }
static void sayf(const char *fmt, ...) {
  char buf[256];
  va_list ap; va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  say(buf);
}

// --- CAN (Vertiv R48-3000e3) --------------------------------------------------
// Protocol: requirements.md 6.14 (IDs, handshake, keep-alive, telemetry decode,
// from anikrooz/Emerson-Vertiv-R48) and 6.15 (set-value encoding = IEEE-754
// big-endian float, verified against six examples). The Huawei R4850G2 protocol
// that was here before is WITHDRAWN (6.13) -- different company, different part.
//
// This is a CONVERSATION, not a poll. The controller must open with a handshake
// and then keep talking every ~6 s; after 20 s of silence the rectifier drops
// its output, after 25 s it wants the handshake again. That is a free interlock
// for CR-01 and a hard rule for the CAN task: it can never stall mid-cycle.
//
// STAGE 1 (this build): READ ONLY. Two modes --
//   LISTEN  (boot default) transmits NOTHING. Prints every frame with its ID.
//           Zero-risk first contact: does the R48 put anything on the bus?
//   ACTIVE  ('r48init') runs the handshake + keep-alive and decodes telemetry.
// No set commands are compiled in. Stage 2 adds them behind R48_STAGE2 with the
// 48.0 V clamp (IRFP260N ceiling) -- see setVoltageOnline() below.
static const int  CAN_TX = 14;                 // pinmap.md: P14 = pin 31
static const int  CAN_RX = 32;                 // pinmap.md: P32 = pin 26 -- NOT GPIO12

// IDs -- all EXTENDED (29-bit). 6.14.
static const uint32_t R48_ID_SET      = 0x0607FF83;  // set voltage / current   (Stage 2)
static const uint32_t R48_ID_SETTINGS = 0x0647FF83;  // settings query (keep-alive)
static const uint32_t R48_ID_CONTROL  = 0x06080783;  // control byte / 'gimme5'
static const uint32_t R48_ID_SYNC1    = 0x0707FF83;  // handshake 1
static const uint32_t R48_ID_SYNC2    = 0x0717FF83;  // handshake 2
// The source recorded these as 0x00860F80xx -- that is the MCP2515 library's
// extended-frame flag (bit 31) left in the value. A 29-bit ID cannot exceed
// 0x1FFFFFFF, so as written they could never match. Measured on the wire
// 2026-09-05: the rectifier answers on 0x060F8003 and 0x060F8007. Our TX IDs
// never carried the flag, which is why the R48 heard us before we heard it.
static const uint32_t R48_ID_HELLO    = 0x060F8003;  // handshake reply, data[3]==0x58/0x5D
static const uint32_t R48_ID_STATUS   = 0x060F8007;  // telemetry / info frames
static const uint32_t R48_ID_SYNCACK  = 0x0707F803;  // reply to our 0x0707FF83 sync

enum R48Mode { R48_LISTEN, R48_INIT, R48_RUN };
static R48Mode  r48Mode = R48_LISTEN;
static uint32_t r48NextTx = 0, r48LastRx = 0, r48Frames = 0, r48Hello = 0;
static bool     r48Got5B = false;

static bool     canUp = false, canRaw = false;
static uint32_t canNextReq = 0, canRx = 0, canTx = 0, canRxErr = 0;

// "ping" -- bench bisection against a second ESP32 running can_loop.cpp as
// LISTENER. THIS board stays the node under test: it is the one whose
// transceiver, wiring and TWAI config have never been proved against a
// known-good peer, and tx_err=128 with rx_err=0 is equally consistent with our
// transceiver not driving as with the rectifier being absent. Testing two fresh
// boards instead would exonerate hardware that was never accused.
static const uint32_t PING_ID = 0x123, ECHO_ID = 0x124;
static bool     pingMode = false, pingWait = false;
static uint32_t pingSeq = 0, pingSent = 0, pingEcho = 0, pingLost = 0, pingTxFail = 0;
static uint32_t pingWaitSeq = 0, pingTxAt = 0;
static float    r48V = NAN, r48I = NAN;      // Vout / Iout from the 0x11 status frame

static const int PIN_SDA = 21;
static const int PIN_SCL = 22;

// A0/A1 both grounded = 0x40. The board's solder pads select 16 addresses, so
// the scan below is the authority, not this constant.
static uint8_t inaAddr = 0x40;

// --- INA228 register map (only what bring-up needs) --------------------------
static const uint8_t REG_CONFIG      = 0x00;
static const uint8_t REG_ADC_CONFIG  = 0x01;
static const uint8_t REG_SHUNT_CAL   = 0x02;
static const uint8_t REG_VSHUNT      = 0x04;   // 24-bit signed, bits 23:4
static const uint8_t REG_VBUS        = 0x05;   // 24-bit unsigned, bits 23:4
static const uint8_t REG_DIETEMP     = 0x06;   // 16-bit signed
static const uint8_t REG_MFG_ID      = 0x3E;   // expect 0x5449 = "TI"
static const uint8_t REG_DEVICE_ID   = 0x3F;   // expect 0x228x

static bool readReg(uint8_t reg, uint8_t *buf, uint8_t n) {
  Wire.beginTransmission(inaAddr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)inaAddr, (int)n) != n) return false;
  for (uint8_t i = 0; i < n; i++) buf[i] = Wire.read();
  return true;
}

static bool read16(uint8_t reg, uint16_t *out) {
  uint8_t b[2];
  if (!readReg(reg, b, 2)) return false;
  *out = ((uint16_t)b[0] << 8) | b[1];
  return true;
}

static bool read24(uint8_t reg, int32_t *out, bool isSigned) {
  uint8_t b[3];
  if (!readReg(reg, b, 3)) return false;
  int32_t v = ((int32_t)b[0] << 16) | ((int32_t)b[1] << 8) | b[2];
  v >>= 4;                                   // low nibble is not data
  if (isSigned && (v & 0x00080000)) v |= 0xFFF00000;   // sign-extend from 20 bits
  *out = v;
  return true;
}

// --- I2C scan ----------------------------------------------------------------
static void scan() {
  sayfln("\nI2C scan on SDA=21 SCL=22 ...");
  uint8_t found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      sayf("  0x%02X  responds", a);
      if (a >= 0x40 && a <= 0x4F) say("   <-- INA228 address range");
      sayfln();
      found++;
      if (a >= 0x40 && a <= 0x4F) inaAddr = a;
    }
  }
  if (!found) {
    sayfln("  NOTHING RESPONDED.");
    sayfln("  Check: 3V3 and GND actually connected? SDA/SCL not swapped?");
    sayfln("  This board has its own pull-ups, so external ones are not needed.");
  } else {
    sayf("  %u device(s). Using 0x%02X.\n", found, inaAddr);
  }
}

// --- identity ----------------------------------------------------------------
// The check that matters: a chip that answers its address might still be
// anything. Manufacturer 0x5449 and device 0x228x say it is genuinely an INA228.
static void identify() {
  uint16_t mfg = 0, dev = 0;
  bool okM = read16(REG_MFG_ID, &mfg);
  bool okD = read16(REG_DEVICE_ID, &dev);
  sayf("\nMFG_ID    0x%04X  %s\n", mfg,
                (okM && mfg == 0x5449) ? "OK  (TI)" : "UNEXPECTED — expect 0x5449");
  sayf("DEVICE_ID 0x%04X  %s\n", dev,
                (okD && (dev & 0xFFF0) == 0x2280) ? "OK  (INA228)"
                                                  : "UNEXPECTED — expect 0x228x");
}

static void dump() {
  uint16_t cfg = 0, adc = 0, cal = 0, temp = 0;
  int32_t  vsh = 0, vbus = 0;
  read16(REG_CONFIG, &cfg);
  read16(REG_ADC_CONFIG, &adc);
  read16(REG_SHUNT_CAL, &cal);
  read16(REG_DIETEMP, (uint16_t *)&temp);
  read24(REG_VSHUNT, &vsh, true);
  read24(REG_VBUS,  &vbus, false);

  // ADCRANGE lives in CONFIG bit 4. 0 = +/-163.84 mV (312.5 nV/LSB),
  // 1 = +/-40.96 mV (78.125 nV/LSB). Bring-up leaves it at 0 so nothing clips
  // before the shunt's real value is known.
  bool  adcRange = cfg & (1 << 4);
  float shuntUV  = vsh * (adcRange ? 0.078125f : 0.3125f);   // microvolts
  float busV     = vbus * 195.3125e-6f;
  float dieC     = (int16_t)temp * 7.8125e-3f;

  sayf("CONFIG 0x%04X  ADC_CONFIG 0x%04X  SHUNT_CAL 0x%04X  ADCRANGE %d\n",
                cfg, adc, cal, adcRange ? 1 : 0);
  sayf("VSHUNT %+8ld raw = %+10.3f uV     VBUS %8ld raw = %7.4f V     DIE %.1f C\n",
                (long)vsh, shuntUV, (long)vbus, busV, dieC);
}

static void canBegin() {
  // NORMAL mode, not listen-only: the request has to be transmitted, and a
  // normal node ACKs frames, which some R48 firmware wants to see before it
  // keeps talking. The request is a read -- all zeros -- and cannot change
  // anything on the rectifier.
  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX, (gpio_num_t)CAN_RX, TWAI_MODE_NORMAL);
  g.rx_queue_len = 32;
  twai_timing_config_t t = TWAI_TIMING_CONFIG_125KBITS();
  twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  if (twai_driver_install(&g, &t, &f) != ESP_OK) { sayfln("CAN: driver install FAILED"); return; }
  if (twai_start() != ESP_OK) { sayfln("CAN: start FAILED"); twai_driver_uninstall(); return; }
  canUp = true;
  sayf("CAN: up, 125 kbps, TX=GPIO%d RX=GPIO%d. R48 mode: %s.\n", CAN_TX, CAN_RX,
       r48Mode == R48_LISTEN ? "LISTEN (transmitting nothing; 'r48init' to talk)"
                             : "ACTIVE");
}

// Tear down and rebuild the driver. Needed after any wiring change: once the
// controller has gone error-passive (tx_err >= 128) it stops taking the bus,
// and the counters only decay through successful frames -- which a bus that
// was just fixed cannot deliver until the controller transmits again. A reboot
// clears it too, but that loses the telnet session and the evidence.
static void canReset() {
  ledcDetachPin(CAN_TX);                  // in case 'toggle' left PWM on the pin
  if (canUp) { twai_stop(); twai_driver_uninstall(); canUp = false; }
  canTx = canRx = canRxErr = 0;
  pingSent = pingEcho = pingLost = pingSeq = pingTxFail = 0; pingWait = false;
  r48V = r48I = NAN; r48Frames = r48Hello = 0; r48Got5B = false; r48NextTx = 0;
  canBegin();
}

static void r48Send(uint32_t id, const uint8_t d[8]) {
  twai_message_t m = {};
  m.extd = 1; m.identifier = id; m.data_length_code = 8;
  memcpy(m.data, d, 8);
  if (twai_transmit(&m, pdMS_TO_TICKS(20)) == ESP_OK) canTx++;
  if (canRaw) { sayf("  TX 0x%08lX :", (unsigned long)id); for (int i=0;i<8;i++) sayf(" %02X", d[i]); sayfln(); }
}

// Handshake: both syncs, then 'gimme5'. Repeated every 10 s until the R48
// answers on R48_ID_HELLO. 6.14 / source sendSync, sendSync2, gimme5.
static void r48Handshake() {
  static const uint8_t sync1[8]  = {0x04,0xF0,0x01,0x5A,0,0,0,0};
  static const uint8_t sync2[8]  = {0x04,0xF0,0x5A,0x00,0,0,0,0};
  static const uint8_t gimme5[8] = {0x20,0xF0,0x00,0x80,0,0,0,0};
  r48Send(R48_ID_SYNC1, sync1);
  r48Send(R48_ID_SYNC2, sync2);
  r48Send(R48_ID_CONTROL, gimme5);
}

// Keep-alive, every 6 s once running: the control byte and the settings query.
// Control bits: dcOff<<7 | fanFull<<4 | flashLed<<3 | acOff<<2 | 1.
// 0x01 = DC on, fan auto, AC on. Nothing here changes a setpoint.
static void r48KeepAlive() {
  static const uint8_t ctrl[8]     = {0x00,0xF0,0x01,0x80,0,0,0,0};
  static const uint8_t settings[8] = {0x03,0xF0,0x00,0x08,0x3F,0x94,0x62,0x9B};
  r48Send(R48_ID_CONTROL, ctrl);
  r48Send(R48_ID_SETTINGS, settings);
}

static void r48Tick() {
  if (r48Mode == R48_LISTEN) return;                 // transmit nothing
  uint32_t now = millis();
  // Silence watchdog -- the rectifier's, mirrored here so we notice first.
  if (r48Mode == R48_RUN && r48LastRx && now - r48LastRx > 25000) {
    sayfln("R48: 25 s silence -- rectifier will want the handshake again. Re-INIT.");
    r48Mode = R48_INIT; r48NextTx = 0;
  } else if (r48Mode == R48_RUN && r48LastRx && now - r48LastRx > 20000) {
    static uint32_t warned = 0;
    if (now - warned > 5000) { warned = now; sayfln("R48: 20 s silence -- output would have dropped."); }
  }
  if (now < r48NextTx) return;
  if (r48Mode == R48_INIT) { r48Handshake(); r48NextTx = now + 10000; }
  else                     { r48KeepAlive(); r48NextTx = now + 6000;  }
}

// IEEE-754 big-endian float -> 4 bytes. 6.15: 52.0 -> 42 50 00 00, verified.
static void encFloatBE(float v, uint8_t *out) {
  uint32_t u; memcpy(&u, &v, 4);
  out[0] = u >> 24; out[1] = u >> 16; out[2] = u >> 8; out[3] = u;
}

#ifdef R48_STAGE2
// ONLINE voltage (cmd 0x21): expires after 30 s of silence, so a mistake cannot
// persist. Clamped to 48.0 V -- the IRFP260N ceiling, 6.14/6.15 -- BEFORE
// encoding, and the clamp is not a parameter. OFFLINE (0x24) is deliberately
// not implemented: it persists in the rectifier across power cycles.
static void setVoltageOnline(float volts) {
  const float R48_VMAX = 48.0f, R48_VMIN = 41.0f;
  if (volts > R48_VMAX) volts = R48_VMAX;
  if (volts < R48_VMIN) volts = R48_VMIN;
  uint8_t d[8] = {0x03,0xF0,0x00,0x21,0,0,0,0};
  encFloatBE(volts, &d[4]);
  r48Send(R48_ID_SET, d);
  sayf("R48: ONLINE voltage -> %.1f V (clamped to %.1f max)\n", volts, R48_VMAX);
}
// ONLINE current limit (cmd 0x22) as a FRACTION of rated. 6.15: the datasheet's
// 62.5 A is the 1.21 value, so the 100 %% base is ~51.7 A. 30 A = 0.58.
static void setCurrentOnline(float amps) {
  const float R48_I_BASE = 51.7f;
  float frac = amps / R48_I_BASE;
  if (frac > 1.21f) frac = 1.21f;
  if (frac < 0.10f) frac = 0.10f;
  uint8_t d[8] = {0x03,0xF0,0x00,0x22,0,0,0,0};
  encFloatBE(frac, &d[4]);
  r48Send(R48_ID_SET, d);
  sayf("R48: ONLINE current -> %.1f A = %.3f of rated\n", frac * R48_I_BASE, frac);
}
#endif

static uint32_t be32(const uint8_t *d) {
  return ((uint32_t)d[0] << 24) | ((uint32_t)d[1] << 16) | ((uint32_t)d[2] << 8) | d[3];
}

static void canPing() {
  twai_message_t m = {};
  m.extd = 0; m.identifier = PING_ID; m.data_length_code = 4;
  m.data[0] = pingSeq >> 24; m.data[1] = pingSeq >> 16;
  m.data[2] = pingSeq >> 8;  m.data[3] = pingSeq;

  // Retire the previous frame only once a NEW one actually goes out. Measured
  // on the bench: when nothing ACKs, frame N retries forever and every later
  // transmit returns queue-full -- and reporting the loss up front reprinted
  // "seq=5 NO ECHO" once a second, which reads as five separate losses instead
  // of one stuck frame. A jam is a different fault from a lost frame and has to
  // count as one.
  if (twai_transmit(&m, pdMS_TO_TICKS(20)) == ESP_OK) {
    if (pingWait) {
      pingLost++;
      sayf("ping seq=%lu  NO ECHO\n", (unsigned long)pingWaitSeq);
    }
    canTx++; pingSent++; pingWait = true; pingWaitSeq = pingSeq; pingTxAt = micros();
    pingSeq++;
  } else if (++pingTxFail == 4) {
    sayfln("ping: TX queue full -- frame still retrying because nothing ACKed it."
           " Further failures counted silently; see 'can'.");
  }
}

static void canService() {
  if (!canUp) return;
  if (pingMode) {
    if (millis() > canNextReq) { canNextReq = millis() + 1000; canPing(); }
  } else r48Tick();

  twai_message_t m;
  while (twai_receive(&m, 0) == ESP_OK) {
    canRx++;
    bool known = false;
    if (!m.extd && m.identifier == ECHO_ID && m.data_length_code == 4) {
      uint32_t got = be32(m.data), rtt = micros() - pingTxAt;
      known = true;
      if (pingWait && got == pingWaitSeq) { pingEcho++; pingWait = false; }
      twai_status_info_t st; twai_get_status_info(&st);
      sayf("ping seq=%lu  ECHO OK  rtt %lu us  tx_err=%lu rx_err=%lu bus_err=%lu\n",
           (unsigned long)got, (unsigned long)rtt, (unsigned long)st.tx_error_counter,
           (unsigned long)st.rx_error_counter, (unsigned long)st.bus_error_count);
    }
    else if (m.extd && !pingMode) {
      // Anything extended that is not our ping protocol is treated as the R48
      // talking. Every frame counts as life for the silence watchdog.
      r48LastRx = millis(); r48Frames++;
      if (m.identifier == R48_ID_HELLO && (m.data[3] == 0x58 || m.data[3] == 0x5D)) {
        // Handshake acknowledged. Source checks 0x58 on first contact and 0x5D
        // on recovery; accept either and say which, the raw dump shows the truth.
        r48Hello++;
        if (r48Mode == R48_INIT) { r48Mode = R48_RUN; r48NextTx = 0; }
        sayf("R48: HELLO (data[3]=0x%02X) -- handshake acknowledged, keep-alive every 6 s.\n", m.data[3]);
        known = true;
      }
      else if (m.data[3] == 0x23 && r48Mode != R48_LISTEN) {
        // Source: rectifier asks for the syncs again.
        r48Handshake(); known = true;
        sayfln("R48: 0x23 -- resent both syncs.");
      }
      else if (m.data[3] == 0x5B) { r48Got5B = true; }
      if (m.data[0] == 0x41 && m.data_length_code == 8 && m.identifier == R48_ID_STATUS) {
        // Info frames, sub-ID in data[3]. 0x5A..0x5C carry the model string as
        // ASCII, four chars each, seen on the wire as "081R" "4830" "00e3" --
        // i.e. "R48-3000e3" in three pieces. Others are unknown and printed raw.
        uint8_t sub = m.data[3];
        if (sub >= 0x5A && sub <= 0x5C) {
          static char model[13] = {0};
          memcpy(&model[(sub - 0x5A) * 4], &m.data[4], 4);
          if (sub == 0x5C) { model[12] = 0; sayf("R48 model string: \"%s\"\n", model); }
          known = true;
        }
      }
      if (m.data[0] == 0x11 && m.data_length_code == 8) {
        // Status frame. Decode per 6.14 -- note /1022 not /1024, and the x2 on
        // current, both from the source and BOTH to be confirmed against the
        // meter on first contact. Printed raw alongside until they are.
        // VERIFIED 2026-09-05 against the meter under a real load (ZVS cycle):
        //   Vout: raw is always a multiple of 1024 -> whole volts. raw/1024.
        //         53 V unloaded (meter 53.4), 20-21 V in current limit.
        //   Iout: the source's 'x2' was missing the same /1024. raw/512 gives
        //         67.4 A at raw 0x86BA against 68 A on the DC clamp (<1 %).
        //         Telemetry lags ~2-3 s on the way down -- a filter.
        r48V = (float)(((uint16_t)m.data[1] << 8) | m.data[2]) / 1024.0f;
        r48I = (float)(((uint16_t)m.data[6] << 8) | m.data[7]) / 512.0f;
        known = !canRaw;
        sayf("R48 status: Vout %.0f V  Iout %.1f A   [raw %02X %02X | %02X %02X]\n",
             r48V, r48I, m.data[1], m.data[2], m.data[6], m.data[7]);
      }
    }
    if (canRaw || !known) {
      sayf("CAN %s 0x%08lX dlc=%d :", m.extd ? "EXT" : "STD", (unsigned long)m.identifier, m.data_length_code);
      for (int i = 0; i < m.data_length_code; i++) sayf(" %02X", m.data[i]);
      sayfln();
    }
  }
  twai_status_info_t st;
  if (twai_get_status_info(&st) == ESP_OK) canRxErr = st.rx_error_counter;
}

// Self-test: prove THIS node's whole path with no peer involved. NO_ACK mode
// drops the acknowledgement requirement, and the self-reception flag makes the
// controller keep a copy of what it sent -- but the frame still has to go out
// through CTX, be driven onto the real bus by the transceiver, come back through
// its receiver and re-enter on CRX. If it returns, every link in the local chain
// works and the fault is the peer or the wire between. If it does not, the fault
// is here, and no amount of testing against another board will show that.
static void canSelfTest() {
  sayfln("self-test: NO_ACK mode, no peer needed ...");
  if (canUp) { twai_stop(); twai_driver_uninstall(); canUp = false; }

  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX,
                                                        (gpio_num_t)CAN_RX,
                                                        TWAI_MODE_NO_ACK);
  g.rx_queue_len = 8;
  twai_timing_config_t t = TWAI_TIMING_CONFIG_125KBITS();
  twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  if (twai_driver_install(&g, &t, &f) != ESP_OK) { sayfln("  install FAILED"); canBegin(); return; }
  if (twai_start() != ESP_OK) { sayfln("  start FAILED"); twai_driver_uninstall(); canBegin(); return; }

  twai_message_t m = {};
  m.identifier = 0x555; m.data_length_code = 2;
  m.data[0] = 0xA5; m.data[1] = 0x5A;
  m.self = 1;
  bool queued = (twai_transmit(&m, pdMS_TO_TICKS(200)) == ESP_OK);
  sayf("  queued: %s\n", queued ? "yes" : "NO -- controller will not accept the frame");

  bool got = false;
  uint32_t t0 = millis();
  while (millis() - t0 < 600 && !got) {
    twai_message_t r;
    if (twai_receive(&r, pdMS_TO_TICKS(50)) == ESP_OK && r.identifier == 0x555) got = true;
  }
  twai_status_info_t st; twai_get_status_info(&st);
  sayf("  own frame back: %s   tx_err=%lu bus_err=%lu\n", got ? "YES" : "no",
       (unsigned long)st.tx_error_counter, (unsigned long)st.bus_error_count);
  if (got)
    sayfln("  [PASS] CTX -> driver -> bus -> receiver -> CRX all work on THIS board.");
  else
    sayfln("  [FAIL] our own frame never came back. The break is on THIS board, not "
           "the peer -- driver not driving, or CTX/CRX not doing what the meter says.");

  twai_stop(); twai_driver_uninstall(); canUp = false;
  canBegin();
}

static void canStatus() {
  if (!canUp) { sayfln("CAN: not up"); return; }
  twai_status_info_t st; twai_get_status_info(&st);
  const char *state = st.state == TWAI_STATE_RUNNING ? "RUNNING" :
                      st.state == TWAI_STATE_BUS_OFF ? "BUS-OFF" :
                      st.state == TWAI_STATE_RECOVERING ? "RECOVERING" : "STOPPED";
  sayf("CAN %s  tx=%lu rx=%lu  tx_err=%lu rx_err=%lu  bus_err=%lu\n", state,
       (unsigned long)canTx, (unsigned long)canRx, (unsigned long)st.tx_error_counter,
       (unsigned long)st.rx_error_counter, (unsigned long)st.bus_error_count);
  sayf("R48  mode %s  frames %lu  hello %lu  Vout %.2f V  Iout %.1f A  last rx %lu s ago\n",
       r48Mode == R48_LISTEN ? "LISTEN" : r48Mode == R48_INIT ? "INIT" : "RUN",
       (unsigned long)r48Frames, (unsigned long)r48Hello, r48V, r48I,
       r48LastRx ? (unsigned long)((millis() - r48LastRx) / 1000) : 0UL);
  if (st.state == TWAI_STATE_BUS_OFF)
    sayfln("  BUS-OFF: tx errors exceeded 255. Termination: Waveshare has 120R onboard; add NOTHING.");
  if (pingMode) {
    sayf("PING mode: %lu sent, %lu echoed, %lu lost, %lu tx-queue-full\n",
         (unsigned long)pingSent, (unsigned long)pingEcho, (unsigned long)pingLost,
         (unsigned long)pingTxFail);
    if (pingEcho > 0 && pingLost == 0 && st.bus_error_count == 0)
      sayfln("  [PASS] round trips clean -- THIS board's transceiver, wiring and TWAI");
    else if (pingEcho > 0)
      sayfln("  [MARGINAL] echoes but with losses -- check 121R termination at both ends.");
    else if (pingSent > 3 && st.tx_error_counter >= 128)
      sayfln("  [FAIL] our transceiver drives, nothing ACKs. CANH/CANL not across "
             "between the boards, or swapped. (Role jumpers are gone -- the peer's "
             "role is fixed at build time.)");
    else if (pingSent > 3)
      sayfln("  [FAIL] no echoes. Is the second board powered and running can_loop?");
    else if (pingTxFail > 3)
      sayfln("  [FAIL] cannot even queue a frame -- frame 1 is still retrying because"
             " nothing ACKed it. Bus wired to a dead/absent peer. Check H/L and GND.");
    return;
  }
  if (canRx == 0 && canTx > 3)
    sayfln("  transmitting but hearing nothing: R48 on mains? CANH/CANL swapped? GND to DC-?");
}

void setup() {
  Serial.begin(115200);
  delay(400);
  sayfln("\n=== INA228 bring-up — 38-pin ESP32 ===");
  sayf("chip %s  rev %d  %d MHz\n",
                ESP.getChipModel(), ESP.getChipRevision(), getCpuFrequencyMhz());
  prefs.begin("ina228", false);
  Wire.begin(PIN_SDA, PIN_SCL, 100000);   // 100 kHz: bring-up, not throughput
  scan();
  identify();
  otaBegin();
  canBegin();
  sayfln("\ncommands:  scan | id | r | stream | stop | can | canraw | canreset | r48 | r48init | r48stop | ping | selftest | dominant | recessive | toggle");
  sayfln("wifi:      ssid <name> | pass <password> | wifi? | wifi clear");
  sayfln("NOTE: with R002 still fitted, VIN+ and VIN- are shorted through");
  sayfln("2 milliohms -- a finger is ~1 megohm, so NOTHING external can move");
  sayfln("VSHUNT. The touch test only works AFTER R002 is removed, when the");
  sayfln("inputs float. MFG_ID + DEVICE_ID above are what prove the chip.");
}

// Credentials live in NVS and are typed at the serial console -- they never
// appear in the repo or in a build. Set once with: wifi <ssid> <password>
static void otaBegin() {
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  if (!ssid.length()) { sayfln("no wifi credentials — 'wifi <ssid> <pass>'"); return; }

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(OTA_HOST);
  WiFi.begin(ssid.c_str(), pass.c_str());
  sayf("wifi: joining %s ", ssid.c_str());
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) { delay(300); say("."); }
  if (WiFi.status() != WL_CONNECTED) { sayfln(" FAILED"); return; }

  // Power save costs 40-140 ms of latency and made espota time out on the rig.
  WiFi.setSleep(false);
  sayf(" %s  (%s.local)\n", WiFi.localIP().toString().c_str(), OTA_HOST);

  ArduinoOTA.setHostname(OTA_HOST);
  ArduinoOTA.onStart([]()  { sayfln("\nOTA start"); });
  ArduinoOTA.onEnd([]()    { sayfln("\nOTA done — rebooting"); });
  ArduinoOTA.onError([](ota_error_t e) { sayf("\nOTA error %u\n", e); });
  ArduinoOTA.begin();
  telnetSrv.begin();
  telnetSrv.setNoDelay(true);   // otherwise output arrives in laggy clumps
  sayf("telnet console: telnet %s 23\n", WiFi.localIP().toString().c_str());
  otaReady = true;
}


// Line assembly is deliberately non-blocking and printable-only.
// readStringUntil() blocks for its 1 s timeout and returns whatever arrives, so
// one stray byte on a floating RX line reads as an unrecognised command and the
// console fills with "?" forever -- and the same blocking stalls ArduinoOTA for
// a second at a time. Accumulate characters instead, drop anything unprintable.
static char    lineBuf[96];
static uint8_t lineLen = 0;

static void handleLine(const char *line) {
  String s(line); s.trim();
  if (!s.length()) return;

  // Separate commands because the combined "wifi <ssid> <pass>" splits on the
  // first space -- an SSID containing a space silently puts half of itself into
  // the password, and the only symptom is 4WAY_HANDSHAKE_TIMEOUT, which reads
  // as a wrong password. Take the whole rest of the line each time instead.
  if (s.startsWith("ssid ")) {
    prefs.putString("ssid", s.substring(5));
    sayf("ssid = \"%s\" (%d chars) — set pass, then reboot\n",
                  s.substring(5).c_str(), s.length() - 5);
  }
  else if (s.startsWith("pass ")) {
    prefs.putString("pass", s.substring(5));
    sayf("pass saved, %d chars — reboot to join\n", s.length() - 5);
  }
  else if (s == "wifi?") {
    // SSID echoed in full, password only as a length: enough to spot a
    // truncation or a stray character without printing the secret.
    String ss = prefs.getString("ssid", "");
    String pp = prefs.getString("pass", "");
    sayf("stored ssid = \"%s\" (%d chars)\n", ss.c_str(), ss.length());
    sayf("stored pass = %d chars\n", pp.length());
  }
  else if (s == "wifi clear") { prefs.remove("ssid"); prefs.remove("pass");
                                sayfln("cleared"); }
  else if (s.startsWith("wifi ")) {
    int sp = s.indexOf(' ', 5);
    if (sp < 0) { sayfln("usage: ssid <name>  then  pass <password>"); return; }
    prefs.putString("ssid", s.substring(5, sp));
    prefs.putString("pass", s.substring(sp + 1));
    sayfln("saved — reboot to join  (use ssid/pass separately if either has a space)");
  }
  else if (s == "can")    canStatus();
  else if (s == "canreset") canReset();
  else if (s == "r48")      canStatus();
  else if (s == "r48init") {
    r48Mode = R48_INIT; r48NextTx = 0; r48LastRx = 0;
    sayfln("R48: ACTIVE -- handshake every 10 s until HELLO, then keep-alive every 6 s.");
    sayfln("     Nothing sent changes a setpoint. 'r48stop' returns to listen-only.");
  }
  else if (s == "r48stop") {
    r48Mode = R48_LISTEN; sayfln("R48: LISTEN -- transmitting nothing.");
  }
  else if (s == "selftest") canSelfTest();
  // Drive the transceiver's D input directly as a plain GPIO, bypassing TWAI.
  // A hand jumper to GND depends on a contact we cannot verify; the P14 wire has
  // been measured at 0 ohms, so this commands the driver through known-good
  // copper. Datasheet: D LOW = dominant, HIGH = recessive.
  // 125 kHz 50% square wave on D -- a CAN-shaped signal. The DC 'dominant'
  // hold is NOT how CAN works: a real dominant bit is 8 us at 125 kbps, and a
  // transceiver with a dominant-state timeout disables its driver after ~1 ms
  // of continuous low. If that is what this chip does, the DC test fails a
  // perfectly good driver every time, and this one does not.
  else if (s == "toggle") {
    if (canUp) { twai_stop(); twai_driver_uninstall(); canUp = false; }
    ledcSetup(0, 125000, 4);              // ch0, 125 kHz, 4-bit resolution
    ledcAttachPin(CAN_TX, 0);
    ledcWrite(0, 8);                      // 8/16 = 50% duty
    sayfln("D (GPIO14): 125 kHz square wave, 50% duty -- CAN-shaped drive.");
    sayfln("  A working driver averages CANH ~2.6 V and CANL ~1.6 V (meter smooths it).");
    sayfln("  Recessive-only averages ~1.5 V on both. 'canreset' to stop.");
  }
  else if (s == "dominant" || s == "recessive") {
    bool dom = (s == "dominant");
    if (canUp) { twai_stop(); twai_driver_uninstall(); canUp = false; }
    pinMode(CAN_TX, OUTPUT);
    digitalWrite(CAN_TX, dom ? LOW : HIGH);
    delay(5);
    sayf("D (GPIO%d) held %s -> transceiver commanded %s.\n", CAN_TX,
         dom ? "LOW" : "HIGH", dom ? "DOMINANT" : "RECESSIVE");
    sayf("  readback of the pin: %d  (should be %d)\n", digitalRead(CAN_TX), dom ? 0 : 1);
    sayfln("  measure CANH and CANL now. DOMINANT should be about 3.0 V and 1.0 V,");
    sayfln("  RECESSIVE about 2.3 V on both. 'canreset' returns to normal CAN.");
  }
  else if (s == "canraw") { canRaw = !canRaw; sayf("raw CAN dump %s\n", canRaw ? "ON" : "off"); }
  else if (s == "ping") {
    pingMode = !pingMode;
    // Restart the controller on every toggle. Measured on the bench: after the
    // rectifier attempt the controller sits error-passive with frame N still
    // retrying, so twai_transmit only ever returns "queue full" and ping
    // reports nothing sent -- a failure caused by the PREVIOUS test, not by the
    // peer. A test that cannot tell those apart is the thing we are trying to
    // escape, so entering ping mode always starts from a clean controller.
    canReset();
    sayf("ping mode %s -- %s\n", pingMode ? "ON" : "off",
         pingMode ? "sending 0x123 at 1 Hz; peer must run the can_loop LISTENER build"
                  : "back to the R48 request frame");
  }
  else if (s == "scan")   scan();
  else if (s == "id")     identify();
  else if (s == "r")      dump();
  else if (s == "stream") { streaming = true;  sayfln("streaming — 'stop' to end"); }
  else if (s == "stop")   { streaming = false; sayfln("stopped"); }
  else sayf("? \"%s\" — scan | id | r | stream | stop | ssid <n> | pass <p> | wifi? | wifi clear\n",
                     s.c_str());
}

void loop() {
  while (Serial.available()) {
    int c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (lineLen) { lineBuf[lineLen] = 0; lineLen = 0; handleLine(lineBuf); }
    } else if (c >= 0x20 && c < 0x7F) {
      if (lineLen < sizeof(lineBuf) - 1) lineBuf[lineLen++] = (char)c;
    }
    // nulls, 0xFF and framing junk are dropped silently -- they are line noise,
    // not commands, and echoing them back is how the console jammed.
  }

  canService();
  if (otaReady) {
    ArduinoOTA.handle();

    // One client at a time. A second connection displaces the first rather than
    // being refused -- a stale session left open by a closed laptop lid should
    // never lock you out of your own board.
    if (telnetSrv.hasClient()) {
      if (telnetCli && telnetCli.connected()) telnetCli.stop();
      telnetCli = telnetSrv.available();
      telnetCli.printf("\n=== INA228 bring-up — telnet console ===\n"
                       "commands:  scan | id | r | stream | stop\n");
    }
    static char tBuf[96];
    static uint8_t tLen = 0;
    while (telnetCli && telnetCli.available()) {
      int c = telnetCli.read();
      if (c == '\n' || c == '\r') {
        if (tLen) { tBuf[tLen] = 0; tLen = 0; handleLine(tBuf); }
      } else if (c >= 0x20 && c < 0x7F) {
        if (tLen < sizeof(tBuf) - 1) tBuf[tLen++] = (char)c;
      }
      // telnet negotiation bytes (0xFF IAC ...) are non-printable and dropped
      // by the same filter that drops line noise on the UART.
    }
  }
  if (streaming && millis() > nextRead) { nextRead = millis() + 500; dump(); if (canUp) sayf("  R48 %.2f V  %.1f A\n", r48V, r48I); }
}
