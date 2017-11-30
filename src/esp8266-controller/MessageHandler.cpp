#include "MessageHandler.h"
#include "myconstants.h"



MessageHandler::MessageHandler(PubSubClient mqtt, const char *mqttBaseTopic) {
  this->mqtt = mqtt;
  this->mqttBaseTopic = mqttBaseTopic;
}

void MessageHandler::setup() {
  // todo : Configure outputs
}

/*
 * Handles messages
 * <reqtype>;<pin-number>;<delay>
 */
void MessageHandler::handleRequest(char* topic, byte* payloadAsBytes, unsigned int length) {
  char payload[100];
  char dbgOut[100];
  MessageHandler::MyRequest request;
  bool status = false;
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  if(length >= 100) {
    // todo: Report
    Serial.println(" Message is too long");
    return;
  }
  strncpy(payload, (char*)payloadAsBytes, length);
  payload[length] = 0;
  Serial.print(payload);
  Serial.println();
  if(!this->decodeRequest(payload, &request)) {
    return;
  }
  snprintf(dbgOut, 100, "Req %d, pin %d, waittime %d", request.req, request.pin, request.waittime);
  Serial.println(dbgOut);
  if(request.waittime < 0) {
    Serial.println("Negative waittime - aborting request");
    return;
  }
  if(request.waittime > 5000) {
    Serial.println("Waittime changed to 5000ms");
  }

  switch(request.req) {
    case REQ_ToggleOnOff:
      status = this->runToggleOnOff(&request);
      break;
    default:
      this->sendMqttResponse(&request, false, "Unknown request");
  }

  this->flashLed(STATUSLED, status ? 2 : 5, 100);
}

bool MessageHandler::decodeRequest(char* requestAsString, MessageHandler::MyRequest *parsed) {
  char *token = strtok(requestAsString, ";");
  if(!token) {
    Serial.println("No request found");
    return false;
  }
  parsed->req = this->decodeRequestType(token);

  token = strtok(NULL, ";");
  if(!token) {
    Serial.println("No pin number found");
    return false;
  }
  parsed->pin = atoi(token);

  token = strtok(NULL, ";");
  if(!token) {
    Serial.println("No wait time found");
    return false;
  }
  parsed->waittime = atoi(token);
  return true;
}

MessageHandler::MyRequestType MessageHandler::decodeRequestType(const char *req) {
  if(strcmp(req, "ToggleOnOff") == 0) {
    return REQ_ToggleOnOff;
  }
  return REQ_None;
}

bool MessageHandler::runToggleOnOff(MessageHandler::MyRequest *req) {
  return true;
}

void MessageHandler::sendMqttResponse(MessageHandler::MyRequest *req, bool status, const char *text) {
  char myString[151];
  snprintf (myString, 150, "{\"req\":%d,\"pin\":%d,\"waittime\":%d,\"status\":%s,\"message\":\"%s\"}", 
  req->req, req->pin, req->waittime, status ? "true" : "false", text);
  String topic = String(this->mqttBaseTopic);
  topic.concat("/response");
  Serial.print("Publish message to ");
  Serial.print(topic.c_str());
  Serial.print(": ");
  Serial.println(myString);    
  this->mqtt.publish(topic.c_str(), myString);
}

/*
 * Flash a led a given number of times
 */
void MessageHandler::flashLed(int ledPin, int numberOfTimes, int waitTime) {
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

