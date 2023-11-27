#ifndef GY_SERIAL_LED_H__
#define GY_SERIAL_LED_H__

#ifdef __cplusplus
extern "C" {
#endif

//******************************************************************************
// fn :LED_Init
//
// brief : 初始化LED引脚为输出模式，并熄灭LED
//
// param : none
//
// return : none
void LED_Init(void);

//******************************************************************************
// fn :LED_Control
//
// brief : 控制LED亮灭
//
// param : state -> LED状态
//
// return : none
void LED_Control(uint8_t led, uint8_t state);


#ifdef __cplusplus
}
#endif

#endif // GY_SERIAL_LED_H__

/** @} */
