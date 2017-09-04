/**
 * @file    ic_driver_twi.h
 * @Author  Paweł Kaźmierzewski <p.kazmierzewski@inteliclinic.com>
 * @date    July, 2017
 * @brief   Brief description
 *
 * Description
 */

#ifndef IC_DRIVER_TWI_H
#define IC_DRIVER_TWI_H
#include <stdint.h>
#include <stddef.h>

#include "ic_common_types.h"
#include "app_twi.h"

typedef void (*ic_twi_event_cb)(void *context);

typedef struct{
  void *nrf_twi_instance;
  ic_twi_event_cb callback;
  bool active;
  uint8_t device_address;
  app_twi_transaction_t transaction;
  app_twi_transfer_t transfers[2];
}ic_twi_instance_s;

#define TWI_REGISTER(name, address)                                                               \
  static ic_twi_instance_s name##_twi_instance = {.device_address};

#define TWI_INIT(name)                                                            \
  ic_twi_init(&name##_twi_instance);

#define TWI_UNINIT(name)                                                                          \
  ic_twi_uninit(&name##_twi_instance);

#define TWI_SEND_DATA(                                                                            \
    name,                                                                                         \
    in_buffer,                                                                                    \
    len,                                                                                          \
    callback)                                                                                     \
  ic_twi_send(&name##_twi_instance, in_buffer, len, callback)

#define TWI_READ_DATA(                                                                            \
    name,                                                                                         \
    reg_addr,                                                                                     \
    in_buffer,                                                                                    \
    len,                                                                                          \
    callback)                                                                                     \
  ic_twi_read(&name##_twi_instance, reg_addr, in_buffer, len, callback)

ic_return_val_e ic_twi_init(ic_twi_instance_s * instance);

ic_return_val_e ic_twi_uninit(ic_twi_instance_s *instance);

ic_return_val_e ic_twi_send(
    ic_twi_instance_s * const instance,
    uint8_t *in_buffer,
    size_t len,
    ic_twi_event_cb callback);

ic_return_val_e ic_twi_read(
    ic_twi_instance_s *const instance,
    uint8_t reg_addr,
    uint8_t *in_buffer,
    size_t len,
    ic_twi_event_cb callback);

#endif /* !IC_DRIVER_TWI_H */
