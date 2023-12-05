/* modify by Adi Candra
   i got some reference u can check it
   https://www.survivingwithandroid.com/esp32-cam-tft-display-picture-st7735/
   mr.lin library resource :)
*/


#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>    // Core graphics library
//#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <TJpg_Decoder.h>
#include "esp_camera.h"
#define CAMERA_MODEL_AI_THINKER // Has PSRAM
//#include "camera_pins.h"
#define TFT_MOSI 47
#define TFT_SCLK 21
#define TFT_CS 14  // Chip select control pin
#define TFT_DC 45  // Data Command control pin
#define TFT_RST -1
#define TFT_BL 48
//#define PIN_BTN 4

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
  // Stop further decoding as image is running off bottom of screen
  if ( y >= tft.height() ) return 0;
  tft.drawRGBBitmap(x, y, bitmap, w, h);
  // Return 1 to decode next block
  return 1;
}

void init_camera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 11;
  config.pin_d1 = 9;
  config.pin_d2 = 8;
  config.pin_d3 = 10;
  config.pin_d4 = 12;
  config.pin_d5 = 18;
  config.pin_d6 = 17;
  config.pin_d7 = 16;
  config.pin_xclk = 15;
  config.pin_pclk = 13;
  config.pin_vsync = 6;
  config.pin_href = 7;
  config.pin_sscb_sda = 4;
  config.pin_sscb_scl = 5;
  config.pin_pwdn = -1;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  //  config.pixel_format = PIXFORMAT_RGB565;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  //  if (psramFound()) {
  //    config.fb_location = CAMERA_FB_IN_PSRAM;
  //    config.frame_size = FRAMESIZE_240X240;
  //    config.jpeg_quality = 10;
  //    config.fb_count = 1;
  //    Serial.println("PSRAM");
  //  } else {
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.frame_size = FRAMESIZE_240X240;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  //  }
  //#if defined(CAMERA_MODEL_ESP_EYE)
  //  pinMode(13, INPUT_PULLUP);
  //  pinMode(14, INPUT_PULLUP);
  //#endif
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    Serial.println("PID");
    s->set_vflip(s, 1); // flip it back
    s->set_hmirror(s, 1);
    //    s->set_brightness(s, 2); // up the brightness just a bit
    //    s->set_saturation(s, 0);
  }

}
camera_fb_t * fb = NULL;
uint32_t interval = 0;
uint16_t *image = NULL;

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32-CAM Picture");
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  // Use this initializer (uncomment) if using a 1.3" or 1.54" 240x240 TFT:
  tft.init(240, 240);
  //  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_GREEN);
  init_camera();
  //  pinMode(PIN_BTN, INPUT);
  TJpgDec.setJpgScale(1);
  // The decoder must be given the exact name of the rendering function above
  TJpgDec.setCallback(tft_output);

}
void take_picture() {
  Serial.println("Taking picture..");

  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
  }
  //  memcpy(image, fb->buf, fb->len);
  uint16_t w = 0, h = 0;
  TJpgDec.getJpgSize(&w, &h, fb->buf, fb->len);
  Serial.print("- Width = "); Serial.print(fb->width); Serial.print(", height = "); Serial.println(fb->height);

  Serial.print("Width = "); Serial.print(w); Serial.print(", height = "); Serial.println(h);
  // Draw the image, top left at 0,0
  TJpgDec.drawJpg(0, 0, fb->buf, fb->len);
  //  tft.drawRGBBitmap(0, 0, image, 240, 240);

  esp_camera_fb_return(fb);

}
void loop() {
  //  while (true) {
  //    //    int state = digitalRead(PIN_BTN);
  //    //    if (state == HIGH) {
  //    //      Serial.println("Button pressed");
  take_picture();
  //    //    }
  //    //    delay(20);
  //  }

}
