#ifndef CONFIG_H
#define CONFIG_H

// ================= WIFI (DEFAULT AP MODE) =================
#define DEFAULT_WIFI_SSID "EVFINGERPRINT"
#define DEFAULT_WIFI_PASS "YNTKTS123"

// ================= FINGERPRINT SENSOR SLEEP =================
// set true jika ingin lampu sensor langsung mati jika tak ada jari namun resiko jadi kurang responsif
// false lampu akan terus berkedip stand by warna biru namun sangat responsif
#define ENABLE_SENSOR_SLEEP false

// ================= PIN =================
#define BUZZER_PIN 4
#define RELAY_PIN 3
#define TOUCH_PIN 7
#define WIFI_TOGGLE_PIN 5

// ================= UART =================
#define RX_PIN 20
#define TX_PIN 21
#define SERIAL_BAUD 57600

// ================= TIME =================
#define LED_IDLE_TIMEOUT 10000 //aktif jika ENABLE_SENSOR_SLEEP set ke true
#define ENROLL_TIMEOUT   10000
#define SCAN_TIMEOUT     5000
#define DEBOUNCE_DELAY   50

// ================= FINGERPRINT =================
// CONFIDENCE_THRESHOLD di bawah 40 Bisa salah terima jari lain
// 40-60 adalah angka standart, 
// di atas 60 jari yang cocok bisa tidak diterima jika bergeser sedikit
// ===============================================
#define CONFIDENCE_THRESHOLD 40 
#define MAX_FINGERPRINT_ID   127

#endif
