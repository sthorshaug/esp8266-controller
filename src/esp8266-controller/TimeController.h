#ifndef TimeController_h
#define TimeController_h
#include <Arduino.h>
class TimeController {
  private:    
    long lastQuery;
    unsigned long lastEpoch;
    unsigned long millisAtEpoch;
    unsigned int wrappedMillis;
    unsigned long lastMillis;
    char formattedTime[50];

    void checkWrappedMillis(unsigned long now);
  public:
    TimeController();
    void setup();
    void loop();
    void queryNtpTime();
    unsigned long currentEpoch();
    int millisSinceEpoch();
};

void initTimeController(bool useNtp);
bool updateUtcTime();
unsigned long getCurrentUtcTime();
#endif
