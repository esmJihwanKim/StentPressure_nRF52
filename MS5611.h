#ifndef _MS5611_H
#define _MS5611_H

#include <stdbool.h>
#include "nrf_drv_twi.h"

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

#define I2C_PROHIBITED_MEM_ADDR   0xFFU

#endif