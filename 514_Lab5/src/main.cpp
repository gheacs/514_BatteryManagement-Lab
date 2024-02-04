#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

const char* ssid = "UW MPSK";
const char* password = ".9=gS.V*A{";
#define DATABASE_URL "https://esp32-firebase-demo-68643-default-rtdb.firebaseio.com/"
#define API_KEY "AIzaSyCaQxBnfSCdqZDxoAHxr8-ez0Dhgxzpa44"

// Define different transmission rates in milliseconds
int uploadIntervals[] = {1000};
int currentIntervalIndex = 0;

int uploadInterval = uploadIntervals[currentIntervalIndex];

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

const int trigPin = D1;
const int echoPin = D0;
const float soundSpeed = 0.034;

// Function prototypes
void connectToWiFi();
void initFirebase();
float measureDistance();
void sendDataToFirebase(float distance);

void setup() {
  Serial.begin(115200);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  unsigned long startTime = millis();
  while (millis() - startTime < 12000) {
    delay(100);
  }

  connectToWiFi();
  
  startTime = millis();
  while (millis() - startTime < 12000) {
    measureDistance();
    delay(100);
  }

  initFirebase();
  
  startTime = millis();
  while (millis() - startTime < 12000) {
    float currentDistance = measureDistance();
    sendDataToFirebase(currentDistance);
    delay(100);
  }

  WiFi.disconnect();
  esp_sleep_enable_timer_wakeup(12000 * 1000);
  esp_deep_sleep_start();
}

void loop() {
  // This is not used
}

float measureDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * soundSpeed / 2;

  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  return distance;
}

void connectToWiFi() {
  Serial.println(WiFi.macAddress());
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  int wifiCnt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    wifiCnt++;
    if (wifiCnt > 5) {
      Serial.println("WiFi connection failed");
      ESP.restart();
    }
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void initFirebase() {
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  config.token_status_callback = tokenStatusCallback;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectNetwork(true);
}

void sendDataToFirebase(float distance) {
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > uploadInterval || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    if (Firebase.RTDB.pushFloat(&fbdo, "test/distance", distance)) {
      Serial.println("PASSED");
      Serial.print("PATH: ");
      Serial.println(fbdo.dataPath());
      Serial.print("TYPE: ");
      Serial.println(fbdo.dataType());
    } else {
      Serial.println("FAILED");
      Serial.print("REASON: ");
      Serial.println(fbdo.errorReason());
    }
    count++;

    // Move to the next interval or reset to the beginning
    currentIntervalIndex = (currentIntervalIndex + 1) % 5;
    uploadInterval = uploadIntervals[currentIntervalIndex];
  }
}
