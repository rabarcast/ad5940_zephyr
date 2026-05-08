#ifndef MADGWICK_FILTER_H
#define MADGWICK_FILTER_H

#include <math.h>
#include <stdint.h>

#define _USE_MATH_DEFINES
#define M_PI 3.14159265358979323846f
#define RAD_TO_DEG 57.2958f

#define BETA        0.1f

#define IMPACT_THRESHOLD     25.0f
#define GYRO_THRESHOLD       200.0f
#define STILL_ACC_THRESHOLD  12.0f
#define STILL_GYRO_THRESHOLD 10.0f
#define ANGLE_THRESHOLD      60.0f
#define FALL_TIME_MS         1500

typedef enum {
    FALL_NONE = 0,
    FALL_DETECTED
} fall_result_t;

typedef enum {
    STATE_NORMAL,
    STATE_IMPACT
} fall_state_t;

/*

| Parámetro | Valor típico |
| --------- | ------------ |
| beta      | 0.05–0.15    |
| impacto   | 20–30 m/s²   |
| gyro      | 150–300 dps  |
| ángulo    | 45–70°       |
| tiempo    | 1–2 s        |

*/

extern fall_result_t fall_result;
extern fall_state_t fall_state;

static float invSqrt(float x);

void Madgwick_Init(void);
void Madgwick_Update(float gx, float gy, float gz,
                     float ax, float ay, float az,
                     float dt);
void Madgwick_GetQuaternion(float *q);
fall_result_t FallDetection_Update(float ax, float ay, float az,
                                   float gx, float gy, float gz,
                                   uint32_t timestamp_ms);

#endif