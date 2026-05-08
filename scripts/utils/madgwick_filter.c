#include "madgwick_filter.h"

static float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;

static float q_prev[4] = {1,0,0,0};

fall_state_t fall_state = STATE_NORMAL;
static uint32_t fall_time = 0;

///// UTILS

static float invSqrt(float x)
{
    return 1.0f / sqrtf(x);
}

static float quaternion_angle(float *qa, float *qb)
{
    float dot = qa[0]*qb[0] + qa[1]*qb[1] + qa[2]*qb[2] + qa[3]*qb[3];

    if (dot > 1.0f) dot = 1.0f;
    if (dot < -1.0f) dot = -1.0f;

    return acosf(2.0f * dot * dot - 1.0f) * RAD_TO_DEG;
}

///// API

void Madgwick_Init(void)
{
    q0 = 1.0f;
    q1 = q2 = q3 = 0.0f;

    q_prev[0] = 1;
    q_prev[1] = q_prev[2] = q_prev[3] = 0;

    fall_state = STATE_NORMAL;
}

void Madgwick_Update(float gx, float gy, float gz,
                     float ax, float ay, float az,
                     float dt)
{
    float norm;
    float s0, s1, s2, s3;
    float qDot0, qDot1, qDot2, qDot3;

    // Normalizar acelerómetro
    norm = sqrtf(ax*ax + ay*ay + az*az);
    if (norm == 0.0f) return;
    ax /= norm;
    ay /= norm;
    az /= norm;

    // Gradiente descendente
    s0 = 4*q0*q2*q2 + 2*q2*ax + 4*q0*q1*q1 - 2*q1*ay;
    s1 = 4*q1*q3*q3 - 2*q3*ax + 4*q0*q0*q1 - 2*q0*ay - 4*q1 + 8*q1*q1*q1 + 8*q1*q2*q2 + 4*q1*az;
    s2 = 4*q0*q0*q2 + 2*q0*ax + 4*q2*q3*q3 - 2*q3*ay - 4*q2 + 8*q2*q1*q1 + 8*q2*q2*q2 + 4*q2*az;
    s3 = 4*q1*q1*q3 - 2*q1*ax + 4*q2*q2*q3 - 2*q2*ay;

    norm = sqrtf(s0*s0 + s1*s1 + s2*s2 + s3*s3);
    s0 /= norm;
    s1 /= norm;
    s2 /= norm;
    s3 /= norm;

    // Derivada del quaternion
    qDot0 = 0.5f * (-q1*gx - q2*gy - q3*gz) - BETA * s0;
    qDot1 = 0.5f * ( q0*gx + q2*gz - q3*gy) - BETA * s1;
    qDot2 = 0.5f * ( q0*gy - q1*gz + q3*gx) - BETA * s2;
    qDot3 = 0.5f * ( q0*gz + q1*gy - q2*gx) - BETA * s3;

    // Integrar
    q0 += qDot0 * dt;
    q1 += qDot1 * dt;
    q2 += qDot2 * dt;
    q3 += qDot3 * dt;

    // Normalizar quaternion
    norm = sqrtf(q0*q0 + q1*q1 + q2*q2 + q3*q3);
    q0 /= norm;
    q1 /= norm;
    q2 /= norm;
    q3 /= norm;
}

void Madgwick_GetQuaternion(float *q)
{
    q[0] = q0;
    q[1] = q1;
    q[2] = q2;
    q[3] = q3;
}

fall_result_t FallDetection_Update(float ax, float ay, float az,
                                   float gx, float gy, float gz,
                                   uint32_t timestamp_ms)
{
    float q[4];
    Madgwick_GetQuaternion(q);

    float acc_mag = sqrtf(ax*ax + ay*ay + az*az);
    float gyr_mag = sqrtf(gx*gx + gy*gy + gz*gz);

    float angle = quaternion_angle(q_prev, q);

    fall_result_t result = FALL_NONE;

    switch (fall_state)
    {
        case STATE_NORMAL:
            if (acc_mag > IMPACT_THRESHOLD && gyr_mag > GYRO_THRESHOLD)
            {
                fall_state = STATE_IMPACT;
                fall_time = timestamp_ms;
            }
            break;

        case STATE_IMPACT:
            if ((timestamp_ms - fall_time) > FALL_TIME_MS)
            {
                if (acc_mag < STILL_ACC_THRESHOLD &&
                    gyr_mag < STILL_GYRO_THRESHOLD &&
                    angle > ANGLE_THRESHOLD)
                {
                    result = FALL_DETECTED;
                }

                fall_state = STATE_NORMAL;
            }
            break;
    }

    // guardar quaternion anterior
    q_prev[0] = q[0];
    q_prev[1] = q[1];
    q_prev[2] = q[2];
    q_prev[3] = q[3];

    return result;
}