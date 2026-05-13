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

    char fall_msg[256];

    while (1) {

        /* ================= LEER SENSOR ================= */

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

        /* ================= DETECCIÓN ================= */

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
            printk("\n");
            printk("====================================\n");
            printk("======= CAIDA DETECTADA ============\n");
            printk("====================================\n");

            /* construir mensaje */

            FallDetection_BuildMessage(
                fall_msg,
                sizeof(fall_msg));

            /* mostrar */

            printk("DATA: %s\n", fall_msg);

            /* enviar BLE */

            int err = ble_send(
                (uint8_t *)fall_msg,
                strlen(fall_msg));

            printk("BLE SEND: %d\n", err);
        }

        k_msleep(10);
    }
}