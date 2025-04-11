/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <unistd.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "camera.h"

static const char *TAG = "main";

/* This callback function runs once per frame. Use it to perform any
 * quick processing you need, or have it put the frame into your application's
 * input queue. If this function takes too long, you'll start losing frames. */
void custom_frame_callback(uvc_frame_t *frame, void *ptr)
{
    static size_t fps;
    static size_t bytes_per_second;
    static int64_t start_time;
    static uint32_t frame_cnt = 0;

    frame_cnt ++;

    int64_t current_time = esp_timer_get_time();
    bytes_per_second += frame->data_bytes;
    fps++;

    if (!start_time) {
        start_time = current_time;
    }

    if (current_time > start_time + 1000000) {
        ESP_LOGI(TAG, "fps: %u, bytes per second: %u", fps, bytes_per_second);
        start_time = current_time;
        bytes_per_second = 0;
        fps = 0;
    }

    // TODO: send frame to KVS bridge server
    ESP_LOGI(TAG, "frame No %ld", frame_cnt);
}

int app_main(int argc, char **argv)
{
    if (camera_start(custom_frame_callback) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start UVC");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "UVC started");

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    return 0;
}
