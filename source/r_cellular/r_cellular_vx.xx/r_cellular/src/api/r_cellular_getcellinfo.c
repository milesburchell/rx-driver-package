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
 * File Name    : r_cellular_getcellinfo.c
 * Description  : Function to get information on the serving and neighbor cells.
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Includes   <System Includes> , "Project Includes"
 *********************************************************************************************************************/
#include "cellular_private_api.h"
#include "cellular_freertos.h"
#include "at_command.h"

/**********************************************************************************************************************
 * Macro definitions
 *********************************************************************************************************************/
#define CELLULAR_GET_INFO_LIMIT     (100)

/**********************************************************************************************************************
 * Typedef definitions
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported global variables
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Private (static) variables and functions
 *********************************************************************************************************************/

/************************************************************************
 * Function Name  @fn            R_CELLULAR_GetCellInfo
 ***********************************************************************/
e_cellular_err_t R_CELLULAR_GetCellInfo(st_cellular_ctrl_t * const p_ctrl, const e_cellular_info_type_t type,
                                            void(* const p_callback)(void * p_args))
{
    e_cellular_err_t ret = CELLULAR_SUCCESS;
    e_cellular_err_semaphore_t semaphore_ret = CELLULAR_SEMAPHORE_SUCCESS;
    uint8_t count = 0;

    if ((NULL == p_ctrl) || (NULL == p_callback) ||
            ((CELLULAR_INFO_TYPE0 != type) && (CELLULAR_INFO_TYPE1 != type) &&
                    (CELLULAR_INFO_TYPE2 != type) && (CELLULAR_INFO_TYPE7 != type) && (CELLULAR_INFO_TYPE9 != type)))
    {
        ret = CELLULAR_ERR_PARAMETER;
    }
    else
    {
        if (CELLULAR_SYSTEM_CLOSE == p_ctrl->system_state)
        {
            ret = CELLULAR_ERR_NOT_OPEN;
        }
    }

    if (CELLULAR_SUCCESS == ret)
    {
        semaphore_ret = cellular_take_semaphore(p_ctrl->at_semaphore);
        if (CELLULAR_SEMAPHORE_SUCCESS == semaphore_ret)
        {
            ret = atc_cfun_check(p_ctrl);

            if (CELLULAR_SUCCESS == ret)
            {
                if (CELLULAR_MODULE_OPERATING_LEVEL1 != p_ctrl->module_status)
                {
                    ret = atc_cfun(p_ctrl, CELLULAR_MODULE_OPERATING_LEVEL1);
                }
                if (CELLULAR_SUCCESS == ret)
                {
                    p_ctrl->recv_data = (void *) &type; //(&e_cellular_info_type_t) -> (void *)
                    p_ctrl->callback.sqnmoni_callback = p_callback;
                    do
                    {
                        ret = atc_sqnmoni(p_ctrl, type);
                        count++;
                    } while ((count < CELLULAR_GET_INFO_LIMIT) && (CELLULAR_SUCCESS != ret));
                    p_ctrl->recv_data = NULL;
                    p_ctrl->callback.sqnmoni_callback = NULL;
                }
            }
            cellular_give_semaphore(p_ctrl->at_semaphore);
        }
        else
        {
            ret = CELLULAR_ERR_OTHER_ATCOMMAND_RUNNING;
        }
    }

    return ret;
}
/**********************************************************************************************************************
 * End of function R_CELLULAR_GetCellInfo
 *********************************************************************************************************************/
