/* fall_detection.h */

#ifndef FALL_DETECTION_H
#define FALL_DETECTION_H

#include <stdint.h>
#include <stddef.h>
#include <zephyr/sys/timeutil.h>
#include <time.h>

typedef enum {
    FALL_NONE = 0,
    FALL_DETECTED
} fall_result_t;

/* ================= FALL DATA ================= */

typedef struct {

    float acceleration;
    float gyroscope;
    float angle;

    float ax;
    float ay;
    float az;

    float gx;
    float gy;
    float gz;

    uint32_t timestamp;

} fall_data_t;

/* ================= API ================= */

void FallDetection_Init(void);

fall_result_t FallDetection_Update(
    float ax, float ay, float az,
    float gx, float gy, float gz,
    float qw, float qx, float qy, float qz,
    uint32_t time_ms);

void FallDetection_GetLastFall(
    fall_data_t *data);

void FallDetection_BuildMessage(
    char *out,
    size_t max_len);

#endif