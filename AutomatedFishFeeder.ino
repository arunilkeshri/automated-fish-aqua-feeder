#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Servo.h>

// WiFi credentials (replace with your actual values)
const char* ssid = "Your_SSID";
const char* password = "Your_PASSWORD";

// Telegram credentials
const char* botToken = "Your_BOT_TOKEN";
const char* chatId   = "Your_CHAT_ID";

// Servo setup
Servo feederServo;
const int servoPin = D4;
const int ledPin = LED_BUILTIN;

// Feeding times (24-hour format)
const int feedHour1 = 9;
const int feedMinute1 = 0;

const int feedHour2 = 18;
const int feedMinute2 = 0;

bool hasFedMorning = false;
bool hasFedEvening = false;

// NTP Client Setup
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800, 60000); // IST timezone (UTC + 5:30)

// Early LED Off
__attribute__((constructor)) void earlyInit() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // LED off (active LOW)
}

// Function to send Telegram message
void sendTelegramMessage(String msg) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;

    msg.replace(" ", "%20");
    String url = "https://api.telegram.org/bot" + String(botToken) + "/sendMessage?chat_id=" + String(chatId) + "&text=" + msg;

    http.begin(client, url);
    int httpCode = http.GET();
    http.end();
  }
}

// Feeding action
void feedFish() {
  digitalWrite(ledPin, LOW); // Turn ON LED
  feederServo.write(180);
  delay(500);
  feederServo.write(0);
  delay(500);
  feederServo.write(180);
  delay(500);
  feederServo.write(0);
  delay(500);
  digitalWrite(ledPin, HIGH); // Turn OFF LED
}

// Setup function
void setup() {
  Serial.begin(9600);
  feederServo.attach(servoPin);
  feederServo.write(0);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH); // Ensure LED is off

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  timeClient.begin();
  Serial.println("WiFi connected.");
}

// Loop
void loop() {
  timeClient.update();
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();

  // Feed in morning
  if (currentHour == feedHour1 && currentMinute == feedMinute1 && !hasFedMorning) {
    feedFish();
    sendTelegramMessage("🐟 Morning feeding done!");
    hasFedMorning = true;
  }

  // Feed in evening
  if (currentHour == feedHour2 && currentMinute == feedMinute2 && !hasFedEvening) {
    feedFish();
    sendTelegramMessage("🐟 Evening feeding done!");
    hasFedEvening = true;
  }

  // Reset flags at midnight
  if (currentHour == 0 && currentMinute == 0) {
    hasFedMorning = false;
    hasFedEvening = false;
  }

  delay(30000); // Check every 30 seconds
}
