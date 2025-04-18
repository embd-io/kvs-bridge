/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <unistd.h>
#include "libuvc/libuvc.h"
#include "libuvc_helper.h"
#include "libuvc_adapter.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "usb/usb_host.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "camera.h"

static const char *TAG = "camera";

#define USB_DISCONNECT_PIN  GPIO_NUM_0

#if (CONFIG_EXAMPLE_UVC_PROTOCOL_MODE_AUTO)
#define EXAMPLE_UVC_PROTOCOL_AUTO_COUNT     3
typedef struct {
    enum uvc_frame_format format;
    int width;
    int height;
    int fps;
    const char* name;
} uvc_stream_profile_t;

uvc_stream_profile_t uvc_stream_profiles[EXAMPLE_UVC_PROTOCOL_AUTO_COUNT] = {
    {UVC_FRAME_FORMAT_MJPEG, 640, 480, 5, "640x480, fps 5"},
    {UVC_FRAME_FORMAT_MJPEG, 320, 240, 30, "320x240, fps 30"},
    {UVC_FRAME_FORMAT_MJPEG, 320, 240,  0, "320x240, any fps"}
};
#endif // CONFIG_EXAMPLE_UVC_PROTOCOL_MODE_AUTO

#if (CONFIG_EXAMPLE_UVC_PROTOCOL_MODE_CUSTOM)
#define FPS                 CONFIG_EXAMPLE_FPS_PARAM
#define WIDTH               CONFIG_EXAMPLE_WIDTH_PARAM
#define HEIGHT              CONFIG_EXAMPLE_HEIGHT_PARAM
#define FORMAT              CONFIG_EXAMPLE_FORMAT_PARAM
#endif // CONFIG_EXAMPLE_UVC_PROTOCOL_MODE_CUSTOM

// Attached camera can be filtered out based on (non-zero value of) PID, VID, SERIAL_NUMBER
#define PID 0
#define VID 0
#define SERIAL_NUMBER NULL

#define UVC_CHECK(exp) do {                 \
    uvc_error_t _err_ = (exp);              \
    if(_err_ < 0) {                         \
        ESP_LOGE(TAG, "UVC error: %s",      \
                 uvc_error_string(_err_));  \
        assert(0);                          \
    }                                       \
} while(0)

static SemaphoreHandle_t ready_to_uninstall_usb;
static EventGroupHandle_t app_flags;

// Handles common USB host library events
static void usb_lib_handler_task(void *args)
{
    while (1) {
        uint32_t event_flags;
        usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
        // Release devices once all clients has deregistered
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            usb_host_device_free_all();
        }
        // Give ready_to_uninstall_usb semaphore to indicate that USB Host library
        // can be deinitialized, and terminate this task.
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            xSemaphoreGive(ready_to_uninstall_usb);
        }
    }

    vTaskDelete(NULL);
}

static esp_err_t initialize_usb_host_lib(void)
{
    TaskHandle_t task_handle = NULL;

    const usb_host_config_t host_config = {
        .intr_flags = ESP_INTR_FLAG_LEVEL1
    };

    esp_err_t err = usb_host_install(&host_config);
    if (err != ESP_OK) {
        return err;
    }

    ready_to_uninstall_usb = xSemaphoreCreateBinary();
    if (ready_to_uninstall_usb == NULL) {
        usb_host_uninstall();
        return ESP_ERR_NO_MEM;
    }

    if (xTaskCreate(usb_lib_handler_task, "usb_events", 4096, NULL, 2, &task_handle) != pdPASS) {
        vSemaphoreDelete(ready_to_uninstall_usb);
        usb_host_uninstall();
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

static void uninitialize_usb_host_lib(void)
{
    xSemaphoreTake(ready_to_uninstall_usb, portMAX_DELAY);
    vSemaphoreDelete(ready_to_uninstall_usb);

    if (usb_host_uninstall() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to uninstall usb_host");
    }
}

void button_callback(int button, int state, void *user_ptr)
{
    printf("button %d state %d\n", button, state);
}

static void libuvc_adapter_cb(libuvc_adapter_event_t event)
{
    xEventGroupSetBits(app_flags, event);
}

static EventBits_t wait_for_event(EventBits_t event)
{
    return xEventGroupWaitBits(app_flags, event, pdTRUE, pdFALSE, portMAX_DELAY) & event;
}

static uvc_error_t uvc_negotiate_stream_profile(uvc_device_handle_t *devh,
                                                uvc_stream_ctrl_t *ctrl)
{
    uvc_error_t res;
#if (CONFIG_EXAMPLE_UVC_PROTOCOL_MODE_AUTO)
    for (int idx = 0; idx < EXAMPLE_UVC_PROTOCOL_AUTO_COUNT; idx++) {
        int attempt = CONFIG_EXAMPLE_NEGOTIATION_ATTEMPTS;
        do {
            /*
            The uvc_get_stream_ctrl_format_size() function will attempt to set the desired format size.
            On first attempt, some cameras would reject the format, even if they support it.
            So we ask 3x by default. The second attempt is usually successful.
            */
            ESP_LOGI(TAG, "Negotiate streaming profile %s ...", uvc_stream_profiles[idx].name);
            res = uvc_get_stream_ctrl_format_size(devh,
                                                  ctrl,
                                                  uvc_stream_profiles[idx].format,
                                                  uvc_stream_profiles[idx].width,
                                                  uvc_stream_profiles[idx].height,
                                                  uvc_stream_profiles[idx].fps);
        } while (--attempt && !(UVC_SUCCESS == res));
        if (UVC_SUCCESS == res) {
            break;
        }
    }

#elif (CONFIG_EXAMPLE_UVC_PROTOCOL_MODE_CUSTOM)
    int attempt = CONFIG_EXAMPLE_NEGOTIATION_ATTEMPTS;
    while (attempt--) {
        ESP_LOGI(TAG, "Negotiate streaming profile %dx%d, %d fps ...", WIDTH, HEIGHT, FPS);
        res = uvc_get_stream_ctrl_format_size(devh, ctrl, FORMAT, WIDTH, HEIGHT, FPS);
        if (UVC_SUCCESS == res) {
            break;
        }
        ESP_LOGE(TAG, "Negotiation failed. Try again (%d) ...", attempt);
    }
#endif // CONFIG_EXAMPLE_UVC_PROTOCOL_MODE_CUSTOM

    if (UVC_SUCCESS == res) {
        ESP_LOGI(TAG, "Negotiation complete.");
    } else {
        ESP_LOGE(TAG, "Try another UVC USB device of change negotiation parameters.");
    }

    return res;
}

esp_err_t camera_start(frame_callback custom_frame_cb_fn)
{
    uvc_context_t *ctx;
    uvc_device_t *dev;
    uvc_device_handle_t *devh;
    uvc_stream_ctrl_t ctrl;

    esp_err_t err = ESP_OK;

    app_flags = xEventGroupCreate();
    assert(app_flags);

    const gpio_config_t input_pin = {
        .pin_bit_mask = BIT64(USB_DISCONNECT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&input_pin));

    ESP_ERROR_CHECK(initialize_usb_host_lib());

    libuvc_adapter_config_t config = {
        .create_background_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .callback = libuvc_adapter_cb
    };

    libuvc_adapter_set_config(&config);

    UVC_CHECK(uvc_init(&ctx, NULL));

    ESP_LOGI(TAG, "Waiting for USB UVC device connection ...");
    wait_for_event(UVC_DEVICE_CONNECTED);

    if (uvc_find_device(ctx, &dev, PID, VID, SERIAL_NUMBER) != UVC_SUCCESS) {
        ESP_LOGW(TAG, "UVC device not found");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "UVC device found");

    // UVC Device open
    UVC_CHECK(uvc_open(dev, &devh));

    // Uncomment to print configuration descriptor
    libuvc_adapter_print_descriptors(devh);

    uvc_set_button_callback(devh, button_callback, NULL);

    // Print known device information
    uvc_print_diag(devh, stderr);

    // Negotiate stream profile
    if (UVC_SUCCESS == uvc_negotiate_stream_profile(devh, &ctrl)) {
        // dwMaxPayloadTransferSize has to be overwritten to MPS (maximum packet size)
        // supported by ESP32-S2(S3), as libuvc selects the highest possible MPS by default.
        ctrl.dwMaxPayloadTransferSize = 512;

        uvc_print_stream_ctrl(&ctrl, stderr);

        UVC_CHECK(uvc_start_streaming(devh, &ctrl, custom_frame_cb_fn, NULL, 0));
        ESP_LOGI(TAG, "Streaming...");
    } else {
        ESP_LOGE(TAG, "Failed to negotiate stream profile.");
        uvc_close(devh);
        uvc_exit(ctx);
        ESP_LOGI(TAG, "UVC exited");
        uninitialize_usb_host_lib();
        err = ESP_FAIL;
    }

    return err;
}