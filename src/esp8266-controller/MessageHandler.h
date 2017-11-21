#ifndef MessageHandler_h
#define MessageHandler_h
#include <Arduino.h>
#include <PubSubClient.h>
class MessageHandler {
  private:    
    PubSubClient mqtt;

    
  public:
    MessageHandler(PubSubClient mqtt);
    void setup();
    void handleRequest(char* topic, byte* payload, unsigned int length);
};
#endif
