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
const char *menu_strings[MENU_ITEMS] = { "Pulse width (ms)", "Pulse cycle (ms)", "Pulses", "PWM", "Polarity", "Refractory (s)", "ARMED! HOLD TO START"};
const char *trainon_strings[MENU_ITEMS] = { "Pulse width (ms)", "Pulse cycle (ms)", "Pulses left", "PWM", "Polarity", "Refractory (s)", "*TRAIN IS GOING*"};
byte row_current = 0;
bool column_select = false; // true = right side, false = left side
bool menu_redraw_required = false;
byte last_key_code = KEY_NONE;
byte i; // indexing

// Train settings
unsigned int nums[num_items] = {200, 500, 5, 0, 0, 10}; // [Pulse width, Pulse cycle, N pulses, PWM, Polarity, refractory]
const int numdiffs[num_items] = {100, 100, 1, 20, 1, 1};
const int numdiffs2[num_items] = {500, 500, 5, 5, 0, 10};
const int numdiffthresh[num_items] = {500, 500, 20, 100, 1, 20};
unsigned int numsrem[num_items]; // [Pulse width, Pulse cycle, N pulses, PWM, Polarity, refractory]
unsigned long refractory = 10*1000;
bool pulseon = false;
bool trainon = false;
bool startbuttondown = false;
const int maxpwm = 250;
bool arm = false;

// Initializing
uint8_t uiKeyCodeFirst = KEY_NONE;
uint8_t uiKeyCodeSecond = KEY_NONE;
uint8_t uiKeyCode = KEY_NONE;

// Time variables
unsigned long startbuttonhold = 1000; // in millisecs
unsigned long startbuttondowntime, startbuttoncurrenttime;
unsigned long t0, tnow, t0train, tnowtrain, StepTimer, InputStepTimer;

// Keypad
unsigned int val; // variable to store the value coming from the analog pin
byte button;      // 5 - nothing, 4 - up, 3 - right, 2 - down, 1 - select, 0 - left
const byte pin = 2; // Output

// i2c
byte m, n; // send
byte o = 1;
bool sendi2c = false;

// safety
bool lockpolarity = true;
const int protecttime = 20; // ms, CPU time around pulses to protect pulse timing

void initialdraw(void) {
  byte h;
  const char *s[5] = { "PWM pulse BOX v1.0", "By Stephen Zhang", "Output Pin - ", "SDA pin - A4", "SCL Pin - A5"};
  char buf[4];
  u8g_uint_t w, d, w2;

  u8g.setFont(u8g_font_6x12);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  
  h = u8g.getFontAscent()-u8g.getFontDescent();
  w = u8g.getWidth();
  w2 = w * 0.75;

  // First row
  d = (w - u8g.getStrWidth(s[0])) / 2;
  u8g.drawBox(0, 0, w, h);
  u8g.setDefaultBackgroundColor();
  u8g.drawStr(d, 0, s[0]);
  
  // Second row
  d = (w - u8g.getStrWidth(s[1])) / 2;
  u8g.setDefaultForegroundColor();
  u8g.drawStr(d, h, s[1]);

  // Output pin 1
  d = u8g.getStrWidth(s[2]);
  u8g.drawStr(1, 3 * h, s[2]);
  snprintf (buf, 4, "%d", pin);
  u8g.drawStr(d, 3 * h, buf);

  // Line 4
  d = u8g.getStrWidth(s[3]);
  u8g.drawStr(1, 4 * h, s[3]);

  // Line 5
  d = u8g.getStrWidth(s[4]);
  u8g.drawStr(1, 5 * h, s[4]);
}

void uiStep(void) {
  // Only draw input once every 50 ms
  if ((millis() - InputStepTimer) < 50){
    return;
  }
  InputStepTimer = millis();
  
  uiKeyCodeSecond = uiKeyCodeFirst;

  val = analogRead(0); // read the analog in value:
  button = val/200;
  // Serial.print("Button: ");
  // Serial.println(button);

  switch (button){
    case 0:
      uiKeyCodeFirst = KEY_LEFT;
      break;
    case 1:
      uiKeyCodeFirst = KEY_SELECT;
      break;
    case 2:
      uiKeyCodeFirst = KEY_DOWN;
      break;
    case 3:
      uiKeyCodeFirst = KEY_RIGHT;
      break;
    case 4:
      uiKeyCodeFirst = KEY_UP;
      break;
    case 5:
      uiKeyCodeFirst = KEY_NONE;
      break;
  }
    
  if ( uiKeyCodeSecond == uiKeyCodeFirst ){
    uiKeyCode = uiKeyCodeFirst;
  }
  else{
    uiKeyCode = KEY_NONE;
  }
}

void drawMenu(void) {
  byte h;
  u8g_uint_t w, d, w2;

  u8g.setFont(u8g_font_6x10);
  u8g.setFontRefHeightText();
  u8g.setFontPosTop();
  
  h = u8g.getFontAscent()-u8g.getFontDescent();
  w = u8g.getWidth();
  w2 = w * menuscale;
  
  for( i = 0; i < MENU_ITEMS; i++ ) {
    if ( i == row_current && !column_select) {
      // Current row, left side focus
      u8g.drawBox(0, i*h, w2, h);
      u8g.setDefaultBackgroundColor();
    }
    else{
      // Not current row
      u8g.setDefaultForegroundColor();
    }

    // Write left side text
    if (trainon){
      if ((numsrem[2] == 0) && (i == MENU_ITEMS - 1)){
        u8g.drawStr(1, i*h, "=TRAIN Resting=");
      }
      else{
        u8g.drawStr(1, i*h, trainon_strings[i]);
      }
    }
    else{
      if((i == MENU_ITEMS - 1) && !arm){
        u8g.drawStr(1, i*h, "[UNARMED]");
      }
      else {
        u8g.drawStr(1, i*h, menu_strings[i]);
      }
      
    }
    

    if ( i == row_current && column_select) {
      // Current row, right side focus
      u8g.drawBox(w2, i*h, w-w2, h);
      u8g.setDefaultBackgroundColor();
    }
    else{
      // Not current row
      u8g.setDefaultForegroundColor();
    }

    // Write right side text
    
    if (i < num_items){
      char buf[9];
      if (trainon){
        if (i == 4){
          // Polarity
          if (numsrem[i] == 1){
            snprintf (buf, 9, "POS");
          }
          else {
            snprintf (buf, 9, "NEG");
          }
        }
        else{
          snprintf (buf, 9, "%d", numsrem[i]);
        }
        
      }
      else {
        if (i == 4){
          // Polarity
          if (nums[i] == 1){
            snprintf (buf, 9, "POS");
          }
          else {
            snprintf (buf, 9, "NEG");
          }
        }
        else{
          snprintf (buf, 9, "%d", nums[i]);
        }
      }
      d = w - u8g.getStrWidth(buf) + 1;
      u8g.drawStr(d, i*h, buf);
    }
   
    
  }

  // Feed back on holding
  if(startbuttondown && !trainon){
    if (arm){
      char buf[9];
      snprintf (buf, 9, "%ds", (startbuttoncurrenttime - startbuttondowntime)/1000);
      d = w - u8g.getStrWidth(buf) + 1;
      u8g.drawStr(d, num_items*h, buf);
    }
    else{
      d = w - u8g.getStrWidth("X") + 1;
      u8g.drawStr(d, num_items*h, "X");
    }
  }

  // Time since last train (train on only)
  if (trainon){
    char buf[9];
    snprintf (buf, 9, "%d", tnowtrain/1000);
    d = w - u8g.getStrWidth(buf) + 1;
    u8g.drawStr(d, num_items*h, buf);
  }
}

void updateMenu(void) {
  // Serial.println(uiKeyCode);
  // No rapid firing unless it's the last row and you push select
  if ( uiKeyCode != KEY_NONE && last_key_code == uiKeyCode ) {
    if ((row_current != MENU_ITEMS - 1) || (uiKeyCode != KEY_SELECT)){
      return;
    } 
  }

  // No changing while train on
  if (trainon){
    return;
  }
  
  last_key_code = uiKeyCode;
  
  switch ( uiKeyCode ) {
    case KEY_DOWN:
      if (!column_select){
        // Going up and down menu
        row_current++;
        if ( row_current >= MENU_ITEMS ){
          row_current = 0;
        }        
      }
      else{
        if ((row_current == 4) && lockpolarity){
          // polarity is locked
          return;
        }

        if ((nums[row_current] >= numdiffs[row_current]) && (nums[row_current] <= numdiffthresh[row_current])){
          nums[row_current] = nums[row_current] - numdiffs[row_current];
        }
        else if (nums[row_current] > numdiffs[row_current]){
          nums[row_current] = nums[row_current] - numdiffs2[row_current];
        }

        if (row_current == 3){
          // pmw i2c
          m = 3;
          n = nums[row_current];
          sendi2c = true;
        }
        if (row_current == 4){
          // polarity i2c
          m = 2;
          n = nums[row_current];
          sendi2c = true;
        }
      }

      
        
      // Serial.println(row_current);
      menu_redraw_required = true;
      break;
      
    case KEY_UP:
      if (!column_select){
        // Going up and down menu
        if ( row_current == 0 ){
          row_current = MENU_ITEMS;
        }
        row_current--;
      }
      else{
        if ((row_current == 3) && (nums[3] == maxpwm)){
          return; // pwm has an upper limit
        }
        if ((row_current == 4) && lockpolarity){
          // polarity is locked
          return;
        }
        
        if (nums[row_current] < numdiffthresh[row_current]){
          nums[row_current] = nums[row_current] + numdiffs[row_current];
        }
        else{
          nums[row_current] = nums[row_current] + numdiffs2[row_current];
        }

        if (row_current == 3){
          // pmw
          m = 3;
          n = nums[row_current];
          sendi2c = true;
        }
        if (row_current == 4){
          // polarity
          m = 2;
          n = nums[row_current];
          sendi2c = true;
        }
      }
      // Serial.println(row_current);
      menu_redraw_required = true;
      break;
      
    case KEY_SELECT:
      if (column_select){
        // Going up and down menu
        row_current++;
        if ( row_current >= num_items ){
          row_current = 0;
        }        
      }
      else if (row_current == MENU_ITEMS - 1){
        startbuttoncurrenttime = millis();
        // First time start button down
        if (!startbuttondown){
          startbuttondowntime = startbuttoncurrenttime;
          startbuttondown = true;

          // Last min pwm sync
          m = 3;
          n = nums[3];
          sendi2c = true;
          
        }
        else if ((startbuttoncurrenttime - startbuttondowntime > startbuttonhold) && !trainon && arm){
          trainon = true;
          t0train = millis();
          
          for( i = 0; i < num_items; i++ ) {
            numsrem[i] = nums[i]; // [Pulse width, Pulse cycle, N pulses, PWM, Polarity, refractory]
          }

          refractory = nums[5] * 1000;
        }
      }
      menu_redraw_required = true;
      break;
      
    case KEY_LEFT:
      if (row_current < num_items){
        column_select = !column_select;
        menu_redraw_required = true;
      }
      // Serial.println(row_current);
      break;
      
    case KEY_RIGHT:
      if (row_current < num_items){
        column_select = !column_select;
        menu_redraw_required = true;
      }
      // Serial.println(row_current);
      break;
    
    case KEY_NONE:
      if (startbuttondown){
        startbuttondown = false;
        menu_redraw_required = true;
      }
      break;
  }
}

// Train function
// [0          , 1          , 2       , 3   , 4       , 5  ]
// [Pulse width, Pulse cycle, N pulses, PWM , Polarity, refractory]
void dotrain(void){
  tnow = millis() - t0;
  tnowtrain = millis() - t0train;

//  Serial.print(tnow);
//  Serial.print(" ");
//  Serial.print(numsrem[1]);
//  Serial.print(". ");
//  Serial.print(pulseon);
//  Serial.print(". ");
//  Serial.println(trainon);
  
  // LED off
  if ((tnow > numsrem[0]) && (pulseon)){
    pulseon = false;
    digitalWrite(pin, LOW);

    // i2c off
    m = 0;
    n = 0;
    sendi2c = true;

    if (debug){
      Serial.print("OFF at ");
      Serial.print(tnow);
      Serial.println(" ms.");
    }
  }

  // LED on
  if ((tnow >= numsrem[1]) && (!pulseon) && (numsrem[2] > 0) && (trainon)){
    // i2c on
    m = 1;
    n = 0;
    sendi2c = true;
          
    // Serial.println("LED on");
    numsrem[2] = numsrem[2] - 1;
    pulseon = true;
    t0 = millis();
    digitalWrite(pin, HIGH);
    // menu_redraw_required = true; // % Changed pulses so redraw

    if (debug){
      Serial.print("ON at ");
      Serial.print(tnow);
      Serial.println(" ms.");
    }
  }

  //
  if((millis() - StepTimer > 500) && trainon){         // output a temperature value per 500ms
    menu_redraw_required = true; // force redraw every 0.5s anyway durign train
    StepTimer = millis();
  }

  // Reset train
  // Autoreset
  if ((tnowtrain > refractory) && (trainon) && (numsrem[2] == 0)){
    trainon = false;
    menu_redraw_required = true;
  }
}

void protectpulse(void){
  // If pulse on, dont refresh screen is pulse is about to be turned off
  if ((tnow > (numsrem[0] + protecttime)) && (pulseon)){
    menu_redraw_required = false;
    if (debug){
      Serial.print("Protection at T = ");
      Serial.println(tnow);
    }
  }
  // If pulse off, dont refresh screen is pulse is about to be turned on
  if ((tnow > (numsrem[1] + protecttime)) && (!pulseon)){
    menu_redraw_required = false;
    if (debug){
      Serial.print("Protection at T = ");
      Serial.println(tnow);
    }
  }
}

// i2c
void i2csend(void){
  Wire.beginTransmission(4); // transmit to device #4
  Wire.write(m);        // sends first byte
  Wire.write(n);        // sends second byte  
  Wire.endTransmission();    // stop transmitting
}

void i2creceive(void){
  Wire.requestFrom(4, 1);
  if (Wire.available() >= 1){
    o = Wire.read();

    if (arm && (o == 0)){
      arm = false;
      menu_redraw_required = true;
    }
    if (!arm && (o == 1)){
      arm = true;
      menu_redraw_required = true;
    }
  }
}

void setup() {
  Wire.begin(); // join i2c bus (address optional for master)
  // i2c initialize
  
  if (debug || debugcheckcycle){
    Serial.begin(9600);
    Serial.println("Debugging.");
  }
  
  // rotate screen, if required
  u8g.setRot180();
  u8g.setContrast(0); // Config the contrast to the best effect
  
  // Initial draw
  u8g.firstPage();
  do {
    initialdraw();
  } while( u8g.nextPage() );
  
  delay(1000);
  
  menu_redraw_required = true;     // force initial redraw

  // Serial.begin(9600);
  
  // Time
  t0 = millis();
  t0train = millis();
  StepTimer = millis();

  // Pins
  pinMode(pin, OUTPUT);

  
}

void loop() {  
  if (debugcheckcycle && trainon){
    Serial.println(tnowtrain); // 6-7 ms right now which is fine
  }
  
  uiStep(); // check for key press
  
  // protect cpu time around pulses
  if (menu_redraw_required && trainon && (numsrem[2] > 0)){
    protectpulse();
  }

//  if (trainon){
//    menu_redraw_required = false;
//  }

//  if (debug && trainon){
//    if (menu_redraw_required){
//      Serial.print("Redraw at ");
//      Serial.print(tnow);
//      Serial.print(" ms. Pulse on is ");
//      Serial.print(pulseon);
//      Serial.print(". Protection condition 1 is ");
//      Serial.print((tnow > (numsrem[0] + protecttime)));
//      Serial.println(".");
//    }
//  }
  
  if ( menu_redraw_required ) {
    u8g.firstPage();

    do  {
      drawMenu();
    } while( u8g.nextPage() );  
    
    menu_redraw_required = false;
  }
  
  if (sendi2c){
    // i2c
    i2csend();
    sendi2c = false;
  }

  i2creceive();
  
  updateMenu(); // update menu bar
  dotrain(); // update train

  // delayMicroseconds(200); // Does this help?
}
