#ifndef GY_PROFILE_LED_H__
#define GY_PROFILE_LED_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"
#include "nrf_sdh_ble.h"

#ifdef __cplusplus
extern "C" {
#endif


//******************************************************************************
// fn :BLE_LED_DEF
//
// brief : 初始化LED服务实例
//
// param : _name -> 实例的名称
//
// return : none
#define BLE_LED_DEF(_name)                                                                          \
static ble_led_t _name;                                                                             \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     2,                                                     \
                     ble_led_on_ble_evt, &_name)


// 定义服务的UUID，以及特征值相关参数
#define LED_UUID_BASE        {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, \
                              0xDE, 0xEF, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00}

#define LED_UUID_SERVICE     0xFFF0
#define LED_UUID_CHAR        0xFFF1

#define LED_UUID_CHAR_LEN    4


// 申明ble_led_t类型
typedef struct ble_led_s ble_led_t;

// 定义一个回调函数
typedef void (*ble_led_write_handler_t) (uint8_t * new_state);

// LED服务初始化结构体
typedef struct
{
    uint8_t *p_led_value;
    ble_led_write_handler_t led_write_handler; /**< Event handler to be called when the LED Characteristic is written. */
} ble_led_init_t;

// LED服务结构体
struct ble_led_s
{
    uint8_t                     uuid_type;           /**< UUID type for the LED Button Service. */
    uint16_t                    service_handle;      /**< Handle of LED Button Service (as provided by the BLE stack). */
    ble_gatts_char_handles_t    led_char_handles;    /**< Handles related to the LED Characteristic. */
    ble_led_write_handler_t     led_write_handler;   /**< Event handler to be called when the LED Characteristic is written. */
};


//******************************************************************************
// fn :ble_led_init
//
// brief : 初始化LED服务
//
// param : p_led -> led服务结构体
//         p_led_init -> led服务初始化结构体
//
// return : uint32_t -> 成功返回SUCCESS，其他返回ERR NO.
uint32_t ble_led_init(ble_led_t * p_lbs, const ble_led_init_t * p_led_init);


//******************************************************************************
// fn :ble_led_on_ble_evt
//
// brief : BLE事件处理函数
//
// param : p_ble_evt -> ble事件
//         p_context -> ble事件处理程序的参数（暂时理解应该是不同的服务，注册时所携带的结构体参数）
//
// return : none
void ble_led_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);


#ifdef __cplusplus
}
#endif

#endif // GY_PROFILE_LED_H__

/** @} */
