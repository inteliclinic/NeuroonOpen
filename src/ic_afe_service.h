/**
 * @file    ic_afe_service.h
 * @Author  Kacper Gracki <k.gracki@inteliclinic.com>
 * @date    July, 2017
 * @brief   Brief description
 *
 * Description
 */

#ifndef _IC_AFE_SERVICE_H
#define _IC_AFE_SERVICE_H

#include <stdint.h>
#include <stddef.h>

#include "ic_driver_afe4400.h"
#include "nrf_drv_gpiote.h"

/**
 * if you want to use AFE4400 with RDY interrupt
 * uncomment below
 */
//#define _USE_AFE_INT

/**
 * @brief Initialize AFE4400 module
 *
 * By calling this function, you initialize all modules used in ic_afe_service
 *
 * afe_gpio_configuration() - configuration used ports/pins for afe4400
 * gpio_interrupt_init()    - interrupt configuration for handling RDY pin in afe4400
 * afe_task() 		    - afe4400 testing function which calls all the necessary functions for start using afe module
 *
 *   -> afe_init() 								- init SPI connection and set afe for reading mode
 *   -> afe_check_diagnostic()							- check diagnostic register to see whether everything is okay with your afe4400
 *   -> afe_set_default_timing()/afe_set_default_timing_fast(data, data_size) 	- set timing values in timing registers
 *   -> afe_set_led_current(led1_value, led2_value) 				- set led current values
 *   -> afe_set_gain(tia_amb_gain_values) 					- set correct values in TIA_AMB_GAIN register
 *   -> afe_begin_measure() 							- set the last needed values for starting measurements
 *
 * It is necessary to call nrf_drv_gpiote_in_event_enable() and NVIC_EnableIRQ() functions AFTER setting afe_begin_measure()
 * to be sure, that interrupt will not start until setting correct values in specific registers
 *
 *   -> nrf_drv_gpiote_in_event_enable(AFE4400_RDY_PIN, true);
 *   -> NVIC_EnableIRQ(GPIOTE_IRQn);
 *
 * @return
 */
ic_return_val_e ic_afe_init(void(*cb)(s_led_val));

/**
 * @brief Deinitialize AFE4400 module
 *
 * Delete all created freeRTOS tasks and timers
 *
 * @return
 */
ic_return_val_e ic_afe_deinit(void);

/**
 * @brief Set gain for afe4400
 * @param tia_amb_value @ref s_tia_amb_gain:
 *
 *  Specific ambient light cancellation amplifier gain, cancellation current and filter corner frequency in AFE4400
 *    .amb_dac using  @ref e_amb_dac  - ambient DAC value (0 - 10uA)
 *    .stg2gain using @ref e_stg2gain - stage 2 gain setting (0dB; 3.5dB; 6dB; 9.5dB; 12dB)
 *    .cfLED using    @ref e_cfLED    - LEDs Cf value
 *    .rfLED using    @ref e_rfLED    - LEDs Rf value
 *
 * @return
 */
ic_return_val_e ic_afe_set_gain(s_tia_amb_gain *tia_amb_value);

/**
 * @brief Set current value on LED1 and LED2
 * @param led1 - current value on led1 (0-255)
 * @param led2 - current value on led2 (0-255)
 *
 * The nominal value of LED current is given by the equation (page 65 in AFE4400 datasheet):
 * ( LED[7:0] / 256 ) * Full-Scale Current
 *
 * @return
 */
ic_return_val_e ic_afe_set_led_current(uint8_t led1, uint8_t led2);

/**
 * @brief Set timing values in specific registers faster than afe_set_timing_data function
 * @param tim_array
 * @param len
 *
 * @return
 */
ic_return_val_e ic_afe_set_timing(uint16_t *tim_array, size_t len);

/**
 * @brief Self-test for afe4400 module
 *  Self-test is necessary for checking whether all used functions
 *  are working fine and afe module can be used in working way
 *
 * @return IC_SUCCESS when self-test went okay
 */
ic_return_val_e ic_afe_self_test();

#endif /* !_IC_AFE_SERVICE_H */
