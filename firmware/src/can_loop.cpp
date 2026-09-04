// Two-node CAN bench test — proves the transceivers and the TWAI setup ALONE,
// with no rectifier involved.
//
// WHY THIS EXISTS: our CAN hardware has never been verified against a known-good
// node. It "looked fine" only because it was wired to something that may not be
// a bus at all, so there was nothing to fail against. tx_err=128 with rx_err=0
// is equally consistent with OUR transceiver not driving as with nothing being
// there. This test separates those two for good.
//
// ROLE is chosen at runtime by a jumper, so BOTH boards run the identical binary
// and neither can be "the one with the wrong build":
//
//     GPIO 27 (P27, pin 30) left OPEN  -> node A, TALKER  (sends 1/second)
//     GPIO 27 jumpered to GND          -> node B, LISTENER (echoes what it hears)
//
// Both nodes ACK each other's frames, so a working pair shows rx climbing on BOTH
// and error counters pinned at zero.
//
// WIRING (per board):
//   NodeMCU 3V3 -> transceiver 3V3        GPIO14 (P14) -> CTX/D
//   NodeMCU GND -> transceiver GND        GPIO32 (P32) -> CRX/R
//
// BUS (between the two boards):
//   CANH <-> CANH,  CANL <-> CANL,  and GND <-> GND between the boards.
//   120 ohm across CANH/CANL at EACH end -> use 220R || 270R = 121 ohm from the
//   EAONE kit; two pairs, one per board.
//
// EXPECTED PASS: both nodes report rx climbing, tx_err = 0, rx_err = 0,
// bus_err = 0, and the listener prints the talker's counter incrementing.

#include <Arduino.h>
#include <driver/twai.h>

static const gpio_num_t CAN_TX   = GPIO_NUM_14;
static const gpio_num_t CAN_RX   = GPIO_NUM_32;
static const int        ROLE_PIN = 27;          // open = talker, GND = listener

static bool talker = true;
static uint32_t seq = 0, rxCount = 0, txCount = 0, lastTx = 0, lastReport = 0;

static void report() {
  twai_status_info_t s;
  if (twai_get_status_info(&s) != ESP_OK) { Serial.println("status read failed"); return; }
  const char *st = s.state == TWAI_STATE_RUNNING    ? "RUNNING"
                 : s.state == TWAI_STATE_BUS_OFF    ? "BUS-OFF"
                 : s.state == TWAI_STATE_RECOVERING ? "RECOVERING" : "STOPPED";
  Serial.printf("[%s] %s  tx=%lu rx=%lu | tx_err=%lu rx_err=%lu bus_err=%lu\n",
                talker ? "TALKER" : "LISTENER", st,
                (unsigned long)txCount, (unsigned long)rxCount,
                (unsigned long)s.tx_error_counter, (unsigned long)s.rx_error_counter,
                (unsigned long)s.bus_error_count);

  // Say what the numbers MEAN, so a failure does not need interpreting later.
  if (rxCount == 0 && txCount > 3) {
    if (s.tx_error_counter >= 128)
      Serial.println("   -> transmitting, NOTHING acknowledging. Other node off, "
                     "CANH/CANL swapped between boards, or no common GND.");
    else
      Serial.println("   -> no traffic yet. Is the other board powered and running?");
  }
  if (s.bus_error_count > 0 && rxCount > 0)
    Serial.println("   -> receiving but with errors: check termination (121R each end).");
}

void setup() {
  Serial.begin(115200);
  delay(400);
  pinMode(ROLE_PIN, INPUT_PULLUP);
  delay(10);
  talker = digitalRead(ROLE_PIN);            // HIGH (open) = talker

  Serial.printf("\n=== CAN two-node bench test — role: %s ===\n",
                talker ? "TALKER (GPIO27 open)" : "LISTENER (GPIO27 to GND)");
  Serial.printf("chip %s   TX=GPIO%d  RX=GPIO%d  125 kbps\n",
                ESP.getChipModel(), (int)CAN_TX, (int)CAN_RX);

  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX, CAN_RX, TWAI_MODE_NORMAL);
  g.rx_queue_len = 16;
  twai_timing_config_t t = TWAI_TIMING_CONFIG_125KBITS();
  twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  if (twai_driver_install(&g, &t, &f) != ESP_OK) { Serial.println("driver install FAILED"); return; }
  if (twai_start() != ESP_OK) { Serial.println("start FAILED"); return; }
  Serial.println("TWAI running. Talker sends ID 0x123 once a second.\n");
}

void loop() {
  if (talker && millis() - lastTx > 1000) {
    lastTx = millis();
    twai_message_t m = {};
    m.identifier = 0x123; m.data_length_code = 4;
    m.data[0] = seq >> 24; m.data[1] = seq >> 16; m.data[2] = seq >> 8; m.data[3] = seq;
    if (twai_transmit(&m, pdMS_TO_TICKS(50)) == ESP_OK) { txCount++; seq++; }
  }

  twai_message_t r;
  while (twai_receive(&r, 0) == ESP_OK) {
    rxCount++;
    Serial.printf("  RX id=0x%03lX dlc=%d :", (unsigned long)r.identifier, r.data_length_code);
    for (int i = 0; i < r.data_length_code; i++) Serial.printf(" %02X", r.data[i]);
    Serial.println();
  }

  if (millis() - lastReport > 3000) { lastReport = millis(); report(); }
}
