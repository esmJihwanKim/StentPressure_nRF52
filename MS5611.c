
#define DEBUG_MODULE "MS5611"

#include "ms5611.h"
#include "i2cdev.h"
#include <stdio.h>
#include <stdlib.h>
#include "Segger_RTT.h"

#include "math.h"
#include "systick.h"

#include "nrf_delay.h"

#define EXTRA_PRECISION      5 	// trick to add more precision to the pressure and temp readings
#define CONVERSION_TIME_MS   10 // conversion time in milliseconds. 10 is minimum
#define PRESSURE_PER_TEMP 5 		// Length of reading cycle: 1x temp, rest pressure. Good values: 1-10
#define FIX_TEMP 25         		// Fixed Temperature. ASL is a function of pressure and temperature, but as the temperature changes so much (blow a little towards the flie and watch it drop 5 degrees) it corrupts the ASL estimates.
																// TLDR: Adjusting for temp changes does more harm than good.

typedef struct
{
  uint16_t psens;
  uint16_t off;
  uint16_t tcs;
  uint16_t tco;
  uint16_t tref;
  uint16_t tsens;
} CalReg;

static uint8_t devAddr;
static bool isInit;

static CalReg   calReg;
static uint32_t lastPresConv;
static uint32_t lastTempConv;
static int32_t  tempCache;
static int32_t  presCache; 

//static uint8_t readState=0;
//static uint32_t lastConv=0;
static int32_t tempDeltaT;



/**
 * Send a reset command to the device. With the reset command the device
 * populates its internal registers with the values read from the PROM.
 */
void ms5611Reset()
{
    i2cdev_writeByte(devAddr, I2CDEV_NO_MEM_ADDR, MS5611_RESET);
}

/**
 * Reads factory calibration and store it into object variables.
 */
bool ms5611ReadPROM()
{
  uint8_t buffer[MS5611_PROM_REG_SIZE];
  uint16_t* pCalRegU16 = (uint16_t*)&calReg;
  int32_t i = 0;
  bool status = false;

  for (i = 0; i < MS5611_PROM_REG_COUNT; i++)
  {
    // start read sequence
    status = i2cdev_writeByte(devAddr, I2CDEV_NO_MEM_ADDR,
                             MS5611_PROM_BASE_ADDR + (i * MS5611_PROM_REG_SIZE));
    // Read conversion
    if (status)
    {
      status = i2cdev_readBytes(devAddr, I2CDEV_NO_MEM_ADDR, MS5611_PROM_REG_SIZE, buffer);
      pCalRegU16[i] = ((uint16_t)buffer[0] << 8) | buffer[1];
    }
  }

  return status;
}




bool ms5611Init(void)
{
  if (isInit)
    return true;

  devAddr = MS5611_ADDR_CSB_LOW;

  ms5611Reset(); // reset the device to populate its internal PROM registers
  nrf_delay_ms(5);
  if (ms5611ReadPROM() == false) // reads the PROM into object variables for later use
  {
      return false;
  }

  isInit = true;

  return true;
}

// see page 11 of the datasheet
void ms5611StartConversion(uint8_t command)
{
  // initialize pressure conversion
  i2cdev_writeByte(devAddr, I2CDEV_NO_MEM_ADDR, command);
}

int32_t ms5611GetConversion(uint8_t command)
{
  int32_t conversion = 0;
  uint8_t buffer[MS5611_D1D2_SIZE];

  // start read sequence
  i2cdev_writeByte(devAddr, I2CDEV_NO_MEM_ADDR, 0);
  // Read conversion
  i2cdev_readBytes(devAddr, I2CDEV_NO_MEM_ADDR, MS5611_D1D2_SIZE, buffer);
  conversion = ((int32_t)buffer[0] << 16) |
               ((int32_t)buffer[1] << 8) | buffer[2];

  return conversion;
}


int32_t ms5611_get_raw_temperature(uint8_t osr)
{
    uint32_t now = ms_ticks;
    if (lastTempConv != 0 && (now - lastTempConv) >= CONVERSION_TIME_MS)
    {
        lastTempConv = 0;
        tempCache = ms5611GetConversion(MS5611_D2 + osr);
        return tempCache;
    }
    else
    {
        if (lastTempConv == 0 && lastPresConv == 0)
        {
            ms5611StartConversion(MS5611_D2 + osr);
            lastTempConv = now;
        }
        return tempCache;
    }
}


int32_t ms5611_get_raw_pressure(uint8_t osr)
{
  uint32_t now = ms_ticks;
  if (lastPresConv != 0 && (now - lastPresConv) >= CONVERSION_TIME_MS)
  {
    lastPresConv = 0;
    presCache = ms5611GetConversion(MS5611_D1 + osr);
    return presCache;
  }
  else
  {
    if (lastPresConv == 0 && lastTempConv == 0)
    {
      ms5611StartConversion(MS5611_D1 + osr);
      lastPresConv = now;
    }
    return presCache;
  }
}

// result_array[0] = temperature 
// result_array[1] = pressure
/* acquires temperature and pressure : take in pointer and manipulate. no output required */
void ms5611_get_calibrated_data(float* result_temperature, float* result_pressure)
{
    int64_t raw_pressure = ms5611_get_raw_pressure(MS5611_OSR_4096);
    int64_t raw_temperature = ms5611_get_raw_temperature(MS5611_OSR_4096); 
    // formulas in datasheet page 7 
    int32_t dT, temp, p;
    int64_t t2, p2, off2, sens2;

    if(raw_pressure == 0 || raw_temperature == 0) 
    {
        NRF_LOG_INFO("ADC SAMPLING ERROR");
    }

    // page 8 & 9 
    // dT = D2 - C5 * 2^8     :  Difference between actual and reference temp 
    dT = raw_temperature - (((int64_t)calReg.tref) << 8); 

    // 2000 + dT * C6 / 2^23  :  Actual temperature 
    temp = 2000 + ((int64_t)dT * (int64_t)calReg.tref >> 23); 

    int64_t off = (((int64_t)calReg.off) << 16 ) + ((calReg.tco * dT) >> 7); 
    int64_t sens = (((int64_t)calReg.psens) << 15) + ((calReg.tcs * dT) >> 8); 
    
    p = (((raw_pressure * sens) >> 21) - off) >> 15; 

    *result_temperature = (float)temp / 100.0; 
    *result_pressure = (float)p / 100.0;

    return; 
}



