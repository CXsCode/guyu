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
// brief : ����disconnect�¼��ĺ�����
//
// param : p_ble_led_c -> led����ṹ��
//         p_ble_evt -> ble�¼�
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
// brief : ����read�¼��ĺ�����
//
// param : p_ble_led_c -> led����ṹ��
//         p_ble_evt -> ble�¼�
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
// brief : ����led�����ֵĺ���
//
// param : p_ble_led_c -> ָ��LED�ͻ��˽ṹ��ָ��
//         p_evt -> ָ������ݿⷢ��ģ����յ����¼���ָ��
//
// return : none
void ble_led_c_on_db_disc_evt(ble_led_c_t * p_ble_led_c, ble_db_discovery_evt_t const * p_evt)
{
    // �ж�LED�����Ƿ������
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
                // ����LED����ֵ��UUID����ȡ���Ǿ��handle_value
                case LED_UUID_CHAR:
                    evt.params.peer_db.led_handle = p_char->characteristic.handle_value;
                    break;

                default:
                    break;
            }
        }

        NRF_LOG_DEBUG("Led Button Service discovered at peer.");
        
        // ���ʵ������db_discovery֮ǰ����ģ������db_handles
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
// brief : LED����ͻ��˳�ʼ������
//
// param : p_ble_led_c -> ָ��LED�ͻ��˽ṹ��ָ��
//         p_ble_led_c_init -> ָ��LED��ʼ���ṹ��ָ��
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

    //����Ĭ��ֵ
    p_ble_led_c->peer_led_db.led_handle         = BLE_GATT_HANDLE_INVALID;
    p_ble_led_c->conn_handle                    = BLE_CONN_HANDLE_INVALID;
    //�����¼�������
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
// brief : BLE�¼�������
//
// param : p_ble_evt -> ble�¼�
//         p_context -> ble�¼��������Ĳ�������ʱ���Ӧ���ǲ�ͬ�Ĺ��ܣ�ע��ʱ��Я���Ľṹ�������
//
// return : none
void ble_led_c_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    if ((p_context == NULL) || (p_ble_evt == NULL))
    {
        return;
    }
    //ע�⵱ǰԴ�ļ���Ӧͷ�ļ���ʹ����NRF_SDH_BLE_OBSERVER()�궨�塣
    //����һ��ble_evt_t�����ȫ�־�̬����������ȡ���ַ��Ȼ�����´��ݡ�
    ble_led_c_t * p_ble_led_c = (ble_led_c_t *)p_context;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_DISCONNECTED:
            //�����ѶϿ��¼�
            on_disconnected(p_ble_led_c, p_ble_evt);
            break;
            
        case BLE_GATTC_EVT_READ_RSP:
            //����Ӧ�¼����ڸ��¼��»�ȡ�Ѷ����Ĵӻ����ݡ�
            NRF_LOG_INFO("rcv read rsp evt.");
            on_read(p_ble_led_c, p_ble_evt);
          break;
          
        case BLE_GATTC_EVT_WRITE_RSP:
            //��ӦBLE_GATT_OP_WRITE_REQ����Ӧд����Ӧ�¼�
            //��д����Ҫ��Ƚϸߵ�Ӧ�ÿ��Դ�������¼���ȷ��������д��ɹ���
            NRF_LOG_INFO("rcv write rsp evt.");
          break;
          
        default:
            break;
    }
}

//******************************************************************************
// fn :ble_led_led_status_send
//
// brief : LED״̬���ƺ���
//
// param : p_ble_led_c -> ָ��Ҫ������LED�ṹʵ����ָ��
//         p_string -> ���͵�LED��ص�����
//         length -> ���͵�LED��ص����ݳ���
//
// return : none
uint32_t ble_led_status_send(ble_led_c_t * p_ble_led_c, uint8_t * p_string, uint16_t length)
{
    VERIFY_PARAM_NOT_NULL(p_ble_led_c);

    //��ز�����֤
    if (length > LED_UUID_CHAR_LEN)
    {
        NRF_LOG_WARNING("Content too long.");
        return NRF_ERROR_INVALID_PARAM;
    }
    if (p_ble_led_c->conn_handle == BLE_CONN_HANDLE_INVALID)
    {
        return NRF_ERROR_INVALID_STATE;
    }

    // ����ble_gattc_write_params_t����
    // write_opȡֵ:
    //   1. BLE_GATT_OP_WRITE_REQ ����Ӧ(�յ�write rsp�¼�)��ȷ�϶Է����յ�.
    //   2. BLE_GATT_OP_WRITE_CMD ������Ӧ�������ٶȸ��졣
    // flagsȡֵ��Թ̶�����ѧ�߿����Թ���
    // handleȡֵ: ���ִӻ�����ʱ���������ֵ���(ͨ��������ֵuuid����)
    // len �� p_value ���������ݳ��Ⱥ���������
    // ��������ȡֵ��Թ̶����ݲ����ǡ�
    ble_gattc_write_params_t const write_params =
    {
        .write_op = BLE_GATT_OP_WRITE_REQ,
        .flags    = BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE,
        .handle   = p_ble_led_c->peer_led_db.led_handle,
        .offset   = 0,
        .len      = length,
        .p_value  = p_string
    };
    
    //����api�����������ݣ�conn_handle��write_params
    return sd_ble_gattc_write(p_ble_led_c->conn_handle, &write_params);
}

//******************************************************************************
// fn :ble_led_status_read
//
// brief : ��ȡLED����ֵ
//
// param : p_ble_led_c -> ָ��Ҫ������LED�ṹʵ����ָ��
//
// return : none
uint32_t ble_led_status_read(ble_led_c_t * p_ble_led_c)
{
    VERIFY_PARAM_NOT_NULL(p_ble_led_c);
    //���һ������Ϊoffset��ȡֵ0���ݲ����Ǹò�����
    return sd_ble_gattc_read(p_ble_led_c->conn_handle,p_ble_led_c->peer_led_db.led_handle,0);
}

//******************************************************************************
// fn :ble_led_c_handles_assign
//
// brief : ����Ϊ��led_cʵ������һ�����
//
// param : p_ble_led_c -> ָ��Ҫ������LED�ṹʵ����ָ��
//         conn_handle -> �������LEDʵ�����������Ӿ��
//         p_peer_handles -> ��ͬ�����ҵ���LED������������@ref BLE_LED_C_EVT_DISCOVERY_COMPLETE�¼���
//
// return : none
uint32_t ble_led_c_handles_assign(ble_led_c_t    * p_ble_led_c,
                                  uint16_t         conn_handle,
                                  const led_db_t * p_peer_handles)
{
    VERIFY_PARAM_NOT_NULL(p_ble_led_c);

    //��������handle
    p_ble_led_c->conn_handle = conn_handle;
    
    //�����ȡ��������ֵ���
    if (p_peer_handles != NULL)
    {
        p_ble_led_c->peer_led_db = *p_peer_handles;
    }
    return NRF_SUCCESS;
}

