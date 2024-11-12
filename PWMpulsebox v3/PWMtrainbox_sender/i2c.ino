// ping arm
void i2carm(void){
  bool input_bit, arm_bit;
  
  m = 7;
  n = 0;
  o = 0;
  i2csend();

  // Get bits
  arm_bit = p & 1;
  input_bit = (p >> 1) & 1;

  // Change arm status
  if ((arm_bit == 1) && !arm){
    arm = true;
    menu_redraw_required = true;
  }
  else if ((arm_bit == 0) && arm){
    arm = false;
    menu_redraw_required = true;
  }

  // Change input status
  if ((input_bit == 1) && !input_en){
    input_en = true;
    menu_redraw_required = true;
  }
  else if ((input_bit == 0) && input_en){
    input_en = false;
    menu_redraw_required = true;
  }
}

// Train function
// [        0,        1,        2,    3,           4,        5]
// [LED width, LED freq, N pulses, PWM, Train cycle, N trains]
void dotrain(void){
  tnowtrain = tnow - t0train;

  //
  if((tnow - StepTimer > 500) && trainon){         // output a temperature value per 500ms
    // Ping remaining pulses
    m = 4;
    n = 2;
    o = 0;
    i2csend();
    numsrem[2] = p * 2;

    m = 4;
    n = 5;
    o = 0;
    i2csend();
    numsrem[5] = p;
    
    menu_redraw_required = true; // force redraw every 0.5s anyway durign train
    StepTimer = tnow;
  }

  // Reset train
  // Autoreset
  if ((tnowtrain > refractory) && (trainon) && (numsrem[5] == 0)){
    // Ping train on
    m = 8;
    n = 0;
    o = 0;
    i2csend();

    if (p == 0){
      trainon = false;
      menu_redraw_required = true;
    }
  }
}


// i2c
void i2csend(void){
  // Send
  Wire.beginTransmission(4); // transmit to device #4
  i2csendbuf[0] = m;
  i2csendbuf[1] = n;
  i2csendbuf[2] = o;
  Wire.write(i2csendbuf, 3); // sends i2c byte
  Wire.endTransmission();    // stop transmitting
  delay(2); // Delay 2 ms

  // Get echo
  Wire.requestFrom(4, 1);
  while (Wire.available() == 0){
    // Wait for attiny to respond
  }
  p = Wire.read();
}

void i2csync_back(void){
  // back sync entire array
  m = 5;
  o = 0;
  
  for (i = 0; i < num_items; i++){
    // buffer
    n = i;
    i2csend();
    
    if (i == 2){
      // Change the number back
      nums[i] = p * pulsemod;
    }
    else if (i == 4){

      // Change the number back
      nums[i] = p * trainmod;
    }
    else{

      // Change the number back
      nums[i] = p;
    }
  }
}

void i2csync(void){
  // sync entire array
  m = 3;
  for (i = 0; i < num_items; i++){
    // buffer
    n = i;
    if (i == 2){
      o = nums[i] / pulsemod;
      
      // Transmit
      i2csend();

      // Change the number back
      nums[i] = p * pulsemod;
    }
    else if (i == 4){
      o = nums[i] / trainmod;
      
      // Transmit
      i2csend();

      // Change the number back
      nums[i] = p * trainmod;
    }
    else{
      o = nums[i];

      // Transmit
      i2csend();

      // Change the number back
      nums[i] = p;
    }
  }
}

void sendalwaysarm(void){
  m = 10;
  n = 1;
  o = 0;
  
  // Transmit
  i2csend();
}

void checkmode(void){
  m = 14;
  n = 0;
  o = 0;
  i2csend();
  receivermode = p;
}
