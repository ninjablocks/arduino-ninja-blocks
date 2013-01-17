#include <SPI.h>        //SPI.h must be included as DMD is written by SPI (the IDE complains otherwise)
#include <DMD.h>        //
#include <TimerOne.h>   //
#include "SystemFont5x7.h"
#include "Arial_black_16.h"
#include <NinjaBlocksSerial.h>
#include <aJSON.h>

#define DISPLAYS_ACROSS 2  // be sure to set this to 1 if you only have one panel
#define DISPLAYS_DOWN 1
#define DISPLAYS_BPP 1
#define USER_VENDOR_ID 0
#define USER_DEVICE_ID 7000
#define WHITE 0xFF
#define BLACK 0

boolean marquee = false; //Set after drawing a marquee

DMD dmd(DISPLAYS_ACROSS, DISPLAYS_DOWN, DISPLAYS_BPP);

void ScanDMD()
{ 
  dmd.scanDisplayBySPI();
}

void setup(void)
{
   Serial.begin(9600);
   
   ninjaBlock.userVID=USER_VENDOR_ID;  // Let the library know that this is your VENDOR ID
   
   //initialize TimerOne's interrupt/CPU usage used to scan and refresh the display
   Timer1.initialize( 5000 );           //period in microseconds to call ScanDMD. Anything longer than 5000 (5ms) and you can see flicker.
   Timer1.attachInterrupt( ScanDMD );   //attach the Timer1 interrupt to ScanDMD which goes to dmd.scanDisplayBySPI()
   //clear/init the DMD pixels held in RAM
  dmd.clearScreen(BLACK);
  dmd.selectFont(Arial_Black_16);
  dmd.drawMarquee("NinjaBlocks Demo",16,(32*DISPLAYS_ACROSS)-1,0,WHITE,BLACK);
}

void loop(void)
{
  long start=millis();
  long timer=start;
  boolean ret=false;
  while(!ret)
  {
     if ((timer+30) < millis()) 
     {
       ret=dmd.stepMarquee(-1,0);
       timer=millis();
     }
     if(ninjaBlock.doReactors())    // Check if there is any commands from Host for your USER_VENDOR_ID
     {
       if (ninjaBlock.intDID==USER_DEVICE_ID)  // is the data for USER_DEVICE_ID ?
       {
         dmd.clearScreen(BLACK);
         dmd.drawMarquee(ninjaBlock.strDATA,strlen(ninjaBlock.strDATA),(32*DISPLAYS_ACROSS)-1,0,WHITE,BLACK);
       }
     }
   }
   // Ok display finished 1 round of Marquee, let server know
   ninjaBlock.doJSONData("0", USER_VENDOR_ID, USER_DEVICE_ID, "DONE", 0, true, 0);

}
