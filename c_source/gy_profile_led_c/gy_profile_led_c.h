#ifndef GY_PROFILE_LED_C_H__
#define GY_PROFILE_LED_C_H__

#include <stdint.h>
#include "ble.h"
#include "ble_db_discovery.h"
#include "nrf_sdh_ble.h"

#ifdef __cplusplus
extern "C" {
#endif

//******************************************************************************
// fn :BLE_LED_C_DEF
//
// brief : 初始化LED客户端服务实例
//
// param : _name -> 实例的名称
//
// return : none
#define BLE_LED_C_DEF(_name)                                                                        \
static ble_led_c_t _name;                                                                           \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     2,                                                   \
                     ble_led_c_on_ble_evt, &_name)



#define LED_UUID_BASE        {0x23, 0xD1, 0xBC, 0xEA, 0x5F, 0x78, 0x23, 0x15, \
                              0xDE, 0xEF, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00}

#define LED_UUID_SERVICE     0xFFF0
#define LED_UUID_CHAR        0xFFF1

#define LED_UUID_CHAR_LEN    4

// 定义服务的UUID，以及特征值相关参数
typedef enum
{
    BLE_LED_C_EVT_DISCOVERY_COMPLETE = 1,  /**< Event indicating that the LED Button Service has been discovered at the peer. */
} ble_led_c_evt_type_t;


// brief结构，其中包含与对等方上的LED Button Service相关的句柄
typedef struct
{
    uint16_t led_handle;          // led特征值的句柄
} led_db_t;

// LED服务结构体
typedef struct
{
    ble_led_c_evt_type_t evt_type;    /**< Type of the event. */
    uint16_t             conn_handle; /**< Connection handle on which the event occured.*/
    union
    {
        led_db_t     peer_db;         /**< LED Button Service related handles found on the peer device. This will be filled if the evt_type is @ref BLE_LED_C_EVT_DISCOVERY_COMPLETE.*/
    } params;
} ble_led_c_evt_t;

// Forward declaration of the ble_led_c_t type.
typedef struct ble_led_c_s ble_led_c_t;

// LED服务客户端回调函数
typedef void (* ble_led_c_evt_handler_t) (ble_led_c_t * p_ble_led_c, ble_led_c_evt_t * p_evt);

// LED客户端结构体
struct ble_led_c_s
{
    uint16_t                conn_handle; 
    led_db_t                peer_led_db; 
    ble_led_c_evt_handler_t evt_handler; 
    uint8_t                 uuid_type;
};

/**@brief LED Button Client initialization structure. */
typedef struct
{
    ble_led_c_evt_handler_t evt_handler;  /**< Event handler to be called by the LED Button Client module whenever there is an event related to the LED Button Service. */
} ble_led_c_init_t;

//******************************************************************************
// fn :ble_led_c_init
//
// brief : LED服务客户端初始化函数
//
// param : p_ble_led_c -> 指向LED客户端结构的指针
//         p_ble_led_c_init -> 指向LED初始化结构的指针
//
// return : none
uint32_t ble_led_c_init(ble_led_c_t * p_ble_led_c, ble_led_c_init_t * p_ble_led_c_init);

//******************************************************************************
// fn :ble_led_c_on_ble_evt
//
// brief : BLE事件处理函数
//
// param : p_ble_evt -> ble事件
//         p_context -> ble事件处理程序的参数（暂时理解应该是不同的功能，注册时所携带的结构体参数）
//
// return : none
void ble_led_c_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);

//******************************************************************************
// fn :ble_led_c_on_db_disc_evt
//
// brief : 处理led服务发现的函数
//
// param : p_ble_led_c -> 指向LED客户端结构的指针
//         p_evt -> 指向从数据库发现模块接收到的事件的指针
//
// return : none
void ble_led_c_on_db_disc_evt(ble_led_c_t * p_ble_led_c, const ble_db_discovery_evt_t * p_evt);

//******************************************************************************
// fn :ble_led_c_handles_assign
//
// brief : 用于为此led_c实例分配一个句柄
//
// param : p_ble_led_c -> 指向要关联的LED结构实例的指针
//         conn_handle -> 与给定的LED实例关联的连接句柄
//         p_peer_handles -> 在同级上找到的LED服务句柄（来自@ref BLE_LED_C_EVT_DISCOVERY_COMPLETE事件）
//
// return : none
uint32_t ble_led_c_handles_assign(ble_led_c_t *    p_ble_led_c,
                                  uint16_t         conn_handle,
                                  const led_db_t * p_peer_handles);

//******************************************************************************
// fn :ble_led_led_status_send
//
// brief : LED状态控制函数
//
// param : p_ble_led_c -> 指向要关联的LED结构实例的指针
//         p_string -> 发送的LED相关的数据
//         length -> 发送的LED相关的数据长度
//
// return : none
uint32_t ble_led_status_send(ble_led_c_t * p_ble_led_c, uint8_t * p_string, uint16_t length);

//******************************************************************************
// fn :ble_led_status_read
//
// brief : 读取LED特征值
//
// param : p_ble_led_c -> 指向要关联的LED结构实例的指针
//
// return : none
uint32_t ble_led_status_read(ble_led_c_t * p_ble_led_c);


#ifdef __cplusplus
}
#endif

#endif // GY_PROFILE_LED_C_H__

/** @} */
