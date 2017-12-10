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
MessageHandler::MessageHandler(PubSubClient *mqtt, const char *mqttBaseTopic, TimeController *timeController) {
  this->mqtt = mqtt;
  this->mqttBaseTopic = mqttBaseTopic;
  this->timeController = timeController;
  for(int i=0; i<MAX_PINNUMBER; i++) {
    this->myIOs[i].active = false;
#ifdef EXTLIB_DHT22
    this->dht22[i] = NULL;
#endif
  }
}

/**
 * Configure a specific pin
 * Must be performed prior to running
 */
bool MessageHandler::assignPinConfiguration(int pin, MessageHandler::PinConfig config) {
  if(pin < 0 || pin > MAX_PINNUMBER) {
    Serial.print("Invalid pin number ");
    Serial.println(pin);
    return false;
  }
  if(this->myIOs[pin].active) {
    Serial.print("Pin ");
    Serial.print(pin);
    Serial.println(" is already taken");
    return false;
  }
  this->myIOs[pin].active = true;
  this->myIOs[pin].config = config;
}

/**
 * Configure all pins
 */
void MessageHandler::setup() {
  for(int i=0; i<MAX_PINNUMBER; i++) {
    if(!this->myIOs[i].active) continue;
    Serial.print("Setting pin ");
    Serial.print(i);
    switch(this->myIOs[i].config) {
      case PINCONFIG_DO:
        Serial.println(" as output");
        pinMode(i, OUTPUT);
        digitalWrite(i, OUTPUT_LOW);
        break;
      case PINCONFIG_DI:
        Serial.println(" as input");
        pinMode(i, INPUT);
        break;
#ifdef EXTLIB_DHT22
      case PINCONFIG_DHT22:
        Serial.println(" as DHT22");
        this->dht22[i] = new DHT(i, DHT22);
        this->dht22[i]->begin();
        break;
#endif
      default:
        // todo Analog IO not supported yet
        Serial.println(" nothing. Not supported");
        break;
    }
  }
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
      status = this->runToggleOnOff(&request, dbgOut);
      break;
    case REQ_ReadDht22:
#ifdef EXTLIB_DHT22
      status = this->runReadDht22(&request, dbgOut, jsonValues);
#else
      strcpy(dbgOut, "DHT22 support not enabled");
#endif
      break;
    default:
      strcpy(dbgOut, "Unknown request");
  }
  this->sendMqttResponse(&request, status, dbgOut, jsonValues);
  this->flashLed(STATUSLED, status ? 2 : 5, 100);
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
  } else if(strcmp(req, "ReadDht22") == 0) {
    return REQ_ReadDht22;
  }
  return REQ_None;
}

/**
 * Perform a ToggleOnOff command
 */
bool MessageHandler::runToggleOnOff(MessageHandler::MyRequest *req, char *text) {
  if(!this->checkPinConfig(req->pin, PINCONFIG_DO)) {
    strcpy(text, "Pin is not configured for output");
    return false;
  }
  digitalWrite(req->pin, OUTPUT_HIGH);
  delay(req->waittime);
  digitalWrite(req->pin, OUTPUT_LOW);
  strcpy(text, "Success");
  return true;
}

/**
 * Perform an on-demand DHT22 reading
 */
bool MessageHandler::runReadDht22(MessageHandler::MyRequest *req, char *text, char *jsonValue) {
#ifdef EXTLIB_DHT22
  if(!this->checkPinConfig(req->pin, PINCONFIG_DHT22)) {
    strcpy(text, "Pin is not configured for DHT22");
    return false;
  }
  DHT *dht = this->dht22[0];
  float t = dht->readTemperature();
  float h = dht->readHumidity();
  char temperature[10], humidity[10];
  if(isnan(t) || isnan(h)) {
    strcpy(text, "Temperature/Humidity was NaN");
    strcpy(jsonValue, ",\"values\":[]");
    return false;
  }
  dtostrf(t, 5, 1, temperature);
  dtostrf(h, 5, 1, humidity);
  snprintf (jsonValue, 50, ",\"values\":[\"temp\":%s,\"hum\":%s]", temperature, humidity);
  strcpy(text, "");
  return true;
#else
  strcpy(text, "DHT22 not enabled");
  return false;
#endif
}

/**
 * Send a status report to the MQTT broker
 */
void MessageHandler::sendMqttResponse(MessageHandler::MyRequest *req, bool status, const char *text, const char *jsonValues) {
  char myString[151];
  snprintf (myString, 150, "{\"time\":%ld,\"req\":%d,\"pin\":%d,\"waittime\":%d,\"status\":%s,\"message\":\"%s\"%s}", 
  this->timeController->currentEpoch(), req->req, req->pin, req->waittime, status ? "true" : "false", text, jsonValues);
  String topic = String(this->mqttBaseTopic);
  topic.concat("/response");
  Serial.print("Publish message to ");
  Serial.print(topic.c_str());
  Serial.print(": ");
  Serial.println(myString);    
  this->mqtt->publish(topic.c_str(), myString);
}

/**
 * Check if a pin is configured correct
 */
bool MessageHandler::checkPinConfig(int pin, MessageHandler::PinConfig config) {
  if(pin > MAX_PINNUMBER || pin < 0) {
    return false;
  }
  return (this->myIOs[pin].active && this->myIOs[pin].config == config);
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


