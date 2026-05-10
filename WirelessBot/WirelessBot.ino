/*
 * ============================================================
 *  Wireless Bot — ESP32 + Xbox Controller (Bluepad32)
 * ============================================================
 *
 *  HARDWARE CONNECTIONS
 *  ─────────────────────────────────────────────────────────
 *  ESP32 Pin   │  ESC Signal Wire
 *  ────────────┼─────────────────
 *    D3 (IO0)  │  ESC 1 (S1) — Left Motor
 *    D2 (IO2)  │  ESC 2 (S2) — Right Motor
 *  ─────────────────────────────────────────────────────────
 *
 *  POWER
 *  ─────────────────────────────────────────────────────────
 *  · ESC BEC (5 V red wire) → ESP32 VIN  (or separate 5 V BEC)
 *  · ESC GND (black wire)   → ESP32 GND  (MUST share ground)
 *  · LiPo / battery         → ESC main power leads
 *  ─────────────────────────────────────────────────────────
 *
 *  LIBRARY REQUIRED
 *  ─────────────────────────────────────────────────────────
 *  Bluepad32  →  Install via Arduino Library Manager
 *  (Supports Xbox One / Series S/X controllers via Bluetooth)
 *  ─────────────────────────────────────────────────────────
 *
 *  XBOX CONTROLLER MAPPING
 *  ─────────────────────────────────────────────────────────
 *  Left  Stick  Y-axis  →  Throttle  (forward / reverse)
 *  Right Stick  X-axis  →  Steering  (left / right turn)
 *  A Button             →  Enable / arm motors
 *  B Button             →  Emergency stop (disarm)
 *  ─────────────────────────────────────────────────────────
 *
 *  ESC SIGNAL SPEC
 *  ─────────────────────────────────────────────────────────
 *  Standard RC PWM  →  1000 µs (full reverse / brake)
 *                       1500 µs (neutral / stopped)
 *                       2000 µs (full forward)
 *  Frequency: 50 Hz (20 ms period)
 *  ─────────────────────────────────────────────────────────
 *
 *  ARMING SEQUENCE (important for most ESCs)
 *  1. Power on with sticks centred (neutral signal 1500 µs)
 *  2. Wait for ESC beeps / LED sequence
 *  3. Press A on the Xbox controller to arm
 *  ─────────────────────────────────────────────────────────
 */

#include <Bluepad32.h>
#include <ESP32Servo.h>   // ESP32-compatible Servo library

// ─────────────── Pin Definitions ────────────────────────────
#define ESC1_PIN  0   // D3 on most ESP32 dev boards = GPIO0
#define ESC2_PIN  2   // D2 on most ESP32 dev boards = GPIO2

// ─────────────── ESC PWM Limits (µs) ────────────────────────
#define ESC_MIN       1000   // Full reverse / brake
#define ESC_NEUTRAL   1500   // Stopped / neutral
#define ESC_MAX       2000   // Full forward

// ─────────────── Joystick Deadband ──────────────────────────
// Bluepad32 axis range: -512 to +511
#define DEADBAND       30    // Ignore small stick movements

// ─────────────── Servo Objects ──────────────────────────────
Servo esc1;   // Left  motor (S1 — D3)
Servo esc2;   // Right motor (S2 — D2)

// ─────────────── Controller Handle ─────────────────────────
ControllerPtr myController = nullptr;

// ─────────────── State ──────────────────────────────────────
bool armed = false;

// ─────────────── Utility: apply deadband ────────────────────
int applyDeadband(int value) {
  return (abs(value) < DEADBAND) ? 0 : value;
}

// ─────────────── Utility: map axis → PWM µs ─────────────────
// Bluepad32 axis: -512 … +511
// For throttle: negative stick = reverse, positive = forward
int axisToPWM(int axisValue) {
  axisValue = constrain(axisValue, -512, 511);
  return map(axisValue, -512, 511, ESC_MIN, ESC_MAX);
}

// ─────────────── Bluepad32 Callbacks ────────────────────────
void onConnectedController(ControllerPtr ctl) {
  myController = ctl;
  Serial.println("[BT] Xbox controller CONNECTED");
  Serial.println("     Press A to arm, B for emergency stop.");

  // Flash the controller LEDs to confirm connection
  ctl->setPlayerLEDs(1);
}

void onDisconnectedController(ControllerPtr ctl) {
  Serial.println("[BT] Xbox controller DISCONNECTED — motors stopped");
  myController = nullptr;
  armed = false;

  // Safety: send neutral signal to both ESCs
  esc1.writeMicroseconds(ESC_NEUTRAL);
  esc2.writeMicroseconds(ESC_NEUTRAL);
}

// ─────────────── Setup ──────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println("=================================");
  Serial.println(" Wireless Bot — ESP32 + Xbox BT");
  Serial.println("=================================");

  // Attach ESCs and send neutral signal first (ESC arming requirement)
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);

  esc1.setPeriodHertz(50);          // Standard 50 Hz RC signal
  esc2.setPeriodHertz(50);

  esc1.attach(ESC1_PIN, ESC_MIN, ESC_MAX);
  esc2.attach(ESC2_PIN, ESC_MIN, ESC_MAX);

  // Send neutral to allow ESCs to arm
  esc1.writeMicroseconds(ESC_NEUTRAL);
  esc2.writeMicroseconds(ESC_NEUTRAL);

  Serial.println("[ESC] Neutral signal sent — waiting for ESC arming beeps...");
  delay(3000);   // Give ESCs time to detect neutral and arm

  // Initialise Bluepad32
  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.forgetBluetoothKeys();       // Clear paired devices for fresh pairing

  Serial.println("[BT]  Bluepad32 initialised — put Xbox controller in pairing mode");
  Serial.println("      (Hold Xbox button + Pair button until LED blinks rapidly)");
}

// ─────────────── Main Loop ──────────────────────────────────
void loop() {
  // Fetch latest BT data
  bool dataUpdated = BP32.update();

  if (!dataUpdated || myController == nullptr || !myController->isConnected()) {
    // No controller — ensure motors are stopped
    if (!armed) {
      esc1.writeMicroseconds(ESC_NEUTRAL);
      esc2.writeMicroseconds(ESC_NEUTRAL);
    }
    return;
  }

  // ── Button: A = Arm, B = Emergency Stop ─────────────────
  if (myController->a()) {
    if (!armed) {
      armed = true;
      Serial.println("[BOT] ARMED — motors enabled");
      myController->setPlayerLEDs(0b1111);  // All LEDs on when armed
    }
  }

  if (myController->b()) {
    armed = false;
    Serial.println("[BOT] EMERGENCY STOP — motors disarmed");
    myController->setPlayerLEDs(1);  // Back to single LED
    esc1.writeMicroseconds(ESC_NEUTRAL);
    esc2.writeMicroseconds(ESC_NEUTRAL);
    return;
  }

  // ── Read Joystick Axes ───────────────────────────────────
  // Left  stick Y: forward / reverse  (axis Y is inverted on Xbox)
  // Right stick X: turn left / right
  int throttle = applyDeadband(-myController->axisY());   // Invert Y so push-up = forward
  int steering = applyDeadband(myController->axisRX());

  // ── Differential Drive Mixing ────────────────────────────
  //
  //   leftSpeed  = throttle + steering
  //   rightSpeed = throttle - steering
  //
  // This gives:
  //   • Forward only  →  both motors equal speed forward
  //   • Turn right    →  left motor faster, right motor slower
  //   • Turn left     →  right motor faster, left motor slower
  //   • Spin in place →  throttle=0, steering≠0

  int leftSpeed  = constrain(throttle + steering, -512, 511);
  int rightSpeed = constrain(throttle - steering, -512, 511);

  // ── Convert to PWM and Write to ESCs ────────────────────
  if (armed) {
    int pwm1 = axisToPWM(leftSpeed);
    int pwm2 = axisToPWM(rightSpeed);

    esc1.writeMicroseconds(pwm1);
    esc2.writeMicroseconds(pwm2);

    // Debug output at reasonable rate
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 200) {
      Serial.printf("[CTRL] Throttle: %4d | Steering: %4d | L-ESC: %d µs | R-ESC: %d µs\n",
                    throttle, steering, pwm1, pwm2);
      lastPrint = millis();
    }
  } else {
    // Disarmed — always neutral
    esc1.writeMicroseconds(ESC_NEUTRAL);
    esc2.writeMicroseconds(ESC_NEUTRAL);
  }
}
