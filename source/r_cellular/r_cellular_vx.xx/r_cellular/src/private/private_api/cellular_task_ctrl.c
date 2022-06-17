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
 * File Name    : cellular_task_ctrl.c
 * Description  : Functions to manage tasks.
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Includes   <System Includes> , "Project Includes"
 *********************************************************************************************************************/
#include "cellular_private_api.h"
#include "cellular_freertos.h"
#include "cellular_receive_task.h"

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

/****************************************************************************
 * Function Name  @fn            cellular_start_recv_task
 ***************************************************************************/
e_cellular_err_t cellular_start_recv_task(st_cellular_ctrl_t * const p_ctrl)
{
    e_cellular_err_t ret = CELLULAR_SUCCESS;

    p_ctrl->eventgroup = cellular_create_event_group("task_event");
    if (NULL == p_ctrl->eventgroup)
    {
        ret =  CELLULAR_ERR_EVENT_GROUP_INIT;
    }
    else
    {
#if BSP_CFG_RTOS_USED == (1)
        ret = cellular_create_task(cellular_recv_task,
                                    "cellular_recv_task",
                                    2048,
                                    (void *)p_ctrl, //(st_cellular_ctrl_t *)->(void *)
                                    CELLULAR_IDLE_PRIORITY,
                                    &p_ctrl->recv_taskhandle);
#elif BSP_CFG_RTOS_USED == (5)
        p_ctrl->recv_taskhandle = cellular_create_task(cellular_recv_task,
                                                        "cellular_recv_task",
                                                        2048,
                                                        (void *)p_ctrl, //(st_cellular_ctrl_t *)->(void *)
                                                        CELLULAR_IDLE_PRIORITY,
                                                        &p_ctrl->recv_taskhandle);
        if (NULL == p_ctrl->recv_taskhandle)
        {
            ret = CELLULAR_ERR_CREATE_TASK;
        }
#endif
    }

    return ret;
}
/**********************************************************************************************************************
 End of function cellular_start_recv_task
 *********************************************************************************************************************/
