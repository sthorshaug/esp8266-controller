#ifndef MessageHandler_h
#define MessageHandler_h
#include <Arduino.h>
#include <PubSubClient.h>
#include "TimeController.h"
#include "myconstants.h"
#ifdef EXTLIB_DHT22
#include "DHT.h"
#endif

class MessageHandler {
  public:
    enum PinConfig {
      PINCONFIG_None,
      PINCONFIG_DI,
      PINCONFIG_DO,
      PINCONFIG_AI,
      PINCONFIG_AO
#ifdef EXTLIB_DHT22
     ,PINCONFIG_DHT22
#endif
    };
  private:    
    enum MyRequestType {
      REQ_None,
      REQ_ToggleOnOff,
      REQ_ReadDht22
    };
    struct MyRequest {
      MyRequestType req;
      int           pin;
      int           waittime;
    };
    struct MyIOs {
      bool active;
      PinConfig config;
    };
    
    PubSubClient *mqtt;
    const char *mqttBaseTopic;
    MyIOs myIOs[MAX_PINNUMBER+1];
    TimeController *timeController;
#ifdef EXTLIB_DHT22
    DHT *dht22[MAX_PINNUMBER+1];
#endif
    
    bool decodeRequest(char* requestAsString, MessageHandler::MyRequest *parsed);
    MyRequestType decodeRequestType(const char *req);
    bool runToggleOnOff(MessageHandler::MyRequest *req, char *text);
    bool runReadDht22(MessageHandler::MyRequest *req, char *text, char *jsonValue);
    void sendMqttResponse(MessageHandler::MyRequest *req, bool status, const char *text, const char *jsonValues);
    bool checkPinConfig(int pin, MessageHandler::PinConfig config);
  public:
    MessageHandler(PubSubClient *mqtt, const char* mqttBaseTopic, TimeController *timeController);
    void setup();
    bool assignPinConfiguration(int pin, MessageHandler::PinConfig config);
    void handleRequest(char* topic, byte* payloadAsBytes, unsigned int length);
    void flashLed(int ledPin, int numberOfTimes, int waitTime);
};
#endif
