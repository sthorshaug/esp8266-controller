#include "MessageHandler.h"
#include "myconstants.h"



MessageHandler::MessageHandler(PubSubClient mqtt) {
  this->mqtt = mqtt;
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

