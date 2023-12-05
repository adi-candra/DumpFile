//************************************************//
//Pinouts
//************************************************//
//Camera
//  PWDN_GPIO_NUM     32
//  RESET_GPIO_NUM    -1
//  XCLK_GPIO_NUM      0
//  SIOD_GPIO_NUM     26
//  SIOC_GPIO_NUM     27
//  Y9_GPIO_NUM       35
//  Y8_GPIO_NUM       34
//  Y7_GPIO_NUM       39
//  Y6_GPIO_NUM       36
//  Y5_GPIO_NUM       21
//  Y4_GPIO_NUM       19
//  Y3_GPIO_NUM       18
//  Y2_GPIO_NUM        5
//  VSYNC_GPIO_NUM    25
//  HREF_GPIO_NUM     23
//  PCLK_GPIO_NUM     22
//************************************************//
//TFT
//  TFT_MOSI           1
//  TFT_SCLK          12
//  TFT_CS            -1
//  TFT_DC            13
//  TFT_RST            3
//************************************************//
//SD Card (Not being used)
//  CLK GPIO          14
//  CMD GPIO          15
//  DATA0 GPIO         2
//  DATA1 GPIO         4 *Not used in 1-bit mode
//  DATA2 GPIO        12 *Not used in 1-bit mode
//  DATA3 GPIO        13 *Not used in 1-bit mode
//************************************************//
//Free Pins
//  Flashlight/GPIO    4
//  GPIO/U2RXD        16
//  Red LED           33 *Not an available GPIO. Just an indicator LED
//************************************************//

#define USE_TFT
#define USE_CAM
#define USE_JPG
#define MYDEBUG



//************************************************//
//Separate includes
//************************************************//
//Camera includes
#ifdef USE_CAM
#include "esp_camera.h"
//  #include "camera_index.h"
// #include "jpg_rot.h"
#endif

//LCD includes
#ifdef USE_TFT
#include <SPI.h>
#ifdef USE_JPG
#include <TJpg_Decoder.h>
//    #include <JPEGDEC.h>
#endif
#include <TFT_eSPI.h>          // Hardware-specific library
#endif

//General includes
#include "Arduino.h"
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"     // to hold led pin state constant

//************************************************//
//Separate feature setups
//************************************************//
//TFT setup
#ifdef USE_TFT
#define WHITE  0x00FFFFFF
#define BLACK  0x00000000
#define RED    0x000000FF
#define GREEN  0x0000FF00
#define BLUE   0x00FF0000
#define YELLOW (RED | GREEN)
#define CYAN   (BLUE | GREEN)
#define PURPLE (BLUE | RED)

TFT_eSPI tft = TFT_eSPI();

#endif

//Camera setup
#ifdef USE_CAM
camera_fb_t * fb = NULL;
int i = 0;
int n = 100; //n is number of frames to average over for fps calculation
float fps;
static esp_err_t cam_err;

// CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#ifdef USE_JPG
#ifdef USE_TFT
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if ( y >= tft.height() ) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}
#endif
bool no_tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if ( y >= tft.height() ) return 0;
  return 1;
}
#endif

bool setup_camera() { //ture: set up for jpg capture, false set up for rgb565 capture
  //Configure the camera
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
  config.fb_location = CAMERA_FB_IN_DRAM;

#ifdef USE_JPG
  config.pixel_format = PIXFORMAT_JPEG;
#else
  config.pixel_format = PIXFORMAT_RGB565;
#endif
  //init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    //debugln("psram found");
    config.frame_size = FRAMESIZE_240X240;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    //debugln("psram not found");
    config.frame_size = FRAMESIZE_240X240;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // camera init
  cam_err = esp_camera_init(&config);
  if (cam_err != ESP_OK) {
    //debugf("Camera init failed with error 0x%x", cam_err);
    return false;
  }

  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_240X240);
  s->set_vflip(s, 1);
  debugln("Camera initialized");

#ifdef USE_JPG
#ifdef USE_TFT
  TJpgDec.setJpgScale(1);
  //  TJpgDec.setSwapBytes(true);
  TJpgDec.setCallback(tft_output);
#endif
#endif
  return true;
}


#include <stddef.h>
#include "esp_heap_caps.h"
////#include "esp_jpg_decode.h"  //espressif's decoder
////#include <TJpg_Decoder.h> //bodmer's jpg decoder
//    #include "esp_system.h"
//    #if ESP_IDF_VERSION_MAJOR >= 4 // IDF 4+
//      #if CONFIG_IDF_TARGET_ESP32 // ESP32/PICO-D4
#include "esp32/spiram.h"
//      #else
//        #error Target CONFIG_IDF_TARGET is not supported
//      #endif
//    #else // ESP32 Before IDF 4.0
//      #include "esp_spiram.h"
//    #endif


#endif


#ifdef USE_TFT
bool setup_lcd() {

  tft.begin();
  tft.setRotation(0);  // 0 & 2 Portrait. 1 & 3 landscape
  tft.fillScreen(TFT_BLUE);
  tft.setCursor(0, 0);
  // Set the font colour to be white with a black background
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  debugln("TFT works");
  // delay(3000);
  debugln("LCD initialized");
  return true;
}
#endif

// debug functions to send debug info to screen or serial
#ifdef MYDEBUG
#ifdef USE_TFT
template <class T>
void debug (T msg) {
  tft.print(msg);
}

template <class T, class T2>
void debug (T msg, T2 fmt) {
  tft.print(msg, fmt);
}

template <class T>
void debugln (T msg) {
  tft.println(msg);
}

template <class T1, class T2>
void debugf (T1 msg, T2 var) {
  tft.printf(msg, var);
}

template <class T1, class T2, class T3>
void debugf (T1 msg, T2 var, T3 var2) {
  tft.printf(msg, var, var2);
}

template <class T1, class T2, class T3, class T4>
void debugf (T1 msg, T2 var, T3 var2, T4 var3) {
  tft.printf(msg, var, var2, var3);
}

void debughome () {
  tft.setCursor(0, 0);
}
#else

template <class T>
void debug (T msg) {
  Serial.print(msg);
}

template <class T, class T2>
void debug (T msg, T2 fmt) {
  Serial.print(msg, fmt);
}

template <class T>
void debugln (T msg) {
  Serial.println(msg);
}

template <class T1, class T2>
void debugf (T1 msg, T2 var) {
  Serial.printf(msg, var);
}

template <class T1, class T2, class T3>
void debugf (T1 msg, T2 var, T3 var2) {
  Serial.printf(msg, var, var2);
}

template <class T1, class T2, class T3, class T4>
void debugf (T1 msg, T2 var, T3 var2, T4 var3) {
  Serial.printf(msg, var, var2, var3);
}

void debughome () {
  __asm__ __volatile__ ("nop\n\t"); //no-operation inline assembly
}
#endif

#else
template <class T>
debug (T msg) {
  __asm__ __volatile__ ("nop\n\t"); //no-operation inline assembly
}

template <class T>
debugln (T msg) {
  __asm__ __volatile__ ("nop\n\t"); //no-operation inline assembly
}

template <class T1, class T2>
debugf (T1 msg, T2 var) {
  __asm__ __volatile__ ("nop\n\t"); //no-operation inline assembly
}

template <class T1, class T2, class T3>
void debugf (T1 msg, T2 var, T3 var2) {
  __asm__ __volatile__ ("nop\n\t"); //no-operation inline assembly
}

void debughome () {
  __asm__ __volatile__ ("nop\n\t"); //no-operation inline assembly
}
#endif

void setup() {
#ifndef USE_TFT
  Serial.begin(115200);
#endif
  debugln("-----------------------------------");
  debugln("starting");

  //  pinMode(4, OUTPUT);// initialize io4 as an output for LED flash.
  //  digitalWrite(4, LOW); // flash off/
  rtc_gpio_hold_en(GPIO_NUM_4); // Hold the state of the pin constant

  //Start the LCD
#ifdef USE_TFT
  if (not setup_lcd()) {
    return;
  }
#endif

  // Configure and start the camera
#ifdef USE_CAM
  if (not setup_camera()) {
    return;
  }
#endif
  debugln("Done configuring peripherals.");

  debugln("\r\nInitialisation done.");

  delay(2000);
}

float captureJPG() {
  long start = millis();
  for (int i = 0; i < n; i++) {
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
  }
  long end = millis();
  return (float)n * 1000.0 / ( (float)end - (float)start);
}

float captureRGB() {
  long start = millis();
  for (int i = 0; i < n; i++) {
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
  }
  long end = millis();
  return (float)n * 1000.0 / ( (float)end - (float)start);
}

#ifdef USE_JPG
float captureDrawJPG() {
  long start = millis();
  for (int i = 0; i < n; i++) {
    fb = esp_camera_fb_get();
    TJpgDec.drawJpg(0, 0, (const uint8_t*)fb->buf, fb->len);
    esp_camera_fb_return(fb);
  }
  long end = millis();
  return (float)n * 1000.0 / ( (float)end - (float)start);
}
#endif

#ifdef USE_JPG
float captureDecodeJPG() {
  TJpgDec.setCallback(no_tft_output);
  long start = millis();
  for (int i = 0; i < n; i++) {
    fb = esp_camera_fb_get();
    TJpgDec.drawJpg(0, 0, (const uint8_t*)fb->buf, fb->len);
    esp_camera_fb_return(fb);
  }
  long end = millis();
  TJpgDec.setCallback(tft_output);
  return (float)n * 1000.0 / ( (float)end - (float)start);
}
#endif

float captureDrawRGB() {
  long start = millis();
  for (int i = 0; i < n; i++) {
    fb = esp_camera_fb_get();
    tft.pushImage(0, 0, 240, 240, (uint16_t*)fb->buf);
    esp_camera_fb_return(fb);
  }
  long end = millis();
  return (float)n * 1000.0 / ( (float)end - (float)start);
}

void loop() {
#ifdef USE_JPG
  float jpg_fps = captureJPG();
  float jpg_dec_fps = captureDecodeJPG();
#ifdef USE_TFT
  float jpgDraw_fps = captureDrawJPG();
  tft.fillScreen(TFT_BLUE);
#endif
  debughome();
  debugf("Jpg capture fps = %.2f", jpg_fps);
  debugf("Jpg capture and decode fps = %.2f", jpg_dec_fps);
#ifdef USE_TFT
  debugf("Jpg capture and draw fps = %.2f", jpgDraw_fps);
#endif
#else
  float rgb_fps = captureRGB();
#ifdef USE_TFT
  float rgbDraw_fps = captureDrawRGB();
  tft.fillScreen(TFT_BLUE);
#endif
  debughome();
  debugf("RGB capture fps = %.2f", rgb_fps);
#ifdef USE_TFT
  debugf("RGB capture and draw fps = %.2f", rgbDraw_fps);
#endif
#endif
  delay(100000);
}
