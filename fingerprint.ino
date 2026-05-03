#include <Adafruit_Fingerprint.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= PIN =================
// ======= for ESP32 C3 Mini =============
#define BUTTON_PIN 5
#define BUZZER_PIN 4
#define RELAY_PIN 3
#define TOUCH_PIN 7

// ================= UART =================
#define RX_PIN 20
#define TX_PIN 21

HardwareSerial mySerial(1);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// ================= STATE =================
bool relayState = false;
bool scanning = false;
bool isSleep = false;
bool isEnrolling = false;
bool isDeleting = false;
bool sensorError = false;

// ================= DEBOUNCE =================
bool touchState = false;
bool lastTouchState = false;
bool lastButtonState = false;
bool buttonState = false;
unsigned long lastTouchChange = 0;
unsigned long lastButtonChange = 0;
const unsigned long debounceDelay = 50;

// ================= AUTO SLEEP =================
unsigned long lastActivity = 0;
const unsigned long sleepTimeout = 20000;

// ================= COOLDOWN =================
unsigned long lastScanTime = 0;
const unsigned long scanCooldown = 500;

// ================= ENROLL & DELETE =================
unsigned long enrollStartTime = 0;
int enrollStep = 0;
int newFingerprintID = 0;
unsigned long buttonPressTime = 0;
bool buttonPressed = false;

// Mode delete
int deleteStep = 0;
int selectedID = 0;
int availableIDs[127];
int availableCount = 0;

// ================= SETTINGS =================
const int CONFIDENCE_THRESHOLD = 40;
const int SCAN_TIMEOUT = 5000;

// ================= LED CONTROL =================

uint16_t calculateChecksum(uint8_t *cmd, int len) {
  uint16_t sum = 0;
  for (int i = 6; i < len - 2; i++) {
    sum += cmd[i];
  }
  return sum;
}

// ==== matikan lampu fingerprint saat sleep =====
void ledOff() {
  uint8_t cmd[] = {
    0xEF, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0x01,
    0x00, 0x07, 0x3C, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  uint16_t sum = calculateChecksum(cmd, sizeof(cmd));
  cmd[14] = (sum >> 8) & 0xFF;
  cmd[15] = sum & 0xFF;
  mySerial.write(cmd, sizeof(cmd));
} 

// ================= OLED =================
void showStatus(String mainText, String subText = "") {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(2);
  int16_t x = (128 - (mainText.length() * 12)) / 2;
  if (x < 0) x = 0;
  display.setCursor(x, 0);
  display.println(mainText);

  if (subText != "") {
    display.setTextSize(1);
    int16_t x2 = (128 - (subText.length() * 6)) / 2;
    if (x2 < 0) x2 = 0;
    display.setCursor(x2, 24);
    display.println(subText);
  }

  display.display();
}

void oledOff() {
  display.clearDisplay();
  display.display();
  display.ssd1306_command(SSD1306_DISPLAYOFF);
}

void oledOn() {
  display.ssd1306_command(SSD1306_DISPLAYON);
}

String relayText() {
  return relayState ? "Vehicle ON" : "Vehicle OFF";
}

void showReady() {
  if (sensorError) {
    showStatus("ERROR", "Check sensor");
    return;
  }
  oledOn();
  showStatus("READY", relayText());
}

// ================= BUZZER =================
void beep(int duration) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}

void beepSuccess() { 
  beep(80);
}

void beepFail() { 
  beep(40);
  delay(40);
  beep(40);
}

void beepEnrollSuccess() {
  beep(80);
  delay(80);
  beep(80);
}

void beepDeleteSuccess() {
  beep(100);
  delay(100);
  beep(100);
}

void beepError() {
  // BEEP ERROR: 3 beep pendek
  beep(50);
  delay(50);
  beep(50);
  delay(50);
  beep(50);
}

// ================= RELAY =================
void setRelay(bool state) {
  relayState = state;
  digitalWrite(RELAY_PIN, state ? HIGH : LOW);
}

void toggleRelay() {
  setRelay(!relayState);
}

// ================= LIST AVAILABLE FINGERPRINTS =================
void getAvailableIDs() {
  availableCount = 0;
  for (int id = 1; id <= 127; id++) {
    if (finger.loadModel(id) == FINGERPRINT_OK) {
      availableIDs[availableCount++] = id;
    }
    delay(10);
  }
}

// ================= DELETE FINGERPRINT BY ID =================
void deleteFingerprintByID(int id) {
  if (finger.deleteModel(id) == FINGERPRINT_OK) {
    showStatus("DELETED", "ID: " + String(id));
    beepDeleteSuccess();
  } else {
    showStatus("FAILED", "ID: " + String(id));
    beepFail();
  }
  delay(1000);
  showReady();
}

// ================= START DELETE MODE =================
void startDeleteMode() {
  if (isDeleting || isEnrolling || scanning) return;
  
  isDeleting = true;
  deleteStep = 0;
  
  getAvailableIDs();
  
  if (availableCount == 0) {
    showStatus("NO DATA", "No fingerprints");
    beepFail();
    delay(1500);
    isDeleting = false;
    showReady();
    return;
  }
  
  selectedID = 0;
  showStatus("SELECT ID", "Touch to choose");
  delay(1000);
  showDeleteSelection();
}

void showDeleteSelection() {
  if (selectedID < availableCount) {
    showStatus("ID: " + String(availableIDs[selectedID]), "Touch=Select");
  } else {
    isDeleting = false;
    showReady();
  }
}

void selectNextID() {
  selectedID++;
  if (selectedID >= availableCount) {
    isDeleting = false;
    showStatus("DONE", "Delete mode exit");
    delay(1000);
    showReady();
  } else {
    showDeleteSelection();
  }
}

void confirmDelete() {
  if (selectedID < availableCount) {
    int idToDelete = availableIDs[selectedID];
    deleteFingerprintByID(idToDelete);
    for (int i = selectedID; i < availableCount - 1; i++) {
      availableIDs[i] = availableIDs[i + 1];
    }
    availableCount--;
    if (availableCount == 0) {
      isDeleting = false;
      showStatus("ALL DELETED");
      delay(1000);
      showReady();
    } else {
      showDeleteSelection();
    }
  }
}

// ================= REINIT FINGERPRINT =================
bool reinitFingerprint() {
  while (mySerial.available()) {
    mySerial.read();
  }
  
  // Coba verify dengan retry
  for (int attempt = 0; attempt < 3; attempt++) {
    if (finger.verifyPassword()) {
      sensorError = false;
      return true;
    }
    delay(100);
  }
  
  sensorError = true;
  return false;
}

// ================= FINGERPRINT ENROLL =================
int getFreeID() {
  for (int id = 1; id <= 127; id++) {
    if (finger.loadModel(id) != FINGERPRINT_OK) {
      return id;
    }
    delay(10);
  }
  return -1;
}

void enrollFingerprint() {
  if (sensorError) {
    showStatus("SENSOR ERR");
    beepFail();
    delay(1000);
    showReady();
    return;
  }
  
  if (isEnrolling || scanning || isDeleting) {
    showStatus("BUSY");
    beepFail();
    delay(500);
    showReady();
    return;
  }
  
  isEnrolling = true;
  enrollStep = 0;
  
  newFingerprintID = getFreeID();
  if (newFingerprintID == -1) {
    showStatus("FULL");
    beepFail();
    isEnrolling = false;
    delay(1000);
    showReady();
    return;
  }
  
  showStatus("ENROLL", "ID:" + String(newFingerprintID));
  delay(800);
  showStatus("Place finger");
  enrollStartTime = millis();
  enrollStep = 1;
}

void processEnroll() {
  if (!isEnrolling) return;
  
  int p = -1;
  
  switch (enrollStep) {
    case 1:
      p = finger.getImage();
      if (p == FINGERPRINT_OK) {
        p = finger.image2Tz(1);
        if (p == FINGERPRINT_OK) {
          showStatus("Remove finger");
          enrollStep = 2;
          enrollStartTime = millis();
          beepSuccess();
        } else {
          showStatus("Bad quality");
          beepFail();
          delay(800);
          showStatus("Place finger");
          enrollStartTime = millis();
        }
      }
      
      if (millis() - enrollStartTime > 8000) {
        showStatus("Timeout");
        beepFail();
        isEnrolling = false;
        delay(800);
        showReady();
      }
      break;
      
    case 2:
      p = finger.getImage();
      if (p == FINGERPRINT_NOFINGER) {
        showStatus("Place again");
        enrollStep = 3;
        enrollStartTime = millis();
      } else if (millis() - enrollStartTime > 5000) {
        showStatus("Timeout");
        beepFail();
        isEnrolling = false;
        delay(800);
        showReady();
      }
      break;
      
    case 3:
      p = finger.getImage();
      if (p == FINGERPRINT_OK) {
        p = finger.image2Tz(2);
        if (p == FINGERPRINT_OK) {
          p = finger.createModel();
          if (p == FINGERPRINT_OK) {
            p = finger.storeModel(newFingerprintID);
            if (p == FINGERPRINT_OK) {
              showStatus("SUCCESS");
              beepEnrollSuccess();
              isEnrolling = false;
              delay(800);
              showReady();
            } else {
              showStatus("STORAGE FAIL");
              beepFail();
              isEnrolling = false;
              delay(800);
              showReady();
            }
          } else {
            showStatus("NO MATCH");
            beepFail();
            isEnrolling = false;
            delay(800);
            showReady();
          }
        } else {
          showStatus("BAD IMAGE");
          beepFail();
          isEnrolling = false;
          delay(800);
          showReady();
        }
      } else if (millis() - enrollStartTime > 8000) {
        showStatus("Timeout");
        beepFail();
        isEnrolling = false;
        delay(800);
        showReady();
      }
      break;
  }
}

// ================= SCAN FINGERPRINT =================
void scanFingerprint() {
  if (sensorError) {
    showStatus("SENSOR ERR");
    beepFail();
    delay(500);
    showReady();
    return;
  }
  
  if (millis() - lastScanTime < scanCooldown) return;
  if (scanning || isEnrolling || isDeleting) return;
  
  scanning = true;
  lastScanTime = millis();
  
  showStatus("SCAN", "Place finger");
  
  unsigned long startTime = millis();
  int p = FINGERPRINT_NOFINGER;
  
  while (p == FINGERPRINT_NOFINGER && millis() - startTime < SCAN_TIMEOUT) {
    p = finger.getImage();
    delay(30);
  }
  
  if (p != FINGERPRINT_OK) {
    scanning = false;
    showReady();
    return;
  }

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) {
    showStatus("BAD IMAGE");
    beepFail();
    delay(300);
    scanning = false;
    showReady();
    return;
  }

  p = finger.fingerSearch();

  if (p == FINGERPRINT_OK && finger.confidence > CONFIDENCE_THRESHOLD) {
    toggleRelay();
    beepSuccess();
    showStatus("UNLOCKED");
    delay(600);
    showReady();
  } else {
    showStatus("DENIED");
    beepFail();
    delay(400);
    showReady();
  }

  scanning = false;
}

// ================= SLEEP =================
void goSleep() {
  if (isSleep || isEnrolling || scanning || isDeleting) return;
  isSleep = true;
  
  ledOff();
  oledOff();
  digitalWrite(BUZZER_PIN, LOW);
}

void wakeUp() {
  if (!isSleep) return;
  
  isSleep = false;
  oledOn();
  showStatus("WAKE");
  delay(100);
  reinitFingerprint();
  showReady();
  lastActivity = millis();
}

// ================= INIT FINGERPRINT WITH RETRY =================
bool initFingerprint() {
  // Beri waktu sensor stabil
  delay(500);
  
  // Flush serial buffer
  while (mySerial.available()) {
    mySerial.read();
  }
  
  // Coba koneksi dengan retry
  for (int attempt = 0; attempt < 5; attempt++) {
    if (finger.verifyPassword()) {
      return true;
    }
    delay(200);
  }
  return false;
}

// ================= SETUP =================
void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(TOUCH_PIN, INPUT);
  
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(RELAY_PIN, LOW);
  relayState = false;
  
  // Inisialisasi OLED
  Wire.begin(6, 8);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  showStatus("STARTING");
  
  // Inisialisasi UART Fingerprint
  mySerial.begin(57600, SERIAL_8N1, RX_PIN, TX_PIN);
  finger.begin(57600);
  
  // Coba koneksi ke sensor
  if (initFingerprint()) {
    sensorError = false;
    showStatus("OK");
    beepSuccess();
    delay(500);
    showReady();
  } else {
    sensorError = true;
    showStatus("SENSOR ERR");
    // BEEP ERROR: 3 kali, tapi tidak infinite loop
    for (int i = 0; i < 3; i++) {
      beepError();
      delay(200);
    }
    // Tetap lanjut ke ready meskipun error (tidak infinite loop)
    showStatus("ERROR", "Check wiring");
  }
  
  lastActivity = millis();
}

// ================= LOOP =================
void loop() {
  // Handle sleep state
  if (isSleep) {
    if (digitalRead(TOUCH_PIN) == HIGH) {
      delay(debounceDelay);
      if (digitalRead(TOUCH_PIN) == HIGH) {
        wakeUp();
      }
    }
    return;
  }
  
  // Handle enroll process
  if (isEnrolling) {
    processEnroll();
    return;
  }
  
  // Handle delete mode
  if (isDeleting) {
    bool reading = digitalRead(TOUCH_PIN);
    if (reading != lastTouchState) {
      lastTouchChange = millis();
    }
    
    if ((millis() - lastTouchChange) > debounceDelay) {
      if (reading != touchState) {
        touchState = reading;
        if (touchState == HIGH) {
          confirmDelete();
          lastActivity = millis();
        }
      }
    }
    lastTouchState = reading;
    
    bool buttonReading = digitalRead(BUTTON_PIN);
    if (buttonReading == LOW && lastButtonState == HIGH) {
      selectNextID();
      lastActivity = millis();
      delay(200);
    }
    lastButtonState = buttonReading;
    
    return;
  }
  
  // Button handling
  bool buttonReading = digitalRead(BUTTON_PIN);
  
  if (buttonReading == LOW && lastButtonState == HIGH) {
    buttonPressed = true;
    buttonPressTime = millis();
    showStatus("HOLD", "5s for delete");
  }
  
  if (buttonReading == HIGH && lastButtonState == LOW && buttonPressed) {
    unsigned long pressDuration = millis() - buttonPressTime;
    buttonPressed = false;
    
    if (pressDuration >= 5000 && !sensorError) {
      startDeleteMode();
    } else if (pressDuration >= 100 && !sensorError) {
      enrollFingerprint();
      lastActivity = millis();
    } else if (pressDuration >= 100 && sensorError) {
      // Jika error, coba reinit
      showStatus("RETRY");
      if (initFingerprint()) {
        sensorError = false;
        beepSuccess();
        showReady();
      } else {
        beepFail();
      }
      lastActivity = millis();
    }
  }
  
  lastButtonState = buttonReading;
  
  // Update display saat button ditahan
  if (buttonPressed && !sensorError) {
    if ((millis() - buttonPressTime) > 5000) {
      showStatus("DELETE MODE", "Release now");
    } else if ((millis() - buttonPressTime) > 3000) {
      showStatus("HOLD", "2s more...");
    }
  }
  
  // Touch sensor scan (normal mode)
  bool reading = digitalRead(TOUCH_PIN);
  if (reading != lastTouchState) {
    lastTouchChange = millis();
  }
  
  if ((millis() - lastTouchChange) > debounceDelay) {
    if (reading != touchState) {
      touchState = reading;
      if (touchState == HIGH && !sensorError) {
        scanFingerprint();
        lastActivity = millis();
      } else if (touchState == HIGH && sensorError) {
        showStatus("SENSOR ERR");
        beepFail();
        delay(300);
        showReady();
        lastActivity = millis();
      }
    }
  }
  lastTouchState = reading;
  
  // Auto sleep
  if (millis() - lastActivity > sleepTimeout && !isSleep && !isEnrolling && !scanning && !isDeleting) {
    goSleep();
  }
}
