/**
 * @file update_manager.h
 * @brief Firmware update management for S32K344 secure boot
 * @date 2025-05-07
 * @author SignalSlinger
 */

#ifndef UPDATE_MANAGER_H_
#define UPDATE_MANAGER_H_

#include <stdint.h>

/* Update status codes */
#define UPDATE_STATUS_SUCCESS       0x00000000
#define UPDATE_STATUS_FAILURE       0x00000001
#define UPDATE_STATUS_INVALID       0x00000002
#define UPDATE_STATUS_IN_PROGRESS   0x00000003
#define UPDATE_STATUS_READY         0x00000004
#define UPDATE_STATUS_NONE          0x00000005
#define UPDATE_STATUS_FLASH_ERROR   0x00000006

/* Update metadata structure */
#define UPDATE_METADATA_MAGIC       0x55504454  /* "UPDT" */
#define UPDATE_METADATA_VERSION     0x00010000  /* v1.0 */

/* Update locations */
#define UPDATE_STORAGE_ADDR         0x00500000  /* Update storage area */
#define UPDATE_STORAGE_SIZE         0x00100000  /* 1MB for update storage */
#define UPDATE_METADATA_ADDR        0x00600000  /* Update metadata */
#define UPDATE_METADATA_SIZE        0x00001000  /* 4KB */

/* Update metadata structure */
typedef struct {
    uint32_t magic;              /* Magic number to identify valid metadata */
    uint32_t version;            /* Metadata structure version */
    uint32_t updateSize;         /* Size of update in bytes */
    uint32_t updateCrc;          /* CRC-32 of update data */
    uint32_t targetVersion[4];   /* Target version (major,minor,patch,build) */
    uint32_t sourceVersion[4];   /* Source version (major,minor,patch,build) */
    uint32_t flags;              /* Update flags */
    uint32_t status;             /* Update status */
    uint32_t signatureOffset;    /* Offset to signature data */
    uint32_t signatureSize;      /* Size of signature data */
    uint32_t reserved[6];        /* Reserved for future use */
    uint32_t metadataCrc;        /* CRC-32 of this metadata structure (except this field) */
} update_metadata_t;

/**
 * @brief Initialize update manager
 * @return Status code
 */
uint32_t Update_Init(void);

/**
 * @brief Check if an update is pending
 * @return 1 if update is pending, 0 otherwise
 */
uint32_t Update_IsPending(void);

/**
 * @brief Apply pending updates
 * @return Status code
 */
uint32_t Update_ApplyPendingUpdates(void);

/**
 * @brief Store backup after successful update
 * @return Status code
 */
uint32_t Update_StoreBackupAfterUpdate(void);

/**
 * @brief Verify update integrity
 * @return Status code
 */
uint32_t Update_VerifyIntegrity(void);

/**
 * @brief Get update metadata
 * @return Pointer to metadata structure (read-only)
 */
const update_metadata_t* Update_GetMetadata(void);

/**
 * @brief Stage update for next boot
 * @param updateData Pointer to update data
 * @param updateSize Size of update data
 * @param signature Pointer to signature data
 * @param signatureSize Size of signature data
 * @return Status code
 */
uint32_t Update_StageUpdate(const uint8_t* updateData, uint32_t updateSize, 
                           const uint8_t* signature, uint32_t signatureSize);

/**
 * @brief Cancel pending update
 * @return Status code
 */
uint32_t Update_CancelPending(void);

#endif /* UPDATE_MANAGER_H_ */