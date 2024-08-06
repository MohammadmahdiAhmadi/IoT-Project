#include <ESP8266WiFi.h>
#include <DHT.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

/************************* Pin Definition *********************************/

#define DHTPIN D2
#define LED_PIN LED_BUILTIN  // Define the pin for the built-in LED

String temp;
String hum;

#define DHTTYPE DHT11 // DHT 11
DHT dht(DHTPIN, DHTTYPE);

// Your WiFi credentials. Set password to "" for open networks.
const char *ssid = "ssid";
const char *password = "password";

ESP8266WebServer server(80);

void handleRoot() {
  server.send(200, "text/plain", "hello from esp8266!");
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT); // Set LED pin as output
  WiFi.begin(ssid, password);
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  dht.begin();
  server.on("/", handleRoot);
  server.on("/dht-temp", []() {
    int t = dht.readTemperature();
    temp = String(t);
    server.send(200, "text/plain", temp);
  });

  server.on("/dht-hum", []() {
    int h = dht.readHumidity();
    hum = String(h);
    server.send(200, "text/plain", hum);
  });
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  static unsigned long lastUpdateTime = 0;
  const unsigned long interval = 2000;

  if (millis() - lastUpdateTime >= interval) {
    lastUpdateTime = millis();
    int t = dht.readTemperature();
    int h = dht.readHumidity();
    if (t < 13 || t > 25 || h < 70 || h > 90) {
      Serial.println("Conditions are not optimal for vermicomposting. LED is ON.");
      digitalWrite(D0, HIGH); // Turn LED on
    } else {
      digitalWrite(D0, LOW); // Turn LED off
      Serial.println("Conditions are optimal for vermicomposting. LED is OFF.");
    }
  }
  server.handleClient();
}
