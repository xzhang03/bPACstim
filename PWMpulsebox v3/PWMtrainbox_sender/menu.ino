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
      u8g.drawStr(1, i*h, trainon_strings[i]);
    }
    else{
      if (i < (MENU_ITEMS - 1)){
        // Non-bottom row
        u8g.drawStr(1, i*h, menu_strings[i]);
      }
      else{
        // Bottom row
        if (!arm && !input_en){
          // No arm, no input enable
          u8g.drawStr(1, i*h, "[UNARMED]");
        }
        else if (arm && !input_en){
          // Arm but no input enable
          u8g.drawStr(1, i*h, "ARMED! HOLD TO START");
        }
        else if (!arm && input_en){
          // Input enable but not arm
          u8g.drawStr(1, i*h, "[LISTENING Unarmed]");
        }
        else if (arm && input_en){
          // Input enable but not arm
          u8g.drawStr(1, i*h, "LISTENING ARMED!");
        }
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
        snprintf (buf, 9, "%d", numsrem[i]);
      }
      else {
        snprintf (buf, 9, "%d", nums[i]);
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
    // ======================================== DOWN ========================================
    case KEY_DOWN:
      if (!column_select){
                
        // Going up and down menu
        row_current++;
        if ( row_current >= MENU_ITEMS ){
          row_current = 0;
        }        
      }
      else{
        // change right hand side values
        // Not allowing anything == 0
        if ((nums[row_current] > numdiffs0[row_current]) && (nums[row_current] <= numdiffthresh0[row_current])){
          // only if number >= increment0 and number <= threshold0 and finemode, does the number go down as numdiffs0 (fine)
          nums[row_current] = nums[row_current] - numdiffs0[row_current];
        }
        else if ((nums[row_current] >= numdiffs[row_current]) && (nums[row_current] <= numdiffthresh[row_current])){
          // only if number >= increment and number <= threshold and number > threshold0, does the number go down as numdiffs (intermediate)
          nums[row_current] = nums[row_current] - numdiffs[row_current];
        }
        else if (nums[row_current] > numdiffs[row_current]){
          // if number > increment and number > threshold, does the number go down as numdiffs2 (large)
          nums[row_current] = nums[row_current] - numdiffs2[row_current];
        }
        
        // i2c
        m = 3;
        n = row_current;
        if (n == 2){
          o = nums[row_current] / pulsemod;
          i2csend();
          nums[row_current] = p * pulsemod;
        }
        else if (n == 4){
          o = nums[row_current] / trainmod;
          i2csend();
          nums[row_current] = p * trainmod;
        }
        else{
          o = nums[row_current];
          i2csend();
          nums[row_current] = p;
        }

      }
      // Serial.println(row_current);
      menu_redraw_required = true;
      break;

    // ======================================== UP ========================================
    case KEY_UP:
      if (!column_select){
        // Going up and down menu
        if ( row_current == 0 ){
          row_current = MENU_ITEMS;
        }
        row_current--;
      }
      else{

        if ((nums[row_current] < numdiffthresh0[row_current])){
          // If number < threshold0 and finemode, go up as numdiffs0 (fine)
          nums[row_current] = nums[row_current] + numdiffs0[row_current];
        }
        else if (nums[row_current] < numdiffthresh[row_current]){
          // If number < thresh and number >= threshold0, go up as numdiffs (intermediate)
          nums[row_current] = nums[row_current] + numdiffs[row_current];
        }
        else{
          // if number >= thresh go up as numdiffs2 (large)
          nums[row_current] = nums[row_current] + numdiffs2[row_current];

        }

        // Check to make sure PWM does not go above 255
        if (row_current == 3){
          if(nums[row_current] > 255){
              nums[row_current] = 255;
          }
        }
        
        // i2c
        m = 3;
        n = row_current;
        if (n == 2){
          o = nums[row_current] / pulsemod;
          i2csend();
          nums[row_current] = p * pulsemod;
        }
        else if (n == 4){
          o = nums[row_current] / trainmod;
          i2csend();
          nums[row_current] = p * trainmod;
        }
        else{
          o = nums[row_current];
          i2csend();
          nums[row_current] = p;
        }
      }
      // Serial.println(row_current);
      menu_redraw_required = true;
      break;

    // ======================================== SELECT ========================================
    case KEY_SELECT:
      if (column_select){
        // Going up and down menu
        row_current++;
        if ( row_current >= num_items ){
          row_current = 0;
        }        
      }
      else if (row_current == MENU_ITEMS - 1){
        if (!input_en){
          // Push button engage only works if not listening
          
          startbuttoncurrenttime = tnow;
          // First time start button down
          if (!startbuttondown){
            startbuttondowntime = startbuttoncurrenttime;
            startbuttondown = true;
  
            // Last min menu sync
  //          i2csync();
            
          }
          else if ((startbuttoncurrenttime - startbuttondowntime > startbuttonhold) && !trainon && arm){
            // ui start
            for( i = 0; i < num_items; i++ ) {
              numsrem[i] = nums[i]; // [LED width, Blank width, N pulses, Freq. Mod, Train cycle,   N trains]
            }
            
            // i2c start
            m = 1;
            n = 0;
            o = 0;
            i2csend();
            if (p == 255){
              refractory = nums[5] * 1000;
              trainon = true;
              t0train = tnow;
            }
          }
        }
      }
      menu_redraw_required = true;
      break;

    // ======================================== LEFT ========================================
    case KEY_LEFT:
      if (row_current < num_items){
        column_select = !column_select;
        menu_redraw_required = true;
      }
      // Serial.println(row_current);
      break;

    // ======================================== RIGHT ========================================
    case KEY_RIGHT:
      if (row_current < num_items){
        column_select = !column_select;
        menu_redraw_required = true;
      }
      // Serial.println(row_current);
      break;

    // ======================================== NONE ========================================
    case KEY_NONE:
      if (startbuttondown){
        startbuttondown = false;
        menu_redraw_required = true;
      }
      break;
  }
}

void initialdraw(void) {
  byte h;
  const char *s[5] = { "PWM TRAIN v3.0", "By Stephen Zhang", "Unknown mode", "SDA A4 | SCL A5"};
  char buf[4];

  // Get modes
  switch (receivermode){
    case 0:
      s[2] = "Autonomous mode";
      break;
    case 1:
      s[2] = "Semiauto mode";
      break;
    case 2:
      s[2] = "Passive mode";
      break;
  }
  
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

  // Line 4
  d = u8g.getStrWidth(s[2]);
  u8g.drawStr(1, 4 * h, s[2]);

  // Line 5
  d = u8g.getStrWidth(s[3]);
  u8g.drawStr(1, 5 * h, s[3]);
}
