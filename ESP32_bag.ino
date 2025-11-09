#include <WiFi.h>
#include <HTTPClient.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 21
#define RST_PIN 22
#define BUZZER_PIN 5
#define ZIPPER_PIN 4

MFRC522 rfid(SS_PIN, RST_PIN);

// WiFi credentials
const char* ssid = "YourWiFiName";
const char* password = "YourWiFiPassword";

// Telegram Bot Info
String BOT_TOKEN = "YOUR_TELEGRAM_BOT_TOKEN";
String CHAT_ID = "YOUR_TELEGRAM_CHAT_ID"; // get this from @userinfobot

// Authorized RFID UID
byte authorizedUID[4] = {0x63, 0x69, 0xFB, 0x27}; // Your valid card UID

bool accessGranted = false;
unsigned long accessTime = 0;
const unsigned long accessWindow = 5000; // 5 seconds

void sendTelegramMessage(String text) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + BOT_TOKEN +
                 "/sendMessage?chat_id=" + CHAT_ID + "&text=" + text;
    http.begin(url);
    http.GET();
    http.end();
  }
}

void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ZIPPER_PIN, INPUT_PULLUP);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  sendTelegramMessage("👜 Smart Anti-Theft Bag System Activated");
}

void loop() {
  // RFID Detection
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
    if (isAuthorizedUID()) {
      accessGranted = true;
      accessTime = millis();
      digitalWrite(BUZZER_PIN, LOW);
      Serial.println("Access Granted: RFID Verified");
      sendTelegramMessage("✅ RFID Verified - Access Granted for 5 seconds");
    } else {
      Serial.println("Unauthorized RFID Tag Detected!");
      sendTelegramMessage("🚫 Unauthorized RFID Detected!");
      triggerAlarm();
    }
    rfid.PICC_HaltA();
  }

  // Zipper Access Check
  if (digitalRead(ZIPPER_PIN) == LOW) { // assuming LOW means opened
    if (accessGranted && millis() - accessTime < accessWindow) {
      Serial.println("Zipper Access Allowed");
    } else {
      Serial.println("⚠️ Unauthorized Zipper Access Detected!");
      sendTelegramMessage("⚠️ Unauthorized Zipper Access Detected!");
      triggerAlarm();
    }
  }

  // Reset access window after 5 seconds
  if (accessGranted && millis() - accessTime > accessWindow) {
    accessGranted = false;
  }
}

bool isAuthorizedUID() {
  for (byte i = 0; i < 4; i++) {
    if (rfid.uid.uidByte[i] != authorizedUID[i]) return false;
  }
  return true;
}

void triggerAlarm() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(300);
    digitalWrite(BUZZER_PIN, LOW);
    delay(200);
  }
}
