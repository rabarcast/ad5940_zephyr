#include "utils.h"

static float acc_mag(float ax, float ay, float az)
{
    return sqrtf(ax*ax + ay*ay + az*az);
}

static float gyr_mag(float gx, float gy, float gz)
{
    return sqrtf(gx*gx + gy*gy + gz*gz);
}