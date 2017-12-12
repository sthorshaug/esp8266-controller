/*
 * MessageHandler
 * Decode and handle incoming MQTT messages
 * 
 * @author Steinar Thorshaug
 */
#include "MessageHandler.h"

/**
 * Constructor
 */
MessageHandler::MessageHandler(PubSubClient *mqtt, const char *mqttBaseTopic, TimeController *timeController, IOHandler *ioHandler) {
  this->mqtt = mqtt;
  this->mqttBaseTopic = mqttBaseTopic;
  this->timeController = timeController;
  this->ioHandler = ioHandler;
}

/*
 * Handles messages
 * <reqtype>;<pin-number>;<delay>
 */
void MessageHandler::handleRequest(char* topic, byte* payloadAsBytes, unsigned int length) {
  char payload[100];
  char dbgOut[100];
  char jsonValues[50];
  MessageHandler::MyRequest request;
  bool status = false;
  jsonValues[0]=0;
  
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

  dbgOut[0] = 0;
  switch(request.req) {
    case REQ_ToggleOnOff:
      status = this->ioHandler->runToggleOnOff(request.pin, request.waittime, dbgOut);
      break;
    case REQ_ReadValues:
      status = this->ioHandler->runReadValues(request.pin, dbgOut, jsonValues);
      break;
    default:
      strcpy(dbgOut, "Unknown request");
  }
  this->sendMqttResponse(&request, status, dbgOut, jsonValues);
  this->ioHandler->flashLed(STATUSLED, status ? 2 : 5, 100);
}

/**
 * Decode a request
 */
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

/**
 * Decode a request type from string
 */
MessageHandler::MyRequestType MessageHandler::decodeRequestType(const char *req) {
  if(strcmp(req, "ToggleOnOff") == 0) {
    return REQ_ToggleOnOff;
  } else if(strcmp(req, "ReadValues") == 0) {
    return REQ_ReadValues;
  }
  return REQ_None;
}


/**
 * Send a status report to the MQTT broker
 */
void MessageHandler::sendMqttResponse(MessageHandler::MyRequest *req, bool status, const char *text, const char *jsonValues) {
  char myString[151];
  char respTopic[20];
  
  snprintf (myString, 150, "{\"time\":%ld,\"req\":%d,\"status\":%s,\"message\":\"%s\"%s}", 
  this->timeController->currentEpoch(), req->req, status ? "true" : "false", text);
  String topic = String(this->mqttBaseTopic);
  snprintf (respTopic, 20, "/response/%d", req->pin);
  topic.concat(respTopic);
  Serial.print("Publish message to ");
  Serial.print(topic.c_str());
  Serial.print(": ");
  Serial.println(myString);    
  if(this->mqtt->publish(topic.c_str(), myString) == 0) {
    Serial.println("MessageHandler: Failed to publish to mqtt. Too long message?");
  }

  if(jsonValues[0] != 0) {
    snprintf (myString, 150, "{\"time\":%ld,%s}", 
    this->timeController->currentEpoch(), jsonValues);
    topic = String(this->mqttBaseTopic);
    snprintf (respTopic, 20, "/values/%d", req->pin);
    topic.concat(respTopic);
    Serial.print("Publish message to ");
    Serial.print(topic.c_str());
    Serial.print(": ");
    Serial.println(myString);    
    if(this->mqtt->publish(topic.c_str(), myString) == 0) {
      Serial.println("MessageHandler: Failed to publish to mqtt. Too long message?");
    }
  }
}


