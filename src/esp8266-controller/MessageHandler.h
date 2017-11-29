#ifndef MessageHandler_h
#define MessageHandler_h
#include <Arduino.h>
#include <PubSubClient.h>


    
class MessageHandler {
  private:    
    PubSubClient mqtt;

    enum MyRequestType {
      REQ_None,
      REQ_ToggleOnOff
    };
    struct MyRequest {
      MyRequestType req;
      int           pin;
      int           waittime;
    };
    bool decodeRequest(char* requestAsString, MessageHandler::MyRequest *parsed);
    MyRequestType decodeRequestType(const char *req);
  public:
    MessageHandler(PubSubClient mqtt);
    void setup();
    void handleRequest(char* topic, byte* payloadAsBytes, unsigned int length);
};
#endif
