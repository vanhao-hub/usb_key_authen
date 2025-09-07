#ifndef STORAGE_PLATFORM_H
#define STORAGE_PLATFORM_H

/**
 * @file storage_platform.h
 * @brief Storage Platform Layer Interface
 * @author USB Key Authentication Team
 * @date 2025-09-07
 * @version 1.0
 * 
 * This file defines the Storage Platform layer that provides high-level
 * storage features like regions, encryption, wear leveling, and file
 * management on top of the raw Storage HAL interface.
 */

#include "hal/interface/storage_hal.h"
#include "hal/interface/crypto_hal.h"

/**
 * @brief Storage regions/partitions
 * 
 * Defines logical storage regions for different types of data.
 * Each region can have different security and access characteristics.
 */
typedef enum {
    STORAGE_REGION_CONFIG = 0,      /**< Device configuration data */
    STORAGE_REGION_CREDENTIALS,     /**< FIDO2 credentials and keys */
    STORAGE_REGION_PIV_CERTS,      /**< PIV certificates and keys */
    STORAGE_REGION_PIN_DATA,       /**< PIN hash and security data */
    STORAGE_REGION_COUNTERS,       /**< Signature counters */
    STORAGE_REGION_LOGS,           /**< Audit and event logs */
    STORAGE_REGION_USER_DATA,      /**< User-defined application data */
    STORAGE_REGION_MAX             /**< Maximum number of regions */
} storage_region_t;

/**
 * @brief Storage operation flags
 */
#define STORAGE_FLAG_ATOMIC         0x01    /**< Atomic read/write operations */
#define STORAGE_FLAG_ENCRYPTED      0x02    /**< Software encryption */
#define STORAGE_FLAG_AUTHENTICATED  0x04    /**< Integrity protection */
#define STORAGE_FLAG_PERSISTENT     0x08    /**< Survives power cycles */
#define STORAGE_FLAG_COMPRESSED     0x10    /**< Data compression */

/**
 * @brief Storage region configuration
 * 
 * Defines the configuration for a specific storage region.
 */
typedef struct {
    storage_region_t region;    /**< Region identifier */
    uint32_t base_address;      /**< Starting address in storage */
    uint32_t size;              /**< Region size in bytes */
    uint32_t flags;             /**< Region flags (STORAGE_FLAG_*) */
    uint32_t access_level;      /**< Security access level required */
    uint32_t backup_address;    /**< Backup region address (for redundancy) */
} storage_region_config_t;

/**
 * @brief Storage region information
 * 
 * Contains information about a configured storage region.
 */
typedef struct {
    storage_region_config_t config;    /**< Region configuration */
    uint32_t used_size;               /**< Currently used bytes */
    uint32_t free_size;               /**< Available bytes */
    uint32_t write_count;             /**< Number of writes to this region */
    uint32_t error_count;             /**< Number of errors in this region */
    bool is_healthy;                  /**< Region health status */
} storage_region_info_t;

/**
 * @brief Storage file handle
 * 
 * Handle for file operations within storage regions.
 */
typedef struct {
    storage_region_t region;    /**< Region containing the file */
    uint32_t file_id;          /**< Unique file identifier */
    uint32_t offset;           /**< Current file offset */
    uint32_t size;             /**< File size */
    uint32_t flags;            /**< File flags */
    bool is_open;              /**< File open status */
} storage_file_t;

/**
 * @brief Storage platform interface
 * 
 * This structure defines the Storage Platform API that provides
 * high-level storage operations on top of the raw Storage HAL.
 */
typedef struct {
    storage_hal_t* hal;        /**< Reference to Storage HAL */
    crypto_hal_t* crypto;      /**< Reference to Crypto HAL (for encryption) */
    bool initialized;          /**< Platform initialization status */
    
    /**
     * @brief Initialize storage platform
     * 
     * Initializes the storage platform and sets up regions.
     * 
     * @param storage_hal Pointer to initialized Storage HAL
     * @param crypto_hal Pointer to initialized Crypto HAL (optional)
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Platform initialized successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid HAL pointers
     * @retval HAL_ERROR_HARDWARE_FAILURE Storage initialization failed
     * 
     * @note crypto_hal can be NULL if encryption is not needed
     */
    hal_result_t (*init)(storage_hal_t* storage_hal, crypto_hal_t* crypto_hal);
    
    /**
     * @brief Deinitialize storage platform
     * 
     * Cleans up the storage platform and flushes all pending operations.
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Platform deinitialized successfully
     * 
     * @note All open files will be closed
     */
    hal_result_t (*deinit)(void);
    
    /**
     * @brief Configure storage region
     * 
     * Sets up a storage region with the specified configuration.
     * 
     * @param region Storage region to configure
     * @param config Pointer to region configuration
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Region configured successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid region or configuration
     * @retval HAL_ERROR_NOT_INITIALIZED Platform not initialized
     * @retval HAL_ERROR_INSUFFICIENT_MEMORY Not enough storage space
     * 
     * @note Region configuration is persistent across reboots
     */
    hal_result_t (*configure_region)(storage_region_t region, 
                                   const storage_region_config_t* config);
    
    /**
     * @brief Get region information
     * 
     * Retrieves information about a configured storage region.
     * 
     * @param region Storage region to query
     * @param info Pointer to store region information
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Information retrieved successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid region or info pointer
     * @retval HAL_ERROR_NOT_INITIALIZED Platform not initialized
     * 
     * @see storage_region_info_t
     */
    hal_result_t (*get_region_info)(storage_region_t region, storage_region_info_t* info);
    
    /**
     * @brief Read data from region
     * 
     * Reads data from a storage region with automatic decryption if enabled.
     * 
     * @param region Storage region to read from
     * @param offset Byte offset within the region
     * @param buffer Buffer to store read data
     * @param length Number of bytes to read
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Data read successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_NOT_INITIALIZED Platform not initialized
     * @retval HAL_ERROR_HARDWARE_FAILURE Storage read error
     * 
     * @note Data is automatically decrypted if region has STORAGE_FLAG_ENCRYPTED
     * @note Integrity is verified if region has STORAGE_FLAG_AUTHENTICATED
     */
    hal_result_t (*read_region)(storage_region_t region, uint32_t offset, 
                               uint8_t* buffer, size_t length);
    
    /**
     * @brief Write data to region
     * 
     * Writes data to a storage region with automatic encryption if enabled.
     * 
     * @param region Storage region to write to
     * @param offset Byte offset within the region
     * @param data Data to write
     * @param length Number of bytes to write
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Data written successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_NOT_INITIALIZED Platform not initialized
     * @retval HAL_ERROR_INSUFFICIENT_MEMORY Region full
     * @retval HAL_ERROR_HARDWARE_FAILURE Storage write error
     * 
     * @note Data is automatically encrypted if region has STORAGE_FLAG_ENCRYPTED
     * @note Integrity protection is added if region has STORAGE_FLAG_AUTHENTICATED
     * @note Wear leveling is performed automatically
     */
    hal_result_t (*write_region)(storage_region_t region, uint32_t offset,
                                const uint8_t* data, size_t length);
    
    /**
     * @brief Erase storage region
     * 
     * Erases all data in a storage region.
     * 
     * @param region Storage region to erase
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Region erased successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid region
     * @retval HAL_ERROR_NOT_INITIALIZED Platform not initialized
     * @retval HAL_ERROR_HARDWARE_FAILURE Storage erase error
     * 
     * @warning This operation destroys all data in the region
     */
    hal_result_t (*erase_region)(storage_region_t region);
    
    /**
     * @brief Open file in region
     * 
     * Opens a file within a storage region for read/write operations.
     * 
     * @param region Storage region containing the file
     * @param file_id Unique file identifier
     * @param file Pointer to store file handle
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS File opened successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_NOT_INITIALIZED Platform not initialized
     * @retval HAL_ERROR_NOT_SUPPORTED File system not available
     * 
     * @note File is created if it doesn't exist
     * @see close_file()
     */
    hal_result_t (*open_file)(storage_region_t region, uint32_t file_id, storage_file_t* file);
    
    /**
     * @brief Close file
     * 
     * Closes an open file and flushes any pending operations.
     * 
     * @param file Pointer to file handle
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS File closed successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid file handle
     * 
     * @see open_file()
     */
    hal_result_t (*close_file)(storage_file_t* file);
    
    /**
     * @brief Read from file
     * 
     * Reads data from an open file.
     * 
     * @param file Pointer to file handle
     * @param buffer Buffer to store read data
     * @param length Number of bytes to read
     * @param bytes_read Pointer to store actual bytes read
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Data read successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_INVALID_STATE File not open for reading
     * 
     * @note File offset is updated after successful read
     */
    hal_result_t (*read_file)(storage_file_t* file, uint8_t* buffer, 
                             size_t length, size_t* bytes_read);
    
    /**
     * @brief Write to file
     * 
     * Writes data to an open file.
     * 
     * @param file Pointer to file handle
     * @param data Data to write
     * @param length Number of bytes to write
     * @param bytes_written Pointer to store actual bytes written
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Data written successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_INVALID_STATE File not open for writing
     * @retval HAL_ERROR_INSUFFICIENT_MEMORY File/region full
     * 
     * @note File offset is updated after successful write
     * @note File size may grow if writing beyond current end
     */
    hal_result_t (*write_file)(storage_file_t* file, const uint8_t* data, 
                              size_t length, size_t* bytes_written);
    
    /**
     * @brief Delete file
     * 
     * Deletes a file from a storage region.
     * 
     * @param region Storage region containing the file
     * @param file_id File identifier to delete
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS File deleted successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_NOT_INITIALIZED Platform not initialized
     * @retval HAL_ERROR_HARDWARE_FAILURE File not found
     * 
     * @warning File data is permanently lost
     */
    hal_result_t (*delete_file)(storage_region_t region, uint32_t file_id);
    
    /**
     * @brief Perform garbage collection
     * 
     * Reclaims space from deleted files and optimizes storage layout.
     * 
     * @param region Storage region to garbage collect (or all regions if invalid)
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Garbage collection completed
     * @retval HAL_ERROR_NOT_INITIALIZED Platform not initialized
     * @retval HAL_ERROR_BUSY Operation in progress
     * 
     * @note This operation may take considerable time
     * @note Improves write performance and available space
     */
    hal_result_t (*garbage_collect)(storage_region_t region);
    
    /**
     * @brief Perform wear leveling
     * 
     * Redistributes data to ensure even wear across flash memory.
     * 
     * @param region Storage region to wear level (or all regions if invalid)
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Wear leveling completed
     * @retval HAL_ERROR_NOT_INITIALIZED Platform not initialized
     * @retval HAL_ERROR_NOT_SUPPORTED Storage type doesn't need wear leveling
     * 
     * @note Primarily for flash memory storage
     * @note May be performed automatically during write operations
     */
    hal_result_t (*wear_leveling)(storage_region_t region);
    
    /**
     * @brief Check storage integrity
     * 
     * Verifies data integrity across all regions using checksums and ECC.
     * 
     * @param region Storage region to check (or all regions if invalid)
     * @param is_valid Pointer to store integrity check result
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Integrity check completed
     * @retval HAL_ERROR_INVALID_PARAM Invalid is_valid pointer
     * @retval HAL_ERROR_NOT_INITIALIZED Platform not initialized
     * 
     * @note *is_valid is set to true if all data is intact
     * @note May take significant time for large regions
     */
    hal_result_t (*check_integrity)(storage_region_t region, bool* is_valid);
    
    /**
     * @brief Get platform statistics
     * 
     * Retrieves comprehensive statistics about storage usage and health.
     * 
     * @param stats Pointer to store platform statistics
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Statistics retrieved successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid stats pointer
     * @retval HAL_ERROR_NOT_INITIALIZED Platform not initialized
     */
    hal_result_t (*get_platform_stats)(storage_stats_t* stats);
    
} storage_platform_t;

/**
 * @brief Storage access levels
 */
#define STORAGE_ACCESS_PUBLIC       0   /**< No authentication required */
#define STORAGE_ACCESS_USER         1   /**< User authentication required */
#define STORAGE_ACCESS_ADMIN        2   /**< Admin authentication required */
#define STORAGE_ACCESS_SECURE       3   /**< Hardware security required */

#endif // STORAGE_PLATFORM_H