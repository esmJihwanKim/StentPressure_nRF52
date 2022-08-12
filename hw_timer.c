#include "hw_timer.h"

// Hardware Timer Instance Declarations
const nrfx_timer_t HW_TIMER_INSTANCE = NRFX_TIMER_INSTANCE(0); 
volatile int32_t ms_ticks = 0;

// Software Timer Instance Declarations






void timer0_handler(nrf_timer_event_t event_type, void* p_context) 
{
    switch(event_type)
    {
        case NRF_TIMER_EVENT_COMPARE0:
          ms_ticks += 1; 
          break;
        default: 
          break; 
    }
}

void hw_timer_init (void)
{
      uint32_t err_code; 
      uint32_t time_ms = 1; 
      uint32_t ticks; 

      nrfx_timer_config_t timer_cfg = NRFX_TIMER_DEFAULT_CONFIG;
      err_code = nrfx_timer_init(&HW_TIMER_INSTANCE, &timer_cfg, timer0_handler); 
      APP_ERROR_CHECK(err_code); 

      ticks = nrfx_timer_ms_to_ticks(&HW_TIMER_INSTANCE, time_ms); 

      // last parameter enables interrupt
      nrfx_timer_extended_compare(&HW_TIMER_INSTANCE, NRF_TIMER_CC_CHANNEL0, ticks, NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, true);
      nrfx_timer_enable(&HW_TIMER_INSTANCE);
      return; 
}

