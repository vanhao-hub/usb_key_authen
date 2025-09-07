#ifndef STORAGE_HAL_H
#define STORAGE_HAL_H

/**
 * @file storage_hal.h
 * @brief Storage Hardware Abstraction Layer Interface
 * @author USB Key Authentication Team
 * @date 2025-09-07
 * @version 1.0
 * 
 * This file defines the raw storage HAL interface for basic read/write/erase
 * operations on physical storage devices. Higher-level features like regions,
 * encryption, and wear leveling are handled by the Storage Platform layer.
 */

#include "hal_common.h"

/**
 * @brief Storage technology types
 * 
 * Defines the underlying storage technology types.
 */
typedef enum {
    STORAGE_TYPE_FLASH = 0,         /**< Flash memory (NOR/NAND) */
    STORAGE_TYPE_EEPROM,            /**< EEPROM memory */
    STORAGE_TYPE_FRAM,              /**< Ferroelectric RAM */
    STORAGE_TYPE_SECURE_ELEMENT     /**< Hardware secure element */
} storage_type_t;

/**
 * @brief Storage device information
 * 
 * Contains basic information about the physical storage device.
 */
typedef struct {
    storage_type_t type;        /**< Storage technology type */
    uint32_t total_size;        /**< Total storage size in bytes */
    uint32_t sector_size;       /**< Erase sector size in bytes (0 if not applicable) */
    uint32_t page_size;         /**< Write page size in bytes (0 if not applicable) */
    uint32_t capabilities;      /**< Hardware capability flags (STORAGE_CAP_*) */
} storage_info_t;

/**
 * @brief Storage usage statistics
 * 
 * Contains basic statistics about storage operations.
 */
typedef struct {
    uint32_t total_reads;       /**< Total number of read operations */
    uint32_t total_writes;      /**< Total number of write operations */
    uint32_t total_erases;      /**< Total number of erase operations */
    uint32_t error_count;       /**< Number of errors encountered */
} storage_stats_t;

/**
 * @brief Storage HAL interface
 * 
 * This structure defines the raw storage HAL API for basic
 * read/write/erase operations on physical storage devices.
 */
typedef struct {
    hal_base_t base;    /**< Base HAL interface */
    
    /**
     * @brief Get storage device information
     * 
     * Retrieves information about the physical storage device.
     * 
     * @param info Pointer to store device information
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Information retrieved successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid info pointer
     * @retval HAL_ERROR_NOT_INITIALIZED Storage HAL not initialized
     * 
     * @see storage_info_t
     */
    hal_result_t (*get_info)(storage_info_t* info);
    
    /**
     * @brief Read data from storage
     * 
     * Reads raw data from the physical storage device.
     * 
     * @param address Physical address to read from
     * @param buffer Buffer to store read data
     * @param length Number of bytes to read
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Data read successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_NOT_INITIALIZED Storage HAL not initialized
     * @retval HAL_ERROR_HARDWARE_FAILURE Storage read error
     * @retval HAL_ERROR_TIMEOUT Read operation timed out
     * 
     * @note Address + length must not exceed device size
     * @note This is raw read - no decryption or region handling
     */
    hal_result_t (*read)(uint32_t address, uint8_t* buffer, size_t length);
    
    /**
     * @brief Write data to storage
     * 
     * Writes raw data to the physical storage device.
     * 
     * @param address Physical address to write to
     * @param data Data to write
     * @param length Number of bytes to write
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Data written successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_NOT_INITIALIZED Storage HAL not initialized
     * @retval HAL_ERROR_HARDWARE_FAILURE Storage write error
     * @retval HAL_ERROR_TIMEOUT Write operation timed out
     * 
     * @note May require prior erase for flash memory
     * @note This is raw write - no encryption or region handling
     * @warning Flash memory may require page-aligned writes
     */
    hal_result_t (*write)(uint32_t address, const uint8_t* data, size_t length);
    
    /**
     * @brief Erase storage sectors
     * 
     * Erases (sets to 0xFF) sectors of the storage device.
     * Required before writing to flash memory.
     * 
     * @param address Starting address (must be sector-aligned)
     * @param length Number of bytes to erase (must be sector-multiple)
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Data erased successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters or alignment
     * @retval HAL_ERROR_NOT_INITIALIZED Storage HAL not initialized
     * @retval HAL_ERROR_NOT_SUPPORTED Erase not supported
     * @retval HAL_ERROR_HARDWARE_FAILURE Storage erase error
     * 
     * @note Erase granularity is device-specific (sector_size)
     * @warning Erases are destructive and cannot be undone
     */
    hal_result_t (*erase)(uint32_t address, size_t length);
    
    /**
     * @brief Get storage statistics
     * 
     * Retrieves basic usage statistics from the storage device.
     * 
     * @param stats Pointer to store statistics
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Statistics retrieved successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid stats pointer
     * @retval HAL_ERROR_NOT_SUPPORTED Statistics not available
     * 
     * @see storage_stats_t
     */
    hal_result_t (*get_stats)(storage_stats_t* stats);
    
    /**
     * @brief Flush pending operations
     * 
     * Ensures all pending write/erase operations are completed.
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS All operations flushed
     * @retval HAL_ERROR_HARDWARE_FAILURE Flush failed
     * @retval HAL_ERROR_TIMEOUT Flush timed out
     * 
     * @note Important for ensuring data integrity
     * @note USB key is always powered, so this is mainly for flash cache
     */
    hal_result_t (*flush)(void);
    
} storage_hal_t;

/** @brief Hardware encryption/decryption supported */
#define STORAGE_CAP_HW_ENCRYPTION   0x0001

/** @brief Atomic operations supported */
#define STORAGE_CAP_ATOMIC_OPS      0x0002

/** @brief Wear leveling handled by hardware */
#define STORAGE_CAP_HW_WEAR_LEVEL   0x0004

/** @brief Bad block management by hardware */
#define STORAGE_CAP_BAD_BLOCK_MGMT  0x0008

#endif // STORAGE_HAL_H