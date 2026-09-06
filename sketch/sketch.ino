#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "MULTY U";
const char* password = "Serhat12.";

WebServer server(80);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  server.on("/", []() {
    server.send(200, "text/plain", "ESP32 Aktif");
  });
  server.begin();
}

void loop() {
  server.handleClient();
}

