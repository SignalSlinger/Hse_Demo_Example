/*
 * @file boot_recovery.c
 * @brief Boot recovery mechanisms implementation
 *
 *  Created on: May 2, 2025
 */

#include "boot_recovery.h"
#include "hse_config.h"
#include "Siul2_Port_Ip.h" // For Port initialization
#include "Siul2_Dio_Ip.h"  // For LED control

/* Internal variables */
static boot_recovery_ctx_t recoveryContext;
static const uint32_t BOOT_LOG_MAGIC = 0x424C4F47; /* "BLOG" */
static const uint32_t BOOT_LOG_VERSION = 0x00010000; /* v1.0 */

/* CRC-32 calculation table */
static const uint32_t crc32_table[256] = {
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    /* ... Add complete CRC table or use a CRC library ... */
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

/**
 * @brief Calculate CRC-32 for data integrity
 * @param data Pointer to data
 * @param length Length of data in bytes
 * @return CRC-32 value
 */
static uint32_t Boot_CalculateCRC32(const uint8_t *data, uint32_t length) {
    uint32_t crc = 0xFFFFFFFF;
    
    for (uint32_t i = 0; i < length; i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    
    return crc ^ 0xFFFFFFFF;
}

/**
 * @brief Get pointer to boot log in non-volatile memory
 * @return Pointer to boot log header
 */
static boot_log_header_t* Boot_GetLogHeader(void) {
    return (boot_log_header_t*)BOOT_LOG_ADDRESS;
}

/**
 * @brief Get pointer to boot log entries
 * @return Pointer to first boot log entry
 */
static boot_log_entry_t* Boot_GetLogEntries(void) {
    return (boot_log_entry_t*)(BOOT_LOG_ADDRESS + sizeof(boot_log_header_t));
}

/**
 * @brief Initialize the boot log if not already initialized
 * @return Status code
 */
static uint32_t Boot_InitializeLog(void) {
    boot_log_header_t* header = Boot_GetLogHeader();
    
    /* Check if log is already initialized */
    if (header->magic == BOOT_LOG_MAGIC && header->version == BOOT_LOG_VERSION) {
        /* Verify CRC */
        uint32_t calculatedCRC = Boot_CalculateCRC32((uint8_t*)header, 
                                                     sizeof(boot_log_header_t) - sizeof(uint32_t));
        if (calculatedCRC != header->crc) {
            /* CRC mismatch, log is corrupted */
            /* Re-initialize log */
        }
    }
    
    /* Initialize boot log if needed */
    if (header->magic != BOOT_LOG_MAGIC || header->version != BOOT_LOG_VERSION) {
        /* Mock implementation - in production, use proper NVM write */
        header->magic = BOOT_LOG_MAGIC;
        header->version = BOOT_LOG_VERSION;
        header->entries = 0;
        header->lastEntryIndex = 0;
        header->recoveryContext.consecutiveFailures = 0;
        header->recoveryContext.recoveryMode = 0;
        header->recoveryContext.lastStatus = BOOT_STATUS_SUCCESS;
        
        /* Calculate and store CRC */
        header->crc = Boot_CalculateCRC32((uint8_t*)header, 
                                           sizeof(boot_log_header_t) - sizeof(uint32_t));
    }
    
    /* Load recovery context */
    recoveryContext = header->recoveryContext;
    
    return 0;
}

/**
 * @brief Write a new entry to the boot log
 * @param status Boot status code
 * @param errorDetails Additional error details
 * @return Status code
 */
static uint32_t Boot_WriteLogEntry(uint8_t status, uint16_t errorDetails) {
    boot_log_header_t* header = Boot_GetLogHeader();
    boot_log_entry_t* entries = Boot_GetLogEntries();
    uint32_t index;
    
    /* Update entry index */
    if (header->entries < MAX_BOOT_LOG_ENTRIES) {
        index = header->entries++;
    } else {
        index = header->lastEntryIndex;
        header->lastEntryIndex = (header->lastEntryIndex + 1) % MAX_BOOT_LOG_ENTRIES;
    }
    
    /* Write log entry */
    entries[index].timestamp = Boot_GetTimestamp();
    entries[index].status = status;
    entries[index].attempts = recoveryContext.consecutiveFailures;
    entries[index].errorDetails = errorDetails;
    
    /* Update recovery context */
    recoveryContext.lastStatus = status;
    
    if (status != BOOT_STATUS_SUCCESS && status != BOOT_STATUS_RECOVERY_ACTIVE) {
        recoveryContext.consecutiveFailures++;
    } else {
        recoveryContext.consecutiveFailures = 0;
    }
    
    header->recoveryContext = recoveryContext;
    
    /* Update CRC */
    header->crc = Boot_CalculateCRC32((uint8_t*)header, 
                                       sizeof(boot_log_header_t) - sizeof(uint32_t));
    
    return 0;
}

/**
 * @brief Initialize boot recovery system
 * @return Status code
 */
uint32_t Boot_RecoveryInit(void) {
    /* Initialize boot log */
    return Boot_InitializeLog();
}

/**
 * @brief Log boot status and handle recovery if needed
 * @param status Boot status code
 * @param errorDetails Additional error information
 * @return Action to take (continue or reset)
 */
uint32_t Boot_LogStatus(uint8_t status, uint16_t errorDetails) {
    /* Log boot status */
    Boot_WriteLogEntry(status, errorDetails);
    
    /* Check if recovery is needed */
    if (status != BOOT_STATUS_SUCCESS) {
        if (recoveryContext.consecutiveFailures >= MAX_BOOT_ATTEMPTS) {
            /* Too many failures, attempt factory reset */
            recoveryContext.recoveryMode = 1;
            Boot_WriteLogEntry(BOOT_STATUS_FACTORY_RESET, 0);
            return Boot_LoadFallbackImage();
        } else if (recoveryContext.consecutiveFailures > 1) {
            /* Multiple failures, attempt recovery */
            return Boot_AttemptRecovery();
        }
    }
    
    return 0; /* Continue normal operation */
}

/**
 * @brief Attempt recovery from boot failure
 * @return Status code
 */
uint32_t Boot_AttemptRecovery(void) {
    /* Visual indicator for recovery mode */
    for (uint32_t i = 0; i < 10; i++) {
        Siul2_Dio_Ip_TogglePins(LED_RED_PORT, (1U << LED_RED_PIN));
        Siul2_Dio_Ip_TogglePins(LED_GREEN_PORT, (1U << LED_GREEN_PIN));
        
        /* Delay */
        volatile uint32_t delay;
        for (delay = 0; delay < 500000; delay++) {}
    }
    
    /* Log recovery attempt */
    Boot_WriteLogEntry(BOOT_STATUS_RECOVERY_ACTIVE, 0);
    
    /* Try alternative boot options based on error type */
    if (recoveryContext.lastStatus == BOOT_STATUS_SIGNATURE_FAILURE || 
        recoveryContext.lastStatus == BOOT_STATUS_KEY_FAILURE) {
        return Boot_LoadFallbackImage();
    }
    
    /* Default recovery - reset the system */
    /* In a real implementation, use an MCU-specific reset function */
    volatile uint32_t* resetReg = (volatile uint32_t*)0xE000ED0C;
    *resetReg = 0x5FA0004;
    
    return 0;
}

/**
 * @brief Load fallback firmware image
 * @return Status code
 */
uint32_t Boot_LoadFallbackImage(void) {
    /* Visual indicator for fallback image load */
    for (uint32_t i = 0; i < 5; i++) {
    	Siul2_Dio_Ip_WritePin(LED_RED_PORT, (1U << LED_RED_PIN), 1);
    	Siul2_Dio_Ip_WritePin(LED_GREEN_PORT, (1U << LED_GREEN_PIN), 0);
        
        /* Delay */
        volatile uint32_t delay;
        for (delay = 0; delay < 500000; delay++) {}
        
        Siul2_Dio_Ip_WritePin(LED_RED_PORT, (1U << LED_RED_PIN), 0);
        Siul2_Dio_Ip_WritePin(LED_GREEN_PORT, (1U << LED_GREEN_PIN), 1);
        
        for (delay = 0; delay < 500000; delay++) {}
    }
    
    /* In a real implementation, copy fallback image to boot area
       or configure system to boot from fallback location */
    
    /* Reset the system to boot from fallback image */
    volatile uint32_t* resetReg = (volatile uint32_t*)0xE000ED0C;
    *resetReg = 0x5FA0004;
    
    return 0;
}

/**
 * @brief Get a timestamp for logging
 * @return Timestamp value (seconds since arbitrary epoch)
 */
uint32_t Boot_GetTimestamp(void) {
    /* In a real implementation, use an RTC or other time source */
    static uint32_t timestamp = 0;
    return timestamp++;
}

/**
 * @brief Get pointer to current recovery context
 * @return Pointer to boot recovery context structure
 */
boot_recovery_ctx_t* Boot_GetRecoveryContext(void)
{
    /* Return pointer to the recovery context in the boot log header */
    boot_log_header_t* header = Boot_GetLogHeader();

    /* Make sure boot log is initialized */
    if (header->magic != BOOT_LOG_MAGIC) {
        Boot_InitializeLog();
    }

    return &(header->recoveryContext);
}

/**
 * @brief Increment boot failure counter
 * @return New failure count
 */
uint32_t Boot_IncrementFailureCount(void)
{
    /* Get pointer to boot log header */
    boot_log_header_t* header = Boot_GetLogHeader();

    /* Check if boot log is initialized */
    if (header->magic != BOOT_LOG_MAGIC) {
        Boot_InitializeLog();
        header = Boot_GetLogHeader();
    }

    /* Increment consecutive failures counter */
    header->recoveryContext.consecutiveFailures++;
    recoveryContext.consecutiveFailures = header->recoveryContext.consecutiveFailures;

    /* Update CRC */
    header->crc = Boot_CalculateCRC32((uint8_t*)header,
                                      sizeof(boot_log_header_t) - sizeof(uint32_t));

    /* Log the boot failure increment */
    Boot_WriteLogEntry(BOOT_STATUS_BOOT_FAILURE,
                      (uint16_t)header->recoveryContext.consecutiveFailures);

    return header->recoveryContext.consecutiveFailures;
}
