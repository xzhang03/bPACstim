#include <Wire.h>

// const byte TriggerPin = 11;
const byte ArmPin = 12;
const byte LEDArm = 11;
const byte LEDPIN1 = 5; // Pin 5: PWM
const byte LEDPIN2 = 6; // Pin 6: digital
const byte LEDPIN3 = 8; // Pin 8: digital unsigned
const byte LEDonboard = 13; // onboard LED to show when Stim is on

// PWM
int pwm = 205; //0 - 385,  250 - 12, 100 - 250, 150 - 185, 200 - 100, 210 - 80, 205 81, 

// polarity (1 - pos, 0 - neg)
bool pol = false;

// logic
bool arm = false;
bool softarm = false; // Software arm (dangerous)

// Train
// [          0,           1,        2,   3,        4,          5]
// [Pulse width, Pulse cycle, N pulses, PWM, Polarity, refractory]
#define num_items 6
unsigned int nums[num_items] = {200, 500, 5, 0, 0, 5}; 
unsigned int numsrem[num_items]; // [Pulse width, Pulse cycle, N pulses, PWM, Polarity, refractory]
byte numsi2c[num_items]; // i2c only, in bytes

// I2c
int m, n, o; // receive

// safety
bool lockpolarity = true;

// indexing
byte i;

// Train
unsigned long t0, tnow, tnowtrain, t0train;
bool pulseon = false;
bool trainon = false;
unsigned long refractory = 5*1000;

// debug
#define debug_train false
bool trainon2 = false;
byte trainon2_ch = 3;

void setup() {
  // put your setup code here, to run once:
  // pinMode(TriggerPin, INPUT); 
  pinMode(ArmPin, INPUT_PULLUP); 
  pinMode(LEDArm, OUTPUT);
  pinMode(LEDPIN1, OUTPUT);
  pinMode(LEDPIN2, OUTPUT);
  pinMode(LEDPIN3, OUTPUT);
  pinMode(trainon2_ch, OUTPUT);

  Wire.begin(4);                // join i2c bus with address #4
  Wire.onReceive(receiveEvent); // register event
  Wire.onRequest(requestEvent); // request event

  // Initial polarity
  resetpol();

  // Train
  t0 = millis();
}

void loop() {
  
  // Arming
  if ((digitalRead(ArmPin) == LOW) || softarm){
    arm = true;
    analogWrite(LEDArm,50);
  }
  else{
    arm = false;
    trainon = false;
    analogWrite(LEDArm,0);

    // Halt and catch fire
    if (pulseon){
      // Stop
      pulseon = false;
      resetpol();
    }
  }

  // Train
  dotrain();

  
  if (debug_train){
    if (!trainon2 && trainon){
      digitalWrite(trainon2_ch, HIGH);
      trainon2 = true;
    }
    else if (trainon2 && !trainon){
      digitalWrite(trainon2_ch, LOW);
      trainon2 = false;
    }
  }
  
}

void receiveEvent(int howMany){
  if (Wire.available() >= 3){
    m = Wire.read();
    n = Wire.read();
    o = Wire.read();
  }
  
  // [          0,           1,        2,   3,        4,          5]
  // [Pulse width, Pulse cycle, N pulses, PWM, Polarity, refractory]
  switch (m){
    case 1:
      // Start
      for( i = 0; i < num_items; i++ ) {
        numsrem[i] = nums[i]; // [Pulse width, Pulse cycle, N pulses, PWM, Polarity, refractory]
      }

      refractory = nums[5] * 1000;
      if (arm){
        trainon = true;
        t0train = millis();
      }
      
      o = 255; // for echo
      break;
      
    case 2:
      // Stop
      pulseon = false;
      resetpol();
      o = 0; // for echo
      break;
      
    case 3:
      // Change single value
      if (n <= 1){
        // convert
        nums[n] = o * 100;
      }
      else if (n == 3){
        nums[n] = o;
        pwm = nums[n];
      }
      else if (n == 4){
        // protect polarity
        if (!lockpolarity){
          nums[n] = o;
          pol = nums[n] == 1;
          resetpol();
        }
      }
      else if (n == 5){
        nums[n] = o;
        refractory = nums[n] * 1000;
      }
      else {
        nums[n] = o;
      }
      break;

    case 10:
      // polarity lock toggle
      if (o == 10){
        // o = 10 is the only unlock
        lockpolarity = false;
      }
      else{
        lockpolarity = true;
      }
      break;

    case 11:
      // soft arm
      if (o == 10){
        softarm = true;
      }
      break;

    case 12:
      // soft unarm
      if (o == 10){
        softarm = false;
      }
      break;
  }
}

void requestEvent() {
  switch (m){
    case 1:
      // Start
      // echo back o 255
      Wire.write(o);
      break;
      
    case 2:
      // Stop
      // echo back o 0
      Wire.write(o);
      break;
      
    case 3:
      // Change single value
      if (n <= 1){
        Wire.write(nums[n]/100);
      }
      else{
        Wire.write(nums[n]);
      }
      break;

    case 4:
      // ping single value in the running nums
      if (n <= 1){
        Wire.write(numsrem[n]/100);
      }
      else{
        Wire.write(numsrem[n]);
      }
      break;
      
    case 5:
      // ping single value
      if (n <= 1){
        Wire.write(nums[n]/100);
      }
      else{
        Wire.write(nums[n]);
      }
      break;

    case 6:
      // ping entire array
      for (i = 0; i < num_items; i++){
        if (i <= 1){
          numsi2c[i] = nums[i] / 100;
        }
        else{
          numsi2c[i] = nums[i];
        }
      }
      Wire.write(numsi2c, num_items);
      break;

    case 7:
      // ping arm
      if (arm){
        Wire.write(1);
      }
      else{
        Wire.write(0);
      }
      break;

    case 8:
      // ping trainon
      if (trainon){
        Wire.write(1);
        
      }
      else{
        Wire.write(0);
      }
      break;

    case 9:
      // initial ping - always return 42
      Wire.write(42);
      break;

    case 10:
      // polarity lock toggle
      Wire.write(o);
      break;

    case 11:
      // softarm
      Wire.write(o);
      break;

    case 12:
      // soft unarm
      Wire.write(o);
      break;
      
    
  }
  // reset m;
  m = 0;
}

void resetpol(){
  if (pol){
    // Positive pol
    analogWrite(LEDPIN1, 0);
    digitalWrite(LEDPIN2, LOW);
    digitalWrite(LEDPIN3, LOW);
    digitalWrite(LEDonboard, LOW);
  }
  else{
    // Negative pol
    analogWrite(LEDPIN1, 255);
    digitalWrite(LEDPIN2, HIGH);
    digitalWrite(LEDPIN3, LOW);
    digitalWrite(LEDonboard, HIGH);
  }
}

// Train function
// [0          , 1          , 2       , 3   , 4       , 5  ]
// [Pulse width, Pulse cycle, N pulses, PWM , Polarity, refractory]
void dotrain(void){
  tnow = millis() - t0;
  tnowtrain = millis() - t0train;

  
  // LED off
  if ((tnow > numsrem[0]) && (pulseon)){
    pulseon = false;

    // off
    resetpol();
  }

  // LED on
  if ((tnow >= numsrem[1]) && (!pulseon) && (numsrem[2] > 0) && (trainon) && (arm)){
    // led on
    pulseon = true;
    analogWrite(LEDPIN1, pwm);
    digitalWrite(LEDPIN3, HIGH);
    if (pol){
      digitalWrite(LEDPIN2, HIGH);
      digitalWrite(LEDonboard, HIGH);
    }
    else{
      digitalWrite(LEDPIN2, LOW);
      digitalWrite(LEDonboard, LOW);
    }
          
    // update pulse numbers
    numsrem[2] = numsrem[2] - 1;
    pulseon = true;
    t0 = millis();
  }


  // Reset train
  // Autoreset
  if ((tnowtrain > refractory) && (trainon) && (numsrem[2] == 0)){
    trainon = false;
  }
}
