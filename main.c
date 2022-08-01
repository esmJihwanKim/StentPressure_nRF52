#include "main.h"

static volatile bool m_xfer_done = false;  // Indicates if operation on TWI has ended.
static const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);  // TWI instance
/*
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
*/

/* I2C Initialization. */
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
    err_code = nrf_drv_twi_init(&m_twi, &config, NULL, NULL);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&m_twi);
}

/* Master - Slave I2C communication building blocks : Read from designated registers */
bool i2c_slave_read_bytes(uint8_t device_address, uint8_t register_address, uint8_t length, uint8_t *data)
{
    if(register_address != I2C_NO_MEM_ADDR)
    {
        nrf_drv_twi_tx(&m_twi, device_address, &register_address, 1, true); 
    }      
    
    ret_code_t error = nrf_drv_twi_rx(&m_twi, device_address, data, length); 
    return error == NRF_SUCCESS;
}

bool i2c_slave_read_byte(uint8_t device_address, uint8_t register_address, uint8_t *data) 
{
    return i2c_slave_read_bytes(device_address, register_address, 1, data);
}

 /* Master - Slave I2C communication building blocks : Write to designated registers */
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

bool i2c_slave_write_byte(uint8_t device_address, uint8_t register_address, uint8_t data)
{
    uint8_t w2_data[2]; 
    uint8_t length = 2;

    if(register_address != I2C_NO_MEM_ADDR)
    {
        w2_data[0] = register_address; 
        w2_data[1] = data; 
    }
    else 
    {
        w2_data[0] = data; 
        w2_data[1] = data; 
        length = 1; 
    }
    return NRF_SUCCESS == nrf_drv_twi_tx(&m_twi, device_address, w2_data, length, false);
}


/* ------------------------MS5611 Sensor Specific Functions from here ------------------------- */ 
typedef struct 
{
    uint16_t c1;      // sens == pressure sensitivity 
    uint16_t c2;      // off  == pressure offset 
    uint16_t c3;      // tcs  == temperature coefficient of pressure sensitivity 
    uint16_t c4;      // tco  == temperature coefficient of pressure offset 
    uint16_t c5;      // tref == reference temperature 
    uint16_t c6;      // tempsens == temperature coefficient of the temperature
} PROM_Coefficients;  // page 8 of data sheet  

static PROM_Coefficients prom_coefficients; 
static uint32_t   last_pressure_conversion; 
static uint32_t   last_temperature_conversion; 
static int32_t    temporary_temperature; 
static uint32_t   now;

/* Resets */  
void ms5611_reset(void)
{
    i2c_slave_write_byte(MS5611_ADDR_CSB_LOW, I2C_NO_MEM_ADDR, MS5611_CMD_RESET);
}


/* READ PROM and save the values into prom_coefficients */ 
bool ms5611_read_prom(void)
{
    uint8_t buffer[MS5611_PROM_REGISTER_CELL_SIZE];
    uint16_t* p_uint16_prom_coefficients = (uint16_t*)&prom_coefficients;
    int32_t i = 0; 
    bool status = false; 
    
    for(i = 0; i < MS5611_PROM_REGISTER_COUNT; i++)
    {
        // read sequence 
        status = i2c_slave_write_byte(MS5611_ADDR_CSB_LOW, I2C_NO_MEM_ADDR, MS5611_PROM_BASE_ADDR+(i*MS5611_PROM_REGISTER_CELL_SIZE));
        // read conversion
        if(status)
        {
            status = i2c_slave_read_bytes(MS5611_ADDR_CSB_LOW, I2C_NO_MEM_ADDR, MS5611_PROM_REGISTER_CELL_SIZE, buffer);
            p_uint16_prom_coefficients[i] = ((uint16_t)buffer[0] << 8 ) | buffer[1]; 
        }
    }
    return status; 
}


/* Initialize MS5611 Sensor : Abstracts reset, ms5611_read_prom*/
bool ms5611_init(void)
{
    static bool is_ms5611_init_complete = false; 
    if(is_ms5611_init_complete) 
    {
        return true;
    }
    ms5611_reset();
    nrf_delay_ms(5); 
    if(ms5611_read_prom() == false)
    {
        return false;
    }
    is_ms5611_init_complete = true;
    return true; 
}

/* Start converting raw data */
int32_t ms5611_start_conversion(uint8_t command)
{
    i2c_slave_write_byte(MS5611_ADDR_CSB_LOW, I2C_NO_MEM_ADDR, command);
}

/* Get the converted data */ 
int32_t ms5611_get_convered_data(uint8_t command)
{
    int32_t conversion = 0; 
    uint8_t buffer[MS5611_SAMPLED_DATA_SIZE]; 
    // request for ADC sampled data - 
    i2c_slave_write_byte(MS5611_ADDR_CSB_LOW, I2C_NO_MEM_ADDR, 0x00);   
    i2c_slave_read_bytes(MS5611_ADDR_CSB_LOW, I2C_NO_MEM_ADDR, MS5611_SAMPLED_DATA_SIZE, buffer); 
    // concatenate 8 bit buffers into one complete variable
    conversion =  ((int32_t)buffer[0] << 16) | ((int32_t)buffer[1] << 8) | (buffer[2]); 
    return conversion;
}


//TODO: Get raw temperature 
int32_t ms5611_get_raw_temperature(uint8_t osr) 
{
   // to be implemented 
}


//TODO: Calculate Temperature 
// dT = D2 - tref = D2 - C5 * 2^8 
// TEMP = 20 + dT*TEMPSENS = 2000 + dT * C6 / 2^23 

/* ---------------------------------------------------------------------------------------------*/



int main(void)
{
    // Basic debugging infrasturcture initialization
    APP_ERROR_CHECK(NRF_LOG_INIT(NULL));
    NRF_LOG_DEFAULT_BACKENDS_INIT();
    NRF_LOG_INFO("TWI sensor application started.");
    NRF_LOG_FLUSH();
    
    // I2C master and slave initialization
    i2c_master_init(); 
    if(ms5611_init() == true)
    {
        NRF_LOG_INFO("MS5611 INIT Succeeded"); 
        NRF_LOG_INFO("c1 = SENS     : 0x%x  == %d", prom_coefficients.c1, prom_coefficients.c1);  // sens == pressure sensitivity 
        NRF_LOG_INFO("c2 = off      : 0x%x  == %d", prom_coefficients.c2, prom_coefficients.c2);  // off  == pressure offset 
        NRF_LOG_INFO("c3 = tcs      : 0x%x  == %d", prom_coefficients.c3, prom_coefficients.c3);  // tcs  == temperature coefficient of pressure sensitivity 
        NRF_LOG_INFO("c4 = tco      : 0x%x  == %d", prom_coefficients.c4, prom_coefficients.c4);  // tco  == temperature coefficient of pressure offset 
        NRF_LOG_INFO("c5 = tref     : 0x%x  == %d", prom_coefficients.c5, prom_coefficients.c5);  // tref == reference temperature 
        NRF_LOG_INFO("c6 = tempsens : 0x%x  == %d", prom_coefficients.c6, prom_coefficients.c6);  // tempsens == temperature coefficient of the temperature                                       
        NRF_LOG_FLUSH();                       
    }                                          
                      
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
