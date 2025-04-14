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
#include "esp_timer.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "esp_event.h"

#include "camera.h"
#include "kvs_bridge.h"

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

    if (kvs_bridge_client_send((char *)frame->data, (uint32_t)frame->data_bytes) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send frame No %ld to KVS bridge server", frame_cnt);
    }
    ESP_LOGI(TAG, "frame No %ld sent to KVS bridge server", frame_cnt);
}

int app_main(int argc, char **argv)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    // connect tcp client
    if (kvs_bridge_client_connect() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect tcp client, make sure kvs-bridge-server-appsrc.py is run first");
        return ESP_FAIL;
    }

    // start camera
    if (camera_start(custom_frame_callback) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start camera stream");
        return ESP_FAIL;
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    return 0;
}
