/*
 * A simple class for keeping the current time
 * Queries a NTP server regulary and keeps track of time since last query
 */
#include "TimeController.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP udp;

/*
 * Wrapper for millis(). Mainly used for wrap after approx 65 secs.
 * Consider to remove at a later stage.
 */
static unsigned long myMillis() {
  return (millis() % 65535);
}

// send an NTP request to the time server at the given address
static unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void printEpoch(unsigned long epoch) {
  Serial.print("Unix time ");
  Serial.print(epoch);
  Serial.print("  ");
  // print the hour, minute and second:
  Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
  Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
  Serial.print(':');
  if ( ((epoch % 3600) / 60) < 10 ) {
    // In the first 10 minutes of each hour, we'll want a leading '0'
    Serial.print('0');
  }
  Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
  Serial.print(':');
  if ( (epoch % 60) < 10 ) {
    // In the first 10 seconds of each minute, we'll want a leading '0'
    Serial.print('0');
  }
  Serial.println(epoch % 60); // print the second
}


class TimeController {
  private:    
    long lastQuery;
    unsigned long lastEpoch;
    unsigned long millisAtEpoch;
    unsigned int wrappedMillis;
    unsigned long lastMillis;
    char formattedTime[50];

    /** 
     *  Check if millisecconds have wrapped since last epoch
     */
    void checkWrappedMillis(unsigned long now) {
      if(this->lastEpoch == 0) {
        return;
      }
      if(now < this->lastMillis) {
        this->wrappedMillis += 1;
        Serial.print("Wrapped again ");
        Serial.println(this->wrappedMillis);
      }
      this->lastMillis = now;
    };

    /**
     * Get number of millisecons since last epoch
     */
    int millisSinceEpoch() {
      unsigned long now = myMillis();  
      this->checkWrappedMillis(now);
      return (int)now-(int)this->millisAtEpoch+(int)this->wrappedMillis*65535;
    };
  public:
    /**
     * Create a timecontroller object
     */
    TimeController() {
      this->wrappedMillis = 0;
      this->lastQuery = 0;
      this->millisAtEpoch = 0;
      this->lastEpoch = 0;
      this->lastMillis = 0;
    };
    
    void setup() {
      Serial.println("Starting UDP for NTP communication");
      udp.begin(localPort);
      Serial.print("Local port: ");
      Serial.println(udp.localPort());
    };

    /**
     * Check if time to request current UTC time
     */
    void loop() {
      unsigned long now = millis();
      if (abs(now - this->lastQuery) > 60000 || this->lastQuery == 0) {
        this->lastQuery = now;
        this->queryNtpTime();
      }
      
      this->checkWrappedMillis(myMillis());
    };

    /**
     * Update current UTC time
     */
    void queryNtpTime() {
      int counter = 0;
      //get a random server from the pool
      WiFi.hostByName(ntpServerName, timeServerIP); 
      udp.flush();
      sendNTPpacket(timeServerIP); // send an NTP packet to a time server
      // wait to see if a reply is available  
      int cb = 0;
      Serial.print("Wait for response");
      while(counter < 10) {
        counter++;
        cb = udp.parsePacket();
        if(!cb) {
          Serial.print(".");
          delay(100);
        } else {
          counter = 100;
        }
      }
      
      if (!cb) {
        Serial.println("no packet yet");
      }
      else {
        Serial.print(" packet received, length=");
        Serial.println(cb);
        // We've received a packet, read the data from it
        udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
    
        //the timestamp starts at byte 40 of the received packet and is four bytes,
        // or two words, long. First, esxtract the two words:
    
        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        // combine the four bytes (two words) into a long integer
        // this is NTP time (seconds since Jan 1 1900):
        unsigned long secsSince1900 = highWord << 16 | lowWord;    
        // now convert NTP time into everyday time:
        // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
        const unsigned long seventyYears = 2208988800UL;
        // subtract seventy years:
        unsigned long epoch = secsSince1900 - seventyYears;
        printEpoch(epoch);
    
        this->lastEpoch = epoch;
        this->millisAtEpoch = myMillis();
        this->wrappedMillis = 0;    
      }
    };

    /**
     * Get current epoch
     */
    unsigned long currentEpoch() {
      if(this->lastEpoch == 0) {
        return 0;
      }
      return (unsigned long)(this->lastEpoch+this->millisSinceEpoch()/1000);
    };
    
};


static TimeController *timecontroller = NULL;
static char timeString[50];

/**
 * Init timecontroller
 */
void initTimeController(bool useNtp) {
  if(useNtp) {
    timecontroller = new TimeController();
    timecontroller->setup();
  }
}

/**
 * Update timecontroller
 * Check for wrapped milliseconds and query NTP if necessary
 */
bool updateTimeController() {
  if(!timecontroller) return false;
  timecontroller->loop();
}

/**
 * Get current UTC time
 */
unsigned long getCurrentUtcTime() {
  if(!timecontroller) return 0;
  return timecontroller->currentEpoch();
}

/**
 * Format current UTC time as a JSON field with comma at the end
 */
char* getCurrentUtcTimeAsJsonField() {
  if(!timecontroller) {
    timeString[0] = 0;
    return timeString;
  }
  snprintf(timeString, 50, "\"time\":%ld,", timecontroller->currentEpoch());
  return timeString;
  return timeString;
}

