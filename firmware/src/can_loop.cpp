// Two-node CAN bench test -- proves the transceivers and the TWAI setup ALONE,
// with no rectifier involved.
//
// WHY THIS EXISTS: our CAN hardware has never been verified against a known-good
// node. It "looked fine" only because it was wired to something that may not be
// a bus at all, so there was nothing to fail against. tx_err=128 with rx_err=0
// is equally consistent with OUR transceiver not driving as with nothing being
// there. This test separates those two for good.
//
// HOW IT IS READ: ROUND TRIP, so ONE serial monitor on the TALKER tells the whole
// story. The listener echoes every frame back on a different ID; the talker prints
// whether the echo came home. You never need to watch the listener -- it only
// needs power. (It still prints on its own port if you want to look, and blinks
// GPIO2 on each frame so you can see it working with no monitor at all.)
//
// Deliberately NO WiFi/OTA here. Both boards need a USB flash anyway, and this is
// a bisection test: adding a network stack adds a failure mode to the very thing
// being trusted to report the answer.
//
// THE TWO BOARDS NEED NOT MATCH. CAN is a bus, not a link -- any TWAI-capable
// ESP32 talks to any other. Second node: a classic ESP32 from B3-R6C3 (parts 58,
// 62, 63) runs this EXACT binary with no rebuild. An S3 needs different pins
// because GPIO26-32 are its flash/PSRAM bus -- see env:can_loop_s3.
// Transceivers: 3x SN65HVD230 (part 19), 2 spare at B3-R3C6.
//
// ROLE is chosen at runtime by a jumper, so BOTH boards run the identical binary
// and neither can be "the one with the wrong build":
//
//     GPIO 27 (P27, pin 30) left OPEN  -> node A, TALKER   -- watch THIS one
//     GPIO 27 jumpered to GND          -> node B, LISTENER -- just needs power
//
// WIRING (per board):
//   NodeMCU 3V3 -> transceiver 3V3        GPIO14 (P14) -> CTX/D
//   NodeMCU GND -> transceiver GND        GPIO32 (P32) -> CRX/R
//
// BUS (between the two boards):
//   CANH <-> CANH,  CANL <-> CANL,  and GND <-> GND between the boards.
//   (Both boards in the same Mac already share ground through USB, but wire it
//   anyway -- a missing ground is the classic silent CAN failure.)
//   120 ohm across CANH/CANL at EACH end -> 220R || 270R = 121 ohm from the
//   EAONE kit; two pairs, one per board. The SN65HVD230 modules have NO onboard
//   termination -- their three resistors are marked 01C = 10k (EIA-96).
//
// PASS = the talker prints "echo OK" every second with all error counters at 0.

#include <Arduino.h>
#include <driver/twai.h>

// Pins are build flags so a DIFFERENT board can be the second node -- the two
// ends of a CAN bus have nothing to do with each other. Defaults are the classic
// ESP32 pins this machine will ship with; -DCAN_* overrides them per env.
#ifndef PIN_CAN_TX
#define PIN_CAN_TX  14      // P14 = pin 31
#endif
#ifndef PIN_CAN_RX
#define PIN_CAN_RX  32      // P32 = pin 26 -- NOT GPIO12
#endif
#ifndef PIN_ROLE
#define PIN_ROLE    27      // open = talker, GND = listener
#endif
#ifndef PIN_LED
#define PIN_LED     2       // onboard LED; -DPIN_LED=-1 if the board has none
#endif

static const gpio_num_t CAN_TX   = (gpio_num_t)PIN_CAN_TX;
static const gpio_num_t CAN_RX   = (gpio_num_t)PIN_CAN_RX;
static const int        ROLE_PIN = PIN_ROLE;
static const int        LED_PIN  = PIN_LED;

static const uint32_t ID_PING = 0x123;
static const uint32_t ID_ECHO = 0x124;

static bool talker = true;
static uint32_t seq = 0, sent = 0, echoed = 0, lost = 0, lastTx = 0, lastReport = 0;
static uint32_t txAt = 0; static bool waiting = false; static uint32_t waitSeq = 0;

static uint32_t be32(const uint8_t *d) {
  return ((uint32_t)d[0] << 24) | ((uint32_t)d[1] << 16) | ((uint32_t)d[2] << 8) | d[3];
}

static void counters(char *out, size_t n) {
  twai_status_info_t s;
  if (twai_get_status_info(&s) != ESP_OK) { snprintf(out, n, "status unreadable"); return; }
  snprintf(out, n, "%s tx_err=%lu rx_err=%lu bus_err=%lu",
           s.state == TWAI_STATE_RUNNING    ? "RUNNING"
         : s.state == TWAI_STATE_BUS_OFF    ? "BUS-OFF"
         : s.state == TWAI_STATE_RECOVERING ? "RECOVERING" : "STOPPED",
           (unsigned long)s.tx_error_counter, (unsigned long)s.rx_error_counter,
           (unsigned long)s.bus_error_count);
}

void setup() {
  Serial.begin(115200);
  delay(400);
  pinMode(ROLE_PIN, INPUT_PULLUP);
  if (LED_PIN >= 0) pinMode(LED_PIN, OUTPUT);
  delay(10);
  talker = digitalRead(ROLE_PIN);                     // HIGH (open) = talker

  Serial.printf("\n=== CAN two-node bench test -- role: %s ===\n",
                talker ? "TALKER (GPIO27 open) -- WATCH THIS PORT"
                       : "LISTENER (GPIO27 to GND) -- echoes, no need to watch");
  Serial.printf("%s  TX=GPIO%d RX=GPIO%d  125 kbps\n\n",
                ESP.getChipModel(), (int)CAN_TX, (int)CAN_RX);

  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX, CAN_RX, TWAI_MODE_NORMAL);
  g.rx_queue_len = 16;
  twai_timing_config_t t = TWAI_TIMING_CONFIG_125KBITS();
  twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  if (twai_driver_install(&g, &t, &f) != ESP_OK) { Serial.println("driver install FAILED"); return; }
  if (twai_start() != ESP_OK) { Serial.println("start FAILED"); return; }
  Serial.println(talker ? "Pinging once a second. Each line is one round trip.\n"
                        : "Echoing every frame. LED blinks on each one.\n");
}

void loop() {
  char c[96];

  if (talker && millis() - lastTx > 1000) {
    lastTx = millis();
    if (waiting) {                                    // previous ping never came home
      lost++;
      counters(c, sizeof c);
      Serial.printf("seq=%-4lu NO ECHO   | %s\n", (unsigned long)waitSeq, c);
    }
    twai_message_t m = {};
    m.identifier = ID_PING; m.data_length_code = 4;
    m.data[0] = seq >> 24; m.data[1] = seq >> 16; m.data[2] = seq >> 8; m.data[3] = seq;
    if (twai_transmit(&m, pdMS_TO_TICKS(50)) == ESP_OK) {
      sent++; waiting = true; waitSeq = seq; txAt = micros(); seq++;
    } else {
      counters(c, sizeof c);
      Serial.printf("TX QUEUE FAILED | %s\n", c);
    }
  }

  twai_message_t r;
  while (twai_receive(&r, 0) == ESP_OK) {
    if (!talker && r.identifier == ID_PING) {         // LISTENER: bounce it back
      if (LED_PIN >= 0) digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      twai_message_t e = r; e.identifier = ID_ECHO;
      twai_transmit(&e, pdMS_TO_TICKS(50));
      Serial.printf("  heard seq=%lu, echoed\n", (unsigned long)be32(r.data));
    } else if (talker && r.identifier == ID_ECHO) {   // TALKER: it came home
      uint32_t got = be32(r.data), rtt = micros() - txAt;
      if (waiting && got == waitSeq) { echoed++; waiting = false; }
      counters(c, sizeof c);
      Serial.printf("seq=%-4lu echo OK   | rtt %lu us | %s\n",
                    (unsigned long)got, (unsigned long)rtt, c);
    }
  }

  // Verdict every 5 s on the talker, so a failure does not need interpreting later.
  if (talker && millis() - lastReport > 5000) {
    lastReport = millis();
    twai_status_info_t s; twai_get_status_info(&s);
    if (echoed > 0 && lost == 0 && s.bus_error_count == 0) {
      Serial.printf("[PASS] %lu/%lu round trips, zero errors -- transceivers and "
                    "TWAI config are good.\n", (unsigned long)echoed, (unsigned long)sent);
    } else if (echoed == 0 && s.tx_error_counter >= 128) {
      Serial.println("[FAIL] transmitting, nothing acknowledging. Listener not powered/"
                     "running, CANH-CANL swapped between boards, or no common GND.");
    } else if (echoed == 0) {
      Serial.println("[FAIL] no echoes yet. Is the second board powered and is its "
                     "GPIO27 actually grounded?");
    } else {
      Serial.printf("[MARGINAL] %lu ok, %lu lost, bus_err=%lu -- check the 121R "
                    "termination at both ends.\n", (unsigned long)echoed,
                    (unsigned long)lost, (unsigned long)s.bus_error_count);
    }
  }
}
