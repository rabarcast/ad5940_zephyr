#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>

#define I2C_NODE DT_NODELABEL(i2c1)
#define BNO055_ADDR 0x28

//Registers
#define BNO055_CHIP_ID 0x00
#define BNO055_ACCEL_DATA 0x08

int main(void)
{
    const struct device *i2c = DEVICE_DT_GET(I2C_NODE);

    if (!device_is_ready(i2c)) {
        printk("I2C not ready\n");
        return 0;
    }

    printk("Reading I2C...\n");

    uint8_t chip_id;
    if (i2c_write_read(i2c, BNO055_ADDR, (uint8_t[]){BNO055_CHIP_ID}, 1, &chip_id, 1)!= 0){
        printk("ERROR while reading chip ID\n");
        return 0;
    }

    printk("Chip ID: 0x%02X\n", chip_id);

    if (chip_id != 0xA0) {
        printk("BNO055 no detectado correctamente\n");
        return 0;
    }

    // Cambiar a modo CONFIG (por seguridad)
    uint8_t config_mode[2] = {0x3D, 0x00};
    i2c_write(i2c, config_mode, 2, BNO055_ADDR);
    k_sleep(K_MSEC(20));

    // Cambiar a modo NDOF
    uint8_t ndof_mode[2] = {0x3D, 0x0C};
    i2c_write(i2c, ndof_mode, 2, BNO055_ADDR);
    k_sleep(K_MSEC(20));

    printk("NDOF Mode Activated\n");

    while (1) {
        uint8_t data[6];
        uint8_t cont = 0;

        // Read Accel X,Y,Z
        if (i2c_write_read(i2c, BNO055_ADDR,
                           (uint8_t[]){BNO055_ACCEL_DATA}, 1,
                           data, 6) == 0) {

            int16_t x = (data[1] << 8) | data[0];
            int16_t y = (data[3] << 8) | data[2];
            int16_t z = (data[5] << 8) | data[4];

            printk("%d: ACCEL X:%d Y:%d Z:%d\n",cont, x, y, z);
            cont++;
        } else {
            printk("Read Error\n");
        }

        k_sleep(K_MSEC(500));
    }
}