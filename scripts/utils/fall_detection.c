/* fall_detection.c */

#include "fall_detection.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

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

/* ================= FALL DATA ================= */

static fall_data_t last_fall;

/* ================= UTILS ================= */

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

/* ================= INIT ================= */

void FallDetection_Init(void)
{
    state = STATE_IDLE;

    memset(&last_fall, 0, sizeof(last_fall));
}

/* ================= UPDATE ================= */

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

                    /* guardar datos */
                    last_fall.acceleration = acc_mag;
                    last_fall.gyroscope = gyro_mag;
                    last_fall.angle = angle;

                    last_fall.ax = ax;
                    last_fall.ay = ay;
                    last_fall.az = az;

                    last_fall.gx = gx;
                    last_fall.gy = gy;
                    last_fall.gz = gz;

                    last_fall.timestamp = time_ms;
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

/* ================= GET DATA ================= */

void FallDetection_GetLastFall(
    fall_data_t *data)
{
    memcpy(data,
           &last_fall,
           sizeof(fall_data_t));
}

/* ================= BUILD STRING ================= */

void FallDetection_BuildMessage(
    char *out,
    size_t max_len)
{
    struct timespec ts;
    struct tm tm_time;

    /* obtener hora del sistema */
    sys_clock_gettime(SYS_CLOCK_REALTIME, &ts);
    gmtime_r(&ts.tv_sec, &tm_time);

    /* formatear fecha */
    char date_str[16];
    char time_str[16];

    snprintf(date_str, sizeof(date_str),
             "%04d-%02d-%02d",
             tm_time.tm_year + 1900,
             tm_time.tm_mon + 1,
             tm_time.tm_mday);

    snprintf(time_str, sizeof(time_str),
             "%02d:%02d:%02d",
             tm_time.tm_hour,
             tm_time.tm_min,
             tm_time.tm_sec);

    /* construir mensaje final */
    snprintf(out,
             max_len,
             "FALL|DATE:%s|TIME:%s|ACC:%.2f|GYR:%.2f|ANG:%.2f|AX:%.2f|AY:%.2f|AZ:%.2f",
             date_str,
             time_str,
             (double)last_fall.acceleration,
             (double)last_fall.gyroscope,
             (double)last_fall.angle,
             (double)last_fall.ax,
             (double)last_fall.ay,
             (double)last_fall.az);
}