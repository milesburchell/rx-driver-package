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
 * Copyright (C) 2019 Renesas Electronics Corporation. All rights reserved.
 *********************************************************************************************************************/
/**********************************************************************************************************************
 * File Name    : r_cellular_closesocket.c
 * Description  : Function to close the socket.
 *********************************************************************************************************************/
/**********************************************************************************************************************
 * History : DD.MM.YYYY Version  Description
 *         : xx.xx.xxxx 1.00     First Release
 *         : 02.09.2021 1.01     Fixed reset timing
 *         : 21.10.2021 1.02     Support for Azure RTOS
 *                               Support for GCC for Renesas GNURX Toolchain
 *         : 15.11.2021 1.03     Improved receiving behavior, removed socket buffers
 *         : 24.01.2022 1.04     R_CELLULAR_SetPSM and R_CELLULAR_SetEDRX have been added as new APIs
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
 * Function Name  @fn            R_CELLULAR_CloseSocket
 ***********************************************************************/
e_cellular_err_t R_CELLULAR_CloseSocket(st_cellular_ctrl_t * const p_ctrl, const uint8_t socket_no)
{
    e_cellular_err_t ret = CELLULAR_SUCCESS;

    if (NULL == p_ctrl)
    {
        ret = CELLULAR_ERR_PARAMETER;
    }

    if (CELLULAR_SUCCESS == ret)
    {
        if (CELLULAR_SYSTEM_CLOSE == p_ctrl->system_state)
        {
            ret = CELLULAR_ERR_NOT_OPEN;
        }
        else if (CELLULAR_SYSTEM_OPEN == p_ctrl->system_state)
        {
            ret =  CELLULAR_ERR_NOT_CONNECT;
        }
        else
        {
            R_BSP_NOP();
        }
    }

    if (CELLULAR_SUCCESS == ret)
    {
        if ((CELLULAR_START_SOCKET_NUMBER > socket_no) || (p_ctrl->creatable_socket < socket_no))
        {
            ret = CELLULAR_ERR_PARAMETER;
        }
    }

    if (CELLULAR_SUCCESS == ret)
    {
        if (CELLULAR_SOCKET_STATUS_CLOSED ==
                p_ctrl->p_socket_ctrl[socket_no - CELLULAR_START_SOCKET_NUMBER].socket_status)
        {
            ret = CELLULAR_ERR_SOCKET_NOT_READY;
        }
    }

    if (CELLULAR_SUCCESS == ret)
    {
        ret = cellular_closesocket(p_ctrl, socket_no);
    }

    return ret;
}
/**********************************************************************************************************************
 * End of function R_CELLULAR_CloseSocket
 *********************************************************************************************************************/
