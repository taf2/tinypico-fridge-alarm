#include <Arduino.h>
#include <SPI.h>
//#include <TinyPICO.h>

#include <WiFi.h>
#include <Wire.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include "ctm_api_root.h"

#ifdef TINYPICO_PINS
#define FRIDGE_PIN 25
#define FREEZER_PIN 33
#define LED 5
#elif  TINYS3_PINS
#define FRIDGE_PIN 21
#define FREEZER_PIN 6
#define LED 34
#elif PICOW
#define FRIDGE_PIN 21
#define FREEZER_PIN 6
#define LED 20
#endif
#define TIME_TO_ALARM 1000 * 60 * 5 // 5 minutes left open we'll alarm
/*
 * each input pin from the detector pin is connected to FRIDGE_PIN/FREEZER_PIN via a voltage divider from the NO (normally open) pin
 * voltage is (1.5k) -- (our signal pin) -- (5.1k) via our ~5v pin we should see between (4v in) 3.06 and 3.86 (5v in) -
 * making it safe to measure as a HIGH signal
 */

bool fridgeOpen = false;
bool freezerOpen = false;
bool didFridgeAlarm = false;
bool didFreezerAlarm = false;
unsigned long lastOpenTime = 0;
unsigned long lastCloseTime = 0;

void blink(int pin, int seconds);
void postAlarm(const char*msg);
void connectToWiFi(const char * ssid, const char * pwd);

const char *ssid = "<%= @config[:ssid] %>";
const char *pass = "<%= @config[:pass] %>";
const char *node = "<%= @config[:events][:name] %>";

//TinyPICO tp = TinyPICO();

void blink(int pin, int seconds) {
  for (int i = 0; i < seconds; ++i) {
    digitalWrite(pin, HIGH);
    delay(500);
    digitalWrite(pin, LOW);
    delay(500);
  }
}

void setup() {

  Serial.begin(115200);

  pinMode(LED, OUTPUT);
  pinMode(FRIDGE_PIN, INPUT_PULLDOWN);
  pinMode(FREEZER_PIN, INPUT_PULLDOWN);

  Serial.println("start up");
  blink(LED, 5);
  digitalWrite(LED, LOW);

  connectToWiFi(ssid, pass);

  digitalWrite(LED, HIGH);
  Serial.println("ready");
}

void notifyDoorOpen(bool &didAlarm, bool &doorOpen, int doorPin, const char *openMessage, const char *closeMessage) {
  if (doorPin == LOW) {
    if (!doorOpen) {
      lastOpenTime = millis();
      Serial.println(openMessage);
      digitalWrite(LED, LOW); // green light off indicating door is open e.g. bad
      doorOpen = true;
      didAlarm = false;
    } else if (millis() - lastOpenTime > 15000) { // left open for more then given seconds
      if (!didAlarm) {
        didAlarm = true;
        postAlarm(openMessage);
      }
    }
  } else {
    if (doorOpen) {
      lastCloseTime = millis();
      Serial.println(closeMessage);
      digitalWrite(LED, HIGH); // green light on indicating door is closed e.g. good
      doorOpen = false;
      lastOpenTime  = 0;
      if (didAlarm) {
        postAlarm(closeMessage);
      }
      didAlarm = false;
    }
  }
}

void loop() {
  int fridgeDoor = digitalRead(FRIDGE_PIN);
  int freezerDoor = digitalRead(FREEZER_PIN);

  notifyDoorOpen(didFridgeAlarm, fridgeOpen, fridgeDoor, "<%= CGI.escape('Fridge Door is Open') %>", "<%= CGI.escape('Fridge Door is Closed') %>");
  notifyDoorOpen(didFreezerAlarm, freezerOpen, freezerDoor, "<%= CGI.escape('Freezer Door is Open') %>", "<%= CGI.escape('Freezer Door is Closed') %>");

  delay(100);
}

void connectToWiFi(const char * ssid, const char * pwd) {
  int ledState = 0;

  Serial.println("Connecting to WiFi network: " + String(ssid));

  WiFi.begin(ssid, pwd);

  while (WiFi.status() != WL_CONNECTED) {
    // Blink LED while we're connecting:
    digitalWrite(LED, ledState);
    ledState = (ledState + 1) % 2; // Flip ledState
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void postAlarm(const char*msg) {
  WiFiClientSecure client;
  client.setCACert(root_ca);
  if (!client.connect("api.calltrackingmetrics.com", 443)) {
    Serial.println(F("Connection error to ctm"));
    blink(LED, 10);
    return;
  }
  String body;
  String header;

  body = "to=<%= CGI.escape(@config[:ctm][:to]) %>&from=<%= CGI.escape(@config[:ctm][:from]) %>&msg=";
  body += msg;

  header = "POST /api/v1/accounts/<%= @config[:ctm][:act] %>/sms HTTP/1.1\r\nHost: api.calltrackingmetrics.com\r\nContent-Type: application/x-www-form-urlencoded\r\nAuthorization: Basic <%= @config[:ctm][:key] %>\r\nConnection: close\r\nContent-Length:";
  header += body.length();
  header += "\r\n\r\n";
  Serial.print(header);
  Serial.print(body);

  client.print(header);
  client.print(body);

  unsigned long timeout = millis();

  while (client.available() == 0) {
    if (millis() - timeout > 15000) {
      Serial.println(">>> Client Timeout !");
      client.stop();
      return;
    }
  }

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("closing connection");
  client.stop();
}
