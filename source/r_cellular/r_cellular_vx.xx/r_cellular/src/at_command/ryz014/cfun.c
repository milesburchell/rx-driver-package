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
 * File Name    : cfun.c
 * Description  : Function to execute the AT command (CFUN).
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Includes   <System Includes> , "Project Includes"
 *********************************************************************************************************************/
#include "at_command.h"
#include "cellular_private_api.h"
#include "cellular_freertos.h"

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
 * Function Name  @fn            atc_cfun
 ************************************************************************************************/
e_cellular_err_t atc_cfun(st_cellular_ctrl_t * const p_ctrl, const e_cellular_module_status_t mode)
{
    e_cellular_err_t ret = CELLULAR_SUCCESS;
    uint8_t str[10] = {0};

    sprintf((char *)str, "%d", mode); // (uint8_t *)->(char *)

    const uint8_t * const p_command_arg[CELLULAR_MAX_ARG_COUNT] = {str};

    atc_generate(p_ctrl->sci_ctrl.atc_buff,
        (const uint8_t *)&gp_at_command[ATC_FUNCTION_LEVEL][0], // (const uint8_t *const *)->(const uint8_t **)
            (const uint8_t **)&p_command_arg);                  // (const uint8_t *const *)->(const uint8_t **)

    ret = cellular_execute_at_command(p_ctrl, p_ctrl->sci_ctrl.atc_timeout, ATC_RETURN_OK, ATC_FUNCTION_LEVEL);

    cellular_delay_task(CELLULAR_CFUN_WAIT);

    return ret;
}
/**********************************************************************************************************************
 * End of function atc_cfun
 *********************************************************************************************************************/

/*************************************************************************************************
 * Function Name  @fn            atc_cfun_check
 ************************************************************************************************/
e_cellular_err_t atc_cfun_check(st_cellular_ctrl_t * const p_ctrl)
{
    e_cellular_err_t ret = CELLULAR_SUCCESS;

    atc_generate(p_ctrl->sci_ctrl.atc_buff,
        (const uint8_t *)&gp_at_command[ATC_FUNCTION_LEVEL_CHECK][0], // (const uint8_t *const *)->(const uint8_t **)
            NULL);

    ret = cellular_execute_at_command(p_ctrl, p_ctrl->sci_ctrl.atc_timeout, ATC_RETURN_OK, ATC_FUNCTION_LEVEL_CHECK);

    return ret;
}
/**********************************************************************************************************************
 * End of function atc_cfun_check
 *********************************************************************************************************************/
