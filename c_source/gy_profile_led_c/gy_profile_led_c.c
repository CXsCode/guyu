#include "sdk_common.h"

#include "gy_profile_led_c.h"
#include "ble_db_discovery.h"
#include "ble_types.h"
#include "ble_srv_common.h"
#include "ble_gattc.h"
#define NRF_LOG_MODULE_NAME gy_profile_led_c
#include "nrf_log.h"
NRF_LOG_MODULE_REGISTER();


//******************************************************************************
// fn :on_disconnected
//
// brief : 处理disconnect事件的函数。
//
// param : p_ble_led_c -> led服务结构体
//         p_ble_evt -> ble事件
//
// return : none
static void on_disconnected(ble_led_c_t * p_ble_led_c, ble_evt_t const * p_ble_evt)
{
    if (p_ble_led_c->conn_handle == p_ble_evt->evt.gap_evt.conn_handle)
    {
        p_ble_led_c->conn_handle                    = BLE_CONN_HANDLE_INVALID;
        p_ble_led_c->peer_led_db.led_handle         = BLE_GATT_HANDLE_INVALID;
    }
}

//******************************************************************************
// fn :on_read
//
// brief : 处理read事件的函数。
//
// param : p_ble_led_c -> led服务结构体
//         p_ble_evt -> ble事件
//
// return : none
static void on_read(ble_led_c_t * p_ble_led_c, ble_evt_t const * p_ble_evt)
{
    if (p_ble_led_c->conn_handle == p_ble_evt->evt.gap_evt.conn_handle)
    {
      NRF_LOG_INFO("Recive Data:%02X,%02X,%02X,%02X",
                   p_ble_evt->evt.gattc_evt.params.read_rsp.data[0],
                   p_ble_evt->evt.gattc_evt.params.read_rsp.data[1],
                   p_ble_evt->evt.gattc_evt.params.read_rsp.data[2],
                   p_ble_evt->evt.gattc_evt.params.read_rsp.data[3]);
    }
}

//******************************************************************************
// fn :ble_led_c_on_db_disc_evt
//
// brief : 处理led服务发现的函数
//
// param : p_ble_led_c -> 指向LED客户端结构的指针
//         p_evt -> 指向从数据库发现模块接收到的事件的指针
//
// return : none
void ble_led_c_on_db_disc_evt(ble_led_c_t * p_ble_led_c, ble_db_discovery_evt_t const * p_evt)
{
    // 判断LED服务是否发现完成
    if (p_evt->evt_type == BLE_DB_DISCOVERY_COMPLETE &&
        p_evt->params.discovered_db.srv_uuid.uuid == LED_UUID_SERVICE &&
        p_evt->params.discovered_db.srv_uuid.type == p_ble_led_c->uuid_type)
    {
        ble_led_c_evt_t evt;

        evt.evt_type    = BLE_LED_C_EVT_DISCOVERY_COMPLETE;
        evt.conn_handle = p_evt->conn_handle;

        for (uint32_t i = 0; i < p_evt->params.discovered_db.char_count; i++)
        {
            const ble_gatt_db_char_t * p_char = &(p_evt->params.discovered_db.charateristics[i]);
            switch (p_char->characteristic.uuid.uuid)
            {
                // 根据LED特征值的UUID，获取我们句柄handle_value
                case LED_UUID_CHAR:
                    evt.params.peer_db.led_handle = p_char->characteristic.handle_value;
                    break;

                default:
                    break;
            }
        }

        NRF_LOG_DEBUG("Led Button Service discovered at peer.");
        
        // 如果实例是在db_discovery之前分配的，则分配db_handles
        if (p_ble_led_c->conn_handle != BLE_CONN_HANDLE_INVALID)
        {
            if (p_ble_led_c->peer_led_db.led_handle         == BLE_GATT_HANDLE_INVALID)
            {
                p_ble_led_c->peer_led_db = evt.params.peer_db;
            }
        }

        p_ble_led_c->evt_handler(p_ble_led_c, &evt);
    }
}

//******************************************************************************
// fn :ble_led_c_init
//
// brief : LED服务客户端初始化函数
//
// param : p_ble_led_c -> 指向LED客户端结构的指针
//         p_ble_led_c_init -> 指向LED初始化结构的指针
//
// return : none
uint32_t ble_led_c_init(ble_led_c_t * p_ble_led_c, ble_led_c_init_t * p_ble_led_c_init)
{
    uint32_t      err_code;
    ble_uuid_t    led_uuid;
    ble_uuid128_t led_base_uuid = {LED_UUID_BASE};

    VERIFY_PARAM_NOT_NULL(p_ble_led_c);
    VERIFY_PARAM_NOT_NULL(p_ble_led_c_init);
    VERIFY_PARAM_NOT_NULL(p_ble_led_c_init->evt_handler);

    //设置默认值
    p_ble_led_c->peer_led_db.led_handle         = BLE_GATT_HANDLE_INVALID;
    p_ble_led_c->conn_handle                    = BLE_CONN_HANDLE_INVALID;
    //设置事件处理函数
    p_ble_led_c->evt_handler                    = p_ble_led_c_init->evt_handler;

    err_code = sd_ble_uuid_vs_add(&led_base_uuid, &p_ble_led_c->uuid_type);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }
    VERIFY_SUCCESS(err_code);

    led_uuid.type = p_ble_led_c->uuid_type;
    led_uuid.uuid = LED_UUID_SERVICE;

    return ble_db_discovery_evt_register(&led_uuid);
}

//******************************************************************************
// fn :ble_led_c_on_ble_evt
//
// brief : BLE事件处理函数
//
// param : p_ble_evt -> ble事件
//         p_context -> ble事件处理程序的参数（暂时理解应该是不同的功能，注册时所携带的结构体参数）
//
// return : none
void ble_led_c_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    if ((p_context == NULL) || (p_ble_evt == NULL))
    {
        return;
    }
    //注意当前源文件对应头文件中使用了NRF_SDH_BLE_OBSERVER()宏定义。
    //包含一个ble_evt_t定义的全局静态变量，这里取其地址。然后向下传递。
    ble_led_c_t * p_ble_led_c = (ble_led_c_t *)p_context;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_DISCONNECTED:
            //连接已断开事件
            on_disconnected(p_ble_led_c, p_ble_evt);
            break;
            
        case BLE_GATTC_EVT_READ_RSP:
            //读响应事件，在该事件下获取已读到的从机数据。
            NRF_LOG_INFO("rcv read rsp evt.");
            on_read(p_ble_led_c, p_ble_evt);
          break;
          
        case BLE_GATTC_EVT_WRITE_RSP:
            //对应BLE_GATT_OP_WRITE_REQ带回应写的响应事件
            //对写数据要求比较高的应用可以处理这个事件，确认数据已写入成功。
            NRF_LOG_INFO("rcv write rsp evt.");
          break;
          
        default:
            break;
    }
}

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
uint32_t ble_led_status_send(ble_led_c_t * p_ble_led_c, uint8_t * p_string, uint16_t length)
{
    VERIFY_PARAM_NOT_NULL(p_ble_led_c);

    //相关参数验证
    if (length > LED_UUID_CHAR_LEN)
    {
        NRF_LOG_WARNING("Content too long.");
        return NRF_ERROR_INVALID_PARAM;
    }
    if (p_ble_led_c->conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    // 定义ble_gattc_write_params_t参数
    // write_op取值:
    //   1. BLE_GATT_OP_WRITE_REQ 带回应(收到write rsp事件)，确认对方已收到.
    //   2. BLE_GATT_OP_WRITE_CMD 不带回应，发送速度更快。
    // flags取值相对固定，初学者可以略过。
    // handle取值: 发现从机服务时保存的特征值句柄(通过特特征值uuid查找)
    // len 和 p_value 待发送数据长度和数据内容
    // 其他参数取值相对固定，暂不考虑。
    ble_gattc_write_params_t const write_params =
    {
        .write_op = BLE_GATT_OP_WRITE_REQ,
        .flags    = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE,
        .handle   = p_ble_led_c->peer_led_db.led_handle,
        .offset   = 0,
        .len      = length,
        .p_value  = p_string
    };
    
    //调用api函数发送数据，conn_handle和write_params
    return sd_ble_gattc_write(p_ble_led_c->conn_handle, &write_params);
}

//******************************************************************************
// fn :ble_led_status_read
//
// brief : 读取LED特征值
//
// param : p_ble_led_c -> 指向要关联的LED结构实例的指针
//
// return : none
uint32_t ble_led_status_read(ble_led_c_t * p_ble_led_c)
{
    VERIFY_PARAM_NOT_NULL(p_ble_led_c);
    //最后一个参数为offset，取值0，暂不考虑该参数。
    return sd_ble_gattc_read(p_ble_led_c->conn_handle,p_ble_led_c->peer_led_db.led_handle,0);
}

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
uint32_t ble_led_c_handles_assign(ble_led_c_t    * p_ble_led_c,
                                  uint16_t         conn_handle,
                                  const led_db_t * p_peer_handles)
{
    VERIFY_PARAM_NOT_NULL(p_ble_led_c);

    //保存连接handle
    p_ble_led_c->conn_handle = conn_handle;
    
    //保存获取到的特征值句柄
    if (p_peer_handles != NULL)
    {
        p_ble_led_c->peer_led_db = *p_peer_handles;
    }
    return NRF_SUCCESS;
}

