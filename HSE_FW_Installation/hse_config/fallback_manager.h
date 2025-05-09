/**
 * @file fallback_manager.h
 * @brief Fallback firmware management for S32K344 secure boot
 * @date 2025-05-07
 * @author SignalSlinger
 */

#ifndef FALLBACK_MANAGER_H_
#define FALLBACK_MANAGER_H_

#include <stdint.h>

/* Fallback firmware metadata structure */
#define FALLBACK_METADATA_MAGIC   0x46414C42   /* "FALB" */
#define FALLBACK_METADATA_VERSION 0x00010000   /* v1.0 */

/* Fallback firmware locations */
#define FALLBACK_FIRMWARE_ADDR    				0x006E1000
#define FALLBACK_FIRMWARE_SIZE    				0x0001E800  /* 121KB */
#define FALLBACK_FIRMWARE_SIGNATURE_ADDR 		0x006FF800  /* Last 3KB of fallback region */
#define FALLBACK_FIRMWARE_SIGNATURE_SIZE  		0x00000800  /* 2KB for signature */
#define FALLBACK_METADATA_ADDR    				0x006E0000	/* Match __FALLBACK_META_START from linker */

/* Application firmware locations */
#define APP_FIRMWARE_ADDR         0x00400000  /* Main application location */
#define APP_FIRMWARE_SIZE         0x002E0000  /* 2.875MB - matches int_pflash length */

/* Fallback firmware status codes */
#define FALLBACK_STATUS_SUCCESS   0x00000000
#define FALLBACK_STATUS_FAILURE   0x00000001
#define FALLBACK_STATUS_INVALID   0x00000002
#define FALLBACK_STATUS_FLASH_ERR 0x00000003

/**
 * @brief Fallback firmware metadata structure
 */
typedef struct {
    uint32_t magic;             /* Magic number to identify valid metadata */
    uint32_t version;           /* Metadata structure version */
    uint32_t firmwareSize;      /* Size of fallback firmware in bytes */
    uint32_t firmwareCrc;       /* CRC-32 of fallback firmware */
    uint32_t creationTime;      /* Timestamp when fallback was created */
    uint32_t updateCount;       /* Number of times this fallback has been used */
    uint8_t  firmwareVersion[4];/* Version of the fallback firmware (major,minor,patch,build) */
    uint8_t  reserved[8];       /* Reserved for future use */
    uint32_t metadataCrc;       /* CRC-32 of this metadata structure (except this field) */
} fallback_metadata_t;

/**
 * @brief Initialize fallback firmware manager
 * @return Status code
 */
uint32_t Fallback_Init(void);

/**
 * @brief Check if valid fallback firmware exists
 * @return 1 if valid fallback exists, 0 otherwise
 */
uint32_t Fallback_IsValid(void);

/**
 * @brief Store current firmware as fallback
 * @return Status code
 */
uint32_t Fallback_StoreCurrentFirmware(void);

/**
 * @brief Recover using fallback firmware
 * @return Status code
 */
uint32_t Fallback_RecoverMainFirmware(void);

/**
 * @brief Get fallback firmware metadata
 * @return Pointer to metadata structure (read-only)
 */
const fallback_metadata_t* Fallback_GetMetadata(void);

/**
 * @brief Verify fallback firmware integrity
 * @return Status code
 */
uint32_t Fallback_VerifyIntegrity(void);

/**
 * @brief Boot from fallback firmware without copying
 * @return Does not return if successful
 */
void Fallback_JumpToFallbackFirmware(void);

#endif /* FALLBACK_MANAGER_H_ */
