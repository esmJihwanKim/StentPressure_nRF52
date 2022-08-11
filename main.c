#include "main.h"

static volatile bool m_xfer_done = false;  // Indicates if operation on TWI has ended.

int main(void)
{
    // Basic debugging infrasturcture initialization
    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    NRF_LOG_DEFAULT_BACKENDS_INIT();
    NRF_LOG_INFO("TWI sensor application started.");
    NRF_LOG_FLUSH();
    
    // 1ms hardware timer
    hw_timer_init();
    

    // I2C master and slave initialization
    i2cdev_initialize();
    i2cdev_enable(true); 
    if(ms5611Init() == true)
    {
        NRF_LOG_INFO("MS5611INIT Succeeded"); 
        NRF_LOG_FLUSH();
    }

    int32_t temperature, pressure; 
    // stops the main function from ending
    while (true)                               
    {              
        //temperature = ms5611_get_raw_temperature(MS5611_OSR_4096); 
        //ressure = ms5611_get_raw_pressure(MS5611_OSR_4096);    
        ms5611_get_calibrated_data(&temperature, &pressure); 
        NRF_LOG_INFO("TEST TEMPERATURE DATA:::%d", temperature);     
        NRF_LOG_INFO("TEST PRESSURE DATA:::%d", pressure);                         
        NRF_LOG_FLUSH(); 
    }

    return 0;
}

/** @} */
