NinjaBlockWiFly
====================

Arduino library for connecting to the Ninja API via Wi-Fly. 

*NinjaBlockWiFly* is based on the original *NinjaBlocksEthernet* minimal library for connecting directly to the Ninja API from any board compatible, but with the WiFly HQ library. The code has also been improved to utilize PROGMEM library to save memory space and allow for other libraries to be utilized as part of your sketch.


Ninja Params
------------
For reference the NinjaBlock params are:

1. *token*  - For hacking use the virtual block token from https://a.ninja.is/hacking , there are other ways to get a token but hard to squeeze into a library for a 328.
2. *nodeID*  - This is the boards ID. Can be any 12+char string. Its used in REST so its a good idea to make it easy to remember, e.g. ARDUINOBLOCK.
3. *vendorID* - Use 0 with the device IDs here http://ninjablocks.com/docs/device-ids and you get pretty widgets for "free". Create your own by pinging help@ninjablocks.com.
4. *deviceID* - Identifies each device and maps to UI elements and properties that allow other apps to interact with your devices without knowing about them, see link above.
5. *guid* - Think of this as port - it is used to differentiate between multiple devices of the same type.
 

You need a Ninja Blocks account to get tokens, signup at https://a.ninja.is 


Usage Notes
-----------

1. Hit the forums http://forums.ninjablocks.com if something isn't clear. 
2. Copy your libraries into the Arduino IDEs library folder 
3. Get a virtual_block_token from https://a.ninja.is/hacking 
4. 
5. Use the Ninja Dashboard, REST interface, or helper libs to pissfart to your hearts content!

<code>
curl -H 'Content-Type: application/json' \\
   -X 'PUT' \\
   -d '{ "DA" : "Arduino REST FTW" }' \\
   -i https://api.ninja.is/rest/v0/device/ARDUINOBLOCK_0_0_7000?user_access_token=YOURTOKEN
</code>

NB: A user access token is not the same as a block token, from https://a.ninja.is/hacking
