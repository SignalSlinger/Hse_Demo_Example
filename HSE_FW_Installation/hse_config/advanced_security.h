/**
 * @file advanced_security.h
 * @brief Advanced security features for S32K344 secure boot
 */

#ifndef ADVANCED_SECURITY_H
#define ADVANCED_SECURITY_H

#include "Mcal.h"

/* Version information structure */
#define FIRMWARE_VERSION_MAJOR    (1U)
#define FIRMWARE_VERSION_MINOR    (0U)
#define FIRMWARE_VERSION_PATCH    (0U)
#define FIRMWARE_VERSION_BUILD    (1U)

/* Anti-rollback configuration */
#define VERSION_STORAGE_ADDRESS   (0x10081000U) /* Adjust based on memory map */
#define MIN_ALLOWED_VERSION       ((FIRMWARE_VERSION_MAJOR << 24) | \
                                  (FIRMWARE_VERSION_MINOR << 16) | \
                                  (FIRMWARE_VERSION_PATCH << 8) | \
                                  (FIRMWARE_VERSION_BUILD))

/* Debug security levels */
#define DEBUG_LEVEL_DISABLED      (0U) /* Debug fully disabled */
#define DEBUG_LEVEL_LIMITED       (1U) /* Limited debug (no memory access) */
#define DEBUG_LEVEL_FULL          (2U) /* Full debug access */

/* Integrity check parameters */
#define RUNTIME_CHECK_INTERVAL_MS (5000U) /* Run integrity check every 5 seconds */
#define CRITICAL_REGION_START     (0x00400000U) /* Adjust based on memory map */
#define CRITICAL_REGION_END       (0x00410000U) /* Adjust based on memory map */

/**
 * @brief Firmware version structure
 */
typedef struct {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    uint8_t build;
} firmware_version_t;

/**
 * @brief Version history structure for anti-rollback
 */
typedef struct {
    uint32_t magic;
    firmware_version_t currentVersion;
    firmware_version_t minimumVersion; /* Lowest version allowed to boot */
    uint32_t updateCounter;
    uint32_t lastUpdateTime;
    uint32_t crc;
} version_history_t;

/**
 * @brief Debug configuration structure
 */
typedef struct {
    uint8_t debugLevel;
    uint8_t passwordProtected;
    uint8_t timeoutEnabled;
    uint8_t reserved;
    uint32_t debugTimeout; /* Debug access timeout in seconds (0=no timeout) */
} debug_config_t;

/**
 * @brief Initialize anti-rollback protection
 * @return Status code
 */
uint32_t Security_InitAntiRollback(void);

/**
 * @brief Check firmware version against anti-rollback policy
 * @param version Firmware version to check
 * @return Status (0=success, non-zero=error)
 */
uint32_t Security_CheckVersion(firmware_version_t version);

/**
 * @brief Update stored firmware version after successful update
 * @param newVersion New firmware version
 * @return Status code
 */
uint32_t Security_UpdateVersion(firmware_version_t newVersion);

/**
 * @brief Configure debug security policy
 * @param config Debug configuration
 * @return Status code
 */
uint32_t Security_ConfigureDebug(debug_config_t config);

/**
 * @brief Perform runtime integrity check of critical code regions
 * @return Status code (0=integrity verified, non-zero=integrity failure)
 */
uint32_t Security_CheckIntegrity(void);

/**
 * @brief Start periodic runtime integrity checks
 * @return Status code
 */
uint32_t Security_StartIntegrityMonitor(void);

/**
 * @brief Get current firmware version
 * @return Firmware version structure
 */
firmware_version_t Security_GetFirmwareVersion(void);

#endif /* ADVANCED_SECURITY_H */
