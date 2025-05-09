/**
 * @file fallback_manager.c
 * @brief Fallback firmware management implementation
 * @date 2025-05-07
 * @author SignalSlinger
 */

#include "fallback_manager.h"
#include "boot_recovery.h"
#include "Siul2_Port_Ip.h" // For Port initialization
#include "Siul2_Dio_Ip.h"  // For LED control
#include <string.h>

/* Flash programming functions - these would be MCU-specific */
extern uint32_t Flash_EraseSector(uint32_t address, uint32_t size);
extern uint32_t Flash_Program(uint32_t address, const uint8_t* data, uint32_t size);
extern void Flash_Cache_Invalidate(void);

/* Static function prototypes */
static uint32_t Fallback_CalculateCRC32(const uint8_t* data, uint32_t length);
static uint32_t Fallback_UpdateMetadata(const fallback_metadata_t* metadata);
static uint32_t Fallback_ValidateMetadata(const fallback_metadata_t* metadata);

/**
 * @brief Calculate CRC-32 checksum
 * @param data Pointer to data
 * @param length Length of data in bytes
 * @return CRC-32 value
 */
static uint32_t Fallback_CalculateCRC32(const uint8_t* data, uint32_t length) {
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
 * @brief Get pointer to fallback metadata
 * @return Pointer to metadata structure
 */
static fallback_metadata_t* Fallback_GetMetadataPtr(void) {
    return (fallback_metadata_t*)FALLBACK_METADATA_ADDR;
}

/**
 * @brief Validate metadata structure
 * @param metadata Pointer to metadata structure
 * @return Status code
 */
static uint32_t Fallback_ValidateMetadata(const fallback_metadata_t* metadata) {
    /* Check magic number */
    if (metadata->magic != FALLBACK_METADATA_MAGIC) {
        return FALLBACK_STATUS_INVALID;
    }
    
    /* Check metadata version */
    if ((metadata->version & 0xFFFF0000) != 
        (FALLBACK_METADATA_VERSION & 0xFFFF0000)) {
        return FALLBACK_STATUS_INVALID;
    }
    
    /* Calculate and verify metadata CRC */
    uint32_t calculatedCrc = Fallback_CalculateCRC32(
        (const uint8_t*)metadata, 
        sizeof(fallback_metadata_t) - sizeof(uint32_t));
    
    if (calculatedCrc != metadata->metadataCrc) {
        return FALLBACK_STATUS_INVALID;
    }
    
    return FALLBACK_STATUS_SUCCESS;
}

/**
 * @brief Update fallback metadata in flash
 * @param metadata Pointer to metadata structure to store
 * @return Status code
 */
static uint32_t Fallback_UpdateMetadata(const fallback_metadata_t* metadata) {
    /* Calculate metadata CRC - create a copy since we need to modify it */
    fallback_metadata_t metadataCopy = *metadata;
    
    metadataCopy.metadataCrc = Fallback_CalculateCRC32(
        (const uint8_t*)&metadataCopy, 
        sizeof(fallback_metadata_t) - sizeof(uint32_t));
    
    /* Erase metadata sector */
    uint32_t status = Flash_EraseSector(FALLBACK_METADATA_ADDR, 4096);
    if (status != 0) {
        return FALLBACK_STATUS_FLASH_ERR;
    }
    
    /* Program new metadata */
    status = Flash_Program(
        FALLBACK_METADATA_ADDR,
        (const uint8_t*)&metadataCopy,
        sizeof(fallback_metadata_t));
    
    if (status != 0) {
        return FALLBACK_STATUS_FLASH_ERR;
    }
    
    /* Invalidate cache */
    Flash_Cache_Invalidate();
    
    return FALLBACK_STATUS_SUCCESS;
}

/**
 * @brief Initialize fallback firmware manager
 * @return Status code
 */
uint32_t Fallback_Init(void) {
    fallback_metadata_t* metadata = Fallback_GetMetadataPtr();
    
    /* Check if metadata is already initialized */
    if (metadata->magic == FALLBACK_METADATA_MAGIC) {
        /* Validate metadata */
        return Fallback_ValidateMetadata(metadata);
    }
    
    /* Metadata not initialized - if this is a first boot,
       we'll initialize it when storing the first fallback */
    return FALLBACK_STATUS_SUCCESS;
}

/**
 * @brief Check if valid fallback firmware exists
 * @return 1 if valid fallback exists, 0 otherwise
 */
uint32_t Fallback_IsValid(void) {
    fallback_metadata_t* metadata = Fallback_GetMetadataPtr();
    
    /* Check metadata validity */
    if (Fallback_ValidateMetadata(metadata) != FALLBACK_STATUS_SUCCESS) {
        return 0;
    }
    
    /* Verify firmware CRC */
    uint32_t calculatedCrc = Fallback_CalculateCRC32(
        (const uint8_t*)FALLBACK_FIRMWARE_ADDR,
        metadata->firmwareSize);
    
    if (calculatedCrc != metadata->firmwareCrc) {
        return 0;
    }
    
    return 1;
}

/**
 * @brief Store current firmware as fallback
 * @return Status code
 */
uint32_t Fallback_StoreCurrentFirmware(void) {
    fallback_metadata_t newMetadata;
    uint32_t status;
    
    /* Visual indicator for backup operation */
    Siul2_Dio_Ip_TogglePins(LED_GREEN_PORT, (1U << LED_GREEN_PIN));
    
    /* Erase fallback firmware region */
    status = Flash_EraseSector(FALLBACK_FIRMWARE_ADDR, FALLBACK_FIRMWARE_SIZE);
    if (status != 0) {
        return FALLBACK_STATUS_FLASH_ERR;
    }
    
    /* Copy current firmware to fallback region */
    status = Flash_Program(
        FALLBACK_FIRMWARE_ADDR,
        (const uint8_t*)APP_FIRMWARE_ADDR,
        APP_FIRMWARE_SIZE);
    
    if (status != 0) {
        return FALLBACK_STATUS_FLASH_ERR;
    }
    
    /* Calculate firmware CRC */
    uint32_t firmwareCrc = Fallback_CalculateCRC32(
        (const uint8_t*)FALLBACK_FIRMWARE_ADDR,
        APP_FIRMWARE_SIZE);
    
    /* Initialize metadata */
    memset(&newMetadata, 0, sizeof(fallback_metadata_t));
    newMetadata.magic = FALLBACK_METADATA_MAGIC;
    newMetadata.version = FALLBACK_METADATA_VERSION;
    newMetadata.firmwareSize = APP_FIRMWARE_SIZE;
    newMetadata.firmwareCrc = firmwareCrc;
    newMetadata.creationTime = Boot_GetTimestamp();
    newMetadata.updateCount = 0;
    
    /* Store firmware version (this would come from version info) */
    newMetadata.firmwareVersion[0] = 1; /* major */
    newMetadata.firmwareVersion[1] = 0; /* minor */
    newMetadata.firmwareVersion[2] = 0; /* patch */
    newMetadata.firmwareVersion[3] = 0; /* build */
    
    /* Update metadata in flash */
    status = Fallback_UpdateMetadata(&newMetadata);
    
    /* Visual indicator for backup completion */
    Siul2_Dio_Ip_TogglePins(LED_GREEN_PORT, (1U << LED_GREEN_PIN));
    
    return status;
}

/**
 * @brief Recover using fallback firmware
 * @return Status code
 */
uint32_t Fallback_RecoverMainFirmware(void) {
    uint32_t status;
    fallback_metadata_t* metadata = Fallback_GetMetadataPtr();
    
    /* Check if fallback is valid */
    if (!Fallback_IsValid()) {
        return FALLBACK_STATUS_INVALID;
    }
    
    /* Visual indicator for recovery operation */
    for (uint32_t i = 0; i < 3; i++) {
    	Siul2_Dio_Ip_WritePin(LED_RED_PORT, (1U << LED_RED_PIN), 1);
        
        /* Delay */
        volatile uint32_t delay;
        for (delay = 0; delay < 200000; delay++) {}
        
        Siul2_Dio_Ip_WritePin(LED_RED_PORT, (1U << LED_RED_PIN), 0);
        
        for (delay = 0; delay < 200000; delay++) {}
    }
    
    /* Erase main firmware region */
    status = Flash_EraseSector(APP_FIRMWARE_ADDR, APP_FIRMWARE_SIZE);
    if (status != 0) {
        return FALLBACK_STATUS_FLASH_ERR;
    }
    
    /* Copy fallback firmware to main region */
    status = Flash_Program(
        APP_FIRMWARE_ADDR,
        (const uint8_t*)FALLBACK_FIRMWARE_ADDR,
        metadata->firmwareSize);
    
    if (status != 0) {
        return FALLBACK_STATUS_FLASH_ERR;
    }
    
    /* Verify the copy operation */
    uint32_t mainCrc = Fallback_CalculateCRC32(
        (const uint8_t*)APP_FIRMWARE_ADDR,
        metadata->firmwareSize);
    
    if (mainCrc != metadata->firmwareCrc) {
        return FALLBACK_STATUS_FAILURE;
    }
    
    /* Update metadata to increment usage count */
    metadata->updateCount++;
    Fallback_UpdateMetadata(metadata);
    
    /* Invalidate cache */
    Flash_Cache_Invalidate();
    
    /* Log recovery operation */
    Boot_LogStatus(BOOT_STATUS_RECOVERY_ACTIVE, 0);
    
    return FALLBACK_STATUS_SUCCESS;
}

/**
 * @brief Get fallback firmware metadata
 * @return Pointer to metadata structure (read-only)
 */
const fallback_metadata_t* Fallback_GetMetadata(void) {
    /* Return a pointer to the metadata if it's valid */
    fallback_metadata_t* metadata = Fallback_GetMetadataPtr();
    
    if (Fallback_ValidateMetadata(metadata) == FALLBACK_STATUS_SUCCESS) {
        return metadata;
    } else {
        return NULL;
    }
}

/**
 * @brief Verify fallback firmware integrity
 * @return Status code
 */
uint32_t Fallback_VerifyIntegrity(void) {
    fallback_metadata_t* metadata = Fallback_GetMetadataPtr();
    
    /* Check metadata validity */
    if (Fallback_ValidateMetadata(metadata) != FALLBACK_STATUS_SUCCESS) {
        return FALLBACK_STATUS_INVALID;
    }
    
    /* Verify firmware CRC */
    uint32_t calculatedCrc = Fallback_CalculateCRC32(
        (const uint8_t*)FALLBACK_FIRMWARE_ADDR,
        metadata->firmwareSize);
    
    if (calculatedCrc != metadata->firmwareCrc) {
        return FALLBACK_STATUS_FAILURE;
    }
    
    return FALLBACK_STATUS_SUCCESS;
}

/**
 * @brief Boot from fallback firmware without copying
 * @return Does not return if successful
 */
void Fallback_JumpToFallbackFirmware(void) {
    /* Check if fallback is valid */
    if (!Fallback_IsValid()) {
        /* Cannot jump to invalid fallback */
        return;
    }
    
    /* This function would contain assembly code to:
     * 1. Set up the stack pointer for the fallback firmware
     * 2. Get the reset vector address from fallback firmware
     * 3. Jump to the reset vector
     * 
     * The exact implementation is highly processor-specific
     * and would require assembly code.
     */
    
    /* Example placeholder for ARM Cortex-M based MCUs */
    typedef void (*ResetFunc)(void);
    
    /* Get reset vector from fallback firmware vector table */
    uint32_t* vectorTable = (uint32_t*)FALLBACK_FIRMWARE_ADDR;
    uint32_t stackPointer = vectorTable[0];
    uint32_t resetHandler = vectorTable[1];
    
    /* Log the jump operation */
    Boot_LogStatus(BOOT_STATUS_RECOVERY_ACTIVE, 1);
    
    /* Disable interrupts */
    __asm volatile ("cpsid i");
    
    /* Set the vector table offset register to use fallback vector table */
    *((volatile uint32_t*)0xE000ED08) = FALLBACK_FIRMWARE_ADDR;
    
    /* Set main stack pointer */
    __asm volatile ("msr msp, %0" : : "r" (stackPointer));
    
    /* Jump to the fallback firmware reset handler */
    ResetFunc Reset_Handler = (ResetFunc)resetHandler;
    Reset_Handler();
    
    /* Should never reach here */
    while(1) {}
}
