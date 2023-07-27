#include <Wire.h>

// const byte TriggerPin = 11;
const byte ArmPin = 12;
const byte LEDArm = 9;
const byte LEDPIN1 = 5; // Pin 5: PWM
const byte LEDPIN2 = 6; // Pin 6: PWM
const byte LEDonboard = 13; // onboard LED to show when Stim is on

// PWM
int pwm = 205; //0 - 385,  250 - 12, 100 - 250, 150 - 185, 200 - 100, 210 - 80, 205 81, 

// polarity (1 - pos, 0 - neg)
bool pol = 0;

// logic
bool arm = false;
bool led = false;

// Serial
int m, n; // receive
int o = 0; // send

void setup() {
  // put your setup code here, to run once:
  // pinMode(TriggerPin, INPUT); 
  pinMode(ArmPin, INPUT_PULLUP); 
  pinMode(LEDArm, OUTPUT);
  pinMode(LEDPIN1, OUTPUT);
  pinMode(LEDPIN2, OUTPUT);
  
  if (pol){
    // Positive pol
    analogWrite(LEDPIN1, 0);
    digitalWrite(LEDPIN2, LOW);
    digitalWrite(LEDonboard, LOW);
  }
  else{
    // Negative pol
    analogWrite(LEDPIN1, 255);
    digitalWrite(LEDPIN2, HIGH);
    digitalWrite(LEDonboard, HIGH);
  }

  Wire.begin(4);                // join i2c bus with address #4
  Wire.onReceive(receiveEvent); // register event
  Wire.onRequest(requestEvent); // request event
}

void loop() {
  
  // Arming
  if (digitalRead(ArmPin) == LOW){
    arm = true;
    o = 1;
    analogWrite(LEDArm,50);
  }
  else{
    arm = false;
    o = 0;
    analogWrite(LEDArm,0);

    // Halt and catch fire
    if (led){
      // Stop
      led = false;
      if (pol){
        analogWrite(LEDPIN1, 0);
        digitalWrite(LEDPIN2, LOW);
        digitalWrite(LEDonboard, LOW);
      }
      else{
        analogWrite(LEDPIN1, 255);
        digitalWrite(LEDPIN2, HIGH);
        digitalWrite(LEDonboard, HIGH);
      }
    }
  }
  delay(10);
}

void receiveEvent(int howMany){
  if (Wire.available() >= 2){
    m = Wire.read();
    n = Wire.read();
  }

  switch (m){
    case 1:
      // Start
      if (arm){
        led = true;
        analogWrite(LEDPIN1, pwm);
        if (pol){
          digitalWrite(LEDPIN2, HIGH);
          digitalWrite(LEDonboard, HIGH);
        }
        else{
          digitalWrite(LEDPIN2, LOW);
          digitalWrite(LEDonboard, LOW);
        }
      }
      break;
    case 0:
      // Stop
      led = false;
      if (pol){
        analogWrite(LEDPIN1, 0);
        digitalWrite(LEDPIN2, LOW);
        digitalWrite(LEDonboard, LOW);
      }
      else{
        analogWrite(LEDPIN1, 255);
        digitalWrite(LEDPIN2, HIGH);
        digitalWrite(LEDonboard, HIGH);
      }
      break;
    case 2:
      // Change polarity
      if (n == 0){
        pol = false;
        analogWrite(LEDPIN1, 255);
        digitalWrite(LEDPIN2, HIGH);
        digitalWrite(LEDonboard, HIGH);
      }
      else{
        pol = true;
        analogWrite(LEDPIN1, 0);
        digitalWrite(LEDPIN2, LOW);
        digitalWrite(LEDonboard, LOW);
      }
      break;
      
    case 20:
      // Ping polarity
      Wire.write(pol);
      break;
      
    case 3:
      // Change pwm
      pwm = n;
      break;
      
    case 30:
      // Ping pwm
      Wire.write(pwm);
      break;
  }
}

void requestEvent() {
  Wire.write(o); // respond with message of 1 byte (arm)
}
