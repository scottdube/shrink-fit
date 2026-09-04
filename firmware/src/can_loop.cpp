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
//   NO EXTERNAL TERMINATION. The blue SN65HVD230 modules already carry it: R2
//   is marked 121 = 120 ohm and sits directly across the CANH/CANL traces, and
//   R1 (103 = 10k) is the fixed RS slope resistor, which is why there is no RS
//   pin to tie down. Two modules = 120 ohm at each end = the 60 ohm a CAN bus
//   wants. Adding a 220||270 pair on top makes a THIRD terminator, ~40 ohm,
//   which loads the drivers.
//   (This comment previously claimed the opposite. It was wrong: an ambiguous
//   "01C" read off one resistor became a confident conclusion that the modules
//   were unterminated. Read R2 on the module in front of you.)
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
static uint32_t seq = 0, sent = 0, echoed = 0, lost = 0, txFail = 0, lastTx = 0, lastReport = 0;
static uint32_t txAt = 0; static bool waiting = false; static uint32_t waitSeq = 0;

static const uint8_t SCAN[] = {4, 13, 15, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 33};
static const uint8_t SCAN_N = sizeof(SCAN);

static void scanGrounded() {
  Serial.print("   grounded pins:");
  bool any = false;
  for (uint8_t i = 0; i < SCAN_N; i++)
    if (digitalRead(SCAN[i]) == LOW) { Serial.printf("  P%d", SCAN[i]); any = true; }
  if (!any) Serial.print("  NONE -- nothing is pulling any GPIO low. Suspect the"
                         " jumper wire itself, not the pin position.");
  Serial.println();
}

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

// Prove THIS node's whole path with no peer. NO_ACK drops the acknowledgement
// requirement and the self flag keeps a copy -- but the frame still leaves via
// CTX, is driven onto the real bus, and re-enters through the receiver on CRX.
// Comes back = every local link works and the fault is elsewhere. Does not =
// the fault is here, which no amount of testing against another board reveals.
static void selfTest() {
  Serial.println("\nself-test: NO_ACK mode, no peer needed ...");
  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX, CAN_RX, TWAI_MODE_NO_ACK);
  g.rx_queue_len = 8;
  twai_timing_config_t t = TWAI_TIMING_CONFIG_125KBITS();
  twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  if (twai_driver_install(&g, &t, &f) != ESP_OK) { Serial.println("  install FAILED"); return; }
  if (twai_start() != ESP_OK) { Serial.println("  start FAILED"); twai_driver_uninstall(); return; }

  twai_message_t m = {};
  m.identifier = 0x556; m.data_length_code = 2; m.data[0] = 0x5A; m.data[1] = 0xA5;
  m.self = 1;
  bool queued = (twai_transmit(&m, pdMS_TO_TICKS(200)) == ESP_OK);
  Serial.printf("  queued: %s\n", queued ? "yes" : "NO");

  bool got = false; uint32_t t0 = millis();
  while (millis() - t0 < 600 && !got) {
    twai_message_t r;
    if (twai_receive(&r, pdMS_TO_TICKS(50)) == ESP_OK && r.identifier == 0x556) got = true;
  }
  twai_status_info_t st; twai_get_status_info(&st);
  Serial.printf("  own frame back: %s   tx_err=%lu bus_err=%lu\n", got ? "YES" : "no",
                (unsigned long)st.tx_error_counter, (unsigned long)st.bus_error_count);
  Serial.println(got ? "  [PASS] CTX -> driver -> bus -> receiver -> CRX all work HERE."
                     : "  [FAIL] own frame never returned -- the break is on THIS board.");
  twai_stop(); twai_driver_uninstall();
}

void setup() {
  Serial.begin(115200);
  delay(400);
  pinMode(ROLE_PIN, INPUT_PULLUP);
  if (LED_PIN >= 0) pinMode(LED_PIN, OUTPUT);
  delay(10);
  // Scan every safely-readable GPIO instead of trusting a pin count. Three
  // attempts were spent on "the 9th pin from the USB end" with the board
  // reporting nothing grounded; naming the pin the wire is ACTUALLY on settles
  // wire-vs-position in one reading. Excluded: 14/32 (CAN, in use), 6-11 (SPI
  // flash), 12 (MTDI strapping -- sets flash voltage at boot), and 34-39
  // (input-only, no internal pullup, so they would float and read as noise).
  for (uint8_t i = 0; i < SCAN_N; i++) pinMode(SCAN[i], INPUT_PULLUP);
#ifdef ROLE_FIXED
  talker = ROLE_FIXED;      // build-time role: no jumper, nothing to seat
#else
  talker = digitalRead(ROLE_PIN);                     // HIGH (open) = talker
#endif

  Serial.printf("\n=== CAN two-node bench test -- role: %s ===\n",
                talker ? "TALKER -- WATCH THIS PORT" : "LISTENER -- echoes back");
#ifdef ROLE_FIXED
  Serial.println("role fixed at build time -- no jumper needed");
#endif
  Serial.printf("%s  TX=GPIO%d RX=GPIO%d  125 kbps\n\n",
                ESP.getChipModel(), (int)CAN_TX, (int)CAN_RX);

  selfTest();                      // before normal mode: proves this board alone

  twai_general_config_t g = TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX, CAN_RX, TWAI_MODE_NORMAL);
  g.rx_queue_len = 16;
  twai_timing_config_t t = TWAI_TIMING_CONFIG_125KBITS();
  twai_filter_config_t f = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  if (twai_driver_install(&g, &t, &f) != ESP_OK) { Serial.println("driver install FAILED"); return; }
  if (twai_start() != ESP_OK) { Serial.println("start FAILED"); return; }
  Serial.println(talker ? "Pinging once a second. Each line is one round trip.\n"
                        : "Echoing every frame. LED blinks on each one.\n");
}

// The role is re-read every pass rather than latched at boot. Two resets were
// spent on a jumper that was never landing, and "add the jumper THEN reset" is
// an instruction that silently produces two talkers when the second half is
// forgotten. Now the board follows the wire, and says so the moment it moves.
static void updateRole() {
#ifdef ROLE_FIXED
  return;                   // role is fixed at build time; ignore the pin
#else
  bool now = digitalRead(ROLE_PIN);
  if (now == talker) return;
  talker = now;
  seq = sent = echoed = lost = txFail = 0; waiting = false;
  Serial.printf("\n*** ROLE -> %s ***\n", talker ? "TALKER (P27 open)"
                                                  : "LISTENER (P27 grounded)");
#endif
}

void loop() {
  char c[96];
  updateRole();

  if (talker && millis() - lastTx > 1000) {
    lastTx = millis();
    twai_message_t m = {};
    m.identifier = ID_PING; m.data_length_code = 4;
    m.data[0] = seq >> 24; m.data[1] = seq >> 16; m.data[2] = seq >> 8; m.data[3] = seq;
    // Retire the pending frame only when a NEW one actually goes out. Reporting
    // it up front reprinted the same seq once a second, which reads as many
    // losses when it is one stuck frame -- a jam and a lost frame are different
    // faults and the log has to say which.
    if (twai_transmit(&m, pdMS_TO_TICKS(50)) == ESP_OK) {
      if (waiting) {
        lost++;
        counters(c, sizeof c);
        Serial.printf("seq=%-4lu NO ECHO   | %s\n", (unsigned long)waitSeq, c);
      }
      sent++; waiting = true; waitSeq = seq; txAt = micros(); seq++;
    } else if (++txFail == 3) {
      Serial.println("TX queue full -- frame 1 is still pending. Counted from here on.");
    }
  }

  twai_message_t r;
  while (twai_receive(&r, 0) == ESP_OK) {
    if (r.identifier != ID_PING && r.identifier != ID_ECHO) {
      // Anything at all arriving proves the bus carries traffic between the
      // boards, even when it is not part of the ping protocol. Silence here
      // while the peer transmits is itself the finding.
      Serial.printf("  HEARD id=0x%03lX dlc=%d\n",
                    (unsigned long)r.identifier, r.data_length_code);
    }
    if (!talker && r.identifier == ID_PING) {         // LISTENER: bounce it back
      if (LED_PIN >= 0) digitalWrite(LED_PIN, !digitalRead(LED_PIN));
      twai_message_t e = r; e.identifier = ID_ECHO;
      if (twai_transmit(&e, pdMS_TO_TICKS(50)) == ESP_OK) echoed++;
      Serial.printf("  heard seq=%lu, echoed\n", (unsigned long)be32(r.data));
    } else if (talker && r.identifier == ID_ECHO) {   // TALKER: it came home
      uint32_t got = be32(r.data), rtt = micros() - txAt;
      if (waiting && got == waitSeq) { echoed++; waiting = false; }
      counters(c, sizeof c);
      Serial.printf("seq=%-4lu echo OK   | rtt %lu us | %s\n",
                    (unsigned long)got, (unsigned long)rtt, c);
    }
  }

  if (!talker && millis() - lastReport > 5000) {
    lastReport = millis();

    // SELF-TEST. A listener that hears nothing is silent whether its
    // transceiver is connected or not -- identical counters both ways, which
    // makes 'is this end even alive?' unanswerable at exactly the moment it
    // matters. So it sends one frame of its own and reads the result: the
    // transmit path exercises the same transceiver the receive path uses.
    twai_message_t p = {};
    p.identifier = 0x7FF; p.data_length_code = 1; p.data[0] = 0xA5;
    bool queued = (twai_transmit(&p, pdMS_TO_TICKS(50)) == ESP_OK);

    twai_status_info_t s; twai_get_status_info(&s);
    Serial.printf("[LISTENER] echoed %lu | rx_err=%lu bus_err=%lu tx_err=%lu\n",
                  (unsigned long)echoed, (unsigned long)s.rx_error_counter,
                  (unsigned long)s.bus_error_count,
                  (unsigned long)s.tx_error_counter);
    if (echoed > 0)
      Serial.println("  [OK] frames received and echoed -- this end is working.");
    else if (!queued && s.tx_error_counter == 0 && s.bus_error_count == 0)
      Serial.println("  [NO TRANSCEIVER] cannot queue, every counter 0 -- the "
                     "controller never sees bus-idle. Check 3V3, GND, "
                     "P14->CTX, P32->CRX.");
    else if (s.tx_error_counter > 0 || s.bus_error_count > 0)
      Serial.println("  [TRANSCEIVER OK, BUS QUIET] our probe goes out and is not "
                     "acknowledged. This end is fine; nothing is answering yet.");
    else
      Serial.println("  [waiting] probe queued, no result yet.");
  }

  // Verdict every 5 s on the talker, so a failure does not need interpreting later.
  if (talker && millis() - lastReport > 5000) {
    lastReport = millis();
    twai_status_info_t s; twai_get_status_info(&s);
    scanGrounded();
    if (echoed > 0 && lost == 0 && s.bus_error_count == 0) {
      Serial.printf("[PASS] %lu/%lu round trips, zero errors -- transceivers and "
                    "TWAI config are good.\n", (unsigned long)echoed, (unsigned long)sent);
    } else if (echoed == 0 && s.tx_error_counter >= 128) {
      Serial.println("[FAIL] transmitting, nothing acknowledging. Listener not powered/"
                     "running, CANH-CANL swapped between boards, or no common GND.");
    } else if (echoed == 0 && txFail > 2 && s.tx_error_counter == 0 && s.bus_error_count == 0) {
      // Measured on a bare board: with NO transceiver wired, GPIO32 floats and
      // the controller never sees bus-idle, so it waits instead of erroring.
      // Every counter stays 0 and the queue fills. That is a DIFFERENT fault
      // from an unacknowledged bus (tx_err=128, bus_err climbing) and must not
      // be read as one -- it means the transceiver is not connected at all.
      Serial.println("[FAIL] queue full with every counter still 0 -- the controller is "
                     "waiting for a bus that never goes idle. Transceiver not wired: "
                     "check 3V3, GND, GPIO14->CTX, GPIO32->CRX.");
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
