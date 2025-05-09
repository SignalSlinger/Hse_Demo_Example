/**
 * @file flash_programming.h
 * @brief Flash programming functions for S32K344
 * @date 2025-05-07
 * @author SignalSlinger
 */

#ifndef FLASH_PROGRAMMING_H_
#define FLASH_PROGRAMMING_H_

#include <stdint.h>

/**
 * @brief Erase a flash sector
 * @param address Starting address of the sector to erase
 * @param size Size of the sector in bytes
 * @return 0 on success, error code otherwise
 */
uint32_t Flash_EraseSector(uint32_t address, uint32_t size);

/**
 * @brief Program flash memory
 * @param address Destination address in flash
 * @param data Source data buffer
 * @param size Size of data to program in bytes
 * @return 0 on success, error code otherwise
 */
uint32_t Flash_Program(uint32_t address, const uint8_t* data, uint32_t size);

/**
 * @brief Invalidate flash cache
 */
void Flash_Cache_Invalidate(void);

#endif /* FLASH_PROGRAMMING_H_ */