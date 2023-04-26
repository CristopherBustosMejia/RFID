//Librerias
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Firebase_ESP_Client.h>
#include <HTTPClient.h>
#include <addons/TokenHelper.h>

//Pins
#define RXD2 16
#define TXD2 17
#define LEDO 5
#define LEDT 18
#define LEDRO 23
#define LEDRT 22

//Firestore Properties
#define FIREBASE_PROJECT_ID "asistenciaccai"
#define API_KEY "AIzaSyBXJKbmPwjP4NukCt7Jyn-QRxeQaLq_KOI"
#define USER_EMAIL "eder.joel55@gmail.com"
#define USER_PASSWORD "Mailo100."

byte threshold = 40;
bool touchActive = false;
bool lastTouchActive = false;
bool testingLower = true;
const char* ssid = "Wikipedia";
const char* pass = "AmmiWangA";
String Text = "", documentPath, auxText = "";
int num, day;
uint32_t epoch, refresh = 0, LedTm = 0;
char C;
bool Flag = true, FlagL = true;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

HTTPClient http;

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600 , SERIAL_8N1, RXD2, TXD2);
  pinMode(LEDO, OUTPUT);
  pinMode(LEDT, OUTPUT);
  pinMode(LEDRO, OUTPUT);
  pinMode(LEDRT, OUTPUT);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  touchAttachInterrupt(4, gotTouchEvent, threshold);
  touchInterruptSetThresholdDirection(testingLower);
  timeClient.begin();
  timeClient.setTimeOffset(0);
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback = tokenStatusCallback;
  fbdo.setResponseSize(2048);
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Conectado");
  digitalWrite(LEDT, LOW);
  digitalWrite(LEDO, LOW);

}

void loop() {
  if(millis()-refresh > 5000){
    if(Flag){
      digitalWrite(LEDRO,HIGH);
    }else{
      digitalWrite(LEDRT,HIGH);
    }
    while(Serial2.available()){
      digitalWrite(LEDO,HIGH);
      C = Serial2.read();
      if(Text.length() >= 13){
        if(Text == auxText){
          digitalWrite(LEDO,LOW);
          Text = "";
        }
        break;
      }else{
        Text += C;
      }
    }
  }
  if(Text != "" && !Serial2.available()){
    digitalWrite(LEDRO,LOW);
    digitalWrite(LEDRT,LOW);
    auxText = Text;
    FirebaseJson content;
    UpdateEpoch();
    epoch = timeClient.getEpochTime();
    num = epoch - 21600;
    day = num / 86400;
    Serial.println("Day has been updated. Current: day " + String(day));
    Serial.println(num);
    if (!Flag) {
      documentPath = "Epoch/" + String(num);
      content.set("fields/Salida/stringValue", Text);
      UpdateData(Text, content);
      digitalWrite(LEDO, LOW);
      Flag = false;
      Serial.println("Completado");
    } else {
      documentPath = "Epoch/" + String(num);
      content.set("fields/Entrada/stringValue", Text);
      UpdateData(Text, content);
      digitalWrite(LEDO , LOW);
      Serial.println("Completado");
    }
    refresh = millis();
    Text = "";
  }
  if (lastTouchActive != touchActive) {
    lastTouchActive = touchActive;
    if (touchActive) {
      if (Flag) {
        Flag = false;
      } else {
        Flag = true;
      }
    }
  }
  if(millis()-LedTm > 500){
    if(FlagL){
      digitalWrite(LEDT,HIGH);
      FlagL = false;
    }else{
      digitalWrite(LEDT,LOW);
      FlagL = true;
    }
    LedTm = millis();
  }
}

void UpdateData(String id, FirebaseJson content) {
  struct fb_esp_firestore_document_write_t update_write;
  std::vector<struct fb_esp_firestore_document_write_t> writes;
  update_write.type = fb_esp_firestore_document_write_type_update;
  update_write.update_document_content = content.raw();
  update_write.update_document_path = documentPath.c_str();
  writes.push_back(update_write);
  if (Firebase.Firestore.commitDocument(&fbdo, FIREBASE_PROJECT_ID, "", writes, "" )) {
    Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
  }
  else {
    UpdateData(id, content);
  }
}

void UpdateEpoch() {
  delay(1000);
  if (!timeClient.update()) {
    timeClient.update();
    Serial.println("Time client updated");
  }
  epoch = timeClient.getEpochTime();
  if (epoch < 1600000000) {
    digitalWrite(LEDT, HIGH);
    delay(2000);
    ESP.restart();
  }
}

void gotTouchEvent() {
  if (lastTouchActive != testingLower) {
    touchActive = !touchActive;
    testingLower = !testingLower;
    // Touch ISR will be inverted: Lower <--> Higher than the Threshold after ISR event is noticed
    touchInterruptSetThresholdDirection(testingLower);
  }
}
