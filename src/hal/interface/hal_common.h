#ifndef HAL_COMMON_H
#define HAL_COMMON_H

/**
 * @file hal_common.h
 * @brief Common definitions and interfaces for Hardware Abstraction Layer
 * @author USB Key Authentication Team
 * @date 2025-09-07
 * @version 1.0
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Common return codes for all HAL modules
 * 
 * These error codes are used consistently across all HAL implementations
 * to provide uniform error handling throughout the system.
 */
typedef enum {
    HAL_SUCCESS = 0,                    /**< Operation completed successfully */
    HAL_ERROR_INVALID_PARAM = -1,       /**< Invalid parameter provided */
    HAL_ERROR_NOT_INITIALIZED = -2,     /**< Module not initialized */
    HAL_ERROR_TIMEOUT = -3,             /**< Operation timed out */
    HAL_ERROR_BUSY = -4,                /**< Resource is busy */
    HAL_ERROR_NOT_SUPPORTED = -5,       /**< Operation not supported */
    HAL_ERROR_HARDWARE_FAILURE = -6,    /**< Hardware malfunction detected */
    HAL_ERROR_INSUFFICIENT_MEMORY = -7, /**< Not enough memory available */
    HAL_ERROR_INVALID_STATE = -8        /**< Invalid state for operation */
} hal_result_t;

/**
 * @brief Base interface for all HAL modules
 * 
 * This structure defines the common interface that all HAL modules must implement.
 * It provides basic lifecycle management functions.
 */
typedef struct {
    /**
     * @brief Initialize the HAL module
     * 
     * Performs hardware-specific initialization including:
     * - Hardware peripheral setup
     * - Internal state initialization
     * - Resource allocation
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Initialization completed successfully
     * @retval HAL_ERROR_HARDWARE_FAILURE Hardware initialization failed
     * @retval HAL_ERROR_INSUFFICIENT_MEMORY Memory allocation failed
     * 
     * @note Must be called before any other module operations
     * @warning Not thread-safe, call from main thread only
     */
    hal_result_t (*init)(void);
    
    /**
     * @brief Deinitialize the HAL module
     * 
     * Performs cleanup operations including:
     * - Hardware peripheral shutdown
     * - Resource deallocation
     * - State cleanup
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Deinitialization completed successfully
     * 
     * @note Module cannot be used after deinitialization without re-init
     * @warning Not thread-safe, ensure no other operations are in progress
     */
    hal_result_t (*deinit)(void);
    
    /**
     * @brief Reset the HAL module to initial state
     * 
     * Performs a soft reset of the module without full deinitialization:
     * - Hardware peripheral reset
     * - State machine reset
     * - Clear pending operations
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Reset completed successfully
     * @retval HAL_ERROR_NOT_INITIALIZED Module not initialized
     * 
     * @note Module remains initialized after reset
     * @see init(), deinit()
     */
    hal_result_t (*reset)(void);
    
    /**
     * @brief Check if the HAL module is initialized
     * 
     * @return true if module is initialized, false otherwise
     * 
     * @note This is a quick status check, does not validate hardware state
     */
    bool (*is_initialized)(void);
} hal_base_t;

/**
 * @brief HAL configuration structure
 * 
 * Common configuration parameters used across HAL modules for
 * validation and version checking.
 */
typedef struct {
    uint32_t magic;         /**< Magic number for validation (HAL_CONFIG_MAGIC) */
    uint32_t version;       /**< HAL version (HAL_VERSION) */
    void* platform_data;    /**< Platform-specific configuration data */
} hal_config_t;

/** @brief Magic number for HAL configuration validation */
#define HAL_CONFIG_MAGIC    0x48414C00  // "HAL\0"

/** @brief HAL major version number */
#define HAL_VERSION_MAJOR   1

/** @brief HAL minor version number */
#define HAL_VERSION_MINOR   0

/** @brief Combined HAL version */
#define HAL_VERSION         ((HAL_VERSION_MAJOR << 16) | HAL_VERSION_MINOR)

#endif // HAL_COMMON_H