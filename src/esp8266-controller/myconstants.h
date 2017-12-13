#ifndef myconstants_h
#define myconstants_h
#include <Arduino.h>

/* 
 *  Framework version
 */
#define SW_VERSION "1.0.0"

/* 
 * Optional external libraries
 * Uncomment the wanted dependencies
 */
//#define EXTLIB_DHT22 // Requires "Adafruit DHT22" and "Adafruit Unified Sensor"

// The outputs are reversed on my ESP8266
const int OUTPUT_HIGH = LOW;
const int OUTPUT_LOW = HIGH;

// Handy constants
const int STATUSLED = BUILTIN_LED;
const int MAX_PINNUMBER=7; // Largest allowed pinnumber

#endif
