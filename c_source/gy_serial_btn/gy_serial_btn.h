#ifndef GY_SERIAL_BTN_H__
#define GY_SERIAL_BTN_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "nrf_drv_gpiote.h"
  
typedef void (* btn_handler_t) (uint8_t butState);

//******************************************************************************
// fn :Btn_Init
//
// brief : 初始化Btn引脚为输入，全边沿敏感模式
//
// param : none
//
// return : none
void BTN_Init(btn_handler_t handler);

//******************************************************************************
// fn :GPIOTE_Init
//
// brief : 初始化GPIOTE模块
//
// param : none
//
// return : none
// ATTENTION:
// 必须在LED_Init与BTN_Init函数之前调用。即，在所有的GPIOTE相关函数之前调用
void GPIOTE_Init(void);


#ifdef __cplusplus
}
#endif

#endif // GY_SERIAL_LED_H__

/** @} */
