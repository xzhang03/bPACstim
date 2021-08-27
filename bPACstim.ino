const byte TriggerPin = 11;
const byte ArmPin = 12;
const byte LEDArm = 9;
const byte LEDPIN1 = 3; // Pin 3: 1 sec, full power
const byte LEDPIN2 = 4; // Pin 4: 2 sec, full power
const byte LEDPIN3 = 5; // Pin 5: 4 sec, full power
const byte LEDPIN4 = 6; // Pin 6: 2 sec, pwm
const byte LEDPIN5 = 9; // Pin 9: 100 ms, pwm
const byte LEDrefractory = 13; // onboard LED to show when refractory is on

const unsigned int LED1 = 1 * 1000; // Pin 3: 1 sec, full power
const unsigned int LED2 = 2 * 1000; // Pin 4: 2 sec, full power
const unsigned int LED3 = 4 * 1000; // Pin 5: 4 sec, full power
const unsigned int LED4 = 2 * 1000; // Pin 6: 2 sec, pwm
const unsigned int LED5 = 100; // Pin 9: 100 ms, pwm
const unsigned int LEDr = 10 * 1000; // Minimal 10 second every trial

// PWM
const int PWM = 205; //0 - 385,  250 - 12, 100 - 250, 150 - 185, 200 - 100, 210 - 80, 205 81, 

// Time
unsigned long ttrig = 0;
unsigned long tnow;

// logic
bool arm = false;
bool led = false;

void setup() {
  // put your setup code here, to run once:
  pinMode(TriggerPin, INPUT); 
  pinMode(ArmPin, INPUT); 
  
  pinMode(LEDArm, OUTPUT);
  pinMode(LEDPIN1, OUTPUT);
  pinMode(LEDPIN2, OUTPUT);
  pinMode(LEDPIN3, OUTPUT);
  pinMode(LEDPIN4, OUTPUT);
  // pinMode(LEDPIN5, OUTPUT);
  pinMode(LEDrefractory, OUTPUT);
  
  digitalWrite(LEDPIN1, HIGH);
  digitalWrite(LEDPIN2, HIGH);
  digitalWrite(LEDPIN3, HIGH);
  digitalWrite(LEDPIN4, HIGH);
//  digitalWrite(LEDPIN5, HIGH);
  digitalWrite(LEDrefractory, LOW);
}

void loop() {
  // Time
  tnow = millis();
  
  // Arming
  if (digitalRead(ArmPin) == HIGH){
    arm = true;
    analogWrite(LEDArm,50);
  }
  else{
    arm = false;
    analogWrite(LEDArm,0);
  }

  // Triggering
  if ((digitalRead(TriggerPin) == HIGH) && (arm == true) && (led == false)){
    led = true;
    ttrig = tnow;
    digitalWrite(LEDrefractory, HIGH);
  }
  if (led = true){
    // LED 1
    if ((tnow - ttrig) < LED1){
      digitalWrite(LEDPIN1, LOW);
    }
    else{
      digitalWrite(LEDPIN1, HIGH);
    }

    // LED 2
    if ((tnow - ttrig) < LED2){
      digitalWrite(LEDPIN2, LOW);
    }
    else{
      digitalWrite(LEDPIN2, HIGH);
    }

    // LED 3
    if ((tnow - ttrig) < LED3){
      digitalWrite(LEDPIN3, LOW);
    }
    else{
      digitalWrite(LEDPIN3, HIGH);
    }

    // LED 4
    /*
    if ((tnow - ttrig) < LED4){
      digitalWrite(LEDPIN4, LOW);
    }
    else{
      digitalWrite(LEDPIN4, HIGH);
    }
    */

    // LED 4
    if ((tnow - ttrig) < LED4){
      analogWrite(LEDPIN4, PWM);
    }
    else{
      analogWrite(LEDPIN4, 1023);
    }

    // LED 5
    if ((tnow - ttrig) < LED5){
      analogWrite(LEDPIN5, PWM);
    }
    else{
      analogWrite(LEDPIN5, 1023H);
    }

    // refractory
    
    if ((tnow - ttrig) > LEDr){
      // digitalWrite(LEDPIN5, HIGH);
      led = false;
    }
  }

  delay(10);
}
