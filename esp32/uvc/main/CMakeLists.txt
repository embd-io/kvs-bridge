idf_component_register(SRCS "kvs_bridge.c" "camera.c" "main.c"
                    INCLUDE_DIRS ""
                    PRIV_REQUIRES
                        nvs_flash
                        usb
                        esp_ringbuf
                        esp_psram    # Required for CONFIG_SPIRAM
                        esp_timer
                        esp_wifi
                        esp_driver_gpio
                    )
