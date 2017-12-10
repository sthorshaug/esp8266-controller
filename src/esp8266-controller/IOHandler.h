#ifndef IOHandler_h
#define IOHandler_h
#include <Arduino.h>
#include "myconstants.h"
#ifdef EXTLIB_DHT22
#include "DHT.h"
#endif

class IOHandler {
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
    struct MyIOs {
      bool active;
      PinConfig config;
    };
    
    IOHandler();
    void setup();
    bool assignPinConfiguration(int pin, IOHandler::PinConfig config);

    void flashLed(int ledPin, int numberOfTimes, int waitTime);
    bool runToggleOnOff(int pin, int waittime, char *text);
    bool runReadDht22(int pin, char *text, char *jsonValue);

  private:
    MyIOs myIOs[MAX_PINNUMBER+1];
#ifdef EXTLIB_DHT22
    DHT *dht22[MAX_PINNUMBER+1];
#endif

    bool checkPinConfig(int pin, IOHandler::PinConfig config);
};

#endif
