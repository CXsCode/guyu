
//******************************************************************
// 头文件定义
//
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "nordic_common.h"
#include "app_error.h"
#include "app_uart.h"
#include "ble_db_discovery.h"
#include "app_timer.h"
#include "app_util.h"
#include "bsp_btn_ble.h"
#include "ble.h"
#include "ble_gap.h"
#include "ble_hci.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "ble_nus_c.h"
#include "nrf_ble_gatt.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_ble_scan.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "gy_common.h"

#include "gy_profile_led_c.h"
#include "gy_serial_btn.h"
#include "boards.h"
#include "bsp.h"

 
#define BUTTON_DETECTION_DELAY          APP_TIMER_TICKS(50)    
//******************************************************************
// 常量定义
//
#define APP_BLE_CONN_CFG_TAG    1   // 
#define APP_BLE_OBSERVER_PRIO   3   // BLE应用程序的观察者优先级,无需修改此值。

NRF_BLE_SCAN_DEF(m_scan);           // 定义扫描实例的名称
NRF_BLE_GATT_DEF(m_gatt);           // GATT实例

BLE_LED_C_DEF(m_ble_led_c);         // LED实例 
BLE_DB_DISCOVERY_DEF(m_db_disc);    // 数据库发现模块实例
// 定义连接的handle
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;  

// 定义扫描限制的UUID
static ble_uuid_t const m_nus_uuid[NRF_BLE_SCAN_UUID_CNT] = {BLE_UUID_NUS_SERVICE, BLE_UUID_TYPE_BLE};

//static ble_uuid_t const m_nus_uuid[NRF_BLE_SCAN_UUID_CNT] = {{BLE_UUID_NUS_SERVICE, BLE_UUID_TYPE_BLE},\
//                                                            {0x0002, BLE_UUID_TYPE_BLE}};

// 定义扫描参数
static ble_gap_scan_params_t m_scan_params = 
{
    .active        = 1,                            // 1为主动扫描，可获得广播数据及扫描回调数据
    .interval      = NRF_BLE_SCAN_SCAN_INTERVAL,   // 扫描间隔：160*0.625 = 100ms  
    .window        = NRF_BLE_SCAN_SCAN_WINDOW,     // 扫描窗口：80*0.625 = 50ms   
    .timeout       = NRF_BLE_SCAN_SCAN_DURATION,   // 扫描持续的时间：设置为0，一直扫描，直到明确的停止扫描
    .filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL,    // 扫描白名单设备 BLE_GAP_SCAN_FP_WHITELIST BLE_GAP_SCAN_FP_ACCEPT_ALL
    .scan_phys     = BLE_GAP_PHY_1MBPS,            // 扫描1MBPS的PHY
};

// 定义连接参数
static ble_gap_conn_params_t m_conn_params = 
{
    .min_conn_interval  = MSEC_TO_UNITS(NRF_BLE_SCAN_MIN_CONNECTION_INTERVAL, UNIT_1_25_MS),  // 最小连接间隔7.5ms
    .max_conn_interval  = MSEC_TO_UNITS(NRF_BLE_SCAN_MAX_CONNECTION_INTERVAL, UNIT_1_25_MS),  // 最大连接间隔30ms  
    .slave_latency      = NRF_BLE_SCAN_SLAVE_LATENCY,                                         // 从机延迟0 
    .conn_sup_timeout   = MSEC_TO_UNITS(NRF_BLE_SCAN_SUPERVISION_TIMEOUT, UNIT_10_MS),        // 超时时间4000ms 
};
//******************************************************************
// fn : assert_nrf_callback
//
// brief : 在SoftDevice中用于处理asserts的函数
// details : 如果SoftDevice中存在assert，则调用此函数
// warning : 这个处理程序只是一个例子，不适用于最终产品。 您需要分析产品在assert时应该如何反应
// warning : 如果assert来自SoftDevice，则系统只能在复位时恢复
//
// param : line_num     失败的assert的行号
//         p_file_name  识别的assert的文件名
//
// return : none
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}

static void timer_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
}
//******************************************************************
// fn : scan_start
//
// brief : 开始扫描
//
// param : none
//
// return : none
static void scan_start(void)
{
    ret_code_t ret;

    ret = nrf_ble_scan_start(&m_scan);
    APP_ERROR_CHECK(ret);
}

//******************************************************************
// fn : scan_evt_handler
//
// brief : 处理扫描回调事件
//
// param : scan_evt_t  扫描事件结构体
//
// return : none
static void scan_evt_handler(scan_evt_t const * p_scan_evt)
{
    uint32_t err_code;
    ble_gap_addr_t peer_addr;
    ble_gap_addr_t const * p_peer_addr;
    switch(p_scan_evt->scan_evt_id)
    {
        // 白名单设置请求
        case NRF_BLE_SCAN_EVT_WHITELIST_REQUEST:
        {
          memset(&peer_addr, 0x00, sizeof(peer_addr));
          peer_addr.addr_id_peer = 1;
          peer_addr.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
          peer_addr.addr[5] = 0xD0;
          peer_addr.addr[4] = 0xD3;
          peer_addr.addr[3] = 0x07;
          peer_addr.addr[2] = 0xCE;
          peer_addr.addr[1] = 0xDF;
          peer_addr.addr[0] = 0xF5;
          p_peer_addr = &peer_addr; 
          
          // 设置白名单
          err_code = sd_ble_gap_whitelist_set(&p_peer_addr, 0x01);
          if (err_code == NRF_SUCCESS)
          {
            NRF_LOG_INFO("Successfully set whitelist!");
          }
          APP_ERROR_CHECK(err_code);
        }break;
        
        // 扫描到的白名单设备数据
        case NRF_BLE_SCAN_EVT_WHITELIST_ADV_REPORT:
        {
          // 判断是否为扫描回调数据
          if(p_scan_evt->params.p_whitelist_adv_report->type.scan_response)
          {
            if(p_scan_evt->params.p_whitelist_adv_report->data.len)    // 存在扫描回调数据
            {
              NRF_LOG_INFO("scan data:  %s",
                            Util_convertHex2Str(
                            p_scan_evt->params.p_whitelist_adv_report->data.p_data,
                            p_scan_evt->params.p_whitelist_adv_report->data.len));
            }
            else
            {
              NRF_LOG_INFO("scan data:  %s","NONE");
            }
            NRF_LOG_INFO("rssi:  %ddBm",p_scan_evt->params.p_whitelist_adv_report->rssi);
          }
          else  // 否则为广播数据
          {
            // 打印扫描的设备MAC
            NRF_LOG_INFO("Device MAC:  %s",
                         Util_convertBdAddr2Str((uint8_t*)p_scan_evt->params.p_whitelist_adv_report->peer_addr.addr));
            
            if(p_scan_evt->params.p_whitelist_adv_report->data.len)    // 存在广播数据
            {
              NRF_LOG_INFO("adv data:  %s",
                            Util_convertHex2Str(
                            p_scan_evt->params.p_whitelist_adv_report->data.p_data,
                            p_scan_evt->params.p_whitelist_adv_report->data.len));
            }
            else
            {
              NRF_LOG_INFO("adv data:  %s","NONE");
            }
          }
          //// 如果扫描到的设备信号强度大于-30dBm
          //if(p_scan_evt->params.p_whitelist_adv_report->rssi > (-40))
          //{
          //  ret_code_t          err_code;
            
          //  // 配置准备连接的设备MAC
          //  ble_gap_addr_t m_addr;
          //  m_addr.addr_id_peer = 1;
          //  m_addr.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
          //  memcpy(m_addr.addr,p_scan_evt->params.p_whitelist_adv_report->peer_addr.addr,BLE_GAP_ADDR_LEN);
            
          //  // 停止扫描
          //  nrf_ble_scan_stop();
          //  // 发起连接
          //  err_code = sd_ble_gap_connect(&m_addr,&m_scan_params,&m_conn_params,APP_BLE_CONN_CFG_TAG);
          //  APP_ERROR_CHECK(err_code);
          //}
        } break;
        case NRF_BLE_SCAN_EVT_NOT_FOUND:
        {
          NRF_LOG_INFO("@@@@@@@@@@@@-------------------------- EVENT ------------------------------");
          // 判断是否为扫描回调数据
          if(p_scan_evt->params.p_not_found->type.scan_response)
          {
            if(p_scan_evt->params.p_not_found->data.len)    // 存在扫描回调数据
            {
              NRF_LOG_INFO("@@@@@@@@@@@@ScanRsp Data:%s",
                            Util_convertHex2Str(
                            p_scan_evt->params.p_not_found->data.p_data,
                            p_scan_evt->params.p_not_found->data.len));
            }
            else
            {
              NRF_LOG_INFO("@@@@@@@@@@@@ScanRsp Data:%s","NONE");
            }
            
          }
          else  // 否则为广播数据
          {
            //打印设备RSSI信号
            NRF_LOG_INFO("@@@@@@@@@@@@Device RSSI: %ddBm",p_scan_evt->params.p_not_found->rssi);
            // 打印扫描的设备MAC
            NRF_LOG_INFO("@@@@@@@@@@@@Device MAC:  %s",
                         Util_convertBdAddr2Str((uint8_t*)p_scan_evt->params.p_not_found->peer_addr.addr));
            
            if(p_scan_evt->params.p_not_found->data.len)    // 存在广播数据
            {
              NRF_LOG_INFO("@@@@@@@@@@@@Adv Data:    %s",
                            Util_convertHex2Str(
                            p_scan_evt->params.p_not_found->data.p_data,
                            p_scan_evt->params.p_not_found->data.len));
            }
            else
            {
              NRF_LOG_INFO("@@@@@@@@@@@@Adv Data:  %s","NONE");
            }
          }
          //// 如果扫描到的设备信号强度大于-30dBm
          //if(p_scan_evt->params.p_not_found->rssi > (-30))
          //{
          //  ret_code_t          err_code;
            
          //  // 配置准备连接的设备MAC
          //  ble_gap_addr_t m_addr;
          //  m_addr.addr_id_peer = 1;
          //  m_addr.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
          //  memcpy(m_addr.addr,p_scan_evt->params.p_not_found->peer_addr.addr,BLE_GAP_ADDR_LEN);
            
          //  // 停止扫描
          //  nrf_ble_scan_stop();
          //  // 发起连接
          //  err_code = sd_ble_gap_connect(&m_addr,&m_scan_params,&m_conn_params,APP_BLE_CONN_CFG_TAG);
          //  APP_ERROR_CHECK(err_code);
          //}

        } break;
        case NRF_BLE_SCAN_EVT_FILTER_MATCH:
        {
          // 下面这一段我们只保留了扫描回调数据获取的部分，因为从机筛选广播的UUID在扫描回调数据
          // 判断是否为扫描回调数据
          if(p_scan_evt->params.filter_match.p_adv_report->type.scan_response)
          {
            NRF_LOG_INFO("Device MAC:  %s",
                         Util_convertBdAddr2Str((uint8_t*)p_scan_evt->params.filter_match.p_adv_report->peer_addr.addr));
            
            if(p_scan_evt->params.filter_match.p_adv_report->data.len)    // 存在扫描回调数据
            {
              NRF_LOG_INFO("ScanRsp Data:%s",
                            Util_convertHex2Str(
                            p_scan_evt->params.filter_match.p_adv_report->data.p_data,
                            p_scan_evt->params.filter_match.p_adv_report->data.len));
            }
            else
            {
              NRF_LOG_INFO("ScanRsp Data:%s","NONE");
            }
            NRF_LOG_INFO("Device RSSI: %ddBm",p_scan_evt->params.filter_match.p_adv_report->rssi);
          }
          //// 如果扫描到的设备信号强度大于-30dBm
          //if(p_scan_evt->params.filter_match.p_adv_report->rssi > (-30))
          //{
          //  ret_code_t          err_code;
            
          //  // 配置准备连接的设备MAC
          //  ble_gap_addr_t m_addr;
          //  m_addr.addr_id_peer = 1;
          //  m_addr.addr_type = BLE_GAP_ADDR_TYPE_RANDOM_STATIC;
          //  memcpy(m_addr.addr,p_scan_evt->params.filter_match.p_adv_report->peer_addr.addr,BLE_GAP_ADDR_LEN);
            
          //  // 停止扫描
          //  nrf_ble_scan_stop();
          //  // 发起连接
          //  err_code = sd_ble_gap_connect(&m_addr,&m_scan_params,&m_conn_params,APP_BLE_CONN_CFG_TAG);
          //  APP_ERROR_CHECK(err_code);
          //}
//          else  // 否则为广播数据
//          {
//            // 打印扫描的设备MAC
//            NRF_LOG_INFO("Device MAC:  %s",
//                         Util_convertBdAddr2Str((uint8_t*)p_scan_evt->params.filter_match.p_adv_report->peer_addr.addr));
//            
//            if(p_scan_evt->params.filter_match.p_adv_report->data.len)    // 存在广播数据
//            {
//              NRF_LOG_INFO("adv data:  %s",
//                            Util_convertHex2Str(
//                            p_scan_evt->params.filter_match.p_adv_report->data.p_data,
//                            p_scan_evt->params.filter_match.p_adv_report->data.len));
//            }
//            else
//            {
//              NRF_LOG_INFO("adv data:  %s","NONE");
//            }
//          }
        } break;
        case NRF_BLE_SCAN_EVT_CONNECTED:
        {
            NRF_LOG_INFO("SCAN CONNECTED!"); 
            NRF_LOG_INFO("Connected. conn_DevAddr: %s\nConnected. conn_handle: 0x%04x\nConnected. conn_Param: %d,%d,%d,%d",
                         Util_convertBdAddr2Str((uint8_t*)p_scan_evt->params.connected.p_connected->peer_addr.addr),
                         p_scan_evt->params.connected.conn_handle,
                         p_scan_evt->params.connected.p_connected->conn_params.min_conn_interval,
                         p_scan_evt->params.connected.p_connected->conn_params.max_conn_interval,
                         p_scan_evt->params.connected.p_connected->conn_params.slave_latency,
                         p_scan_evt->params.connected.p_connected->conn_params.conn_sup_timeout
                         );
        }break;
        
        case NRF_BLE_SCAN_EVT_CONNECTING_ERROR:
        {
            NRF_LOG_INFO("SCAN CONNECTING ERROR!");
            NRF_LOG_INFO("Disconnected. reason: 0x%04x",
                         p_scan_evt->params.connecting_err.err_code);
        }break;
        default:
           break;
    }
}
//******************************************************************
// fn : scan_init
//
// brief : 初始化扫描（未设置扫描数据限制）
//
// param : none
//
// return : none
static void scan_init(void)
{
    ret_code_t          err_code;
    nrf_ble_scan_init_t init_scan;

    // 清空扫描结构体参数
    memset(&init_scan, 0, sizeof(init_scan));
    
    //init_scan.connect_if_match = 1;
    //init_scan.conn_cfg_tag = APP_BLE_CONN_CFG_TAG;

    // 配置扫描的参数
    init_scan.p_scan_param = &m_scan_params;
    
    // 配置连接的参数
    init_scan.p_conn_param = &m_conn_params;

    // 初始化扫描
    err_code = nrf_ble_scan_init(&m_scan, &init_scan, scan_evt_handler);
    APP_ERROR_CHECK(err_code);



    //// 设置扫描的UUID限制
    //err_code = nrf_ble_scan_filter_set(&m_scan, SCAN_UUID_FILTER, &m_nus_uuid);
    //APP_ERROR_CHECK(err_code);

    //// 使能扫描的UUID限制
    //err_code = nrf_ble_scan_filters_enable(&m_scan, NRF_BLE_SCAN_UUID_FILTER, false);
    //APP_ERROR_CHECK(err_code);
}

//******************************************************************
// fn : ble_evt_handler
//
// brief : BLE事件回调
// details : 包含以下几种事件类型：COMMON、GAP、GATT Client、GATT Server、L2CAP
//
// param : ble_evt_t  事件类型
//         p_context  未使用
//
// return : none
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_gap_evt_t const * p_gap_evt = &p_ble_evt->evt.gap_evt;
    ble_gap_evt_connected_t const * p_connected_evt = &p_gap_evt->params.connected;
    switch (p_ble_evt->header.evt_id)
    {
        // 连接
        case BLE_GAP_EVT_CONNECTED:
            NRF_LOG_INFO("Connected. conn_DevAddr: %s\nConnected. conn_handle: 0x%04x\nConnected. conn_Param: %d,%d,%d,%d",
                         Util_convertBdAddr2Str((uint8_t*)p_connected_evt->peer_addr.addr),
                         p_gap_evt->conn_handle,
                         p_connected_evt->conn_params.min_conn_interval,
                         p_connected_evt->conn_params.max_conn_interval,
                         p_connected_evt->conn_params.slave_latency,
                         p_connected_evt->conn_params.conn_sup_timeout
                         );
            m_conn_handle = p_gap_evt->conn_handle;
            break;
        // 断开连接
        case BLE_GAP_EVT_DISCONNECTED:
            NRF_LOG_INFO("Disconnected. conn_handle: 0x%x, reason: 0x%04x",
                         p_gap_evt->conn_handle,
                         p_gap_evt->params.disconnected.reason);
            m_conn_handle = BLE_CONN_HANDLE_INVALID;
            // 如果需要异常断开重连，可以打开下面的注释
            // scan_start();  // 重新开始扫描
            break;
        // 连接参数更新请求
        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
            NRF_LOG_INFO("conn_Param Update  Request");
            
            sd_ble_gap_conn_param_update(p_gap_evt->conn_handle,
                                                    &p_gap_evt->params.conn_param_update_request.conn_params);
            break;
            
        // 连接参数更新
        case BLE_GAP_EVT_CONN_PARAM_UPDATE:
            NRF_LOG_INFO("conn_Param Update: %d,%d,%d,%d",
                         p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params.min_conn_interval,
                         p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params.max_conn_interval,
                         p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params.slave_latency,
                         p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params.conn_sup_timeout
                         );
            break;
        default:
            break;
    }
}

//******************************************************************
// fn : ble_stack_init
//
// brief : 用于初始化BLE协议栈
// details : 初始化SoftDevice、BLE事件中断
//
// param : none
//
// return : none
static void ble_stack_init(void)
{
    ret_code_t err_code;

    // SD使能请求，配置时钟，配置错误回调，中断（中断优先级栈堆默认设置）
    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    // SD默认配置（如下），SD RAM起始地址配置（0x200029e0）
    // 作为从机时的最大连接数量0
    // 作为主机时的最大连接数据1（本工程是主机）
    // 初始化MTU大小23
    // 供应商特定的UUID数量1
    // 属性表大小248（必须是4的倍数，以字节为单位）
    // 配置服务更改特性数量0
    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
    APP_ERROR_CHECK(err_code);

    // 使能BLE栈堆
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    // 注册BLE事件的处理程序，所有BLE的事件都将分派ble_evt_handler回调
    NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
}

//******************************************************************
// fn : log_init
//
// brief : 初始化log打印
//
// param : none
//
// return : none
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}

//******************************************************************
// fn : power_management_init
//
// brief : 初始化电源管理
//
// param : none
//
// return : none
static void power_management_init(void)
{
    ret_code_t err_code;
    err_code = nrf_pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

//******************************************************************
// fn : idle_state_handle
//
// brief : 处理空闲状态的功能（用于主循环）
// details : 处理任何挂起的日志操作，然后休眠直到下一个事件发生
//
// param : none
//
// return : none
static void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {
        nrf_pwr_mgmt_run();
    }
}

//******************************************************************
// fn : gatt_evt_handler
//
// brief : GATT事件回调
//
// param : p_gatt  gatt类型结构体
//         p_evt  gatt事件
//
// return : none
void gatt_evt_handler(nrf_ble_gatt_t * p_gatt, nrf_ble_gatt_evt_t const * p_evt)
{
    if ((m_conn_handle == p_evt->conn_handle) && (p_evt->evt_id == NRF_BLE_GATT_EVT_ATT_MTU_UPDATED))
    {
        NRF_LOG_INFO("Data len is set to 0x%X(%d)", 
                     p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH, 
                     p_evt->params.att_mtu_effective - OPCODE_LENGTH - HANDLE_LENGTH);
    }
}


//******************************************************************
// fn : gatt_init
//
// brief : 初始化GATT
//
// param : none
//
// return : none
void gatt_init(void)
{
    ret_code_t err_code;

    err_code = nrf_ble_gatt_init(&m_gatt, gatt_evt_handler);
    APP_ERROR_CHECK(err_code);
    
    err_code = nrf_ble_gatt_att_mtu_central_set(&m_gatt, 100);
    APP_ERROR_CHECK(err_code);
}










static void button_event_handler(bsp_event_t evt)
{
    ret_code_t err_code;
    uint8_t buf[LED_UUID_CHAR_LEN] = {0x01,0x01,0x01,0x01};
    switch (evt)
    {
    case BSP_EVENT_KEY_0:
      buf[0] = 0x00;
      break;
    case BSP_EVENT_KEY_1:
      buf[1] = 0x00;
      break;
    case BSP_EVENT_KEY_2:
      buf[2] = 0x00;
      break;
    case BSP_EVENT_KEY_3:
      buf[3] = 0x00;
      //注意，这里测试，当用BLE_GATT_OP_WRITE_REQ方式发送数据，调用ble_led_status_read读特征值将无效。
      //需要等待WRITE_RSP事件后，在发读特征值请求，此时才能成功读取到特征值数据。
      ble_led_status_read(&m_ble_led_c);
    default:
      break;
    }
    NRF_LOG_INFO("gattc write data.");
    ble_led_status_send(&m_ble_led_c,buf,LED_UUID_CHAR_LEN);    // 发送Wirte属性数据包
  
    //立即读特征值，注意，当上述写特征值函数是BLE_GATT_OP_WRITE_REQ时，这里立即读将无效。
    NRF_LOG_INFO("gattc send read data request.");
    ble_led_status_read(&m_ble_led_c);                          // 发送Read属性的读取消息
}



//******************************************************************
// fn : db_disc_handler
//
// brief : 用于处理数据库发现事件的函数
// details : 此函数是一个回调函数，用于处理来自数据库发现模块的事件。
//           根据发现的UUID，此功能将事件转发到各自的服务。
// 
// param : p_event -> 指向数据库发现事件的指针
//
// return : none
static void db_disc_handler(ble_db_discovery_evt_t * p_evt)
{
    ble_led_c_on_db_disc_evt(&m_ble_led_c, p_evt);
}

//******************************************************************
// fn : db_discovery_init
//
// brief : 用于初始化数据库发现模块的函数
//
// param : none
//
// return : none
static void db_discovery_init(void)
{
    //初始化发现服务组件，并提供处理服务的处理函数，当发现服务完成后，立即调用该处理函数。
    ret_code_t err_code = ble_db_discovery_init(db_disc_handler);
    APP_ERROR_CHECK(err_code);
}

//******************************************************************
// fn : ble_led_c_evt_handler
//
// brief : LED服务事件
//
// param : none
//
// return : none                 
static void ble_led_c_evt_handler(ble_led_c_t * p_ble_led_c, ble_led_c_evt_t * p_evt)
{
    ret_code_t err_code;

    switch (p_evt->evt_type)
    {
        case BLE_LED_C_EVT_DISCOVERY_COMPLETE:
            //服务发现已完成事件，除了保存相关参数外，还可以执行其他动作，例如立即读取或者发送从机数据等。这便是该事件独立出来的作用。
            NRF_LOG_INFO("Discovery complete.");
            
            err_code = ble_led_c_handles_assign(&m_ble_led_c, p_evt->conn_handle, &p_evt->params.peer_db);
            APP_ERROR_CHECK(err_code);
            
            //可以开始执行其他动作了。
            NRF_LOG_INFO("Connected to device with Ghostyu LED Service.");
            
            
            break;
        default:
            break;
    }
}

//******************************************************************
// fn : led_c_init
//
// brief : 初始化LED服务客户端（Ghostyu LED Service client）
//
// param : none
//
// return : none
static void led_c_init(void)
{
    //ret_code_t       err_code;
    //ble_led_c_init_t init;

    ////本例子初始化仅需提供一个从机服务获取完成后的事件处理函数。
    //init.evt_handler = ble_led_c_evt_handler;

    ////函数参数是指针的形式，因此携带的参数要使用&符号，取变量地址。
    //err_code = ble_led_c_init(&m_ble_led_c, &init);
    //APP_ERROR_CHECK(err_code);
}
void bsp_configuration()
{
    uint32_t err_code;

    err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, button_event_handler);
    APP_ERROR_CHECK(err_code);
}
//******************************************************************
// fn : main
//
// brief : 主函数
//
// param : none
//
// return : none
int main(void)
{
    // 初始化
    log_init();             // 初始化LOG打印，由RTT工作
    timer_init();
    //设置RTT终端显示的字体颜色为绿色，控制命令是 "\033[32m"
    NRF_LOG_ERROR("\033[32m Text Color: Green\r\n");
    // 打印例程名称
    NRF_LOG_INFO("1.2_ble_central_scan_all is running...");
    
    bsp_configuration();

    power_management_init();// 初始化电源控制
    ble_stack_init();       // 初始化BLE栈堆

    NRF_LOG_INFO("scan init");
    scan_init();            // 初始化扫描
    
    
    gatt_init();

    //db_discovery_init();

    
    
    //led_c_init();



    NRF_LOG_INFO("scan start");
    scan_start();           // 开始扫描
    
    // 进入主循环
    for (;;)
    {
        idle_state_handle();   // 空闲状态处理
    }
}