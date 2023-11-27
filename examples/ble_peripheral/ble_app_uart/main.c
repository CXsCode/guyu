
//******************************************************************
// 头文件定义
//
#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "ble_hci.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "app_timer.h"
#include "ble_nus.h"
#include "app_uart.h"
#include "app_util_platform.h"
#include "bsp_btn_ble.h"
#include "nrf_pwr_mgmt.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "gy_common.h"

#include "gy_profile_led.h"
#include "gy_serial_led.h"
//******************************************************************
// 常量定义
//
#define APP_BLE_CONN_CFG_TAG    1   // 
#define APP_BLE_OBSERVER_PRIO   3   // BLE应用程序的观察者优先级,无需修改此值。

BLE_ADVERTISING_DEF(m_advertising); // 广播实例

NRF_BLE_GATT_DEF(m_gatt);           // GATT实例

BLE_LED_DEF(m_led);                 // LED实例
NRF_BLE_QWR_DEF(m_qwr);             // 队列实例

// 定义连接的handle
static uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;  

#define DEVICE_NAME  "GY-NRF52832"
// 定义广播的UUID
static ble_uuid_t m_adv_uuids[] = {{BLE_UUID_NUS_SERVICE, BLE_UUID_TYPE_BLE}};

// 定义连接参数（为了展示连接参数的更新，我们设置连接间隔为固定的20ms）
static ble_gap_conn_params_t m_conn_params = 
{
    .min_conn_interval  = MSEC_TO_UNITS(40, UNIT_1_25_MS),  // 最小连接间隔40ms
    .max_conn_interval  = MSEC_TO_UNITS(40, UNIT_1_25_MS),  // 最大连接间隔40ms  
    .slave_latency      = 0,                                // 隐藏周期0 
    .conn_sup_timeout   = MSEC_TO_UNITS(2000, UNIT_10_MS),  // 超时时间4000ms 
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
    
void adv_evt_callback(ble_adv_evt_t evt)
{
    NRF_LOG_INFO("advertising event:%d",evt);
}
    
//******************************************************************
// fn : advertising_start
//
// brief : 用于开启广播
//
// param : none
//
// return : none
static void advertising_start(void)
{
    uint32_t err_code = ble_advertising_start(&m_advertising, BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
}

//******************************************************************
// fn : advertising_init
//
// brief : 用于初始化广播
//
// param : none
//
// return : none

/*
广播原始数据如下
static uint8 advData[] =
{
  0x02,                  //A长度字段，取值 0x02
  BLE_GAP_AD_TYPE_FLAGS, //A类型字段，取值 0x01
  BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE, //A数据字段，取值 0x06，等于 0x04 + 0x02

  0x0C,                  //B长度字段，取值 0x0C (十进制=12)
  BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME, //B类型字段，取值 0x09
  'G','Y','-','N','R','F','5','2','8','3','2'  //B数据字段 取值 GY-NRF52832 字符串
};

扫描回应原始数据
static uint8 scanRspData[] =
{
  0x08,                  //A长度字段，取值 0x08
  BLE_GAP_AD_TYPE_LE_BLUETOOTH_DEVICE_ADDRESS, //A类型字段，取值 0x1B
  0xB5,0xD1,0xCE,0xA6,0xA8,0xCA,0x01           //A数据字段，取值 B5D1CEA6A8CA01
};
*/

static void advertising_init(void)
{
    uint32_t               err_code;
    ble_advertising_init_t init;

    memset(&init, 0, sizeof(init));

    ////为了掩饰广播的状态变化，这里设置一个广播事件回调函数。
    //init.evt_handler = adv_evt_callback;
    
    // 广播数据包含所有设备名称（FULL NAME）
    init.advdata.name_type = BLE_ADVDATA_FULL_NAME;

    // 扫描回调数据中包含16bit UUID：0x0001
    init.srdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    init.srdata.uuids_complete.p_uuids  = m_adv_uuids;

    // 扫描回调数据中包含设备MAC地址
    init.srdata.include_ble_device_addr = true;
    
#if 0
    //设置受限广播模式，广播超时后停止广播
    init.advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_LIMITED_DISC_MODE;
      
    // 配置广播周期，先快速广播18s（周期40ms），再慢速广播18s（周期100ms），最后停止广播
    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = 64;       // 64*0.625 = 40ms
    init.config.ble_adv_fast_timeout  = 1800;     // 1800*10ms = 18s
    init.config.ble_adv_slow_enabled  = true;
    init.config.ble_adv_slow_interval = 160;      // 160*0.625 = 100ms
    init.config.ble_adv_slow_timeout  = 1800;     // 1800*10ms = 18s
#else
    //设置一般广播模式，持续广播，永不超时
    init.advdata.flags = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    
    init.config.ble_adv_fast_enabled  = true;
    init.config.ble_adv_fast_interval = 64;       // 160*0.625 = 100ms
    init.config.ble_adv_fast_timeout  = 0;         // 0表示不超时
#endif
    
    err_code = ble_advertising_init(&m_advertising, &init);
    APP_ERROR_CHECK(err_code);

    ble_advertising_conn_cfg_tag_set(&m_advertising, APP_BLE_CONN_CFG_TAG);
}

//******************************************************************
// fn : gap_params_init
//
// brief : 初始化GAP
// details : 此功能将设置设备的所有必需的GAP（通用访问配置文件）参数。它还设置权限和外观。
//
// param : none
//
// return : none
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
    NRF_LOG_INFO("set device name: %s",DEVICE_NAME);
    // 设置设备名称
    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *) DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);
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
            // No implementation needed.
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

    // SD默认配置（如下），SD RAM起始地址配置（0x20002a98）
    // 作为从机时的最大连接数量1（本工程为从机）
    // 作为主机时的最大连接数据0
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

static void mac_printf(void)
{
  int i = 0;
  ble_gap_addr_t bleAddr;
  sd_ble_gap_addr_get(&bleAddr);
  for(i=0;i<6;i++)
  {
    NRF_LOG_INFO("addr:%x ",bleAddr.addr[i]);
  }
}

//******************************************************************
// fn : on_conn_params_evt
//
// brief : 处理连接更新参数的事件回调
//
// param : p_evt -> 接收到的连接模块返回的任务事件
//
// return : none
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt)
{
    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_SUCCEEDED)
    {
        NRF_LOG_INFO("connParam Update Success"); 
    }
      
    if (p_evt->evt_type == BLE_CONN_PARAMS_EVT_FAILED)
    {
        NRF_LOG_INFO("connParam Update Failed"); 
    }
}

//******************************************************************
// fn : conn_params_error_handler
//
// brief : 处理连接更新参数异常的事件回调
//
// param : nrf_error -> 异常标志
//
// return : none
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}
//******************************************************************
// fn : conn_params_init
//
// brief : 初始化连接参数模块的功能
//
// param : none
//
// return : none
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = &m_conn_params;
    cp_init.first_conn_params_update_delay = APP_TIMER_TICKS(5000);
    cp_init.next_conn_params_update_delay  = APP_TIMER_TICKS(5000);
    cp_init.max_conn_params_update_count   = 3;
    cp_init.start_on_notify_cccd_handle    = BLE_GATT_HANDLE_INVALID;
    cp_init.disconnect_on_fail             = false;
    cp_init.evt_handler                    = on_conn_params_evt;
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}
//******************************************************************
// fn : timers_init
//
// brief : 初始化定时器功能
//
// param : none
//
// return : none
static void timers_init(void)
{
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);
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

    err_code = nrf_ble_gatt_att_mtu_periph_set(&m_gatt, 80);
    APP_ERROR_CHECK(err_code);
}
//******************************************************************
// fn : led_write_handler
//
// brief : 处理主机(GATTC)Write事件的处理函数，用于接收数据。
// details : 
//
// param : new_state 四字节灯控数据。单字节0x00表示点亮LED，0x01表示关闭LED，(led为共阳驱动)
//
// return : none
static void led_write_handler(uint8_t * new_state)
{
    NRF_LOG_INFO("Recive State:%02X,%02X,%02X,%02X",new_state[0],new_state[1],new_state[2],new_state[3]);
    nrf_gpio_pin_write(BSP_LED_0, new_state[0]);
    nrf_gpio_pin_write(BSP_LED_1, new_state[1]);
    nrf_gpio_pin_write(BSP_LED_2, new_state[2]);
    nrf_gpio_pin_write(BSP_LED_3, new_state[3]);
}
//******************************************************************
// fn : services_init
//
// brief : 初始化复位（本例程展示NUS：Nordic Uart Service）
//
// param : none
//
// return : none
static void services_init(void)
{
    uint32_t           err_code;
    ble_led_init_t     led_init;
    //led状态默认值
    uint8_t led_value[LED_UUID_CHAR_LEN] = {1,1,1,1};

    //清除赋值
    memset(&led_init, 0, sizeof(led_init));

    //设置特征值初始取值。
    led_init.p_led_value = led_value;
    //设置处理主机(GATTC)Write事件的处理函数，用于接收数据。
    led_init.led_write_handler = led_write_handler;
    //函数参数是指针的形式，因此携带的参数要使用&符号，取变量地址。
    err_code = ble_led_init(&m_led, &led_init);
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
    timers_init();
    LED_Init();
    //设置RTT终端显示的字体颜色为绿色，控制命令是 "\033[32m"
    NRF_LOG_ERROR("\033[32m Text Color: Green\r\n");
    // 打印例程名称
    NRF_LOG_INFO("2.2_ble_peripheral_adv_all example running....");
    
    power_management_init();// 初始化电源控制
    ble_stack_init();       // 初始化BLE栈堆
    gap_params_init();      // 初始化GAP

    gatt_init();
    
    services_init();
    NRF_LOG_INFO("advertising init");
    advertising_init();     // 初始化广播
    NRF_LOG_INFO("conn_params_init");
    conn_params_init();
    NRF_LOG_INFO("advertising start");
    advertising_start();    // 开启广播
    
    // 进入主循环
    for (;;)
    {
        idle_state_handle();  // 空闲状态处理
    }
}
