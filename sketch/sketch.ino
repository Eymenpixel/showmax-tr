#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEClient.h>

// Wi-Fi Bilgileri
const char* ssid = "MULTY U";
const char* password = "Serhat12.";

// WebSocket & Web Sunucu
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// BLE Durumu
static boolean doConnect = false;
static boolean connected = false;
static BLEAdvertisedDevice* myDevice = nullptr;
static BLERemoteCharacteristic* pRemoteCharacteristic = nullptr;

// HID Service & Report UUID (Standart BLE Klavye)
static BLEUUID serviceUUID("1812");
static BLEUUID charUUID("2A4D");

void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    if (length > 2 && pData[2] != 0) {
      uint8_t keyCode = pData[2];
      char msg[32];
      snprintf(msg, sizeof(msg), "KEY:%d", keyCode);
      ws.textAll(msg);
      Serial.printf("[BLE] Tus Kodu: %d -> WS Gonderildi\n", keyCode);
    }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    connected = true;
    Serial.println("[BLE] MK370 Klavye Baglandi!");
  }
  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("[BLE] Klavye Baglantisi Koptu!");
  }
};

bool connectToServer() {
    Serial.print("[BLE] Baglaniliyor: ");
    Serial.println(myDevice->getAddress().toString().c_str());

    BLEClient* pClient = BLEDevice::createClient();
    pClient->setClientCallbacks(new MyClientCallback());
    pClient->connect(myDevice);

    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      pClient->disconnect();
      return false;
    }

    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      pClient->disconnect();
      return false;
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

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

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("[WS] Tarayici baglandi: ID %u\n", client->id());
    client->text("ESP32_CONNECTED");
  } else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("[WS] Tarayici ayrildi: ID %u\n", client->id());
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- ESP32-C3 Baslatiliyor ---");

  // Wi-Fi Baglantisi
  Serial.printf("Wi-Fi Baglaniliyor: %s\n", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  // Serial Monitore IP Yazdirma
  Serial.println("\n[WiFi] Baglanti Basarili!");
  Serial.print("[WiFi] ESP32 IP Adresi: ");
  Serial.println(WiFi.localIP());
  Serial.print("[WiFi] WebSocket URL: ws://");
  Serial.print(WiFi.localIP());
  Serial.println("/ws");

  // WebSocket Baslatma
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  server.begin();
  Serial.println("[WS] Sunucu Aktif (Port: 80)");

  // BLE Baslatma
  BLEDevice::init("ESP32-C3-Stream");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  Serial.println("[BLE] MK370 Klavye araniyor...");
  pBLEScan->start(5, false);
}

void loop() {
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("[BLE] Eslestirme Basarili!");
    } else {
      Serial.println("[BLE] Baglanti Kurulamadi.");
    }
    doConnect = false;
  }
  ws.cleanupClients();
  delay(10);
}
