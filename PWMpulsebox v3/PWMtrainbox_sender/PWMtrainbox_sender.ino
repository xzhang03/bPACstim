/*

  Menu.pde
  
  Simple Menu Selection

  >>> Before compiling: Please remove comment from the constructor of the 
  >>> connected graphics display (see below).
  
  Universal 8bit Graphics Library, https://github.com/olikraus/u8glib/
  
  Copyright (c) 2012, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, 
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list 
    of conditions and the following disclaimer.
    
  * Redistributions in binary form must reproduce the above copyright notice, this 
    list of conditions and the following disclaimer in the documentation and/or other 
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
  

*/

#include <Wire.h>
#include "U8glib.h"

U8GLIB_NHD_C12864 u8g(13, 11, 10, 9, 8);    // SPI Com: SCK = 13, MOSI = 11, CS = 10, CD = 9, RST = 8

// debug
#define debug false
#define debugcheckcycle false
#define alwaysarmed false

// UI settings
#define KEY_NONE 0
#define KEY_UP 1
#define KEY_DOWN 2
#define KEY_SELECT 3
#define KEY_LEFT 4
#define KEY_RIGHT 5

#define menuscale 0.75

#define MENU_ITEMS 7
#define num_items 6

const char *menu_strings[MENU_ITEMS] = { "LED width (ms)", "Pulse freq (hz)", "Pulses", "PWM", "Train cycle (s)", "N trains", "Unused"};
const char *trainon_strings[MENU_ITEMS] = { "LED width (ms)", "Pulse freq (hz)", "Pulses left", "PWM", "Train cycle (s)", "Trains left", "*TRAIN IS GOING*"};
byte row_current = 0;
bool column_select = false; // true = right side, false = left side
bool menu_redraw_required = false;
byte last_key_code = KEY_NONE;
byte i; // indexing

// Train settings
// [        0,        1,        2,   3,           4,        5]
// [LED width, LED freq, N pulses, PWM, Train cycle, N trains]
uint32_t nums[num_items]; // Current value (set in receiver, {10, 10, 10, 0, 10, 1})
const uint32_t numdiffs[num_items] = {10, 5, 10, 20, 10, 5}; // intermediate
const uint32_t numdiffs2[num_items] = {0, 0, 0, 5, 0, 0}; // large (0 means capping)
const uint32_t numdiffs0[num_items] = {1, 1, 2, 0, 5, 1}; // fine
const uint32_t numdiffthresh[num_items] = {250, 100, 500, 100, 1000, 200}; // Going down: num > threshold, go down as numdiffs2; Going up: num >= threshold, go up as numdiffs2.
const uint32_t numdiffthresh0[num_items] = {20, 20, 10, 0, 30, 10}; // Going down: num <= threshold0, go down as numdiffs0; Going up: num < threshold0, go up as numdiffs0.
unsigned int numsrem[num_items]; // [LED width, Blank width, N pulses, Freq. Mod., Train cycle, N trains]
const byte pulsemod = 2; // for pulse number num[2]
const byte trainmod = 5; // for train cycle num[4]
unsigned long refractory = 5*1000;
bool trainon = false;
bool startbuttondown = false;

// Arm and input
bool arm = false;
bool input_en = false;

// Initializing
uint8_t uiKeyCodeFirst = KEY_NONE;
uint8_t uiKeyCodeSecond = KEY_NONE;
uint8_t uiKeyCode = KEY_NONE;

// Time variables
unsigned long startbuttonhold = 1000; // in millisecs
unsigned long startbuttondowntime, startbuttoncurrenttime;
unsigned long tnow;
unsigned long t0train, tnowtrain, StepTimer, InputStepTimer, ArmStepTimer;
unsigned long tdebug = 0;
unsigned long tdebug2 = 0;

// Keypad
unsigned int val; // variable to store the value coming from the analog pin
byte button;      // 5 - nothing, 4 - up, 3 - right, 2 - down, 1 - select, 0 - left
const byte pin = 2; // Output

// i2c
byte m = 0; // send
byte n = 0; // send
byte o = 0; // send
byte p = 0; // receive
byte i2csendbuf[3]; //
bool sendi2c = false;
bool synci2c = false;

// Receiver mode
uint8_t receivermode = 0; // 0 - autonomous, 1 - semiauto, 2 - passive

void setup() {
  Wire.begin(); // join i2c bus (address optional for master)

  // i2c initialize
//  i2csync();
  i2csync_back();

  // i2c get receiver mode
  checkmode();
  
  #if (debug || debugcheckcycle)
    Serial.begin(9600);
    Serial.println("Debugging.");
  #endif
  
  // rotate screen, if required
  u8g.setRot180();
  u8g.setContrast(0); // Config the contrast to the best effect
  
  // Initial draw
  u8g.firstPage();
  do {
    initialdraw();
  } while( u8g.nextPage() );
  
  
  menu_redraw_required = true;     // force initial redraw

  // Serial.begin(9600);
  
  // Time
  t0train = millis();
  StepTimer = millis();
  ArmStepTimer = millis();

  // Arm
  #if alwaysarmed
    arm = alwaysarmed;
    sendalwaysarm();
  #endif
  
  // Pins
  pinMode(pin, OUTPUT);
  pinMode(13, OUTPUT);

  while((millis() - t0train) < 500){
    // nothing
  }
}

void loop() {  
  // Timer
  tnow = millis();

  #if debugcheckcycle
    if (trainon){
      Serial.println(tnowtrain); // 6-7 ms right now which is fine
    }
  #endif

  #if debug
    if ((tnow - tdebug) > 1000){
      tdebug = tnow;
      tdebug2++;
      Serial.print("T="); 
      Serial.print(tdebug2); 
      Serial.print(" bytes in i2c buffer: "); 
      Serial.println(Wire.available());

      receivermode = checkmode();
      Serial.print(receivermode);
      switch (receivermode ){
        case 0:
          Serial.println(" Autonomous mode.");
          break;
        case 1:
          Serial.println(" Semiauto mode.");
          break;
        case 2:
          Serial.println(" Passive mode.");
          break;
        default:
          Serial.println(" Unknown mode.");
          break;
      }
    }
  #endif
  
  uiStep(); // check for key press
  
  
  if ( menu_redraw_required ) {
    u8g.firstPage();

    do  {
      drawMenu();
    } while( u8g.nextPage() );  
    
    menu_redraw_required = false;
  }

  #if !alwaysarmed
    if ((tnow - ArmStepTimer) > 300){
      ArmStepTimer = tnow;
      i2carm();
    }
  #endif
  
  updateMenu(); // update menu bar
  dotrain(); // update train

  delayMicroseconds(200); // Does this help?
}
