# ESP8266-controller
This software is intended as a generic framework for controlling an ESP8266 module.  
The framework does nothing fancy by itself.  

## What does it do?
The framework supports the following:
 * Connect to a WiFi network
 * Sync to UTC time
 * Connect to a MQTT broker
 * Publish alive messages with some usefull(?) information
 * Subscribes to MQTT topics and decodes requests
 * Perform basic IO operations

## How to use
### Modify the code
In the _esp8266-controller.ino_ file, modify the following:  
NETWORK_SSID - Set to your wanted SSID  
NETWORK_PASSWORD - Set to your prefered password  
MQTT_SERVER - IP of MQTT broker to connect to  
MQTT_TOPIC_STATUS_BASE - Base topic this software will publish info to  
MQTT_TOPIC_SUBSCRIBE - Topic to subscribe to  

Modify the function _configurePinIO()_ in the same file. This function configures 
the allowed IO pins and their properties.  

Upload the program using your preferred method.

### Communicate using MQTT
You should shortly after boot (given that the SSID is correct and the application 
is able to connect to the MQTT broker) see a message in the /alive topic with parent 
topic defined by _MQTT_TOPIC_STATUS_BASE_.  

Send a message to the the subscribed MQTT topic defined by _MQTT_TOPIC_SUBSCRIBE_.  
The message syntax is _command;pin;waittime_  
An example is _ToggleOnOff;4;500_.  
This means Toggle pin number 4 on, wait for 500ms, then toggle pin number 4 off.  

The response will be given in the /response topic under the parent defined by 
_MQTT_TOPIC_STATUS_BASE_. The reponse contains the command, a boolean status and 
a string describing what happened.

### Supported commands
This is the currently supported commands. If you miss something, implement or make 
a request :-).

#### ToggleOnOff
Toggle a pin high for a given amount of time.

