#include <TinyPICO.h>

#include <WiFi.h>
#include <Wire.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

#define FREEZER_PIN 25
#define LED 5

bool freezerOpen = false;

void blink(int pin, int seconds);
void printLine();
void requestURL(const char * host, uint8_t port);
void connectToWiFi(const char * ssid, const char * pwd);

const char *ssid = "<%= @config[:ssid] %>";
const char *pass = "<%= @config[:pass] %>";
const char *node = "<%= @config[:events][:name] %>";

TinyPICO tp = TinyPICO();

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
  pinMode(FREEZER_PIN, INPUT);

  Serial.println("start up");
  blink(LED, 5);
  digitalWrite(LED, LOW);

  connectToWiFi(ssid, pass);

  digitalWrite(LED, LOW);
  Serial.println("ready");
}

void loop() {
  int freezerDoor = digitalRead(FREEZER_PIN);

  if (freezerDoor == LOW) {
    if (!freezerOpen) {
      Serial.println("freezer open");
      digitalWrite(LED, LOW); // green light off indicating door is open e.g. bad
      freezerOpen = true;
    }
  } else {
    if (freezerOpen) {
      Serial.println("freezer closed");
      digitalWrite(LED, HIGH); // green light on indicating door is closed e.g. good
      freezerOpen = false;
    }
  }
  delay(100);
}

void connectToWiFi(const char * ssid, const char * pwd) {
  int ledState = 0;

  printLine();
  Serial.println("Connecting to WiFi network: " + String(ssid));

  WiFi.begin(ssid, pwd);

  while (WiFi.status() != WL_CONNECTED)
  {
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

void requestURL(const char * host, uint8_t port) {
  printLine();
  Serial.println("Connecting to domain: " + String(host));

  // Use WiFiClient class to create TCP connections
  WiFiClient client;
  if (!client.connect(host, port)) {
    Serial.println("connection failed");
    return;
  }
  Serial.println("Connected!");
  printLine();

  // This will send the request to the server
  client.print((String)"GET / HTTP/1.1\r\n" +
               "Host: " + String(host) + "\r\n" +
               "Connection: close\r\n\r\n");
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
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

void printLine() {
  Serial.println();
  for (int i=0; i<30; i++)
    Serial.print("-");
  Serial.println();
}
