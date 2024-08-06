#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "DHT.h"
#define DHTPIN D2
#define DHTTYPE DHT11

const char* ssid = "S20";        // WiFi name
const char* password = "bixx5003";    // WiFi password
const char* mqtt_server = "mqtt.cloud.kaaiot.com";
const String TOKEN = "esp8266";        // Endpoint token - you get (or specify) it during device provisioning
const String APP_VERSION = "cov2he2c8hds7380365g-v1";  // Application version - you specify it during device provisioning

const unsigned long fiveSeconds = 1 * 5 * 1000UL;
static unsigned long lastPublish = 0 - fiveSeconds;

WiFiClient espClient;
PubSubClient client(espClient);

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

float h_last = 0;
float t_last = 0;

void loop() {
  delay(2000);
  setup_wifi();
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastPublish >= fiveSeconds) {
    lastPublish += fiveSeconds;
    DynamicJsonDocument telemetry(1023);
    telemetry.createNestedObject();

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    float f = dht.readTemperature(true);

    if (isnan(h) || isnan(t) || isnan(f)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      telemetry[0]["temperature"] = t_last;
      telemetry[0]["humidity"] = h_last;
    } else {
      telemetry[0]["temperature"] = t;
      telemetry[0]["humidity"] = h;
      t_last = t;
      h_last = h;
    }
    String topic = "kp1/" + APP_VERSION + "/dcx/" + TOKEN + "/json";
    client.publish(topic.c_str(), telemetry.as<String>().c_str());
    Serial.println("Published on topic: " + topic);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("\nHandling message on topic: %s\n", topic);

  for (int i = 0; i < length; i++) {
    Serial.print((char) payload[i]);
  }

  if (String(topic).endsWith("/command/SWITCH/status")) {
    DynamicJsonDocument doc(1023);
    deserializeJson(doc, payload, length);
    JsonVariant json_var = doc.as<JsonVariant>();

    DynamicJsonDocument commandResponse(1023);
    for (int i = 0; i < json_var.size(); i++) {
      unsigned int command_id = json_var[i]["id"].as<unsigned int>();
      commandResponse.createNestedObject();
      commandResponse[i]["id"] = command_id;
      commandResponse[i]["statusCode"] = 200;
      commandResponse[i]["payload"] = "done";
    }

    String responseTopic = "kp1/" + APP_VERSION + "/cex/" + TOKEN + "/result/SWITCH";
    client.publish(responseTopic.c_str(), commandResponse.as<String>().c_str());
    Serial.println("Published response to SWITCH command on topic: " + responseTopic);
  }
}

void setup_wifi() {
  if (WiFi.status() != WL_CONNECTED) {
    delay(200);
    Serial.println();
    Serial.printf("Connecting to [%s]", ssid);
    WiFi.begin(ssid, password);
    connectWiFi();
  }
}

void connectWiFi() {
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    char *client_id = "client-id-123ab";
    if (client.connect(client_id)) {
      Serial.println("Connected to WiFi");
      // ... and resubscribe
      subscribeToCommand();
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void subscribeToCommand() {
  String topic = "kp1/" + APP_VERSION + "/cex/" + TOKEN + "/command/SWITCH/status";
  client.subscribe(topic.c_str());
  Serial.println("Subscribed on topic: " + topic);
}