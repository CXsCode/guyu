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
// brief : ��ʼ��Btn����Ϊ���룬ȫ��������ģʽ
//
// param : none
//
// return : none
void BTN_Init(btn_handler_t handler);

//******************************************************************************
// fn :GPIOTE_Init
//
// brief : ��ʼ��GPIOTEģ��
//
// param : none
//
// return : none
// ATTENTION:
// ������LED_Init��BTN_Init����֮ǰ���á����������е�GPIOTE��غ���֮ǰ����
void GPIOTE_Init(void);


#ifdef __cplusplus
}
#endif

#endif // GY_SERIAL_LED_H__

/** @} */
