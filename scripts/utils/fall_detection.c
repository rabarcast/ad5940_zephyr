/* fall_detection.c */

#include "fall_detection.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

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
        {
            /*
             * Detectar impacto fuerte + rotación fuerte
             */

            if(acc_mag > 20.0f &&
               gyro_mag > 150.0f)
            {
                state = STATE_IMPACT;
                state_time = time_ms;

                q_start[0] = qw;
                q_start[1] = qx;
                q_start[2] = qy;
                q_start[3] = qz;

                printk("FALL_IMPACT DETECTED\n");
                return FALL_IMPACT_DETECTED;
            }

            break;
        }

        case STATE_IMPACT:
        {
            /*
             * Esperar estabilización
             */

            if(time_ms - state_time > 500)
            {
                float angle =
                    quaternion_angle(
                        q_start,
                        q_now);

                printk("ANGLE: %.2f\n", angle);

                /*
                 * caída válida:
                 * mucho cambio angular
                 */

                if(angle > 45.0f)
                {
                    state = STATE_STILLNESS;
                    state_time = time_ms;

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

                    printk("FALL_POSSIBLE\n");
                    return FALL_POSSIBLE;
                }
                else
                {
                    state = STATE_IDLE;
                }
            }

            break;
        }

        case STATE_STILLNESS:
        {
            /*
             * si vuelve a moverse:
             * cancelar
             */

            if(gyro_mag > 80.0f)
            {
                printk("MOVEMENT DETECTED\n");

                state = STATE_IDLE;
            }

            /*
             * quieto durante 2 segundos
             */

            if(time_ms - state_time >
               STILLNESS_TIME_MS)
            {
                printk("FALL CONFIRMED\n");

                state = STATE_IDLE;

                return FALL_DETECTED;
            }

            break;
        }

        default:
            state = STATE_IDLE;
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
    fall_result_t result,
    char *out,
    size_t max_len)
{
    /* construir mensaje final */
    char type[32] = {0};
    switch(result){
        case 1:
            strcpy(type, "FALL_DETECTED");
            break;
        case 2:
            strcpy(type, "FALL_IMPACT_DETECTED");
            break;
        case 3:
            strcpy(type, "FALL_POSSIBLE");
            break;
        default:
            break;
    }
    snprintf(out,
             max_len,
             "%s|DATE:%s|TIME:%s|ACC:%.2f|GYR:%.2f|ANG:%.2f|AX:%.2f|AY:%.2f|AZ:%.2f|END\n",
             type,
             NULL,
             NULL,
             (double)last_fall.acceleration,
             (double)last_fall.gyroscope,
             (double)last_fall.angle,
             (double)last_fall.ax,
             (double)last_fall.ay,
             (double)last_fall.az);
}