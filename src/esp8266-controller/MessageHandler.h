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
    struct ScheduledItem {
      bool          active;
      MyRequest     req;
      unsigned long lastExecuted;
      unsigned long interval;
    };
  
    PubSubClient *mqtt;
    const char *mqttBaseTopic;
    IOHandler *ioHandler;
    ScheduledItem schedules[10];
    int activeSchedules;
    
    
    bool decodeRequest(char* requestAsString, MessageHandler::MyRequest *parsed);
    MyRequestType decodeRequestType(const char *req);
    void sendMqttResponse(MessageHandler::MyRequest *req, bool status, const char *text, const char *jsonValues);
    
  public:
    MessageHandler(PubSubClient *mqtt, const char* mqttBaseTopic, IOHandler *ioHandler);
    void handleRequest(char* topic, byte* payloadAsBytes, unsigned int length);
    void handleRequest(MessageHandler::MyRequest *req);
    bool addScheduledRequest(MyRequest *req, unsigned long interval);
    bool executeSchedulesRequests();
};
#endif
