#include "sdkconfig.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>            // struct addrinfo
#include <arpa/inet.h>
#include "esp_netif.h"
#include "esp_log.h"

// Server config
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV4_ADDR
#define PORT CONFIG_EXAMPLE_PORT

static const char *TAG = "kbridge_client";

extern const uint8_t frame_0001_jpg_start[] asm("_binary_frame_0001_jpg_start");
extern const uint8_t frame_0001_jpg_end[] asm("_binary_frame_0001_jpg_end");

const uint8_t *frames_jpg_start[] = {
    frame_0001_jpg_start,
};

const uint8_t *frames_jpg_end[] = {
    frame_0001_jpg_end,
};

void kbridge_client(void)
{
    // char rx_buffer[128];
    char host_ip[] = HOST_IP_ADDR;
    int addr_family = 0;
    int ip_protocol = 0;

    uint32_t uDataLen = frames_jpg_end[0] - frames_jpg_start[0] + 1;
    char* pData = (char *)malloc(uDataLen);
    if (pData == NULL) {
        ESP_LOGE(TAG, "Memory allocation failed");
        while (1) {
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
    }
    memcpy(pData, frames_jpg_start[0], uDataLen);

    while (1) {
        struct sockaddr_in dest_addr;
        inet_pton(AF_INET, host_ip, &dest_addr.sin_addr);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;

        int sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_ip, PORT);

        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Successfully connected");

        while (1) {
            // Send image size
            ESP_LOGI(TAG, "Sending image size: %ld", uDataLen);
            uint32_t uDataLen_be = htonl(uDataLen);
            err = send(sock, (uint8_t *)&uDataLen_be, sizeof(uDataLen_be), 0);
            ESP_LOGI(TAG, "Image size sent successfully");

            // Send image data
            ESP_LOGI(TAG, "Sending %ld bytes", uDataLen);
            err = send(sock, pData, uDataLen, 0);
            if (err < 0) {
                ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                break;
            }
            ESP_LOGI(TAG, "Image data sent successfully");


            vTaskDelay(30 / portTICK_PERIOD_MS);
#if 0
            int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // Error occurred during receiving
            if (len < 0) {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
                ESP_LOGI(TAG, "%s", rx_buffer);
            }
#endif 

        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
}
