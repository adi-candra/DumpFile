#include <esp_now.h>
#include <WiFi.h>

// REPLACE WITH YOUR RECEIVER MAC Address
//voice command A8:42:E3:48:55:48

uint8_t broadcastAddress[] = {0xA8, 0x42, 0xE3, 0x48, 0x55, 0x48};

esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

String jsonString;
uint32_t interval = 0;

void setup() {
  // Init Serial Monitor
  pinMode(14, OUTPUT);
  Serial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

}

void loop() {
  // Set values to send
  if ((millis() - interval) >= 1000) {
    interval = millis();
    digitalWrite(14, !digitalRead(14));
  }

  if (Serial.available()) {
    jsonString = "";

    while (Serial.available()) {
      // get the new byte:
      char inChar = (char)Serial.read();
      // add it to the inputString:
      jsonString += inChar;
    }

    Serial.println(jsonString);
    unsigned char buf[jsonString.length() + 1];
    jsonString.getBytes(buf, sizeof(buf));

    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &buf, sizeof(buf));

    if (result == ESP_OK) {
      Serial.println("Sent with success");
    }
    else {
      Serial.println("Error sending the data");
    }
  }
}
