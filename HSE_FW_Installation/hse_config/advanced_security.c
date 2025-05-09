/*
 * @file advanced_security.c
 * @brief Advanced security features implementation
 *
 *  Created on: May 2, 2025
 */

#include "advanced_security.h"
#include "hse_config.h"
#include "boot_recovery.h" /* For boot logging */
#include "Siul2_Port_Ip.h" // For Port initialization
#include "Siul2_Dio_Ip.h"  // For LED control

/* Internal variables */
static const uint32_t VERSION_MAGIC = 0x56455253; /* "VERS" */
static firmware_version_t currentVersion = {
    FIRMWARE_VERSION_MAJOR,
    FIRMWARE_VERSION_MINOR,
    FIRMWARE_VERSION_PATCH,
    FIRMWARE_VERSION_BUILD
};

/* Reference hash for critical memory regions (calculated at build time) */
static const uint8_t criticalRegionReferenceHash[32] = {
    /* This would be filled with the actual hash of the protected region */
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20
};

/* Flag for periodic integrity checks */
static volatile uint8_t integrityCheckEnabled = 0;

/**
 * @brief Calculate CRC-32 for data integrity
 * @param data Pointer to data
 * @param length Length of data in bytes
 * @return CRC-32 value
 */
static uint32_t Security_CalculateCRC32(const uint8_t *data, uint32_t length) {
    /* Reuse the CRC function from boot_recovery.c or implement it here */
    uint32_t crc = 0xFFFFFFFF;
    const uint32_t crc32_table[256] = {
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
        /* ... Add complete CRC table or use a CRC library ... */
        0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
    };
    
    for (uint32_t i = 0; i < length; i++) {
        crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    
    return crc ^ 0xFFFFFFFF;
}

/**
 * @brief Get pointer to version history in non-volatile memory
 * @return Pointer to version history structure
 */
static version_history_t* Security_GetVersionHistory(void) {
    return (version_history_t*)VERSION_STORAGE_ADDRESS;
}

/**
 * @brief Initialize the version history if not already initialized
 * @return Status code
 */
static uint32_t Security_InitVersionHistory(void) {
    version_history_t* history = Security_GetVersionHistory();
    
    /* Check if version history is already initialized */
    if (history->magic == VERSION_MAGIC) {
        /* Verify CRC */
        uint32_t calculatedCRC = Security_CalculateCRC32((uint8_t*)history, 
                                                         sizeof(version_history_t) - sizeof(uint32_t));
        if (calculatedCRC != history->crc) {
            /* CRC mismatch, history is corrupted */
            /* Re-initialize history */
        }
    }
    
    /* Initialize version history if needed */
    if (history->magic != VERSION_MAGIC) {
        /* Mock implementation - in production, use proper NVM write */
        history->magic = VERSION_MAGIC;
        history->currentVersion = currentVersion;
        history->minimumVersion.major = FIRMWARE_VERSION_MAJOR;
        history->minimumVersion.minor = FIRMWARE_VERSION_MINOR;
        history->minimumVersion.patch = FIRMWARE_VERSION_PATCH;
        history->minimumVersion.build = FIRMWARE_VERSION_BUILD;
        history->updateCounter = 0;
        history->lastUpdateTime = Boot_GetTimestamp();
        
        /* Calculate and store CRC */
        history->crc = Security_CalculateCRC32((uint8_t*)history, 
                                                sizeof(version_history_t) - sizeof(uint32_t));
    }
    
    return 0;
}

/**
 * @brief Convert version structure to uint32 for comparison
 * @param version Version structure
 * @return 32-bit version value
 */
static uint32_t Security_VersionToUint32(firmware_version_t version) {
    return ((uint32_t)version.major << 24) | 
           ((uint32_t)version.minor << 16) | 
           ((uint32_t)version.patch << 8) | 
           (uint32_t)version.build;
}

/**
 * @brief Initialize anti-rollback protection
 * @return Status code
 */
uint32_t Security_InitAntiRollback(void) {
    return Security_InitVersionHistory();
}

/**
 * @brief Check firmware version against anti-rollback policy
 * @param version Firmware version to check
 * @return Status (0=success, non-zero=error)
 */
uint32_t Security_CheckVersion(firmware_version_t version) {
    version_history_t* history = Security_GetVersionHistory();
    
    /* Convert versions to uint32 for easier comparison */
    uint32_t versionValue = Security_VersionToUint32(version);
    uint32_t minVersionValue = Security_VersionToUint32(history->minimumVersion);
    
    /* Check if version is greater than or equal to minimum allowed */
    if (versionValue < minVersionValue) {
        /* Version is too old, violates anti-rollback policy */
        return 1;
    }
    
    return 0;
}

/**
 * @brief Update stored firmware version after successful update
 * @param newVersion New firmware version
 * @return Status code
 */
uint32_t Security_UpdateVersion(firmware_version_t newVersion) {
    version_history_t* history = Security_GetVersionHistory();
    
    /* Check if new version is valid */
    if (Security_CheckVersion(newVersion) != 0) {
        return 1;
    }
    
    /* Update version history */
    history->currentVersion = newVersion;
    history->updateCounter++;
    history->lastUpdateTime = Boot_GetTimestamp();
    
    /* If new version is higher, update minimum version as well */
    uint32_t newVersionValue = Security_VersionToUint32(newVersion);
    uint32_t minVersionValue = Security_VersionToUint32(history->minimumVersion);
    
    if (newVersionValue > minVersionValue) {
        history->minimumVersion = newVersion;
    }
    
    /* Update CRC */
    history->crc = Security_CalculateCRC32((uint8_t*)history, 
                                            sizeof(version_history_t) - sizeof(uint32_t));
    
    return 0;
}

/**
 * @brief Configure debug security policy
 * @param config Debug configuration
 * @return Status code
 */
uint32_t Security_ConfigureDebug(debug_config_t config) {
    /* In a real implementation, this would configure hardware debug security */
    /* For S32K344, this would program the appropriate SIU/WKPU registers */
    
    /* Example: Configure debug based on level */
    switch (config.debugLevel) {
        case DEBUG_LEVEL_DISABLED:
            /* Disable all debug interfaces */
            /* Example: SIU->MISCELLANEOUS_CONTROL_REGISTER &= ~(SIU_MCR_DBFEN) */
            break;
        
        case DEBUG_LEVEL_LIMITED:
            /* Configure limited debug access */
            /* Example: Set up password protection, memory protection, etc. */
            break;
            
        case DEBUG_LEVEL_FULL:
            /* Enable full debug access */
            /* Example: SIU->MISCELLANEOUS_CONTROL_REGISTER |= SIU_MCR_DBFEN */
            break;
            
        default:
            return 1; /* Invalid level */
    }
    
    return 0;
}

/**
 * @brief Calculate SHA-256 hash (simplified example)
 * @param data Pointer to data
 * @param length Length of data in bytes
 * @param hash Output hash (must be 32 bytes)
 */
static void Security_CalculateSHA256(const uint8_t* data, uint32_t length, uint8_t* hash) {
    /* In a real implementation, use HSE crypto services or software SHA-256 */
    
    /* Simplified example (not a real SHA-256) */
    for (uint32_t i = 0; i < 32; i++) {
        hash[i] = 0;
    }
    
    for (uint32_t i = 0; i < length; i++) {
        hash[i % 32] ^= data[i];
    }
}

/**
 * @brief Perform runtime integrity check of critical code regions
 * @return Status code (0=integrity verified, non-zero=integrity failure)
 */
uint32_t Security_CheckIntegrity(void) {
    uint8_t calculatedHash[32];
    
    /* Calculate hash of critical region */
    Security_CalculateSHA256((const uint8_t*)CRITICAL_REGION_START, 
                              CRITICAL_REGION_END - CRITICAL_REGION_START,
                              calculatedHash);
    
    /* Compare with reference hash */
    for (uint32_t i = 0; i < 32; i++) {
        if (calculatedHash[i] != criticalRegionReferenceHash[i]) {
            /* Integrity check failed */
            return 1;
        }
    }
    
    return 0; /* Integrity verified */
}

/**
 * @brief Periodic integrity check (would be called from a timer interrupt)
 */
void Security_PeriodicIntegrityCheck(void) {
    if (integrityCheckEnabled) {
        if (Security_CheckIntegrity() != 0) {
            /* Integrity check failed - handle error */
            /* Log integrity failure */
            Boot_LogStatus(BOOT_STATUS_MEMORY_FAILURE, 0);
            
            /* Take appropriate action (reset, enter safe mode, etc.) */
            /* For demonstration, we'll just toggle LEDs */
            Siul2_Dio_Ip_TogglePins(LED_RED_PORT, (1U << LED_RED_PIN));
        }
    }
}

/**
 * @brief Start periodic runtime integrity checks
 * @return Status code
 */
uint32_t Security_StartIntegrityMonitor(void) {
    /* In a real implementation, configure a timer to periodically call
       Security_PeriodicIntegrityCheck() */
    
    /* Enable integrity checks */
    integrityCheckEnabled = 1;
    
    return 0;
}

/**
 * @brief Get current firmware version
 * @return Firmware version structure
 */
firmware_version_t Security_GetFirmwareVersion(void) {
    return currentVersion;
}
