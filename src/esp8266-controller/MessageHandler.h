#ifndef MessageHandler_h
#define MessageHandler_h
#include <Arduino.h>
#include <PubSubClient.h>
#include "myconstants.h"


    
class MessageHandler {
  public:
    enum PinConfig {
      PINCONFIG_None,
      PINCONFIG_DI,
      PINCONFIG_DO,
      PINCONFIG_AI,
      PINCONFIG_AO
    };
  private:    
    enum MyRequestType {
      REQ_None,
      REQ_ToggleOnOff
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
    
    PubSubClient mqtt;
    const char *mqttBaseTopic;
    MyIOs myIOs[MAX_PINNUMBER+1];
    
    bool decodeRequest(char* requestAsString, MessageHandler::MyRequest *parsed);
    MyRequestType decodeRequestType(const char *req);
    bool runToggleOnOff(MessageHandler::MyRequest *req, char *text);
    void sendMqttResponse(MessageHandler::MyRequest *req, bool status, const char *text);
    bool checkPinConfig(int pin, MessageHandler::PinConfig config);
  public:
    MessageHandler(PubSubClient mqtt, const char* mqttBaseTopic);
    void setup();
    bool assignPinConfiguration(int pin, MessageHandler::PinConfig config);
    void handleRequest(char* topic, byte* payloadAsBytes, unsigned int length);
    void flashLed(int ledPin, int numberOfTimes, int waitTime);
};
#endif
