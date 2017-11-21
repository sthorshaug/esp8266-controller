#include "MessageHandler.h"
#include "myconstants.h"

MessageHandler::MessageHandler(PubSubClient mqtt) {
  this->mqtt = mqtt;
}

void MessageHandler::setup() {
  // todo : Configure outputs
}

void MessageHandler::handleRequest(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String dataReceived;
  for (int i = 0; i < length; i++) {
    dataReceived += (char)payload[i];
  }
  Serial.print(dataReceived.c_str());
  Serial.println();
}

