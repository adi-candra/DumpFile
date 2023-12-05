#include <Arduino.h>

#include <WiFi.h>
#include <HTTPClient.h>
#define HTTPCLIENT_1_1_COMPATIBLE

//static const char *ssid     = "MT7603E.1.AP";  // your network SSID (name of wifi network)
//static const char *password = ""; // your network password

static const char *ssid     = "@wifi.adi";  // your network SSID (name of wifi network)
static const char *password = "Digitels123@!"; // your network password
// Set time via NTP, as required for x.509 validation
void setClock() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");  // UTC

  Serial.print(F("Waiting for NTP time sync: "));
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    yield();
    delay(500);
    Serial.print(F("."));
    now = time(nullptr);
  }

  Serial.println(F(""));
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
}

char payload[] = "{\"device_source\": \"00-B0-D0-63-C2-26\",\"device_destination\":[\"00-B0-D0-63-C2-26\",\"00-B0-D0-63-C2-26\",\"00-B0-D0-63-C2-26\"],\"device_command_type\":\"lift\",\"user_id\": 0,\"property_id\": 1,\"device_command\":\"9000\"}";
//char serverAddress[] = "https://6551f1ec5c69a77903294ff3.mockapi.io/command";  // server address
char serverAddress[] = "https://devs.digitels.me/go/v1/api/device/command";  // server address

void setup() {
  pinMode(14, OUTPUT);
  Serial.begin(115200);
  Serial.print("Attempting to connect to SSID: ");
  WiFi.begin(ssid, password);

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.subnetMask());
  Serial.println(WiFi.gatewayIP());

}

void loop() {
  // wait for WiFi connection
  if ((WiFi.status() == WL_CONNECTED)) {
    setClock();
    HTTPClient http;
    Serial.print("[HTTP] begin...\n");
    http.begin(serverAddress);
    http.addHeader("Content-Type", "application/json");
    //    http.addHeader("Content-Length", payload.length());
    http.addHeader("Authorization", "Basic ZGlnaXRlbHM6RGlnaXRlbHMzMjEh");
    //        http.setAuthorization("ZGlnaXRlbHM6RGlnaXRlbHMzMjEh");
    //    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    //    http.setRedirectLimit(20);
    //    http.setURL(serverAddress);
    Serial.print("[HTTP] POST...\n");
    int httpCode = http.POST(payload);
    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] POST... code: %d ", httpCode);
      String codetext = http.getString();
      Serial.println(codetext);
      // file found at server

    } else {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  }

  delay(10000);
}
