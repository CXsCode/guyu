#include <stdbool.h>
#include <stdint.h>
#include "nrf_gpio.h"
#include "pca10040.h"

static uint8_t Leds[] = LEDS_LIST;
//******************************************************************************
// fn :LED_Init
//
// brief : 初始化LED引脚为输出模式，并熄灭LED
//
// param : none
//
// return : none
void LED_Init(void)
{
  uint8_t i = 0;
  
  //配置LED引脚为输出模式
  nrf_gpio_range_cfg_output(LED_START, LED_STOP);
  
  //置LED引脚为高电平，即LED灭
  for(i = 0 ; i < LEDS_NUMBER; i++)
  {
    nrf_gpio_pin_set(Leds[i]);
  }
}

//******************************************************************************
// fn :LED_Control
//
// brief : 控制LED亮灭
//
// param : state -> LED状态
//
// return : none
void LED_Control(uint8_t led, uint8_t state)
{
  if((led < Leds[0]) || (led > Leds[3]))
  {
    return;
  }
  
  if(state)
  {
    nrf_gpio_pin_set(led);
  }
  else
  {
    nrf_gpio_pin_clear(led);
  }
}
