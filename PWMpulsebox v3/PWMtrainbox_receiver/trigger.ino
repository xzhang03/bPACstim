
void check_arm(void){
  byte ledbright = 30;
  if (!alwaysarm){
    if ((digitalRead(ArmPin) == LOW)){
      // Armed
      arm = true;
      analogWrite(ArmLED, ledbright);
    }
    else{
      // Unarmed
      arm = false;
      analogWrite(ArmLED, 0);
      trainon = false;
  
      // Halt and catch fire
      if (trainon){
        resettrain();
      }
    }
  }
}

void check_trigin_en(void){
  byte ledbright = 30;
  if (digitalRead(FPin_en)){
    // Trigger in enabled
    input_en = true;
    analogWrite(FPin_enled, ledbright);
  }
  else{
    // Trigger in unenabled
    input_en = false;
    analogWrite(FPin_enled, 0);
  }
}

void check_trigin(void){
  if (input_en && (digitalRead(FPin) == 1)){
    if (arm && (numsrem[2] == 0)){
      numsrem[2] = nums[2];
      t0train = tnowtrain;
    }
  }
}
