void receiveEvent(int howMany){
  if (Wire.available() >= 3){
    m = Wire.read();
    n = Wire.read();
    o = Wire.read();
  }
  
  // [        0,        1,        2,        3,           4,        5]
  // [LED width, LED freq, N pulses, Ext trig, Train cycle, N trains]
  switch (m){
    case 1:
      // Start
      for( i = 0; i < num_items; i++ ) {
        numsrem[i] = nums[i]; // [LED width, Blank width, N pulses, Freq Mod, Train cycle, N trains]
      }
      numsrem[5] = numsrem[5] - 1;
      
      if (arm){
        trainon = true;
        t0train = tnowtrain;
      }
      
      o = 255; // for echo
      break;
      
    case 2:
      // Stop
      resettrain();
      o = 0; // for echo
      break;
      
    case 3:
      // Change single value
      switch (n){
        case 0:
          nums[n] = o;
          ledwidth = nums[0] * 1000;
          blankwidth = ledwidth + blank_gap;
          break;
        case 1:
          nums[n] = o;
          ledcycle = 1000000 / nums[1]; // in us
          break;
        case 2:
          // convert
          nums[n] = o * pulsemod;
          break;
        case 4:
          nums[n] = o * trainmod;  
          break;
        default:
          nums[n] = o;
          break;
      }

      break;

    case 10:
      // Change always arm
      if (n == 1){
        alwaysarm = true;
        arm = true;
        analogWrite(ArmLED, 30);
      }
      else{
        alwaysarm = false;
        arm = false;
        analogWrite(ArmLED, 0);
      }
      break;

    case 13:
      // change mods
      if (n == 0){
        pulsemod = o;
      }
      else if (n == 1){
        trainmod = o;
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
      if (n == 2){
        Wire.write(nums[n]/pulsemod);
      }
      else if (n == 4){
        Wire.write(nums[n]/trainmod);
      }
      else{
        Wire.write(nums[n]);
      }
      break;

    case 4:
      // ping single value in the running nums
      if (n == 2){
        Wire.write(numsrem[n]/pulsemod);
      }
      else if (n == 4){
        Wire.write(numsrem[n]/trainmod);
      }
      else{
        Wire.write(numsrem[n]);
      }
      break;
      
    case 5:
      // ping single value
      if (n == 2){
        Wire.write(nums[n]/pulsemod);
      }
      else if (n == 4){
        Wire.write(nums[n]/trainmod);
      }
      else{
        Wire.write(nums[n]);
      }
      break;

    case 6:
      // ping entire array
      for (i = 0; i < num_items; i++){
        if (n == 2){
          numsi2c[i] = nums[i] / pulsemod;
        }
        else if (n == 4){
          numsi2c[i] = nums[i] / trainmod;
        }
        else{
          numsi2c[i] = nums[i];
        }
      }
      Wire.write(numsi2c, num_items);
      break;

    case 7:
      // ping [input_en arm]
      Wire.write((input_en << 1) + arm);
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
      // Always arm
      if (alwaysarm){
        Wire.write(1);
      }
      else{
        Wire.write(0);
      }
      break;
      
    case 13:
      // Mods
      Wire.write(o);
      break;
      
    
  }
  // reset m;
  m = 0;
}
