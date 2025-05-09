/*
 * main.c
 *
 *  Created on: May 2, 2025
 */

/* Including necessary configuration files. */
#include "Mcal.h"
#include "Siul2_Port_Ip.h" // For Port initialization
#include "Siul2_Dio_Ip.h"  // For LED control
#include "hse_config.h"
#include "boot_recovery.h"
#include "advanced_security.h"
#include "fallback_manager.h"
#include "update_manager.h"

/* Function prototypes */
void Show_StatusLED(uint8_t pattern);
void Delay_ms(uint32_t ms);
uint32_t Create_InitialFallback(void);
uint32_t Check_PendingUpdates(void);

/* Status LED patterns */
#define LED_PATTERN_BOOT_SUCCESS 0x01
#define LED_PATTERN_FALLBACK_CREATION 0x02
#define LED_PATTERN_UPDATE_READY 0x03
#define LED_PATTERN_ERROR 0x04

int main(void)
{
    uint32_t status;
    uint32_t init_status = 0;

    /* Initialize GPIO */
    Siul2_Port_Ip_Init(NUM_OF_CONFIGURED_PINS0, g_pin_mux_InitConfigArr0);

    /* Initialize HSE for secure boot */
    status = HSE_SecureBoot_Init();

    if (status != HSE_STATUS_SUCCESS)
    {
        /* Boot failure */
        Show_StatusLED(LED_PATTERN_ERROR);

        /* Log the failure */
        Boot_LogStatus(BOOT_STATUS_BOOT_FAILURE, (uint16_t)status);

        /* Update boot failure counter */
        Boot_IncrementFailureCount();

        /* Perform device reset to attempt recovery */
        volatile uint32_t* resetReg = (volatile uint32_t*)0xE000ED0C;
        *resetReg = 0x5FA0004;

        /* Should not reach here */
        while(1) {}
    }

    /* Boot successful - create initial fallback if needed */
    init_status = Create_InitialFallback();
    if (init_status != 0) {
        /* Log fallback creation failure - non-fatal */
        Boot_LogStatus(BOOT_STATUS_WARNING, (uint16_t)init_status);
    }

    /* Check for pending updates */
    if (Check_PendingUpdates() == 1) {
        /* Update is pending, notify user */
        Show_StatusLED(LED_PATTERN_UPDATE_READY);

        /* Start update process */
        Update_ApplyPendingUpdates();
    }

    /* Show boot success pattern */
    Show_StatusLED(LED_PATTERN_BOOT_SUCCESS);

    /* Log successful boot */
    Boot_LogStatus(BOOT_STATUS_SUCCESS, 0);

    /* Main application loop */
    for(;;)
    {
        /* Application code would go here */

        /* Perform periodic integrity checks */
        HSE_PerformIntegrityCheck();

        /* Add delay to reduce CPU usage */
        Delay_ms(100);
    }

    return 0;
}

/*
 * Create initial fallback firmware if none exists
 * Returns: 0 on success, error code otherwise
 */
uint32_t Create_InitialFallback(void)
{
    uint32_t status;

    /* Initialize fallback manager */
    status = Fallback_Init();
    if (status != FALLBACK_STATUS_SUCCESS) {
        return 1; /* Fallback initialization failed */
    }

    /* Check if valid fallback already exists */
    if (Fallback_IsValid()) {
        return 0; /* Fallback already exists, nothing to do */
    }

    /* No valid fallback exists, create one from current firmware */
    Show_StatusLED(LED_PATTERN_FALLBACK_CREATION);

    /* Create fallback using current firmware */
    status = Fallback_StoreCurrentFirmware();
    if (status != FALLBACK_STATUS_SUCCESS) {
        return 2; /* Fallback creation failed */
    }

    /* Verify fallback integrity */
    status = Fallback_VerifyIntegrity();
    if (status != FALLBACK_STATUS_SUCCESS) {
        return 3; /* Fallback integrity check failed */
    }

    /* Verify fallback signature */
    status = HSE_VerifyFallbackSignature();
    if (status != HSE_STATUS_SUCCESS) {
        return 4; /* Fallback signature verification failed */
    }

    /* Log successful fallback creation */
    Boot_LogStatus(BOOT_STATUS_FALLBACK_CREATED, 0);

    return 0; /* Success */
}

/*
 * Check for pending firmware updates
 * Returns: 1 if update is pending, 0 otherwise
 */
uint32_t Check_PendingUpdates(void)
{
    return Update_IsPending();
}

/*
 * Show status LED pattern
 * pattern: LED pattern code to display
 */
void Show_StatusLED(uint8_t pattern)
{
    uint32_t i;

    /* Clear LEDs */
    Siul2_Dio_Ip_WritePin(LED_RED_PORT, (1U << LED_RED_PIN), 0);
    Siul2_Dio_Ip_WritePin(LED_GREEN_PORT, (1U << LED_GREEN_PIN), 0);

    /* Display LED pattern based on code */
    switch (pattern) {
        case LED_PATTERN_BOOT_SUCCESS:
            /* Green LED on steady */
        	Siul2_Dio_Ip_WritePin(LED_GREEN_PORT, (1U << LED_GREEN_PIN), 1);
            break;

        case LED_PATTERN_FALLBACK_CREATION:
            /* Green LED blink 3 times */
            for (i = 0; i < 3; i++) {
            	Siul2_Dio_Ip_WritePin(LED_GREEN_PORT, (1U << LED_GREEN_PIN), 1);
                Delay_ms(200);
                Siul2_Dio_Ip_WritePin(LED_GREEN_PORT, (1U << LED_GREEN_PIN), 0);
                Delay_ms(200);
            }
            break;

        case LED_PATTERN_UPDATE_READY:
            /* Alternating green/red */
            for (i = 0; i < 5; i++) {
            	Siul2_Dio_Ip_WritePin(LED_GREEN_PORT, (1U << LED_GREEN_PIN), 1);
            	Siul2_Dio_Ip_WritePin(LED_RED_PORT, (1U << LED_RED_PIN), 0);
                Delay_ms(200);
                Siul2_Dio_Ip_WritePin(LED_GREEN_PORT, (1U << LED_GREEN_PIN), 0);
                Siul2_Dio_Ip_WritePin(LED_RED_PORT, (1U << LED_RED_PIN), 1);
                Delay_ms(200);
            }
            break;

        case LED_PATTERN_ERROR:
            /* Red LED rapid blink */
            for (i = 0; i < 10; i++) {
            	Siul2_Dio_Ip_WritePin(LED_RED_PORT, (1U << LED_RED_PIN), 1);
                Delay_ms(100);
                Siul2_Dio_Ip_WritePin(LED_RED_PORT, (1U << LED_RED_PIN), 0);
                Delay_ms(100);
            }
            break;

        default:
            /* Unknown pattern - both LEDs on */
        	Siul2_Dio_Ip_WritePin(LED_RED_PORT, (1U << LED_RED_PIN), 1);
        	Siul2_Dio_Ip_WritePin(LED_GREEN_PORT, (1U << LED_GREEN_PIN), 1);
            break;
    }
}

/*
 * Delay function (milliseconds)
 */
void Delay_ms(uint32_t ms)
{
    /* Simple delay implementation */
    volatile uint32_t cycles = ms * 8000; /* Adjust based on CPU clock */
    while (cycles--) { }
}
