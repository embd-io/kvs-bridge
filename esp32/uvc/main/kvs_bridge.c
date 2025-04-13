#include "sdkconfig.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>            // struct addrinfo
#include <arpa/inet.h>
#include "esp_netif.h"
#include "esp_log.h"

static const char *TAG = "kvs_bridge";

// TCP socket hanlde
int sock = -1;

bool kvs_bridge_client_connected = false;

// Server config
#define HOST_IP_ADDR CONFIG_EXAMPLE_IPV4_ADDR
#define PORT CONFIG_EXAMPLE_PORT

esp_err_t kvs_bridge_client_disconnect ()
{
    if (!kvs_bridge_client_connected) {
        ESP_LOGI(TAG, "Client already disconnected");
        return ESP_OK;
    }
    if (sock != -1) {
        ESP_LOGE(TAG, "Shutting down socket and restarting...");
        shutdown(sock, 0);
        close(sock);
        sock = -1;
        kvs_bridge_client_connected = false;
        ESP_LOGI(TAG, "Client disconnected");
    }
    return ESP_OK;
}

esp_err_t kvs_bridge_client_send(char* pFrame, uint32_t uFrameSz)
{
    esp_err_t err = ESP_OK;

    if (!kvs_bridge_client_connected)
    {
        ESP_LOGE(TAG, "Client not connected");
        return ESP_FAIL;
    }

    // Send image size
    ESP_LOGI(TAG, "Sending image size: %ld", uFrameSz);
    uint32_t uFrameSz_be = htonl(uFrameSz);
    err = send(sock, (uint8_t *)&uFrameSz_be, sizeof(uFrameSz_be), 0);
    if (err < 0) {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Image size sent successfully");

    // Send image data
    ESP_LOGI(TAG, "Sending %ld bytes", uFrameSz);
    err = send(sock, pFrame, uFrameSz, 0);
    if (err < 0) {
        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Image data sent successfully");

    return ESP_OK;
}

esp_err_t kvs_bridge_client_connect (void)
{
    char host_ip[] = HOST_IP_ADDR;
    int addr_family = 0;
    int ip_protocol = 0;
        
    // Socket configuration
    struct sockaddr_in dest_addr;
    inet_pton(AF_INET, host_ip, &dest_addr.sin_addr);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    sock =  socket(addr_family, SOCK_STREAM, ip_protocol);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Socket created, connecting to %s:%d", host_ip, PORT);

    // Connect to server
    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to connect: errno %d", errno);
        shutdown(sock, 0);
        close(sock);
        sock = -1;
        return ESP_FAIL;   
    }
    ESP_LOGI(TAG, "Successfully connected");
    kvs_bridge_client_connected = true;

    return ESP_OK;
}
