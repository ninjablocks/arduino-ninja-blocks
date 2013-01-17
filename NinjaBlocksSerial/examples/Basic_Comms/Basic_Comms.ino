/*
  NinjaBlock Basic Comms Example - @askpete
  
  This sketch uses the NinjaBlockSerial library to create a rudimentary button sensor 
  and a simple led actuator. This is awesome because it gives your Arduino project a
  REST interface in a couple of minutes.
  
  Ninja Params
  ------------
  
  You need a Ninja Blocks account to use the library, use the invite code 1012BB to signup
  at https://a.ninja.is if you don't have one. 

  For reference the NinjaBlock params are:
  
  token    For hacking use the virtual block token from https://a.ninja.is/hacking , there
           are other ways to get a token but hard to squeeze into a library for a 328.
  nodeID   This is the boards ID. Can be any 12+char string. Its used in REST so its a
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

     Connect an led to pin 13 and ground.

  2) Copy the NinjaBlock library into your Arduino libraries dir.
  
  3) Upload via Arduino ISP programmer, and view button changes via serial port.
  
  4) Test turning the LED on/off by sending the following command string via arduino the
     serial port.

  {"DEVICE":[{"G":"0","V":0,"D":1000,"DA":"FFFFFF"}]}
  {"DEVICE":[{"G":"0","V":0,"D":1000,"DA":"000000"}]}
  
  5) With a computer running Ninja Blocks client code (https://github.com/ninjablocks/client),
     for example a RaspberryPi, modify the .js file variable config.devtty to be the computers
     serial connection to the arduino. For example, /dev/tty.usbmodem####, or /dev/ttyACM#.
     Now the client code will communicate to the arduino via json commands.
  
  4) When you ground your button the first time it will appear on your dashboard. Turn your 
     led on by clicking on white and off by clicking on black. Yay.

  7) The real fun is using REST interface, e.g. turn your light on with
  
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
#include <NinjaBlocksSerial.h>
#include <aJSON.h>

//#include <MemoryFree.h>


#define DEFAULT_VENDOR_ID 0
#define LED_DEVICE_ID 1000
#define BUTTON_DEVICE_ID 5

byte button = 5; // Jumper this to ground to press the "button"
byte led = 13;  // A lot of Arduino's have an on board led connected to pin 13 and/or a 
                // a built in 1k resistor, so you can just play with the led that there
                // or stick an LED directly into 13 and ground.

boolean isButtonDown = false;

void setup(){

    pinMode(button, INPUT);  
    digitalWrite(button, HIGH); // Use the built in pull up resistor

    pinMode(led, OUTPUT);  

    Serial.begin(9600);
    Serial.println("Starting..");
//    delay(1000);   // This delay is to wait for the Ethernet Controller to get ready

//    NinjaBlock.host = "api.ninja.is";
//    NinjaBlock.port = 80;
//    NinjaBlock.nodeID = "ARDUINOBLOCK";
//    NinjaBlock.token = "VIRTUAL_BLOCK_TOKEN"; // Get yours from https://a.ninja.is/hacking 
    ninjaBlock.guid = "0";
    ninjaBlock.vendorID=DEFAULT_VENDOR_ID;
    ninjaBlock.deviceID=LED_DEVICE_ID;

    // Tell Ninja we exist and are alive and off.
    Serial.println("Creating LED");

    ninjaBlock.doJSONData("0", DEFAULT_VENDOR_ID, LED_DEVICE_ID, "000000", 0, true, 0);
}

void loop() {

    if(ninjaBlock.doReactors()) {
        // If this function returns true, there are commands (data) from the server
        // Return values are:
        // NinjaBlock.strGUID
        // NinjaBlock.intVID
        // NinjaBlock.intDID
        // NinjaBlock.intDATA - if data is integer
        // NinjaBlock.strDATA - if data is string (note char[64])

        if (ninjaBlock.IsDATAString) {

            // Serial.print("strDATA=");
            // Serial.println(NinjaBlock.strDATA);

            if (ninjaBlock.intDID == LED_DEVICE_ID) {
                // FFFFFF is "white" in the RGB widget we identified as
                if (strcmp(ninjaBlock.strDATA,"FFFFFF") == 0) { 
                    Serial.println("LED ON");
                    digitalWrite(led, HIGH); 
                } else if (strcmp(ninjaBlock.strDATA,"000000") == 0) {
                    Serial.println("LED OFF");
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
    if (digitalRead(button) == LOW) {
        if (!isButtonDown) {
            Serial.println("Button Down");
            ninjaBlock.doJSONData("0", DEFAULT_VENDOR_ID, BUTTON_DEVICE_ID, "", 1, false, 0);
            isButtonDown = true;
        }
    } else {
        if (isButtonDown) {
            Serial.println("Button Up");
            ninjaBlock.doJSONData("0", DEFAULT_VENDOR_ID, BUTTON_DEVICE_ID, "", 0, false, 0);
            isButtonDown = false;
        }
    }

}
