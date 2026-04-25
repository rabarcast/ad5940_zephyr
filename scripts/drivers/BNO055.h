/*
    BNO055 module header file
*/

#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define REG_CHIP_ID     0x00
#define REG_OPR_MODE    0x3D
#define REG_PWR_MODE    0x3E
#define REG_UNIT_SEL    0x3B
#define REG_SYS_TRIGGER 0x3F

#define REG_ACC_CONFIG  0x08
#define REG_GYR_CONFIG0 0x0A
#define REG_GYR_CONFIG1 0x0B

#define REG_ACC_DATA    0x08
#define REG_QUAT_DATA   0x20

struct bno055_dev{
    struct i2c_dt_spec i2c;
};

int bno055_write_reg(const struct i2c_dt_spec *i2c,uint8_t reg, uint8_t value);
int bno055_read_reg(const struct i2c_dt_spec *i2c, uint8_t reg, uint8_t *value);
int bno055_reset(struct bno055_dev *dev);
int bno055_read_chip_id(struct bno055_dev *dev, uint8_t *id);
int bno055_init(struct bno055_dev *dev);
int bno055_read_accel(struct bno055_dev *dev,float *x, float *y, float *z);
int bno055_read_gyro(struct bno055_dev *dev, float *x, float *y, float *z);
int bno055_read_quat(struct bno055_dev *dev, float *w, float *x, float *y, float *z);
