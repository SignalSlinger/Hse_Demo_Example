/**
 * @file update_manager.c
 * @brief Firmware update management implementation
 * @date 2025-05-07
 * @author SignalSlinger
 */

#include "update_manager.h"
#include "fallback_manager.h"
#include "flash_programming.h"
#include "boot_recovery.h"
#include "hse_config.h"
#include "Siul2_Port_Ip.h" // For Port initialization
#include "Siul2_Dio_Ip.h"  // For LED control
#include <string.h>
#include "hse_config.h"

/* Static function prototypes */
static uint32_t Update_CalculateCRC32(const uint8_t* data, uint32_t length);
static uint32_t Update_ValidateMetadata(const update_metadata_t* metadata);
static uint32_t Update_UpdateMetadata(const update_metadata_t* metadata);
static uint32_t Update_ApplyUpdate(void);
static uint32_t Update_VerifyUpdate(void);

/**
 * @brief Calculate CRC-32 checksum
 * @param data Pointer to data
 * @param length Length of data in bytes
 * @return CRC-32 value
 */
static uint32_t Update_CalculateCRC32(const uint8_t* data, uint32_t length) {
    uint32_t crc = 0xFFFFFFFF;
    const uint32_t crc32_table[16] = {
        0x00000000, 0x1DB71064, 0x3B6E20C8, 0x26D930AC,
        0x76DC4190, 0x6B6B51F4, 0x4DB26158, 0x5005713C,
        0xEDB88320, 0xF00F9344, 0xD6D6A3E8, 0xCB61B38C,
        0x9B64C2B0, 0x86D3D2D4, 0xA00AE278, 0xBDBDF21C
    };
    
    for (uint32_t i = 0; i < length; i++) {
        crc = (crc >> 4) ^ crc32_table[(crc ^ (data[i] >> 0)) & 0x0F];
        crc = (crc >> 4) ^ crc32_table[(crc ^ (data[i] >> 4)) & 0x0F];
    }
    
    return ~crc;
}

/**
 * @brief Get pointer to update metadata
 * @return Pointer to metadata structure
 */
static update_metadata_t* Update_GetMetadataPtr(void) {
    return (update_metadata_t*)UPDATE_METADATA_ADDR;
}

/**
 * @brief Validate metadata structure
 * @param metadata Pointer to metadata structure
 * @return Status code
 */
static uint32_t Update_ValidateMetadata(const update_metadata_t* metadata) {
    /* Check magic number */
    if (metadata->magic != UPDATE_METADATA_MAGIC) {
        return UPDATE_STATUS_INVALID;
    }
    
    /* Check metadata version */
    if ((metadata->version & 0xFFFF0000) != 
        (UPDATE_METADATA_VERSION & 0xFFFF0000)) {
        return UPDATE_STATUS_INVALID;
    }
    
    /* Calculate and verify metadata CRC */
    uint32_t calculatedCrc = Update_CalculateCRC32(
        (const uint8_t*)metadata, 
        sizeof(update_metadata_t) - sizeof(uint32_t));
    
    if (calculatedCrc != metadata->metadataCrc) {
        return UPDATE_STATUS_INVALID;
    }
    
    /* Check update size */
    if (metadata->updateSize == 0 || metadata->updateSize > UPDATE_STORAGE_SIZE) {
        return UPDATE_STATUS_INVALID;
    }
    
    return UPDATE_STATUS_SUCCESS;
}

/**
 * @brief Update metadata in flash
 * @param metadata Pointer to metadata structure to store
 * @return Status code
 */
static uint32_t Update_UpdateMetadata(const update_metadata_t* metadata) {
    /* Calculate metadata CRC - create a copy since we need to modify it */
    update_metadata_t metadataCopy = *metadata;
    
    metadataCopy.metadataCrc = Update_CalculateCRC32(
        (const uint8_t*)&metadataCopy, 
        sizeof(update_metadata_t) - sizeof(uint32_t));
    
    /* Erase metadata sector */
    uint32_t status = Flash_EraseSector(UPDATE_METADATA_ADDR, UPDATE_METADATA_SIZE);
    if (status != 0) {
        return UPDATE_STATUS_FLASH_ERROR;
    }
    
    /* Program new metadata */
    status = Flash_Program(
        UPDATE_METADATA_ADDR,
        (const uint8_t*)&metadataCopy,
        sizeof(update_metadata_t));
    
    if (status != 0) {
        return UPDATE_STATUS_FLASH_ERROR;
    }
    
    /* Invalidate cache */
    Flash_Cache_Invalidate();
    
    return UPDATE_STATUS_SUCCESS;
}

/**
 * @brief Initialize update manager
 * @return Status code
 */
uint32_t Update_Init(void) {
    update_metadata_t* metadata = Update_GetMetadataPtr();
    
    /* Check if metadata is already initialized */
    if (metadata->magic == UPDATE_METADATA_MAGIC) {
        /* Validate metadata */
        return Update_ValidateMetadata(metadata);
    }
    
    /* Metadata not initialized - create default metadata */
    update_metadata_t newMetadata;
    memset(&newMetadata, 0, sizeof(update_metadata_t));
    newMetadata.magic = UPDATE_METADATA_MAGIC;
    newMetadata.version = UPDATE_METADATA_VERSION;
    newMetadata.status = UPDATE_STATUS_NONE;
    
    /* Update metadata in flash */
    return Update_UpdateMetadata(&newMetadata);
}

/**
 * @brief Check if an update is pending
 * @return 1 if update is pending, 0 otherwise
 */
uint32_t Update_IsPending(void) {
    update_metadata_t* metadata = Update_GetMetadataPtr();
    
    /* Check if metadata is valid */
    if (Update_ValidateMetadata(metadata) != UPDATE_STATUS_SUCCESS) {
        return 0;
    }
    
    /* Check if update is ready */
    return (metadata->status == UPDATE_STATUS_READY) ? 1 : 0;
}

/**
 * @brief Apply pending updates
 * @return Status code
 */
uint32_t Update_ApplyPendingUpdates(void) {
    uint32_t status;
    update_metadata_t* metadata = Update_GetMetadataPtr();
    
    /* Check if update is pending */
    if (!Update_IsPending()) {
        return UPDATE_STATUS_NONE;
    }
    
    /* Set update status to in progress */
    metadata->status = UPDATE_STATUS_IN_PROGRESS;
    status = Update_UpdateMetadata(metadata);
    if (status != UPDATE_STATUS_SUCCESS) {
        return status;
    }
    
    /* Visual indicator for update process */
    Siul2_Dio_Ip_WritePin(LED_GREEN_PORT, (1U << LED_GREEN_PIN), 1);
    
    /* Apply the update */
    status = Update_ApplyUpdate();
    if (status != UPDATE_STATUS_SUCCESS) {
        /* Update failed, log the error */
        Boot_LogStatus(BOOT_STATUS_UPDATE_FAILURE, (uint16_t)status);
        
        /* Reset the update status */
        metadata->status = UPDATE_STATUS_FAILURE;
        Update_UpdateMetadata(metadata);
        
        /* Visual indicator for update failure */
        Siul2_Dio_Ip_WritePin(LED_GREEN_PORT, (1U << LED_GREEN_PIN), 0);
        Siul2_Dio_Ip_WritePin(LED_RED_PORT, (1U << LED_RED_PIN), 1);
        
        return status;
    }
    
    /* Verify the update */
    status = Update_VerifyUpdate();
    if (status != UPDATE_STATUS_SUCCESS) {
        /* Verification failed, log the error */
        Boot_LogStatus(BOOT_STATUS_UPDATE_VERIFY_FAILURE, (uint16_t)status);
        
        /* Reset the update status */
        metadata->status = UPDATE_STATUS_FAILURE;
        Update_UpdateMetadata(metadata);
        
        /* Visual indicator for verification failure */
        Siul2_Dio_Ip_WritePin(LED_GREEN_PORT, (1U << LED_GREEN_PIN), 0);
        Siul2_Dio_Ip_WritePin(LED_RED_PORT, (1U << LED_RED_PIN), 1);
        
        return status;
    }
    
    /* Store backup after successful update */
    status = Update_StoreBackupAfterUpdate();
    if (status != UPDATE_STATUS_SUCCESS) {
        /* Backup failed, log the warning (non-fatal) */
        Boot_LogStatus(BOOT_STATUS_WARNING, (uint16_t)status);
    }
    
    /* Update successful, update metadata */
    metadata->status = UPDATE_STATUS_SUCCESS;
    Update_UpdateMetadata(metadata);
    
    /* Log update success */
    Boot_LogStatus(BOOT_STATUS_UPDATE_SUCCESS, 0);
    
    /* Visual indicator for update success */
    Siul2_Dio_Ip_WritePin(LED_RED_PORT, (1U << LED_RED_PIN), 0);
    Siul2_Dio_Ip_WritePin(LED_GREEN_PORT, (1U << LED_GREEN_PIN), 1);
    
    /* Reset the device to boot with the new firmware */
    volatile uint32_t* resetReg = (volatile uint32_t*)0xE000ED0C;
    *resetReg = 0x5FA0004;
    
    /* Should not reach here */
    return UPDATE_STATUS_SUCCESS;
}

/**
 * @brief Apply the update to main firmware
 * @return Status code
 */
static uint32_t Update_ApplyUpdate(void) {
    uint32_t status;
    update_metadata_t* metadata = Update_GetMetadataPtr();
    
    /* Erase application flash area */
    status = Flash_EraseSector(APP_FIRMWARE_ADDR, metadata->updateSize);
    if (status != 0) {
        return UPDATE_STATUS_FLASH_ERROR;
    }
    
    /* Copy update to application area */
    status = Flash_Program(
        APP_FIRMWARE_ADDR, 
        (const uint8_t*)UPDATE_STORAGE_ADDR,
        metadata->updateSize);
    
    if (status != 0) {
        return UPDATE_STATUS_FLASH_ERROR;
    }
    
    /* Update signature if present */
    if (metadata->signatureSize > 0 && metadata->signatureOffset > 0) {
        const uint8_t* signatureAddr = (const uint8_t*)(UPDATE_STORAGE_ADDR + metadata->signatureOffset);
        status = HSE_UpdateSignatures(signatureAddr, metadata->signatureSize);
        if (status != HSE_STATUS_SUCCESS) {
            return UPDATE_STATUS_FAILURE;
        }
    }
    
    /* Invalidate cache */
    Flash_Cache_Invalidate();
    
    return UPDATE_STATUS_SUCCESS;
}

/**
 * @brief Verify the applied update
 * @return Status code
 */
static uint32_t Update_VerifyUpdate(void) {
    update_metadata_t* metadata = Update_GetMetadataPtr();
    
    /* Calculate CRC of the installed firmware */
    uint32_t calculatedCrc = Update_CalculateCRC32(
        (const uint8_t*)APP_FIRMWARE_ADDR,
        metadata->updateSize);
    
    /* Verify CRC matches expected value */
    if (calculatedCrc != metadata->updateCrc) {
        return UPDATE_STATUS_FAILURE;
    }
    
    /* Verify signature if required */
    if (metadata->signatureSize > 0) {
        if (HSE_VerifyAppSignature() != HSE_STATUS_SUCCESS) {
            return UPDATE_STATUS_FAILURE;
        }
    }
    
    return UPDATE_STATUS_SUCCESS;
}

/**
 * @brief Store backup after successful update
 * @return Status code
 */
uint32_t Update_StoreBackupAfterUpdate(void) {
    uint32_t status;
    
    /* Initialize fallback manager */
    status = Fallback_Init();
    if (status != FALLBACK_STATUS_SUCCESS) {
        return UPDATE_STATUS_FAILURE;
    }
    
    /* Store current firmware as fallback */
    status = Fallback_StoreCurrentFirmware();
    if (status != FALLBACK_STATUS_SUCCESS) {
        return UPDATE_STATUS_FAILURE;
    }
    
    /* Verify fallback integrity */
    status = Fallback_VerifyIntegrity();
    if (status != FALLBACK_STATUS_SUCCESS) {
        return UPDATE_STATUS_FAILURE;
    }
    
    /* Verify fallback signature */
    if (HSE_VerifyFallbackSignature() != HSE_STATUS_SUCCESS) {
        return UPDATE_STATUS_FAILURE;
    }
    
    /* Log successful fallback update */
    Boot_LogStatus(BOOT_STATUS_FALLBACK_UPDATED, 0);
    
    return UPDATE_STATUS_SUCCESS;
}

/**
 * @brief Verify update integrity
 * @return Status code
 */
uint32_t Update_VerifyIntegrity(void) {
    update_metadata_t* metadata = Update_GetMetadataPtr();
    
    /* Validate metadata */
    if (Update_ValidateMetadata(metadata) != UPDATE_STATUS_SUCCESS) {
        return UPDATE_STATUS_INVALID;
    }
    
    /* Verify update data CRC */
    uint32_t calculatedCrc = Update_CalculateCRC32(
        (const uint8_t*)UPDATE_STORAGE_ADDR,
        metadata->updateSize);
    
    if (calculatedCrc != metadata->updateCrc) {
        return UPDATE_STATUS_FAILURE;
    }
    
    return UPDATE_STATUS_SUCCESS;
}

/**
 * @brief Get update metadata
 * @return Pointer to metadata structure (read-only)
 */
const update_metadata_t* Update_GetMetadata(void) {
    update_metadata_t* metadata = Update_GetMetadataPtr();
    
    /* Return a pointer to the metadata if it's valid */
    if (Update_ValidateMetadata(metadata) == UPDATE_STATUS_SUCCESS) {
        return metadata;
    } else {
        return NULL;
    }
}

/**
 * @brief Stage update for next boot
 * @param updateData Pointer to update data
 * @param updateSize Size of update data
 * @param signature Pointer to signature data
 * @param signatureSize Size of signature data
 * @return Status code
 */
uint32_t Update_StageUpdate(const uint8_t* updateData, uint32_t updateSize, 
                           const uint8_t* signature, uint32_t signatureSize) {
    uint32_t status;
    update_metadata_t newMetadata;
    
    /* Validate parameters */
    if (updateData == NULL || updateSize == 0 || updateSize > UPDATE_STORAGE_SIZE) {
        return UPDATE_STATUS_INVALID;
    }
    
    /* Initialize metadata */
    memset(&newMetadata, 0, sizeof(update_metadata_t));
    newMetadata.magic = UPDATE_METADATA_MAGIC;
    newMetadata.version = UPDATE_METADATA_VERSION;
    newMetadata.updateSize = updateSize;
    newMetadata.updateCrc = Update_CalculateCRC32(updateData, updateSize);
    newMetadata.status = UPDATE_STATUS_READY;
    
    /* Add signature information if provided */
    if (signature != NULL && signatureSize > 0) {
        newMetadata.signatureOffset = updateSize;
        newMetadata.signatureSize = signatureSize;
    }
    
    /* Erase update storage */
    status = Flash_EraseSector(UPDATE_STORAGE_ADDR, UPDATE_STORAGE_SIZE);
    if (status != 0) {
        return UPDATE_STATUS_FLASH_ERROR;
    }
    
    /* Store update data */
    status = Flash_Program(UPDATE_STORAGE_ADDR, updateData, updateSize);
    if (status != 0) {
        return UPDATE_STATUS_FLASH_ERROR;
    }
    
    /* Store signature if provided */
    if (signature != NULL && signatureSize > 0) {
        status = Flash_Program(
            UPDATE_STORAGE_ADDR + updateSize,
            signature,
            signatureSize);
        
        if (status != 0) {
            return UPDATE_STATUS_FLASH_ERROR;
        }
    }
    
    /* Update metadata */
    status = Update_UpdateMetadata(&newMetadata);
    if (status != UPDATE_STATUS_SUCCESS) {
        return status;
    }
    
    /* Log update ready */
    Boot_LogStatus(BOOT_STATUS_UPDATE_READY, 0);
    
    return UPDATE_STATUS_SUCCESS;
}

/**
 * @brief Cancel pending update
 * @return Status code
 */
uint32_t Update_CancelPending(void) {
    update_metadata_t* metadata = Update_GetMetadataPtr();
    
    /* Check if update is pending */
    if (!Update_IsPending()) {
        return UPDATE_STATUS_NONE;
    }
    
    /* Update metadata to cancel the update */
    metadata->status = UPDATE_STATUS_NONE;
    
    /* Update metadata in flash */
    uint32_t status = Update_UpdateMetadata(metadata);
    if (status != UPDATE_STATUS_SUCCESS) {
        return status;
    }
    
    /* Log update cancelled */
    Boot_LogStatus(BOOT_STATUS_UPDATE_CANCELLED, 0);
    
    return UPDATE_STATUS_SUCCESS;
}
