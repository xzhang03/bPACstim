void uiStep(void) {
  // Only draw input once every 50 ms
  if ((tnow - InputStepTimer) < 50){
    return;
  }
  InputStepTimer = tnow;
  
  uiKeyCodeSecond = uiKeyCodeFirst;

  val = analogRead(A0); // read the analog in value:
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
