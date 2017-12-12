#ifndef MessageHandler_h
#define MessageHandler_h
#include <Arduino.h>
#include <PubSubClient.h>
#include "TimeController.h"
#include "IOHandler.h"

class MessageHandler {
  public:    
    enum MyRequestType {
      REQ_None,
      REQ_ToggleOnOff,
      REQ_ReadValues
    };
    struct MyRequest {
      MyRequestType req;
      int           pin;
      int           waittime;
    };
  
  private:  
    PubSubClient *mqtt;
    const char *mqttBaseTopic;
    TimeController *timeController;
    IOHandler *ioHandler;
    
    bool decodeRequest(char* requestAsString, MessageHandler::MyRequest *parsed);
    MyRequestType decodeRequestType(const char *req);
    void sendMqttResponse(MessageHandler::MyRequest *req, bool status, const char *text, const char *jsonValues);
    
  public:
    MessageHandler(PubSubClient *mqtt, const char* mqttBaseTopic, TimeController *timeController, IOHandler *ioHandler);
    void handleRequest(char* topic, byte* payloadAsBytes, unsigned int length);
    void handleRequest(MessageHandler::MyRequest *req);
};
#endif
