
#include "nrf_drv_twi.h"
#include "app_util_platform.h"
#include "app_error.h"
#include "i2cdev.h"

#include <stdio.h>
#include <string.h>

#define I2CDEV_NO_MEM_ADDR  0xFF

#define SCL_PIN 22
#define SDA_PIN 23

static nrf_drv_twi_t m_twi=NRF_DRV_TWI_INSTANCE(0);

void i2cdev_initialize(void) {
    ret_code_t err_code;

    const nrf_drv_twi_config_t config = {
       .scl                = SCL_PIN,
       .sda                = SDA_PIN,
       .frequency          = NRF_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_LOWEST
    };

    err_code = nrf_drv_twi_init(&m_twi, &config, NULL, NULL);
    APP_ERROR_CHECK(err_code);
}

/** Enable or disable I2C
 * @param isEnabled true = enable, false = disable
 */
void i2cdev_enable(bool isEnabled) {
  
	if (isEnabled)
		nrf_drv_twi_enable(&m_twi);
	else
		nrf_drv_twi_disable(&m_twi);

}


/** Read single byte from an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param data Container for byte value read from device
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in i2cdev_readTimeout)
 * @return Status of read operation (true = success)
 */
int8_t i2cdev_readByte(uint8_t devAddr, uint8_t regAddr, uint8_t *data) {
    return i2cdev_readBytes(devAddr, regAddr, 1, data);
}

/** Read multiple bytes from an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr First register regAddr to read from
 * @param length Number of bytes to read
 * @param data Buffer to store read data in
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in i2cdev_readTimeout)
 * @return I2C_TransferReturn_TypeDef http://downloads.energymicro.com/documentation/doxygen/group__I2C.html
 */
int8_t i2cdev_readBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t *data) {
	
	//Used for MS5611 Pressure sensor
	if(regAddr != I2CDEV_NO_MEM_ADDR) 
        {
		nrf_drv_twi_tx(&m_twi,devAddr,&regAddr,1,true);
	}
	
	ret_code_t r= nrf_drv_twi_rx(&m_twi,devAddr,data,length);

	return r==NRF_SUCCESS;
  
}

/** Write single byte to an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register address to write to
 * @param data New byte value to write
 * @return Status of operation (true = success)
 */
bool i2cdev_writeByte(uint8_t devAddr, uint8_t regAddr, uint8_t data) {
    uint8_t w2_data[2];
		uint8_t length = 2;
		
		if(regAddr != I2CDEV_NO_MEM_ADDR) {
			w2_data[0] = regAddr;
			w2_data[1] = data;
		} else {
			w2_data[0] = data;
			w2_data[1] = data;
			length = 1;
		}
    return NRF_SUCCESS==nrf_drv_twi_tx(&m_twi,devAddr,w2_data,length,false);
}

/** Write multiple bytes to an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr First register address to write to
 * @param length Number of bytes to write
 * @param data Buffer to copy new data from
 * @return Status of operation (true = success)
 */
bool i2cdev_writeBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t* data) {
	uint8_t buffer[32];
	buffer[0] = regAddr;
	uint8_t i = 1;
	while(i < (length + 1))
		buffer[i++] = *data++;
	
	return NRF_SUCCESS==nrf_drv_twi_tx(&m_twi,devAddr,buffer,length+1,false);
}

