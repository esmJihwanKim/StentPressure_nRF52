#ifndef _MS5611_H
#define _MS5611_H

#include <stdbool.h>
#include "nrf_drv_twi.h"

/* nRF pins for I2C bus */
#define SCL_PIN 22
#define SDA_PIN 23


/* TWI instance ID. */
#define TWI_INSTANCE_ID    0

/* Common addresses definition for temperature sensor. */
#define MS5611_ADDR_CSB_LOW       0x77U  // CSB = GND

#define MS5611_CMD_READ_ADC       0x00U
#define MS5611_CMD_READ_PROM      0xA0U
#define MS5611_CMD_RESET          0x1EU
#define MS5611_CMD_CONVERT_D1     0x40U
#define MS5611_CMD_CONVERT_D2     0x50U

#define MS5611_OSR_4096           0x08U

#define MS5611_PROM_BASE_ADDR               0xA2U
#define MS5611_PROM_REGISTER_COUNT          6
#define MS5611_PROM_REGISTER_CELL_SIZE      2   // 2 bytes exsits in each PROM register 

#define I2C_NO_MEM_ADDR   0xFFU

// D1 and D2 result size (bytes) : 8 * 3 = 24 bits -> from 24bit ADC
#define MS5611_SAMPLED_DATA_SIZE     3   

extern volatile uint32_t ms_ticks;

#endif