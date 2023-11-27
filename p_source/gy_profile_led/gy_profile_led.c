#include "sdk_common.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "ble_srv_common.h"
#include "gy_profile_led.h"

//******************************************************************************
// fn :on_write
//
// brief : 处理Write事件的函数。该事件来自主机(GATTC)的Write写特征值
//
// param : p_led -> led服务结构体
//         p_ble_evt -> ble事件
//
// return : none
static void on_write(ble_led_t * p_led, ble_evt_t const * p_ble_evt)
{
    ble_gatts_evt_write_t const * p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    //参数判断和筛选
    if (   (p_evt_write->handle == p_led->led_char_handles.value_handle)
        && (p_evt_write->len <= LED_UUID_CHAR_LEN)
        && (p_led->led_write_handler != NULL))
    {
        //调用mainc中service_init函数设置的处理函数。并传递从无线端接收到的数据。
        p_led->led_write_handler((uint8_t*)p_evt_write->data);
    }
}

//******************************************************************************
// fn :ble_led_on_ble_evt
//
// brief : BLE事件处理函数
//
// param : p_ble_evt -> ble事件
//         p_context -> ble事件处理程序的参数（暂时理解应该是不同的功能，注册时所携带的结构体参数）
//
// return : none
void ble_led_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_led_t * p_led = (ble_led_t *)p_context;

    switch (p_ble_evt->header.evt_id)
    {
        // GATTClient的Write事件，本从机里对应为GATTS(Server端)
        case BLE_GATTS_EVT_WRITE:
            on_write(p_led, p_ble_evt);
            NRF_LOG_INFO("BLE_GATTS_EVT_WRITE");
            break;

        default:
            NRF_LOG_INFO("other:%d",p_ble_evt->header.evt_id);
            break;
    }
}

//******************************************************************************
// fn :ble_led_init
//
// brief : 初始化LED服务
//
// param : p_led -> led服务结构体
//         p_led_init -> led服务初始化结构体
//
// return : uint32_t -> 成功返回SUCCESS，其他返回ERR NO.
uint32_t ble_led_init(ble_led_t * p_led, const ble_led_init_t * p_led_init)
{
    uint32_t              err_code;
    ble_uuid_t            ble_uuid;
    ble_add_char_params_t add_char_params;

    // 初始化服务结构体
    p_led->led_write_handler = p_led_init->led_write_handler;
    
    // 添加服务（128bit UUID），uuid_type由此判断是128位
    ble_uuid128_t base_uuid = {LED_UUID_BASE};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_led->uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = p_led->uuid_type;  //type value = 2(BLE_UUID_TYPE_VENDOR_BEGIN), is 128bit uuid;  value = 1(BLE_UUID_TYPE_BLE), is 16bit uuid
    ble_uuid.uuid = LED_UUID_SERVICE;
    
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_led->service_handle);
    VERIFY_SUCCESS(err_code);

    // 添加LED特征值（属性是Write和Read、长度是4）
    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid             = LED_UUID_CHAR;
    add_char_params.uuid_type        = p_led->uuid_type;
    add_char_params.init_len         = LED_UUID_CHAR_LEN;
    add_char_params.max_len          = LED_UUID_CHAR_LEN;
    add_char_params.p_init_value     = p_led_init->p_led_value;
    add_char_params.char_props.read  = 1;
    add_char_params.char_props.write = 1;

    add_char_params.read_access  = SEC_OPEN;
    add_char_params.write_access = SEC_OPEN;

    return characteristic_add(p_led->service_handle, &add_char_params, &p_led->led_char_handles);
}


