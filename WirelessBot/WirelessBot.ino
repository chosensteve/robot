#include <Bluepad32.h>
#include <ESP32Servo.h>

#define ESC1_PIN  0
#define ESC2_PIN  2

#define ESC_MIN       1000
#define ESC_NEUTRAL   1500
#define ESC_MAX       2000

#define DEADBAND       30

Servo esc1;
Servo esc2;

ControllerPtr ctrl = nullptr;

int applyDeadband(int v) {
  return (abs(v) < DEADBAND) ? 0 : v;
}

int axisToPWM(int av) {
  av = constrain(av, -512, 511);
  return map(av, -512, 511, ESC_MIN, ESC_MAX);
}

void onConnectedController(ControllerPtr ctl) {
  ctrl = ctl;
  Serial.println("[BT] Xbox controller CONNECTED");
}

void onDisconnectedController(ControllerPtr ctl) {
  Serial.println("[BT] Xbox controller DISCONNECTED");
  ctrl = nullptr;
  esc1.writeMicroseconds(ESC_NEUTRAL);
  esc2.writeMicroseconds(ESC_NEUTRAL);
}

void setup() {
  Serial.begin(115200);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);

  esc1.setPeriodHertz(50);
  esc2.setPeriodHertz(50);

  esc1.attach(ESC1_PIN, ESC_MIN, ESC_MAX);
  esc2.attach(ESC2_PIN, ESC_MIN, ESC_MAX);

  esc1.writeMicroseconds(ESC_NEUTRAL);
  esc2.writeMicroseconds(ESC_NEUTRAL);

  delay(3000);

  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.forgetBluetoothKeys();
}

void loop() {
  bool upd = BP32.update();

  if (!upd || ctrl == nullptr || !ctrl->isConnected()) {
    esc1.writeMicroseconds(ESC_NEUTRAL);
    esc2.writeMicroseconds(ESC_NEUTRAL);
    return;
  }

  int thr = applyDeadband(-ctrl->axisY());
  int str = applyDeadband(ctrl->axisRX());

  int lSpd = constrain(thr + str, -512, 511);
  int rSpd = constrain(thr - str, -512, 511);

  int p1 = axisToPWM(lSpd);
  int p2 = axisToPWM(rSpd);

  esc1.writeMicroseconds(p1);
  esc2.writeMicroseconds(p2);

  static unsigned long lp = 0;
  if (millis() - lp > 200) {
    Serial.printf("[CTRL] Thr: %4d | Str: %4d | L: %d us | R: %d us\n", thr, str, p1, p2);
    lp = millis();
  }
}
