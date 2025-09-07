/**
 * @file storage_platform.c
 * @brief Storage Platform Layer Implementation
 * @author USB Key Authentication Team
 * @date 2025-09-07
 * @version 1.0
 * 
 * This file implements the Storage Platform layer functionality including
 * region management, encryption, wear leveling, and file operations.
 */

#include "storage_platform.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief Storage platform state
 * 
 * Internal state structure for the storage platform.
 */
typedef struct {
    storage_hal_t* hal;                                    /**< Storage HAL reference */
    crypto_hal_t* crypto;                                  /**< Crypto HAL reference */
    bool initialized;                                      /**< Initialization status */
    storage_region_config_t regions[STORAGE_REGION_MAX];   /**< Region configurations */
    bool region_configured[STORAGE_REGION_MAX];           /**< Region setup status */
    uint32_t wear_level_counter;                          /**< Wear leveling counter */
    uint32_t gc_threshold;                                 /**< Garbage collection threshold */
} storage_platform_state_t;

/**
 * @brief Global storage platform instance
 */
static storage_platform_state_t g_storage_state = {0};

/**
 * @brief Storage platform interface instance
 */
static storage_platform_t g_storage_platform;

/**
 * @brief Calculate CRC32 for data integrity
 * 
 * Simple CRC32 implementation for data integrity checking.
 * 
 * @param data Data to calculate CRC for
 * @param length Length of data
 * @return CRC32 value
 */
static uint32_t calculate_crc32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    const uint32_t polynomial = 0xEDB88320;
    
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ polynomial;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return ~crc;
}

/**
 * @brief Encrypt data for storage
 * 
 * Encrypts data using the crypto HAL if available.
 * 
 * @param plaintext Input data
 * @param plaintext_len Length of input data
 * @param ciphertext Output buffer
 * @param ciphertext_len Pointer to output buffer size
 * @return HAL_SUCCESS on success, error code otherwise
 */
static hal_result_t encrypt_data(const uint8_t* plaintext, size_t plaintext_len,
                                uint8_t* ciphertext, size_t* ciphertext_len) {
    if (!g_storage_state.crypto) {
        // No crypto available, just copy data
        if (*ciphertext_len < plaintext_len) {
            return HAL_ERROR_INSUFFICIENT_MEMORY;
        }
        memcpy(ciphertext, plaintext, plaintext_len);
        *ciphertext_len = plaintext_len;
        return HAL_SUCCESS;
    }
    
    // TODO: Implement encryption using crypto HAL
    // For now, just copy data (placeholder)
    if (*ciphertext_len < plaintext_len) {
        return HAL_ERROR_INSUFFICIENT_MEMORY;
    }
    memcpy(ciphertext, plaintext, plaintext_len);
    *ciphertext_len = plaintext_len;
    
    return HAL_SUCCESS;
}

/**
 * @brief Decrypt data from storage
 * 
 * Decrypts data using the crypto HAL if available.
 * 
 * @param ciphertext Input encrypted data
 * @param ciphertext_len Length of encrypted data
 * @param plaintext Output buffer
 * @param plaintext_len Pointer to output buffer size
 * @return HAL_SUCCESS on success, error code otherwise
 */
static hal_result_t decrypt_data(const uint8_t* ciphertext, size_t ciphertext_len,
                                uint8_t* plaintext, size_t* plaintext_len) {
    if (!g_storage_state.crypto) {
        // No crypto available, just copy data
        if (*plaintext_len < ciphertext_len) {
            return HAL_ERROR_INSUFFICIENT_MEMORY;
        }
        memcpy(plaintext, ciphertext, ciphertext_len);
        *plaintext_len = ciphertext_len;
        return HAL_SUCCESS;
    }
    
    // TODO: Implement decryption using crypto HAL
    // For now, just copy data (placeholder)
    if (*plaintext_len < ciphertext_len) {
        return HAL_ERROR_INSUFFICIENT_MEMORY;
    }
    memcpy(plaintext, ciphertext, ciphertext_len);
    *plaintext_len = ciphertext_len;
    
    return HAL_SUCCESS;
}

/**
 * @brief Validate region parameters
 * 
 * Validates that region configuration is valid.
 * 
 * @param region Region ID
 * @param config Region configuration
 * @return true if valid, false otherwise
 */
static bool validate_region_config(storage_region_t region, const storage_region_config_t* config) {
    if (region >= STORAGE_REGION_MAX || !config) {
        return false;
    }
    
    if (config->size == 0) {
        return false;
    }
    
    // Get storage info to validate addresses
    storage_info_t storage_info;
    if (g_storage_state.hal->get_info(&storage_info) != HAL_SUCCESS) {
        return false;
    }
    
    // Check if region fits in storage
    if (config->base_address + config->size > storage_info.total_size) {
        return false;
    }
    
    // Check backup address if specified
    if (config->backup_address != 0) {
        if (config->backup_address + config->size > storage_info.total_size) {
            return false;
        }
    }
    
    return true;
}

// Implementation of platform interface functions

static hal_result_t storage_platform_init(storage_hal_t* storage_hal, crypto_hal_t* crypto_hal) {
    if (g_storage_state.initialized) {
        printf("[STORAGE_PLATFORM] Already initialized\n");
        return HAL_ERROR_INVALID_STATE;
    }
    
    if (!storage_hal) {
        printf("[STORAGE_PLATFORM] Storage HAL is required\n");
        return HAL_ERROR_INVALID_PARAM;
    }
    
    printf("[STORAGE_PLATFORM] Initializing storage platform\n");
    
    // Store HAL references
    g_storage_state.hal = storage_hal;
    g_storage_state.crypto = crypto_hal;  // Optional
    
    // Initialize state
    memset(g_storage_state.regions, 0, sizeof(g_storage_state.regions));
    memset(g_storage_state.region_configured, 0, sizeof(g_storage_state.region_configured));
    g_storage_state.wear_level_counter = 0;
    g_storage_state.gc_threshold = 75;  // 75% usage triggers GC
    
    // Verify storage HAL is working
    storage_info_t info;
    hal_result_t result = storage_hal->get_info(&info);
    if (result != HAL_SUCCESS) {
        printf("[STORAGE_PLATFORM] Storage HAL get_info failed: %d\n", result);
        return result;
    }
    
    printf("[STORAGE_PLATFORM] Storage device: %u bytes, sector: %u, page: %u\n",
           info.total_size, info.sector_size, info.page_size);
    
    g_storage_state.initialized = true;
    
    printf("[STORAGE_PLATFORM] Storage platform initialized successfully\n");
    return HAL_SUCCESS;
}

static hal_result_t storage_platform_deinit(void) {
    if (!g_storage_state.initialized) {
        return HAL_ERROR_NOT_INITIALIZED;
    }
    
    printf("[STORAGE_PLATFORM] Deinitializing storage platform\n");
    
    // Flush any pending operations
    if (g_storage_state.hal && g_storage_state.hal->flush) {
        g_storage_state.hal->flush();
    }
    
    // Clear state
    memset(&g_storage_state, 0, sizeof(g_storage_state));
    
    printf("[STORAGE_PLATFORM] Storage platform deinitialized\n");
    return HAL_SUCCESS;
}

static hal_result_t storage_platform_configure_region(storage_region_t region, 
                                                     const storage_region_config_t* config) {
    if (!g_storage_state.initialized) {
        return HAL_ERROR_NOT_INITIALIZED;
    }
    
    if (!validate_region_config(region, config)) {
        printf("[STORAGE_PLATFORM] Invalid region configuration\n");
        return HAL_ERROR_INVALID_PARAM;
    }
    
    printf("[STORAGE_PLATFORM] Configuring region %d: addr=0x%08X, size=%u\n",
           region, config->base_address, config->size);
    
    // Store region configuration
    g_storage_state.regions[region] = *config;
    g_storage_state.region_configured[region] = true;
    
    // Initialize region if needed (erase to prepare for use)
    if (config->flags & STORAGE_FLAG_PERSISTENT) {
        // For persistent regions, we might want to preserve existing data
        // Just mark as configured
    } else {
        // For non-persistent regions, erase to ensure clean state
        hal_result_t result = g_storage_state.hal->erase(config->base_address, config->size);
        if (result != HAL_SUCCESS) {
            printf("[STORAGE_PLATFORM] Failed to erase region %d: %d\n", region, result);
            g_storage_state.region_configured[region] = false;
            return result;
        }
    }
    
    printf("[STORAGE_PLATFORM] Region %d configured successfully\n", region);
    return HAL_SUCCESS;
}

static hal_result_t storage_platform_get_region_info(storage_region_t region, 
                                                    storage_region_info_t* info) {
    if (!g_storage_state.initialized) {
        return HAL_ERROR_NOT_INITIALIZED;
    }
    
    if (region >= STORAGE_REGION_MAX || !info) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    if (!g_storage_state.region_configured[region]) {
        return HAL_ERROR_INVALID_STATE;
    }
    
    // Fill in region info
    info->config = g_storage_state.regions[region];
    
    // TODO: Calculate actual used/free space by scanning region
    // For now, provide placeholder values
    info->used_size = 0;
    info->free_size = info->config.size;
    info->write_count = 0;
    info->error_count = 0;
    info->is_healthy = true;
    
    return HAL_SUCCESS;
}

static hal_result_t storage_platform_read_region(storage_region_t region, uint32_t offset,
                                                uint8_t* buffer, size_t length) {
    if (!g_storage_state.initialized) {
        return HAL_ERROR_NOT_INITIALIZED;
    }
    
    if (region >= STORAGE_REGION_MAX || !buffer) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    if (!g_storage_state.region_configured[region]) {
        return HAL_ERROR_INVALID_STATE;
    }
    
    storage_region_config_t* config = &g_storage_state.regions[region];
    
    // Check bounds
    if (offset + length > config->size) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    uint32_t physical_address = config->base_address + offset;
    
    // Read from storage
    hal_result_t result = g_storage_state.hal->read(physical_address, buffer, length);
    if (result != HAL_SUCCESS) {
        printf("[STORAGE_PLATFORM] Read failed from region %d: %d\n", region, result);
        return result;
    }
    
    // Decrypt if encrypted
    if (config->flags & STORAGE_FLAG_ENCRYPTED) {
        uint8_t* decrypted_buffer = malloc(length);
        if (!decrypted_buffer) {
            return HAL_ERROR_INSUFFICIENT_MEMORY;
        }
        
        size_t decrypted_length = length;
        result = decrypt_data(buffer, length, decrypted_buffer, &decrypted_length);
        
        if (result == HAL_SUCCESS) {
            memcpy(buffer, decrypted_buffer, decrypted_length);
        }
        
        free(decrypted_buffer);
        
        if (result != HAL_SUCCESS) {
            printf("[STORAGE_PLATFORM] Decryption failed for region %d: %d\n", region, result);
            return result;
        }
    }
    
    // TODO: Verify integrity if authenticated
    
    return HAL_SUCCESS;
}

static hal_result_t storage_platform_write_region(storage_region_t region, uint32_t offset,
                                                 const uint8_t* data, size_t length) {
    if (!g_storage_state.initialized) {
        return HAL_ERROR_NOT_INITIALIZED;
    }
    
    if (region >= STORAGE_REGION_MAX || !data) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    if (!g_storage_state.region_configured[region]) {
        return HAL_ERROR_INVALID_STATE;
    }
    
    storage_region_config_t* config = &g_storage_state.regions[region];
    
    // Check bounds
    if (offset + length > config->size) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    uint32_t physical_address = config->base_address + offset;
    const uint8_t* write_data = data;
    size_t write_length = length;
    
    uint8_t* encrypted_buffer = NULL;
    
    // Encrypt if needed
    if (config->flags & STORAGE_FLAG_ENCRYPTED) {
        encrypted_buffer = malloc(length + 16);  // Extra space for encryption padding
        if (!encrypted_buffer) {
            return HAL_ERROR_INSUFFICIENT_MEMORY;
        }
        
        size_t encrypted_length = length + 16;
        hal_result_t result = encrypt_data(data, length, encrypted_buffer, &encrypted_length);
        
        if (result != HAL_SUCCESS) {
            free(encrypted_buffer);
            printf("[STORAGE_PLATFORM] Encryption failed for region %d: %d\n", region, result);
            return result;
        }
        
        write_data = encrypted_buffer;
        write_length = encrypted_length;
    }
    
    // TODO: Add integrity protection if authenticated
    
    // Write to storage
    hal_result_t result = g_storage_state.hal->write(physical_address, write_data, write_length);
    
    if (encrypted_buffer) {
        free(encrypted_buffer);
    }
    
    if (result != HAL_SUCCESS) {
        printf("[STORAGE_PLATFORM] Write failed to region %d: %d\n", region, result);
        return result;
    }
    
    // Flush if atomic
    if (config->flags & STORAGE_FLAG_ATOMIC) {
        g_storage_state.hal->flush();
    }
    
    // Update wear leveling counter
    g_storage_state.wear_level_counter++;
    
    return HAL_SUCCESS;
}

static hal_result_t storage_platform_erase_region(storage_region_t region) {
    if (!g_storage_state.initialized) {
        return HAL_ERROR_NOT_INITIALIZED;
    }
    
    if (region >= STORAGE_REGION_MAX) {
        return HAL_ERROR_INVALID_PARAM;
    }
    
    if (!g_storage_state.region_configured[region]) {
        return HAL_ERROR_INVALID_STATE;
    }
    
    storage_region_config_t* config = &g_storage_state.regions[region];
    
    printf("[STORAGE_PLATFORM] Erasing region %d\n", region);
    
    hal_result_t result = g_storage_state.hal->erase(config->base_address, config->size);
    if (result != HAL_SUCCESS) {
        printf("[STORAGE_PLATFORM] Erase failed for region %d: %d\n", region, result);
    }
    
    return result;
}

// Initialize the platform interface structure
void storage_platform_init_interface(void) {
    g_storage_platform.hal = NULL;
    g_storage_platform.crypto = NULL;
    g_storage_platform.initialized = false;
    
    // Set function pointers
    g_storage_platform.init = storage_platform_init;
    g_storage_platform.deinit = storage_platform_deinit;
    g_storage_platform.configure_region = storage_platform_configure_region;
    g_storage_platform.get_region_info = storage_platform_get_region_info;
    g_storage_platform.read_region = storage_platform_read_region;
    g_storage_platform.write_region = storage_platform_write_region;
    g_storage_platform.erase_region = storage_platform_erase_region;
    
    // TODO: Implement remaining functions (file operations, GC, wear leveling, etc.)
}

/**
 * @brief Get storage platform instance
 * 
 * Returns the global storage platform instance.
 * 
 * @return Pointer to storage platform instance
 */
storage_platform_t* get_storage_platform(void) {
    return &g_storage_platform;
}