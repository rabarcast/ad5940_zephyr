#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>

#include <math.h>
#include <string.h>

#include "BNO055.h"
#include "fall_detection.h"
#include "ble.h"

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

    /* SENSOR */

    if (bno055_init(&bno) != 0) {

        printk("Error inicializando BNO055\n");

        return 0;
    }

    printk("BNO055 listo\n");

    /* FALL DETECTION */

    FallDetection_Init();

    /* VARIABLES */

    float ax, ay, az;
    float gx, gy, gz;
    float qw, qx, qy, qz;

    while (1) {

        /* LEER SENSOR */

        bno055_read_accel(&bno,
                          &ax,
                          &ay,
                          &az);

        bno055_read_gyro(&bno,
                         &gx,
                         &gy,
                         &gz);

        bno055_read_quat(&bno,
                         &qw,
                         &qx,
                         &qy,
                         &qz);

        /* DETECCIÓN DE CAÍDA */

        if (FallDetection_Update(
                ax,
                ay,
                az,
                gx,
                gy,
                gz,
                qw,
                qx,
                qy,
                qz,
                k_uptime_get()) == FALL_DETECTED)
        {
            printk("=========== CAIDA DETECTADA ===========\n");

            uint8_t encrypted[4];

            encrypt_fall(encrypted);

            ble_send(encrypted,
                     sizeof(encrypted));
        }

        k_msleep(10);
    }
}