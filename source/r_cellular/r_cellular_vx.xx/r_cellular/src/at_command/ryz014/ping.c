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
 * File Name    : ping.c
 * Description  : Function to execute the AT command (PING).
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
#define CELLULAR_PING_TIMEOUT       (60000)
#if BSP_CFG_RTOS_USED == (5)
#define MAX_DOMAIN_NAME_LENGTH      (255)
#define TOTAL_OTHER_BLOCK_SIZE      (MAX_DOMAIN_NAME_LENGTH + 1 + sizeof(void *))
#endif

/**********************************************************************************************************************
 * Typedef definitions
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported global variables
 *********************************************************************************************************************/
#if BSP_CFG_RTOS_USED == (5)
TX_BLOCK_POOL g_cellular_other_pool;
static uint8_t s_cellular_other_pool[MAX_DOMAIN_NAME_LENGTH + 1];
#endif

/**********************************************************************************************************************
 * Private (static) variables and functions
 *********************************************************************************************************************/

/*************************************************************************************************
 * Function Name  @fn            atc_ping
 ************************************************************************************************/
e_cellular_err_t atc_ping(st_cellular_ctrl_t * const p_ctrl, const uint8_t * const p_host)
{
    e_cellular_err_t ret = CELLULAR_SUCCESS;
    e_cellular_err_atc_t at_ret = CELLULAR_ATC_OK;
    uint8_t len = 0;
    void * p_malloc_ret = NULL;
#if BSP_CFG_RTOS_USED == (5)
    UINT rtos_ret;
#endif
    len = strlen((char *)p_host);   //(uint8_t *) -> (char *)

#if BSP_CFG_RTOS_USED == (1)
    p_malloc_ret = cellular_malloc((len + 1) * sizeof(uint8_t));
#elif BSP_CFG_RTOS_USED == (5)
    rtos_ret = tx_block_pool_create(&g_cellular_other_pool, "other pool",
            sizeof(MAX_DOMAIN_NAME_LENGTH + 1), s_cellular_other_pool, TOTAL_OTHER_BLOCK_SIZE);

    if (TX_SUCCESS == rtos_ret)
    {
        tx_block_allocate(&g_cellular_other_pool, &p_malloc_ret, TX_NO_WAIT);
    }
#endif

    if (NULL != p_malloc_ret)
    {
        strncpy((char *)p_malloc_ret, (char *)p_host, len + 1);   // (uint8_t *)->(char *)

        const uint8_t * const p_command_arg[CELLULAR_MAX_ARG_COUNT] = {p_malloc_ret}; // (&uint8_t[])->(uint8_t *)

        atc_generate(p_ctrl->sci_ctrl.atc_buff,
            (const uint8_t *)&gp_at_command[ATC_PING][0], // (const uint8_t *const *)->(const uint8_t **)
                (const uint8_t **)&p_command_arg);        // (const uint8_t *const *)->(const uint8_t **)

        at_ret = cellular_execute_at_command(p_ctrl, CELLULAR_PING_TIMEOUT, ATC_RETURN_OK, ATC_PING);

        cellular_free(p_malloc_ret);
#if BSP_CFG_RTOS_USED == (5)
        tx_block_pool_delete(&g_cellular_other_pool);
#endif

        if (CELLULAR_ATC_OK != at_ret)
        {
            ret = CELLULAR_ERR_MODULE_COM;
        }
    }
    else
    {
        ret = CELLULAR_ERR_MEMORY_ALLOCATION;
    }

    return ret;
}
/**********************************************************************************************************************
 * End of function atc_ping
 *********************************************************************************************************************/
