/**
 * @file hal_manager.c
 * @brief Hardware Abstraction Layer Manager Implementation
 * @author USB Key Authentication Team
 * @date 2025-09-07
 * @version 1.0
 * 
 * This file implements the HAL manager functionality including platform
 * detection, HAL module initialization, and access management.
 */

#include "hal_manager.h"
#include <stdio.h>
#include <string.h>

// External HAL implementations - Mock (always available)
extern usb_hid_hal_t mock_usb_hid_hal;
extern crypto_hal_t mock_crypto_hal;
extern storage_hal_t mock_storage_hal;

// STM32 HAL implementations (conditionally compiled)
#ifdef HAL_STM32_ENABLED
extern usb_hid_hal_t stm32_usb_hid_hal;
extern crypto_hal_t stm32_crypto_hal;
extern storage_hal_t stm32_storage_hal;
#endif

// ESP32 HAL implementations (conditionally compiled)
#ifdef HAL_ESP32_ENABLED
extern usb_hid_hal_t esp32_usb_hid_hal;
extern crypto_hal_t esp32_crypto_hal;
extern storage_hal_t esp32_storage_hal;
#endif

/**
 * @brief Global HAL manager instance
 * 
 * Single global instance of the HAL manager that maintains the state
 * of all HAL modules and the current platform configuration.
 */
static hal_manager_t g_hal_manager = {0};

/**
 * @brief Initialize HAL module with error checking
 * 
 * Helper function to initialize a HAL module and provide detailed error reporting.
 * 
 * @param module_name Human-readable name of the module for error messages
 * @param hal_base Pointer to the HAL base interface
 * 
 * @return HAL_SUCCESS on success, error code from module initialization otherwise
 */
static hal_result_t init_hal_module(const char* module_name, hal_base_t* hal_base) {
    if (!hal_base || !hal_base->init) {
        printf("[HAL_MANAGER] %s HAL has no init function\n", module_name);
        return HAL_ERROR_NOT_SUPPORTED;
    }
    
    printf("[HAL_MANAGER] Initializing %s HAL...\n", module_name);
    hal_result_t result = hal_base->init();
    
    if (result == HAL_SUCCESS) {
        printf("[HAL_MANAGER] %s HAL initialized successfully\n", module_name);
    } else {
        printf("[HAL_MANAGER] %s HAL initialization failed: %d\n", module_name, result);
    }
    
    return result;
}

/**
 * @brief Deinitialize HAL module with error checking
 * 
 * Helper function to deinitialize a HAL module safely.
 * 
 * @param module_name Human-readable name of the module for logging
 * @param hal_base Pointer to the HAL base interface
 */
static void deinit_hal_module(const char* module_name, hal_base_t* hal_base) {
    if (hal_base && hal_base->deinit) {
        printf("[HAL_MANAGER] Deinitializing %s HAL...\n", module_name);
        hal_base->deinit();
    }
}

hal_result_t hal_manager_init(hal_platform_t platform) {
    // Check if already initialized
    if (g_hal_manager.initialized) {
        printf("[HAL_MANAGER] HAL manager already initialized\n");
        return HAL_ERROR_INVALID_STATE;
    }
    
    printf("[HAL_MANAGER] Initializing HAL manager for platform: %s\n", 
           hal_get_platform_name(platform));
    
    // Auto-detect platform if requested
    if (platform == HAL_PLATFORM_AUTO_DETECT) {
        platform = hal_detect_platform();
        printf("[HAL_MANAGER] Auto-detected platform: %s\n", 
               hal_get_platform_name(platform));
    }
    
    // Validate platform
    if (platform >= HAL_PLATFORM_AUTO_DETECT) {
        printf("[HAL_MANAGER] Invalid platform: %d\n", platform);
        return HAL_ERROR_INVALID_PARAM;
    }
    
    // Select HAL implementations based on platform
    hal_result_t result = HAL_SUCCESS;
    
    switch (platform) {
        case HAL_PLATFORM_MOCK:
            printf("[HAL_MANAGER] Using Mock HAL implementations\n");
            g_hal_manager.usb_hid = &mock_usb_hid_hal;
            g_hal_manager.crypto = &mock_crypto_hal;
            g_hal_manager.storage = &mock_storage_hal;
            break;
            
#ifdef HAL_STM32_ENABLED
        case HAL_PLATFORM_STM32:
            printf("[HAL_MANAGER] Using STM32 HAL implementations\n");
            g_hal_manager.usb_hid = &stm32_usb_hid_hal;
            g_hal_manager.crypto = &stm32_crypto_hal;
            g_hal_manager.storage = &stm32_storage_hal;
            break;
#endif

#ifdef HAL_ESP32_ENABLED
        case HAL_PLATFORM_ESP32:
            printf("[HAL_MANAGER] Using ESP32 HAL implementations\n");
            g_hal_manager.usb_hid = &esp32_usb_hid_hal;
            g_hal_manager.crypto = &esp32_crypto_hal;
            g_hal_manager.storage = &esp32_storage_hal;
            break;
#endif
            
        default:
            printf("[HAL_MANAGER] Platform %s not supported in this build\n", 
                   hal_get_platform_name(platform));
            return HAL_ERROR_NOT_SUPPORTED;
    }
    
    // Initialize all HAL modules in dependency order
    
    // 1. Initialize Storage HAL first (needed for crypto key storage)
    result = init_hal_module("Storage", &g_hal_manager.storage->base);
    if (result != HAL_SUCCESS) {
        goto cleanup_and_exit;
    }
    
    // 2. Initialize Crypto HAL (depends on storage for key management)
    result = init_hal_module("Crypto", &g_hal_manager.crypto->base);
    if (result != HAL_SUCCESS) {
        deinit_hal_module("Storage", &g_hal_manager.storage->base);
        goto cleanup_and_exit;
    }
    
    // 3. Initialize USB HID HAL last (depends on crypto for authentication)
    result = init_hal_module("USB HID", &g_hal_manager.usb_hid->base);
    if (result != HAL_SUCCESS) {
        deinit_hal_module("Crypto", &g_hal_manager.crypto->base);
        deinit_hal_module("Storage", &g_hal_manager.storage->base);
        goto cleanup_and_exit;
    }
    
    // Mark as initialized
    g_hal_manager.platform = platform;
    g_hal_manager.initialized = true;
    
    printf("[HAL_MANAGER] All HAL modules initialized successfully for %s platform\n",
           hal_get_platform_name(platform));
    
    return HAL_SUCCESS;

cleanup_and_exit:
    // Clear the manager state on failure
    memset(&g_hal_manager, 0, sizeof(g_hal_manager));
    printf("[HAL_MANAGER] HAL manager initialization failed\n");
    return result;
}

hal_result_t hal_manager_deinit(void) {
    if (!g_hal_manager.initialized) {
        printf("[HAL_MANAGER] HAL manager not initialized\n");
        return HAL_ERROR_NOT_INITIALIZED;
    }
    
    printf("[HAL_MANAGER] Deinitializing HAL manager (platform: %s)\n",
           hal_get_platform_name(g_hal_manager.platform));
    
    // Deinitialize in reverse order of initialization
    deinit_hal_module("USB HID", &g_hal_manager.usb_hid->base);
    deinit_hal_module("Crypto", &g_hal_manager.crypto->base);
    deinit_hal_module("Storage", &g_hal_manager.storage->base);
    
    // Clear the manager state
    memset(&g_hal_manager, 0, sizeof(g_hal_manager));
    
    printf("[HAL_MANAGER] HAL manager deinitialized successfully\n");
    return HAL_SUCCESS;
}

hal_manager_t* hal_manager_get_instance(void) {
    return g_hal_manager.initialized ? &g_hal_manager : NULL;
}

usb_hid_hal_t* hal_get_usb_hid(void) {
    if (!g_hal_manager.initialized) {
        printf("[HAL_MANAGER] HAL manager not initialized - cannot get USB HID HAL\n");
        return NULL;
    }
    return g_hal_manager.usb_hid;
}

crypto_hal_t* hal_get_crypto(void) {
    if (!g_hal_manager.initialized) {
        printf("[HAL_MANAGER] HAL manager not initialized - cannot get Crypto HAL\n");
        return NULL;
    }
    return g_hal_manager.crypto;
}

storage_hal_t* hal_get_storage(void) {
    if (!g_hal_manager.initialized) {
        printf("[HAL_MANAGER] HAL manager not initialized - cannot get Storage HAL\n");
        return NULL;
    }
    return g_hal_manager.storage;
}

hal_platform_t hal_detect_platform(void) {
    // Platform detection logic based on compile-time definitions
    // In a real implementation, this might also probe hardware registers
    
    printf("[HAL_MANAGER] Detecting hardware platform...\n");
    
#if defined(STM32F4) || defined(STM32F7) || defined(STM32H7) || defined(STM32L4)
    printf("[HAL_MANAGER] STM32 platform detected from compile-time definitions\n");
    return HAL_PLATFORM_STM32;
    
#elif defined(ESP32) || defined(ESP32S2) || defined(ESP32S3) || defined(ESP32C3)
    printf("[HAL_MANAGER] ESP32 platform detected from compile-time definitions\n");
    return HAL_PLATFORM_ESP32;
    
#else
    printf("[HAL_MANAGER] No specific platform detected, defaulting to Mock\n");
    return HAL_PLATFORM_MOCK;
#endif
}

const char* hal_get_platform_name(hal_platform_t platform) {
    switch (platform) {
        case HAL_PLATFORM_MOCK:         return "Mock";
        case HAL_PLATFORM_STM32:        return "STM32";
        case HAL_PLATFORM_ESP32:        return "ESP32";
        case HAL_PLATFORM_AUTO_DETECT:  return "Auto-Detect";
        default:                        return "Unknown";
    }
}