#ifndef CONFIG_H
#define CONFIG_H

// ================= WIFI (DEFAULT AP MODE) =================
#define DEFAULT_WIFI_SSID "EVFINGERPRINT"
#define DEFAULT_WIFI_PASS "YNTKTS123"

// ================= WIFI POWER SAVING =================
#define WIFI_TIMEOUT_MS   300000  // 5 menit = 300000 ms

// ================= PIN =================
#define BUZZER_PIN 4
#define RELAY_PIN 3
#define TOUCH_PIN 7
#define RESET_WIFI_PIN 5

// ================= UART =================
#define RX_PIN 20
#define TX_PIN 21
#define SERIAL_BAUD 57600

// ================= TIME =================
#define LED_IDLE_TIMEOUT 10000
#define ENROLL_TIMEOUT   10000
#define SCAN_TIMEOUT     5000
#define DEBOUNCE_DELAY   50

// ================= FINGERPRINT =================
#define CONFIDENCE_THRESHOLD 40
#define MAX_FINGERPRINT_ID   127

#endif
