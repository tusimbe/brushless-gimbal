/*************************/
/* RC-Decoder            */
/*************************/

// pinChange Int driven Functions
void intDecodePWMRoll()
{ 
  uint32_t microsNow = micros();
  uint16_t pulseInPWMtmp;
 
  if (PCintPort::pinState==HIGH)
  {
    microsRisingEdgeRoll = microsNow;
  }
  else
  {
    pulseInPWMtmp = (microsNow - microsRisingEdgeRoll)/CC_FACTOR;
    if ((pulseInPWMtmp >= MIN_RC) && (pulseInPWMtmp <= MAX_RC)) 
    {
      // update if within expected RC range
      pulseInPWMRoll = pulseInPWMtmp;
      microsLastPWMRollUpdate = microsNow;
      validRCRoll=true;
      updateRCRoll=true;
    }
  }
}

void intDecodePWMPitch()
{ 
  uint32_t microsNow = micros();
  uint16_t pulseInPWMtmp;

  if (PCintPort::pinState==HIGH)
  {
    microsRisingEdgePitch = microsNow;
  }
  else
  {
    pulseInPWMtmp = (microsNow - microsRisingEdgePitch)/CC_FACTOR;
    if ((pulseInPWMtmp >= MIN_RC) && (pulseInPWMtmp <= MAX_RC)) 
    {
      // update if within expected RC range
      pulseInPWMPitch = pulseInPWMtmp;
      microsLastPWMPitchUpdate = microsNow;
      validRCPitch=true;
      updateRCPitch=true;
    }
  }
}

void checkPWMRollTimeout()
{
  uint32_t microsNow = micros();
  if (((microsNow - microsLastPWMRollUpdate)/CC_FACTOR) > RC_PPM_TIMEOUT) 
  {
    pulseInPWMRoll = MID_RC;
    microsLastPWMRollUpdate = microsNow;
    validRCRoll=false;
    updateRCRoll=true;
  }
}

void checkPWMPitchTimeout()
{
  uint32_t microsNow = micros();
  if (((microsNow - microsLastPWMPitchUpdate)/CC_FACTOR) > RC_PPM_TIMEOUT) 
  {
    pulseInPWMPitch = MID_RC;
    microsLastPWMPitchUpdate = microsNow;
    validRCPitch=false;
    updateRCPitch=true;
  }
}

void initRCPins()
{  
  pinMode(RC_PIN_ROLL, INPUT); digitalWrite(RC_PIN_ROLL, HIGH);
  PCintPort::attachInterrupt(RC_PIN_ROLL, &intDecodePWMRoll, CHANGE);
  pinMode(RC_PIN_PITCH, INPUT); digitalWrite(RC_PIN_PITCH, HIGH);
  PCintPort::attachInterrupt(RC_PIN_PITCH, &intDecodePWMPitch, CHANGE);
}

// Proportional RC control
void evaluateRCSignalProportional()
{
  #define RCSTOP_ANGLE 5.0

  if(updateRCPitch==true)
  {
    pulseInPWMPitch = constrain(pulseInPWMPitch,MIN_RC,MAX_RC);
    if(pulseInPWMPitch>=MID_RC+RC_DEADBAND)
    {
      pitchRCSpeed = 0.1 * (float)(pulseInPWMPitch - (MID_RC + RC_DEADBAND))/ (float)(MAX_RC - (MID_RC + RC_DEADBAND)) + 0.9 * pitchRCSpeed;
    }
    else if(pulseInPWMPitch<=MID_RC-RC_DEADBAND)
    {
      pitchRCSpeed = -0.1 * (float)((MID_RC - RC_DEADBAND) - pulseInPWMPitch)/ (float)((MID_RC - RC_DEADBAND)-MIN_RC) + 0.9 * pitchRCSpeed;
    }
    else pitchRCSpeed = 0.0;
    // if((angle[PITCH] <= (config.minRCPitch+RCSTOP_ANGLE))||(angle[ROLL]>=(config.maxRCPitch-RCSTOP_ANGLE))) pitchRCSpeed = 0.0;
    if((angle[PITCH] <= (config.minRCPitch+RCSTOP_ANGLE))&&(pitchRCSpeed > 0.0))pitchRCSpeed = 0.0;
    if((angle[PITCH] >= (config.maxRCPitch-RCSTOP_ANGLE))&&(pitchRCSpeed < 0.0))pitchRCSpeed = 0.0;
    updateRCPitch=false;
  }
  if(updateRCRoll==true)
  {
    pulseInPWMRoll = constrain(pulseInPWMRoll,MIN_RC,MAX_RC);
    if(pulseInPWMRoll>=MID_RC+RC_DEADBAND)
    {
      rollRCSpeed = 0.1 * (float)(pulseInPWMRoll - (MID_RC + RC_DEADBAND))/ (float)(MAX_RC - (MID_RC + RC_DEADBAND)) + 0.9 * rollRCSpeed;
    }
    else if(pulseInPWMRoll<=MID_RC-RC_DEADBAND)
    {
      rollRCSpeed = -0.1 * (float)((MID_RC - RC_DEADBAND) - pulseInPWMRoll)/ (float)((MID_RC - RC_DEADBAND)-MIN_RC) + 0.9 * rollRCSpeed;
    }
    else rollRCSpeed = 0.0;
    //if((angle[ROLL] <= (config.minRCRoll+RCSTOP_ANGLE))||(angle[ROLL]>=(config.maxRCRoll-RCSTOP_ANGLE))) rollRCSpeed = 0.0;
    if((angle[ROLL] <= (config.minRCRoll+RCSTOP_ANGLE))&&(rollRCSpeed > 0.0))rollRCSpeed = 0.0;
    if((angle[ROLL] >= (config.maxRCRoll-RCSTOP_ANGLE))&&(rollRCSpeed < 0.0))rollRCSpeed = 0.0;
    updateRCRoll=false;
  }
}

// Absolute RC control
void evaluateRCSignalAbsolute()
{
  // Get Setpoint from RC-Channel if available.
  // LPF on pitchSetpoint
  if(updateRCPitch==true)
  {
    pulseInPWMPitch = constrain(pulseInPWMPitch,MIN_RC,MAX_RC);
    pitchRCSetpoint = 0.1 * (config.minRCPitch + (float)(pulseInPWMPitch - MIN_RC)/(float)(MAX_RC - MIN_RC) * (config.maxRCPitch - config.minRCPitch)) + 0.9 * pitchRCSetpoint;
    updateRCPitch=false;
  }
  if(updateRCRoll==true)
  {
    pulseInPWMRoll = constrain(pulseInPWMRoll,MIN_RC,MAX_RC);
    rollRCSetpoint = 0.1 * (config.minRCRoll + (float)(pulseInPWMRoll - MIN_RC)/(float)(MAX_RC - MIN_RC) * (config.maxRCRoll - config.minRCRoll)) + 0.9 * rollRCSetpoint;
    updateRCRoll=false;
  }
}
















