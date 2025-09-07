#ifndef HAL_MANAGER_H
#define HAL_MANAGER_H

/**
 * @file hal_manager.h
 * @brief Hardware Abstraction Layer Manager Interface
 * @author USB Key Authentication Team
 * @date 2025-09-07
 * @version 1.0
 * 
 * This file defines the HAL manager interface that provides centralized
 * management and access to all HAL modules. It handles platform detection,
 * initialization, and provides a unified interface for accessing different
 * HAL implementations (Mock, STM32, ESP32, etc.).
 */

#include "interface/hal_common.h"
#include "interface/usb_hid_hal.h"
#include "interface/crypto_hal.h"
#include "interface/storage_hal.h"

/**
 * @brief Supported HAL platform types
 * 
 * Defines the different hardware platforms supported by the HAL system.
 * Each platform has its own specific HAL implementations.
 */
typedef enum {
    HAL_PLATFORM_MOCK = 0,          /**< Mock implementation for testing */
    HAL_PLATFORM_STM32,             /**< STM32 microcontroller family */
    HAL_PLATFORM_ESP32,             /**< ESP32 microcontroller family */
    HAL_PLATFORM_AUTO_DETECT        /**< Automatic platform detection */
} hal_platform_t;

/**
 * @brief HAL manager structure
 * 
 * Central management structure that holds references to all HAL modules
 * and maintains the overall HAL system state.
 */
typedef struct {
    hal_platform_t platform;       /**< Currently selected platform */
    usb_hid_hal_t* usb_hid;        /**< USB HID HAL instance */
    crypto_hal_t* crypto;          /**< Crypto HAL instance */
    storage_hal_t* storage;        /**< Storage HAL instance */
    bool initialized;              /**< Manager initialization state */
} hal_manager_t;

/**
 * @brief Initialize HAL manager
 * 
 * Initializes the HAL manager and all HAL modules for the specified platform.
 * This is the main entry point for setting up the entire HAL system.
 * 
 * @param platform Target platform to initialize for
 * 
 * @return HAL_SUCCESS on success, error code otherwise
 * @retval HAL_SUCCESS All HAL modules initialized successfully
 * @retval HAL_ERROR_INVALID_PARAM Invalid platform specified
 * @retval HAL_ERROR_NOT_SUPPORTED Platform not supported in this build
 * @retval HAL_ERROR_INVALID_STATE HAL manager already initialized
 * @retval HAL_ERROR_HARDWARE_FAILURE Hardware initialization failed
 * @retval HAL_ERROR_INSUFFICIENT_MEMORY Memory allocation failed
 * 
 * @note Must be called before using any HAL functionality
 * @note If HAL_PLATFORM_AUTO_DETECT is used, platform will be detected automatically
 * @warning Not thread-safe, call from main thread only
 * 
 * @see hal_manager_deinit(), hal_detect_platform()
 * 
 * @code
 * // Example usage
 * hal_result_t result = hal_manager_init(HAL_PLATFORM_AUTO_DETECT);
 * if (result != HAL_SUCCESS) {
 *     printf("HAL initialization failed: %d\n", result);
 *     return -1;
 * }
 * @endcode
 */
hal_result_t hal_manager_init(hal_platform_t platform);

/**
 * @brief Deinitialize HAL manager
 * 
 * Deinitializes all HAL modules and cleans up the HAL manager.
 * Should be called during system shutdown.
 * 
 * @return HAL_SUCCESS on success, error code otherwise
 * @retval HAL_SUCCESS All HAL modules deinitialized successfully
 * @retval HAL_ERROR_NOT_INITIALIZED HAL manager not initialized
 * 
 * @note HAL functionality cannot be used after deinitialization
 * @warning Not thread-safe, ensure no HAL operations are in progress
 * 
 * @see hal_manager_init()
 * 
 * @code
 * // Example usage during shutdown
 * hal_result_t result = hal_manager_deinit();
 * if (result != HAL_SUCCESS) {
 *     printf("HAL deinitialization failed: %d\n", result);
 * }
 * @endcode
 */
hal_result_t hal_manager_deinit(void);

/**
 * @brief Get HAL manager instance
 * 
 * Returns a pointer to the global HAL manager instance.
 * Used for direct access to the manager structure.
 * 
 * @return Pointer to HAL manager instance, or NULL if not initialized
 * 
 * @note The returned pointer is valid only while HAL manager is initialized
 * @warning Do not modify the returned structure directly
 * 
 * @see hal_manager_init()
 * 
 * @code
 * hal_manager_t* manager = hal_manager_get_instance();
 * if (manager) {
 *     printf("Current platform: %s\n", hal_get_platform_name(manager->platform));
 * }
 * @endcode
 */
hal_manager_t* hal_manager_get_instance(void);

/**
 * @brief Get USB HID HAL instance
 * 
 * Returns a pointer to the USB HID HAL implementation for the current platform.
 * 
 * @return Pointer to USB HID HAL instance, or NULL if not initialized
 * 
 * @note The returned pointer is valid only while HAL manager is initialized
 * @warning Do not cache this pointer across HAL reinitializations
 * 
 * @see hal_manager_init(), usb_hid_hal_t
 * 
 * @code
 * usb_hid_hal_t* usb_hal = hal_get_usb_hid();
 * if (usb_hal && usb_hal->is_connected()) {
 *     // USB device is connected
 *     usb_hal->send_report(0x01, data, sizeof(data));
 * }
 * @endcode
 */
usb_hid_hal_t* hal_get_usb_hid(void);

/**
 * @brief Get Crypto HAL instance
 * 
 * Returns a pointer to the Crypto HAL implementation for the current platform.
 * 
 * @return Pointer to Crypto HAL instance, or NULL if not initialized
 * 
 * @note The returned pointer is valid only while HAL manager is initialized
 * @warning Do not cache this pointer across HAL reinitializations
 * 
 * @see hal_manager_init(), crypto_hal_t
 * 
 * @code
 * crypto_hal_t* crypto_hal = hal_get_crypto();
 * if (crypto_hal) {
 *     crypto_key_t public_key, private_key;
 *     hal_result_t result = crypto_hal->generate_key_pair(
 *         CRYPTO_ALG_ECC_P256, &public_key, &private_key);
 * }
 * @endcode
 */
crypto_hal_t* hal_get_crypto(void);

/**
 * @brief Get Storage HAL instance
 * 
 * Returns a pointer to the Storage HAL implementation for the current platform.
 * 
 * @return Pointer to Storage HAL instance, or NULL if not initialized
 * 
 * @note The returned pointer is valid only while HAL manager is initialized
 * @warning Do not cache this pointer across HAL reinitializations
 * 
 * @see hal_manager_init(), storage_hal_t
 * 
 * @code
 * storage_hal_t* storage_hal = hal_get_storage();
 * if (storage_hal) {
 *     uint8_t config_data[256];
 *     hal_result_t result = storage_hal->read(
 *         STORAGE_REGION_CONFIG, 0, config_data, sizeof(config_data));
 * }
 * @endcode
 */
storage_hal_t* hal_get_storage(void);

/**
 * @brief Detect current hardware platform
 * 
 * Automatically detects the current hardware platform based on
 * compile-time definitions, hardware registers, or other platform-specific methods.
 * 
 * @return Detected platform type
 * @retval HAL_PLATFORM_STM32 STM32 platform detected
 * @retval HAL_PLATFORM_ESP32 ESP32 platform detected
 * @retval HAL_PLATFORM_MOCK Mock platform (default/unknown)
 * 
 * @note Detection is based on compile-time definitions and hardware probing
 * @note Returns HAL_PLATFORM_MOCK if platform cannot be determined
 * 
 * @see hal_manager_init(), hal_get_platform_name()
 * 
 * @code
 * hal_platform_t platform = hal_detect_platform();
 * printf("Detected platform: %s\n", hal_get_platform_name(platform));
 * 
 * hal_result_t result = hal_manager_init(platform);
 * @endcode
 */
hal_platform_t hal_detect_platform(void);

/**
 * @brief Get platform name string
 * 
 * Returns a human-readable string name for the specified platform.
 * 
 * @param platform Platform type to get name for
 * 
 * @return Pointer to platform name string
 * @retval "Mock" For HAL_PLATFORM_MOCK
 * @retval "STM32" For HAL_PLATFORM_STM32
 * @retval "ESP32" For HAL_PLATFORM_ESP32
 * @retval "Auto-Detect" For HAL_PLATFORM_AUTO_DETECT
 * @retval "Unknown" For invalid platform values
 * 
 * @note Returned string is statically allocated and does not need to be freed
 * @note Safe to call even if HAL manager is not initialized
 * 
 * @see hal_detect_platform(), hal_manager_init()
 * 
 * @code
 * for (int i = 0; i < HAL_PLATFORM_AUTO_DETECT; i++) {
 *     printf("Platform %d: %s\n", i, hal_get_platform_name(i));
 * }
 * @endcode
 */
const char* hal_get_platform_name(hal_platform_t platform);

#endif // HAL_MANAGER_H