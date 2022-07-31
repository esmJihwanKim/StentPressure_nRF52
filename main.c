#include "main.h"


/* Indicates if operation on TWI has ended. */
static volatile bool m_xfer_done = false; 

/* TWI instance. */
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

/* Buffer for samples read from temperature sensor. */
static uint8_t m_sample;



/*
    Handler for I2C event occuring 
*/
void i2c_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
    switch (p_event->type)
    {
        case NRF_DRV_TWI_EVT_DONE:
            m_xfer_done = true;
            break;
        default:
            break;
    }
}

/*
    I2C Initialization.
*/
void i2c_master_init (void)
{
    ret_code_t err_code;

    const nrf_drv_twi_config_t config = {
       .scl                = SCL_PIN,
       .sda                = SDA_PIN,
       .frequency          = NRF_DRV_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_LOW,
       .clear_bus_init     = false
    };
      
    // initialization using nRF driver. param : instance, configuration, twi_handler, init handler
    err_code = nrf_drv_twi_init(&m_twi, &config, i2c_handler, NULL);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&m_twi);
}

void i2c_slave_read_bytes(uint8_t device_address, uint8_t register_address, uint8_t length, uint8_t *data)
{
    //
    if(register_address != I2C_PROHIBITED_MEM_ADDR)
        nrf_drv_twi_tx(&m_twi, device_address, &register_address, 1, true); 
    
    ret_code_t error = nrf_drv_twi_rx(&m_twi, device_address, data, length); 
}


bool i2c_slave_write_bytes(uint8_t device_address, uint8_t register_address, uint8_t length, uint8_t *data)
{
    // fill the first byte with address and the rest with data
    uint8_t buffer[32]; 
    buffer[0] = register_address; 
    uint8_t i = 1; 
    while(i < (length + 1))
    {
        buffer[i++] = *data++; 
    }
    return NRF_SUCCESS == nrf_drv_twi_tx(&m_twi, device_address, buffer, length+1, false);  
} 


int main(void)
{
    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    NRF_LOG_INFO("\r\nTWI sensor example started.");
    NRF_LOG_FLUSH();
    i2c_master_init();
    nrf_delay_ms(500);



    while (true)
    {
        nrf_delay_ms(500);

        do
        {
            __WFE();
        }while (m_xfer_done == false);

        NRF_LOG_FLUSH();
    }
}

/** @} */
