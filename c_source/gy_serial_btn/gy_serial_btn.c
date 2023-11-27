#include <stdbool.h>
#include <stdint.h>
#include "nrf_gpio.h"
#include "nrf_drv_gpiote.h"
#include "pca10040.h"
#include "nrf_log.h"
#include "app_timer.h"
#include "gy_serial_btn.h"

static uint8_t Btns[] = BUTTONS_LIST;
APP_TIMER_DEF(m_timer_btn);         // ��BTNʹ�õĶ�ʱ��ʵ��
btn_handler_t btn_touchEvt_handler;

//******************************************************************************
// fn :BTN_pin_handler
//
// brief : BTN���ű����жϻص�����
//
// param : pin -> ���ź�
//         action -> ���Ա仯��Ϊ
//
// return : none
void BTN_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  switch(pin)
  {
    case BUTTON_1:
    case BUTTON_2:
    case BUTTON_3:
    case BUTTON_4:
    app_timer_start(m_timer_btn, APP_TIMER_TICKS(50), NULL);    // ������ʱ��
      break;
    default:
      break;
  }
}

//******************************************************************
// fn : btn_timeout_handler
//
// brief : ������ʱ����ʱ����
// 
// param : p_event -> ָ�����ݿⷢ���¼���ָ��
//
// return : none
static void btn_timeout_handler(void * p_context)
{ 
  if(nrf_drv_gpiote_in_is_set(BUTTON_1) == 0)
  {
    NRF_LOG_INFO("BTN1");
    btn_touchEvt_handler(BUTTON_1);
  }

  if(nrf_drv_gpiote_in_is_set(BUTTON_2) == 0)
  {
    NRF_LOG_INFO("BTN2");
    btn_touchEvt_handler(BUTTON_2);
  }
  
  if(nrf_drv_gpiote_in_is_set(BUTTON_3) == 0)
  {
    NRF_LOG_INFO("BTN3");
    btn_touchEvt_handler(BUTTON_3);
  }
  
  if(nrf_drv_gpiote_in_is_set(BUTTON_4) == 0)
  {
    NRF_LOG_INFO("BTN4");
    btn_touchEvt_handler(BUTTON_4);
  }

}

//******************************************************************************
// fn :Btn_Init
//
// brief : ��ʼ��Btn����Ϊ���룬ȫ��������ģʽ
//
// param : none
//
// return : none
void BTN_Init(btn_handler_t handler)
{
  btn_touchEvt_handler = handler;
  
  //�����������ýṹ
  nrf_drv_gpiote_in_config_t btn_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
  btn_config.pull = NRF_GPIO_PIN_PULLUP;
  //����Btn����Ϊ��������
  for(uint8_t i = 0 ; i < BUTTONS_NUMBER ; i++)
  {
    nrf_drv_gpiote_in_init(Btns[i], &btn_config, BTN_pin_handler);
    nrf_drv_gpiote_in_event_enable(Btns[i], true);
  }
 
  // ����һ����ʱ�������ڰ�������
  app_timer_create(&m_timer_btn,APP_TIMER_MODE_SINGLE_SHOT,btn_timeout_handler);
}

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
void GPIOTE_Init(void)
{
  //��ʼ��gpiote����
  nrf_drv_gpiote_init();
}
