/**
 * @file boot_recovery.h
 * @brief Boot recovery mechanisms for S32K344 secure boot
 */

#ifndef BOOT_RECOVERY_H
#define BOOT_RECOVERY_H

#include "Mcal.h"

/* Boot status codes */
#define BOOT_STATUS_SUCCESS                  0x0000
#define BOOT_STATUS_HSE_FAILURE              0x0001
#define BOOT_STATUS_SIGNATURE_FAILURE        0x0002
#define BOOT_STATUS_RUNTIME_FAILURE          0x0003
#define BOOT_STATUS_RECOVERY_ACTIVE          0x0004
#define BOOT_STATUS_FACTORY_RESET            0x0005
#define BOOT_STATUS_ROLLBACK_DETECTED        0x0006
#define BOOT_STATUS_MEMORY_FAILURE           0x0007
#define BOOT_STATUS_BOOT_FAILURE             0x0008
#define BOOT_STATUS_WARNING                  0x0009
#define BOOT_STATUS_KEY_FAILURE              0x000A
#define BOOT_STATUS_FALLBACK_CREATED         0x0050
#define BOOT_STATUS_FALLBACK_UPDATED         0x0051
#define BOOT_STATUS_UPDATE_READY             0x0060
#define BOOT_STATUS_UPDATE_SUCCESS           0x0061
#define BOOT_STATUS_UPDATE_FAILURE           0x0062
#define BOOT_STATUS_UPDATE_VERIFY_FAILURE    0x0063
#define BOOT_STATUS_UPDATE_CANCELLED         0x0064
#define BOOT_STATUS_SIGNATURE_VALID          0x0070
#define BOOT_STATUS_METADATA_INVALID         0x0071

/* Maximum boot attempts before factory reset */
#define MAX_BOOT_ATTEMPTS                    (3U)

/* Boot log storage in non-volatile memory */
#define BOOT_LOG_ADDRESS                     (0x10080000U)
#define MAX_BOOT_LOG_ENTRIES                 (10U)

/* Fallback firmware information */
#define FALLBACK_FIRMWARE_ADDRESS            (0x006E1000U)
#define FALLBACK_FIRMWARE_SIGNATURE_ADDRESS  (0x006FF800U)

/* Boot log entry structure */
typedef struct {
    uint32_t timestamp;
    uint8_t status;
    uint8_t attempts;
    uint16_t errorDetails;
} boot_log_entry_t;

/* Boot recovery context */
typedef struct {
    uint8_t consecutiveFailures;
    uint8_t recoveryMode;
    uint8_t lastStatus;
    uint8_t reserved;
} boot_recovery_ctx_t;

/* Boot log header */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t entries;
    uint32_t lastEntryIndex;
    boot_recovery_ctx_t recoveryContext;
    uint32_t crc;
} boot_log_header_t;

/**
 * @brief Initialize boot recovery system
 * @return Status code
 */
uint32_t Boot_RecoveryInit(void);

/**
 * @brief Log boot status and handle recovery if needed
 * @param status Boot status code
 * @param errorDetails Additional error information
 * @return Action to take (continue or reset)
 */
uint32_t Boot_LogStatus(uint8_t status, uint16_t errorDetails);

/**
 * @brief Attempt recovery from boot failure
 * @return Status code
 */
uint32_t Boot_AttemptRecovery(void);

/**
 * @brief Load fallback firmware image
 * @return Status code
 */
uint32_t Boot_LoadFallbackImage(void);

/**
 * @brief Get a timestamp for logging
 * @return Timestamp value (seconds since arbitrary epoch)
 */
uint32_t Boot_GetTimestamp(void);

/**
 * @brief Get pointer to current recovery context
 * @return Pointer to boot recovery context structure
 */
boot_recovery_ctx_t* Boot_GetRecoveryContext(void);

uint32_t Boot_IncrementFailureCount(void);

#endif /* BOOT_RECOVERY_H */
