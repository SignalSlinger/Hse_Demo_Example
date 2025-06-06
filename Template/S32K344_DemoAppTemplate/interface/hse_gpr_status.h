/**
*   @file    hse_gpr_status.h
*
*   @brief   HSE GPR Status
*   @details This file contains the HSE GPR Status
*
*   @addtogroup hse_gpr HSE GPR Status
*   @ingroup class_interface
*   @{
*/
/*==================================================================================================
*
*   Copyright 2022 NXP.
*
*   This software is owned or controlled by NXP and may only be used strictly in accordance with
*   the applicable license terms. By expressly accepting such terms or by downloading, installing,
*   activating and/or otherwise using the software, you are agreeing that you have read, and that
*   you agree to comply with and are bound by, such license terms. If you do not agree to
*   be bound by the applicable license terms, then you may not retain, install, activate or
*   otherwise use the software.
==================================================================================================*/
/*==================================================================================================
==================================================================================================*/

#ifndef HSE_GPR_STATUS_H
#define HSE_GPR_STATUS_H

#ifdef __cplusplus
extern "C"{
#endif

/*==================================================================================================
*                                        INCLUDE FILES
* 1) system and project includes
* 2) needed interfaces from external units
* 3) internal and external interfaces from this unit
==================================================================================================*/

#include "std_typedefs.h"
#include "hse_platform.h"

#define HSE_START_PRAGMA_PACK
#include "hse_compiler_abs.h"

/*==================================================================================================
*                              SOURCE FILE VERSION INFORMATION
==================================================================================================*/

/*==================================================================================================
*                                     FILE VERSION CHECKS
==================================================================================================*/

/*==================================================================================================
*                                          CONSTANTS
==================================================================================================*/

/*==================================================================================================
*                                      DEFINES AND MACROS
==================================================================================================*/

/** @brief    HSE Tamper Config Register Address
 *  @details  This status GPR register is updated when a tamper is configured in HSE during initialization or via attribute.
 *            The HOST can read the HSE register to check what tampers are configured.
 *            This register is read-only.
 *            @note
 *            - For HSE_H/M, HSE-GPR REG3 used.
 *            - For HSE_B,   CONFIG_REG4 used.
 */
#if defined(HSE_H)
    #if (HSE_PLATFORM == HSE_S32ZE)
    #define HSE_GPR_STATUS_ADDRESS                 (0x42280024UL)   /**< @brief HSE-GPR REG3 is in Subsystem Register Description */
    #else
    #define HSE_GPR_STATUS_ADDRESS                 (0x4007C920UL)   /**< @brief HSE-GPR REG3 is in Subsystem Register Description */
    #endif
#elif defined(HSE_M)
    #define HSE_GPR_STATUS_ADDRESS                 (0x400D8928UL)   /**< @brief HSE-GPR REG3 is in Subsystem Register Description */
    #if (HSE_PLATFORM == HSE_SAF85XX)
    #define HSE_TMU_BIST_MODE_TEST_BJT_CORE_SEQ1   (0x400D8940UL)   /**< @brief HSE-GPR REG9 is in Subsystem Register Description, <br>
                                                                                Sequence 1 – To get the XOUT value in BIST mode BJT Core,
                                                                                             This result is denoted as XOUTbist. */
    #define HSE_TMU_BIST_MODE_TEST_BJT_CORE_SEQ2   (0x400D8944UL)   /**< @brief HSE-GPR REG10 is in Subsystem Register Description, <br>
                                                                                Sequence 2 – To get the XOUT value in temperature acquisition mode.
                                                                                             This result is denoted as XOUT. */
    #define HSE_TMU_BIST_MODE_TEST_ADC_OUTPUT      (0x400D8948UL)   /**< @brief HSE-GPR REG11 is in Subsystem Register Description, <br>
                                                                                Read the adcout data for BIST mode test ADC, <br>
                                                                                The obtained (ADCout / 32768.0) has to be approximately equal to 0.4 */
    #define HSE_GPR_XOSC_CLK_SWITCH_STATUS_ADDRESS (0x400D8950UL)   /**< @brief HSE-GPR REG13 is in Subsystem Register Description */
    #endif /* (HSE_PLATFORM == HSE_SAF85XX) */
#elif defined(HSE_B)
    #define HSE_GPR_STATUS_ADDRESS                 (0x4039C02CUL)   /**< @brief CONFIG_REG4 is in Configuration GPR Description */
#endif

/** @brief    HSE Tamper Config Status bits (register address is #HSE_GPR_STATUS_ADDRESS) */
typedef uint32_t hseTamperConfigStatus_t;
#define HSE_CMU_TAMPER_CONFIG_STATUS        ((hseTamperConfigStatus_t)1U << 0U) /**< @brief HSE-GPR REG3[0]- this bit is set when the CMU tamper is configured:
                                                                                            - For HSE-H, the clock must be configured in this range:
                                                                                                10Mhz < clock frequency < 420Mhz.
                                                                                            - For HSE-B, the clock must be configured in this range:
                                                                                                3Mhz < clock frequency < 126Mhz.
                                                                                            - For HSE-M, the clock must be configured in this range:
                                                                                                - s32r41x: 45.6Mhz < clock frequency < 420Mhz.
                                                                                                - saf85xx: 39.96Mhz < clock frequency < 320.32Mhz. */

#ifdef HSE_SPT_PHYSICAL_TAMPER_CONFIG
#define HSE_PHYSICAL_TAMPER_CONFIG_STATUS    ((hseTamperConfigStatus_t)1U << 1U) /**< @brief HSE-GPR REG3[1]- this bit is set when the physical tamper is configured.
                                                                                             Note that the application must configure SIUL2 Pads before enabling the tamper. */
#endif /* HSE_SPT_PHYSICAL_TAMPER_CONFIG */

#if (defined(HSE_SPT_TEMP_SENS_VIO_CONFIG) || defined(HSE_SPT_TMU_REG_CONFIG))
#define HSE_TEMP_SENSOR_VIO_CONFIG_STATUS    ((hseTamperConfigStatus_t)1U << 2U) /**< @brief HSE-GPR REG3[2] this bit is set when the
                                                                                             configuration of Temperature Sensor violation. */
#endif /* HSE_SPT_TEMP_SENS_VIO_CONFIG || HSE_SPT_TMU_REG_CONFIG */

#if defined(HSE_M)
#define TMU_CMU_TAMPER_CONFIG_STATUS         ((hseTamperConfigStatus_t)1U << 3U) /**< @brief HSE-GPR REG3[3]- this bit is set when the TMU_CMU tamper is configured.
                                                                                             The TMU clock must be configured in this range
                                                                                               - s32r41x: 11.4Mhz  < clock frequency < 131.25Mhz.
                                                                                               - saf85xx: 39.96Mhz < clock frequency < 40.04Mhz
*/
#endif /* HSE_M */



/** @brief    HSE XOSC Switch Status bits (register address is #HSE_GPR_XOSC_CLK_SWITCH_STATUS_ADDRESS) */
#if (HSE_PLATFORM == HSE_SAF85XX)
typedef uint32_t hseXoscClkSwitchStatus_t;
#define HSE_XOSC_CLK_SWITCH_NOT_RUN_STATUS     ((hseXoscClkSwitchStatus_t)0U)       /**< @brief HSE-GPR REG13[2:1] = 00b when XOSC clock switch not run */
#define HSE_XOSC_CLK_SWITCH_FAIL_STATUS        ((hseXoscClkSwitchStatus_t)1U << 1U) /**< @brief HSE-GPR REG13[2:1] = 01b; this bit is set when the XOSC clock switch is failed */
#define HSE_XOSC_CLK_SWITCH_SUCCESS_STATUS     ((hseXoscClkSwitchStatus_t)1U << 2U) /**< @brief HSE-GPR REG13[2:1] = 10b; this bit is set when the XOSC clock switch is successful */
#endif /* HSE_PLATFORM == HSE_SAF85XX */

/*==================================================================================================
*                                             ENUMS
==================================================================================================*/

/*==================================================================================================
                                 STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/

/*==================================================================================================
                                 GLOBAL VARIABLE DECLARATIONS
==================================================================================================*/

/*==================================================================================================
                                     FUNCTION PROTOTYPES
==================================================================================================*/

#define HSE_STOP_PRAGMA_PACK
#include "hse_compiler_abs.h"

#ifdef __cplusplus
}
#endif

#endif /* HSE_GPR_STATUS_H */

/** @} */
