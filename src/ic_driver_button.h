/**
 * @file    ic_driver_button.h
 * @Author  Paweł Kaźmierzewski <p.kazmierzewski@inteliclinic.com>
 * @date    June, 2017
 * @brief   Brief description
 *
 * Description
 */

#ifndef IC_DRIVER_BUTTON_H
#define IC_DRIVER_BUTTON_H

#include "ic_config.h"

enum exti_edge_dir{
  EXTI_EDGE_UP = 0x00,
  EXTI_EDGE_DOWN
};

typedef void(*p_btn_code)();
typedef void(*p_exti_code)(enum exti_edge_dir);

void ic_btn_pwr_press_handle_init(p_btn_code code);
void ic_btn_pwr_release_handle_init(p_btn_code code);
void ic_btn_pwr_long_press_handle_init(p_btn_code code);
void ic_btn_usb_connect_handle_init(p_btn_code code);
void ic_btn_usb_disconnect_handle_init(p_btn_code code);
void ic_acc_exti_handle_init(p_exti_code code);
void ic_afe_exti_handle_init(p_exti_code code);
ic_return_val_e ic_neuroon_exti_init(void);

#endif /* !IC_DRIVER_BUTTON_H */
