#include <WiFi.h>
#include <AsyncTelegram2.h>
#include "esp_camera.h"
#include "soc/soc.h"           // Brownout error fix
#include "soc/rtc_cntl_reg.h"  // Brownout error fix
#include <WiFiClientSecure.h>
#include <Stepper.h>
#include "EEPROM.h"

WiFiClientSecure client;
Stepper stepper(200, 13, 12);  // DIRECTION pin & STEP Pin of Driver


char ssid[] = "wifikotak";  // SSID WiFi network
char pass[] = "12341234";  // Password  WiFi network

const char* token = "6020602577:AAE1KU2KtTUCvdm5hku0UFdvC9MRpFgMWU8";

// Check the userid with the help of bot @JsonDumpBot or @getidsbot (work also with groups)
// https://t.me/JsonDumpBot  or  https://t.me/getidsbot
int64_t userid = 827131911;


//myvariable
// A variable to store telegram message data
TBMessage msg;
#define LAMP_PIN 4
camera_fb_t* fb = NULL;
unsigned long nowmillis = 0;

#define motorInterfaceType 1
#define sense_pintu 15
#define sele_pintu 2
#define sele_lock 14
String kode = "0";
String inkode = "0";
uint8_t isikotak = 0;
boolean scan = false;
uint8_t stepp = 0;
unsigned long lastmilis = 0;
int pintustate = 0;
unsigned long intervalnotif = 0;

String replyStr;
String str0 = "empty";
String str1 = "empty1";
String str2 = "empty2";
String str3 = "empty3";
String ssid_2 = "empty";  // second SSID WiFi network
String pass_2 = "empty";  // second Password  WiFi network
String userid2 = "empty";
uint8_t userState = 0;
// Timezone definition to get properly time from NTP server
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"

AsyncTelegram2 myBot(client);

#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

static camera_config_t camera_config = {
  .pin_pwdn = PWDN_GPIO_NUM,
  .pin_reset = RESET_GPIO_NUM,
  .pin_xclk = XCLK_GPIO_NUM,
  .pin_sscb_sda = SIOD_GPIO_NUM,
  .pin_sscb_scl = SIOC_GPIO_NUM,
  .pin_d7 = Y9_GPIO_NUM,
  .pin_d6 = Y8_GPIO_NUM,
  .pin_d5 = Y7_GPIO_NUM,
  .pin_d4 = Y6_GPIO_NUM,
  .pin_d3 = Y5_GPIO_NUM,
  .pin_d2 = Y4_GPIO_NUM,
  .pin_d1 = Y3_GPIO_NUM,
  .pin_d0 = Y2_GPIO_NUM,
  .pin_vsync = VSYNC_GPIO_NUM,
  .pin_href = HREF_GPIO_NUM,
  .pin_pclk = PCLK_GPIO_NUM,
  .xclk_freq_hz = 20000000,  //XCLK 20MHz or 10MHz
  .ledc_timer = LEDC_TIMER_0,
  .ledc_channel = LEDC_CHANNEL_0,
  .pixel_format = PIXFORMAT_JPEG,  //YUV422,GRAYSCALE,RGB565,JPEG
  .frame_size = FRAMESIZE_UXGA,    //QQVGA-UXGA Do not use sizes above QVGA when not JPEG
  .jpeg_quality = 12,              //0-63 lower number means higher quality
  .fb_count = 1                    //if more than one, i2s runs in continuous mode. Use only with JPEG
};


static esp_err_t init_camera() {
  //initialize the camera
  Serial.print("Camera init... ");
  esp_err_t err = esp_camera_init(&camera_config);

  if (err != ESP_OK) {
    delay(100);  // need a delay here or the next serial o/p gets missed
    Serial.printf("\n\nCRITICAL FAILURE: Camera sensor failed to initialise.\n\n");
    Serial.printf("A full (hard, power off/on) reboot will probably be needed to recover from this.\n");
    return err;
  } else {
    Serial.println("succeeded");

    // Get a reference to the sensor
    sensor_t* s = esp_camera_sensor_get();

    // Dump camera module, warn for unsupported modules.
    switch (s->id.PID) {
      case OV9650_PID: Serial.println("WARNING: OV9650 camera module is not properly supported, will fallback to OV2640 operation"); break;
      case OV7725_PID: Serial.println("WARNING: OV7725 camera module is not properly supported, will fallback to OV2640 operation"); break;
      case OV2640_PID: Serial.println("OV2640 camera module detected"); break;
      case OV3660_PID: Serial.println("OV3660 camera module detected"); break;
      default: Serial.println("WARNING: Camera module is unknown and not properly supported, will fallback to OV2640 operation");
    }
  }
  return ESP_OK;
}

size_t sendPicture(TBMessage& msg) {
  // Take Picture with Camera;
  Serial.println("Camera capture requested");

  // Take picture with Camera and send to Telegram
  digitalWrite(LAMP_PIN, HIGH);
  // Clear buffer
  nowmillis = millis();
  while ((millis() - nowmillis) < 100) {
    esp_camera_fb_return(fb);
    fb = NULL;
  }
  fb = esp_camera_fb_get();
  digitalWrite(LAMP_PIN, LOW);
  if (!fb) {
    Serial.println("Camera capture failed");
    return 0;
  }
  size_t len = fb->len;
  if (!myBot.sendPhoto(msg, fb->buf, fb->len)) {
    len = 0;
    //    myBot.sendMessage(msg, "Error! Picture not sent.");
  }
  return len;
}

void setup() {

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);  // disable brownout detector
  pinMode(LAMP_PIN, OUTPUT);                  // set the lamp pin as output

  Serial.begin(9600);

  // mulai eeprom dengan alamat 192 Bytes
  if (!EEPROM.begin(208)) {
    Serial.println("Failed to initialise EEPROM");
    Serial.println("Restarting...");
    ESP.restart();
  }
  //store string length up to 32 byte
  str0 = EEPROM.readString(0);
  str1 = EEPROM.readString(32);
  str2 = EEPROM.readString(64);
  str3 = EEPROM.readString(128);
  ssid_2 = EEPROM.readString(160);
  pass_2 = EEPROM.readString(176);
  userid2 = EEPROM.readString(192);
  char ss[sizeof ssid_2];
  char pp[sizeof pass_2];

  //  char* s = ssid_2;
  ssid_2.toCharArray(ss, sizeof(ss));
  pass_2.toCharArray(pp, sizeof(pp));
  Serial.println(ss);
  Serial.println(pp);

  // Start WiFi connection
  WiFi.begin(ssid, pass);
  userState = 0;
  nowmillis = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if ((millis() - nowmillis) > 2000) {
      WiFi.begin(ss, pp);
      userState = 1;
      while (WiFi.status() != WL_CONNECTED) {
        if ((millis() - nowmillis) >= 4000) {
          ESP.restart();
        }
      }
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());

  // Sync time with NTP
  configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  client.setCACert(telegram_cert);

  // Set the Telegram bot properies
  myBot.setUpdateTime(1000);
  myBot.setTelegramToken(token);

  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");

  // Send a welcome message to user when ready
  char welcome_msg[256];
  sprintf(welcome_msg, "BOT @%s online.\nMenu:\n/list = Menampilkan daftar nomor resi.\n/secret = Mengganti password Pemilik.\n/id = Tambah nomor resi. \n/wifi = Masukkan SSID dan PASSWORD WIFI dengan ;.", myBot.getBotName());
  if (userState == 0) {
    myBot.sendTo(userid, welcome_msg);
  }
  if (userState == 1) {
    myBot.sendTo(userid2.toInt(), welcome_msg);
  }

  // Init the camera module (accordind the camera_config_t defined)
  digitalWrite(LAMP_PIN, HIGH);
  init_camera();
  fb = esp_camera_fb_get();
  // Clear buffer
  nowmillis = millis();
  while ((millis() - nowmillis) < 1000) {
    esp_camera_fb_return(fb);
    fb = 0;
  }
  digitalWrite(LAMP_PIN, LOW);

  stepper.setSpeed(100);
  pinMode(sele_pintu, OUTPUT);
  pinMode(sele_lock, OUTPUT);
  pinMode(sense_pintu, INPUT);
  digitalWrite(sele_pintu, HIGH);
  digitalWrite(sele_lock, HIGH);
}

void loop() {
  if ((millis() - intervalnotif) > 600000) {
    intervalnotif = millis();
    myBot.sendMessage(msg, "Device still online!!");
  }
  // if there is an incoming message...
  if (myBot.getNewMessage(msg)) {
    Serial.print("New message from chat_id: ");
    Serial.println(msg.sender.id);
    MessageType msgType = msg.messageType;

    if (msgType == MessageText) {
      // Received a text message
      if (msg.text.equalsIgnoreCase("/photo")) {
        Serial.println("\nSending Photo from CAM");
        int sended = sendPicture(msg);
        if (sended == 0)
          Serial.println("Picture sent successfull");
        //        else
        //          myBot.sendMessage(msg, "Error, picture not sent.");
      } else if (msg.text.equalsIgnoreCase("/list")) {
        replyStr = String(str1) + "\n" + String(str2) + "\n" + String(str3);
        myBot.sendMessage(msg, replyStr);
      } else if (msg.text.equalsIgnoreCase("/secret")) {
        myBot.sendMessage(msg, "Silahkan kirim password!!");
        replacepassword();
      } else if (msg.text.equalsIgnoreCase("/id")) {
        myBot.sendMessage(msg, "Silahkan kirim nomor resi!!");
        replacetrackingid();
      } else if (msg.text.equalsIgnoreCase("/wifi")) {
        myBot.sendMessage(msg, "Silahkan kirim ssid dan password WIFI!!");
        replaceWIFI();
      } else {
        Serial.print("\nText message received: ");
        Serial.println(msg.text);
        replyStr = "Message received:\n";
        replyStr += msg.text;
        replyStr += "\nTry with /photo";
        myBot.sendMessage(msg, replyStr);
      }
    }
  }


  while (Serial.available() > 0) {
    uint8_t inchar = Serial.read();
    if (inchar != 13) {
      inkode += char(inchar);
      //      Serial.println(char(inchar));
    } else {
      kode = inkode;
      inkode = "";
      //      Serial.println(kode);
    }
  }

  if (str0 == kode) {
    Serial.println("barcode for super user");
    digitalWrite(sele_pintu, LOW);
    digitalWrite(sele_lock, LOW);
    Serial.println("door open");
    if ((millis() - lastmilis) >= 3000) {
      lastmilis = millis();
      digitalWrite(sele_pintu, HIGH);
      scan = true;
      while (scan) {
        if (digitalRead(sense_pintu) == LOW) {
          Serial.println("door close");
          kode = "";
          scan = false;
          isikotak = 0;
        }
      }
    }
  }
  else if ((str1 == kode || str2 == kode || str3 == kode) && isikotak < 3) {
    Serial.println("barcode for courier");
    digitalWrite(sele_pintu, LOW);
    Serial.println("door open");
    if ((millis() - lastmilis) >= 3000) {
      lastmilis = millis();
      digitalWrite(sele_pintu, HIGH);
      scan = true;
      while (scan) {
        pintustate = digitalRead(sense_pintu);
        if (pintustate == LOW) {
          int sended = sendPicture(msg);
          if (sended == 0)
            Serial.println("Picture sent successfull");
          //          else
          //            myBot.sendMessage(msg, "Error, picture not sent.");
          kode = "";
          Serial.println("door close");

          digitalWrite(sele_lock, LOW);
          delay(1000);
          stepper.step(800);
          delay(1000);
          scan = false;
        }
      }
      isikotak++;
      replyStr = "Paket Terisi:\n" + String(isikotak);
      myBot.sendMessage(msg, replyStr);
    }

  } else {
    lastmilis = millis();
    digitalWrite(sele_lock, HIGH);
    digitalWrite(sele_pintu, HIGH);
  }
}

void replacepassword() {
  // A variable to store telegram message data
  TBMessage msg;
  nowmillis = millis();
  while ((millis() - nowmillis) < 10000) {
    // if there is an incoming message...
    if (myBot.getNewMessage(msg)) {
      Serial.print("New message from chat_id: ");
      Serial.println(msg.sender.id);
      MessageType msgType = msg.messageType;
      if (msgType == MessageText) {
        str0 = msg.text;
        EEPROM.writeString(0, str0);
        EEPROM.commit();
        myBot.sendMessage(msg, "Ganti password berhasil!!");
        return;
      }
    }
  }
}
void replacetrackingid() {
  // A variable to store telegram message data
  TBMessage msg;
  nowmillis = millis();
  while ((millis() - nowmillis) < 10000) {
    // if there is an incoming message...
    if (myBot.getNewMessage(msg)) {
      Serial.print("New message from chat_id: ");
      Serial.println(msg.sender.id);
      MessageType msgType = msg.messageType;
      if (msgType == MessageText) {
        String dumpstring = msg.text;
        if (dumpstring.startsWith("1;")) {
          str1 = dumpstring.substring(2, dumpstring.length());
        }
        else if (dumpstring.startsWith("2;")) {
          str2 = dumpstring.substring(2, dumpstring.length());
        }
        else if (dumpstring.startsWith("3;")) {
          str3 = dumpstring.substring(2, dumpstring.length());
        }
        else {
          str3 = str2;
          str2 = str1;
          str1 = msg.text;
        }
        EEPROM.writeString(32, str1);
        EEPROM.writeString(64, str2);
        EEPROM.writeString(128, str3);
        EEPROM.commit();
        myBot.sendMessage(msg, "Nomor resi berhasil ditambah!!");
        return;
      }
    }
  }
}

void replaceWIFI() {
  // A variable to store telegram message data
  TBMessage msg;
  nowmillis = millis();
  while ((millis() - nowmillis) < 10000) {
    // if there is an incoming message...
    if (myBot.getNewMessage(msg)) {
      Serial.print("New message from chat_id: ");
      Serial.println(msg.sender.id);
      MessageType msgType = msg.messageType;
      if (msgType == MessageText) {
        String dumpstring = msg.text;
        int positionindex = dumpstring.indexOf(";");
        ssid_2 = dumpstring.substring(0, positionindex);
        pass_2 = dumpstring.substring(positionindex + 1, dumpstring.length());
        EEPROM.writeString(160, ssid_2);
        EEPROM.writeString(176, pass_2);
        EEPROM.commit();
        myBot.sendMessage(msg, "Ganti SSID dan PASSWORD WIFI berhasil!!");
        return;
      }
    }
  }
}
