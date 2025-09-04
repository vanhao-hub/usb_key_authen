# API Specification

Tài liệu này tổng hợp tất cả các định nghĩa về hàm, struct, enum, interface liên quan đến framework.

## 1. Hardware Abstraction Layer (HAL)
```c
// Core interface definitions
typedef struct {
    int (*init)(void);
    int (*deinit)(void);
    int (*reset)(void);
} hal_base_t;

// USB HID HAL
typedef struct {
    hal_base_t base;
    int (*send_report)(uint8_t endpoint, uint8_t* data, size_t len);
    int (*receive_report)(uint8_t endpoint, uint8_t* buffer, size_t* len);
    int (*set_descriptor)(usb_descriptor_t* desc);
} usb_hid_hal_t;

// Crypto HAL  
typedef struct {
    hal_base_t base;
    int (*generate_key_pair)(crypto_key_type_t type, crypto_key_t* public_key, crypto_key_t* private_key);
    int (*sign)(crypto_key_t* private_key, uint8_t* data, size_t data_len, uint8_t* signature, size_t* sig_len);
    int (*verify)(crypto_key_t* public_key, uint8_t* data, size_t data_len, uint8_t* signature, size_t sig_len);
    int (*random_bytes)(uint8_t* buffer, size_t len);
    int (*hash)(crypto_hash_type_t type, uint8_t* data, size_t len, uint8_t* hash);
} crypto_hal_t;

// Storage HAL
typedef struct {
    hal_base_t base;
    int (*read)(uint32_t address, uint8_t* buffer, size_t len);
    int (*write)(uint32_t address, uint8_t* data, size_t len);
    int (*erase)(uint32_t address, size_t len);
    int (*get_size)(void);
} storage_hal_t;
```

## 2. Platform Layer: Protocol & Transport
```c
// Protocol Layer interface
typedef struct {
    int (*init)(void);
    int (*process_command)(uint8_t* cmd, size_t cmd_len, uint8_t* response, size_t* resp_len);
    int (*get_info)(protocol_info_t* info);
} protocol_interface_t;

// CTAP2 Protocol
typedef struct {
    protocol_interface_t base;
    int (*make_credential)(ctap2_make_credential_req_t* req, ctap2_make_credential_resp_t* resp);
    int (*get_assertion)(ctap2_get_assertion_req_t* req, ctap2_get_assertion_resp_t* resp);
    int (*get_info)(ctap2_info_t* info);
    int (*client_pin)(ctap2_client_pin_req_t* req, ctap2_client_pin_resp_t* resp);
} ctap2_protocol_t;

// Transport Layer interface
typedef struct {
    int (*init)(void);
    int (*send)(uint8_t* data, size_t len);
    int (*receive)(uint8_t* buffer, size_t* len);
    int (*set_transport_config)(void* config);
} transport_interface_t;
```

## 3. Application Layer
```c
// FIDO2 Application
typedef struct {
    int (*authenticator_make_credential)(webauthn_make_credential_req_t* req, webauthn_credential_t* cred);
    int (*authenticator_get_assertion)(webauthn_get_assertion_req_t* req, webauthn_assertion_t* assertion);
    int (*authenticator_get_info)(webauthn_authenticator_info_t* info);
    int (*set_pin)(uint8_t* pin, size_t pin_len);
    int (*verify_pin)(uint8_t* pin, size_t pin_len);
} fido2_app_t;
```

## 4. Configuration & Plugin System
```c
// config.h - Compile-time feature selection
#define FEATURE_FIDO2_ENABLED       1
#define FEATURE_PIV_ENABLED         0  // Disable for prototype
#define FEATURE_OTP_ENABLED         0  // Disable for prototype
#define FEATURE_OPENPGP_ENABLED     0  // Disable for prototype

#define FEATURE_USER_VERIFICATION   1  // PIN support
#define FEATURE_RESIDENT_KEYS       1  // Discoverable credentials
#define FEATURE_CREDENTIAL_MGMT     1  // Credential management

#define MAX_RESIDENT_KEYS          25
#define MAX_CREDENTIAL_SIZE        1024

// Plugin registration system
typedef struct {
    char name[32];
    protocol_interface_t* protocol;
    void* context;
} protocol_plugin_t;

int register_protocol(protocol_plugin_t* plugin);
int unregister_protocol(const char* name);
protocol_interface_t* get_protocol(const char* name);
```

## 5. Threading & State Management
```c
// Event system for handling USB/user interactions
typedef enum {
    EVENT_USB_DATA_RECEIVED,
    EVENT_USER_PRESENCE,
    EVENT_TIMER_EXPIRED,
    EVENT_CREDENTIAL_REQUEST,
} event_type_t;

typedef struct {
    event_type_t type;
    void* data;
    size_t data_len;
    void (*callback)(void* context);
    void* context;
} event_t;

int event_queue_push(event_t* event);
int event_queue_process(void);

// Global authenticator state
typedef enum {
    STATE_IDLE,
    STATE_PROCESSING_COMMAND,
    STATE_WAITING_USER_PRESENCE,
    STATE_WAITING_PIN_ENTRY,
    STATE_ERROR
} authenticator_state_t;

typedef struct {
    authenticator_state_t current_state;
    uint32_t timeout_ms;
    void (*timeout_callback)(void);
} state_machine_t;
```

## 6. Security Architecture
```c
// Secure key storage
typedef struct {
    uint32_t key_id;
    crypto_key_type_t type;
    uint8_t usage_flags;
    uint8_t key_data[64];  // Encrypted/wrapped
    uint32_t creation_time;
} stored_key_t;

// Key derivation
int derive_attestation_key(uint8_t* master_secret, uint8_t* key_out);
int derive_credential_key(uint8_t* rp_id, uint8_t* user_id, uint8_t* key_out);

// PIN/User verification
typedef struct {
    uint8_t pin_hash[32];
    uint8_t pin_retries;
    bool pin_set;
    uint32_t consecutive_pin_failures;
} pin_state_t;

int verify_user_presence(void);
int verify_user_verification(uint8_t* pin, size_t pin_len);
```

## 7. Build System Configuration
```cmake
# Configurable build targets
option(TARGET_MCU "Target MCU family" "STM32F4")
option(ENABLE_FIDO2 "Enable FIDO2 support" ON)
option(ENABLE_PIV "Enable PIV support" OFF)
option(ENABLE_DEBUG "Enable debug features" OFF)

# Platform-specific HAL selection
if(TARGET_MCU STREQUAL "STM32F4")
    add_subdirectory(src/hal/stm32)
elseif(TARGET_MCU STREQUAL "ESP32")
    add_subdirectory(src/hal/esp32)
endif()
```
