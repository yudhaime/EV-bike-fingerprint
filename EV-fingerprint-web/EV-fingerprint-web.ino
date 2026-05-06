#include <Adafruit_Fingerprint.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

#include "config.h"
#include "webdisplay.h"

// ================= WIFI TOGGLE BUTTON =================
unsigned long lastToggleTime = 0;
bool toggleTriggered = false;

// ================= SERIAL =================
HardwareSerial mySerial(1);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// ================= WEB =================
WebServer server(80);

// ================= STORAGE =================
Preferences prefs;
String fingerNames[MAX_FINGERPRINT_ID + 1];
bool idUsed[MAX_FINGERPRINT_ID + 1];

// ================= WIFI STORAGE =================
String wifiSsid = "";
String wifiPass = "";
bool wifiEnabled = false;           // Default MATI setelah booting
bool wifiAutoOnAfterRestart = false; // Flag untuk restart karena ganti WiFi

// ================= STATE =================
bool relayState = false;
bool scanning = false;
String lastTrigger = "-";

// ================= SLEEP =================
bool isSleep = false;
unsigned long lastActivity = 0;

// ================= TOUCH =================
bool lastTouchState = false;

// ================= ENROLL =================
enum EnrollState {
  ENROLL_IDLE,
  ENROLL_WAIT_FINGER,
  ENROLL_REMOVE,
  ENROLL_IMAGE2,
  ENROLL_STORE
};

EnrollState enrollState = ENROLL_IDLE;
String enrollMessage = "Menunggu...";
String enrollResult = "IDLE";
int enrollID = -1;
unsigned long enrollTimer = 0;

// ================= CEK SENSOR FINGERPRINT =================
bool sensorError = false;

void beepError(){
  digitalWrite(BUZZER_PIN, HIGH);
  delay(300);
  digitalWrite(BUZZER_PIN, LOW);
  delay(100);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(80);
  digitalWrite(BUZZER_PIN, LOW);
  delay(80);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(80);
  digitalWrite(BUZZER_PIN, LOW);
}

void resetSensorErrorFlag(){
  sensorError = false;
}

bool cekSensorFingerprint(){
  if(!finger.verifyPassword()){
    if(!sensorError){
      sensorError = true;
      for(int i=0;i<3;i++){
        beepError();
        delay(200);
      }
    }
    return false;
  }
  
  if(sensorError){
    sensorError = false;
    beep(50);
    delay(100);
    beep(50);
  }
  return true;
}

void beep(int d){
  digitalWrite(BUZZER_PIN, HIGH);
  delay(d);
  digitalWrite(BUZZER_PIN, LOW);
}

// ================= TOGGLE WIFI =================
void toggleWiFi(){
  if(wifiEnabled){
    // Matikan WiFi
    wifiEnabled = false;
    server.stop();
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
  } else {
    // Nyalakan WiFi
    wifiEnabled = true;
    setupWiFi();
    server.begin();
  }
}

void cekWifiToggleButton(){
  static unsigned long pressStartTime = 0;
  static bool buttonPressed = false;
  static bool beep1Done = false;
  static bool beep2Done = false;
  
  bool buttonState = digitalRead(WIFI_TOGGLE_PIN);
  
  if(buttonState == LOW){
    if(!buttonPressed){
      buttonPressed = true;
      pressStartTime = millis();
      beep1Done = false;
      beep2Done = false;
    }
    else {
      unsigned long pressDuration = millis() - pressStartTime;
      
      if(pressDuration >= 1000 && !beep1Done){
        beep1Done = true;
        beep(80);
      }
      
      if(pressDuration >= 2000 && !beep2Done){
        beep2Done = true;
        beep(80);
      }
      
      if(pressDuration >= 3000 && !toggleTriggered){
        toggleTriggered = true;
        beep(1000);  // Beep panjang 1 detik sebagai konfirmasi
        toggleWiFi();
      }
    }
  }
  else {
    buttonPressed = false;
    toggleTriggered = false;
  }
}

// ================= RELAY =================
void setRelay(bool s){
  relayState = s;
  digitalWrite(RELAY_PIN, s);
  lastActivity = millis();
}

void toggleRelay(){
  setRelay(!relayState);
}

// ================= LOAD NAMES =================
void loadNames(){
  for(int i=1;i<=MAX_FINGERPRINT_ID;i++){
    fingerNames[i] = prefs.getString(("id_"+String(i)).c_str(), "");
    idUsed[i] = prefs.getBool(("used_"+String(i)).c_str(), false);
    
    if(!idUsed[i]){
      fingerNames[i] = "";
    }
  }
}

// ================= LOAD WIFI =================
void loadWifiCredentials(){
  wifiSsid = prefs.getString("wifi_ssid", DEFAULT_WIFI_SSID);
  wifiPass = prefs.getString("wifi_pass", DEFAULT_WIFI_PASS);
  wifiAutoOnAfterRestart = prefs.getBool("wifi_auto_on", false);
}

void saveWifiCredentials(String ssid, String pass){
  prefs.putString("wifi_ssid", ssid);
  prefs.putString("wifi_pass", pass);
  wifiSsid = ssid;
  wifiPass = pass;
}

// ================= SETUP WIFI =================
void setupWiFi(){
  WiFi.mode(WIFI_OFF);
  delay(100);
  WiFi.mode(WIFI_AP);
  delay(100);
  
  String cleanSSID = wifiSsid;
  cleanSSID.replace(" ", "");
  cleanSSID.replace(":", "");
  cleanSSID.replace(";", "");
  
  WiFi.softAP(cleanSSID.c_str(), wifiPass.c_str());
}

// ================= WAKEUP SENSOR =================
void wakeupSensor(){
  if(ENABLE_SENSOR_SLEEP){
    mySerial.write(0x55);
    delay(100);
    finger.verifyPassword();
    delay(50);
  }
}

// ================= SAFE ID =================
int getSafeFreeID(){
  for(int i=1;i<=MAX_FINGERPRINT_ID;i++){
    if(!idUsed[i]){
      if(finger.loadModel(i) != FINGERPRINT_OK){
        return i;
      }
    }
  }
  return -1;
}

// ================= SCAN =================
void scanFingerprint(){
  if(isSleep || enrollState != ENROLL_IDLE || scanning) return;

  scanning = true;

  wakeupSensor();

  int p;
  unsigned long t = millis();

  while((p = finger.getImage()) != FINGERPRINT_OK && millis()-t < SCAN_TIMEOUT);

  if(p != FINGERPRINT_OK){
    scanning = false;
    return;
  }

  finger.image2Tz();
  p = finger.fingerSearch();

  if(p == FINGERPRINT_OK && finger.confidence > CONFIDENCE_THRESHOLD){

    String name = fingerNames[finger.fingerID];
    if(name != "") lastTrigger = "by " + name;
    else lastTrigger = "by ID " + String(finger.fingerID);

    toggleRelay();
    beep(150);
  } else {
    beep(40); delay(40); beep(40);
  }

  scanning = false;
}

// ================= ENROLL PROCESS =================
void processEnroll(){
  if(enrollState == ENROLL_IDLE) return;

  if(millis() - enrollTimer > ENROLL_TIMEOUT){
    enrollMessage = "TIMEOUT";
    enrollResult = "FAIL";
    enrollState = ENROLL_IDLE;
    resetSensorErrorFlag();
    return;
  }

  switch(enrollState){

    case ENROLL_WAIT_FINGER:
      wakeupSensor();
      
      if(finger.getImage() == FINGERPRINT_OK){
        if(finger.image2Tz(1) == FINGERPRINT_OK){
          enrollMessage = "LEPAS JARI";
          enrollState = ENROLL_REMOVE;
        }
      }
      break;

    case ENROLL_REMOVE:
      if(finger.getImage() == FINGERPRINT_NOFINGER){
        enrollMessage = "TEMPEL LAGI";
        enrollState = ENROLL_IMAGE2;
      }
      break;

    case ENROLL_IMAGE2:
      if(finger.getImage() == FINGERPRINT_OK){
        if(finger.image2Tz(2) == FINGERPRINT_OK){
          enrollState = ENROLL_STORE;
        }
      }
      break;

    case ENROLL_STORE:
      if(finger.createModel() == FINGERPRINT_OK &&
         finger.storeModel(enrollID) == FINGERPRINT_OK){

        idUsed[enrollID] = true;
        prefs.putBool(("used_"+String(enrollID)).c_str(), true);

        enrollMessage = "BERHASIL";
        enrollResult = "SUCCESS";

        beep(80);
        delay(120);
        beep(80);
        
        resetSensorErrorFlag();

      } else {

        enrollMessage = "GAGAL";
        enrollResult = "FAIL";

        for(int i=0;i<5;i++){
          beep(50);
          delay(60);
        }
        
        resetSensorErrorFlag();
      }

      enrollState = ENROLL_IDLE;
      lastActivity = millis();
      break;
  }
}

// ================= LED OFF =================
void ledOff(){
  uint8_t cmd[] = {
    0xEF,0x01,0xFF,0xFF,0xFF,0xFF,0x01,
    0x00,0x07,0x3C,0x04,0x00,0x00,0x00,0x00,0x00
  };

  uint16_t sum = 0;
  for(int i=6;i<14;i++) sum += cmd[i];

  cmd[14] = (sum >> 8) & 0xFF;
  cmd[15] = sum & 0xFF;

  mySerial.write(cmd, sizeof(cmd));
}

// ================= SLEEP =================
void goSleep(){
  if(isSleep || scanning || enrollState != ENROLL_IDLE) return;

  isSleep = true;
  
  if(ENABLE_SENSOR_SLEEP){
    ledOff();
  }
  
  digitalWrite(BUZZER_PIN, LOW);
}

void wakeUp(){
  if(!isSleep) return;

  isSleep = false;
  while(mySerial.available()) mySerial.read();
  
  wakeupSensor();
  
  lastActivity = millis();
}

// ================= FAST WAKE =================
void handleSleep(){
  if(isSleep){
    static unsigned long wakeTimer = 0;
    if(digitalRead(TOUCH_PIN) == HIGH){
      if(wakeTimer == 0) wakeTimer = millis();
      if(millis() - wakeTimer > 25){
        wakeUp();
        wakeTimer = 0;
      }
    } else {
      wakeTimer = 0;
    }
    return;
  }
}

// ================= WEB HANDLERS =================
void handleRoot(){ 
  if(!wifiEnabled) return;
  server.send(200,"text/html",getWebPage()); 
}

void handleRelay(){
  if(!wifiEnabled) return;
  toggleRelay();
  lastTrigger = "by web";
  server.send(200,"text/plain","OK");
}

void handleStatus(){
  if(!wifiEnabled) return;
  String json = "{\"state\":\"" + String(relayState?"ON":"OFF") +
                "\",\"source\":\"" + lastTrigger + "\"}";
  server.send(200,"application/json",json);
}

void handleEnroll(){
  if(!wifiEnabled) return;
  
  resetSensorErrorFlag();
  
  if(isSleep) wakeUp();

  int id = getSafeFreeID();

  if(id < 0){
    enrollMessage = "FULL";
    enrollResult = "FULL";
    server.send(200,"text/plain","FULL");
    return;
  }

  enrollID = id;
  enrollState = ENROLL_WAIT_FINGER;
  enrollMessage = "Tempelkan jari ke sensor - ID: " + String(enrollID);
  enrollResult = "PROCESS";
  enrollTimer = millis();

  for(int i=0;i<4;i++){
    beep(60);
    delay(80);
  }

  server.send(200,"text/plain","OK");
}

void handleEnrollStatus(){
  if(!wifiEnabled) return;
  String json = "{\"msg\":\"" + enrollMessage +
                "\",\"result\":\"" + enrollResult + "\"}";
  server.send(200,"application/json",json);
}

void handleDelete(){
  if(!wifiEnabled) return;
  int id = server.arg("id").toInt();
  finger.deleteModel(id);
  idUsed[id] = false;
  prefs.putBool(("used_"+String(id)).c_str(), false);
  prefs.remove(("id_"+String(id)).c_str());
  fingerNames[id] = "";
  server.send(200,"text/plain","OK");
}

void handleDeleteAll(){
  if(!wifiEnabled) return;
  for(int i=1;i<=MAX_FINGERPRINT_ID;i++){
    finger.deleteModel(i);
    idUsed[i] = false;
    prefs.putBool(("used_"+String(i)).c_str(), false);
    prefs.remove(("id_"+String(i)).c_str());
    fingerNames[i] = "";
  }
  server.send(200,"text/plain","OK");
}

void handleList(){
  if(!wifiEnabled) return;
  String json = "[";
  bool first = true;
  for(int i=1;i<=MAX_FINGERPRINT_ID;i++){
    if(idUsed[i]){
      if(!first) json += ",";
      first = false;
      json += "{\"id\":" + String(i) + ",\"name\":\"" + fingerNames[i] + "\"}";
    }
  }
  json += "]";
  server.send(200,"application/json",json);
}

void handleSetName(){
  if(!wifiEnabled) return;
  int id = server.arg("id").toInt();
  String name = server.arg("name");
  fingerNames[id] = name;
  prefs.putString(("id_"+String(id)).c_str(), name);
  server.send(200,"text/plain","OK");
}

void handleGetWifi(){
  if(!wifiEnabled) return;
  String json = "{\"ssid\":\"" + wifiSsid + "\"}";
  server.send(200,"application/json",json);
}

void handleSetWifi(){
  if(!wifiEnabled) return;
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  
  if(ssid.length() > 0){
    saveWifiCredentials(ssid, pass);
    
    // Set flag agar setelah restart WiFi menyala otomatis
    prefs.putBool("wifi_auto_on", true);
    
    server.send(200,"text/plain","OK");
    delay(1000);
    ESP.restart();
  } else {
    server.send(400,"text/plain","SSID cannot be empty");
  }
}

void handleWifiOff(){
  if(!wifiEnabled){
    server.send(200,"text/plain","Already OFF");
    return;
  }
  
  // Matikan WiFi
  wifiEnabled = false;
  server.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  
  server.send(200,"text/plain","OK");
}

// ================= SETUP =================
void setup(){
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(TOUCH_PIN, INPUT);
  pinMode(WIFI_TOGGLE_PIN, INPUT_PULLUP);

  prefs.begin("fingerDB", false);
  loadNames();
  loadWifiCredentials();

  mySerial.begin(SERIAL_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
  finger.begin(SERIAL_BAUD);
  delay(100);
  
  finger.verifyPassword();

  // KONTROL WIFI BERDASARKAN FLAG
  if(wifiAutoOnAfterRestart){
    // Restart karena ganti WiFi dari web -> WiFi menyala
    wifiEnabled = true;
    setupWiFi();
    server.begin();
    
    // Hapus flag setelah digunakan
    prefs.putBool("wifi_auto_on", false);
    wifiAutoOnAfterRestart = false;
  } else {
    // Default: WiFi MATI
    wifiEnabled = false;
    WiFi.mode(WIFI_OFF);
  }
  
  // Setup web server routes (tetap di-set meskipun wifiEnabled false)
  server.on("/",handleRoot);
  server.on("/relay",handleRelay);
  server.on("/status",handleStatus);
  server.on("/enroll",handleEnroll);
  server.on("/enrollStatus",handleEnrollStatus);
  server.on("/delete",handleDelete);
  server.on("/deleteAll",handleDeleteAll);
  server.on("/list",handleList);
  server.on("/setname",handleSetName);
  server.on("/getWifi",handleGetWifi);
  server.on("/setWifi",handleSetWifi);
  server.on("/wifi/off", handleWifiOff);
  
  // Indikasi setup selesai dengan beep
  beep(100);
  delay(100);
  beep(100);
  
  lastActivity = millis();
}

// ================= LOOP =================
void loop(){
  cekWifiToggleButton();
  
  if(wifiEnabled){
    server.handleClient();
  }

  handleSleep();

  if(isSleep) return;

  processEnroll();

  bool r = digitalRead(TOUCH_PIN);

  if(r && !lastTouchState){
    scanFingerprint();
    lastActivity = millis();
  }

  lastTouchState = r;

  if(millis() - lastActivity > LED_IDLE_TIMEOUT){
    goSleep();
  }
}
