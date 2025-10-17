#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Motor driver pins — change to your wiring
#define IN1 26
#define IN2 25
#define IN3 33
#define IN4 32

// BLE UUIDs — must match the app
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Forward declarations of motor functions
void forward();
void backward();
void left();
void right();
void stopBot();

// BLE globals
BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
bool deviceConnected = false;

// Callback for BLE server (connect/disconnect)
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("✅ Phone connected");
  }

  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("❌ Phone disconnected");
    pServer->getAdvertising()->start(); // Restart advertising
  }
};

// Callback for received BLE data
class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String rxValue = pCharacteristic->getValue(); // Arduino String
    if (rxValue.length() > 0) {
      char receivedChar = rxValue.charAt(0); // first character
      Serial.print("Received: ");
      Serial.println(receivedChar);

      switch (receivedChar) {
        case 'F': forward(); break;
        case 'B': backward(); break;
        case 'L': left(); break;
        case 'R': right(); break;
        case 'S': stopBot(); break;
        default: stopBot(); break;
      }
    }
  }
};

void setup() {
  Serial.begin(115200);

  // Motor pin setup
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  stopBot();

  // BLE setup
  BLEDevice::init("ESP32 Bot Controller");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE
                    );
  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->setValue("Ready");

  pService->start();
  pServer->getAdvertising()->start();
  Serial.println("🚀 Waiting for connection...");
}

void loop() {
  // Nothing needed here — all handled by callbacks
}

// ----------------- Motor functions -----------------
void forward() {
  Serial.println("Moving Forward");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(20)
  stop()
}

void backward() {
  Serial.println("Moving Backward");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  delay(20)
  stop()
}

void left() {
  Serial.println("Turning Left");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  delay(20)
  stop()
}

void right() {
  Serial.println("Turning Right");
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  delay(20)
  stop()
}

void stop() {
  Serial.println("Stopped");
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}
