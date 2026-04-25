#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>

#include "BNO055.h"

#define I2C_NODE DT_NODELABEL(bno055)

static struct bno055_dev bno = {
    .i2c = I2C_DT_SPEC_GET(I2C_NODE),
};

int main(void)
{
    printk("=== FALLING DETECTION PROJECT (BNO055) ===\n");

    // Verificar I2C
    if (!device_is_ready(bno.i2c.bus)) {
        printk("Error: I2C no listo\n");
        return 0;
    }

    printk("I2C OK\n");

    // Inicializar sensor
    if (bno055_init(&bno) != 0) {
        printk("Error inicializando BNO055\n");
        return 0;
    }

    printk("BNO055 listo\n");

    float ax = 0, ay = 0, az = 0;
    float gx = 0, gy = 0, gz = 0;
    float qw = 0, qx = 0, qy = 0, qz = 0;
    while (1) {

        // Leer datos
        if (bno055_read_accel(&bno, &ax, &ay, &az) != 0) {
            printk("Error leyendo ACC\n");
        }

        if (bno055_read_gyro(&bno, &gx, &gy, &gz) != 0) {
            printk("Error leyendo GYRO\n");
        }

        if (bno055_read_quat(&bno, &qw, &qx, &qy, &qz) != 0) {
            printk("Error leyendo QUAT\n");
        }

        // Imprimir datos
        printk("ACC [m/s2]  X:%6.2f Y:%6.2f Z:%6.2f\n", (double)ax, (double)ay, (double)az);
        printk("GYR [dps]   X:%6.2f Y:%6.2f Z:%6.2f\n", (double)gx, (double)gy, (double)gz);
        printk("QUAT        W:%6.3f X:%6.3f Y:%6.3f Z:%6.3f\n",
               (double)qw, (double)qx, (double)qy, (double)qz);

        printk("-----------------------------\n");

        // Frecuencia de lectura (cada 2 segundos muestra los datos para que se vea un poco mejor)
        k_sleep(K_MSEC(2000)); // 10 Hz (bajo consumo)
    }
}