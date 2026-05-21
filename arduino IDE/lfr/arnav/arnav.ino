//  CAL          — sensor calibration (sweep over line)
//  START        — start PID line following
//  STOP         — stop motors, halt PID
//
//  PID tuning (value is float, e.g. KP2.5):
//  KP<val>      — set Kp    e.g. KP1.8
//  KI<val>      — set Ki    e.g. KI0.002
//  KD<val>      — set Kd    e.g. KD8.5
//  SP<val>      — set base speed  e.g. SP120
//
//  STATIC       — static self-correction test
//                 Bot sits still, you push it off line,
//                 it corrects back using PID output
//
//  TUNE         — print current PID values
//  READ         — live sensor + PID debug stream
//  S            — stop everything
//  PING         — BT link test

#include <Arduino.h>

#define BT      Serial2
#define BT_BAUD 9600


const uint8_t SENSOR_PIN[7] = { 20, 19, 18, 17, 16, 15, 14 };
#define NUM_SENSORS 7
#define IN1 24
#define IN2 25
#define IN3 37
#define IN4 36
#define ENC1_A 2
#define ENC1_B 3
#define ENC2_A 4
#define ENC2_B 5
#define BUZZER 0
float Kp        = 2.0f;
float Ki        = 0.002f;
float Kd        = 0.5f;
int   baseSpeed = 120;      // 0-255 cruise speed
int   maxSpeed  = 200;      // motor output clamp
int   minSpeed  = -80;      // allow slight reverse for sharp turns
int   calMin[NUM_SENSORS];   // darkest (black)
int   calMax[NUM_SENSORS];   // lightest (white)
bool  calibrated = false;
const int WEIGHT[7] = { -3, -2, -1, 0, 1, 2, 3 };
volatile long enc1 = 0, enc2 = 0;

float pidError    = 0;
float pidLastErr  = 0;
float pidIntegral = 0;
float pidOutput   = 0;

bool  running     = false;   
bool  staticMode  = false;   
bool  liveStream  = false;
bool  flipDir     = false;

unsigned long lastPidTime  = 0;
unsigned long lastPrintTime= 0;
#define PID_INTERVAL_MS   10   
#define PRINT_INTERVAL_MS 150   

void FASTRUN isr1(){ digitalRead(ENC1_B)?enc1++:enc1--; }
void FASTRUN isr2(){ digitalRead(ENC2_B)?enc2++:enc2--; }

void out  (const String& s){ Serial.print(s);   BT.print(s);   }
void outln(const String& s){ Serial.println(s); BT.println(s); }
void outln()               { Serial.println();  BT.println();  }

void setM1(int s){
  s = constrain(s, -255, 255);
  if      (s>0){ analogWrite(IN1,0);  analogWrite(IN2,s);  }
  else if (s<0){ analogWrite(IN1,-s); analogWrite(IN2,0);  }
  else         { analogWrite(IN1,0);  analogWrite(IN2,0);  }
}
void setM2(int s){
  s = constrain(s, -255, 255);
  if      (s>0){ analogWrite(IN3,s);  analogWrite(IN4,0);  }
  else if (s<0){ analogWrite(IN3,0);  analogWrite(IN4,-s); }
  else         { analogWrite(IN3,0);  analogWrite(IN4,0);  }
}
void motorStop(){ setM1(0); setM2(0); }

void beep(int n=1,int ms=80){
  for(int i=0;i<n;i++){
    digitalWrite(BUZZER,HIGH);delay(ms);
    digitalWrite(BUZZER,LOW);
    if(i<n-1)delay(40);
  }
}

void readNorm(int* norm){
  for(int i=0;i<NUM_SENSORS;i++){
    int raw = analogRead(SENSOR_PIN[i]);
    if(!calibrated){ norm[i]=0; continue; }
    // clamp to calibrated range
    raw = constrain(raw, calMin[i], calMax[i]);
    // map: calMin=white(0)  calMax=black(1000)
    norm[i] = map(raw, calMin[i], calMax[i], 0, 1000);
  }
}

float getPosition(){
  int norm[NUM_SENSORS];
  readNorm(norm);

  long weightedSum = 0;
  long totalSum    = 0;

  for(int i=0;i<NUM_SENSORS;i++){
    weightedSum += (long)norm[i] * WEIGHT[i];
    totalSum    += norm[i];
  }

  if(totalSum < 100){
   
    return pidLastErr;
  }

  return (float)weightedSum / (float)totalSum;
}


void pidStep(){
  unsigned long now = millis();
  float dt = (now - lastPidTime) / 1000.0f;
  if(dt <= 0) dt = 0.01f;
  lastPidTime = now;

  float position = getPosition();   // -3 to +3 (weighted avg)
  pidError = position;              // setpoint = 0

  // Integral with windup clamp
  pidIntegral += pidError * dt;
  pidIntegral  = constrain(pidIntegral, -500.0f, 500.0f);

  float derivative = (pidError - pidLastErr) / dt;
  pidLastErr = pidError;

  pidOutput = Kp * pidError
            + Ki * pidIntegral
            + Kd * derivative;

  float corrected = flipDir ? -pidOutput : pidOutput;

  if(staticMode){
    
    if(abs(pidError) > 0.3f){
      int correction = (int)constrain(corrected * 60.0f, -200.0f, 200.0f);
      setM1(baseSpeed / 2 + correction);
      setM2(baseSpeed / 2 - correction);
    } else {
      motorStop();
    }
  } else {
    int steer      = (int)constrain(corrected * 60.0f, -200.0f, 200.0f);
    int leftSpeed  = baseSpeed + steer;
    int rightSpeed = baseSpeed - steer;
    leftSpeed  = constrain(leftSpeed,  minSpeed, maxSpeed);
    rightSpeed = constrain(rightSpeed, minSpeed, maxSpeed);
    setM1(leftSpeed);
    setM2(rightSpeed);
  }
}

void printLive(){
  int norm[NUM_SENSORS];
  readNorm(norm);

  // Sensor bar
  out("S: ");
  for(int i=0;i<NUM_SENSORS;i++){
    if(norm[i]>700)      out("[###]");
    else if(norm[i]>300) out("[-#-]");
    else                 out("[   ]");
  }

  // Position + PID
  out("  Pos:");
  char buf[8]; dtostrf(getPosition(),6,2,buf); out(buf);
  out("  Err:");  dtostrf(pidError,6,2,buf);   out(buf);
  out("  Out:");  dtostrf(pidOutput,6,2,buf);  out(buf);
  out("  I:");    dtostrf(pidIntegral,6,2,buf);out(buf);
  outln();
}


#define CAL_DURATION_MS 5000   // 5 seconds to sweep

void calibrate(){
  outln("==========================================");
  outln("  SENSOR CALIBRATION");
  outln("==========================================");
  outln("Sweep the sensor array slowly back and");
  outln("forth over the line for 5 seconds.");
  outln("Starting in 3s...");
  delay(3000);
  beep(2,80);

  int raw[NUM_SENSORS];
  for(int i=0;i<NUM_SENSORS;i++){
    raw[i]    = analogRead(SENSOR_PIN[i]);
    calMin[i] = raw[i];
    calMax[i] = raw[i];
  }

  unsigned long t0 = millis();
  unsigned long lastDot = 0;
  out("Sampling");

  while(millis()-t0 < CAL_DURATION_MS){
    for(int i=0;i<NUM_SENSORS;i++){
      int v = analogRead(SENSOR_PIN[i]);
      if(v < calMin[i]) calMin[i]=v;
      if(v > calMax[i]) calMax[i]=v;
    }
    if(millis()-lastDot > 500){ out("."); lastDot=millis(); }
    delay(2);
  }

  outln(" Done!");
  outln("");
  outln("Results (min=white  max=black):");
  for(int i=0;i<NUM_SENSORS;i++){
    out("  S"); out(String(i));
    out("  min="); out(String(calMin[i]));
    out("  max="); out(String(calMax[i]));
    out("  range="); outln(String(calMax[i]-calMin[i]));
  }


  outln("");
  bool ok = true;
  for(int i=0;i<NUM_SENSORS;i++){
    if(calMax[i]-calMin[i] < 100){
      out("  WARNING S"); out(String(i)); outln(" — small range, may not see line");
      ok=false;
    }
  }

  calibrated = true;
  beep(ok?3:2, 80);
  outln("");
  outln("Calibration saved.");
  outln("Send START to follow line, or STATIC to test self-correction.");
  outln("==========================================");
}

void printTune(){
  outln("--- PID Parameters ---");
  char buf[10];
  out("  Kp        = "); dtostrf(Kp,7,4,buf); outln(buf);
  out("  Ki        = "); dtostrf(Ki,7,4,buf); outln(buf);
  out("  Kd        = "); dtostrf(Kd,7,4,buf); outln(buf);
  out("  BaseSpeed = "); outln(String(baseSpeed));
  out("  MaxSpeed  = "); outln(String(maxSpeed));
  out("  MinSpeed  = "); outln(String(minSpeed));
  out("  Calibrated= "); outln(calibrated?"YES":"NO");
  out("  Running   = "); outln(running?"YES":"NO");
  out("  StaticMode= "); outln(staticMode?"YES":"NO");
  out("  FlipDir   = "); outln(flipDir?"YES (inverted)":"NO (normal)");
  outln("----------------------");
}

void processCommand(String cmd){
  cmd.trim(); cmd.replace("\r",""); cmd.replace("\n",""); cmd.toUpperCase();
  if(!cmd.length()) return;


  if(cmd=="PING"){ outln("PONG — link OK"); beep(1,50); return; }

  if(cmd=="CAL"){
    running=false; staticMode=false; motorStop();
    calibrate();
    return;
  }

  if(cmd=="TUNE"){ printTune(); return; }

  if(cmd=="READ"){
    if(!calibrated){ outln("Run CAL first."); return; }
    liveStream=true;
    outln("Live stream ON — send S to stop");
    return;
  }

  if(cmd=="S"||cmd=="STOP"){
    running=false; staticMode=false; liveStream=false;
    motorStop();
    pidIntegral=0; pidLastErr=0;
    outln("Stopped.");
    return;
  }

  if(cmd=="START"){
    if(!calibrated){ outln("Run CAL first."); return; }
    staticMode=false;
    pidIntegral=0; pidLastErr=0;
    running=true;
    lastPidTime=millis();
    outln("PID line following STARTED.");
    outln("Send STOP or S to halt.");
    beep(2,80);
    return;
  }

  if(cmd=="STATIC"){
    if(!calibrated){ outln("Run CAL first."); return; }
    running=false; motorStop();
    pidIntegral=0; pidLastErr=0;
    staticMode=true;
    lastPidTime=millis();
    outln("==========================================");
    outln("  STATIC SELF-CORRECTION MODE");
    outln("  Bot is stationary on the line.");
    outln("  Push it sideways with your hand.");
    outln("  It will nudge itself back to centre.");
    outln("  Send STOP or S to exit.");
    outln("==========================================");
    beep(3,60);
    return;
  }

  if(cmd=="FLIP"){
    flipDir = !flipDir;
    out("Correction direction FLIPPED. flipDir=");
    outln(flipDir?"TRUE (inverted)":"FALSE (normal)");
    outln("If bot still diverges, send FLIP again.");
    return;
  }


  if(cmd.startsWith("KP") && cmd.length()>2){
    Kp = cmd.substring(2).toFloat();
    out("Kp = "); outln(String(Kp,4));
    return;
  }
  if(cmd.startsWith("KI") && cmd.length()>2){
    Ki = cmd.substring(2).toFloat();
    out("Ki = "); outln(String(Ki,6));
    return;
  }
  if(cmd.startsWith("KD") && cmd.length()>2){
    Kd = cmd.substring(2).toFloat();
    out("Kd = "); outln(String(Kd,4));
    return;
  }
  if(cmd.startsWith("SP") && cmd.length()>2){
    baseSpeed = cmd.substring(2).toInt();
    baseSpeed = constrain(baseSpeed, 0, 220);
    out("BaseSpeed = "); outln(String(baseSpeed));
    return;
  }

  out("?? Unknown: "); outln(cmd);
}

void setup(){
  analogReadResolution(10);

  for(int i=0;i<NUM_SENSORS;i++) pinMode(SENSOR_PIN[i],INPUT);

  pinMode(IN1,OUTPUT); pinMode(IN2,OUTPUT);
  pinMode(IN3,OUTPUT); pinMode(IN4,OUTPUT);
  motorStop();

  pinMode(ENC1_A,INPUT_PULLUP); pinMode(ENC1_B,INPUT_PULLUP);
  pinMode(ENC2_A,INPUT_PULLUP); pinMode(ENC2_B,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ENC1_A),isr1,RISING);
  attachInterrupt(digitalPinToInterrupt(ENC2_A),isr2,RISING);

  pinMode(BUZZER,OUTPUT); digitalWrite(BUZZER,LOW);

  BT.begin(BT_BAUD);
  Serial.begin(115200);

  // Default cal (uncalibrated fallback)
  for(int i=0;i<NUM_SENSORS;i++){
    calMin[i]=100; calMax[i]=900;
  }

  delay(300);
  beep(2,100);

  outln("==========================================");
  outln("  Teensy 4.1  PID Line Follower");
  outln("  Default: Kp=1.5 Ki=0.002 Kd=8.0");
  outln("");
  outln("  CAL    — calibrate sensors");
  outln("  START  — start line following");
  outln("  STATIC — static self-correct test");
  outln("  STOP/S — stop");
  outln("  KP1.8  KI0.002  KD8.5  SP120");
  outln("  FLIP   — invert correction if bot diverges");
  outln("  TUNE   — show PID values");
  outln("  READ   — live sensor stream");
  outln("  PING   — BT test");
  outln("==========================================");
  outln("Send CAL first!");
}

// ============================================================
//  LOOP
// ============================================================
String btBuf="", usBuf="";

void readPort(Stream& src, String& buf){
  while(src.available()){
    char c=src.read();
    if(&src==(Stream*)&BT) Serial.write(c);
    if(c=='\n'||c=='\r'){
      if(buf.length()){ processCommand(buf); buf=""; }
    } else buf+=c;
  }
}

void loop(){
  readPort(BT,    btBuf);
  readPort(Serial,usBuf);

  unsigned long now=millis();

  // PID loop
  if((running||staticMode) && now-lastPidTime>=PID_INTERVAL_MS){
    pidStep();
  }

  // Live debug stream
  if(liveStream && now-lastPrintTime>=PRINT_INTERVAL_MS){
    lastPrintTime=now;
    printLive();
  }
}