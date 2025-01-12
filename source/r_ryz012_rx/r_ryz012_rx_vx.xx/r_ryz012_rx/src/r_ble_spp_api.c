/**********************************************************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
 * other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
 * EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
 * SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO
 * THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
 * this software. By using this software, you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *
 * Copyright (C) 2022 Renesas Electronics Corporation. All rights reserved.
 *********************************************************************************************************************/
/**********************************************************************************************************************
 * File Name    : r_ble_spp_api.c
 * Description  :
 *********************************************************************************************************************/
/**********************************************************************************************************************
 Includes   <System Includes> , "Project Includes"
 *********************************************************************************************************************/
#include "r_ble_api.h"
#include "r_ble_spp.h"
#include "rm_ble_abs.h"
#include "rm_ble_abs_api.h"

/***********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define BLE_MODULE_RESET_TIMEOUT            (50)
#define BLE_MODULE_STABILIZE_TIMEOUT        (500)
#define BLE_MODULE_COMMAND_RECV_TIMEOUT     (1000)
#define BLE_MODULE_MTU_MAX                  (128)

#define BLE_PARAMETER_NOT_USED(p)           (void) ((p))

#define ENDIAN_LSB16(n)                     (uint8_t) (n & 0xFF)
#define ENDIAN_MSB16(n)                     (uint8_t) (n >> 8)

#define R_BLE_SPP_EVENT_CONN_IND            (0x0783)
#define R_BLE_SPP_EVENT_DISCONN_IND         (0x0784)
#define R_BLE_SPP_EVENT_SET_DISABLE_ADV     (0x0721)
#define R_BLE_SPP_EVENT_SET_ENABLE_ADV      (0x070A)
#define R_BLE_SPP_EVENT_SET_ADV_SRES        (0x0722)
#define R_BLE_SPP_EVENT_SET_ADV_PARAM       (0x0720)
#define R_BLE_SPP_EVENT_GET_ATTR            (0x072A)
#define R_BLE_SPP_EVENT_SET_ATTR            (0x0729)
#define R_BLE_SPP_EVENT_DATA_RECEIVED       (0x07A0)
#define R_BLE_SPP_EVENT_SET_MAC_ID          (0x0725)
#define R_BLE_SPP_EVENT_SEND_NOTIFY         (0x071C)
#define R_BLE_SPP_EVENT_SEND_INDICATION     (0x0726)
#define R_BLE_SPP_EVENT_SET_TX_POWER        (0x0705)

#define BLE_SPP_COMMAND(command, reply_event)                           \
    {                                                                   \
        uint16_t cmd_status = (uint16_t) (command);                     \
                                                                        \
        if (!cmd_status)                                                \
        {                                                               \
            cmd_status = r_ble_spp_api_check_return_valid(reply_event); \
        }                                                               \
                                                                        \
        if (cmd_status)                                                 \
        {                                                               \
            return BLE_ERR_INVALID_PTR;                                 \
        }                                                               \
    }

#if defined(__CCRX__) || defined(__ICCRX__) || defined(__RX__)
/* RYZ012 Reset port pin macros.  */
#define BLE_RESET_PDR(x, y)                     (BLE_RESET_PDR_PREPROC(x, y))
#define BLE_RESET_PDR_PREPROC(x, y)             ((PORT ## x .PDR.BIT.B ## y))
#define BLE_RESET_PODR(x, y)                    (BLE_RESET_PODR_PREPROC(x, y))
#define BLE_RESET_PODR_PREPROC(x, y)            ((PORT ## x .PODR.BIT.B ## y))

/* SCI Mode(Uart/Spi) pin macros.  */
#define BLE_SCI_MODE_PDR(x, y)                  (BLE_SCI_MODE_PDR_PREPROC(x, y))
#define BLE_SCI_MODE_PDR_PREPROC(x, y)          ((PORT ## x .PDR.BIT.B ## y))
#define BLE_SCI_MODE_PODR(x, y)                 (BLE_SCI_MODE_PODR_PREPROC(x, y))
#define BLE_SCI_MODE_PODR_PREPROC(x, y)         ((PORT ## x .PODR.BIT.B ## y))
#endif


/**********************************************************************************************************************
 Local Typedef definitions
 *********************************************************************************************************************/
typedef enum
{
    BLE_SPP_COMMS_UART,
    BLE_SPP_COMMS_SPI
} ble_abs_spp_comms_transport_mode_t;

/**********************************************************************************************************************
 Exported global variables
 *********************************************************************************************************************/
//extern ble_abs_instance_ctrl_t * gp_instance_ctrl;
void rm_ble_spp_callback(void * p_args);

volatile uint32_t g_spp_tx_data_empty = 0;
ble_gap_app_cb_t g_gap_cb   = NULL;
ble_event_cb_t   g_cb_event = NULL;
ble_gatt_client_application_callback_t g_spp_gatt_client_cb = NULL;
ble_gatt_server_application_callback_t g_spp_gatt_server_cb = NULL;

uint32_t ble_version_major   = 0;
uint32_t ble_version_minor   = 0;
uint32_t ble_version_special = 0;

/**********************************************************************************************************************
 Private (static) variables and functions
 *********************************************************************************************************************/
static void         r_ble_spp_api_set_ryz012_communication_mode (ble_abs_spp_comms_transport_mode_t mode);
static void         r_ble_spp_api_mw_callback (r_ble_spp_payload_t * p_payload);
static ble_status_t r_ble_spp_api_check_return_valid (uint32_t expected_spp_reply);

static r_ble_spp_payload_t g_current_spp_payload;
static r_ble_spp_payload_t g_current_spp_async_payload;
static volatile uint32_t   g_spp_cmd_ready   = 0;
static volatile uint32_t   g_spp_cmd_unknown = 0;
static volatile uint32_t   g_spp_cmd_failed  = 0;
static volatile uint32_t   g_spp_cmd_async   = 0;
static r_ble_spp_cfg_t     r_ble_spp_api_transport_api;
static bool                g_dynamic_profile_set;

/* LUT used in R_BLE_GAP_SetAdvParam */
static const uint8_t g_ch_map_lut[8] = {0, 0, 1, 0, 2, 0, 1, 3};


ble_status_t R_BLE_Open (void)
{
    r_ble_spp_api_transport_api.mw_callback = &r_ble_spp_api_mw_callback;

    r_ble_spp_api_set_ryz012_communication_mode(BLE_SPP_COMMS_UART);

    if (R_BLE_SPP_SUCCESS == R_BLE_SPP_GAP_Open(&r_ble_spp_api_transport_api))
    {
        FSP_ERROR_RETURN(BLE_SUCCESS, r_ble_spp_api_check_return_valid(R_BLE_SPP_EVENT_BLE_READY));
    }
    return BLE_SUCCESS;
}

ble_status_t R_BLE_Close (void)
{
    /* Disconnect any connection already established */
    R_BLE_SPP_GAP_Disconnect();

    /* Restart the device */
    R_BLE_SPP_GAP_Restart();

    /* Close the communication port */
    R_BLE_SPP_GAP_Close(&r_ble_spp_api_transport_api);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GAP_Init (ble_gap_app_cb_t gap_cb)
{
    if (NULL == gap_cb)
    {
        return BLE_ERR_INVALID_PTR;
    }

    g_gap_cb = gap_cb;
    g_gap_cb(BLE_GAP_EVENT_STACK_ON, BLE_SUCCESS, NULL);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GATTS_SetDbInst (st_ble_gatts_db_cfg_t * p_db_inst)
{
    BLE_PARAMETER_NOT_USED(p_db_inst);

    /* Only set this once (QE currently calls this twice, once in RM_BLE_ABS_Open and a second time by itself) */
    if (!g_dynamic_profile_set)
    {
        /* Get number of loop attributes */
        uint16_t attributes = qe_ble_profile[0].encapsulated_attributes;

        r_ble_spp_profile_params_t params;

        /* Add profile attributes to BLE module internal table. */
        for (uint32_t i = 0; i < attributes + 1U; i++)
        {
            params.attNum         = qe_ble_profile[i].encapsulated_attributes;
            params.attrLen        = qe_ble_profile[i].value_length;
            params.pAttrValue     = qe_ble_profile[i].value;
            params.payload_length = (uint16_t) (8 + qe_ble_profile[i].uuid_length + qe_ble_profile[i].value_length);
            params.perm           = qe_ble_profile[i].permissions;
            params.read_flag      = qe_ble_profile[i].notify_read;
            params.uuid           = qe_ble_profile[i].uuid;
            params.uuidLen        = qe_ble_profile[i].uuid_length;
            params.write_flag     = qe_ble_profile[i].notify_write;

            BLE_SPP_COMMAND(R_BLE_SPP_GAP_Config_Profile(&params), R_BLE_SPP_EVENT_DYNAMIC_ATTRIBUTE_CREATE);
        }

        /* Finalize the profile table on the device */
        BLE_SPP_COMMAND(R_BLE_SPP_GAP_Profile_Complete(), R_BLE_SPP_EVENT_DYNAMIC_ATTRIBUTE_COMPLETE);

        g_gap_cb(BLE_GAP_EVENT_DISCONN_IND, BLE_SUCCESS, NULL);

        g_dynamic_profile_set = true;
    }

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GAP_StopAdv (uint8_t adv_hdl)
{
    BLE_PARAMETER_NOT_USED(adv_hdl);

    BLE_SPP_COMMAND(R_BLE_SPP_GAP_StopAdv(), R_BLE_SPP_EVENT_SET_DISABLE_ADV);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GAP_SetAdvSresData (st_ble_gap_adv_data_t * p_adv_srsp_data)
{
    r_ble_spp_adv_data_t advertising_data;
    advertising_data.data_type   = R_BLE_SPP_ADV_DATA_MODE;
    advertising_data.data_length = p_adv_srsp_data->data_length;
    advertising_data.p_data      = p_adv_srsp_data->p_data;

    /* Set advertising data */
    BLE_SPP_COMMAND(R_BLE_SPP_GAP_SetAdvSresData(&advertising_data), R_BLE_SPP_EVENT_SET_ADV_SRES);

    advertising_data.data_type = R_BLE_SPP_SCAN_RSP_DATA_MODE;

    /* Set scan response data */
    BLE_SPP_COMMAND(R_BLE_SPP_GAP_SetAdvSresData(&advertising_data), R_BLE_SPP_EVENT_SET_ADV_SRES);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GAP_StartPerdAdv (uint8_t adv_hdl)
{
    BLE_PARAMETER_NOT_USED(adv_hdl);

    return BLE_ERR_UNSUPPORTED;
}

ble_status_t R_BLE_GAP_SetPerdAdvParam (st_ble_gap_perd_adv_param_t * p_perd_adv_param)
{
    BLE_PARAMETER_NOT_USED(p_perd_adv_param);

    return BLE_ERR_UNSUPPORTED;
}

ble_status_t R_BLE_GAP_StartAdv (uint8_t adv_hdl, uint16_t duration, uint8_t max_extd_adv_evts)
{
    BLE_PARAMETER_NOT_USED(adv_hdl);
    BLE_PARAMETER_NOT_USED(duration);
    BLE_PARAMETER_NOT_USED(max_extd_adv_evts);

    BLE_SPP_COMMAND(R_BLE_SPP_GAP_StartAdv(), R_BLE_SPP_EVENT_SET_ENABLE_ADV);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GAP_SetAdvParam (st_ble_gap_adv_param_t * p_adv_param)
{
    uint8_t chan_map;

    BLE_SPP_COMMAND(R_BLE_SPP_GAP_Init(), R_BLE_SPP_EVENT_BLE_INIT);

    /* Select lowest configured channel or all three */
    chan_map = g_ch_map_lut[p_adv_param->adv_ch_map];

    r_ble_spp_adv_param_t adv_param;
    adv_param.adv_ch_map    = chan_map;
    adv_param.filter_policy = (r_ble_spp_filter_policy_t)p_adv_param->filter_policy;
    adv_param.adv_phy       = (r_ble_spp_phy_type_t)0x01;
    adv_param.sec_adv_phy   = (r_ble_spp_phy_type_t)0x01;
    adv_param.adv_prop_type = (r_ble_spp_adv_prop_type_t)0x0000;
    adv_param.adv_intv_min  = p_adv_param->adv_intv_min;
    adv_param.adv_intv_max  = p_adv_param->adv_intv_max;

    BLE_SPP_COMMAND(R_BLE_SPP_GAP_SetAdvParam(&adv_param), R_BLE_SPP_EVENT_SET_ADV_PARAM);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GATTS_Init (uint8_t cb_num)
{
    BLE_PARAMETER_NOT_USED(cb_num);
    ble_status_t err;

    err = R_BLE_GATTS_SetDbInst(NULL);

    return err;
}

uint32_t R_BLE_IsTaskFree (void)
{
    return 0;
}

ble_status_t R_BLE_GATT_GetMtu (uint16_t conn_hdl, uint16_t * p_mtu)
{
    BLE_PARAMETER_NOT_USED(conn_hdl);

#if BLE_ABS_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(p_mtu);
#endif

    *p_mtu = BLE_MODULE_MTU_MAX;

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GATTS_GetAttr (uint16_t conn_hdl, uint16_t attr_hdl, st_ble_gatt_value_t * p_value)
{
    BLE_PARAMETER_NOT_USED(conn_hdl);

#if BLE_ABS_CFG_PARAM_CHECKING_ENABLE
    FSP_ASSERT(p_value);
#endif

    r_ble_spp_gatt_hdl_value_pair_t value;
    r_ble_spp_gatt_value_t          gatt_value;
    gatt_value.value_len = qe_ble_profile[attr_hdl].value_length;
    value.attr_hdl       = attr_hdl;
    value.value          = gatt_value;

    BLE_SPP_COMMAND(R_BLE_SPP_GATTS_GetAttr(&value), R_BLE_SPP_EVENT_GET_ATTR);

    p_value->p_value   = &g_current_spp_payload.out_data[1];
    p_value->value_len = gatt_value.value_len;

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GATTS_SetAttr (uint16_t conn_hdl, uint16_t attr_hdl, st_ble_gatt_value_t * p_value)
{
    BLE_PARAMETER_NOT_USED(conn_hdl);

    r_ble_spp_gatt_hdl_value_pair_t value;
    r_ble_spp_gatt_value_t          gatt_value;
    gatt_value.value_len = p_value->value_len;
    gatt_value.p_value   = p_value->p_value;
    value.attr_hdl       = attr_hdl;
    value.value          = gatt_value;

    BLE_SPP_COMMAND(R_BLE_SPP_GATTS_SetAttr(&value), R_BLE_SPP_EVENT_SET_ATTR);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GATTS_Notification (uint16_t conn_hdl, st_ble_gatt_hdl_value_pair_t * p_ntf_data)
{
    BLE_PARAMETER_NOT_USED(conn_hdl);

    r_ble_spp_gatt_hdl_value_pair_t value;
    r_ble_spp_gatt_value_t          gatt_value;
    gatt_value.value_len = p_ntf_data->value.value_len;
    gatt_value.p_value   = p_ntf_data->value.p_value;
    value.attr_hdl       = p_ntf_data->attr_hdl;
    value.value          = gatt_value;

    BLE_SPP_COMMAND(R_BLE_SPP_GATTS_Notification(&value), R_BLE_SPP_EVENT_SEND_NOTIFY);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GATTS_Indication (uint16_t conn_hdl, st_ble_gatt_hdl_value_pair_t * p_ind_data)
{
    BLE_PARAMETER_NOT_USED(conn_hdl);

    r_ble_spp_gatt_hdl_value_pair_t value;
    r_ble_spp_gatt_value_t          gatt_value;
    gatt_value.value_len = p_ind_data->value.value_len;
    gatt_value.p_value   = p_ind_data->value.p_value;
    value.attr_hdl       = p_ind_data->attr_hdl;
    value.value          = gatt_value;

    BLE_SPP_COMMAND(R_BLE_SPP_GATTS_Indication(&value), R_BLE_SPP_EVENT_SEND_INDICATION);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_VS_SetBdAddr (uint8_t area, st_ble_dev_addr_t * p_addr)
{
    BLE_PARAMETER_NOT_USED(area);
    ble_status_t status;

    if (R_BLE_SPP_GAP_SetMACID((r_ble_spp_peer_addrType_t) p_addr->type, (uint8_t *) p_addr->addr) == R_BLE_SPP_SUCCESS)
    {
        status = r_ble_spp_api_check_return_valid(R_BLE_SPP_EVENT_SET_MAC_ID);
        if (status)
        {
            return BLE_ERR_INVALID_PTR;
        }
    }

    return BLE_SUCCESS;
}

ble_status_t R_BLE_SetEvent (ble_event_cb_t cb)
{
    g_cb_event = cb;

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GAP_GetVerInfo (void)
{
    return BLE_SUCCESS;
}

ble_status_t R_BLE_GATTS_RegisterCb (ble_gatts_app_cb_t cb, uint8_t priority)
{
    BLE_PARAMETER_NOT_USED(priority);

    g_spp_gatt_server_cb = cb;

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GATTC_Init (uint8_t cb_num)
{
    BLE_PARAMETER_NOT_USED(cb_num);

    g_spp_gatt_client_cb = NULL;

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GATTC_RegisterCb (ble_gattc_app_cb_t cb, uint8_t priority)
{
    BLE_PARAMETER_NOT_USED(priority);

    g_spp_gatt_client_cb = cb;

    return BLE_SUCCESS;
}

ble_status_t R_BLE_VS_SetTxPower (uint16_t conn_hdl, uint8_t tx_power)
{
    BLE_PARAMETER_NOT_USED(conn_hdl);
    ble_status_t status;

    if (R_BLE_SPP_GAP_SetTxPower(tx_power) == R_BLE_SPP_SUCCESS)
    {
        status = r_ble_spp_api_check_return_valid(R_BLE_SPP_EVENT_SET_TX_POWER);
        if (status)
        {
            return BLE_ERR_INVALID_PTR;
        }
    }
    else
    {
        return BLE_ERR_INVALID_OPERATION;
    }

    return BLE_SUCCESS;
}

/*
 * Blow functions are not implemented
 */

ble_status_t R_BLE_VS_Init (ble_vs_app_cb_t vs_cb)
{
    BLE_PARAMETER_NOT_USED(vs_cb);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GAP_SetPairingParams (st_ble_gap_pairing_param_t * p_pair_param)
{
    BLE_PARAMETER_NOT_USED(p_pair_param);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GAP_SetLocIdInfo (st_ble_dev_addr_t * p_lc_id_addr, uint8_t * p_lc_irk)
{
    BLE_PARAMETER_NOT_USED(p_lc_id_addr);
    BLE_PARAMETER_NOT_USED(p_lc_irk);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GATTC_ReadChar (uint16_t conn_hdl, uint16_t value_hdl)
{
    BLE_PARAMETER_NOT_USED(conn_hdl);
    BLE_PARAMETER_NOT_USED(value_hdl);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GATTC_WriteChar (uint16_t conn_hdl, st_ble_gatt_hdl_value_pair_t * p_write_data)
{
    BLE_PARAMETER_NOT_USED(conn_hdl);
    BLE_PARAMETER_NOT_USED(p_write_data);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GATTC_WriteCharWithoutRsp (uint16_t conn_hdl, st_ble_gatt_hdl_value_pair_t * p_write_data)
{
    BLE_PARAMETER_NOT_USED(conn_hdl);
    BLE_PARAMETER_NOT_USED(p_write_data);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GATTC_ReadLongChar (uint16_t conn_hdl, uint16_t value_hdl, uint16_t offset)
{
    BLE_PARAMETER_NOT_USED(conn_hdl);
    BLE_PARAMETER_NOT_USED(value_hdl);
    BLE_PARAMETER_NOT_USED(offset);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GATTC_WriteLongChar (uint16_t conn_hdl, st_ble_gatt_hdl_value_pair_t * p_write_data, uint16_t offset)
{
    BLE_PARAMETER_NOT_USED(conn_hdl);
    BLE_PARAMETER_NOT_USED(p_write_data);
    BLE_PARAMETER_NOT_USED(offset);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GATTS_RspExMtu (uint16_t conn_hdl, uint16_t mtu)
{
    BLE_PARAMETER_NOT_USED(conn_hdl);
    BLE_PARAMETER_NOT_USED(mtu);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_VS_GetTxBufferNum (uint32_t * p_buffer_num)
{
    BLE_PARAMETER_NOT_USED(p_buffer_num);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_VS_StartTxFlowEvtNtf (void)
{
    return BLE_SUCCESS;
}

ble_status_t R_BLE_GATTC_DiscPrimServ (uint16_t conn_hdl, uint8_t * p_uuid, uint8_t uuid_type)
{
    BLE_PARAMETER_NOT_USED(conn_hdl);
    BLE_PARAMETER_NOT_USED(p_uuid);
    BLE_PARAMETER_NOT_USED(uuid_type);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GATTC_DiscIncServ (uint16_t conn_hdl, st_ble_gatt_hdl_range_t * p_range)
{
    BLE_PARAMETER_NOT_USED(conn_hdl);
    BLE_PARAMETER_NOT_USED(p_range);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GATTC_DiscAllChar (uint16_t conn_hdl, st_ble_gatt_hdl_range_t * p_range)
{
    BLE_PARAMETER_NOT_USED(conn_hdl);
    BLE_PARAMETER_NOT_USED(p_range);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GATTC_DiscAllCharDesc (uint16_t conn_hdl, st_ble_gatt_hdl_range_t * p_range)
{
    BLE_PARAMETER_NOT_USED(conn_hdl);
    BLE_PARAMETER_NOT_USED(p_range);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GATTS_SetPrepareQueue (st_ble_gatt_pre_queue_t * p_pre_queues, uint8_t queue_num)
{
    BLE_PARAMETER_NOT_USED(p_pre_queues);
    BLE_PARAMETER_NOT_USED(queue_num);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_GAP_UpdConn (uint16_t conn_hdl, uint8_t mode, uint16_t accept, st_ble_gap_conn_param_t * p_conn_updt_param)
{
    BLE_PARAMETER_NOT_USED(conn_hdl);
    BLE_PARAMETER_NOT_USED(mode);
    BLE_PARAMETER_NOT_USED(accept);
    BLE_PARAMETER_NOT_USED(p_conn_updt_param);

    return BLE_SUCCESS;
}

ble_status_t R_BLE_VS_GetBdAddr (uint8_t area, uint8_t addr_type)
{
    BLE_PARAMETER_NOT_USED(area);
    BLE_PARAMETER_NOT_USED(addr_type);

    return BLE_SUCCESS;
}

/******************************************************************************
 * Static Private Functions
 ******************************************************************************/

static void r_ble_spp_api_set_ryz012_communication_mode (ble_abs_spp_comms_transport_mode_t mode)
{
    BLE_RESET_PDR(BLE_CFG_RESET_PORT, BLE_CFG_RESET_PIN) = 1;
    BLE_SCI_MODE_PDR(BLE_CFG_SCI_MODE_PORT, BLE_CFG_SCI_MODE_PIN) = 1;

    /* Reset Low */
    BLE_RESET_PODR(BLE_CFG_RESET_PORT, BLE_CFG_RESET_PIN) = 0;

    R_BSP_SoftwareDelay(BLE_MODULE_RESET_TIMEOUT, BSP_DELAY_MILLISECS);

    if (BLE_SPP_COMMS_UART == mode)
    {
        /* Select is low as UART */
        BLE_SCI_MODE_PODR(BLE_CFG_SCI_MODE_PORT, BLE_CFG_SCI_MODE_PIN) = 0;
    }
    else
    {
        /* Select is High as SPI */
        BLE_SCI_MODE_PODR(BLE_CFG_SCI_MODE_PORT, BLE_CFG_SCI_MODE_PIN) = 1;
    }
    R_BSP_SoftwareDelay(BLE_MODULE_RESET_TIMEOUT, BSP_DELAY_MILLISECS);

    BLE_RESET_PODR(BLE_CFG_RESET_PORT, BLE_CFG_RESET_PIN) = 1;

    R_BSP_SoftwareDelay(BLE_MODULE_RESET_TIMEOUT, BSP_DELAY_MILLISECS);

    /* Dynamic profile can now be configured again */
    g_dynamic_profile_set = false;
}

ble_status_t R_BLE_Execute (void)
{
    st_ble_gatt_value_t value;
    ble_status_t        status   = BLE_SUCCESS;
    uint16_t            attr_hdl = 0;

    if (NULL != g_cb_event)
    {
        g_cb_event();
        g_cb_event = NULL;
    }

    if (g_spp_cmd_async)
    {
        switch (g_current_spp_async_payload.event_id)
        {
            case R_BLE_SPP_EVENT_CONN_IND:
            {
                g_gap_cb(BLE_GAP_EVENT_CONN_IND, BLE_SUCCESS, NULL);
                if (NULL != g_spp_gatt_server_cb)
                {
                    g_spp_gatt_server_cb(BLE_GATTC_EVENT_CONN_IND, BLE_SUCCESS, NULL);
                }

                if (NULL != g_spp_gatt_client_cb)
                {
                    g_spp_gatt_client_cb(BLE_GATTS_EVENT_CONN_IND, BLE_SUCCESS, NULL);
                }
                break;
            }
            case R_BLE_SPP_EVENT_DISCONN_IND:
            {
                g_gap_cb(BLE_GAP_EVENT_DISCONN_IND, BLE_SUCCESS, NULL);
                if (NULL != g_spp_gatt_server_cb)
                {
                    g_spp_gatt_server_cb(BLE_GATTC_EVENT_DISCONN_IND, BLE_SUCCESS, NULL);
                }

                if (NULL != g_spp_gatt_client_cb)
                {
                    g_spp_gatt_client_cb(BLE_GATTS_EVENT_DISCONN_IND, BLE_SUCCESS, NULL);
                }
                break;
            }
            case R_BLE_SPP_EVENT_DATA_RECEIVED:
            {
                attr_hdl        = *((uint16_t *) (&g_current_spp_async_payload.out_data[1]));
                value.value_len = (uint16_t) (g_current_spp_async_payload.out_len - 5);
                value.p_value   = &g_current_spp_async_payload.out_data[3];

                status = R_BLE_GATTS_SetAttr(0, attr_hdl, &value);

                if (NULL != g_spp_gatt_server_cb)
                {
                    st_ble_gatts_evt_data_t      event_write_data;
                    st_ble_gatts_write_rsp_evt_t event_write;
                    event_write.attr_hdl       = attr_hdl;
                    event_write_data.conn_hdl  = 0;
                    event_write_data.param_len = value.value_len;
                    event_write_data.p_param   = (void *) &event_write;
                    g_spp_gatt_server_cb(BLE_GATTS_EVENT_WRITE_RSP_COMP, BLE_SUCCESS, (void *) &event_write_data);
                }
                break;
            }
            case R_BLE_SPP_EVENT_SET_ENABLE_ADV:
            {
                g_gap_cb(BLE_GAP_EVENT_ADV_ON, BLE_SUCCESS, NULL);
                break;
            }
            case R_BLE_SPP_EVENT_SET_DISABLE_ADV:
            {
                g_gap_cb(BLE_GAP_EVENT_ADV_OFF, BLE_SUCCESS, NULL);
                break;
            }
        }
        g_spp_cmd_async = 0;
    }
    return status;
}

static void r_ble_spp_api_mw_callback (r_ble_spp_payload_t * p_payload)
{
    /* NOTE: Async event will be detected and sent to upper layer callback */
    switch (p_payload->event_id)
    {
        case R_BLE_SPP_EVENT_CONN_IND:
        case R_BLE_SPP_EVENT_DISCONN_IND:
        case R_BLE_SPP_EVENT_DATA_RECEIVED:
        {
            g_current_spp_async_payload = *p_payload;
            g_spp_cmd_async             = 1;
            break;
        }
        case R_BLE_SPP_EVENT_SET_ENABLE_ADV:
        case R_BLE_SPP_EVENT_SET_DISABLE_ADV:
        {
            g_current_spp_payload       = *p_payload;
            g_current_spp_async_payload = *p_payload;

            if ((p_payload->out_data[0] == 0) && (p_payload->out_len >= 1))
            {
                g_spp_cmd_ready = 1;
                g_spp_cmd_async = 1;
            }
            else
            {
                g_spp_cmd_failed = 1;
            }
            break;
        }
        case R_BLE_SPP_EVENT_BLE_READY:
        case R_BLE_SPP_EVENT_SET_MAC_ID:
        case R_BLE_SPP_EVENT_DYNAMIC_ATTRIBUTE_CREATE:
        case R_BLE_SPP_EVENT_DYNAMIC_ATTRIBUTE_COMPLETE:
        case R_BLE_SPP_EVENT_BLE_INIT:
        case R_BLE_SPP_EVENT_SET_ADV_SRES:
        case R_BLE_SPP_EVENT_GET_ATTR:
        case R_BLE_SPP_EVENT_SET_ATTR:
        case R_BLE_SPP_EVENT_SET_ADV_PARAM:
        case R_BLE_SPP_EVENT_SET_TX_POWER:
        {
            g_current_spp_payload = *p_payload;

            if ((p_payload->out_data[0] == 0) && (p_payload->out_len >= 1))
            {
                g_spp_cmd_ready = 1;
            }
            else
            {
                g_spp_cmd_failed = 1;
            }
            break;
        }
        default:
        {
            g_current_spp_payload = *p_payload;
            g_spp_cmd_unknown     = 1;
            break;
        }
    }
}

void rm_ble_spp_callback (void * pArgs)
{
    sci_cb_args_t * p_args = (sci_cb_args_t *) pArgs;
    if (SCI_EVT_RX_CHAR == p_args->event)
    {
        wrap_sci_ctrl(SCI_CMD_RX_Q_FLUSH, NULL);
        r_ble_spp_rx_char_callback((uint8_t) p_args->byte);
    }
    else if (SCI_EVT_TEI == p_args->event)
    {
        g_spp_tx_data_empty = 1;
    }
    else
    {
        ; /* Do nothing */
    }
}

static ble_status_t r_ble_spp_api_check_return_valid (uint32_t expected_spp_reply)
{
    uint32_t countdown = BLE_MODULE_RESET_TIMEOUT;

    while (countdown && !g_spp_tx_data_empty)
    {
        R_BSP_SoftwareDelay(1, BSP_DELAY_MILLISECS);
        countdown--;
    }

    while (!g_spp_cmd_ready && countdown && !g_spp_cmd_failed)
    {
        R_BSP_SoftwareDelay(1, BSP_DELAY_MILLISECS);
        countdown--;
    }

    if (g_spp_cmd_ready)
    {
        if (g_current_spp_payload.event_id != expected_spp_reply)
        {
            return BLE_ERR_HC_UNKNOWN_HCI_CMD;
        }
    }
    else if (g_spp_cmd_failed)
    {
        return BLE_ERR_HC_CMD_DISALLOWED;
    }
    else
    {
        return BLE_ERR_RSP_TIMEOUT;
    }

    g_spp_cmd_ready  = 0;
    g_spp_cmd_failed = 0;

    return BLE_SUCCESS;
}
