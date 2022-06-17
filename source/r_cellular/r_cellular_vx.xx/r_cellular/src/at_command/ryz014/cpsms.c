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
 * File Name    : cpsms.c
 * Description  : Function to execute the AT command (CPSMS).
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Includes   <System Includes> , "Project Includes"
 *********************************************************************************************************************/
#include "at_command.h"
#include "cellular_private_api.h"

/**********************************************************************************************************************
 * Macro definitions
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Typedef definitions
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported global variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private (static) variables and functions
 *********************************************************************************************************************/

/*************************************************************************************************
 * Function Name  @fn            atc_cpsms
 ************************************************************************************************/
e_cellular_err_t atc_cpsms(st_cellular_ctrl_t * const p_ctrl, const e_cellular_psm_mode_t mode,
                            const e_cellular_tau_cycle_t tau,
                            const e_cellular_cycle_multiplier_t tau_multiplier,
                            const e_cellular_active_cycle_t active,
                            const e_cellular_cycle_multiplier_t active_multiplier)
{
    e_cellular_err_t ret = CELLULAR_SUCCESS;
    e_cellular_err_atc_t at_ret = CELLULAR_ATC_OK;
    uint8_t str[3][9] = {0};

    uint8_t value;

    str[0][0] = mode + 0x30;

    value = tau;
    str[1][0] = ((value & 0x04) >> 2) + 0x30;
    str[1][1] = ((value & 0x02) >> 1) + 0x30;
    str[1][2] = (value & 0x01) + 0x30;

    value = tau_multiplier;
    str[1][3] = ((value & 0x10) >> 4) + 0x30;
    str[1][4] = ((value & 0x08) >> 3) + 0x30;
    str[1][5] = ((value & 0x04) >> 2) + 0x30;
    str[1][6] = ((value & 0x02) >> 1) + 0x30;
    str[1][7] = (value & 0x01) + 0x30;

    value = active;
    str[2][0] = ((value & 0x04) >> 2) + 0x30;
    str[2][1] = ((value & 0x02) >> 1) + 0x30;
    str[2][2] = (value & 0x01) + 0x30;

    value = active_multiplier;
    str[2][3] = ((value & 0x10) >> 4) + 0x30;
    str[2][4] = ((value & 0x08) >> 3) + 0x30;
    str[2][5] = ((value & 0x04) >> 2) + 0x30;
    str[2][6] = ((value & 0x02) >> 1) + 0x30;
    str[2][7] = (value & 0x01) + 0x30;

    const uint8_t * const p_command_arg[CELLULAR_MAX_ARG_COUNT] = {str[0], str[1], str[2]};

    atc_generate(p_ctrl->sci_ctrl.atc_buff,
        (const uint8_t *)&gp_at_command[ATC_SET_PSM][0],    // (const uint8_t *const *)->(const uint8_t **)
            (const uint8_t **)&p_command_arg);              // (const uint8_t *const *)->(const uint8_t **)

    at_ret = cellular_execute_at_command(p_ctrl, p_ctrl->sci_ctrl.atc_timeout, ATC_RETURN_OK, ATC_SET_PSM);

    if (CELLULAR_ATC_OK != at_ret)
    {
        ret = CELLULAR_ERR_MODULE_COM;
    }

    return ret;
}
/**********************************************************************************************************************
 * End of function atc_cpsms
 *********************************************************************************************************************/

/*************************************************************************************************
 * Function Name  @fn            atc_cpsms_check
 ************************************************************************************************/
e_cellular_err_t atc_cpsms_check(st_cellular_ctrl_t * const p_ctrl)
{
    e_cellular_err_t ret = CELLULAR_SUCCESS;
    e_cellular_err_atc_t at_ret = CELLULAR_ATC_OK;

    atc_generate(p_ctrl->sci_ctrl.atc_buff,
        (const uint8_t *)&gp_at_command[ATC_GET_PSM][0], // (const uint8_t *const *)->(const uint8_t **)
            NULL);

    at_ret = cellular_execute_at_command(p_ctrl, p_ctrl->sci_ctrl.atc_timeout,
                                            ATC_RETURN_OK, ATC_GET_PSM);

    if (CELLULAR_ATC_OK != at_ret)
    {
        ret = CELLULAR_ERR_MODULE_COM;
    }

    return ret;
}
/**********************************************************************************************************************
 * End of function atc_cpsms_check
 *********************************************************************************************************************/
