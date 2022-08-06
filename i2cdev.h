

#ifndef _I2CDEV_H_
#define _I2CDEV_H_
#include <stdint.h>
#include <stdbool.h>
#include "compiler_abstraction.h"
#include "nrf.h"
#ifdef SOFTDEVICE_PRESENT
#include "nrf_soc.h"
#include "app_error.h"
#endif

#define I2C_SDA_PORT gpioPortA
#define I2C_SDA_PIN 0
#define I2C_SDA_MODE gpioModeWiredAnd
#define I2C_SDA_DOUT 1

#define I2C_SCL_PORT gpioPortA
#define I2C_SCL_PIN 1
#define I2C_SCL_MODE gpioModeWiredAnd
#define I2C_SCL_DOUT 1


#define I2CDEV_DEFAULT_READ_TIMEOUT 0

void i2cdev_initialize(void);
void i2cdev_enable(bool isEnabled);
int8_t i2cdev_readByte(uint8_t devAddr, uint8_t regAddr, uint8_t *data);
int8_t i2cdev_readBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t *data);
bool i2cdev_writeByte(uint8_t devAddr, uint8_t regAddr, uint8_t data);
bool i2cdev_writeBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t *data);

#endif /* _I2CDEV_H_ */
