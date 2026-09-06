#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>

const char* ssid = "MULTY U";
const char* password = "Serhat12.";

WiFiServer server(80);
WiFiClient client;

static boolean doConnect = false;
static boolean connected = false;
static BLEAdvertisedDevice* myDevice = nullptr;
static BLERemoteCharacteristic* pRemoteCharacteristic = nullptr;

static BLEUUID serviceUUID("1812");
static BLEUUID charUUID("2A4D");

void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    if (length > 2 && pData[2] != 0) {
      uint8_t keyCode = pData[2];
      Serial.printf("[BLE] Tus: %d\n", keyCode);
      if (client && client.connected()) {
        client.printf("HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: text/plain\r\n\r\nKEY:%d", keyCode);
      }
    }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    connected = true;
    Serial.println("[BLE] MK370 Baglandi!");
  }
  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("[BLE] Baglanti Koptu!");
  }
};

bool connectToServer() {
    Serial.print("[BLE] Baglaniliyor: ");
    Serial.println(myDevice->getAddress().toString().c_str());

    BLEClient* pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    pClient->connect(myDevice);

    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (!pRemoteService) {
      pClient->disconnect();
      return false;
    }

    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (!pRemoteCharacteristic) {
      pClient->disconnect();
      return false;
    }

    if (pRemoteCharacteristic->canNotify()) {
      pRemoteCharacteristic->registerForNotify(notifyCallback);
    }

    connected = true;
    return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveName() && 
       (advertisedDevice.getName().find("MK370") != std::string::npos || 
        advertisedDevice.getName().find("Logi") != std::string::npos)) {
      Serial.printf("[BLE] Cihaz Bulundu: %s\n", advertisedDevice.getName().c_str());
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- ESP32-C3 Baslatiliyor ---");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n[WiFi] Baglandi!");
  Serial.print("[WiFi] IP Adresi: ");
  Serial.println(WiFi.localIP());

  server.begin();

  BLEDevice::init("ESP32-C3-Stream");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  Serial.println("[BLE] MK370 araniyor...");
  pBLEScan->start(5, false);
}

void loop() {
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("[BLE] Eslesti!");
    }
    doConnect = false;
  }

  WiFiClient newClient = server.available();
  if (newClient) {
    client = newClient;
  }
  delay(10);
}
