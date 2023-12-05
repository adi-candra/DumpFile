

/*
   @Descripttion :
   @version      :
   @Author       : Kevincoooool
   @Date         : 2021-09-04 16:11:59
   @LastEditors: Please set LastEditors
   @LastEditTime: 2023-01-03 11:10:25
   @FilePath: \S3_DEMO\12.fast_qrcode_lvgl\main\app_main.c
*/

#include <stdio.h>

//#include "lv_examples/src/lv_demo_widgets/lv_demo_widgets.h"
//#include "lv_examples/src/lv_demo_music/lv_demo_music.h"
//#include "lv_examples/src/lv_demo_benchmark/lv_demo_benchmark.h"
#include <lv_ex_conf.h>
#include <lv_conf.h>
#include "lvgl_helpers.h"
#include "esp_freertos_hooks.h"
#include "esp_vfs.h"
#include "esp_spiffs.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "app_camera.h"
#include "app_main.h"
#include "page_cam.h"

#define TAG "ESP32S3"

camera_fb_t *fb = NULL;

#include "driver/gpio.h"
static void lv_tick_task(void *arg)
{
  (void)arg;
  lv_tick_inc(10);
}

SemaphoreHandle_t xGuiSemaphore;

static void gui_task(void *arg)
{
  xGuiSemaphore = xSemaphoreCreateMutex();
  lv_init(); // lvgl内核初始化

  lvgl_driver_init(); // lvgl显示接口初始化
  //申请两个buffer来给lvgl刷屏用
  /*外部PSRAM方式*/
  // lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(DISP_BUF_SIZE * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  // lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(DISP_BUF_SIZE * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  /*内部DMA方式*/
  lv_color_t *buf1 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);
  // lv_color_t *buf2 = heap_caps_malloc(DISP_BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA);

  // static lv_color_t buf1[DISP_BUF_SIZE];
  // static lv_color_t buf2[DISP_BUF_SIZE];
  static lv_disp_buf_t disp_buf;
  uint32_t size_in_px = DISP_BUF_SIZE;
  lv_disp_buf_init(&disp_buf, buf1, NULL, size_in_px);

  lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.flush_cb = disp_driver_flush;
  disp_drv.buffer = &disp_buf;
  lv_disp_drv_register(&disp_drv);

  // lv_indev_drv_t indev_drv;
  // lv_indev_drv_init(&indev_drv);
  // indev_drv.read_cb = touch_driver_read;
  // indev_drv.type = LV_INDEV_TYPE_POINTER;
  // lv_indev_drv_register(&indev_drv);

  // esp_register_freertos_tick_hook(lv_tick_task);
  /* 创建一个定时器中断来进入 lv_tick_inc 给lvgl运行提供心跳 这里是10ms一次 主要是动画运行要用到 */
  const esp_timer_create_args_t periodic_timer_args = {
    .callback = &lv_tick_task,
    .name = "periodic_gui"
  };
  esp_timer_handle_t periodic_timer;
  ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
  ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 10 * 1000));

  page_cam_load();//进入摄像头显示界面

  while (1)
  {
    /* Delay 1 tick (assumes FreeRTOS tick is 10ms */
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Try to take the semaphore, call lvgl related function on success */
    if (pdTRUE == xSemaphoreTake(xGuiSemaphore, portMAX_DELAY))
    {

      lv_task_handler();
      xSemaphoreGive(xGuiSemaphore);
    }
  }
}


void setup()
{

  // 初始化nvs用于存放wifi或者其他需要掉电保存的东西
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  /*创建lvgl任务显示*/
  xTaskCreatePinnedToCore(&gui_task, "gui task", 1024 * 5, NULL, 5, NULL, 1);

}