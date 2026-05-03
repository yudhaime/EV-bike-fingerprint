#include <Adafruit_Fingerprint.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

#include "config.h"
#include "webdisplay.h"

// ================= SERIAL =================
HardwareSerial mySerial(1);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// ================= WEB =================
WebServer server(80);

// ================= STORAGE =================
Preferences prefs;
String fingerNames[128];
bool idUsed[128];

// ================= STATE =================
bool relayState = false;
bool scanning = false;
String lastTrigger = "-";

// ================= SLEEP =================
bool isSleep = false;
unsigned long lastActivity = 0;

// ================= TOUCH =================
bool touchState = false;
bool lastTouchState = false;
unsigned long lastTouchChange = 0;

// ================= ENROLL =================
enum EnrollState {
  ENROLL_IDLE,
  ENROLL_WAIT_FINGER,
  ENROLL_IMAGE1,
  ENROLL_REMOVE,
  ENROLL_IMAGE2,
  ENROLL_STORE
};

EnrollState enrollState = ENROLL_IDLE;
String enrollMessage = "Menunggu...";
String enrollResult = "IDLE";
int enrollID = -1;
unsigned long enrollTimer = 0;

// ================= BUZZER =================
void beep(int d){
  digitalWrite(BUZZER_PIN, HIGH);
  delay(d);
  digitalWrite(BUZZER_PIN, LOW);
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

// ================= LOAD =================
void loadNames(){
  for(int i=1;i<=MAX_FINGERPRINT_ID;i++){
    fingerNames[i] = prefs.getString(("id_"+String(i)).c_str(), "");
    idUsed[i] = prefs.getBool(("used_"+String(i)).c_str(), false);
  }
}

// ================= COUNT =================
int getUsedCount(){
  int c=0;
  for(int i=1;i<=MAX_FINGERPRINT_ID;i++){
    if(idUsed[i]) c++;
  }
  return c;
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
    return;
  }

  switch(enrollState){

    case ENROLL_WAIT_FINGER:
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

      } else {

        enrollMessage = "GAGAL";
        enrollResult = "FAIL";

        for(int i=0;i<5;i++){
          beep(50);
          delay(60);
        }
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
  ledOff();
  digitalWrite(BUZZER_PIN, LOW);
}

void wakeUp(){
  if(!isSleep) return;

  isSleep = false;
  while(mySerial.available()) mySerial.read();
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

// ================= WEB =================
void handleRoot(){ server.send(200,"text/html",getWebPage()); }

void handleRelay(){
  toggleRelay();
  server.send(200,"text/plain","OK");
}

// 🔥 STATUS (INI YANG DIPAKAI WEB SYNC)
void handleStatus(){
  String json = "{\"state\":\"" + String(relayState?"ON":"OFF") +
                "\",\"source\":\"" + lastTrigger + "\"}";
  server.send(200,"application/json",json);
}

void handleEnroll(){

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
  enrollMessage = "ID: " + String(enrollID);
  enrollResult = "PROCESS";
  enrollTimer = millis();

  for(int i=0;i<4;i++){
    beep(60);
    delay(80);
  }

  server.send(200,"text/plain","OK");
}

void handleEnrollStatus(){
  String json = "{\"msg\":\"" + enrollMessage +
                "\",\"result\":\"" + enrollResult + "\"}";
  server.send(200,"application/json",json);
}

void handleDelete(){
  int id = server.arg("id").toInt();
  finger.deleteModel(id);
  idUsed[id] = false;
  prefs.putBool(("used_"+String(id)).c_str(), false);
  server.send(200,"text/plain","OK");
}

void handleDeleteAll(){
  for(int i=1;i<=MAX_FINGERPRINT_ID;i++){
    finger.deleteModel(i);
    idUsed[i] = false;
    prefs.putBool(("used_"+String(i)).c_str(), false);
  }
  server.send(200,"text/plain","OK");
}

void handleList(){
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
  int id = server.arg("id").toInt();
  String name = server.arg("name");

  fingerNames[id] = name;
  prefs.putString(("id_"+String(id)).c_str(), name);

  server.send(200,"text/plain","OK");
}

// ================= SETUP =================
void setup(){
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(TOUCH_PIN, INPUT);

  prefs.begin("fingerDB", false);
  loadNames();

  mySerial.begin(SERIAL_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
  finger.begin(SERIAL_BAUD);

  WiFi.softAP(WIFI_SSID, WIFI_PASS);

  server.on("/",handleRoot);
  server.on("/relay",handleRelay);
  server.on("/status",handleStatus);
  server.on("/enroll",handleEnroll);
  server.on("/enrollStatus",handleEnrollStatus);
  server.on("/delete",handleDelete);
  server.on("/deleteAll",handleDeleteAll);
  server.on("/list",handleList);
  server.on("/setname",handleSetName);

  server.begin();

  lastActivity = millis();
}

// ================= LOOP =================
void loop(){
  server.handleClient();

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
