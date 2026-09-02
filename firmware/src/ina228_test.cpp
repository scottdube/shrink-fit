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
#include <Wire.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <Preferences.h>

// Hostname deliberately DIFFERENT from the test rig's "shrinkfit-testrig" --
// both are on the same network and an mDNS collision would send a flash to the
// wrong board. No OTA password, consistent with the rig (Scott, 2026-08-27).
static const char *OTA_HOST = "shrinkfit-ina228";
static Preferences prefs;
static bool otaReady = false;
static void otaBegin();   // defined below, after the register helpers

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
  Serial.println("\nI2C scan on SDA=21 SCL=22 ...");
  uint8_t found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  0x%02X  responds", a);
      if (a >= 0x40 && a <= 0x4F) Serial.print("   <-- INA228 address range");
      Serial.println();
      found++;
      if (a >= 0x40 && a <= 0x4F) inaAddr = a;
    }
  }
  if (!found) {
    Serial.println("  NOTHING RESPONDED.");
    Serial.println("  Check: 3V3 and GND actually connected? SDA/SCL not swapped?");
    Serial.println("  This board has its own pull-ups, so external ones are not needed.");
  } else {
    Serial.printf("  %u device(s). Using 0x%02X.\n", found, inaAddr);
  }
}

// --- identity ----------------------------------------------------------------
// The check that matters: a chip that answers its address might still be
// anything. Manufacturer 0x5449 and device 0x228x say it is genuinely an INA228.
static void identify() {
  uint16_t mfg = 0, dev = 0;
  bool okM = read16(REG_MFG_ID, &mfg);
  bool okD = read16(REG_DEVICE_ID, &dev);
  Serial.printf("\nMFG_ID    0x%04X  %s\n", mfg,
                (okM && mfg == 0x5449) ? "OK  (TI)" : "UNEXPECTED — expect 0x5449");
  Serial.printf("DEVICE_ID 0x%04X  %s\n", dev,
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

  Serial.printf("CONFIG 0x%04X  ADC_CONFIG 0x%04X  SHUNT_CAL 0x%04X  ADCRANGE %d\n",
                cfg, adc, cal, adcRange ? 1 : 0);
  Serial.printf("VSHUNT %+8ld raw = %+10.3f uV     VBUS %8ld raw = %7.4f V     DIE %.1f C\n",
                (long)vsh, shuntUV, (long)vbus, busV, dieC);
}

void setup() {
  Serial.begin(115200);
  delay(400);
  Serial.println("\n=== INA228 bring-up — 38-pin ESP32 ===");
  Serial.printf("chip %s  rev %d  %d MHz\n",
                ESP.getChipModel(), ESP.getChipRevision(), getCpuFrequencyMhz());
  prefs.begin("ina228", false);
  Wire.begin(PIN_SDA, PIN_SCL, 100000);   // 100 kHz: bring-up, not throughput
  scan();
  identify();
  otaBegin();
  Serial.println("\ncommands:  scan | id | r (read once) | stream | stop | wifi <ssid> <pass>");
  Serial.println("Shorting VIN+ to VIN- should drive VSHUNT to ~0. Touching them");
  Serial.println("with a finger should move it. That is the analog front end alive.");
}

// Credentials live in NVS and are typed at the serial console -- they never
// appear in the repo or in a build. Set once with: wifi <ssid> <password>
static void otaBegin() {
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  if (!ssid.length()) { Serial.println("no wifi credentials — 'wifi <ssid> <pass>'"); return; }

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(OTA_HOST);
  WiFi.begin(ssid.c_str(), pass.c_str());
  Serial.printf("wifi: joining %s ", ssid.c_str());
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 15000) { delay(300); Serial.print("."); }
  if (WiFi.status() != WL_CONNECTED) { Serial.println(" FAILED"); return; }

  // Power save costs 40-140 ms of latency and made espota time out on the rig.
  WiFi.setSleep(false);
  Serial.printf(" %s  (%s.local)\n", WiFi.localIP().toString().c_str(), OTA_HOST);

  ArduinoOTA.setHostname(OTA_HOST);
  ArduinoOTA.onStart([]()  { Serial.println("\nOTA start"); });
  ArduinoOTA.onEnd([]()    { Serial.println("\nOTA done — rebooting"); });
  ArduinoOTA.onError([](ota_error_t e) { Serial.printf("\nOTA error %u\n", e); });
  ArduinoOTA.begin();
  otaReady = true;
}

static bool streaming = false;
static uint32_t nextRead = 0;

void loop() {
  if (Serial.available()) {
    String s = Serial.readStringUntil('\n'); s.trim();
    if (s.startsWith("wifi ")) {
      int sp = s.indexOf(' ', 5);
      if (sp < 0) Serial.println("usage: wifi <ssid> <password>");
      else {
        prefs.putString("ssid", s.substring(5, sp));
        prefs.putString("pass", s.substring(sp + 1));
        Serial.println("saved — reboot to join");
      }
    }
    else if (s == "scan")   scan();
    else if (s == "id")     identify();
    else if (s == "r")      dump();
    else if (s == "stream") { streaming = true;  Serial.println("streaming — 'stop' to end"); }
    else if (s == "stop")   { streaming = false; Serial.println("stopped"); }
    else if (s.length())    Serial.println("? scan | id | r | stream | stop");
  }
  if (otaReady) ArduinoOTA.handle();
  if (streaming && millis() > nextRead) { nextRead = millis() + 500; dump(); }
}
