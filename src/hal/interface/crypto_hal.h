#ifndef CRYPTO_HAL_H
#define CRYPTO_HAL_H

/**
 * @file crypto_hal.h
 * @brief Cryptographic Hardware Abstraction Layer Interface
 * @author USB Key Authentication Team
 * @date 2025-09-07
 * @version 1.0
 * 
 * This file defines the cryptographic HAL interface for hardware and software
 * cryptographic operations. It provides a unified API for key generation,
 * signing, hashing, and other cryptographic primitives used in FIDO2/WebAuthn.
 */

#include "hal_common.h"

/**
 * @brief Cryptographic algorithm types
 * 
 * Defines the supported cryptographic algorithms for key generation
 * and cryptographic operations.
 */
typedef enum {
    CRYPTO_ALG_AES_128 = 0,     /**< AES-128 symmetric encryption */
    CRYPTO_ALG_AES_256,         /**< AES-256 symmetric encryption */
    CRYPTO_ALG_ECC_P256,        /**< ECDSA P-256 (secp256r1) */
    CRYPTO_ALG_ECC_P384,        /**< ECDSA P-384 (secp384r1) */
    CRYPTO_ALG_ECC_P521,        /**< ECDSA P-521 (secp521r1) */
    CRYPTO_ALG_ED25519,         /**< EdDSA Ed25519 */
    CRYPTO_ALG_RSA_2048,        /**< RSA-2048 */
    CRYPTO_ALG_RSA_4096         /**< RSA-4096 */
} crypto_algorithm_t;

/**
 * @brief Hash algorithm types
 * 
 * Defines the supported hash algorithms for cryptographic operations.
 */
typedef enum {
    CRYPTO_HASH_SHA1 = 0,       /**< SHA-1 (160-bit) - deprecated */
    CRYPTO_HASH_SHA224,         /**< SHA-224 (224-bit) */
    CRYPTO_HASH_SHA256,         /**< SHA-256 (256-bit) */
    CRYPTO_HASH_SHA384,         /**< SHA-384 (384-bit) */
    CRYPTO_HASH_SHA512          /**< SHA-512 (512-bit) */
} crypto_hash_algorithm_t;

/**
 * @brief Cryptographic key types
 * 
 * Defines the types of cryptographic keys supported by the system.
 */
typedef enum {
    CRYPTO_KEY_TYPE_SYMMETRIC = 0,  /**< Symmetric key (AES) */
    CRYPTO_KEY_TYPE_ECC_PRIVATE,    /**< ECC private key */
    CRYPTO_KEY_TYPE_ECC_PUBLIC,     /**< ECC public key */
    CRYPTO_KEY_TYPE_RSA_PRIVATE,    /**< RSA private key */
    CRYPTO_KEY_TYPE_RSA_PUBLIC      /**< RSA public key */
} crypto_key_type_t;

/** @brief Maximum key size in bytes */
#define CRYPTO_MAX_KEY_SIZE         512

/** @brief Maximum signature size in bytes */
#define CRYPTO_MAX_SIGNATURE_SIZE   512

/** @brief Maximum hash size in bytes (SHA-512) */
#define CRYPTO_MAX_HASH_SIZE        64

/**
 * @brief Cryptographic key structure
 * 
 * Represents a cryptographic key with its metadata.
 */
typedef struct {
    crypto_key_type_t type;     /**< Key type */
    crypto_algorithm_t algorithm; /**< Algorithm this key is used with */
    uint8_t* data;              /**< Key data buffer */
    size_t size;                /**< Size of key data in bytes */
    uint32_t flags;             /**< Key flags (CRYPTO_KEY_FLAG_*) */
} crypto_key_t;

/** @brief Key can be exported from secure storage */
#define CRYPTO_KEY_FLAG_EXPORTABLE  0x01

/** @brief Key is stored in hardware security module */
#define CRYPTO_KEY_FLAG_HARDWARE    0x02

/** @brief Key survives power cycles */
#define CRYPTO_KEY_FLAG_PERSISTENT  0x04

/**
 * @brief Cryptographic operation context
 * 
 * Maintains state for multi-step cryptographic operations like streaming hash.
 */
typedef struct {
    crypto_algorithm_t algorithm;   /**< Algorithm being used */
    void* context_data;             /**< Algorithm-specific context */
    size_t context_size;            /**< Size of context data */
    uint32_t flags;                 /**< Operation flags */
} crypto_context_t;

/**
 * @brief Random number generation interface
 * 
 * Provides access to hardware and software random number generators.
 */
typedef struct {
    /**
     * @brief Generate random bytes
     * 
     * Generates cryptographically secure random bytes.
     * 
     * @param buffer Buffer to store random data
     * @param length Number of random bytes to generate
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Random data generated successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_HARDWARE_FAILURE RNG hardware failure
     * 
     * @note Generated data is cryptographically secure
     * @warning Do not use for production crypto without proper entropy
     */
    hal_result_t (*generate_random)(uint8_t* buffer, size_t length);
    
    /**
     * @brief Add entropy to random number generator
     * 
     * Adds external entropy to improve randomness quality.
     * 
     * @param data Entropy data to add
     * @param length Size of entropy data
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Entropy added successfully
     * @retval HAL_ERROR_NOT_SUPPORTED Entropy addition not supported
     */
    hal_result_t (*add_entropy)(const uint8_t* data, size_t length);
    
    /**
     * @brief Get entropy estimate
     * 
     * Returns an estimate of available entropy in bits.
     * 
     * @param bits Pointer to store entropy estimate
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Entropy estimate retrieved
     * @retval HAL_ERROR_NOT_SUPPORTED Entropy estimation not supported
     */
    hal_result_t (*get_entropy_estimate)(uint32_t* bits);
} crypto_rng_t;

/**
 * @brief Cryptographic HAL interface
 * 
 * This structure defines the complete cryptographic HAL API including
 * key management, signing, hashing, and random number generation.
 */
typedef struct {
    hal_base_t base;    /**< Base HAL interface */
    
    /**
     * @brief Generate cryptographic key pair
     * 
     * Generates a new public/private key pair using the specified algorithm.
     * 
     * @param algorithm Cryptographic algorithm to use
     * @param public_key Pointer to store generated public key
     * @param private_key Pointer to store generated private key
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Key pair generated successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid algorithm or NULL pointers
     * @retval HAL_ERROR_NOT_SUPPORTED Algorithm not supported
     * @retval HAL_ERROR_HARDWARE_FAILURE Crypto hardware failure
     * @retval HAL_ERROR_INSUFFICIENT_MEMORY Memory allocation failed
     * 
     * @note Generated keys must be freed with delete_key()
     * @see delete_key()
     */
    hal_result_t (*generate_key_pair)(crypto_algorithm_t algorithm, 
                                     crypto_key_t* public_key, 
                                     crypto_key_t* private_key);
    
    /**
     * @brief Import cryptographic key
     * 
     * Imports a key from external data into the crypto system.
     * 
     * @param key_data Raw key data to import
     * @param key_size Size of key data in bytes
     * @param type Type of key being imported
     * @param key Pointer to store imported key
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Key imported successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_NOT_SUPPORTED Key type not supported
     * @retval HAL_ERROR_INSUFFICIENT_MEMORY Memory allocation failed
     * 
     * @note Key data format depends on key type and algorithm
     */
    hal_result_t (*import_key)(const uint8_t* key_data, size_t key_size, 
                              crypto_key_type_t type, crypto_key_t* key);
    
    /**
     * @brief Export cryptographic key
     * 
     * Exports a key to external buffer for storage or transmission.
     * 
     * @param key Key to export
     * @param buffer Buffer to store exported key data
     * @param buffer_size Pointer to buffer size (input) and actual size (output)
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Key exported successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_NOT_SUPPORTED Key not exportable
     * @retval HAL_ERROR_INSUFFICIENT_MEMORY Buffer too small
     * 
     * @note Only keys with CRYPTO_KEY_FLAG_EXPORTABLE can be exported
     * @note *buffer_size is updated with actual exported size
     */
    hal_result_t (*export_key)(const crypto_key_t* key, uint8_t* buffer, size_t* buffer_size);
    
    /**
     * @brief Delete cryptographic key
     * 
     * Securely deletes a key and frees associated resources.
     * 
     * @param key Key to delete
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Key deleted successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid key pointer
     * 
     * @note Key structure is zeroed after deletion
     * @warning Key cannot be used after deletion
     */
    hal_result_t (*delete_key)(crypto_key_t* key);
    
    /**
     * @brief Create digital signature
     * 
     * Signs data using the provided private key.
     * 
     * @param private_key Private key for signing
     * @param data Data to sign
     * @param data_length Size of data in bytes
     * @param signature Buffer to store signature
     * @param signature_length Pointer to signature buffer size (input) and actual size (output)
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Signature created successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_NOT_SUPPORTED Algorithm not supported for signing
     * @retval HAL_ERROR_INSUFFICIENT_MEMORY Signature buffer too small
     * 
     * @note Signature format depends on algorithm (DER for ECDSA, etc.)
     * @see verify()
     */
    hal_result_t (*sign)(const crypto_key_t* private_key, 
                        const uint8_t* data, size_t data_length,
                        uint8_t* signature, size_t* signature_length);
    
    /**
     * @brief Verify digital signature
     * 
     * Verifies a signature using the provided public key.
     * 
     * @param public_key Public key for verification
     * @param data Original data that was signed
     * @param data_length Size of data in bytes
     * @param signature Signature to verify
     * @param signature_length Size of signature in bytes
     * 
     * @return HAL_SUCCESS if signature is valid, error code otherwise
     * @retval HAL_SUCCESS Signature is valid
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_NOT_SUPPORTED Algorithm not supported for verification
     * @retval HAL_ERROR_HARDWARE_FAILURE Signature verification failed
     * 
     * @note HAL_SUCCESS means signature is cryptographically valid
     * @see sign()
     */
    hal_result_t (*verify)(const crypto_key_t* public_key,
                          const uint8_t* data, size_t data_length,
                          const uint8_t* signature, size_t signature_length);
    
    /**
     * @brief Compute hash of data (single-shot)
     * 
     * Computes a cryptographic hash of the provided data.
     * 
     * @param algorithm Hash algorithm to use
     * @param data Data to hash
     * @param data_length Size of data in bytes
     * @param hash Buffer to store computed hash
     * @param hash_length Pointer to hash buffer size (input) and actual size (output)
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Hash computed successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_NOT_SUPPORTED Hash algorithm not supported
     * @retval HAL_ERROR_INSUFFICIENT_MEMORY Hash buffer too small
     * 
     * @note For large data, use hash_init/update/finalize for better performance
     * @see hash_init(), hash_update(), hash_finalize()
     */
    hal_result_t (*hash)(crypto_hash_algorithm_t algorithm,
                        const uint8_t* data, size_t data_length,
                        uint8_t* hash, size_t* hash_length);
    
    /**
     * @brief Initialize streaming hash operation
     * 
     * Initializes a hash context for streaming hash computation.
     * 
     * @param algorithm Hash algorithm to use
     * @param context Hash context to initialize
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Hash context initialized
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_NOT_SUPPORTED Hash algorithm not supported
     * @retval HAL_ERROR_INSUFFICIENT_MEMORY Context allocation failed
     * 
     * @note Context must be finalized with hash_finalize()
     * @see hash_update(), hash_finalize()
     */
    hal_result_t (*hash_init)(crypto_hash_algorithm_t algorithm, crypto_context_t* context);
    
    /**
     * @brief Update streaming hash with data
     * 
     * Adds data to an ongoing hash computation.
     * 
     * @param context Hash context from hash_init()
     * @param data Data to add to hash
     * @param length Size of data in bytes
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Data added to hash successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_INVALID_STATE Context not initialized
     * 
     * @note Can be called multiple times to hash large amounts of data
     * @see hash_init(), hash_finalize()
     */
    hal_result_t (*hash_update)(crypto_context_t* context, const uint8_t* data, size_t length);
    
    /**
     * @brief Finalize streaming hash operation
     * 
     * Completes hash computation and retrieves the final hash value.
     * 
     * @param context Hash context from hash_init()
     * @param hash Buffer to store computed hash
     * @param hash_length Pointer to hash buffer size (input) and actual size (output)
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Hash computed successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_INVALID_STATE Context not initialized
     * @retval HAL_ERROR_INSUFFICIENT_MEMORY Hash buffer too small
     * 
     * @note Context is invalidated after finalization
     * @see hash_init(), hash_update()
     */
    hal_result_t (*hash_finalize)(crypto_context_t* context, uint8_t* hash, size_t* hash_length);
    
    /**
     * @brief Encrypt data
     * 
     * Encrypts plaintext using the provided key.
     * 
     * @param key Encryption key (symmetric or public key)
     * @param plaintext Data to encrypt
     * @param plaintext_length Size of plaintext in bytes
     * @param ciphertext Buffer to store encrypted data
     * @param ciphertext_length Pointer to ciphertext buffer size (input) and actual size (output)
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Data encrypted successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_NOT_SUPPORTED Algorithm not supported for encryption
     * @retval HAL_ERROR_INSUFFICIENT_MEMORY Ciphertext buffer too small
     * 
     * @note Encryption mode and padding depend on algorithm
     * @see decrypt()
     */
    hal_result_t (*encrypt)(const crypto_key_t* key, const uint8_t* plaintext, size_t plaintext_length,
                           uint8_t* ciphertext, size_t* ciphertext_length);
    
    /**
     * @brief Decrypt data
     * 
     * Decrypts ciphertext using the provided key.
     * 
     * @param key Decryption key (symmetric or private key)
     * @param ciphertext Data to decrypt
     * @param ciphertext_length Size of ciphertext in bytes
     * @param plaintext Buffer to store decrypted data
     * @param plaintext_length Pointer to plaintext buffer size (input) and actual size (output)
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Data decrypted successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_NOT_SUPPORTED Algorithm not supported for decryption
     * @retval HAL_ERROR_INSUFFICIENT_MEMORY Plaintext buffer too small
     * @retval HAL_ERROR_HARDWARE_FAILURE Decryption failed (wrong key, corrupted data)
     * 
     * @see encrypt()
     */
    hal_result_t (*decrypt)(const crypto_key_t* key, const uint8_t* ciphertext, size_t ciphertext_length,
                           uint8_t* plaintext, size_t* plaintext_length);
    
    /** @brief Random number generation interface */
    crypto_rng_t rng;
    
    /**
     * @brief Get cryptographic capabilities
     * 
     * Returns a bitmask of supported cryptographic features.
     * 
     * @param capabilities Pointer to store capability flags
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Capabilities retrieved successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid capabilities pointer
     * 
     * @see CRYPTO_CAP_HW_AES, CRYPTO_CAP_HW_ECC, etc.
     */
    hal_result_t (*get_capabilities)(uint32_t* capabilities);
    
    /**
     * @brief Get unique device identifier
     * 
     * Retrieves a unique identifier for the cryptographic device.
     * 
     * @param device_id Buffer to store device ID
     * @param id_length Pointer to buffer size (input) and actual ID length (output)
     * 
     * @return HAL_SUCCESS on success, error code otherwise
     * @retval HAL_SUCCESS Device ID retrieved successfully
     * @retval HAL_ERROR_INVALID_PARAM Invalid parameters
     * @retval HAL_ERROR_NOT_SUPPORTED Device ID not available
     * @retval HAL_ERROR_INSUFFICIENT_MEMORY Buffer too small
     * 
     * @note Device ID is typically used for key derivation or attestation
     */
    hal_result_t (*get_device_id)(uint8_t* device_id, size_t* id_length);
    
} crypto_hal_t;

/** @brief Hardware AES acceleration available */
#define CRYPTO_CAP_HW_AES           0x0001

/** @brief Hardware ECC acceleration available */
#define CRYPTO_CAP_HW_ECC           0x0002

/** @brief Hardware RSA acceleration available */
#define CRYPTO_CAP_HW_RSA           0x0004

/** @brief Hardware random number generator available */
#define CRYPTO_CAP_HW_RNG           0x0008

/** @brief Hardware hash acceleration available */
#define CRYPTO_CAP_HW_HASH          0x0010

/** @brief Secure key storage available */
#define CRYPTO_CAP_SECURE_STORAGE   0x0020

/** @brief Key derivation functions available */
#define CRYPTO_CAP_KEY_DERIVATION   0x0040

#endif // CRYPTO_HAL_H