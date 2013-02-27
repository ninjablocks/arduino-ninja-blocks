/*
  NinjaBlock Basic Comms Example - @askpete
  
  This sketch uses the NinjaBlockEthernet library to create a rudimentary button sensor 
  and a simple led actuator. This is awesome because it gives your Arduino project a
  REST interface in a couple of minutes.
  
  @justy : Note that the EtherTen reserves pins 11-13 for Ethernet and Card operations.  The Arduino Ethernet
  shield uses pin 10 and 4 also.
  
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
  
  1) I've tested this with a Freetronics Etherten (http://www.freetronics.com/etherten ) and 
     an Arduino Ethernet (http://arduino.cc/en/Main/ArduinoBoardEthernet ), but any board or 
     shield compatible with the standard Ethernet library should be fine. 

     For the button we'll just ground pin 5 - feel free to wire up a button. In the real 
     world you would obviously want to debounce.

     Connect the anode (long lead, +ve) of a LED to pin 7, and connect that LED's cathode (short lead, -ve) to GND through a 330R-1K resistor. 

  2) Copy the NinjaBlock library into your Arduino libraries dir. 
  
  3) Upload and plug in the intwertubes
  
  4) When you ground your button the first time it will appear on your dashboard. Turn your 
     led on by clicking on white and off by clicking on black. Yay.

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

  TODO: Write a proper how-to for this 

*/

#include <SPI.h>
#include <Ethernet.h>
#include <NinjaBlockEthernet.h>
//#include <MemoryFree.h>

#define DEFAULT_VENDOR_ID 0
#define LED_DEVICE_ID 1000
#define BUTTON_DEVICE_ID 5

#define ENABLE_SERIAL true    // @justy : You may wish to disable Serial, but it's also handy to be able to use it for debugging, so here's a master switch.

byte button = 5; // Jumper this to ground to press the "button"
byte led = 7;  // Connect the anode (long lead, +ve) of a LED to this pin, and connect that LED's cathode (short lead, -ve) to GND through a 330R-1K resistor. 

boolean isButtonDown = false;

void setup(){

    pinMode(button, INPUT);  
    digitalWrite(button, HIGH); // Use the built in pull up resistor

    pinMode(led, OUTPUT);  
  #if ENABLE_SERIAL
    Serial.begin(9600);
    Serial.println("Starting..");
  #endif
    delay(1000);   // This delay is to wait for the Ethernet Controller to get ready

    NinjaBlock.host = "api.ninja.is";
    NinjaBlock.port = 80;
    NinjaBlock.nodeID = "ETHERSHIELDBLOCK";  // Name this as you wish
    NinjaBlock.token = "VIRTUAL_BLOCK_TOKEN"; // Get yours from https://a.ninja.is/hacking 
    NinjaBlock.guid = "0";
    NinjaBlock.vendorID=DEFAULT_VENDOR_ID;
    NinjaBlock.deviceID=LED_DEVICE_ID;

    if (NinjaBlock.begin()==0)
        Serial.println("Init failed");

    // Tell Ninja we exist and are alive and off.
     #if ENABLE_SERIAL 
       Serial.println("Creating LED");
     #endif
    NinjaBlock.send("000000"); // We send 000000 because we are identifying as an RGB led

}

void loop() {

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
            // Serial.println(NinjaBlock.strDATA);

            if (NinjaBlock.intDID == 1000) {

                // FFFFFF is "white" in the RGB widget we identified as
                if (strcmp(NinjaBlock.strDATA,"FFFFFF") == 0) { 
                      #if ENABLE_SERIAL
                        Serial.println("LED ON");
                        #endif
                    digitalWrite(led, HIGH); 
                } else if (strcmp(NinjaBlock.strDATA,"000000") == 0) {
                    #if ENABLE_SERIAL
                      Serial.println("LED OFF");
                      #endif
                    digitalWrite(led, LOW); 
                }

            }
        } else {
            // Do something with int data
            // Serial.print("intDATA=");
            // Serial.println(NinjaBlock.intDATA);
         }

    }

    // We do it this way so it doesn't block the receive. The slight delay in the
    // loop gives us (extremely bodgy) debouncing too.
    // @justy : Note that in previous iterations of this sketch the logic here was reversed- i.e. we drove the pin LOW when the button was pressed.
    if (digitalRead(button) == HIGH) {
        if (!isButtonDown) {
            #if ENABLE_SERIAL
              Serial.println("Button Down");
            #endif
            NinjaBlock.deviceID=5;
            NinjaBlock.send(1);
            isButtonDown = true;
        }
    } else {
        if (isButtonDown) {
            #if ENABLE_SERIAL
              Serial.println("Button Up");
            #endif
            NinjaBlock.deviceID=5;
            NinjaBlock.send(0);
            isButtonDown = false;
        }
    }

}
