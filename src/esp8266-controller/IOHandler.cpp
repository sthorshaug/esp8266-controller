#include "IOHandler.h"

IOHandler::IOHandler() {
  // Initialize values
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
bool IOHandler::assignPinConfiguration(int pin, IOHandler::PinConfig config) {
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
void IOHandler::setup() {
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
 * Flash a led a given number of times
 */
void IOHandler::flashLed(int ledPin, int numberOfTimes, int waitTime) {
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

/**
 * Perform a ToggleOnOff command
 */
bool IOHandler::runToggleOnOff(int pin, int waittime, char *text) {
  if(!this->checkPinConfig(pin, PINCONFIG_DO)) {
    strcpy(text, "Pin is not configured for output");
    return false;
  }
  digitalWrite(pin, OUTPUT_HIGH);
  delay(waittime);
  digitalWrite(pin, OUTPUT_LOW);
  strcpy(text, "");
  return true;
}

/**
 * Perform a read values command
 */
bool IOHandler::runReadValues(int pin, char *text, char *jsonValue) {
  if(this->checkPinConfig(pin, PINCONFIG_DI)) {
    strcpy(text, "DI reading not supported");
    return false;
  }
#ifdef EXTLIB_DHT22
  else if(this->checkPinConfig(pin, PINCONFIG_DHT22)) {
    return this->readDht22(pin, text, jsonValue);
  }
#endif
  else {
    strcpy(text, "Pin does not support readings");
    return false;
  }
}

/**
 * Perform an on-demand DHT22 reading
 */
bool IOHandler::readDht22(int pin, char *text, char *jsonValue) {
#ifdef EXTLIB_DHT22
  if(!this->checkPinConfig(pin, PINCONFIG_DHT22)) {
    strcpy(text, "Pin is not configured for DHT22");
    return false;
  }
  DHT *dht = this->dht22[pin];
  float t = dht->readTemperature();
  float h = dht->readHumidity();
  char temperature[10], humidity[10];
  if(isnan(t) || isnan(h)) {
    strcpy(text, "Temperature/Humidity was NaN");
    strcpy(jsonValue, "");
    return false;
  }
  dtostrf(t, 5, 1, temperature);
  dtostrf(h, 5, 1, humidity);
  snprintf (jsonValue, 50, "\"temp\":%s,\"hum\":%s", temperature, humidity);
  strcpy(text, "");
  return true;
#else
  strcpy(text, "DHT22 not enabled");
  return false;
#endif
}

/**
 * Check if a pin is configured correct
 */
bool IOHandler::checkPinConfig(int pin, IOHandler::PinConfig config) {
  if(pin > MAX_PINNUMBER || pin < 0) {
    return false;
  }
  return (this->myIOs[pin].active && this->myIOs[pin].config == config);
}
