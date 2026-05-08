#ifndef FALL_DETECTION_H
#define FALL_DETECTION_H

#include <stdint.h>

typedef enum {
    FALL_NONE = 0,
    FALL_DETECTED
} fall_result_t;

void FallDetection_Init(void);

fall_result_t FallDetection_Update(
    float ax, float ay, float az,
    float gx, float gy, float gz,
    float qw, float qx, float qy, float qz,
    uint32_t time_ms);

#endif