#ifndef KVS_BRIDGE_H
#define KVS_BRIDGE_H

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <esp_err.h>

esp_err_t kvs_bridge_client_disconnect();
esp_err_t kvs_bridge_client_send(char* pFrame, uint32_t uFrameSz);
esp_err_t kvs_bridge_client_connect(void);

#endif // KVS_BRIDGE_H