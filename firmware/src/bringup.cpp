// bringup.cpp — first-flash diagnostic. Not part of the machine firmware.
//
// Answers three things that are currently open:
//   1. Does this Super Mini have PSRAM, and is it octal? Octal PSRAM consumes
//      GPIO 33–37, which config.h currently warns about but cannot confirm.
//   2. Actual flash size — platformio.ini assumes the devkitc-1 default, and a
//      mismatch causes upload or partition trouble.
//   3. Does TWAI initialise, and does the rectifier broadcast?
//
// TWAI runs in **LISTEN_ONLY**. It does not transmit, does not ACK, and cannot
// jam the bus — the correct mode for first contact with the R48 (see the
// bring-up order in r48_can.cpp).
//
// Build:   pio run -e bringup -t upload && pio device monitor -e bringup
#include <Arduino.h>
#include <Wire.h>
#include <driver/twai.h>
#include "config.h"

static void banner() {
  Serial.println("\n===== shrink-fit bring-up =====");
  Serial.printf("chip      : %s rev %d, %d core(s)\n",
                ESP.getChipModel(), ESP.getChipRevision(), ESP.getChipCores());
  Serial.printf("flash     : %u bytes (%.1f MB)\n",
                ESP.getFlashChipSize(), ESP.getFlashChipSize() / 1048576.0);

  // --- the GPIO 33-37 question -------------------------------------------
  size_t psram = ESP.getPsramSize();
  if (psram == 0) {
    Serial.println("psram     : NONE  -> GPIO 33-37 are FREE to use");
  } else {
    Serial.printf("psram     : %u bytes (%.1f MB) present\n", psram, psram / 1048576.0);
    Serial.println("            -> if OCTAL, GPIO 33-37 are consumed. Quad leaves them free.");
    Serial.println("            -> config.h keeps the design inside GPIO 1-13 either way.");
  }
  Serial.printf("free heap : %u\n", ESP.getFreeHeap());
}

static void i2cScan() {
  Serial.println("\n--- I2C scan ---");
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, 400000);
  int found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  0x%02X%s\n", a, a == INA228_ADDR ? "  <- expected INA228" : "");
      found++;
    }
  }
  if (!found) Serial.println("  (nothing) — expected until the INA228 is fitted");
}

static bool twaiUp = false;

static void twaiListenOnly() {
  Serial.println("\n--- TWAI (listen-only) ---");
  twai_general_config_t g =
      TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_LISTEN_ONLY);
  g.rx_queue_len = 32;
  twai_timing_config_t t = TWAI_TIMING_CONFIG_125KBITS();
  twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();

  if (twai_driver_install(&g, &t, &f) != ESP_OK) { Serial.println("  install FAILED"); return; }
  if (twai_start() != ESP_OK) { Serial.println("  start FAILED"); twai_driver_uninstall(); return; }

  twaiUp = true;
  Serial.printf("  up at %d kbps on TX=%d RX=%d, LISTEN-ONLY\n",
                CAN_BITRATE_KBPS, (int)CAN_TX_PIN, (int)CAN_RX_PIN);
  Serial.println("  silence here is expected until the SN65HVD230 and the");
  Serial.println("  rectifier's 2-pin JST are wired. Silence WITH them wired means");
  Serial.println("  wrong bit rate, swapped CANH/CANL, or it isn't CAN.");
}

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 3000) { delay(10); }   // native USB CDC settle
  banner();
  i2cScan();
  twaiListenOnly();
  Serial.println("\n--- listening ---");
}

void loop() {
  if (!twaiUp) { delay(1000); return; }

  twai_message_t m;
  while (twai_receive(&m, pdMS_TO_TICKS(100)) == ESP_OK) {
    Serial.printf("ID 0x%08lX %s dlc=%d :", (unsigned long)m.identifier,
                  m.extd ? "ext" : "std", m.data_length_code);
    for (int i = 0; i < m.data_length_code; i++) Serial.printf(" %02X", m.data[i]);
    Serial.println();
  }

  // Bus health every 5 s — error counters rising with no frames means the bit
  // rate is wrong or the pair is swapped.
  static uint32_t tLast = 0;
  if (millis() - tLast > 5000) {
    tLast = millis();
    twai_status_info_t s;
    if (twai_get_status_info(&s) == ESP_OK) {
      Serial.printf("[bus] state=%d rx=%lu rx_missed=%lu bus_err=%lu\n",
                    (int)s.state, (unsigned long)s.msgs_to_rx,
                    (unsigned long)s.rx_missed_count,
                    (unsigned long)s.bus_error_count);
    }
  }
}
