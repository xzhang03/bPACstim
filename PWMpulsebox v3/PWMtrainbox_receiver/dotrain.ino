// Train function
// [        0,        1,        2,        3,           4,        5]
// [LED width, LED freq, N pulses, Ext trig, Train cycle, N trains]
void dotrain(void){

  // Turn off pulse
  if ((pulseon) && ((tnow - t0) >= ledwidth)){
    digitalWrite(TrigPin, LOW);
    digitalWrite(onboardLED, LOW);
    analogWrite(LEDPIN1, 255);
    pulseon = false;
  }

  // Turn off blank
  if ((!pulseon) && (blankon) && ((tnow - t0) >= blankwidth)){
    // Safety. Don't turn off unless pulse is off
    digitalWrite(BlankPin, LOW);
    blankon = false;
  }

  // Turn on pulse (internal and external)
  if ((!pulseon) && (arm) && (numsrem[2] > 0) && ((tnow - t0) >= ledcycle)){
    #if autonomousmode
      // Turn on pulses immediatedly
      turnpulseon();
    #endif
    #if passivemode
      // Turn on pulses immediatedly
      turnpulseon();
    #endif
    #if semiautomode
      // Wait for frame pulse
      semiautoarm = true;
    #endif
    
  }

  // Semiautomode: pulse when frame pulse is on
  #if semiautomode
    if (semiautoarm && (digitalRead(FPin) == trigin_activehigh)){
      turnpulseon();
      semiautoarm = false;
    }
  #endif

  // Refill pulses (internal only here)
  if ((numsrem[2] == 0) && (numsrem[5] > 0) && trainon){
    if ((tnowtrain - t0train) >= (numsrem[4] * 1000)){
      refill_pulses();
    }
  }
  
  // Autoreset
  if ((!pulseon) && (numsrem[2] == 0) && (numsrem[5] == 0)){
    resettrain();
  }
}

void resettrain(void){
  trainon = false;  
}

// Turning pulse on
void turnpulseon(void){
  // Type
  byte LED_power = nums[3];
  
  // Blank
  digitalWrite(BlankPin, HIGH);
  blankon = true;
  
  // Turn on pulses immediatedly
  numsrem[2] = numsrem[2] - 1;
  digitalWrite(TrigPin, HIGH);
  digitalWrite(onboardLED, HIGH);
  analogWrite(LEDPIN1, LED_power);

  // Switches and counters
  pulseon = true;
  t0 = tnow;

  #if debug
    pulse_serial();
  #endif
}

// Refill pulses
void refill_pulses(void){
  if (arm){
    // Armed + (train on or external trigger mode)
    numsrem[2] = nums[2];
    numsrem[5] = numsrem[5] - 1;
    t0train = tnowtrain;
  }
}

// Debug
void pulse_serial(void){
  #if debug
    Serial.print("T: ");
    Serial.print(tnowtrain/1000);
    Serial.print(" Train on: ");
    Serial.print(trainon);
    Serial.print(" Arm on: ");
    Serial.print(arm);
    Serial.print(" Trigin on: ");
    Serial.print(input_en);
    Serial.print(" Pulse on: ");
    Serial.print(pulseon);
    Serial.print(" numsrem: ");
    Serial.print(numsrem[0]);
    Serial.print(" ");
    Serial.print(numsrem[1]);
    Serial.print(" ");
    Serial.print(numsrem[2]);
    Serial.print(" ");
    Serial.print(numsrem[3]);
    Serial.print(" ");
    Serial.print(numsrem[4]);
    Serial.print(" ");
    Serial.println(numsrem[5]);
  #endif
}
