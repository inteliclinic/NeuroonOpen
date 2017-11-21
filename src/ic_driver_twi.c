/**
 * @file    ic_driver_twi.c
 * @Author  Paweł Kaźmierzewski <p.kazmierzewski@inteliclinic.com>
 * @date    July, 2017
 * @brief   Brief description
 *
 * Description
 */
#include "nrf_drv_twi.h"
#include "app_twi.h"

#include "ic_driver_twi.h"
#include "ic_config.h"

#include "app_error.h"
#include "sdk_errors.h"
#include "nrf_assert.h"

#define NRF_LOG_MODULE_NAME "TWI"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "nrf_gpio.h"

#define TWI_READ_OP(addr) (addr|0x01)
#define TWI_WRITE_OP(addr) (addr&0xFE)

// Helper macro. Not intended to be used directly
#define IC_TWI_TRANSFER(_name, _operation, _p_data, _length, _flags)                              \
    _name.p_data    = (uint8_t *)(_p_data);                                                       \
    _name.length    = _length;                                                                    \
    _name.operation = _operation;                                                                 \
    _name.flags     = _flags

// Macro covering Nordics APP_TWI_WRITE
#define IC_TWI_WRITE(name, address, p_data, length, flags)                                        \
    IC_TWI_TRANSFER(name, TWI_WRITE_OP(address), p_data, length, flags)

// Macro covering Nordics APP_TWI_READ
#define IC_TWI_READ(name, address, p_data, length, flags)                                         \
    IC_TWI_TRANSFER(name, TWI_READ_OP(address), p_data, length, flags)

/**
 * @brief
 */
static struct{
  app_twi_t nrf_drv_instance;
  uint8_t twi_instance_cnt;
}m_curren_state =
{
  .nrf_drv_instance = APP_TWI_INSTANCE(IC_TWI_INSTANCE),
  .twi_instance_cnt = 0,
};

static inline void pop_element(ic_twi_transaction_queue_s *queue){
  if(queue->head != queue->tail){
    if(++queue->tail==IC_TWI_PENDIG_TRANSACTIONS)
      queue->tail = 0;
  }
}

static inline bool put_queue_top(
    ic_twi_transaction_queue_s *queue,
    ic_twi_event_cb callback,
    void *context)
{
  uint8_t _head = queue->head;
  uint8_t _tail = queue->tail;

  if(++_head == IC_TWI_PENDIG_TRANSACTIONS)
    _head = 0;

  if(_head == _tail)
    return false;

  queue->callback_array[queue->head].callback = callback;
  queue->callback_array[queue->head].context = context;

  queue->head++;

  return true;
}

#define get_queue_top(v) v.callback_array[v.head]

#define is_queue_empty(v) (v.head == v.tail)

/**
 * @brief TWI IRQ handler
 *
 * @param result    message from driver @ref NRF_ERRORS_BASE
 * @param p_context user data passed by driver
 */
static void m_twi_event_handler(uint32_t result, void *p_context){

  ic_twi_instance_s *_instance = p_context;

  if(_instance != NULL){
    _instance->active = false;
    if(!is_queue_empty(_instance->callback_queue)){
      __auto_type _callback = get_queue_top(_instance->callback_queue).callback;
      __auto_type _context = get_queue_top(_instance->callback_queue).context;

      if(_callback != NULL)
        _callback(result == NRF_SUCCESS ? IC_SUCCESS : IC_ERROR, _context);

      pop_element(&_instance->callback_queue);
    }
  }else{
    NRF_LOG_INFO("no instance data!\n");
  }
}

/**
 * @brief Transaction function
 *
 * @param instance  Externally allocated device instance.
 * @param address   Devices TWI address.
 * @param reg_addr  Target register for read purpose.
 * @param buffer    Preallocated transfer buffer.
 * @param len       Length buffer.
 * @param callback  IRQ handler code.
 * @param read      if true - read transaction. Otherwise - write.
 *
 * @return  IC_SUCCESS, when everything went ok. IC_BUSY when previously started device transaction
 * wasnt handled yet
 */
static ic_return_val_e m_ic_twi_transaction(
    ic_twi_instance_s *const instance,
    uint8_t address,
    uint8_t reg_addr,
    uint8_t *buffer,
    size_t len,
    ic_twi_event_cb callback,
    void *context,
    bool read,
    bool force)
{
  ASSERT(instance!=NULL);
  ASSERT(buffer!=NULL);
  ASSERT(len<=255);

  if(!put_queue_top(&instance->callback_queue, callback, context) == true && !force) {
    return IC_SOFTWARE_BUSY;
  }

  if(read){
    IC_TWI_WRITE(instance->transfers[0], address, &reg_addr, 1, APP_TWI_NO_STOP);
    IC_TWI_READ(instance->transfers[1], address, buffer, len, 0x00);
    instance->transaction.number_of_transfers = 2;
  }
  else{
    IC_TWI_WRITE(instance->transfers[0], address, buffer, len, 0x00);
    instance->transaction.number_of_transfers = 1;
  }

  if(callback != NULL) instance->active = true;

  __auto_type _ret_val = callback == NULL ?
    app_twi_perform(
        &m_curren_state.nrf_drv_instance,
        instance->transaction.p_transfers,
        instance->transaction.number_of_transfers,
        NULL) :
    app_twi_schedule(
        &m_curren_state.nrf_drv_instance,
        &instance->transaction);

  switch (_ret_val){
    case NRF_SUCCESS:
      return IC_SUCCESS;
    case NRF_ERROR_BUSY:
    default:
      if(callback != NULL) instance->active = false;
      return _ret_val == NRF_ERROR_BUSY? IC_DRIVER_BUSY : IC_ERROR;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ic_return_val_e ic_twi_init(ic_twi_instance_s * instance){

  ASSERT(instance!=NULL);

  instance->nrf_twi_instance = (void *)&m_curren_state.nrf_drv_instance;
  instance->active = false;

  instance->transaction.callback = m_twi_event_handler;
  instance->transaction.p_transfers = instance->transfers;
  instance->transaction.p_user_data = instance;

  if(m_curren_state.twi_instance_cnt++ == 0){
    nrf_drv_twi_config_t _twi_config = NRF_DRV_TWI_DEFAULT_CONFIG;
    _twi_config.sda                 = IC_TWI_SDA_PIN;
    _twi_config.scl                 = IC_TWI_SCL_PIN;
    _twi_config.interrupt_priority  = IC_TWI_IRQ_PRIORITY;
    _twi_config.frequency           = (nrf_twi_frequency_t)IC_TWI_FREQUENCY;
    _twi_config.clear_bus_init      = true;
    _twi_config.hold_bus_uninit     = true;

    uint32_t err_code;

    APP_TWI_INIT(
        &m_curren_state.nrf_drv_instance,
        &_twi_config,
        IC_TWI_PENDIG_TRANSACTIONS,
        err_code);
    APP_ERROR_CHECK(err_code);
  }

  return IC_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ic_return_val_e ic_twi_deinit(ic_twi_instance_s *instance){
  instance->nrf_twi_instance = NULL;
  instance->active = false;

  instance->transaction.callback = NULL;
  instance->transaction.p_transfers = NULL;
  instance->transaction.p_user_data = NULL;

  if (--m_curren_state.twi_instance_cnt == 0){
    app_twi_uninit(&m_curren_state.nrf_drv_instance);
  }

  nrf_gpio_cfg_default(IC_TWI_SCL_PIN);
  nrf_gpio_cfg_default(IC_TWI_SDA_PIN);

  return IC_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
ic_return_val_e ic_twi_send(
    ic_twi_instance_s * const instance,
    uint8_t *buffer,
    size_t len,
    ic_twi_event_cb callback,
    void *context,
    bool force)

{

  return m_ic_twi_transaction(
      instance,                 // ic_twi_instance_s *const instance,
      instance->device_address, // uint8_t address,
      0x00,                     // uint8_t reg_addr,
      buffer,                   // uint8_t *buffer,
      len,                      // size_t len,
      callback,                 // ic_twi_event_cb callback,
      context,
      false,
      force);                   // bool read

}

////////////////////////////////////////////////////////////////////////////////////////////////////
ic_return_val_e ic_twi_read(
    ic_twi_instance_s *const instance,
    uint8_t reg_addr,
    uint8_t *buffer,
    size_t len,
    ic_twi_event_cb callback,
    void *context,
    bool force)
{

  return m_ic_twi_transaction(
      instance,                 // ic_twi_instance_s *const instance,
      instance->device_address, // uint8_t address,
      reg_addr,                 // uint8_t reg_addr,
      buffer,                   // uint8_t *buffer,
      len,                      // size_t len,
      callback,                 // ic_twi_event_cb callback,
      context,
      true,
      force);                    // bool read

}
