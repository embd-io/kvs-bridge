#include "sdkconfig.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>            // struct addrinfo
#include <arpa/inet.h>
#include "esp_netif.h"
#include "esp_log.h"

static const char *TAG = "mjpeg_producer";

// Server config
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV4_ADDR
#define PORT CONFIG_EXAMPLE_PORT

// Static MJPEG image data
extern const uint8_t *frames_jpg_start[];
extern const uint8_t *frames_jpg_end[];

const float frame_rate_ms = 1000 / 30; // 30 fps

void kbridge_client(void)
{
    // char rx_buffer[128];
    char host_ip[] = HOST_IP_ADDR;
    int addr_family = 0;
    int ip_protocol = 0;

    while (1) {
        
        // Socket configuration
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

        // Connect to server
        int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        if (err != 0) {
            ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Successfully connected");

        int nb_images = sizeof((uint8_t*)frames_jpg_start) / sizeof(frames_jpg_start[0]);
        int image_index = 0;

        char* pData = NULL;
        uint32_t uDataLen = 0;
        int idx = 0;

        while (1) {

            // Allocate memory and read jpeg frames from flash
            idx = image_index % nb_images;
            uDataLen = frames_jpg_end[idx] - frames_jpg_start[idx] + 1;
            pData = (char *)malloc(uDataLen);
            if (pData == NULL) {
                ESP_LOGE(TAG, "Memory allocation failed for image %d", image_index);
                while (1) {
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                }
            }
            memcpy(pData, frames_jpg_start[idx], uDataLen);
            ESP_LOGI(TAG, "Image %d size: %ld", (image_index % nb_images) / nb_images, uDataLen);
            image_index++;

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

            vTaskDelay(frame_rate_ms / portTICK_PERIOD_MS);
        
            // Free allocated memory
            free(pData);
            pData = NULL;
        }

        if (sock != -1) {
            ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(sock, 0);
            close(sock);
        }
    }
}
