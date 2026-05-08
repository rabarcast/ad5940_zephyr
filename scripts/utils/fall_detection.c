#include "fall_detection.h"
#include <math.h>

#define FREEFALL_THRESHOLD     5.0f
#define IMPACT_THRESHOLD       25.0f
#define STILLNESS_GYRO         0.5f
#define ANGLE_THRESHOLD        60.0f

#define IMPACT_WINDOW_MS       1000
#define STILLNESS_TIME_MS      2000

typedef enum {
    STATE_IDLE,
    STATE_FREEFALL,
    STATE_IMPACT,
    STATE_STILLNESS
} state_t;

static state_t state;

static uint32_t state_time;

static float q_start[4];

static float quaternion_angle(
    float *qa,
    float *qb)
{
    float dot =
        qa[0]*qb[0] +
        qa[1]*qb[1] +
        qa[2]*qb[2] +
        qa[3]*qb[3];

    if (dot > 1.0f) dot = 1.0f;
    if (dot < -1.0f) dot = -1.0f;

    return acosf(2.0f * dot * dot - 1.0f)
           * 57.2958f;
}

void FallDetection_Init(void)
{
    state = STATE_IDLE;
}

fall_result_t FallDetection_Update(
    float ax, float ay, float az,
    float gx, float gy, float gz,
    float qw, float qx, float qy, float qz,
    uint32_t time_ms)
{
    float acc_mag =
        sqrtf(ax*ax + ay*ay + az*az);

    float gyro_mag =
        sqrtf(gx*gx + gy*gy + gz*gz);

    float q_now[4] = {
        qw, qx, qy, qz
    };

    switch(state)
    {
        case STATE_IDLE:

            if(acc_mag < FREEFALL_THRESHOLD)
            {
                state = STATE_FREEFALL;
                state_time = time_ms;

                q_start[0] = qw;
                q_start[1] = qx;
                q_start[2] = qy;
                q_start[3] = qz;
            }

            break;

        case STATE_FREEFALL:

            if(acc_mag > IMPACT_THRESHOLD)
            {
                state = STATE_IMPACT;
                state_time = time_ms;
            }

            else if(time_ms - state_time > 1000)
            {
                state = STATE_IDLE;
            }

            break;

        case STATE_IMPACT:

            if(time_ms - state_time > IMPACT_WINDOW_MS)
            {
                float angle =
                    quaternion_angle(
                        q_start,
                        q_now);

                if(angle > ANGLE_THRESHOLD)
                {
                    state = STATE_STILLNESS;
                    state_time = time_ms;
                }
                else
                {
                    state = STATE_IDLE;
                }
            }

            break;

        case STATE_STILLNESS:

            if(gyro_mag > STILLNESS_GYRO)
            {
                state = STATE_IDLE;
            }

            if(time_ms - state_time >
               STILLNESS_TIME_MS)
            {
                state = STATE_IDLE;
                return FALL_DETECTED;
            }

            break;
    }

    return FALL_NONE;
}