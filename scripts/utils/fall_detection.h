/* fall_detection.h */

#ifndef FALL_DETECTION_H
#define FALL_DETECTION_H

#include <stdint.h>
#include <stddef.h>
#include <zephyr/sys/timeutil.h>
#include <time.h>

#define FREEFALL_THRESHOLD     7.5f
#define IMPACT_THRESHOLD       20.0f
#define STILLNESS_GYRO         15.0f
#define ANGLE_THRESHOLD        60.0f

#define IMPACT_WINDOW_MS       1000
#define STILLNESS_TIME_MS      2000

typedef enum {
    FALL_NONE = 0,
    FALL_DETECTED,
    FALL_IMPACT_DETECTED,
    FALL_POSSIBLE
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
    fall_result_t,
    char *out,
    size_t max_len);

#endif