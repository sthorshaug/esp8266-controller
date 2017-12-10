/*
 * ESP8266 controller
 * An implemention for controlling and polling an ESP8266 board through mqtt.
 * 
 * @author Steinar Thorshaug
 */
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "myconstants.h"
#include "TimeController.h"
#include "MessageHandler.h"
#include "IOHandler.h"

/*
 * Constants
 */
const char* SW_VERSION = "1.0.0.a1";

/*
 * Parameters to change
 * Change these parameters into your values
 */
const char* NETWORK_SSID = "NetworkSSID";
const char* NETWORK_PASSWORD = "NetworkPassword";
const char* MQTT_SERVER = "ipOfMqttServer";
const char* MQTT_TOPIC_STATUS_BASE = "topic_to_use_as_base";
const char* MQTT_TOPIC_SUBSCRIBE = "topic_to_use/control/+"; // Subscribe to all sub topics

/*
 * Our framework
 */
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
TimeController timeController;
IOHandler ioHandler;
MessageHandler messageHandler(&mqttClient, MQTT_TOPIC_STATUS_BASE, &timeController, &ioHandler);
long lastTimeStatusToMqtt = 0;
char genericString[150];
String chipIdAsString;
const char *chipId;

/**
 * Configure all input and outputs prior to calling messagehandler.setup()
 */
void configurePinIO() {
  // An example of adding a digital out pin
  ioHandler.assignPinConfiguration(4, IOHandler::PINCONFIG_DO);
  
  // An example on adding a DHT22 sensor
 #ifdef EXTLIB_DHT22
  ioHandler.assignPinConfiguration(0, IOHandler::PINCONFIG_DHT22);
  #endif
}

/*
 * Callback for new MQTT data
 */
void mqttDataCallback(char* topic, byte* payload, unsigned int length) {
  messageHandler.handleRequest(topic, payload, length);
}

/*
 * Assert that we are connected to the MQTT broker
 */
void mqttReconnect() {
  // Loop until we're reconnected
  while (!mqttClient.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = String("ESP8266 ");
    clientId.concat(chipId);
    // Attempt to connect
    if (mqttClient.connect("ESP8266 Client")) {
      Serial.println("connected");      
      mqttClient.subscribe(MQTT_TOPIC_SUBSCRIBE);
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Flash onboard LED
      ioHandler.flashLed(STATUSLED, 3, 200);
      delay(4000);
    }
  }
}

/**
 * Format and send an alive message to the MQTT broker
 */
void sendAliveMessage(long timeNow) {
  IPAddress myIp = WiFi.localIP();
  snprintf (genericString, 150, "{\"brand\":\"ESP8266\",\"id\":\"%s\",\"time\":%ld,\"rssi\":%ld,\"ip\":\"%d.%d.%d.%d\",\"version\":\"%s\"}", 
    chipId, timeController.currentEpoch(), WiFi.RSSI(), myIp[0], myIp[1], myIp[2], myIp[3], SW_VERSION);
  String topic = String(MQTT_TOPIC_STATUS_BASE);
  topic.concat("/alive");
  Serial.print("Publish message to ");
  Serial.print(topic.c_str());
  Serial.print(": ");
  Serial.println(genericString);    
  mqttClient.publish(topic.c_str(), genericString);
  digitalWrite(STATUSLED, OUTPUT_HIGH);
  delay(100);
  digitalWrite(STATUSLED, OUTPUT_LOW);
}

/**
 * Setup application
 */
void setup() {
  Serial.begin(115200);
  delay(10);

  chipIdAsString = String(ESP.getChipId());
  chipId = chipIdAsString.c_str();

  pinMode(STATUSLED, OUTPUT);
  digitalWrite(STATUSLED, OUTPUT_LOW);

  // Connect to WiFi network
  Serial.println();
  Serial.print("Chip ID ");
  Serial.println(chipId);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(NETWORK_SSID);
  
  WiFi.begin(NETWORK_SSID, NETWORK_PASSWORD);  
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(STATUSLED, OUTPUT_HIGH);
    delay(250);
    digitalWrite(STATUSLED, OUTPUT_LOW);
    delay(250);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("My ip is ");
  Serial.println(WiFi.localIP());
  Serial.println("");
  Serial.println("Start MQTT");
  mqttClient.setServer(MQTT_SERVER, 1883);
  mqttClient.setCallback(mqttDataCallback);
  timeController.setup();
  configurePinIO();
  ioHandler.setup();
}

/**
 * Main loop
 */
void loop() {
  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  mqttClient.loop();
  timeController.loop();
  long now = millis();
  if ((abs(now - lastTimeStatusToMqtt) > 30000) || (lastTimeStatusToMqtt == 0)) {
    lastTimeStatusToMqtt = now;
    sendAliveMessage(now);
  } else {
    delay(100);
  }  
}

