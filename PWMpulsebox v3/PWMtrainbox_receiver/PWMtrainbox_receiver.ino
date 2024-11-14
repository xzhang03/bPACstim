#include <Wire.h>
#define debug false
#define i2cadd 4

// ==================================================
// Modes: pick 1 from the 3 below
// In passive mode, each trig in pulse triggers an LED pulse whose duration is defined by the trigin pulse
// Otherwise, each trig in pulse triggers a train
// This is active high
#define passivemode false
bool passiveon = false;

// In automode, device generates its own train, or each trig in pulse would trigger a train
#define autonomousmode false

// In semiautomode, device generates its own train, but each LED and blank pulse only comes out when a frame pulse is in
#define semiautomode true
#define trigin_activehigh true
bool semiautoarm = false;

// ==================================================

// Arm
const byte ArmPin = 7;
const byte ArmLED = 11;

// Frame and trigger
const byte FPin = A5; // Input
const byte FPin_en = A4; // Input enable
const byte FPin_enled = 9; // Input enable LED

const byte LEDPIN1 = 5; // PWM out
const byte BlankPin = 12;
const byte TrigPin = 10; // Output
const byte onboardLED = 13; // Output indicator

// logic
bool arm = false;
bool alwaysarm = false;
bool input_en = false;

// Train
// [        0,        1,        2,   3,           4,        5]
// [LED width, LED freq, N pulses, PWM, Train cycle, N trains]
#define num_items 6
unsigned int nums[num_items] = {10, 20, 40, 100, 10, 1}; 
unsigned int numsrem[num_items] = {10, 10, 0, 100, 10, 1}; // [LED width, LED freq, N pulses, Ext trig, Train cycle, N trains]
byte numsi2c[num_items]; // i2c only, in bytes


// Pulse and train modifiers
byte pulsemod = 2;
byte trainmod = 5;

// I2c
int m, n, o; // receive

// indexing
byte i;

// Pulse
uint32_t ledwidth = 10000;
uint32_t ledcycle = 50000; // us

// Train
unsigned long t0, tnow, tnowtrain, t0train;
bool pulseon = false;
bool trainon = false;

// Blank
bool blankon = false;
uint32_t blank_gap = 2000; // 2 ms time to let led drop off
uint32_t blankwidth = 12000;

// Step size
//uint32_t stepsz = 200;

void setup() {
  // put your setup code here, to run once:
  // pinMode(TriggerPin, INPUT); 
  pinMode(ArmPin, INPUT_PULLUP); 
  pinMode(ArmLED, OUTPUT);

  // Trigger in
  pinMode(FPin, INPUT);
  pinMode(FPin_en, INPUT);
  pinMode(FPin_enled, OUTPUT);

  // Output
  pinMode(TrigPin, OUTPUT);
  pinMode(onboardLED, OUTPUT);
  pinMode(LEDPIN1, OUTPUT);
  pinMode(BlankPin, OUTPUT);
  digitalWrite(LEDPIN1, HIGH); // Suppress light

  Wire.begin(i2cadd);                // join i2c bus with address #4
  Wire.onReceive(receiveEvent); // register event
  Wire.onRequest(requestEvent); // request event
  
  // Initial polarity
  resettrain();

  // Train timers
  t0 = micros();
  tnow = t0;
  t0train = t0 / 1000;
  tnowtrain = t0train;

  #if (debug)
    Serial.begin(9600);
  #endif
}

void loop() {
  // Timing
  tnow = micros();
  tnowtrain = tnow / 1000;
  
  // Arming
  check_arm();

  // Trigger enable
  check_trigin_en();
  
  // Check interrupt
  #if passivemode
    // Each input pulse triggers an LED pulse
    check_trigin_passive();
  #endif 
  #if autonomousmode
    // Each input pulse triggers a train
    check_trigin();
  #endif
  
  
  // Train
  dotrain();

//  delayMicroseconds(stepsz);
}
