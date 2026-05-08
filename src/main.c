#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>

#include <math.h>
#include <string.h>

#include "BNO055.h"
#include "madgwick_filter.h"
#include "ble.h"

#define RAD (3.14159265359f / 180.0f)

#define I2C_NODE DT_NODELABEL(bno055)

/* ================= SENSOR ================= */

static struct bno055_dev bno = {
    .i2c = I2C_DT_SPEC_GET(I2C_NODE),
};

/* ================= ENCRYPT ================= */

static void encrypt_fall(uint8_t *out)
{
    uint8_t plain[] = "FALL";

    uint8_t key = 0x5A;

    for (int i = 0; i < 4; i++) {

        out[i] = plain[i] ^ key;
    }
}

/* ================= MAIN ================= */

int main(void)
{
    printk("=== FALL DETECTION PROJECT ===\n");

    /* BLE */

    ble_init();

    /* I2C */

    if (!device_is_ready(bno.i2c.bus)) {

        printk("Error: I2C no listo\n");

        return 0;
    }

    printk("I2C OK\n");

    /* sensor */

    if (bno055_init(&bno) != 0) {

        printk("Error inicializando BNO055\n");

        return 0;
    }

    printk("BNO055 listo\n");

    float ax = 0;
    float ay = 0;
    float az = 0;

    float gx = 0;
    float gy = 0;
    float gz = 0;

    float qw = 0;
    float qx = 0;
    float qy = 0;
    float qz = 0;

    Madgwick_Init();

    int64_t last_time = k_uptime_get();

    while (1) {

        /* leer sensor */

        bno055_read_accel(&bno, &ax, &ay, &az);

        bno055_read_gyro(&bno, &gx, &gy, &gz);

        bno055_read_quat(&bno, &qw, &qx, &qy, &qz);

        /* deg/s -> rad/s */

        gx *= RAD;
        gy *= RAD;
        gz *= RAD;

        /* dt */

        int64_t now = k_uptime_get();

        float dt = (now - last_time) / 1000.0f;

        last_time = now;

        /* filtro */

        Madgwick_Update(gx,
                         gy,
                         gz,
                         ax,
                         ay,
                         az,
                         dt);

        /* caída */

        if (FallDetection_Update(ax,
                                 ay,
                                 az,
                                 gx,
                                 gy,
                                 gz,
                                 now) == FALL_DETECTED) {

            printk("CAIDA DETECTADA\n");

            uint8_t encrypted[4];

            encrypt_fall(encrypted);

            int err = ble_send(encrypted, sizeof(encrypted));

            printk("BLE SEND: %d\n", err);
        }

        k_msleep(10);
    }
}