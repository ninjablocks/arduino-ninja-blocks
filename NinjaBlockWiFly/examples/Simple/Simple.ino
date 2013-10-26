/*
  NinjaBlock Basic Comms Example - @ozziegurkan
  
  This sketch uses the NinjaBlockWiFly library to create a Tri-State RF433 actuator
  and a simple led actuator. This is awesome because it gives your Arduino project a
  REST interface in a couple of minutes.

  It utilizes Wi-Fly card by Roving Networks supplied by Sparkfun.com through the use of
  SoftwareSerial library built into Arduino. 
  
  Ninja Params
  ------------
  
  You need a Ninja Blocks account to use the library, use the invite code 1012BB to signup
  at https://a.ninja.is if you don't have one. 

  For reference the NinjaBlock params are:
  
  token    For hacking use the virtual block token from https://a.ninja.is/hacking , there
           are other ways to get a token but hard to squeeze into a library for a 328.
  nodeID   This is the board's ID. Can be any 12+char string. It's used in REST so it's a
           good idea to make it easy to remember, e.g. ARDUINOBLOCK.
  vendorID Use 0 with the device IDs here http://ninjablocks.com/docs/device-ids and 
           you get pretty widgets for "free". Create your own by pinging help@ninjablocks.com.
  deviceID Identifies each device and maps to UI elements and properties that allow other apps
           to interact with your devices without knowing about them, see link above.
  guid     Think of this as port - it is used to differentiate between multiple devices of
           the same type.
     
  How to use this example
  -----------------------
  
  1) I've tested this with a RN-XV Wi-Fly module from Sparkfun. I am using the 433 transmitter
  from Seeed Studio. 

      Connect the RX pin of the Arduino to the TX pin of the 433 transmitter.
      Connect the anode (long lead, +ve) of a LED to pin 7, and connect that LED's cathode (short lead, -ve) to GND through a 330R-1K resistor. 

  2) Copy the NinjaBlockWiFly library into your Arduino libraries dir. 
  
  3) Upload and plug in the intwertubes
  
  4) Your OnBoard RGB LED and RF433 widgets will show up automatically. Turn your led on by 
  clicking on white and off by clicking on black. "Listen" to 433 signals by clicking on your remote
  and then send it back down to trigger something.

  5) The real fun is using REST interface, e.g. turn your light on with
  
  curl -H 'Content-Type: application/json' \
       -X 'PUT' \
       -d '{ "DA" : "FFFFFF" }' \
       -i https://api.ninja.is/rest/v0/device/ARDUINOBLOCK_0_0_1000?user_access_token=YOURTOKEN
  
  NB: Access tokens are not the same as block tokens, get yours from https://a.ninja.is/hacking

  You can also add a callback for your button that pings a uri of your choice whenever its 
  pressed, or create a rule that sends an sms, posts to facebook, hits a webhook or whatever.
  I hope you do something interesting, be sure and let me know in the forums 
  http://ninjablocks.com/forums

*/

#include "NinjaBlockWiFly.h"
#include "RCSwitch.h"
#include "WiFlyHQ.h"
#include <SoftwareSerial.h>

#undef DEBUG_SERIAL
#define DEBUG_SERIAL

#ifdef DEBUG_SERIAL
#define DSBEGIN() Serial.begin(57600)
#define DPRINT(item) Serial.print(item)
#define DPRINTLN(item) Serial.println(item)
#else
#define DSBEGIN()
#define DPRINT(item)
#define DPRINTLN(item)
#endif

#define DEFAULT_VENDOR_ID 0
#define LED_DEVICE_ID 1000
#define RF_DEVICE_ID 11
#define BUTTON_DEVICE_ID 5

const byte led = 7;  // Connect the anode (long lead, +ve) of a LED to this pin, and connect that LED's cathode (short lead, -ve) to GND through a 330R-1K resistor. 
const byte rfTx = 4; // transmit pin

SoftwareSerial WiFlySerial(2,3);
WiFly wifly;
RCSwitch mySwitch = RCSwitch();

char LED_VALUE[] = "000000";
const char mySSID[] = "SSID";
const char myPassword[] = "PASS";

void setup(){
  DSBEGIN();
  DPRINTLN(F("Starting"));

  pinMode(led, OUTPUT);  

  WiFlySerial.begin(9600);
  if( !wifly.begin(&WiFlySerial) ) {
    DPRINTLN(F("Init Ethernet failed"));
    wifly.terminal();
  }  
  if( !wifly.isAssociated() ) {
    DPRINTLN(F("Joining Network"));
    if (wifly.join(mySSID, myPassword, true)) {
      wifly.save();
      DPRINTLN(F("Joined wifi network"));
    } 
    else {
      DPRINTLN(F("Failed to join wifi network"));
      wifly.terminal();
    }
  } 
  else {
    DPRINTLN(F("Already joined network"));
  }

  NinjaBlock.client = wifly;
  #ifdef DEBUG_SERIAL
  NinjaBlock.debugSerial = &Serial;
  #endif
  NinjaBlock.host = "api.ninja.is";
  NinjaBlock.port = 80;
  NinjaBlock.nodeID = "ARDUINOBLOCK";
  NinjaBlock.token = "YOUR_TOKEN";
  if (NinjaBlock.begin()==0) {
      DPRINTLN(F("Init failed"));
      wifly.terminal();    
  }

  //Register LED
  NinjaBlock.guid="0";
  NinjaBlock.vendorID=DEFAULT_VENDOR_ID;
  NinjaBlock.deviceID=LED_DEVICE_ID;
  NinjaBlock.send(LED_VALUE);

  //Register RF device
  NinjaBlock.guid="0";
  NinjaBlock.vendorID=DEFAULT_VENDOR_ID;
  NinjaBlock.deviceID = RF_DEVICE_ID;
  NinjaBlock.send("110111010101011100000000"); //sample RF string
}

void loop() {
  sendObjects();

  if(NinjaBlock.receive()) {
      // If this function returns true, there are commands (data) from the server
      // Return values are:
      // NinjaBlock.strGUID
      // NinjaBlock.intVID
      // NinjaBlock.intDID
      // NinjaBlock.intDATA - if data is integer
      // NinjaBlock.strDATA - if data is string (note char[64])

      if (NinjaBlock.IsDATAString) {

          // Serial.print("strDATA=");
          // DPRINTLN(NinjaBlock.strDATA);

          if (NinjaBlock.intDID == 1000) {
              // FFFFFF is "white" in the RGB widget we identified as
              if (strcmp(NinjaBlock.strDATA,"FFFFFF") == 0) { 
                  DPRINTLN(F("LED ON"));
                  digitalWrite(led, HIGH); 
              } else if (strcmp(NinjaBlock.strDATA,"000000") == 0) {
                  DPRINTLN(F("LED OFF"));
                  digitalWrite(led, LOW); 
              }
              
              //update sender
              strcpy(LED_VALUE, NinjaBlock.strDATA);
              NinjaBlock.guid="0";
              NinjaBlock.vendorID=DEFAULT_VENDOR_ID;
              NinjaBlock.deviceID=LED_DEVICE_ID;
              NinjaBlock.send(LED_VALUE);
          }
          else if( NinjaBlock.intDID == 11 ) {
            //convert to tri-state
            //receiving binary, convert to hex string            
            // DPRINT(F("strData="));
            // DPRINTLN(NinjaBlock.strDATA);
            char* tsString = bin2tristate(NinjaBlock.strDATA);
            DPRINT(F("tri-state-string=")); DPRINTLN(tsString);
            mySwitch.enableTransmit(rfTx);  
            mySwitch.setProtocol(1, 200); //change this for your plugs
            mySwitch.sendTriState(tsString);
            mySwitch.disableTransmit();

            //update sender
            NinjaBlock.guid="0";
            NinjaBlock.vendorID=DEFAULT_VENDOR_ID;
            NinjaBlock.deviceID=RF_DEVICE_ID;
            NinjaBlock.send(NinjaBlock.strDATA);            
          }
      } else {
          // Do something with int data
          // Serial.print("intDATA=");
          // DPRINTLN(NinjaBlock.intDATA);
       }

  }

}

void sendObjects() {
  //write any code to send data up to Ninja servers
}

/**
**  Utility method from RCSwitch to convert the binary from tri-state remote
**  into actual tri-state. The Ninja server converts the Hex into Binary 
**  so it has to be converted back to Tri-State HEX (which isn't the same as just
**  converting it to Hex). 
**/
static char* bin2tristate(char* bin) {
  char returnValue[50];
  int pos = 0;
  int pos2 = 0;
  while (bin[pos]!='\0' && bin[pos+1]!='\0') {
    if (bin[pos]=='0' && bin[pos+1]=='0') {
      returnValue[pos2] = '0';
    } else if (bin[pos]=='1' && bin[pos+1]=='1') {
      returnValue[pos2] = '1';
    } else if (bin[pos]=='0' && bin[pos+1]=='1') {
      returnValue[pos2] = 'F';
    } else {
      return "not applicable";
    }
    pos = pos+2;
    pos2++;
  }
  returnValue[pos2] = '\0';
  return returnValue;
}
