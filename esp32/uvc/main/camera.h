#ifndef CAMERA_H
#define CAMERA_H

#include "libuvc/libuvc.h"
#include "esp_err.h"

typedef void (*frame_callback)(uvc_frame_t *frame, void *ptr);
esp_err_t camera_start(frame_callback custom_frame_cb_fn);

#endif // CAMERA_H