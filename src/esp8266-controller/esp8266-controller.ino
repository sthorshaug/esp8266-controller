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
 * Parameters to change
 * Change these parameters into your values
 */
const char* NETWORK_SSID = "NetworkSSID";
const char* NETWORK_PASSWORD = "NetworkPassword";
const char* MQTT_SERVER = "ipOfMqttServer";
const char* MQTT_TOPIC_STATUS_BASE = "topic_to_use_as_base";
const char* MQTT_TOPIC_SUBSCRIBE = "topic_to_use/control";

const bool USE_NTP = true; // Set to false to not sync to UTC time

/*
 * Our framework
 */
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
IOHandler ioHandler;
MessageHandler messageHandler(&mqttClient, MQTT_TOPIC_STATUS_BASE, &ioHandler);


/**
 * Configure all input and outputs prior to calling messagehandler.setup()
 */
void configurePinIO() {
  // An example of adding a digital out pin
  ioHandler.assignPinConfiguration(4, IOHandler::PINCONFIG_DO);
  
  // An example on adding a DHT22 sensor
#ifdef EXTLIB_DHT22
  ioHandler.assignPinConfiguration(0, IOHandler::PINCONFIG_DHT22);
  
  // Scheduled requests example
  MessageHandler::MyRequest request1;
  request1.req = MessageHandler::MyRequestType::REQ_ReadValues;
  request1.pin = 0;
  request1.waittime = 0;
  messageHandler.addScheduledRequest(&request1, 60000);
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
    clientId.concat(String(ESP.getChipId()));
    // Attempt to connect
    if (mqttClient.connect(clientId.c_str())) {
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
 * Check that WIFI is connected
 * If not connected, wait untill connected
 */
static void wifiReconnect() {
  if(WiFi.status() == WL_CONNECTED) return;
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
}


/**
 * Setup application
 */
void setup() {
  Serial.begin(115200);
  delay(10);

  pinMode(STATUSLED, OUTPUT);
  digitalWrite(STATUSLED, OUTPUT_LOW);

  // Connect to WiFi network
  Serial.println();
  Serial.print("Chip ID ");
  Serial.println(ESP.getChipId());
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(NETWORK_SSID);
  
  WiFi.begin(NETWORK_SSID, NETWORK_PASSWORD);  
  wifiReconnect();
  Serial.println("");
  Serial.println("Start MQTT");
  mqttClient.setServer(MQTT_SERVER, 1883);
  mqttClient.setCallback(mqttDataCallback);
  initTimeController(USE_NTP);
  configurePinIO();
  ioHandler.setup();
}

/**
 * Main loop
 */
void loop() {
  wifiReconnect();
  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  mqttClient.loop();
  updateTimeController();
  messageHandler.loop();
  delay(100); 
}

