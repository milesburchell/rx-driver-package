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
* Copyright (C) 2013-2021 Renesas Electronics Corporation. All rights reserved.
***********************************************************************************************************************/
/***********************************************************************************************************************
* File Name    : r_rtc_rx.c
* Description  : Main driver code for the Real Time Clock (RTC) peripheral on RX MCUs.
************************************************************************************************************************
* History : DD.MM.YYYY Version Description
*           09.09.2013 1.00    First release.
*           16.04.2014 2.00    Updated for new API.
*           03.09.2014 2.10    Added support for RX64M.
*           03.12.2014 2.20    Added support for RX113.
*           22.01.2015 2.30    Added support for RX71M.
*           06.07.2015 2.40    Added support for RX231.
*           01.03.2016 2.41    Added support for RX130.
*                              Added support for RX230.
*                              Added the rtc_enable_ints function in order to enable the interrupt
*                              regardless of the cold start or warm start.
*           01.10.2016 2.50    Added support for RX65N.
*                              Change the range of values that can be set in the interrupt priority level.
*                              (Can set value of '0')
*                              Change the specification for the registration of the callback function.
*           31.07.2017 2.70    Added support for RX65N-2MB and RX130-512KB.
*                              Removed support for RX210 and RX63N/631.
*                              Changed return type of rtc_bcd_to_dec function from uint8_t type to int type.
*           20.09.2017 2.71    Fixed issue that count stop occurred at warm start.
*                              - Changed R_RTC_Open so that rtc_set_output is called only at cold start.
*           20.05.2019 2.75    Added support for GNUC and ICCRX.
*                              Fixed coding style. 
*           20.06.2019 2.76    Added support for RX23W.
*           30.07.2019 2.77    Added support for RX72M.
*           22.11.2019 2.78    Added support for RX66N and RX72N.
*                              Modified comment of API function to Doxygen style.
*           30.06.2021 2.81    Added support for RX671.
*           31.07.2021 2.82    Added support for RX140.
***********************************************************************************************************************/

/***********************************************************************************************************************
Includes   <System Includes> , "Project Includes"
***********************************************************************************************************************/
#include "platform.h"
#include "r_rtc_rx_if.h"
#include "r_rtc_rx_config.h"
#include "r_rtc_rx_private.h"

/***********************************************************************************************************************
Macro definitions
***********************************************************************************************************************/

/***********************************************************************************************************************
Typedef definitions
***********************************************************************************************************************/

/***********************************************************************************************************************
Private global variables and functions
***********************************************************************************************************************/

/* Control block for RTC */
static rtc_ctrl_t rcb = {false, FIT_NO_FUNC};

#if (RTC_CFG_PARAM_CHECKING_ENABLE)
static rtc_err_t rtc_check_periodic (rtc_periodic_t freq, uint8_t priority, rtc_cb_func_t p_callback);
static bool check_valid_time (tm_t * p_time);
#endif

static bool leap_year_test (int curr_year);

/* Number of days in month. */
const uint8_t g_days_in_month[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };


/***********************************************************************************************************************
 * Function Name: R_RTC_Open
 ******************************************************************************************************************/ /**
 * @brief This function initializes the RTC, sets the current date/time, configures the relevant interrupt,
 *        and starts counting.\n The function initializes the RTC FIT module. This function must be called
 *        before calling any other API functions.
 * @param[in] p_init
 *             Pointer to initialization structure.
 * @param[in] p_current
 *             Pointer to date/time structure to set current time.
 * @retval    RTC_SUCCESS 
 * @retval    RTC_ERR_ALREADY_OPEN     R_RTC_Open has already been called.
 * @retval    RTC_ERR_BAD_PARAM        Missing or invalid parameter specified
 * @retval    RTC_ERR_MISSING_CALLBACK Callback function has not been set. 
 * @retval    RTC_ERR_TIME_FORMAT      Improper time format (field out of range)
 * @details   This function initializes the RTC and starts the RTC counter.
 *            The function returns RTC_SUCCESS after the RTC has been initialized and started counting successfully.\n 
 *            When the "set_time" member of the "rtc_init_t" structure is set to 'true', the RTC is initialized
 *            and date/time is specified with the "p_current_time" argument. When the "set_time" member is false,
 *            the "p_current_time" argument is ignored. Normally,
 *            "true" is set at cold start and "false" at warm start (such as reset).\n 
 *            The "tm_t" structure which is used for "p_current_time" is defined in the C standard library.
 *            If the compiler does not support it, the "tm_t" structure defined in the "r_rtc_rx_if.h" file is used.
 * @note      Before calling this function, start the sub-clock oscillator and wait for oscillation to stabilize.
 *            For details on oscillating the sub-clock and specifying the oscillation stabilization wait time,
 *            refer to the User's Manual: Hardware for the MCU used.\n 
 *            This function must be called regardless of cold start or warm start.\n
 *            And keep following notes when using clock output.\n
 *            \li Configure the RTCOUT pin with the application software after initializing clock output 
 *            with the R_RTC_Open function or R_RTC_Control function.
 *            \li In warm start mode (rtc_init_t->set_time = false ), configuration of clock output
 *            by R_RTC_Open function are invalid. To use clock output at warm start, configure clock output
 *            with the R_RTC_Control function after calling the R_RTC_Open function.
 *            
 */
rtc_err_t R_RTC_Open (rtc_init_t * p_init, tm_t *p_current)
{
    rtc_err_t err = RTC_SUCCESS;

    /* Prevent multiple initialization */
    if (R_BSP_HardwareLock(BSP_LOCK_RTC) == false)
    {
        return RTC_ERR_ALREADY_OPEN;
    }

#if (RTC_CFG_PARAM_CHECKING_ENABLE)
    /* Check input values. */
    if ((FIT_NO_PTR == p_init) || (NULL == p_init)
     || (p_init->output_freq >= RTC_OUTPUT_END_ENUM))
    {
        R_BSP_HardwareUnlock(BSP_LOCK_RTC);
        return RTC_ERR_BAD_PARAM;
    }

    err = rtc_check_periodic(p_init->periodic_freq, p_init->periodic_priority, p_init->p_callback);
    if (RTC_SUCCESS != err)
    {
        R_BSP_HardwareUnlock(BSP_LOCK_RTC);
        return err;
    }

    if (true == p_init->set_time)
    {
        if ((FIT_NO_PTR == p_current) || (NULL == p_current))
        {
            err = RTC_ERR_BAD_PARAM;
        }
        else if (!check_valid_time(p_current))
        {
            err = RTC_ERR_TIME_FORMAT;
        }
        else
        {
            /* nothing to do */
        }

        if (RTC_SUCCESS != err)
        {
            R_BSP_HardwareUnlock(BSP_LOCK_RTC);
            return err;
        }
    }
#endif

    /* Set the current time if specified (implied cold start) */
    if (true == p_init->set_time)
    {
        rtc_init();
        rtc_set_current_time(p_current);

        /* Set the RTC output */
        rtc_set_output(p_init->output_freq);
    }

    /* Enable the RTC interrupts */
    rtc_enable_ints();

    /* Set callback function for interrupts */
    rcb.p_callback = p_init->p_callback;

    /* Set the periodic frequency */
    rtc_set_periodic(p_init->periodic_freq, p_init->periodic_priority);

    /* Start the RTC counter */
    rtc_counter_run(RTC_COUNTER_START);

    /* Remember that RTC is initialized. */
    rcb.initialized = true;

    return err;     // SUCCESS
}
/**********************************************************************************************************************
End of function R_RTC_Open
***********************************************************************************************************************/


#if (RTC_CFG_PARAM_CHECKING_ENABLE)
/***********************************************************************************************************************
* Function Name: check_valid_time
* Description  : Checks to make sure date/time fields are in range.
* Arguments    : p_time -
*                    Pointer to time structure.
* Return Value : true -
*                    Time information is valid.
*                false -
*                    Time information is invalid.
***********************************************************************************************************************/
static bool check_valid_time (tm_t * p_time)
{
    /* Used for computing max day of the month. */
    uint32_t max_days;

    /* Check seconds. */
    if ((p_time->tm_sec < 0) || (p_time->tm_sec > 59))
    {
        return false;
    }

    /* Check minutes. */
    if ((p_time->tm_min < 0) || (p_time->tm_min > 59))
    {
        return false;
    }

    /* Check hours. */
    if ((p_time->tm_hour < 0) || (p_time->tm_hour > 23))
    {
        return false;
    }

    /* Check day of the week. */
    if ((p_time->tm_wday < 0) || (p_time->tm_wday > 6))
    {
        return false;
    }

    /* Check month. */
    if ((p_time->tm_mon < 0) || (p_time->tm_mon > 11))
    {
        return false;
    }

    /* Check day of the month. */
    /* First compute max days in a month. */
    max_days = g_days_in_month[p_time->tm_mon];

    /* If month is February and this is leap year then add 1 more day. */
    if (1 == p_time->tm_mon)
    {
        /* Check if leap year. */
        if (leap_year_test(p_time->tm_year + 1900))
        {
            /* Add 1 day for leap year. */
            max_days += 1;
        }
    }

    if ((p_time->tm_mday <= 0) || (p_time->tm_mday > max_days))
    {
        return false;
    }

    /* Check year */
    /* MCUs supports years 2000-2099; adjust for years since 1900 */
    if ((p_time->tm_year < 100) || (p_time->tm_year >= 200))
    {
        return false;
    }

    return true;
}
/**********************************************************************************************************************
End of function check_valid_time
***********************************************************************************************************************/


/***********************************************************************************************************************
* Function Name: rtc_check_periodic
* Description  : This function checks to see if the periodic interrupt parameters are valid.
* Arguments    : freq
*                   - frequency; must match enum
*                priority
*                   - interrupt priority; between 0 and 15 (inclusive)
*                p_callback
*                   - callback function pointer; non-NULL
* Return Value : RTC_SUCCESS
*                   - arguments are valid
*                RTC_ERR_BAD_PARAM
*                   - priority out of range
*                RTC_ERR_MISSING_CALLBACK
*                   - callback function required but not specified
***********************************************************************************************************************/
static rtc_err_t rtc_check_periodic (rtc_periodic_t freq, uint8_t priority, rtc_cb_func_t p_callback)
{

    if ((RTC_PERIODIC_OFF != freq) && ((freq < RTC_PERIODIC_256_HZ) || (freq > RTC_PERIODIC_2_SEC)))
    {
        return RTC_ERR_BAD_PARAM;
    }

    if (RTC_PERIODIC_OFF != freq)
    {
        if (NULL == p_callback)
        {
            return RTC_ERR_MISSING_CALLBACK;
        }
        if (priority > 15)
        {
            return RTC_ERR_BAD_PARAM;
        }
    }

    return RTC_SUCCESS;
}
/**********************************************************************************************************************
End of function rtc_check_periodic
***********************************************************************************************************************/
#endif  /* if parameter checking on */


/***********************************************************************************************************************
* Function Name: leap_year_test
* Description  : Determines whether this is a leap year.
* Arguments    : curr_year -
*                    Year to check.
* Return Value : true
*                    - Input year is a leap year.
*                false
*                    - Input year is not a leap year.
***********************************************************************************************************************/
static bool leap_year_test (int curr_year)
{
    /* Leap year or not? */
    bool is_leap = false;

    /* Leap year is a year divisible by 4 and not divisible by 100 (unless also divisible by 400).
     * So the year 2000 and 2400 are leap years, but 2100, 2200, and 2300 are not.
     * Years 2001, 2002, and 2003 are not leap years, but 2004 is.
     */
    if (0 == (curr_year % 400))
    {
        is_leap = true;
    }
    else if ((0 != (curr_year % 100)) && (0 == (curr_year % 4)))
    {
        is_leap = true;
    }
    else
    {
        /* Nothing to do */
    }

    return is_leap;
}
/**********************************************************************************************************************
End of function leap_year_test
***********************************************************************************************************************/


/***********************************************************************************************************************
 * Function Name: R_RTC_Control
 ******************************************************************************************************************/ /**
 * @brief This function updates the current date/time and the alarm date/time, and configures the time
 *        capture function (only when available in the MCU) and other settings.
 * @param[in] cmd
 *             Command to process
 * @param[in] p_args
 *             Pointer to optional argument structure (refer to the Description for each command setting.)
 * @retval    RTC_SUCCESS 
 * @retval    RTC_ERR_NOT_OPENED       R_RTC_Open is not called.
 * @retval    RTC_ERR_BAD_PARAM        Missing or invalid parameter specified
 * @retval    RTC_ERR_MISSING_CALLBACK Callback function has not been specified. 
 * @retval    RTC_ERR_TIME_FORMAT      Improper time format (field out of range)
 * @retval    RTC_ERR_NO_CAPTURE       Time capture event is not detected.
 * @details   This function updates the current date/time and the alarm date/time, and configures the time
 *            capture function (only when available in the MCU) and other settings.
 *            See Section 3.5 in the application note for details.
 * @note      When using time capture function, the pins to be used must be configured by the application
 *            software before executing the RTC_CMD_CONFIG_CAPTURE command in the R_RTC_Control function after
 *            calling the R_RTC_Open function.\n 
 *            And executing the RTC_CMD_SET_OUTPUT command or the RTC_CMD_SET_CURRENT_TIME command
 *            stops RTC counting while processing.
 */
rtc_err_t R_RTC_Control (rtc_cmd_t cmd, void *p_args)
{
    rtc_err_t           err=RTC_SUCCESS;
    rtc_output_t        output_freq;
    rtc_periodic_cfg_t  *p_periodic;
    rtc_alarm_ctrl_t    *p_alm_ctrl;
    tm_t                *p_time;
#if !defined(BSP_MCU_RX11_ALL) && !defined(BSP_MCU_RX130) && !defined(BSP_MCU_RX140)
    rtc_capture_cfg_t   *p_capture;
    rtc_pin_t           pin;
#endif


    /* Has RTC been initialized yet? */
    if (!rcb.initialized)
    {
        return RTC_ERR_NOT_OPENED;
    }


#if (RTC_CFG_PARAM_CHECKING_ENABLE)
    if (((NULL == p_args) || (FIT_NO_PTR == p_args))
            && (RTC_CMD_START_COUNTERS != cmd)
            && (RTC_CMD_STOP_COUNTERS != cmd)
            && (RTC_CMD_PARTIAL_RESET != cmd))
    {
        return RTC_ERR_BAD_PARAM;
    }

    if (cmd >= RTC_CMD_END_ENUM)
    {
        return RTC_ERR_BAD_PARAM;
    }
#endif


    switch (cmd)
    {
        case RTC_CMD_SET_OUTPUT :
            output_freq = *((rtc_output_t *) p_args);
#if (RTC_CFG_PARAM_CHECKING_ENABLE)
            if (output_freq >= RTC_OUTPUT_END_ENUM)
            {
                return RTC_ERR_BAD_PARAM;
            }
#endif
            rtc_set_output(output_freq);
        break;

        case RTC_CMD_SET_PERIODIC :
            p_periodic = (rtc_periodic_cfg_t *) p_args;
#if (RTC_CFG_PARAM_CHECKING_ENABLE)
            err = rtc_check_periodic(p_periodic->frequency, p_periodic->int_priority, rcb.p_callback);
            if (RTC_SUCCESS != err)
            {
                return err;
            }
#endif
            rtc_set_periodic(p_periodic->frequency, p_periodic->int_priority);
        break;

        case RTC_CMD_SET_CURRENT_TIME :
            p_time = (tm_t *) p_args;
#if (RTC_CFG_PARAM_CHECKING_ENABLE)
            if (!check_valid_time(p_time))
            {
                return RTC_ERR_TIME_FORMAT;
            }
#endif
            rtc_set_current_time(p_time);
        break;

        case RTC_CMD_SET_ALARM_TIME :
            p_time = (tm_t *) p_args;
#if (RTC_CFG_PARAM_CHECKING_ENABLE)
            if (!check_valid_time(p_time))
            {
                return RTC_ERR_TIME_FORMAT;
            }
#endif
            rtc_set_alarm_time(p_time);
        break;

        case RTC_CMD_ENABLE_ALARM :
            p_alm_ctrl = (rtc_alarm_ctrl_t *) p_args;
#if (RTC_CFG_PARAM_CHECKING_ENABLE)
            if (p_alm_ctrl->int_priority > 15)
            {
                return RTC_ERR_BAD_PARAM;
            }
#endif
            rtc_enable_alarms(p_alm_ctrl);
        break;

        case RTC_CMD_START_COUNTERS :
            rtc_counter_run(RTC_COUNTER_START);
        break;

        case RTC_CMD_STOP_COUNTERS :
            rtc_counter_run(RTC_COUNTER_STOP);
        break;

        case RTC_CMD_PARTIAL_RESET :     // primarily Output clock, Alarms and Capture registers
            rtc_reset();
        break;

#if !defined(BSP_MCU_RX11_ALL) && !defined(BSP_MCU_RX130) && !defined(BSP_MCU_RX140)
        case RTC_CMD_CONFIG_CAPTURE :
            p_capture = (rtc_capture_cfg_t *) p_args;
#if (RTC_CFG_PARAM_CHECKING_ENABLE)
            if ((p_capture->pin >= RTC_NUM_PINS)
             || (p_capture->edge >= RTC_EDGE_END_ENUM)
             || ((RTC_FILTER_OFF != p_capture->filter) &&
                 (RTC_FILTER_DIV1 != p_capture->filter) &&
                 (RTC_FILTER_DIV32 != p_capture->filter)))
            {
                return RTC_ERR_BAD_PARAM;
            }
#endif
            rtc_config_capture(p_capture);
        break;

        case RTC_CMD_CHECK_PIN0_CAPTURE :
            err = rtc_check_capture(RTC_PIN_0, (tm_t *) p_args);
        break;

        case RTC_CMD_CHECK_PIN1_CAPTURE :
            err = rtc_check_capture(RTC_PIN_1, (tm_t *) p_args);
        break;

        case RTC_CMD_CHECK_PIN2_CAPTURE :
            err = rtc_check_capture(RTC_PIN_2, (tm_t *) p_args);
        break;

        case RTC_CMD_DISABLE_CAPTURE :
            pin = *((rtc_pin_t *) p_args);
    #if (RTC_CFG_PARAM_CHECKING_ENABLE)
            if (pin >= RTC_NUM_PINS)
            {
                return RTC_ERR_BAD_PARAM;
            }
    #endif
            rtc_disable_capture(pin);
        break;

#endif /* not RX11x, RX130, RX140 */

        default :
            return RTC_ERR_BAD_PARAM;
    }

    return err;
}
/**********************************************************************************************************************
End of function R_RTC_Control
***********************************************************************************************************************/


/***********************************************************************************************************************
 * Function Name: R_RTC_Read
 ******************************************************************************************************************/ /**
 * @brief This function returns the current date/time and the alarm date/time set in the RTC.
 * @param[out] p_current
 *             Pointer for loading the current date/time from the RTC. Specify NULL or FIT_NO_PTR to skip
 *             reading the current date/time.
 * @param[out] p_alarm
 *             Pointer for loading the alarm date/time from the RTC. Specify NULL or FIT_NO_PTR to skip
 *             reading the alarm date/time.
 * @retval    RTC_SUCCESS 
 * @retval    RTC_ERR_NOT_OPENED R_RTC_Open is not called.
 * @details   This function reads the current date/time and the alarm date/time.
 * @note      To read the current date/time using this function after return from a reset, deep software
 *            standby mode, software standby mode, or the battery backup state, wait for 1/128 second
 *            while counting has been started with the condition of (RCR2.START bit = 1).\n 
 *            When a carry of the RTC counter occurs while reading the current time, this function reads
 *            the current time again. For checking the carry, the function uses carry interrupt status flag (IR bit).\n 
 *            For that, it enables carry interrupt (RCR1.CIE bit = 1).  So, do not clear this status flag
 *            in the application software.
 */
rtc_err_t R_RTC_Read (tm_t *p_current, tm_t *p_alarm)
{
#if (RTC_CFG_CALCULATE_YDAY == 1)
    int32_t loop;     // Loop counter
#endif      
    rtc_err_t ret = RTC_SUCCESS;

#if (RTC_CFG_PARAM_CHECKING_ENABLE)
    if (!rcb.initialized)
    {
        return RTC_ERR_NOT_OPENED;
    }
#endif

    /* Load current time */
    if ((FIT_NO_PTR != p_current) && (NULL != p_current))
    {
        rtc_read_current(p_current);

        /* Daylight Savings Time unknown */
        p_current->tm_isdst = -1;

        /* Initialize day of year */
        p_current->tm_yday = 0;

#if (RTC_CFG_CALCULATE_YDAY == 1)
        /* Calculating days since January 1st. */
        /* WAIT_LOOP */
        for (loop = 0; loop < p_current->tm_mon; loop++)
        {
            /* Add days for previous months. */
            p_current->tm_yday += g_days_in_month[loop];

            /* Check for leap year. */
            if (1 == loop)
            {
                if (leap_year_test(p_current->tm_year+1900))
                {
                    /* Add 1 day leap year. */
                    p_current->tm_yday += 1;
                }
            }
        }

        /* Add days for this current month. */
        p_current->tm_yday += p_current->tm_mday;
        p_current->tm_yday -= 1; // range is [0, 365]
#endif      
    }

    /* Load alarm time */
    if ((FIT_NO_PTR != p_alarm) && (NULL != p_alarm))
    {
        rtc_read_alarm(p_alarm);
    }

    return ret;
}
/**********************************************************************************************************************
End of function R_RTC_Read
***********************************************************************************************************************/


/***********************************************************************************************************************
 * Function Name: R_RTC_Close
 ******************************************************************************************************************/ /**
 * @details   This function stops counting, resets the RTC, and disables all RTC interrupts.
 * @note      None.
 */
void R_RTC_Close (void)
{

    if (false == rcb.initialized)
    {
        return;
    }

    rtc_counter_run(RTC_COUNTER_STOP);      // stop counters
    rtc_disable_ints();                     // disable interrupts
    rtc_reset();                            // clear capture and alarms

    /* Do NOT stop sub-clock in case needed by application */

    rcb.initialized = false;
    R_BSP_HardwareUnlock(BSP_LOCK_RTC);

    return;
}
/**********************************************************************************************************************
End of function R_RTC_Close
***********************************************************************************************************************/


/***********************************************************************************************************************
 * Function Name: R_RTC_GetVersion
 ******************************************************************************************************************/ /**
 * @brief This function returns the driver version number at runtime.
 * @return    Version number.
 * @details   Returns the version of this module. The top 2 bytes are the major version number and the bottom
 *            2 bytes are the minor version number.
 * @note      None.
 */
uint32_t R_RTC_GetVersion (void)
{
    return ((((uint32_t) RTC_RX_VERSION_MAJOR) << 16) | ((uint32_t) RTC_RX_VERSION_MINOR));
}
/**********************************************************************************************************************
End of function R_RTC_GetVersion
***********************************************************************************************************************/


/***********************************************************************************************************************
* Function Name: rtc_bcd_to_dec
* Description  : Converts from binary coded decimal (BCD) to decimal
* Arguments    : to_convert -
*                    Value to convert.
* Return Value : Converted value.
***********************************************************************************************************************/
int rtc_bcd_to_dec (uint8_t to_convert)
{
    return (int) ((((to_convert & 0xF0) >> 4) * 10) + (to_convert & 0x0F));
}
/**********************************************************************************************************************
End of function rtc_bcd_to_dec
***********************************************************************************************************************/


/***********************************************************************************************************************
* Function Name: rtc_dec_to_bcd
* Description  : Converts from decimal to binary coded decimal (BCD)
* Arguments    : to_convert -
*                    Value to convert.
* Return Value : Converted value.
***********************************************************************************************************************/
uint8_t rtc_dec_to_bcd (uint8_t to_convert)
{
    return (uint8_t) ((((to_convert / 10) << 4) & 0xF0) | (to_convert % 10));
}
/**********************************************************************************************************************
End of function rtc_dec_to_bcd
***********************************************************************************************************************/


/***********************************************************************************************************************
* Function Name: rtc_alm_isr
* Description  : This alarm interrupt handler calls the callback function with an event
*                indicating that an alarm event occurred.
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/
R_BSP_PRAGMA_STATIC_INTERRUPT(rtc_alm_isr, VECT(RTC,ALM))
R_BSP_ATTRIB_STATIC_INTERRUPT void rtc_alm_isr(void)
{
    if ((NULL != rcb.p_callback) && (FIT_NO_FUNC != rcb.p_callback))
    {
        rtc_cb_evt_t event = RTC_EVT_ALARM;

        /* presence of callback function verified in Control() */
        rcb.p_callback(&event);
    }
}
/**********************************************************************************************************************
End of function rtc_alm_isr
***********************************************************************************************************************/


/***********************************************************************************************************************
* Function Name: rtc_prd_isr
* Description  : This periodic interrupt handler calls the callback function with an event
*                indicating that a periodic interrupt occurred.
* Arguments    : None
* Return Value : None
***********************************************************************************************************************/
R_BSP_PRAGMA_STATIC_INTERRUPT(rtc_prd_isr, VECT(RTC,PRD))
R_BSP_ATTRIB_STATIC_INTERRUPT void rtc_prd_isr(void)
{
    if ((NULL != rcb.p_callback) && (FIT_NO_FUNC != rcb.p_callback))
    {
        rtc_cb_evt_t event = RTC_EVT_PERIODIC;

        /* presence of callback function verified in Open() and Control() */
        rcb.p_callback(&event);
    }
}
/**********************************************************************************************************************
End of function rtc_prd_isr
***********************************************************************************************************************/

