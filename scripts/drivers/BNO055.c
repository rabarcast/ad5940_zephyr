/*

BNO055 Bosch Module Driver

*/

#include "BNO055.h"

int bno055_write_reg(const struct i2c_dt_spec *i2c,
                            uint8_t reg,
                            uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    return i2c_write_dt(i2c, buf, 2);
}

int bno055_read_reg(const struct i2c_dt_spec *i2c,
                           uint8_t reg,
                           uint8_t *value)
{
    return i2c_write_read_dt(i2c, &reg, 1, value, 1);
}

int bno055_reset(struct bno055_dev *dev)
{
    int ret = bno055_write_reg(&dev->i2c, REG_SYS_TRIGGER, 0x20);
    k_sleep(K_MSEC(650));
    return ret;
}

int bno055_read_chip_id(struct bno055_dev *dev, uint8_t *id)
{
    int ret = bno055_read_reg(&dev->i2c, REG_CHIP_ID, id);

    if (ret == 0) {
        printk("Chip ID: 0x%02X\n", *id);
    } else {
        printk("Error leyendo ID\n");
    }

    return ret;
}

int bno055_init(struct bno055_dev *dev)
{
    uint8_t id;

    k_sleep(K_MSEC(650));

    if (bno055_read_chip_id(dev, &id) != 0 || id != 0xA0) {
        printk("BNO055 no detectado\n");
        return -1;
    }

    printk("BNO055 OK\n");

    // CONFIG MODE
    bno055_write_reg(&dev->i2c, REG_OPR_MODE, 0x00);
    k_sleep(K_MSEC(20));

    // LOW POWER
    bno055_write_reg(&dev->i2c, REG_PWR_MODE, 0x01);

    // UNITS: m/s2 + Android
    bno055_write_reg(&dev->i2c, REG_UNIT_SEL, 0x00);

    // ACC: 16G | 125Hz | low power
    uint8_t acc = (0x03) | (0x04 << 2) | (0x02 << 5);
    bno055_write_reg(&dev->i2c, REG_ACC_CONFIG, acc);

    // GYRO: 500 dps | 116 Hz
    uint8_t gyr0 = (0x02 << 3) | (0x04);
    bno055_write_reg(&dev->i2c, REG_GYR_CONFIG0, gyr0);

    // GYRO normal
    bno055_write_reg(&dev->i2c, REG_GYR_CONFIG1, 0x00);

    // NDOF
    bno055_write_reg(&dev->i2c, REG_OPR_MODE, 0x0C);
    k_sleep(K_MSEC(20));

    return 0;
}

int bno055_read_accel(struct bno055_dev *dev,
                      float *x, float *y, float *z)
{
    uint8_t reg = REG_ACC_DATA;
    uint8_t data[6];

    if (i2c_write_read_dt(&dev->i2c, &reg, 1, data, 6) != 0) {
        return -1;
    }

    int16_t rx = (data[1] << 8) | data[0];
    int16_t ry = (data[3] << 8) | data[2];
    int16_t rz = (data[5] << 8) | data[4];

    *x = rx * 0.00981f;
    *y = ry * 0.00981f;
    *z = rz * 0.00981f;

    return 0;
}

int bno055_read_gyro(struct bno055_dev *dev,
                     float *x, float *y, float *z)
{
    uint8_t reg = 0x14;
    uint8_t data[6];

    if (i2c_write_read_dt(&dev->i2c, &reg, 1, data, 6) != 0) {
        return -1;
    }

    int16_t rx = (data[1] << 8) | data[0];
    int16_t ry = (data[3] << 8) | data[2];
    int16_t rz = (data[5] << 8) | data[4];

    // 1 LSB = 1/16 dps
    *x = rx / 16.0f;
    *y = ry / 16.0f;
    *z = rz / 16.0f;

    return 0;
}

int bno055_read_quat(struct bno055_dev *dev,
                     float *w, float *x,
                     float *y, float *z)
{
    uint8_t reg = REG_QUAT_DATA;
    uint8_t data[8];

    if (i2c_write_read_dt(&dev->i2c, &reg, 1, data, 8) != 0) {
        return -1;
    }

    int16_t qw = (data[1] << 8) | data[0];
    int16_t qx = (data[3] << 8) | data[2];
    int16_t qy = (data[5] << 8) | data[4];
    int16_t qz = (data[7] << 8) | data[6];

    *w = qw / 16384.0f;
    *x = qx / 16384.0f;
    *y = qy / 16384.0f;
    *z = qz / 16384.0f;

    return 0;
}