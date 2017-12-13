/*
 * MessageHandler
 * Decode and handle incoming MQTT messages
 * 
 * @author Steinar Thorshaug
 */
#include <ESP8266WiFi.h>
#include "MessageHandler.h"
#include "TimeController.h"

static char genericString[151];

/**
 * Constructor
 */
MessageHandler::MessageHandler(PubSubClient *mqtt, const char *mqttBaseTopic, IOHandler *ioHandler) {
  this->mqtt = mqtt;
  this->mqttBaseTopic = mqttBaseTopic;
  this->ioHandler = ioHandler;
  for(int i=0; i<10; i++) {
    this->schedules[i].active = false;
  }
  this->activeSchedules = 0;
}

/**
 * Add a scheduled repeated request
 */
bool MessageHandler::addScheduledRequest(MyRequest *req, unsigned long interval) {
  if(this->activeSchedules >= 10) {
    return false;
  }
  ScheduledItem *item = &this->schedules[this->activeSchedules];
  this->activeSchedules++;
  item->req = *req;
  item->active = true;
  item->interval = interval;
  item->lastExecuted = 0;
}

/**
 * Check if any scheduled requests are pending
 */
bool MessageHandler::executeScheduledRequests() {
  ScheduledItem *item = NULL;
  bool returnval = false;
  unsigned long timenow = millis();
  
  for(int i=0; i<this->activeSchedules; i++) {
    item = &this->schedules[i];
    if(!item->active) continue;
    if(abs(timenow - item->lastExecuted) > item->interval) {
      this->handleRequest(&item->req);
      returnval = true;
      item->lastExecuted = timenow;
    }
  }

  return returnval;
}

/**
 * Format and send an alive message to the MQTT broker
 */
void MessageHandler::sendAliveMessage() {
  IPAddress myIp = WiFi.localIP();
  snprintf (genericString, 150, "{%s\"rssi\":%ld,\"ip\":\"%d.%d.%d.%d\"}", 
    getCurrentUtcTimeAsJsonField(), WiFi.RSSI(), myIp[0], myIp[1], myIp[2], myIp[3]);
  String topic = String(this->mqttBaseTopic);
  topic.concat("/alive");
  Serial.print("Publish message to ");
  Serial.print(topic.c_str());
  Serial.print(": ");
  Serial.println(genericString);    
  if(this->mqtt->publish(topic.c_str(), genericString) == 0) {
    Serial.println("Failed to publish alive message to MQTT. Too long message?");
  }
  digitalWrite(STATUSLED, OUTPUT_HIGH);
  delay(100);
  digitalWrite(STATUSLED, OUTPUT_LOW);
}

/**
 * Send about-message to MQTT broker
 */
void MessageHandler::sendAboutMessage() {
  snprintf (genericString, 150, "{\"brand\":\"ESP8266\",\"id\":\"%ld\",\"version\":\"%s\"}", 
    /*chipId*/ESP.getChipId(), SW_VERSION);
  String topic = String(this->mqttBaseTopic);
  topic.concat("/about");
  Serial.print("Publish message to ");
  Serial.print(topic.c_str());
  Serial.print(": ");
  Serial.println(genericString);    
  if(this->mqtt->publish(topic.c_str(), genericString) == 0) {
    Serial.println("Failed to publish to MQTT. Too long message?");
  }
}

/*
 * Handles messages
 * <reqtype>;<pin-number>;<delay>
 */
void MessageHandler::handleRequest(char* topic, byte* payloadAsBytes, unsigned int length) {
  char payload[100];
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
  this->handleRequest(&request);
  
}

/**
 * Handle a decoded request
 */
void MessageHandler::handleRequest(MessageHandler::MyRequest *req) {
  char dbgOut[100];
  char jsonValues[50];
  bool status = false;
  
  jsonValues[0]=0;
  snprintf(dbgOut, 100, "Req %d, pin %d, waittime %d", req->req, req->pin, req->waittime);
  Serial.println(dbgOut);
  if(req->waittime < 0) {
    Serial.println("Negative waittime - aborting request");
    return;
  }
  if(req->waittime > 5000) {
    Serial.println("Waittime changed to 5000ms");
  }

  dbgOut[0] = 0;
  switch(req->req) {
    case REQ_ToggleOnOff:
      status = this->ioHandler->runToggleOnOff(req->pin, req->waittime, dbgOut);
      break;
    case REQ_ReadValues:
      status = this->ioHandler->runReadValues(req->pin, dbgOut, jsonValues);
      break;
    default:
      strcpy(dbgOut, "Unknown request");
  }
  this->sendMqttResponse(req, status, dbgOut, jsonValues);
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
  char respTopic[20];
  
  snprintf (genericString, 150, "{%s\"req\":%d,\"status\":%s,\"message\":\"%s\"}", 
    getCurrentUtcTimeAsJsonField(), req->req, status ? "true" : "false", text);
  String topic = String(this->mqttBaseTopic);
  snprintf (respTopic, 20, "/response/%d", req->pin);
  topic.concat(respTopic);
  Serial.print("Publish message to ");
  Serial.print(topic.c_str());
  Serial.print(": ");
  Serial.println(genericString);    
  if(this->mqtt->publish(topic.c_str(), genericString) == 0) {
    Serial.println("MessageHandler: Failed to publish to mqtt. Too long message?");
  }

  if(jsonValues[0] != 0) {
    snprintf (genericString, 150, "{\"time\":%ld,%s}", getCurrentUtcTime(), jsonValues);
    topic = String(this->mqttBaseTopic);
    snprintf (respTopic, 20, "/values/%d", req->pin);
    topic.concat(respTopic);
    Serial.print("Publish message to ");
    Serial.print(topic.c_str());
    Serial.print(": ");
    Serial.println(genericString);    
    if(this->mqtt->publish(topic.c_str(), genericString) == 0) {
      Serial.println("MessageHandler: Failed to publish to mqtt. Too long message?");
    }
  }
}

/**
 * Loop function
 */
void MessageHandler::loop() {
  static long lastTimeStatusToMqtt = 0;
  long now = millis();
  if ((abs(now - lastTimeStatusToMqtt) > 30000) || (lastTimeStatusToMqtt == 0)) {
    static int aboutCounter = 10;
    lastTimeStatusToMqtt = now;
    this->sendAliveMessage();
    aboutCounter++;
    if(aboutCounter >= 10) {
      aboutCounter = 0;
      this->sendAboutMessage();
    }
  }

  /* Check if time to send something */
  this->executeScheduledRequests();
}

