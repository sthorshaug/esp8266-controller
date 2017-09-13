/*
 * ESP8266 controller
 * An implemention for controlling and polling an ESP8266 board through mqtt.
 * 
 * @author Steinar Thorshaug
 * @version 1.0.0
 */
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

/*
 * Parameters to change
 * Change these parameters into your values
 */
const char* NETWORK_SSID = "network_ssid";
const char* NETWORK_PASSWORD = "network_password";
const char* MQTT_SERVER = "ip_of_mqtt_server";
const char* MQTT_TOPIC_STATUS_BASE = "topic_to_use_as_base";
const char* MQTT_TOPIC_SUBSCRIBE = "topic_to_use/control/+"; // Subscribe to all sub topics

// The outputs are reversed on my ESP8266
const int OUTPUT_HIGH = LOW;
const int OUTPUT_LOW = HIGH;

// Handy constants
const int STATUSLED = BUILTIN_LED;

// My public variables
WiFiClient wifiClient;
long lastTimeStatusToMqtt = 0;
char genericString[150];
String chipIdAsString;
const char *chipId;

/*
 * Flash a led a given number of times
 */
void flashLed(int ledPin, int numberOfTimes, int waitTime) {
  int i;
  for(i=0; i<numberOfTimes; i++) {
    digitalWrite(ledPin, OUTPUT_HIGH);
    delay(waitTime);
    digitalWrite(ledPin, OUTPUT_LOW);
    if(i<numberOfTimes-1) {
      delay(waitTime);
    }
  }
}

/*
 * Callback for new MQTT data
 */
void mqttDataCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    char receivedChar = (char)payload[i];
    Serial.print(receivedChar);
  }
  Serial.println();
}

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
      // Flash light
      flashLed(STATUSLED, 3, 200);
      delay(4000);
    }
  }
}

void sendAliveMessage(long timeNow) {
  IPAddress myIp = WiFi.localIP();
  snprintf (genericString, 150, "{\"brand\":\"ESP8266\",\"id\":\"%s\",\"time\":%ld,\"rssi\":%ld,\"ip\":\"%d.%d.%d.%d\"}", chipId, timeNow, WiFi.RSSI(), myIp[0],myIp[1],myIp[2],myIp[3]);
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

}

void loop() {
  if (!mqttClient.connected()) {
    mqttReconnect();
  }
  mqttClient.loop();
  long now = millis();
  if (abs(now - lastTimeStatusToMqtt) > 10000) {
    lastTimeStatusToMqtt = now;
    sendAliveMessage(now);
  } else {
    delay(100);
  }
}

