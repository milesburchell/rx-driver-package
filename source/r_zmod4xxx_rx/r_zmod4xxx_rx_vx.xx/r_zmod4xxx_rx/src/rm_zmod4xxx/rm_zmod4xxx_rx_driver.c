/***********************************************************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No 
* other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all 
* applicable laws, including copyright laws. 
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM 
* EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES 
* SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS 
* SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of 
* this software. By using this software, you agree to the additional terms and conditions found by accessing the 
* following link:
* http://www.renesas.com/disclaimer
*
* Copyright (C) 2021 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/

/**********************************************************************************************************************
 * Includes   <System Includes> , "Project Includes"
 *********************************************************************************************************************/
#include "r_zmod4xxx_if.h"

/**********************************************************************************************************************
 * Macro definitions
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Local Typedef definitions
 *********************************************************************************************************************/

/**********************************************************************************************************************
 * Exported global variables
 *********************************************************************************************************************/
fsp_err_t rm_zmod4xxx_delay_ms(rm_zmod4xxx_ctrl_t * const p_ctrl, uint32_t const delay_ms);
fsp_err_t rm_zmod4xxx_irq_open(rm_zmod4xxx_ctrl_t * const p_api_ctrl);
fsp_err_t rm_zmod4xxx_irq_close(rm_zmod4xxx_ctrl_t * const p_api_ctrl);
void      rm_zmod4xxx_irq_callback(external_irq_callback_args_t * p_args);

/**********************************************************************************************************************
 * Private (static) variables and functions
 *********************************************************************************************************************/

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief Delay some milliseconds.
 *
 * @retval FSP_SUCCESS              successfully configured.
 **********************************************************************************************************************/
fsp_err_t rm_zmod4xxx_delay_ms (rm_zmod4xxx_ctrl_t * const p_ctrl, uint32_t const delay_ms)
{
    FSP_PARAMETER_NOT_USED(p_ctrl);

    /* Software delay */
    R_BSP_SoftwareDelay(delay_ms, BSP_DELAY_MILLISECS);

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Open IRQ driver and set callback function.
 *
 * @retval FSP_SUCCESS              successfully configured.
 **********************************************************************************************************************/
fsp_err_t rm_zmod4xxx_irq_open (rm_zmod4xxx_ctrl_t * const p_api_ctrl)
{
#if RM_ZMOD4XXX_CFG_IRQ_ENABLE
    fsp_err_t err = FSP_SUCCESS;
    rm_zmod4xxx_instance_ctrl_t * p_ctrl         = (rm_zmod4xxx_instance_ctrl_t *) p_api_ctrl;
    external_irq_instance_t     * p_irq_instance = (external_irq_instance_t *) p_ctrl->p_irq_instance;
    irq_err_t ret;

    /* Open IRQ driver */
    ret = R_IRQ_Open(p_irq_instance->number, 
                     p_irq_instance->trigger, 
                     p_irq_instance->priority, 
                     p_irq_instance->p_handle, 
                     p_irq_instance->p_callback);

    switch (ret)
    {
        case IRQ_SUCCESS :
         err = FSP_SUCCESS;
         break;
        case IRQ_ERR_BAD_NUM :
         err = FSP_ERR_INVALID_CHANNEL;
         break;
        case IRQ_ERR_NOT_CLOSED :
         err = FSP_ERR_IN_USE;
         break;
        case IRQ_ERR_INVALID_PTR :
         err = FSP_ERR_INVALID_POINTER;
         break;
        case IRQ_ERR_INVALID_ARG :
         err = FSP_ERR_INVALID_ARGUMENT;
         break;
        case IRQ_ERR_LOCK :
         err = FSP_ERR_HW_LOCKED;
         break;
        default :
         err = FSP_ERR_ASSERTION;
         break;
    }

    FSP_ERROR_RETURN(FSP_SUCCESS == err, err);
#endif
    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief Close IRQ driver.
 *
 * @retval FSP_SUCCESS              successfully configured.
 **********************************************************************************************************************/
fsp_err_t rm_zmod4xxx_irq_close (rm_zmod4xxx_ctrl_t * const p_api_ctrl)
{
#if RM_ZMOD4XXX_CFG_IRQ_ENABLE
    rm_zmod4xxx_instance_ctrl_t * p_ctrl         = (rm_zmod4xxx_instance_ctrl_t *) p_api_ctrl;
    external_irq_instance_t     * p_irq_instance = (external_irq_instance_t *) p_ctrl->p_irq_instance;

    /* Close IRQ driver */
    R_IRQ_Close(*p_irq_instance->p_handle);
#endif
    return FSP_SUCCESS;
}

void rm_zmod4xxx_irq_callback (external_irq_callback_args_t * p_args)
{
    rm_zmod4xxx_instance_ctrl_t * p_ctrl = (rm_zmod4xxx_instance_ctrl_t *) p_args->p_context;
    rm_zmod4xxx_callback_args_t   zmod4xxx_callback_args;

    if (0 == p_ctrl->open)
    {
        /* Set IRQ flag */
        p_ctrl->init_process_params.measurement_finished = true;
    }
    else
    {
        /* Set context */
        zmod4xxx_callback_args.p_context = p_ctrl->p_context;

        /* Set event */
        zmod4xxx_callback_args.event = RM_ZMOD4XXX_EVENT_MEASUREMENT_COMPLETE;
        p_ctrl->event                = RM_ZMOD4XXX_EVENT_MEASUREMENT_COMPLETE;

        if (NULL != p_ctrl->p_irq_callback)
        {
            /* Call callback function */
            p_ctrl->p_irq_callback(&zmod4xxx_callback_args);
        }
    }
}
