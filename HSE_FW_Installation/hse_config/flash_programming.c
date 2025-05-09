/**
 * @file flash_programming.c
 * @brief Flash programming functions for S32K344
 * @date 2025-05-07
 * @author SignalSlinger
 */

#include "flash_programming.h"
#include "Mcal.h"

/* S32K344 Flash Controller register definitions */
#define PFLASH_BASE          0x40268000UL
#define PFLASH_MCR           (*(volatile uint32_t*)(PFLASH_BASE + 0x00))
#define PFLASH_MCRS          (*(volatile uint32_t*)(PFLASH_BASE + 0x04))
#define PFLASH_MCRE          (*(volatile uint32_t*)(PFLASH_BASE + 0x08))
#define PFLASH_SEL           (*(volatile uint32_t*)(PFLASH_BASE + 0x0C))
#define PFLASH_UT0           (*(volatile uint32_t*)(PFLASH_BASE + 0x10))
#define PFLASH_UT1           (*(volatile uint32_t*)(PFLASH_BASE + 0x14))
#define PFLASH_UT2           (*(volatile uint32_t*)(PFLASH_BASE + 0x18))
#define PFLASH_UT3           (*(volatile uint32_t*)(PFLASH_BASE + 0x1C))
#define PFLASH_LOCK0         (*(volatile uint32_t*)(PFLASH_BASE + 0x20))
#define PFLASH_LOCK1         (*(volatile uint32_t*)(PFLASH_BASE + 0x24))
#define PFLASH_LOCK2         (*(volatile uint32_t*)(PFLASH_BASE + 0x28))
#define PFLASH_LOCK3         (*(volatile uint32_t*)(PFLASH_BASE + 0x2C))
#define PFLASH_PFCBLK_SPELOCK    (*(volatile uint32_t*)(PFLASH_BASE + 0x82C))

/* Cache Control register */
#define CACHE_CTRL_BASE      0x4023C000UL
#define CACHE_CTRL_CCR       (*(volatile uint32_t*)(CACHE_CTRL_BASE + 0x00))

/* Flash command values */
#define FLASH_CMD_ERASE_SECTOR       0x000009
#define FLASH_CMD_PROGRAM_LONGWORD   0x000008
#define FLASH_CMD_CHECK_STATUS       0x000001

/* Flash status values */
#define FLASH_STATUS_DONE            0x00000001
#define FLASH_STATUS_ERROR           0x00000004

/**
 * @brief Wait for flash operation to complete
 * @return 0 on success, error code otherwise
 */
static uint32_t Flash_WaitForDone(void) {
    uint32_t status;
    uint32_t timeout = 500000; /* Appropriate timeout value */
    
    do {
        status = PFLASH_MCRS;
        
        if (--timeout == 0) {
            return 1; /* Timeout error */
        }
    } while ((status & FLASH_STATUS_DONE) == 0);
    
    /* Check for errors */
    if (status & FLASH_STATUS_ERROR) {
        return 2; /* Flash operation error */
    }
    
    return 0; /* Success */
}

/**
 * @brief Unlock flash for programming
 * @return 0 on success, error code otherwise
 */
static uint32_t Flash_Unlock(void) {
    /* Unlock flash blocks as needed */
    PFLASH_LOCK0 = 0xFFFFFFFF;
    PFLASH_LOCK1 = 0xFFFFFFFF;
    PFLASH_LOCK2 = 0xFFFFFFFF;
    PFLASH_LOCK3 = 0xFFFFFFFF;
    
    /* Unlock special regions */
    PFLASH_PFCBLK_SPELOCK = 0xFFFFFFFF;
    
    return 0;
}

/**
 * @brief Erase a flash sector
 * @param address Starting address of the sector to erase
 * @param size Size of the sector in bytes
 * @return 0 on success, error code otherwise
 */
uint32_t Flash_EraseSector(uint32_t address, uint32_t size) {
    uint32_t status;
    
    /* Unlock flash */
    status = Flash_Unlock();
    if (status != 0) {
        return status;
    }
    
    /* Setup erase parameters */
    PFLASH_UT0 = address;
    PFLASH_UT1 = size;
    
    /* Issue erase command */
    PFLASH_MCR = FLASH_CMD_ERASE_SECTOR;
    
    /* Wait for operation to complete */
    status = Flash_WaitForDone();
    
    /* Invalidate cache */
    Flash_Cache_Invalidate();
    
    return status;
}

/**
 * @brief Program flash memory
 * @param address Destination address in flash
 * @param data Source data buffer
 * @param size Size of data to program in bytes
 * @return 0 on success, error code otherwise
 */
uint32_t Flash_Program(uint32_t address, const uint8_t* data, uint32_t size) {
    uint32_t status;
    uint32_t alignedSize;
    uint32_t numWords;
    uint32_t i;
    const uint32_t* wordData = (const uint32_t*)data;
    
    /* Check alignment */
    if (address % 4 != 0) {
        return 3; /* Address not aligned */
    }
    
    /* Calculate aligned size */
    alignedSize = (size + 3) & ~3;
    numWords = alignedSize / 4;
    
    /* Unlock flash */
    status = Flash_Unlock();
    if (status != 0) {
        return status;
    }
    
    /* Program data word by word */
    for (i = 0; i < numWords; i++) {
        /* Setup program parameters */
        PFLASH_UT0 = address + (i * 4);
        PFLASH_UT1 = wordData[i];
        
        /* Issue program command */
        PFLASH_MCR = FLASH_CMD_PROGRAM_LONGWORD;
        
        /* Wait for operation to complete */
        status = Flash_WaitForDone();
        if (status != 0) {
            return status;
        }
    }
    
    /* Invalidate cache */
    Flash_Cache_Invalidate();
    
    return 0;
}

/**
 * @brief Invalidate flash cache
 */
void Flash_Cache_Invalidate(void) {
    /* Set cache invalidate bit */
    CACHE_CTRL_CCR |= 0x00000002;
    
    /* Wait for invalidation to complete */
    while (CACHE_CTRL_CCR & 0x00000002) {
        /* Wait */
    }
}